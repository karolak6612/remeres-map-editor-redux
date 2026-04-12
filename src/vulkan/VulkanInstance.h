#ifndef RME_VULKAN_INSTANCE_H_
#define RME_VULKAN_INSTANCE_H_

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

class VulkanInstance {
public:
    VulkanInstance();
    ~VulkanInstance();

    bool Initialize(const std::vector<const char*>& requiredExtensions);

    VkInstance GetInstance() const { return m_instance; }

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

    bool CheckValidationLayerSupport();
    void SetupDebugMessenger();
};

#endif
