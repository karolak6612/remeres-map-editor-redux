#include "rendering/core/primitive_renderer.h"

PrimitiveRenderer::PrimitiveRenderer(VulkanContext* vkContext) : vkContext_(vkContext) {}
PrimitiveRenderer::~PrimitiveRenderer() {}
void PrimitiveRenderer::initialize(VkRenderPass renderPass, uint32_t subpass) {}
void PrimitiveRenderer::shutdown() {}
void PrimitiveRenderer::setProjectionMatrix(const glm::mat4& projection) {}
void PrimitiveRenderer::drawTriangle(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, const glm::vec4& color) {}
void PrimitiveRenderer::drawLine(const glm::vec2& p1, const glm::vec2& p2, const glm::vec4& color) {}
void PrimitiveRenderer::drawRect(const glm::vec4& rect, const glm::vec4& color) {}
void PrimitiveRenderer::drawBox(const glm::vec4& rect, const glm::vec4& color, float thickness) {}
void PrimitiveRenderer::flush(VkCommandBuffer cmdBuffer) {}
void PrimitiveRenderer::flushTriangles(VkCommandBuffer cmdBuffer) {}
void PrimitiveRenderer::flushLines(VkCommandBuffer cmdBuffer) {}
void PrimitiveRenderer::flushPrimitives(VkCommandBuffer cmdBuffer, std::vector<Vertex>& vertices, uint32_t vertexCount) {}
