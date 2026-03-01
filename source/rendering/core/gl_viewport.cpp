#include "app/main.h"
#include <glad/glad.h>
#include "rendering/core/gl_viewport.h"
#include "rendering/core/view_state.h"

namespace GLViewport {

	void Apply(const ViewState& view) {
		glViewport(view.viewport_x, view.viewport_y, view.screensize_x, view.screensize_y);
	}

	void ClearBackground(const glm::vec4& color, bool clear_stencil) {
		glClearColor(color.r, color.g, color.b, color.a);
		GLbitfield mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
		if (clear_stencil) {
			mask |= GL_STENCIL_BUFFER_BIT;
		}
		glClear(mask);
	}

} // namespace GLViewport
