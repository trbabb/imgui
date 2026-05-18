// See widget_zoo.h.

#include "widget_zoo.h"
#include "imgui.h"

namespace ScaleHarness {

// Persistent widget state held in static locals. We avoid any time/frame-
// dependent state so that, given identical input, every call produces the
// same draw data.
void RenderWidgetZoo()
{
    // Anchor the window to a known position/size so headless renders are
    // reproducible regardless of .ini state.
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(420, 540), ImGuiCond_Always);
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

    ImGui::Separator();

    // Small scrollable child to exercise clip rects and scrollbars.
    ImGui::BeginChild("scroll_child", ImVec2(0, 120), ImGuiChildFlags_Borders);
    for (int i = 0; i < 20; i++)
        ImGui::Text("scroll line %d", i);
    ImGui::EndChild();

    // ProgressBar exercises rect drawing + text overlay.
    ImGui::ProgressBar(0.65f, ImVec2(-1, 0), "65%");

    ImGui::End();
}

} // namespace ScaleHarness
