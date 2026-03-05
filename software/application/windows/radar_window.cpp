#include "radar_window.h"

#include <cstdint>
#include <cmath>

#include "imgui.h"

#include "image_loader/image_loader.h"

const char* radar_image = "radar_image.png";

namespace
{
    constexpr float kPi = 3.14159265358979323846f;

    MyTextureData g_radar_texture;
    bool g_radar_texture_loaded = false;
    bool g_radar_texture_failed = false;
    float g_sweep_angle_deg = 0.0f;
    float g_sweep_speed_deg_per_sec = 50.0f;
    float g_sweep_length_ratio = 0.895f;
    float g_center_offset_x = 0.7f;
    float g_center_offset_y = 1.4f;
    bool g_show_debug_axes = false;

    ImageLoaderContext BuildImageLoaderContext(const UiFrameContext& context)
    {
        return ImageLoaderContext{
          .physical_device = context.vulkan_physical_device,
          .device = context.vulkan_device,
          .queue = context.vulkan_queue,
          .command_pool = context.vulkan_command_pool,
          .allocator = context.vulkan_allocator,
        };
    }

    void RenderRadarWindow(const UiFrameContext& context)
    {
        if (!g_radar_texture_loaded && !g_radar_texture_failed)
        {
            const ImageLoaderContext loader_context = BuildImageLoaderContext(context);
            g_radar_texture_loaded = LoadTextureFromFile(radar_image, loader_context, &g_radar_texture);
            g_radar_texture_failed = !g_radar_texture_loaded;
        }

        ImGui::Begin("Radar");
        if (g_radar_texture_loaded)
        {
            // These are commented out debug info and controls that I used while developing the radar window. They are left here for future reference and testing.

            // The slider for the sweep line length has a sensitivity of 3 decimal places,
            // because my OCD was tripping over the fact that the sweep line length was not perfectly reaching the edge of the radar image. 
            // The default value of 0.895 was chosen by trial and error to make the sweep line reach the edge of the radar image without going over it.

            //ImGui::Text("pointer = %p", (void*)(intptr_t)g_radar_texture.DS);
            //ImGui::Text("size = %d x %d", g_radar_texture.Width, g_radar_texture.Height);
            //ImGui::SliderFloat("Sweep line length", &g_sweep_length_ratio, 0.1f, 1.0f, "%.3f");
            //ImGui::SliderFloat("Sweep speed", &g_sweep_speed_deg_per_sec, 0.0f, 360.0f, "%.1f deg/s");
            //ImGui::SliderFloat("Center offset X", &g_center_offset_x, -200.0f, 200.0f, "%.1f px");
            //ImGui::SliderFloat("Center offset Y", &g_center_offset_y, -200.0f, 200.0f, "%.1f px");
            //ImGui::Checkbox("Show debug axes", &g_show_debug_axes);
            
            ImGui::Separator();

            const ImVec2 available = ImGui::GetContentRegionAvail();
            if (available.x > 0.0f && available.y > 0.0f)
            {
                const float texture_aspect = static_cast<float>(g_radar_texture.Width) / static_cast<float>(g_radar_texture.Height);

                float draw_width = available.x;
                float draw_height = draw_width / texture_aspect;

                if (draw_height > available.y)
                {
                    draw_height = available.y;
                    draw_width = draw_height * texture_aspect;
                }

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (available.x - draw_width) * 0.5f);
                                const ImVec2 image_pos = ImGui::GetCursorScreenPos();
                ImGui::Image((ImTextureID)g_radar_texture.DS, ImVec2(draw_width, draw_height));

                                const float delta_time = (context.framerate > 0.0f) ? (1.0f / context.framerate) : 0.0f;
                                g_sweep_angle_deg += g_sweep_speed_deg_per_sec * delta_time;
                                while (g_sweep_angle_deg >= 360.0f)
                                {
                                        g_sweep_angle_deg -= 360.0f;
                                }

                                const ImVec2 center(
                                    image_pos.x + draw_width * 0.5f + g_center_offset_x,
                                    image_pos.y + draw_height * 0.5f + g_center_offset_y);
                                const float max_radius = (draw_width < draw_height ? draw_width : draw_height) * 0.5f;
                                const float sweep_length = max_radius * g_sweep_length_ratio;
                                const float angle_rad = (-90.0f + g_sweep_angle_deg) * (kPi / 180.0f);
                                const ImVec2 end_point(
                                    center.x + std::cos(angle_rad) * sweep_length,
                                    center.y + std::sin(angle_rad) * sweep_length);

                                ImGui::GetWindowDrawList()->AddLine(center, end_point, IM_COL32(0, 255, 0, 255), 2.0f);

                                if (g_show_debug_axes)
                                {
                                    const float axis_extent = sweep_length;
                                    const ImVec2 x_start(center.x - axis_extent, center.y);
                                    const ImVec2 x_end(center.x + axis_extent, center.y);
                                    const ImVec2 y_start(center.x, center.y - axis_extent);
                                    const ImVec2 y_end(center.x, center.y + axis_extent);

                                        ImGui::GetWindowDrawList()->AddLine(x_start, x_end, IM_COL32(255, 0, 0, 255), 2.0f);
                                        ImGui::GetWindowDrawList()->AddLine(y_start, y_end, IM_COL32(255, 0, 0, 255), 2.0f);
                                }
            }
        }
        else
        {
            ImGui::Text("Texture not loaded.");
            ImGui::Text("Expected file: %s", radar_image);
        }
        ImGui::Separator();
        ImGui::End();
    }
}// Window namespace

UiWindow CreateRadarWindow()
{
    return UiWindow{
      .id = "radar",
      .render = RenderRadarWindow,
    };
}
