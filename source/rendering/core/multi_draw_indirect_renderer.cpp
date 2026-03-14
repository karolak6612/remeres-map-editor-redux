#include "rendering/core/multi_draw_indirect_renderer.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <utility>

MultiDrawIndirectRenderer::MultiDrawIndirectRenderer() {
}

MultiDrawIndirectRenderer::~MultiDrawIndirectRenderer() {
	cleanup();
}

MultiDrawIndirectRenderer::MultiDrawIndirectRenderer(MultiDrawIndirectRenderer&& other) noexcept
	:
	num_commands_(other.num_commands_),
	command_buffer_(std::move(other.command_buffer_)), available_(other.available_), initialized_(other.initialized_) {
	for (size_t i = 0; i < num_commands_; ++i) {
		commands_[i] = other.commands_[i];
	}
	other.num_commands_ = 0;
	other.available_ = false;
	other.initialized_ = false;
}

MultiDrawIndirectRenderer& MultiDrawIndirectRenderer::operator=(MultiDrawIndirectRenderer&& other) noexcept {
	if (this != &other) {
		cleanup();
		num_commands_ = other.num_commands_;
		for (size_t i = 0; i < num_commands_; ++i) {
			commands_[i] = other.commands_[i];
		}
		command_buffer_ = std::move(other.command_buffer_);
		available_ = other.available_;
		initialized_ = other.initialized_;
		other.num_commands_ = 0;
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
	available_ = (glMultiDrawElementsIndirect != nullptr);

	if (!available_) {
		// Warning: MDI disabled
		return false;
	}

	// Create buffer for indirect commands
	command_buffer_ = std::make_unique<GLBuffer>();

	// Pre-allocate buffer storage
	glNamedBufferStorage(command_buffer_->GetID(), MAX_COMMANDS * sizeof(DrawElementsIndirectCommand), nullptr, GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);

	initialized_ = true;
	return true;
}

void MultiDrawIndirectRenderer::cleanup() {
	command_buffer_.reset();
	num_commands_ = 0;
	initialized_ = false;
}

void MultiDrawIndirectRenderer::clear() {
	num_commands_ = 0;
}

void MultiDrawIndirectRenderer::addDrawCommand(GLuint count, GLuint instanceCount, GLuint firstIndex, GLuint baseVertex, GLuint baseInstance) {
	if (num_commands_ >= MAX_COMMANDS) {
		// Max commands reached, ignoring
		return;
	}

	if (instanceCount == 0) {
		return; // Skip empty draws
	}

	DrawElementsIndirectCommand& cmd = commands_[num_commands_++];
	cmd.count = count;
	cmd.instanceCount = instanceCount;
	cmd.firstIndex = firstIndex;
	cmd.baseVertex = baseVertex;
	cmd.baseInstance = baseInstance;
}

void MultiDrawIndirectRenderer::upload() {
	if (num_commands_ == 0 || !initialized_) {
		return;
	}

	void* ptr = glMapNamedBufferRange(command_buffer_->GetID(), 0, num_commands_ * sizeof(DrawElementsIndirectCommand), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	if (ptr) {
		for (size_t i = 0; i < num_commands_; ++i) {
			static_cast<DrawElementsIndirectCommand*>(ptr)[i] = commands_[i];
		}
		glUnmapNamedBuffer(command_buffer_->GetID());
	}
}

void MultiDrawIndirectRenderer::execute() {
	if (num_commands_ == 0 || !available_ || !initialized_) {
		return;
	}

	// Bind buffer for draw call
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, command_buffer_->GetID());
	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT,
								nullptr, // Offset 0 in bound buffer
								static_cast<GLsizei>(num_commands_), sizeof(DrawElementsIndirectCommand));
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}
