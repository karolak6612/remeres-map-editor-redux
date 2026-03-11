#ifndef RME_RENDERING_UI_GL_CONTEXT_MANAGER_H_
#define RME_RENDERING_UI_GL_CONTEXT_MANAGER_H_

#include <memory>

class wxGLCanvas;
class wxGLContext;
struct NVGcontext;

namespace rme::rendering {

struct NanoVGContextDeleter {
	void operator()(NVGcontext* nvg) const;
};

class GLContextManager {
public:
	explicit GLContextManager(wxGLCanvas* canvas);
	~GLContextManager();

	GLContextManager(const GLContextManager&) = delete;
	GLContextManager& operator=(const GLContextManager&) = delete;

	bool MakeCurrent();
	NVGcontext* GetNVG();

	[[nodiscard]] bool IsInitialized() const {
		return gl_context_ != nullptr;
	}

private:
	void EnsureNanoVG();

	wxGLCanvas* canvas_;
	std::unique_ptr<wxGLContext> gl_context_;
	std::unique_ptr<NVGcontext, NanoVGContextDeleter> nvg_;
};

} // namespace rme::rendering

#endif
