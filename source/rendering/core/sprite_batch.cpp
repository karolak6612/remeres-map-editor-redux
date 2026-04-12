#include "rendering/core/sprite_batch.h"
#include "rendering/core/shared_geometry.h"

SpriteBatch::SpriteBatch(VulkanContext* vkContext) : vkContext_(vkContext) {}
SpriteBatch::~SpriteBatch() {}
SpriteBatch::SpriteBatch(SpriteBatch&& other) noexcept : vkContext_(other.vkContext_) {}
SpriteBatch& SpriteBatch::operator=(SpriteBatch&& other) noexcept { return *this; }

bool SpriteBatch::initialize(VkRenderPass renderPass, uint32_t subpass) {
    // Basic implementation outline
    return true;
}
void SpriteBatch::begin(const glm::mat4& projection, const AtlasManager& atlas_manager) {}
void SpriteBatch::draw(float x, float y, float w, float h, const AtlasRegion& region) {}
void SpriteBatch::draw(float x, float y, float w, float h, const AtlasRegion& region, float r, float g, float b, float a) {}
void SpriteBatch::drawRect(float x, float y, float w, float h, const glm::vec4& color, const AtlasManager& atlas_manager) {}
void SpriteBatch::drawRectLines(float x, float y, float w, float h, const glm::vec4& color, const AtlasManager& atlas_manager) {}
void SpriteBatch::flush(VkCommandBuffer cmdBuffer, const AtlasManager& atlas_manager) {}
void SpriteBatch::end(VkCommandBuffer cmdBuffer, const AtlasManager& atlas_manager) {}
void SpriteBatch::setGlobalTint(float r, float g, float b, float a, const AtlasManager& atlas_manager, VkCommandBuffer cmdBuffer) {}
void SpriteBatch::ensureCapacity(size_t capacity) {}
