#include "rendering/drawers/minimap_renderer.h"

MinimapRenderer::MinimapRenderer(VulkanContext* vkContext) : vkContext_(vkContext) {}
MinimapRenderer::~MinimapRenderer() {}

void MinimapRenderer::initialize(VkRenderPass renderPass, uint32_t subpass) {}

void MinimapRenderer::draw(const RenderView& view, VkCommandBuffer cmdBuffer) {}
