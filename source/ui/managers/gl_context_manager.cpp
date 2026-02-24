#include "app/main.h"
#include "ui/managers/gl_context_manager.h"
#include <spdlog/spdlog.h>

GLContextManager g_gl_context;

GLContextManager::GLContextManager() = default;
GLContextManager::~GLContextManager() = default;

wxGLContext* GLContextManager::GetGLContext(wxGLCanvas* win) {
	if (win) {
		RegisterCanvas(win);
	}

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
		if (win) {
			win->SetCurrent(*OGLContext);
			if (!gladLoadGL()) {
				spdlog::error("GLContextManager: Failed to initialize GLAD!");
			} else {
				spdlog::info("GLContextManager: GLAD initialized successfully");
			}
		}
	}

	return OGLContext.get();
}

void GLContextManager::RegisterCanvas(wxGLCanvas* canvas) {
	if (canvas) {
		m_canvases.insert(canvas);
	}
}

void GLContextManager::UnregisterCanvas(wxGLCanvas* canvas) {
	m_canvases.erase(canvas);
	if (m_fallbackCanvas == canvas) {
		m_fallbackCanvas = nullptr;
	}
}

bool GLContextManager::EnsureContextCurrent(wxGLContext& ctx, wxGLCanvas* preferredCanvas) {
	// 1. Try preferred canvas if valid and shown
	if (preferredCanvas && preferredCanvas->IsShownOnScreen()) {
		if (preferredCanvas->SetCurrent(ctx)) {
			return true;
		}
	}

	// 2. Try the designated fallback canvas if it's shown
	if (m_fallbackCanvas && m_fallbackCanvas != preferredCanvas && m_fallbackCanvas->IsShownOnScreen()) {
		wxLogNull logNo; // Suppress "Invalid Pixel Format" or other errors if incompatible
		if (m_fallbackCanvas->SetCurrent(ctx)) {
			return true;
		}
	}

	// 3. Try any other registered canvas as a last resort
	for (auto* canvas : m_canvases) {
		if (canvas != preferredCanvas && canvas != m_fallbackCanvas && canvas->IsShownOnScreen()) {
			wxLogNull logNo;
			if (canvas->SetCurrent(ctx)) {
				return true;
			}
		}
	}

	return false;
}
