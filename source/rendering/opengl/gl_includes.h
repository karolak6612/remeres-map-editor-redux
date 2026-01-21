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

#ifndef RME_GL_INCLUDES_H_
#define RME_GL_INCLUDES_H_

// Note: GLAD is now included in main.h BEFORE any OpenGL/wxWidgets headers
// This file provides helper functions for GLAD initialization

// GLAD header should already be included via main.h
// Only include if not already defined (for standalone usage)
#ifndef __glad_h_
	#include <glad/glad.h>
#endif

namespace rme {
	namespace render {
		namespace gl {

			/// Initialize GLAD OpenGL loader
			/// Must be called after OpenGL context is created
			/// @return true if initialization succeeded
			inline bool initializeGLAD() {
				static bool initialized = false;
				if (!initialized) {
					if (!gladLoadGL()) {
						return false;
					}
					initialized = true;
				}
				return true;
			}

			/// Check if GLAD has been initialized
			inline bool isGLADInitialized() {
				return gladLoadGL != nullptr;
			}

		} // namespace gl
	} // namespace render
} // namespace rme

#endif // RME_GL_INCLUDES_H_
