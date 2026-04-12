#ifndef RME_RENDERING_DRAWERS_MINIMAP_RENDERER_H_
#define RME_RENDERING_DRAWERS_MINIMAP_RENDERER_H_

#include "rendering/core/render_view.h"
#include "rendering/core/vulkan_context.h"
#include <memory>

class MinimapRenderer {
public:
    MinimapRenderer(VulkanContext* vkContext);
    ~MinimapRenderer();

    void initialize(VkRenderPass renderPass, uint32_t subpass);
    void draw(const RenderView& view, VkCommandBuffer cmdBuffer = VK_NULL_HANDLE);

private:
    VulkanContext* vkContext_ = nullptr;
};

#endif
