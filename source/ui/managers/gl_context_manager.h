#ifndef RME_GL_CONTEXT_MANAGER_H_
#define RME_GL_CONTEXT_MANAGER_H_

#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <memory>

class GLContextManager {
public:
	GLContextManager();
	~GLContextManager();

	wxGLContext* GetGLContext(wxGLCanvas* win);

	// Tries to make the context current on the preferred canvas, or falls back to
	// other available canvases if the preferred one is hidden (to avoid assertions).
	bool EnsureContextCurrent(wxGLContext& ctx, wxGLCanvas* preferredCanvas = nullptr);

private:
	std::unique_ptr<wxGLContext> OGLContext;
};

extern GLContextManager g_gl_context;

#endif
