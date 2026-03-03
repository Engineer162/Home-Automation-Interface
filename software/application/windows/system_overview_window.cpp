#include "system_overview_window.h"

#include "imgui.h"

namespace
{
void RenderSystemOverview(const UiFrameContext& context)
{
    ImGui::Begin("System Overview");
    ImGui::Text("Home automation interface is running.");
    ImGui::Separator();
    ImGui::Text("Platform: %s", context.platform);
    ImGui::Text("Display size: %d x %d", context.framebuffer_width, context.framebuffer_height);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0F / context.framerate, context.framerate);
    ImGui::End();
}
}  // namespace

UiWindow CreateSystemOverviewWindow()
{
    return UiWindow{
      .id = "system_overview",
      .render = RenderSystemOverview,
    };
}
