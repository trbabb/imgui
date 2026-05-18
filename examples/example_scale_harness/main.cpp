// Scale harness — GLFW + OpenGL 3 binary used for developing PushView/PopView.
//
// Two modes:
//   (default)    Interactive window for hands-on inspection. Renders the
//                widget zoo (see widget_zoo.cpp) and runs until the window
//                closes. Use this for manual testing of input behavior.
//   --headless   Renders the zoo into an offscreen FBO of the requested size,
//                reads it back, and writes a PNG via libpng. Uses a hidden
//                GLFW window only as a GL context provider. Use this for
//                reproducible visual diffs across machines.
//
// Usage:
//   example_scale_harness                                # interactive
//   example_scale_harness --headless [-o out.png]        # headless to PNG
//                              [-W width] [-H height]
//                              [--frames N]
//
// --frames N runs N NewFrame/Render cycles before the readback. Some imgui
// state (e.g. auto-fitting, font atlas upload, the first-frame ItemAdd
// settling) only resolves on later frames, so the default is 2.

#include "widget_zoo.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#define GL_SILENCE_DEPRECATION
#if defined(__APPLE__)
// Pull in GL 3.2 core symbols (FBO entry points) directly. The imgui OpenGL3
// backend uses its own loader internally so its symbols aren't reachable from
// here; on macOS the core-profile FBO functions are available as direct
// framework symbols and that's the simplest way to get them.
#include <OpenGL/gl3.h>
#endif
#include <GLFW/glfw3.h>

#include <png.h>

// ---------------------------------------------------------------------------
// PNG writer
// ---------------------------------------------------------------------------

// Write an RGBA8 buffer to a PNG file. `pixels` is row-major, top-to-bottom,
// `width*4` bytes per row. Returns true on success.
//
// Used by the headless mode after flipping the GL readback (which is
// bottom-to-top) into top-down order.
static bool WritePNG_RGBA8(const char* path, int width, int height, const unsigned char* pixels)
{
    FILE* fp = std::fopen(path, "wb");
    if (!fp) {
        std::fprintf(stderr, "scale_harness: cannot open %s for writing\n", path);
        return false;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) { std::fclose(fp); return false; }
    png_infop info = png_create_info_struct(png);
    if (!info) { png_destroy_write_struct(&png, nullptr); std::fclose(fp); return false; }

    // libpng's error path is setjmp-based. We don't allocate anything below
    // that needs cleanup outside the destroy call, so the longjmp landing
    // pad just tears down and bails.
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        std::fclose(fp);
        return false;
    }

    png_init_io(png, fp);
    png_set_IHDR(png, info,
                 (png_uint_32)width, (png_uint_32)height,
                 8, PNG_COLOR_TYPE_RGBA,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    std::vector<png_bytep> rows((size_t)height);
    for (int y = 0; y < height; y++)
        rows[(size_t)y] = (png_bytep)(pixels + (size_t)y * (size_t)width * 4);
    png_write_image(png, rows.data());
    png_write_end(png, nullptr);

    png_destroy_write_struct(&png, &info);
    std::fclose(fp);
    return true;
}

// ---------------------------------------------------------------------------
// GLFW / GL setup shared between modes
// ---------------------------------------------------------------------------

static void glfw_error_callback(int error, const char* description)
{
    std::fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Pick the GL+GLSL version to request. Mirrors example_glfw_opengl3 so the
// harness uses exactly the same backend init path as the canonical example.
static const char* SetupGLVersionHints()
{
#if defined(__APPLE__)
    // GL 3.2 core; Apple requires forward-compat.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    return "#version 150";
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    return "#version 130";
#endif
}

// ---------------------------------------------------------------------------
// Interactive mode
// ---------------------------------------------------------------------------

static int RunInteractive()
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    const char* glsl_version = SetupGLVersionHints();

    GLFWwindow* window = glfwCreateWindow(1280, 800, "Scale Harness", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = nullptr; // no persistence — keeps runs reproducible

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Live-editable view transform applied to the widget zoo. The control
    // panel itself is drawn OUTSIDE the PushView scope so the user can
    // always reach it regardless of how extreme the scale gets.
    float  view_scale  = 1.0f;
    ImVec2 view_pivot  = ImVec2(400.0f, 300.0f);
    bool   view_enable = true;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Control panel (always at identity).
        ImGui::SetNextWindowPos(ImVec2(800, 20), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(360, 200), ImGuiCond_FirstUseEver);
        ImGui::Begin("View Controls");
        ImGui::Checkbox("Apply view transform", &view_enable);
        ImGui::SliderFloat("scale",  &view_scale,    0.25f, 4.0f, "%.3f");
        ImGui::SliderFloat("pivot.x",&view_pivot.x,  0.0f, 1280.0f, "%.0f");
        ImGui::SliderFloat("pivot.y",&view_pivot.y,  0.0f,  800.0f, "%.0f");
        ImGui::TextDisabled("composed: scale=%.3f offset=(%.1f, %.1f)",
                            ImGui::GetViewScale(), ImGui::GetViewOffset().x, ImGui::GetViewOffset().y);
        ImGui::TextDisabled("(PR 1: visual only — hit-test stays unscaled)");
        ImGui::End();

        if (view_enable && view_scale != 1.0f)
            ImGui::PushView(view_scale, view_pivot);
        ScaleHarness::RenderWidgetZoo();
        if (view_enable && view_scale != 1.0f)
            ImGui::PopView();

        ImGui::Render();
        int fb_w, fb_h;
        glfwGetFramebufferSize(window, &fb_w, &fb_h);
        glViewport(0, 0, fb_w, fb_h);
        glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

// ---------------------------------------------------------------------------
// Headless mode
// ---------------------------------------------------------------------------

static int RunHeadless(const char* out_path, int width, int height, int frames,
                       float view_scale, ImVec2 view_pivot, bool view_pivot_set)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    const char* glsl_version = SetupGLVersionHints();

    // Hidden window solely as a GL context provider. On macOS the actual
    // backing surface size of a hidden window is unreliable (Retina hints
    // are honored inconsistently), so we never render into the window's
    // default framebuffer — we render into our own FBO sized exactly to
    // the requested output. This is the only path that's reproducible
    // across machines.
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "scale_harness_headless", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    const int fb_w = width;
    const int fb_h = height;

    // Create offscreen FBO with an RGBA8 color renderbuffer at the exact
    // requested size. No depth/stencil — imgui doesn't need either.
    GLuint fbo = 0, color_rbo = 0;
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &color_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, color_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, fb_w, fb_h);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_rbo);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::fprintf(stderr, "scale_harness: FBO incomplete\n");
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = nullptr;
    io.BackendPlatformName = "scale_harness_headless";

    ImGui::StyleColorsDark();
    // We intentionally do not init imgui_impl_glfw — its NewFrame would
    // overwrite io.DisplaySize / io.DisplayFramebufferScale from the
    // (hidden) window's properties.
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Run a few frames so first-frame settling (font atlas upload, item
    // auto-sizing) resolves before we read pixels back.
    for (int frame = 0; frame < frames; frame++)
    {
        ImGui_ImplOpenGL3_NewFrame();
        io.DisplaySize = ImVec2((float)fb_w, (float)fb_h);
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
        io.DeltaTime = 1.0f / 60.0f;
        ImGui::NewFrame();

        // Apply view transform if scale differs from identity. Default pivot
        // is the framebuffer center so e.g. `--scale 2` zooms toward the
        // middle without dragging the zoo offscreen.
        const ImVec2 pivot = view_pivot_set ? view_pivot
                                            : ImVec2((float)fb_w * 0.5f, (float)fb_h * 0.5f);
        const bool view_active = (view_scale != 1.0f);
        if (view_active)
            ImGui::PushView(view_scale, pivot);
        ScaleHarness::RenderWidgetZoo();
        if (view_active)
            ImGui::PopView();

        ImGui::Render();
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glDisable(GL_SCISSOR_TEST); // backend may have left scissor enabled from a previous frame
        glViewport(0, 0, fb_w, fb_h);
        glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    // Read back from the FBO's color attachment.
    std::vector<unsigned char> pixels((size_t)fb_w * (size_t)fb_h * 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, fb_w, fb_h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // glReadPixels returns rows bottom-to-top; flip into top-down order
    // expected by PNG and most image viewers.
    std::vector<unsigned char> flipped((size_t)fb_w * (size_t)fb_h * 4);
    const size_t row_bytes = (size_t)fb_w * 4;
    for (int y = 0; y < fb_h; y++)
        std::memcpy(&flipped[(size_t)y * row_bytes],
                    &pixels[(size_t)(fb_h - 1 - y) * row_bytes],
                    row_bytes);

    bool ok = WritePNG_RGBA8(out_path, fb_w, fb_h, flipped.data());
    if (ok)
        std::printf("scale_harness: wrote %s (%dx%d)\n", out_path, fb_w, fb_h);

    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &color_rbo);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return ok ? 0 : 1;
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main(int argc, char** argv)
{
    bool headless = false;
    const char* out_path = "scale_harness.png";
    int width = 800;
    int height = 600;
    int frames = 2;
    float view_scale = 1.0f;
    ImVec2 view_pivot(0, 0);
    bool view_pivot_set = false;

    for (int i = 1; i < argc; i++)
    {
        const char* a = argv[i];
        if (std::strcmp(a, "--headless") == 0) {
            headless = true;
        } else if (std::strcmp(a, "-o") == 0 && i + 1 < argc) {
            out_path = argv[++i];
        } else if (std::strcmp(a, "-W") == 0 && i + 1 < argc) {
            width = std::atoi(argv[++i]);
        } else if (std::strcmp(a, "-H") == 0 && i + 1 < argc) {
            height = std::atoi(argv[++i]);
        } else if (std::strcmp(a, "--frames") == 0 && i + 1 < argc) {
            frames = std::atoi(argv[++i]);
        } else if (std::strcmp(a, "--scale") == 0 && i + 1 < argc) {
            view_scale = (float)std::atof(argv[++i]);
        } else if (std::strcmp(a, "--pivot") == 0 && i + 1 < argc) {
            // Accept "x,y".
            const char* s = argv[++i];
            float px = 0, py = 0;
            if (std::sscanf(s, "%f,%f", &px, &py) != 2) {
                std::fprintf(stderr, "scale_harness: --pivot expects 'x,y' (got '%s')\n", s);
                return 2;
            }
            view_pivot = ImVec2(px, py);
            view_pivot_set = true;
        } else if (std::strcmp(a, "-h") == 0 || std::strcmp(a, "--help") == 0) {
            std::printf(
                "Usage: %s [--headless [-o out.png] [-W w] [-H h] [--frames N]\n"
                "                      [--scale s] [--pivot x,y]]\n"
                "  --scale and --pivot apply a PushView/PopView around the widget zoo.\n"
                "  Default pivot is the framebuffer center.\n",
                argv[0]);
            return 0;
        } else {
            std::fprintf(stderr, "scale_harness: unknown arg '%s'\n", a);
            return 2;
        }
    }

    return headless ? RunHeadless(out_path, width, height, frames, view_scale, view_pivot, view_pivot_set)
                    : RunInteractive();
}
