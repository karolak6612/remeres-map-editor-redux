//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include "rendering/postprocess/post_process_pipeline.h"
#include "rendering/postprocess/post_process_manager.h"
#include "rendering/core/gl_resources.h"
#include "rendering/core/shader_program.h"
#include "rendering/core/render_view.h"
#include "rendering/core/render_settings.h"

#include <algorithm>
#include <spdlog/spdlog.h>
#include <glm/glm.hpp>

static const char* screen_vert = R"(
#version 450 core
layout(location = 0) in vec2 aPos; // -1..1
layout(location = 1) in vec2 aTexCoord; // 0..1

out vec2 vTexCoord;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
)";

PostProcessPipeline::PostProcessPipeline() : post_process_mgr_(std::make_unique<PostProcessManager>()) {
	post_process_mgr_->LoadFromRegistry();
}

PostProcessPipeline::~PostProcessPipeline() = default;

std::vector<std::string> PostProcessPipeline::GetEffectNames() const {
	return post_process_mgr_->GetEffectNames();
}

void PostProcessPipeline::EnsureInitialized() {
	if (pp_vao_) {
		return;
	}

	post_process_mgr_->Initialize(screen_vert);

	pp_vao_ = std::make_unique<GLVertexArray>();
	pp_vbo_ = std::make_unique<GLBuffer>();
	pp_ebo_ = std::make_unique<GLBuffer>();

	float quadVertices[] = {
		// positions   // texCoords
		-1.0f, 1.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f
	};

	unsigned int quadIndices[] = {
		0, 1, 2,
		0, 2, 3
	};

	glNamedBufferStorage(pp_vbo_->GetID(), sizeof(quadVertices), quadVertices, 0);
	glNamedBufferStorage(pp_ebo_->GetID(), sizeof(quadIndices), quadIndices, 0);

	glVertexArrayVertexBuffer(pp_vao_->GetID(), 0, pp_vbo_->GetID(), 0, 4 * sizeof(float));
	glVertexArrayElementBuffer(pp_vao_->GetID(), pp_ebo_->GetID());

	glEnableVertexArrayAttrib(pp_vao_->GetID(), 0);
	glVertexArrayAttribFormat(pp_vao_->GetID(), 0, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(pp_vao_->GetID(), 0, 0);

	glEnableVertexArrayAttrib(pp_vao_->GetID(), 1);
	glVertexArrayAttribFormat(pp_vao_->GetID(), 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
	glVertexArrayAttribBinding(pp_vao_->GetID(), 1, 0);
}

bool PostProcessPipeline::Begin(const ViewState& view, const RenderSettings& options) {
	EnsureInitialized();
	bool use_fbo = (options.screen_shader_name != ShaderNames::NONE) || options.anti_aliasing;
	if (use_fbo) {
		UpdateFBO(view, options);
	}
	return use_fbo;
}

void PostProcessPipeline::End(const ViewState& view, const RenderSettings& options) {
	DrawPostProcess(view, options);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(view.viewport_x, view.viewport_y, view.screensize_x, view.screensize_y);
}

void PostProcessPipeline::DrawPostProcess(const ViewState& view, const RenderSettings& options) {
	if (!scale_fbo_ || !pp_vao_) {
		return;
	}

	ShaderProgram* shader = post_process_mgr_->GetEffect(options.screen_shader_name);
	if (!shader) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(view.viewport_x, view.viewport_y, view.screensize_x, view.screensize_y);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shader->Use();
	shader->SetInt("u_Texture", 0);
	shader->SetVec2("u_TextureSize", glm::vec2(fbo_width_, fbo_height_));

	glBindTextureUnit(0, scale_texture_->GetID());
	glBindVertexArray(pp_vao_->GetID());
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	shader->Unuse();
}

void PostProcessPipeline::UpdateFBO(const ViewState& view, const RenderSettings& options) {
	float scale_factor = view.zoom < 1.0f ? view.zoom : 1.0f;

	int target_w = std::max(1, static_cast<int>(view.screensize_x * scale_factor));
	int target_h = std::max(1, static_cast<int>(view.screensize_y * scale_factor));

	bool fbo_resized = false;
	if (fbo_width_ != target_w || fbo_height_ != target_h || !scale_fbo_) {
		fbo_width_ = target_w;
		fbo_height_ = target_h;
		scale_fbo_ = std::make_unique<GLFramebuffer>();
		scale_texture_ = std::make_unique<GLTextureResource>(GL_TEXTURE_2D);

		glTextureStorage2D(scale_texture_->GetID(), 1, GL_RGBA8, fbo_width_, fbo_height_);
		glTextureParameteri(scale_texture_->GetID(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(scale_texture_->GetID(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glNamedFramebufferTexture(scale_fbo_->GetID(), GL_COLOR_ATTACHMENT0, scale_texture_->GetID(), 0);
		GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
		glNamedFramebufferDrawBuffers(scale_fbo_->GetID(), 1, drawBuffers);

		if (fbo_width_ < 1 || fbo_height_ < 1) {
			spdlog::error("PostProcessPipeline: FBO dimension is zero ({}, {})!", fbo_width_, fbo_height_);
		}
		fbo_resized = true;
	}

	if (scale_texture_ && (fbo_resized || options.anti_aliasing != last_aa_mode_)) {
		GLenum filter = options.anti_aliasing ? GL_LINEAR : GL_NEAREST;
		glTextureParameteri(scale_texture_->GetID(), GL_TEXTURE_MIN_FILTER, filter);
		glTextureParameteri(scale_texture_->GetID(), GL_TEXTURE_MAG_FILTER, filter);
		last_aa_mode_ = options.anti_aliasing;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, scale_fbo_->GetID());
	glViewport(0, 0, fbo_width_, fbo_height_);
}
