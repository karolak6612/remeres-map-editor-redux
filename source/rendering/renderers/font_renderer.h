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

#ifndef RME_FONT_RENDERER_H_
#define RME_FONT_RENDERER_H_

#include "../interfaces/i_font_renderer.h"
#include <map>
#include <string>

namespace rme {
	namespace render {

		/// Implementation of IFontRenderer using texture-mapped fonts
		/// Replaces GLUT bitmap fonts with modern OpenGL rendering
		class FontRenderer : public IFontRenderer {
		public:
			FontRenderer();
			virtual ~FontRenderer();

			void initialize();
			void shutdown();

			// IFontRenderer interface
			void drawChar(char c) override;
			void drawString(int x, int y, std::string_view text, const Color& color = colors::White) override;
			int getCharWidth(char c) const override;
			int getStringWidth(std::string_view text) const override;
			int getFontHeight() const noexcept override {
				return fontHeight_;
			}
			bool isReady() const noexcept override {
				return initialized_;
			}

		private:
			struct GlyphInfo {
				float u1, v1, u2, v2;
				int width;
				int height;
				int advance;
			};

			void createFontTexture();

			bool initialized_ = false;
			uint32_t textureId_ = 0;
			int fontHeight_ = 14;
			int textureWidth_ = 0;
			int textureHeight_ = 0;
			std::map<char, GlyphInfo> glyphs_;
		};

	} // namespace render
} // namespace rme

#endif // RME_FONT_RENDERER_H_
