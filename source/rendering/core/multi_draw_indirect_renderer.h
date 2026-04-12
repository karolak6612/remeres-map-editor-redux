#ifndef RME_RENDERING_CORE_MULTI_DRAW_INDIRECT_RENDERER_H_
#define RME_RENDERING_CORE_MULTI_DRAW_INDIRECT_RENDERER_H_

#include "app/main.h"
#include "rendering/core/vulkan_resources.h"
#include <vector>
#include <cstdint>
#include <memory>

/**
 * GPU command buffer for vkCmdDrawIndexedIndirect (Vulkan).
 *
 * Reduces 2-5 draw calls (one per atlas) to ONE draw call total.
 * GPU executes command buffer directly, eliminating CPUâ†’GPU sync per draw.
 */
class MultiDrawIndirectRenderer {
public:
	/**
	 * Vulkan indirect draw command structure (must match spec exactly).
	 */
	struct DrawElementsIndirectCommand {
		uint32_t count; // Number of indices per instance (6 for quad)
		uint32_t instanceCount; // Number of instances (sprites in this batch)
		uint32_t firstIndex; // Offset into EBO
		uint32_t baseVertex; // Offset into VBO
		uint32_t baseInstance; // Offset into instance buffer
	};

	static constexpr int MAX_COMMANDS = 16; // Support up to 16 atlases

	MultiDrawIndirectRenderer();
	~MultiDrawIndirectRenderer();

	// Non-copyable, but movable
	MultiDrawIndirectRenderer(const MultiDrawIndirectRenderer&) = delete;
	MultiDrawIndirectRenderer& operator=(const MultiDrawIndirectRenderer&) = delete;

	MultiDrawIndirectRenderer(MultiDrawIndirectRenderer&& other) noexcept;
	MultiDrawIndirectRenderer& operator=(MultiDrawIndirectRenderer&& other) noexcept;

	/**
	 * Initialize GPU buffer for indirect commands.
	 * @return true if successful (requires Vulkan)
	 */
	bool initialize();

	/**
	 * Cleanup GPU resources.
	 */
	void cleanup();

	/**
	 * Clear command buffer for new frame.
	 */
	void clear();

	/**
	 * Add a draw command to the buffer.
	 * @param count Indices per instance (usually 6 for quad)
	 * @param instanceCount Number of sprites in this batch
	 * @param firstIndex Offset into index buffer
	 * @param baseVertex Offset into vertex buffer
	 * @param baseInstance Offset into instance data
	 */
	void addDrawCommand(uint32_t count, uint32_t instanceCount, uint32_t firstIndex = 0, uint32_t baseVertex = 0, uint32_t baseInstance = 0);

	/**
	 * Upload command buffer to GPU.
	 */
	void upload();

	/**
	 * Execute all draw commands with single vkCmdDrawIndexedIndirect call.
	 * VAO and shader must already be bound.
	 */
	void execute();

	/**
	 * Get number of pending commands.
	 */
	size_t getCommandCount() const {
		return commands_.size();
	}

	/**
	 * Check if MDI is available (Vulkan).
	 */
	bool isAvailable() const {
		return available_;
	}

private:
	std::vector<DrawElementsIndirectCommand> commands_;
	std::unique_ptr<VulkanBuffer> command_buffer_;
	bool available_ = false;
	bool initialized_ = false;
};

#endif
