#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_vulkan.h"

#include "ui_window.h"
#include "windows/status_window.h"
#include "windows/system_overview_window.h"
#include "windows/radar_window.h"

namespace
{
constexpr int kWindowWidth = 800;
constexpr int kWindowHeight = 480;
constexpr int kMinImageCount = 2;
constexpr int kFrameCapFps = 60;

VkAllocationCallbacks* g_allocator = nullptr;
VkInstance g_instance = VK_NULL_HANDLE;
VkPhysicalDevice g_physical_device = VK_NULL_HANDLE;
VkDevice g_device = VK_NULL_HANDLE;
uint32_t g_graphics_queue_family = UINT32_MAX;
VkQueue g_graphics_queue = VK_NULL_HANDLE;
VkDescriptorPool g_descriptor_pool = VK_NULL_HANDLE;

ImGui_ImplVulkanH_Window g_main_window_data;
int g_swapchain_rebuild = 0;

void CheckVkResult(VkResult err)
{
    if (err == VK_SUCCESS)
    {
        return;
    }

    std::cerr << "Vulkan error: " << static_cast<int>(err) << std::endl;
    if (err < 0)
    {
        std::abort();
    }
}

std::vector<const char*> GetRequiredInstanceExtensions(SDL_Window* window)
{
    unsigned int extension_count = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, nullptr))
    {
        throw std::runtime_error(std::string("SDL_Vulkan_GetInstanceExtensions failed: ") + SDL_GetError());
    }

    std::vector<const char*> extensions(extension_count);
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, extensions.data()))
    {
        throw std::runtime_error(std::string("SDL_Vulkan_GetInstanceExtensions (names) failed: ") + SDL_GetError());
    }

    return extensions;
}

void SetupVulkan(const std::vector<const char*>& instance_extensions)
{
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Home Automation Interface";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "None";
    app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size());
    create_info.ppEnabledExtensionNames = instance_extensions.data();

    CheckVkResult(vkCreateInstance(&create_info, g_allocator, &g_instance));

    uint32_t gpu_count = 0;
    CheckVkResult(vkEnumeratePhysicalDevices(g_instance, &gpu_count, nullptr));
    if (gpu_count == 0)
    {
        throw std::runtime_error("No Vulkan physical devices found.");
    }

    std::vector<VkPhysicalDevice> physical_devices(gpu_count);
    CheckVkResult(vkEnumeratePhysicalDevices(g_instance, &gpu_count, physical_devices.data()));
    g_physical_device = physical_devices[0];

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(g_physical_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(g_physical_device, &queue_family_count, queue_families.data());

    for (uint32_t i = 0; i < queue_family_count; ++i)
    {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            g_graphics_queue_family = i;
            break;
        }
    }

    if (g_graphics_queue_family == UINT32_MAX)
    {
        throw std::runtime_error("No graphics queue family available.");
    }

    const float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = g_graphics_queue_family;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;

    const std::array<const char*, 1> device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    device_create_info.ppEnabledExtensionNames = device_extensions.data();

    CheckVkResult(vkCreateDevice(g_physical_device, &device_create_info, g_allocator, &g_device));
    vkGetDeviceQueue(g_device, g_graphics_queue_family, 0, &g_graphics_queue);

    const VkDescriptorPoolSize pool_sizes[] = {
      {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
    };

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * static_cast<uint32_t>(std::size(pool_sizes));
    pool_info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
    pool_info.pPoolSizes = pool_sizes;

    CheckVkResult(vkCreateDescriptorPool(g_device, &pool_info, g_allocator, &g_descriptor_pool));
}

void CleanupVulkan()
{
    vkDestroyDescriptorPool(g_device, g_descriptor_pool, g_allocator);
    vkDestroyDevice(g_device, g_allocator);
    vkDestroyInstance(g_instance, g_allocator);
}

void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height)
{
    wd->Surface = surface;

    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR(g_physical_device, g_graphics_queue_family, wd->Surface, &res);
    if (res != VK_TRUE)
    {
        throw std::runtime_error("Selected queue family does not support this surface.");
    }

    const VkFormat request_surface_image_format[] = {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM};
    const VkColorSpaceKHR request_surface_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
      g_physical_device,
      wd->Surface,
      request_surface_image_format,
      static_cast<size_t>(std::size(request_surface_image_format)),
      request_surface_color_space);

    const VkPresentModeKHR request_present_modes[] = {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_FIFO_KHR};
    wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(
      g_physical_device,
      wd->Surface,
      request_present_modes,
      static_cast<size_t>(std::size(request_present_modes)));

    ImGui_ImplVulkanH_CreateOrResizeWindow(
      g_instance,
      g_physical_device,
      g_device,
      wd,
      g_graphics_queue_family,
      g_allocator,
      width,
      height,
      kMinImageCount);
}

void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data)
{
    VkSemaphore image_acquired_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkResult err = vkAcquireNextImageKHR(g_device, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);

    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
    {
        g_swapchain_rebuild = 1;
        return;
    }
    CheckVkResult(err);

    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];

    CheckVkResult(vkWaitForFences(g_device, 1, &fd->Fence, VK_TRUE, UINT64_MAX));
    CheckVkResult(vkResetFences(g_device, 1, &fd->Fence));
    CheckVkResult(vkResetCommandPool(g_device, fd->CommandPool, 0));

    VkCommandBufferBeginInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    CheckVkResult(vkBeginCommandBuffer(fd->CommandBuffer, &info));

    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = wd->RenderPass;
    render_pass_info.framebuffer = fd->Framebuffer;
    render_pass_info.renderArea.extent.width = wd->Width;
    render_pass_info.renderArea.extent.height = wd->Height;
    render_pass_info.clearValueCount = 1;

    VkClearValue clear_color = {{{0.06F, 0.08F, 0.10F, 1.0F}}};
    render_pass_info.pClearValues = &clear_color;
    vkCmdBeginRenderPass(fd->CommandBuffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

    vkCmdEndRenderPass(fd->CommandBuffer);
    CheckVkResult(vkEndCommandBuffer(fd->CommandBuffer));

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &image_acquired_semaphore;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &fd->CommandBuffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &render_complete_semaphore;

    CheckVkResult(vkQueueSubmit(g_graphics_queue, 1, &submit_info, fd->Fence));
}

void FramePresent(ImGui_ImplVulkanH_Window* wd)
{
    if (g_swapchain_rebuild)
    {
        return;
    }

    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &wd->Swapchain;
    info.pImageIndices = &wd->FrameIndex;

    const VkResult err = vkQueuePresentKHR(g_graphics_queue, &info);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
    {
        g_swapchain_rebuild = 1;
        return;
    }

    CheckVkResult(err);
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->ImageCount;
}

void RenderDockSpace()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags host_window_flags = ImGuiWindowFlags_NoDocking;
    host_window_flags |= ImGuiWindowFlags_NoTitleBar;
    host_window_flags |= ImGuiWindowFlags_NoCollapse;
    host_window_flags |= ImGuiWindowFlags_NoResize;
    host_window_flags |= ImGuiWindowFlags_NoMove;
    host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
    host_window_flags |= ImGuiWindowFlags_NoNavFocus;
    host_window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0F, 0.0F));
    ImGui::Begin("MainDockSpace", nullptr, host_window_flags);
    ImGui::PopStyleVar(3);

    const ImGuiID dockspace_id = ImGui::GetID("HomeAutomationDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0F, 0.0F), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();
}

}  // namespace

int main(int, char**)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_WindowFlags window_flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Home Automation Interface", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, kWindowWidth, kWindowHeight, window_flags);
    if (window == nullptr)
    {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    try
    {
        const auto extensions = GetRequiredInstanceExtensions(window);
        SetupVulkan(extensions);

        VkSurfaceKHR surface = VK_NULL_HANDLE;
        if (!SDL_Vulkan_CreateSurface(window, g_instance, &surface))
        {
            throw std::runtime_error(std::string("SDL_Vulkan_CreateSurface failed: ") + SDL_GetError());
        }

        int framebuffer_w = 0;
        int framebuffer_h = 0;
        SDL_Vulkan_GetDrawableSize(window, &framebuffer_w, &framebuffer_h);
        SetupVulkanWindow(&g_main_window_data, surface, framebuffer_w, framebuffer_h);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();

        ImGui_ImplSDL2_InitForVulkan(window);

        ImGui_ImplVulkan_InitInfo init_info{};
        init_info.Instance = g_instance;
        init_info.PhysicalDevice = g_physical_device;
        init_info.Device = g_device;
        init_info.QueueFamily = g_graphics_queue_family;
        init_info.Queue = g_graphics_queue;
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = g_descriptor_pool;
        init_info.RenderPass = g_main_window_data.RenderPass;
        init_info.Subpass = 0;
        init_info.MinImageCount = kMinImageCount;
        init_info.ImageCount = g_main_window_data.ImageCount;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = g_allocator;
        init_info.CheckVkResultFn = CheckVkResult;
        ImGui_ImplVulkan_Init(&init_info);

        ImGui_ImplVulkan_CreateFontsTexture();
        CheckVkResult(vkDeviceWaitIdle(g_device));
        ImGui_ImplVulkan_DestroyFontsTexture();

                // Here we create the windows that will be rendered in the main loop.
                const std::vector<UiWindow> ui_windows = {
                    CreateSystemOverviewWindow(),
                    CreateStatusWindow(),
                    CreateRadarWindow(),
                };

        bool done = false;
        while (!done)
        {
            const Uint64 frame_start_ticks = SDL_GetTicks64();

            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                ImGui_ImplSDL2_ProcessEvent(&event);
                if (event.type == SDL_QUIT)
                {
                    done = true;
                }
                if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                {
                    done = true;
                }
                if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                {
                    g_swapchain_rebuild = 1;
                }
            }

            if (g_swapchain_rebuild)
            {
                SDL_Vulkan_GetDrawableSize(window, &framebuffer_w, &framebuffer_h);
                if (framebuffer_w > 0 && framebuffer_h > 0)
                {
                    ImGui_ImplVulkan_SetMinImageCount(kMinImageCount);
                    ImGui_ImplVulkanH_CreateOrResizeWindow(
                      g_instance,
                      g_physical_device,
                      g_device,
                      &g_main_window_data,
                      g_graphics_queue_family,
                      g_allocator,
                      framebuffer_w,
                      framebuffer_h,
                      kMinImageCount);
                    g_main_window_data.FrameIndex = 0;
                    g_swapchain_rebuild = 0;
                }
            }

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            RenderDockSpace();

                        const UiFrameContext frame_context{
                            .platform = SDL_GetPlatform(),
                            .framebuffer_width = framebuffer_w,
                            .framebuffer_height = framebuffer_h,
                            .framerate = io.Framerate,
                            .vulkan_physical_device = g_physical_device,
                            .vulkan_device = g_device,
                            .vulkan_queue = g_graphics_queue,
                            .vulkan_command_pool = g_main_window_data.Frames[g_main_window_data.FrameIndex].CommandPool,
                            .vulkan_allocator = g_allocator,
                        };

                        for (const UiWindow& ui_window : ui_windows)
                        {
                                ui_window.render(frame_context);
                        }

            ImGui::Render();
            ImDrawData* draw_data = ImGui::GetDrawData();
            const bool is_minimized = (draw_data->DisplaySize.x <= 0.0F || draw_data->DisplaySize.y <= 0.0F);
            if (!is_minimized)
            {
                FrameRender(&g_main_window_data, draw_data);
                FramePresent(&g_main_window_data);
            }

            if (kFrameCapFps > 0)
            {
                const Uint64 frame_target_ms = 1000ULL / static_cast<Uint64>(kFrameCapFps);
                const Uint64 frame_elapsed_ms = SDL_GetTicks64() - frame_start_ticks;
                if (frame_elapsed_ms < frame_target_ms)
                {
                    SDL_Delay(static_cast<Uint32>(frame_target_ms - frame_elapsed_ms));
                }
            }
        }

        CheckVkResult(vkDeviceWaitIdle(g_device));
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        ImGui_ImplVulkanH_DestroyWindow(g_instance, g_device, &g_main_window_data, g_allocator);
        CleanupVulkan();
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
