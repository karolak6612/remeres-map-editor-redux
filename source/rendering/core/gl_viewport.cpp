#include "app/main.h"
#include "rendering/core/gl_viewport.h"
#include "rendering/core/view_state.h"

namespace GLViewport {

	void Apply(const ViewState& view) {
		glViewport(view.viewport_x, view.viewport_y, view.screensize_x, view.screensize_y);
	}

	void ClearBackground() {
		// Black Background
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

} // namespace GLViewport
