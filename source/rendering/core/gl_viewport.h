#ifndef RME_RENDERING_CORE_GL_VIEWPORT_H_
#define RME_RENDERING_CORE_GL_VIEWPORT_H_

struct ViewState;

namespace GLViewport {
	void Apply(const ViewState& view);
	void ClearBackground();
};

#endif
