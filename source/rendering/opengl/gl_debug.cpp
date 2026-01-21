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

#include "gl_debug.h"
#include <cstdio>

namespace rme {
	namespace render {
		namespace gl {

			bool GLDebug::debugEnabled_ = false;
			bool GLDebug::breakOnError_ = false;

			bool GLDebug::checkError(const char* context) {
				GLenum error = glGetError();
				if (error == GL_NO_ERROR) {
					return true;
				}

				while (error != GL_NO_ERROR) {
					fprintf(stderr, "GL Error [%s]: %s (0x%04X)\n", context, getErrorString(error), error);
					error = glGetError();
				}

				if (breakOnError_) {
#ifdef _MSC_VER
					__debugbreak();
#elif defined(__GNUC__)
					__builtin_trap();
#endif
				}

				return false;
			}

			const char* GLDebug::getErrorString(GLenum error) {
				switch (error) {
					case GL_NO_ERROR:
						return "NO_ERROR";
					case GL_INVALID_ENUM:
						return "INVALID_ENUM";
					case GL_INVALID_VALUE:
						return "INVALID_VALUE";
					case GL_INVALID_OPERATION:
						return "INVALID_OPERATION";
					case GL_STACK_OVERFLOW:
						return "STACK_OVERFLOW";
					case GL_STACK_UNDERFLOW:
						return "STACK_UNDERFLOW";
					case GL_OUT_OF_MEMORY:
						return "OUT_OF_MEMORY";
					default:
						return "UNKNOWN_ERROR";
				}
			}

			void GLDebug::logState() {
				GLint viewport[4];
				glGetIntegerv(GL_VIEWPORT, viewport);
				fprintf(stderr, "GL State:\n");
				fprintf(stderr, "  Viewport: %d, %d, %d, %d\n", viewport[0], viewport[1], viewport[2], viewport[3]);

				GLint matrixMode;
				glGetIntegerv(GL_MATRIX_MODE, &matrixMode);
				fprintf(stderr, "  Matrix Mode: 0x%04X\n", matrixMode);

				GLboolean blend = glIsEnabled(GL_BLEND);
				fprintf(stderr, "  Blend: %s\n", blend ? "enabled" : "disabled");

				GLboolean texture2d = glIsEnabled(GL_TEXTURE_2D);
				fprintf(stderr, "  Texture 2D: %s\n", texture2d ? "enabled" : "disabled");

				GLint boundTexture;
				glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture);
				fprintf(stderr, "  Bound Texture: %d\n", boundTexture);
			}

			void GLDebug::logInfo() {
				const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
				const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
				const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));

				fprintf(stderr, "OpenGL Info:\n");
				fprintf(stderr, "  Version: %s\n", version ? version : "unknown");
				fprintf(stderr, "  Renderer: %s\n", renderer ? renderer : "unknown");
				fprintf(stderr, "  Vendor: %s\n", vendor ? vendor : "unknown");
			}

			void GLDebug::enableDebugOutput() {
				// Note: ARB_debug_output requires OpenGL 4.3+
				// For legacy OpenGL, we just set a flag
				debugEnabled_ = true;
			}

			void GLDebug::disableDebugOutput() {
				debugEnabled_ = false;
			}

			void GLDebug::assertNoError(const char* context) {
#ifdef __DEBUG__
				GLenum error = glGetError();
				if (error != GL_NO_ERROR) {
					fprintf(stderr, "GL ASSERTION FAILED [%s]: %s (0x%04X)\n", context, getErrorString(error), error);
	#ifdef _MSC_VER
					__debugbreak();
	#elif defined(__GNUC__)
					__builtin_trap();
	#endif
				}
#else
				(void)context;
#endif
			}

		} // namespace gl
	} // namespace render
} // namespace rme
