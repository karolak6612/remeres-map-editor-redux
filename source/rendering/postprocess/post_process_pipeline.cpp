//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "rendering/postprocess/post_process_pipeline.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/render_view.h"
#include "rendering/core/shader_program.h"
#include "rendering/postprocess/post_process_manager.h"
#include <algorithm>
#include <spdlog/spdlog.h>

PostProcessPipeline::PostProcessPipeline(PostProcessManager& manager)
	: post_process_manager_(manager) {}

PostProcessPipeline::~PostProcessPipeline() = default;

void PostProcessPipeline::Initialize(const std::string& default_vertex_source) {
	if (pp_vao) {
		return;
	}

	post_process_manager_.Initialize(default_vertex_source);

	pp_vao = std::make_unique<GLVertexArray>();
	pp_vbo = std::make_unique<GLBuffer>();
	pp_ebo = std::make_unique<GLBuffer>();

	static constexpr float quadVertices[] = {
		// positions   // texCoords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f,
	};

	static constexpr unsigned int quadIndices[] = { 0, 1, 2, 0, 2, 3 };

	glNamedBufferStorage(pp_vbo->GetID(), sizeof(quadVertices), quadVertices, 0);
	glNamedBufferStorage(pp_ebo->GetID(), sizeof(quadIndices), quadIndices, 0);

	glVertexArrayVertexBuffer(pp_vao->GetID(), 0, pp_vbo->GetID(), 0, 4 * sizeof(float));
	glVertexArrayElementBuffer(pp_vao->GetID(), pp_ebo->GetID());

	glEnableVertexArrayAttrib(pp_vao->GetID(), 0);
	glVertexArrayAttribFormat(pp_vao->GetID(), 0, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(pp_vao->GetID(), 0, 0);

	glEnableVertexArrayAttrib(pp_vao->GetID(), 1);
	glVertexArrayAttribFormat(pp_vao->GetID(), 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
	glVertexArrayAttribBinding(pp_vao->GetID(), 1, 0);
}

bool PostProcessPipeline::BeginCapture(const RenderView& view, const DrawingOptions& options) {
	bool use_fbo = (options.screen_shader_name != ShaderNames::NONE) || options.anti_aliasing;
	if (use_fbo) {
		UpdateFBO(view, options);
		return true;
	}
	return false;
}

void PostProcessPipeline::EndCaptureAndDraw(const RenderView& view, const DrawingOptions& options) {
	if (!scale_fbo || !pp_vao) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return;
	}

	ShaderProgram* shader = post_process_manager_.GetEffect(options.screen_shader_name);
	if (!shader) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(view.viewport_x, view.viewport_y, view.screensize_x, view.screensize_y);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shader->Use();
	shader->SetInt("u_Texture", 0);
	shader->SetVec2("u_TextureSize", glm::vec2(fbo_width, fbo_height));

	glBindTextureUnit(0, scale_texture->GetID());
	glBindVertexArray(pp_vao->GetID());
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	shader->Unuse();
}

bool PostProcessPipeline::UpdateFBO(const RenderView& view, const DrawingOptions& options) {
	float scale_factor = view.zoom < 1.0f ? view.zoom : 1.0f;

	int target_w = std::max(1, static_cast<int>(view.screensize_x * scale_factor));
	int target_h = std::max(1, static_cast<int>(view.screensize_y * scale_factor));

	bool fbo_resized = false;
	if (fbo_width != target_w || fbo_height != target_h || !scale_fbo) {
		fbo_width = target_w;
		fbo_height = target_h;
		scale_fbo = std::make_unique<GLFramebuffer>();
		scale_texture = std::make_unique<GLTextureResource>(GL_TEXTURE_2D);

		glTextureStorage2D(scale_texture->GetID(), 1, GL_RGBA8, fbo_width, fbo_height);
		glTextureParameteri(scale_texture->GetID(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(scale_texture->GetID(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glNamedFramebufferTexture(scale_fbo->GetID(), GL_COLOR_ATTACHMENT0, scale_texture->GetID(), 0);
		GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
		glNamedFramebufferDrawBuffers(scale_fbo->GetID(), 1, drawBuffers);

		GLenum status = glCheckNamedFramebufferStatus(scale_fbo->GetID(), GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			spdlog::error("PostProcessPipeline: Framebuffer incomplete (status: {:#x})", status);
			scale_fbo.reset();
			scale_texture.reset();
			fbo_width = 0;
			fbo_height = 0;
			return false;
		}
		fbo_resized = true;
	}

	if (scale_texture && (fbo_resized || options.anti_aliasing != m_lastAaMode)) {
		GLenum filter = options.anti_aliasing ? GL_LINEAR : GL_NEAREST;
		glTextureParameteri(scale_texture->GetID(), GL_TEXTURE_MIN_FILTER, filter);
		glTextureParameteri(scale_texture->GetID(), GL_TEXTURE_MAG_FILTER, filter);
		m_lastAaMode = options.anti_aliasing;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, scale_fbo->GetID());
	glViewport(0, 0, fbo_width, fbo_height);
	return true;
}
