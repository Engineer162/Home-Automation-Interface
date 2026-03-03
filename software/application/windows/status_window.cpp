#include "status_window.h"

#include "imgui.h"

namespace
{
    void RenderStatusWindow(const UiFrameContext& context)
        {
            ImGui::Begin("Status");
            ImGui::Text("Example modular window");
            ImGui::Separator();
            ImGui::Text("Resolution: %d x %d", context.framebuffer_width, context.framebuffer_height);
            ImGui::Text("FPS: %.1f", context.framerate);
            ImGui::End();
        }
}// Window namespace

UiWindow CreateStatusWindow()
{
    return UiWindow{
      .id = "status",
      .render = RenderStatusWindow,
    };
}
