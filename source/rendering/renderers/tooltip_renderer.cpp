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

#include "../../main.h"
#include "tooltip_renderer.h"
#include "../opengl/gl_state.h"
#include "../opengl/gl_primitives.h"
#include "../interfaces/i_font_renderer.h"
#include <algorithm>

namespace rme {
	namespace render {

		TooltipRenderer::TooltipRenderer() = default;

		TooltipRenderer::~TooltipRenderer() {
			shutdown();
		}

		void TooltipRenderer::initialize() {
			if (initialized_) {
				return;
			}
			tooltips_.reserve(32);
			initialized_ = true;
		}

		void TooltipRenderer::shutdown() {
			if (!initialized_) {
				return;
			}
			tooltips_.clear();
			initialized_ = false;
		}

		void TooltipRenderer::addTooltip(int x, int y, const std::string& text, const Color& color) {
			tooltips_.emplace_back(x, y, text, color);
		}

		void TooltipRenderer::renderTooltips() {
			if (tooltips_.empty()) {
				return;
			}

			for (const auto& tooltip : tooltips_) {
				renderTooltip(tooltip);
			}
		}

		void TooltipRenderer::clearTooltips() {
			tooltips_.clear();
		}

		void TooltipRenderer::renderTooltip(const Tooltip& tooltip) {
			if (!fontRenderer_) {
				return;
			}

			const char* text = tooltip.text.c_str();
			float line_width = 0.0f;
			float width = 2.0f;
			float height = static_cast<float>(fontRenderer_->getFontHeight());
			int char_count = 0;
			int line_char_count = 0;

			const int MAX_CHARS_PER_LINE = 32;
			const int MAX_CHARS = 256;

			for (const char* c = text; *c != '\0'; c++) {
				if (*c == '\n' || (line_char_count >= MAX_CHARS_PER_LINE && *c == ' ')) {
					height += static_cast<float>(fontRenderer_->getFontHeight());
					line_width = 0.0f;
					line_char_count = 0;
				} else {
					line_width += static_cast<float>(fontRenderer_->getCharWidth(*c));
				}
				width = std::max<float>(width, line_width);
				char_count++;
				line_char_count++;

				if (tooltip.ellipsis && char_count > (MAX_CHARS + 3)) {
					break;
				}
			}

			float scale = 1.0f;
			width = (width + 8.0f) * scale;
			height = (height + 4.0f) * scale;

			float x = tooltip.x + (kTileSize / 2.0f);
			float y = tooltip.y;
			float center = width / 2.0f;
			float space = (7.0f * scale);
			float startx = x - center;
			float endx = x + center;
			float starty = y - (height + space);
			float endy = y - space;

			float vertexes[9][2] = {
				{ x, starty }, { endx, starty }, { endx, endy }, { x + space, endy }, { x, y }, { x - space, endy }, { startx, endy }, { startx, starty }, { x, starty }
			};

			gl::GLState::instance().disableTexture2D();
			gl::GLState::instance().enableBlend();

			glColor4ub(tooltip.color.r, tooltip.color.g, tooltip.color.b, 255);
			glBegin(GL_POLYGON);
			for (int i = 0; i < 8; ++i) {
				glVertex2f(vertexes[i][0], vertexes[i][1]);
			}
			glEnd();

			glColor4ub(0, 0, 0, 255);
			glLineWidth(1.0);
			glBegin(GL_LINES);
			for (int i = 0; i < 8; ++i) {
				glVertex2f(vertexes[i][0], vertexes[i][1]);
				glVertex2f(vertexes[i + 1][0], vertexes[i + 1][1]);
			}
			glEnd();

			startx += (3.0f * scale);
			// Start rendering from the top of the box
			float currentY = starty + (2.0f * scale);

			std::string currentLine;
			line_char_count = 0;
			char_count = 0;

			for (const char* c = text; *c != '\0'; c++) {
				if (*c == '\n' || (line_char_count >= MAX_CHARS_PER_LINE && *c == ' ')) {
					fontRenderer_->drawString(static_cast<int>(startx), static_cast<int>(currentY), currentLine, colors::Black);
					currentY += static_cast<float>(fontRenderer_->getFontHeight()) * scale;
					currentLine.clear();
					line_char_count = 0;
				} else {
					currentLine += *c;
					line_char_count++;
				}
				char_count++;

				if (tooltip.ellipsis && char_count >= MAX_CHARS) {
					currentLine += "...";
					break;
				}
			}
			if (!currentLine.empty()) {
				fontRenderer_->drawString(static_cast<int>(startx), static_cast<int>(currentY), currentLine, colors::Black);
			}

			gl::GLState::instance().enableTexture2D();
		}

		int TooltipRenderer::getTextWidth(const std::string& text) {
			if (!this->fontRenderer_) {
				return 0;
			}
			return this->fontRenderer_->getStringWidth(text);
		}

	} // namespace render
} // namespace rme
