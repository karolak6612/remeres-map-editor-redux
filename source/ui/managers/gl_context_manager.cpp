#include "app/main.h"
#include "ui/managers/gl_context_manager.h"
#include <spdlog/spdlog.h>

GLContextManager g_gl_context;

GLContextManager::GLContextManager() = default;
GLContextManager::~GLContextManager() = default;

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
	}

	// Initialize GLAD once the window is actually shown (has a valid XID)
	if (!m_gladInitialized && win->IsShownOnScreen()) {
		win->SetCurrent(*OGLContext);
		if (!gladLoadGL()) {
			spdlog::error("GLContextManager: Failed to initialize GLAD!");
		} else {
			m_gladInitialized = true;
			spdlog::info("GLContextManager: GLAD initialized successfully");
		}
	}

	return OGLContext.get();
}

wxGLAttributes& GLContextManager::GetDefaultAttributes() {
	static wxGLAttributes attrs;
	static bool initialized = false;
	if (!initialized) {
		attrs.PlatformDefaults()
			.RGBA()
			.MinRGBA(8, 8, 8, 8)
			.DoubleBuffer()
			.Depth(24)
			.Stencil(8)
			.EndList();
		initialized = true;
	}
	return attrs;
}
