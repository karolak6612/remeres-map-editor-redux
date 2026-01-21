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

#include "gl_context.h"
#include "gl_state.h"
#include <cstdio>

namespace rme {
	namespace render {
		namespace gl {

			void GLContext::setupViewport(int width, int height, float zoom) {
				glViewport(0, 0, width, height);

				// Setup projection
				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();

				// Orthographic projection with zoom
				float scaledWidth = static_cast<float>(width) * zoom;
				float scaledHeight = static_cast<float>(height) * zoom;
				glOrtho(0, scaledWidth, scaledHeight, 0, -1.0, 1.0);

				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();
			}

			void GLContext::setupOrtho(int width, int height) {
				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				glOrtho(0, width, height, 0, -1.0, 1.0);
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();
			}

			void GLContext::beginFrame(float r, float g, float b) {
				// Reset state cache at start of frame
				GLState::instance().resetCache();

				// Clear buffers
				glClearColor(r, g, b, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);

				// Reset modelview
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();
			}

			void GLContext::endFrame() {
				// Ensure all commands are executed
				glFlush();
			}

			void GLContext::setupBlending() {
				GLState::instance().enableBlend();
				GLState::instance().setBlendAlpha();
			}

			const char* GLContext::getVersionString() {
				return reinterpret_cast<const char*>(glGetString(GL_VERSION));
			}

			const char* GLContext::getRendererString() {
				return reinterpret_cast<const char*>(glGetString(GL_RENDERER));
			}

			bool GLContext::checkErrors() {
				GLenum error = glGetError();
				return error == GL_NO_ERROR;
			}

			void GLContext::logGLError(const char* context) {
				GLenum error;
				while ((error = glGetError()) != GL_NO_ERROR) {
					const char* errorStr = "Unknown";
					switch (error) {
						case GL_INVALID_ENUM:
							errorStr = "INVALID_ENUM";
							break;
						case GL_INVALID_VALUE:
							errorStr = "INVALID_VALUE";
							break;
						case GL_INVALID_OPERATION:
							errorStr = "INVALID_OPERATION";
							break;
						case GL_STACK_OVERFLOW:
							errorStr = "STACK_OVERFLOW";
							break;
						case GL_STACK_UNDERFLOW:
							errorStr = "STACK_UNDERFLOW";
							break;
						case GL_OUT_OF_MEMORY:
							errorStr = "OUT_OF_MEMORY";
							break;
					}
					fprintf(stderr, "GL Error [%s]: %s\n", context, errorStr);
				}
			}

		} // namespace gl
	} // namespace render
} // namespace rme
