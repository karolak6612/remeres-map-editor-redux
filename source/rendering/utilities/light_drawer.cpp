#include "rendering/utilities/light_drawer.h"
#include "rendering/core/shared_geometry.h"

LightDrawer::LightDrawer(VulkanContext* vkContext) : vkContext_(vkContext) {}

LightDrawer::~LightDrawer() {}

void LightDrawer::initialize(VkRenderPass renderPass, uint32_t subpass) {}

void LightDrawer::updateFBO(const RenderView& view) {}

void LightDrawer::draw(const RenderView& view, bool fogEnabled, const LightBuffer& light_buffer,
              const glm::vec4& global_light_color, float light_intensity, float ambient_light_level, VkCommandBuffer cmdBuffer) {}
