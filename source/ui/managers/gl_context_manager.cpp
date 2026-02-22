#include "app/main.h"
#include "ui/managers/gl_context_manager.h"
#include <spdlog/spdlog.h>

GLContextManager g_gl_context;

GLContextManager::GLContextManager() {
}

GLContextManager::~GLContextManager() {
}

wxGLContext* GLContextManager::GetGLContext(wxGLCanvas* win) {
	if (!OGLContext) {
#ifdef __WXOSX__
		OGLContext = std::make_unique<wxGLContext>(win, nullptr);
#else
		wxGLContextAttrs ctxAttrs;
		ctxAttrs.PlatformDefaults().CoreProfile().MajorVersion(4).MinorVersion(5).EndList();
		OGLContext = std::make_unique<wxGLContext>(win, nullptr, &ctxAttrs);
		spdlog::info("GLContextManager: Created new OpenGL 4.5 Core Profile context");
#endif
		// Initialize GLAD for the new context
		win->SetCurrent(*OGLContext);
		if (!gladLoadGL()) {
			spdlog::error("GLContextManager: Failed to initialize GLAD!");
		} else {
			spdlog::info("GLContextManager: GLAD initialized successfully");
		}
	}

	return OGLContext.get();
}
