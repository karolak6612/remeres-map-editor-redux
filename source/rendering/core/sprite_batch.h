#ifndef RME_RENDERING_CORE_SPRITE_BATCH_H_
#define RME_RENDERING_CORE_SPRITE_BATCH_H_

#include "app/main.h"
#include "rendering/core/ring_buffer.h"
#include "rendering/core/sprite_instance.h"
#include "rendering/core/shader_program.h"
#include "rendering/core/texture_atlas.h"
#include "rendering/core/atlas_manager.h"
#include "rendering/core/vulkan_resources.h"
#include "rendering/core/vulkan_context.h"
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <optional>

class SpriteBatch {
public:
    static constexpr size_t MAX_SPRITES_PER_BATCH = 100000;

    SpriteBatch(VulkanContext* vkContext);
    ~SpriteBatch();

    SpriteBatch(const SpriteBatch&) = delete;
    SpriteBatch& operator=(const SpriteBatch&) = delete;
    SpriteBatch(SpriteBatch&& other) noexcept;
    SpriteBatch& operator=(SpriteBatch&& other) noexcept;

    bool initialize(VkRenderPass renderPass, uint32_t subpass);

    void begin(const glm::mat4& projection, const AtlasManager& atlas_manager);
    void draw(float x, float y, float w, float h, const AtlasRegion& region);
    void draw(float x, float y, float w, float h, const AtlasRegion& region, float r, float g, float b, float a);
    void drawRect(float x, float y, float w, float h, const glm::vec4& color, const AtlasManager& atlas_manager);
    void drawRectLines(float x, float y, float w, float h, const glm::vec4& color, const AtlasManager& atlas_manager);

    // Command buffer is required to record the draw commands
    void flush(VkCommandBuffer cmdBuffer, const AtlasManager& atlas_manager);
    void end(VkCommandBuffer cmdBuffer, const AtlasManager& atlas_manager);

    void setGlobalTint(float r, float g, float b, float a, const AtlasManager& atlas_manager, VkCommandBuffer cmdBuffer = VK_NULL_HANDLE);

    void ensureCapacity(size_t capacity);

    int getDrawCallCount() const { return draw_call_count_; }
    int getSpriteCount() const { return sprite_count_; }

private:
    VulkanContext* vkContext_ = nullptr;
    std::unique_ptr<ShaderProgram> shader_;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;

    std::unique_ptr<VulkanBuffer> vertexBuffer_; // unit quad
    std::unique_ptr<VulkanBuffer> indexBuffer_;
    RingBuffer ring_buffer_;

    std::vector<SpriteInstance> pending_sprites_;
    glm::mat4 projection_ { 1.0f };
    glm::vec4 global_tint_ { 1.0f };

    bool in_batch_ = false;

    int draw_call_count_ = 0;
    int sprite_count_ = 0;
};

#endif
