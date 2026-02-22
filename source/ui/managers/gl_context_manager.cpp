#include "app/main.h"
#include "ui/managers/gl_context_manager.h"
#include <spdlog/spdlog.h>

GLContextManager g_gl_context;

GLContextManager::GLContextManager() :
	OGLContext(nullptr) {
}

GLContextManager::~GLContextManager() {
	delete OGLContext;
}

wxGLContext* GLContextManager::GetGLContext(wxGLCanvas* win) {
	if (OGLContext == nullptr) {
#ifdef __WXOSX__
		OGLContext = new wxGLContext(win, nullptr);
#else
		wxGLContextAttrs ctxAttrs;
		ctxAttrs.PlatformDefaults().CoreProfile().MajorVersion(3).MinorVersion(3).EndList();
		OGLContext = newd wxGLContext(win, nullptr, &ctxAttrs);
		spdlog::info("GLContextManager: Created new OpenGL 3.3 Core Profile context");
#endif
		if (OGLContext && OGLContext->IsOK()) {
			// Initialize GLAD for the new context
			win->SetCurrent(*OGLContext);
			if (!gladLoadGL()) {
				spdlog::error("GLContextManager: Failed to initialize GLAD!");
			} else {
				spdlog::info("GLContextManager: GLAD initialized successfully");
			}
		} else {
			spdlog::error("GLContextManager: Failed to create OpenGL context!");
		}
	}

	return OGLContext;
}
