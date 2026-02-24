#ifndef RME_RENDERING_UI_SCOPED_WX_GL_CONTEXT_H_
#define RME_RENDERING_UI_SCOPED_WX_GL_CONTEXT_H_

#include <wx/glcanvas.h>

/**
 * @brief RAII wrapper for ensuring a wxGLContext is current on a wxGLCanvas.
 */
class ScopedWxGLContext {
public:
	[[nodiscard]] ScopedWxGLContext(wxGLCanvas* canvas, wxGLContext* context) :
		canvas_(canvas),
		context_(context) {
		if (canvas_ && context_) {
			canvas_->SetCurrent(*context_);
		}
	}

	~ScopedWxGLContext() {
		// wxWidgets does not provide a reliable way to query and restore the previous context.
		// The purpose of this scope is to ensure *this* context is current for the duration of the block.
	}

	ScopedWxGLContext(const ScopedWxGLContext&) = delete;
	ScopedWxGLContext& operator=(const ScopedWxGLContext&) = delete;

private:
	wxGLCanvas* canvas_;
	wxGLContext* context_;
};

#endif // RME_RENDERING_UI_SCOPED_WX_GL_CONTEXT_H_
