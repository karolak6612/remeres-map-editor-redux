#ifndef RME_RENDERING_CORE_GL_VIEWPORT_H_
#define RME_RENDERING_CORE_GL_VIEWPORT_H_

#include <glm/glm.hpp>

struct ViewState;

namespace GLViewport {
	void Apply(const ViewState& view);
	void ClearBackground(const glm::vec4& color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), bool clear_stencil = true);
};

#endif
