#ifndef RME_GL_CONTEXT_MANAGER_H_
#define RME_GL_CONTEXT_MANAGER_H_

#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <memory>

#include <set>

class GLContextManager {
public:
	GLContextManager();
	~GLContextManager();

	wxGLContext* GetGLContext(wxGLCanvas* win);

	// Tries to make the context current on the preferred canvas, or falls back to
	// other available canvases if the preferred one is hidden (to avoid assertions).
	bool EnsureContextCurrent(wxGLContext& ctx, wxGLCanvas* preferredCanvas = nullptr);

	void RegisterCanvas(wxGLCanvas* canvas);
	void UnregisterCanvas(wxGLCanvas* canvas);

	void SetFallbackCanvas(wxGLCanvas* canvas) {
		m_fallbackCanvas = canvas;
	}

private:
	std::unique_ptr<wxGLContext> OGLContext;
	std::set<wxGLCanvas*> m_canvases;
	wxGLCanvas* m_fallbackCanvas = nullptr;
};

extern GLContextManager g_gl_context;

#endif
