#include "rendering/core/multi_draw_indirect_renderer.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <utility>

MultiDrawIndirectRenderer::MultiDrawIndirectRenderer() {
	commands_.reserve(MAX_COMMANDS);
}

MultiDrawIndirectRenderer::~MultiDrawIndirectRenderer() {
	cleanup();
}

MultiDrawIndirectRenderer::MultiDrawIndirectRenderer(MultiDrawIndirectRenderer&& other) noexcept
	:
	commands_(std::move(other.commands_)),
	command_buffer_(std::move(other.command_buffer_)), available_(other.available_), initialized_(other.initialized_) {
	other.available_ = false;
	other.initialized_ = false;
}

MultiDrawIndirectRenderer& MultiDrawIndirectRenderer::operator=(MultiDrawIndirectRenderer&& other) noexcept {
	if (this != &other) {
		cleanup();
		commands_ = std::move(other.commands_);
		command_buffer_ = std::move(other.command_buffer_);
		available_ = other.available_;
		initialized_ = other.initialized_;
		other.available_ = false;
		other.initialized_ = false;
	}
	return *this;
}

bool MultiDrawIndirectRenderer::initialize() {
	if (initialized_) {
		return true;
	}

	// Runtime-only check - function pointer is set by GLAD if GL 4.3+ is available
	available_ = (vkCmdDrawIndexedIndirect != nullptr);

	if (!available_) {
		// Warning: MDI disabled
		return false;
	}

	// Create buffer for indirect commands
	command_buffer_ = std::make_unique<GLBuffer>();

	// Pre-allocate buffer storage
	// gl API removed

	initialized_ = true;
	return true;
}

void MultiDrawIndirectRenderer::cleanup() {
	command_buffer_.reset();
	commands_.clear();
	initialized_ = false;
}

void MultiDrawIndirectRenderer::clear() {
	commands_.clear();
}

void MultiDrawIndirectRenderer::addDrawCommand(uint32_t count, uint32_t instanceCount, uint32_t firstIndex, uint32_t baseVertex, uint32_t baseInstance) {
	if (commands_.size() >= MAX_COMMANDS) {
		// Max commands reached, ignoring
		return;
	}

	if (instanceCount == 0) {
		return; // Skip empty draws
	}

	DrawElementsIndirectCommand cmd;
	cmd.count = count;
	cmd.instanceCount = instanceCount;
	cmd.firstIndex = firstIndex;
	cmd.baseVertex = baseVertex;
	cmd.baseInstance = baseInstance;

	commands_.push_back(cmd);
}

void MultiDrawIndirectRenderer::upload() {
	if (commands_.empty() || !initialized_) {
		return;
	}

	// gl API removed
}

void MultiDrawIndirectRenderer::execute() {
	if (commands_.empty() || !available_ || !initialized_) {
		return;
	}

	// Bind buffer for draw call
	// gl API removed
	vkCmdDrawIndexedIndirect(0 /* GL CONST REMOVED */, 0 /* GL CONST REMOVED */,
								nullptr, // Offset 0 in bound buffer
								static_cast<int32_t>(commands_.size()), sizeof(DrawElementsIndirectCommand));
	// gl API removed
}
