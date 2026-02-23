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

bool GLContextManager::EnsureContextCurrent(wxGLContext& ctx, wxGLCanvas* preferredCanvas) {
	// 1. Try preferred canvas if valid and shown
	if (preferredCanvas && preferredCanvas->IsShown()) {
		if (preferredCanvas->SetCurrent(ctx)) {
			return true;
		}
	}

	// 2. Try the window associated with the context itself
	if (wxWindow* ctxWin = ctx.GetWindow()) {
		if (ctxWin != preferredCanvas && ctxWin->IsShown()) {
			if (auto* canvas = dynamic_cast<wxGLCanvas*>(ctxWin)) {
				if (canvas->SetCurrent(ctx)) {
					return true;
				}
			}
		}
	}

	// 3. Try global context's window as fallback
	if (OGLContext) {
		if (wxWindow* globalWin = OGLContext->GetWindow()) {
			if (globalWin != preferredCanvas && globalWin != ctx.GetWindow() && globalWin->IsShown()) {
				if (auto* globalCanvas = dynamic_cast<wxGLCanvas*>(globalWin)) {
					// Assumes compatible pixel format
					if (globalCanvas->SetCurrent(ctx)) {
						return true;
					}
				}
			}
		}
	}

	return false;
}
