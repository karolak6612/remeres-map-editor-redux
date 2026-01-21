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

#ifndef RME_I_FONT_RENDERER_H_
#define RME_I_FONT_RENDERER_H_

#include <cstdint>
#include <string>
#include <string_view>
#include "../types/color.h"

namespace rme {
	namespace render {

		/// Interface for font/text rendering
		/// Abstracts font rendering to allow replacing GLUT bitmap fonts
		class IFontRenderer {
		public:
			virtual ~IFontRenderer() = default;

			/// Render a single character at current raster position
			/// @param c Character to render
			virtual void drawChar(char c) = 0;

			/// Render a string at the specified position
			/// @param x Screen X position
			/// @param y Screen Y position
			/// @param text Text to render
			/// @param color Text color
			virtual void drawString(int x, int y, std::string_view text, const Color& color = colors::White) = 0;

			/// Get the width of a character in pixels
			/// @param c Character to measure
			/// @return Width in pixels
			[[nodiscard]] virtual int getCharWidth(char c) const = 0;

			/// Get the width of a string in pixels
			/// @param text Text to measure
			/// @return Width in pixels
			[[nodiscard]] virtual int getStringWidth(std::string_view text) const = 0;

			/// Get the font height in pixels
			[[nodiscard]] virtual int getFontHeight() const noexcept = 0;

			/// Check if the font is loaded and ready
			[[nodiscard]] virtual bool isReady() const noexcept = 0;

			// Prevent copying
			IFontRenderer(const IFontRenderer&) = delete;
			IFontRenderer& operator=(const IFontRenderer&) = delete;

		protected:
			IFontRenderer() = default;
			IFontRenderer(IFontRenderer&&) = default;
			IFontRenderer& operator=(IFontRenderer&&) = default;
		};

	} // namespace render
} // namespace rme

#endif // RME_I_FONT_RENDERER_H_
