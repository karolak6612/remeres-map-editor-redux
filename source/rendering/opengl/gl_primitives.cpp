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
#include "gl_primitives.h"
#include "gl_state.h"

namespace rme {
	namespace render {
		namespace gl {

			void Primitives::drawTexturedQuad(int x, int y, int w, int h, float u0, float v0, float u1, float v1) {
				LOG_RENDER_TRACE("[GL_STATE] drawTexturedQuad - Rect: ({},{},{},{}), UV: ({},{}) to ({},{})", x, y, w, h, u0, v0, u1, v1);
				glBegin(GL_QUADS);
				glTexCoord2f(u0, v0);
				glVertex2i(x, y);
				glTexCoord2f(u1, v0);
				glVertex2i(x + w, y);
				glTexCoord2f(u1, v1);
				glVertex2i(x + w, y + h);
				glTexCoord2f(u0, v1);
				glVertex2i(x, y + h);
				glEnd();
			}

			void Primitives::drawTexturedTile(int x, int y) {
				drawTexturedQuad(x, y, kTileSize, kTileSize);
			}

			void Primitives::drawTexturedQuadTinted(int x, int y, int w, int h, const Color& tint, float u0, float v0, float u1, float v1) {
				GLState::instance().setColor(tint);
				drawTexturedQuad(x, y, w, h, u0, v0, u1, v1);
			}

			void Primitives::drawFilledQuad(int x, int y, int w, int h, const Color& color) {
				LOG_RENDER_TRACE("[GL_STATE] drawFilledQuad - Rect: ({},{},{},{}), Color: ({},{},{},{})", x, y, w, h, color.r, color.g, color.b, color.a);
				GLState::instance().disableTexture2D();
				GLState::instance().setColor(color);

				glBegin(GL_QUADS);
				glVertex2i(x, y);
				glVertex2i(x + w, y);
				glVertex2i(x + w, y + h);
				glVertex2i(x, y + h);
				glEnd();
			}

			void Primitives::drawFilledQuad(int x, int y, int w, int h) {
				GLState::instance().disableTexture2D();

				glBegin(GL_QUADS);
				glVertex2i(x, y);
				glVertex2i(x + w, y);
				glVertex2i(x + w, y + h);
				glVertex2i(x, y + h);
				glEnd();
			}

			void Primitives::drawFilledRect(const Rect& rect, const Color& color) {
				drawFilledQuad(rect.x, rect.y, rect.width, rect.height, color);
			}

			void Primitives::drawRect(int x, int y, int w, int h, const Color& color, float lineWidth) {
				auto& state = GLState::instance();
				state.disableTexture2D();
				state.setColor(color);
				state.setLineWidth(lineWidth);

				glBegin(GL_LINE_LOOP);
				glVertex2i(x, y);
				glVertex2i(x + w, y);
				glVertex2i(x + w, y + h);
				glVertex2i(x, y + h);
				glEnd();
			}

			void Primitives::drawDashedRect(int x, int y, int w, int h, const Color& color, float lineWidth) {
				auto& state = GLState::instance();
				state.disableTexture2D();
				state.setColor(color);
				state.setLineWidth(lineWidth);
				state.enableLineStipple();
				state.setLineStipple(1, 0x0F0F);

				glBegin(GL_LINE_LOOP);
				glVertex2i(x, y);
				glVertex2i(x + w, y);
				glVertex2i(x + w, y + h);
				glVertex2i(x, y + h);
				glEnd();

				state.disableLineStipple();
			}

			void Primitives::drawLine(int x1, int y1, int x2, int y2, const Color& color, float lineWidth) {
				auto& state = GLState::instance();
				state.disableTexture2D();
				state.setColor(color);
				state.setLineWidth(lineWidth);

				glBegin(GL_LINES);
				glVertex2i(x1, y1);
				glVertex2i(x2, y2);
				glEnd();
			}

			void Primitives::drawDashedLine(int x1, int y1, int x2, int y2, const Color& color, float lineWidth) {
				auto& state = GLState::instance();
				state.disableTexture2D();
				state.setColor(color);
				state.setLineWidth(lineWidth);
				state.enableLineStipple();
				state.setLineStipple(1, 0x0F0F);

				glBegin(GL_LINES);
				glVertex2i(x1, y1);
				glVertex2i(x2, y2);
				glEnd();

				state.disableLineStipple();
			}

			void Primitives::beginLines(const Color& color, float lineWidth) {
				auto& state = GLState::instance();
				state.disableTexture2D();
				state.setColor(color);
				state.setLineWidth(lineWidth);
				glBegin(GL_LINES);
			}

			void Primitives::addLineVertex(int x, int y) {
				glVertex2i(x, y);
			}

			void Primitives::endLines() {
				glEnd();
			}

			void Primitives::drawSprite(int x, int y, GLuint textureId, int w, int h, const Color& tint) {
				auto& state = GLState::instance();
				state.enableTexture2D();
				state.bindTexture2D(textureId);
				state.setColor(tint);

				drawTexturedQuad(x, y, w, h);
			}

			void Primitives::setScissor(int x, int y, int w, int h) {
				LOG_RENDER_DEBUG("[GL_STATE] Enabling scissor test - Rect: ({},{},{},{})", x, y, w, h);
				glEnable(GL_SCISSOR_TEST);
				glScissor(x, y, w, h);
			}

			void Primitives::clearScissor() {
				LOG_RENDER_DEBUG("[GL_STATE] Disabling scissor test");
				glDisable(GL_SCISSOR_TEST);
			}

		} // namespace gl
	} // namespace render
} // namespace rme
