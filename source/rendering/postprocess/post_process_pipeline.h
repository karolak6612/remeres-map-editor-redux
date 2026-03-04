#ifndef RME_RENDERING_POSTPROCESS_PIPELINE_H_
#define RME_RENDERING_POSTPROCESS_PIPELINE_H_

#include "rendering/core/drawing_options.h"
#include "rendering/core/gl_resources.h"
#include "rendering/core/view_state.h"
#include <memory>

class PostProcessPipeline {
public:
    PostProcessPipeline();
    ~PostProcessPipeline();

    PostProcessPipeline(const PostProcessPipeline&) = delete;
    PostProcessPipeline& operator=(const PostProcessPipeline&) = delete;

    void Initialize();

    // Binds the FBO if needed and applicable, returning true if bound.
    bool BeginCapture(const ViewState& view, const DrawingOptions& options);

    // Draws the FBO to the default framebuffer.
    void EndCaptureAndDraw(const ViewState& view, const DrawingOptions& options);

private:
    bool UpdateFBO(const ViewState& view, const DrawingOptions& options);

    std::unique_ptr<GLFramebuffer> scale_fbo;
    std::unique_ptr<GLTextureResource> scale_texture;
    int fbo_width = 0;
    int fbo_height = 0;
    bool m_lastAaMode = false;

    std::unique_ptr<GLVertexArray> pp_vao;
    std::unique_ptr<GLBuffer> pp_vbo;
    std::unique_ptr<GLBuffer> pp_ebo;
};

#endif
