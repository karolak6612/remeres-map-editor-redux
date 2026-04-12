#ifndef RME_RENDERING_CORE_PRIMITIVE_RENDERER_H_
#define RME_RENDERING_CORE_PRIMITIVE_RENDERER_H_

#include "app/main.h"
#include "rendering/core/shader_program.h"
#include "rendering/core/vulkan_context.h"
#include "rendering/core/vulkan_resources.h"
#include "rendering/core/ring_buffer.h"
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <string_view>

class PrimitiveRenderer {
public:
    PrimitiveRenderer(VulkanContext* vkContext);
    ~PrimitiveRenderer();

    void initialize(VkRenderPass renderPass, uint32_t subpass);
    void shutdown();

    void setProjectionMatrix(const glm::mat4& projection);

    void drawTriangle(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, const glm::vec4& color);
    void drawLine(const glm::vec2& p1, const glm::vec2& p2, const glm::vec4& color);

    void drawRect(const glm::vec4& rect, const glm::vec4& color);
    void drawBox(const glm::vec4& rect, const glm::vec4& color, float thickness);

    void flush(VkCommandBuffer cmdBuffer);

private:
    struct Vertex {
        glm::vec2 pos;
        glm::vec4 color;
    };

    void flushTriangles(VkCommandBuffer cmdBuffer);
    void flushLines(VkCommandBuffer cmdBuffer);
    void flushPrimitives(VkCommandBuffer cmdBuffer, std::vector<Vertex>& vertices, uint32_t vertexCount);

    VulkanContext* vkContext_ = nullptr;
    std::unique_ptr<ShaderProgram> shaderLines_;
    std::unique_ptr<ShaderProgram> shaderTriangles_;

    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;

    RingBuffer ring_buffer_;

    std::vector<Vertex> triangle_verts_;
    std::vector<Vertex> line_verts_;

    glm::mat4 projection_ { 1.0f };
    bool initialized_ = false;

    static constexpr size_t MAX_VERTICES = 10000;
};

#endif
