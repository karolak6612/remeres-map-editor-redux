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

#ifndef RME_GL_DEBUG_H_
#define RME_GL_DEBUG_H_

#include <glad/glad.h>
#include <string>

namespace rme {
	namespace render {
		namespace gl {

			/// OpenGL debug utilities
			/// Provides error checking and debug output
			class GLDebug {
			public:
				/// Check for GL errors and log them
				/// @param context Description of where the check is happening
				/// @return true if no errors, false if errors found
				static bool checkError(const char* context);

				/// Get the string representation of a GL error code
				/// @param error GL error code
				/// @return Error string
				static const char* getErrorString(GLenum error);

				/// Log current GL state (for debugging)
				static void logState();

				/// Log GL version and renderer info
				static void logInfo();

				/// Enable OpenGL debug output (if supported)
				static void enableDebugOutput();

				/// Disable OpenGL debug output
				static void disableDebugOutput();

				/// Check if debug output is enabled
				[[nodiscard]] static bool isDebugEnabled() noexcept {
					return debugEnabled_;
				}

				/// Set whether to break on GL errors (for debugging)
				static void setBreakOnError(bool enable) {
					breakOnError_ = enable;
				}

				/// Assert no GL errors (debug builds only)
				/// @param context Description of where the assertion is
				static void assertNoError(const char* context);

			private:
				GLDebug() = delete; // Static class

				static bool debugEnabled_;
				static bool breakOnError_;
			};

			/// RAII helper for scoped GL error checking
			class GLErrorScope {
			public:
				explicit GLErrorScope(const char* context) : context_(context) {
					GLDebug::checkError(context_); // Clear any existing errors
				}

				~GLErrorScope() {
					GLDebug::checkError(context_);
				}

				GLErrorScope(const GLErrorScope&) = delete;
				GLErrorScope& operator=(const GLErrorScope&) = delete;

			private:
				const char* context_;
			};

/// Macro for scoped GL error checking
#ifdef __DEBUG__
	#define GL_CHECK_SCOPE(ctx) rme::render::gl::GLErrorScope _gl_scope_(ctx)
	#define GL_CHECK_ERROR(ctx) rme::render::gl::GLDebug::checkError(ctx)
#else
	#define GL_CHECK_SCOPE(ctx) (void)0
	#define GL_CHECK_ERROR(ctx) (void)0
#endif

		} // namespace gl
	} // namespace render
} // namespace rme

#endif // RME_GL_DEBUG_H_
