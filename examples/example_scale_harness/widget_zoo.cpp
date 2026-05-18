// See widget_zoo.h.

#include "widget_zoo.h"
#include "imgui.h"
#include <cstdio>

namespace ScaleHarness {

// Persistent widget state held in static locals. We avoid any time/frame-
// dependent state so that, given identical input, every call produces the
// same draw data.
void RenderWidgetZoo()
{
    // Seed a known position/size on first use so headless renders are
    // reproducible. FirstUseEver (not Always) lets the user move/resize
    // the window in interactive mode; the headless harness sets
    // io.IniFilename = nullptr so a fresh process always starts from these
    // values.
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(420, 540), ImGuiCond_FirstUseEver);
    ImGui::Begin("Widget Zoo", nullptr, ImGuiWindowFlags_NoSavedSettings);

    ImGui::TextUnformatted("Plain text line.");
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Colored text.");
    ImGui::TextDisabled("Disabled text.");
    ImGui::Separator();

    static bool checkbox_value = true;
    ImGui::Checkbox("Checkbox", &checkbox_value);

    static int radio_value = 0;
    ImGui::RadioButton("A", &radio_value, 0); ImGui::SameLine();
    ImGui::RadioButton("B", &radio_value, 1); ImGui::SameLine();
    ImGui::RadioButton("C", &radio_value, 2);

    static int counter = 0;
    if (ImGui::Button("Button"))
        counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::Separator();

    static float f_slider = 0.42f;
    ImGui::SliderFloat("float slider", &f_slider, 0.0f, 1.0f);

    static int i_slider = 7;
    ImGui::SliderInt("int slider", &i_slider, 0, 10);

    static float color3[3] = { 0.45f, 0.55f, 0.60f };
    ImGui::ColorEdit3("color", color3);

    static char text_buf[64] = "edit me";
    ImGui::InputText("input", text_buf, IM_ARRAYSIZE(text_buf));

    static int combo_idx = 1;
    const char* combo_items[] = { "alpha", "beta", "gamma", "delta" };
    ImGui::Combo("combo", &combo_idx, combo_items, IM_ARRAYSIZE(combo_items));

    ImGui::Separator();

    if (ImGui::TreeNode("Tree"))
    {
        ImGui::BulletText("first child");
        ImGui::BulletText("second child");
        if (ImGui::TreeNode("nested"))
        {
            ImGui::BulletText("nested leaf");
            ImGui::TreePop();
        }
        ImGui::TreePop();
    }

    static float drag_value = 0.5f;
    ImGui::DragFloat("drag", &drag_value, 0.01f, 0.0f, 1.0f);

    ImGui::Separator();

    // Scrollable child with selectable rows. Exercises clip rects, scrollbars,
    // and gives interactive hit-test targets inside a sub-region (useful when
    // we start scaling and need to verify clicks inside child windows).
    ImGui::BeginChild("scroll_child", ImVec2(0, 120), ImGuiChildFlags_Borders);
    static int selected_row = -1;
    for (int i = 0; i < 20; i++)
    {
        char label[32];
        std::snprintf(label, sizeof(label), "scroll line %d", i);
        if (ImGui::Selectable(label, selected_row == i))
            selected_row = i;
    }
    ImGui::EndChild();

    // Display-only progress bar. Not interactive by design — included to
    // exercise rect drawing + text overlay rather than as a hit-test target.
    ImGui::ProgressBar(0.65f, ImVec2(-1, 0), "progress 65%");

    ImGui::End();
}

} // namespace ScaleHarness
