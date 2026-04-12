#ifndef VULKAN_SURFACE_H
#define VULKAN_SURFACE_H

#include <vulkan/vulkan.h>
#include <wx/window.h>

VkSurfaceKHR CreateVulkanSurface(VkInstance instance, wxWindow* window);

#endif
