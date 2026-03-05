#pragma once

#include <vulkan/vulkan.h>

struct MyTextureData
{
	VkDescriptorSet DS = VK_NULL_HANDLE;
	int Width = 0;
	int Height = 0;
	int Channels = 0;

	VkImageView ImageView = VK_NULL_HANDLE;
	VkImage Image = VK_NULL_HANDLE;
	VkDeviceMemory ImageMemory = VK_NULL_HANDLE;
	VkSampler Sampler = VK_NULL_HANDLE;
	VkBuffer UploadBuffer = VK_NULL_HANDLE;
	VkDeviceMemory UploadBufferMemory = VK_NULL_HANDLE;
};

struct ImageLoaderContext
{
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkQueue queue = VK_NULL_HANDLE;
	VkCommandPool command_pool = VK_NULL_HANDLE;
	const VkAllocationCallbacks* allocator = nullptr;
};

bool LoadTextureFromFile(const char* filename, const ImageLoaderContext& context, MyTextureData* tex_data);
void RemoveTexture(const ImageLoaderContext& context, MyTextureData* tex_data);
