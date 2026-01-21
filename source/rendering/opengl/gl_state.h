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

#ifndef RME_GL_STATE_H_
#define RME_GL_STATE_H_

#include <glad/glad.h>
#include "../types/color.h"

namespace rme {
	namespace render {
		namespace gl {

			/// OpenGL state manager with caching to reduce redundant state changes
			/// Implements the State pattern for OpenGL context state
			class GLState {
			public:
				/// Get the singleton instance
				static GLState& instance();

				// Texture binding with caching
				/// Bind a 2D texture, skipping if already bound
				void bindTexture2D(GLuint textureId);

				/// Unbind any currently bound 2D texture
				void unbindTexture2D();

				/// Get the currently bound texture
				[[nodiscard]] GLuint getCurrentTexture() const noexcept {
					return currentTexture_;
				}

				// State toggles
				/// Enable blending
				void enableBlend();

				/// Disable blending
				void disableBlend();

				/// Enable 2D texturing
				void enableTexture2D();

				/// Disable 2D texturing
				void disableTexture2D();

				/// Enable line stipple pattern
				void enableLineStipple();

				/// Disable line stipple pattern
				void disableLineStipple();

				// Blend modes
				/// Set standard alpha blending (src_alpha, one_minus_src_alpha)
				void setBlendAlpha();

				/// Set additive blending
				void setBlendAdditive();

				/// Set light blending mode (dst_color, one_minus_src_alpha)
				void setBlendLight();

				/// Set multiply blending (dst_color, zero)
				void setBlendMultiply();

				// Color state
				/// Set the current color using Color struct
				void setColor(const Color& color);

				/// Set the current color using RGBA components
				void setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

				/// Set the current color using float components (0.0-1.0)
				void setColorF(float r, float g, float b, float a = 1.0f);

				// Matrix operations (legacy, but needed for compatibility)
				/// Push the current matrix onto the stack
				void pushMatrix();

				/// Pop the matrix from the stack
				void popMatrix();

				/// Load identity matrix
				void loadIdentity();

				/// Translate the current matrix
				void translate(float x, float y, float z = 0.0f);

				/// Scale the current matrix
				void scale(float x, float y, float z = 1.0f);

				// Line attributes
				/// Set line width
				void setLineWidth(float width);

				/// Set line stipple pattern
				void setLineStipple(GLint factor, GLushort pattern);

				// Reset all cached state
				/// Reset internal cache (call when context changes)
				void resetCache();

				// Prevent copying
				GLState(const GLState&) = delete;
				GLState& operator=(const GLState&) = delete;

			private:
				GLState();
				~GLState() = default;

				// Cached state
				GLuint currentTexture_ = 0;
				bool blendEnabled_ = false;
				bool texture2DEnabled_ = false;
				bool lineStippleEnabled_ = false;
				float lineWidth_ = 1.0f;
			};

		} // namespace gl
	} // namespace render
} // namespace rme

#endif // RME_GL_STATE_H_
