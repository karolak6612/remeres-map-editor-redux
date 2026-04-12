#ifndef RME_RENDERING_CORE_SHADER_PROGRAM_H_
#define RME_RENDERING_CORE_SHADER_PROGRAM_H_

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <glm/glm.hpp>

class ShaderProgram {
public:
    ShaderProgram(VkDevice device);
    ~ShaderProgram();

    // Instead of loading source strings, we'll load SPIR-V binaries (from byte arrays or files)
    bool LoadSPIRV(const std::vector<uint32_t>& vertSpv, const std::vector<uint32_t>& fragSpv);

    // Creates the graphics pipeline. Needs layout, render pass, etc.
    bool BuildPipeline(VkRenderPass renderPass, uint32_t subpass, VkPipelineLayout layout,
                       const std::vector<VkVertexInputBindingDescription>& vertexBindings,
                       const std::vector<VkVertexInputAttributeDescription>& vertexAttributes);

    void Bind(VkCommandBuffer cmdBuffer) const;

    VkPipeline GetPipeline() const { return pipeline_; }

    bool IsValid() const { return pipeline_ != VK_NULL_HANDLE; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkShaderModule vertModule_ = VK_NULL_HANDLE;
    VkShaderModule fragModule_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;

    VkShaderModule CreateShaderModule(const std::vector<uint32_t>& code);
};

#endif
