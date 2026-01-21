//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#ifndef RME_GL_CONTEXT_H_
#define RME_GL_CONTEXT_H_

#include <glad/glad.h>
#include "../types/render_types.h"

namespace rme {
	namespace render {
		namespace gl {

			/// OpenGL context setup and management
			/// Handles viewport, projection, and per-frame setup
			class GLContext {
			public:
				/// Setup viewport and projection for 2D rendering
				/// @param width Viewport width
				/// @param height Viewport height
				/// @param zoom Zoom level (1.0 = 100%)
				static void setupViewport(int width, int height, float zoom = 1.0f);

				/// Setup orthographic projection matching viewport
				/// @param width Viewport width in pixels
				/// @param height Viewport height in pixels
				static void setupOrtho(int width, int height);

				/// Begin a new frame (clear, reset state)
				/// @param r Clear color red (0-1)
				/// @param g Clear color green (0-1)
				/// @param b Clear color blue (0-1)
				static void beginFrame(float r = 0.0f, float g = 0.0f, float b = 0.0f);

				/// End the current frame
				static void endFrame();

				/// Setup standard alpha blending
				static void setupBlending();

				/// Get OpenGL version info
				/// @return Version string
				static const char* getVersionString();

				/// Get OpenGL renderer info
				/// @return Renderer string
				static const char* getRendererString();

				/// Check for and clear GL errors
				/// @return true if no errors
				static bool checkErrors();

				/// Log current GL error (debug helper)
				static void logGLError(const char* context);

			private:
				GLContext() = delete; // Static class
			};

		} // namespace gl
	} // namespace render
} // namespace rme

#endif // RME_GL_CONTEXT_H_
