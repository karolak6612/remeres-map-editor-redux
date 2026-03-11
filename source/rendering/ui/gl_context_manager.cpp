#include "app/main.h"

#include "rendering/ui/gl_context_manager.h"

#include "rendering/core/text_renderer.h"
#include "rendering/ui/map_display.h"
#include "ui/managers/gl_context_manager.h"

#include <glad/glad.h>
#include <nanovg_gl.h>
#include <spdlog/spdlog.h>
#include <wx/glcanvas.h>

namespace rme::rendering {

void NanoVGContextDeleter::operator()(NVGcontext* nvg) const {
	if (nvg) {
		nvgDeleteGL3(nvg);
	}
}

GLContextManager::GLContextManager(wxGLCanvas* canvas) :
	canvas_(canvas) {
	if (!canvas_) {
		return;
	}

	auto* map_canvas = dynamic_cast<MapCanvas*>(canvas_);
	gl_context_ = std::make_unique<wxGLContext>(canvas_, map_canvas ? map_canvas->GetSharedGLContext() : nullptr);
	if (!gl_context_ || !gl_context_->IsOK()) {
		spdlog::error("MapCanvas: Failed to create wxGLContext");
		gl_context_.reset();
	}
}

GLContextManager::~GLContextManager() {
	if (gl_context_ && MakeCurrent()) {
		nvg_.reset();
	}
	if (canvas_) {
		g_gl_context.UnregisterCanvas(canvas_);
	}
}

bool GLContextManager::MakeCurrent() {
	if (!gl_context_) {
		return false;
	}

	if (g_gl_context.EnsureContextCurrent(*gl_context_, canvas_)) {
		g_gl_context.SetFallbackCanvas(canvas_);
		return true;
	}
	return false;
}

NVGcontext* GLContextManager::GetNVG() {
	EnsureNanoVG();
	return nvg_.get();
}

void GLContextManager::EnsureNanoVG() {
	if (nvg_ || !MakeCurrent()) {
		return;
	}

	if (!gladLoadGL()) {
		spdlog::error("MapCanvas: Failed to initialize GLAD");
		return;
	}

	nvg_.reset(nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES));
	if (!nvg_) {
		spdlog::error("MapCanvas: Failed to initialize NanoVG");
		return;
	}

	TextRenderer::LoadFont(nvg_.get());
}

} // namespace rme::rendering
