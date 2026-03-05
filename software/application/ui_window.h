#pragma once

#include <vulkan/vulkan.h>

struct UiFrameContext
{
    const char* platform;
    int framebuffer_width;
    int framebuffer_height;
    float framerate;
    VkPhysicalDevice vulkan_physical_device;
    VkDevice vulkan_device;
    VkQueue vulkan_queue;
    VkCommandPool vulkan_command_pool;
    const VkAllocationCallbacks* vulkan_allocator;
};

using UiWindowRenderFn = void(*)(const UiFrameContext&);

struct UiWindow
{
    const char* id;
    UiWindowRenderFn render;
};
