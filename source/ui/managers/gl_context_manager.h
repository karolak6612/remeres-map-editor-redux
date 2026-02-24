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

	static wxGLAttributes& GetDefaultAttributes();

private:
	std::unique_ptr<wxGLContext> OGLContext;
	bool m_gladInitialized = false;
};

extern GLContextManager g_gl_context;

#endif
