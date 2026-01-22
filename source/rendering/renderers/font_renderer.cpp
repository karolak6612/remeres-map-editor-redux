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

#include "../../logging/logger.h"
#include "../../main.h"
#include "font_renderer.h"
#include "../opengl/gl_state.h"

namespace rme {
	namespace render {

		FontRenderer::FontRenderer() = default;

		FontRenderer::~FontRenderer() {
			shutdown();
		}

		void FontRenderer::initialize() {
			LOG_RENDER_INFO("[INIT] Initializing FontRenderer...");
			if (initialized_) {
				return;
			}

			createFontTexture();
			initialized_ = true;
		}

		void FontRenderer::shutdown() {
			LOG_RENDER_INFO("[INIT] Shutting down FontRenderer...");
			if (!initialized_) {
				return;
			}

			if (textureId_ != 0) {
				glDeleteTextures(1, &textureId_);
				textureId_ = 0;
			}

			glyphs_.clear();
			initialized_ = false;
		}

		void FontRenderer::createFontTexture() {
			// Create a font texture using wxWidgets
			// We'll use a standard font like Arial or Helvetica
			wxFont font(fontHeight_, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

			// We'll render characters 32-126
			const int startChar = 32;
			const int endChar = 126;

			// Measure all characters to determine texture size
			wxMemoryDC tempDC;
			tempDC.SetFont(font);

			int totalWidth = 0;
			int maxHeight = 0;
			for (int i = startChar; i <= endChar; ++i) {
				int w, h;
				tempDC.GetTextExtent(static_cast<wxChar>(i), &w, &h);
				totalWidth += w + 2;
				maxHeight = std::max(maxHeight, h);
			}

			// Texture size (power of two for safety)
			textureWidth_ = 256;
			textureHeight_ = 256;
			while (textureWidth_ < totalWidth) {
				textureWidth_ <<= 1;
			}

			wxBitmap bitmap(textureWidth_, textureHeight_);
			wxMemoryDC dc(bitmap);
			dc.SetBackground(*wxBLACK_BRUSH);
			dc.Clear();
			dc.SetFont(font);
			dc.SetTextForeground(*wxWHITE);

			int curX = 0;
			int curY = 0;
			for (int i = startChar; i <= endChar; ++i) {
				int w, h;
				dc.GetTextExtent(static_cast<wxChar>(i), &w, &h);

				if (curX + w > textureWidth_) {
					curX = 0;
					curY += maxHeight + 2;
				}

				dc.DrawText(static_cast<wxChar>(i), curX, curY);

				GlyphInfo& glyph = glyphs_[static_cast<char>(i)];
				glyph.u1 = static_cast<float>(curX) / textureWidth_;
				glyph.v1 = static_cast<float>(curY) / textureHeight_;
				glyph.u2 = static_cast<float>(curX + w) / textureWidth_;
				glyph.v2 = static_cast<float>(curY + h) / textureHeight_;
				glyph.width = w;
				glyph.height = h;
				glyph.advance = w;

				curX += w + 2;
			}

			// Upload to OpenGL
			LOG_RENDER_INFO("[RESOURCE] Creating font texture: {}x{}", textureWidth_, textureHeight_);
			wxImage image = bitmap.ConvertToImage();
			unsigned char* data = image.GetData();

			glGenTextures(1, &textureId_);
			glBindTexture(GL_TEXTURE_2D, textureId_);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth_, textureHeight_, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		void FontRenderer::drawChar(char c) {
			if (!initialized_) {
				return;
			}

			auto it = glyphs_.find(c);
			if (it == glyphs_.end()) {
				return;
			}

			const GlyphInfo& glyph = it->second;

			glBegin(GL_QUADS);
			glTexCoord2f(glyph.u1, glyph.v1);
			glVertex2f(0, 0);
			glTexCoord2f(glyph.u2, glyph.v1);
			glVertex2f(glyph.width, 0);
			glTexCoord2f(glyph.u2, glyph.v2);
			glVertex2f(glyph.width, glyph.height);
			glTexCoord2f(glyph.u1, glyph.v2);
			glVertex2f(0, glyph.height);
			glEnd();

			glTranslatef(glyph.advance, 0, 0);
		}

		void FontRenderer::drawString(int x, int y, std::string_view text, const Color& color) {
			if (!initialized_) {
				return;
			}

			gl::GLState::instance().enableTexture2D();
			glBindTexture(GL_TEXTURE_2D, textureId_);

			glColor4ub(color.r, color.g, color.b, color.a);

			glPushMatrix();
			glTranslatef(static_cast<float>(x), static_cast<float>(y), 0);

			for (char c : text) {
				drawChar(c);
			}

			glPopMatrix();
		}

		int FontRenderer::getCharWidth(char c) const {
			auto it = glyphs_.find(c);
			if (it == glyphs_.end()) {
				return 0;
			}
			return it->second.width;
		}

		int FontRenderer::getStringWidth(std::string_view text) const {
			int totalWidth = 0;
			for (char c : text) {
				totalWidth += getCharWidth(c);
			}
			return totalWidth;
		}

	} // namespace render
} // namespace rme
