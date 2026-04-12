#ifndef RME_RENDERING_UTILITIES_LIGHT_DRAWER_H_
#define RME_RENDERING_UTILITIES_LIGHT_DRAWER_H_

#include "rendering/core/render_view.h"
#include "rendering/core/light_buffer.h"
#include "rendering/core/vulkan_context.h"
#include "rendering/core/vulkan_resources.h"
#include "rendering/core/shader_program.h"
#include <memory>

class LightDrawer {
public:
    LightDrawer(VulkanContext* vkContext);
    ~LightDrawer();

    void initialize(VkRenderPass renderPass, uint32_t subpass);

    void draw(const RenderView& view, bool fogEnabled, const LightBuffer& light_buffer,
              const glm::vec4& global_light_color, float light_intensity, float ambient_light_level, VkCommandBuffer cmdBuffer = VK_NULL_HANDLE);

private:
    VulkanContext* vkContext_ = nullptr;
    std::unique_ptr<ShaderProgram> shader_;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;

    std::unique_ptr<VulkanBuffer> vbo_;
    std::unique_ptr<VulkanBuffer> ebo_;

    std::unique_ptr<VulkanImage> fbo_image_;
    VkFramebuffer vkFramebuffer_ = VK_NULL_HANDLE;
    int fbo_width_ = 0;
    int fbo_height_ = 0;

    void updateFBO(const RenderView& view);
};

#endif
