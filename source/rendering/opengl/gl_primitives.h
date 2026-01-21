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

#ifndef RME_GL_PRIMITIVES_H_
#define RME_GL_PRIMITIVES_H_

#include <glad/glad.h>
#include "../types/color.h"
#include "../types/render_types.h"

namespace rme {
	namespace render {
		namespace gl {

			/// Utility class for common OpenGL drawing primitives
			/// Encapsulates immediate mode drawing calls
			class Primitives {
			public:
				// Textured quads
				/// Draw a textured quad with specified texture coordinates
				static void drawTexturedQuad(int x, int y, int w, int h, float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f);

				/// Draw a textured quad at tile size (32x32)
				static void drawTexturedTile(int x, int y);

				/// Draw a textured quad with color tint
				static void drawTexturedQuadTinted(int x, int y, int w, int h, const Color& tint, float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f);

				// Colored quads (no texture)
				/// Draw a filled quad with solid color
				static void drawFilledQuad(int x, int y, int w, int h, const Color& color);

				/// Draw a filled quad using current color
				static void drawFilledQuad(int x, int y, int w, int h);

				/// Draw a filled rect
				static void drawFilledRect(const Rect& rect, const Color& color);

				// Outline rectangles
				/// Draw a rectangle outline
				static void drawRect(int x, int y, int w, int h, const Color& color, float lineWidth = 1.0f);

				/// Draw a dashed rectangle outline
				static void drawDashedRect(int x, int y, int w, int h, const Color& color, float lineWidth = 1.0f);

				// Lines
				/// Draw a single line
				static void drawLine(int x1, int y1, int x2, int y2, const Color& color, float lineWidth = 1.0f);

				/// Draw a dashed line
				static void drawDashedLine(int x1, int y1, int x2, int y2, const Color& color, float lineWidth = 1.0f);

				// Batch line drawing (for grids)
				/// Begin a batch of lines
				static void beginLines(const Color& color, float lineWidth = 1.0f);

				/// Add a line vertex to the batch
				static void addLineVertex(int x, int y);

				/// End the line batch
				static void endLines();

				// Sprites
				/// Draw a sprite at the given position with texture ID
				/// @param x Screen X position
				/// @param y Screen Y position
				/// @param textureId OpenGL texture ID
				/// @param w Width (default: tile size)
				/// @param h Height (default: tile size)
				/// @param tint Color tint (default: white/no tint)
				static void drawSprite(int x, int y, GLuint textureId, int w = kTileSize, int h = kTileSize, const Color& tint = colors::White);

				// Utility
				/// Set scissor rectangle for clipping
				static void setScissor(int x, int y, int w, int h);

				/// Clear scissor (disable clipping)
				static void clearScissor();

			private:
				Primitives() = delete; // Static class, no instantiation
			};

		} // namespace gl
	} // namespace render
} // namespace rme

#endif // RME_GL_PRIMITIVES_H_
