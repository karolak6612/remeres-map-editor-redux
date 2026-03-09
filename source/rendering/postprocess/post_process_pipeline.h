//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_POSTPROCESS_PIPELINE_H_
#define RME_RENDERING_POSTPROCESS_PIPELINE_H_

#include "rendering/core/gl_resources.h"
#include <memory>
#include <string>

struct RenderView;
struct DrawingOptions;
class PostProcessManager;

class PostProcessPipeline {
public:
	explicit PostProcessPipeline(PostProcessManager& manager);
	~PostProcessPipeline();

	PostProcessPipeline(const PostProcessPipeline&) = delete;
	PostProcessPipeline& operator=(const PostProcessPipeline&) = delete;

	void Initialize(const std::string& default_vertex_source);

	// Binds the FBO if post-processing is needed. Returns true if FBO is active.
	bool BeginCapture(const RenderView& view, const DrawingOptions& options);

	// Resolves the FBO to the default framebuffer with the selected effect.
	void EndCaptureAndDraw(const RenderView& view, const DrawingOptions& options);

private:
	bool UpdateFBO(const RenderView& view, const DrawingOptions& options);

	std::unique_ptr<GLFramebuffer> scale_fbo;
	std::unique_ptr<GLTextureResource> scale_texture;
	int fbo_width = 0;
	int fbo_height = 0;
	bool m_lastAaMode = false;

	std::unique_ptr<GLVertexArray> pp_vao;
	std::unique_ptr<GLBuffer> pp_vbo;
	std::unique_ptr<GLBuffer> pp_ebo;

	PostProcessManager& post_process_manager_;
};

#endif
