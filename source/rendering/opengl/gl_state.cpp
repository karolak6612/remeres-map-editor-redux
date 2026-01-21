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

#include "gl_state.h"

namespace rme {
	namespace render {
		namespace gl {

			GLState::GLState() {
				resetCache();
			}

			GLState& GLState::instance() {
				static GLState instance;
				return instance;
			}

			void GLState::bindTexture2D(GLuint textureId) {
				if (currentTexture_ != textureId) {
					glBindTexture(GL_TEXTURE_2D, textureId);
					currentTexture_ = textureId;
				}
			}

			void GLState::unbindTexture2D() {
				if (currentTexture_ != 0) {
					glBindTexture(GL_TEXTURE_2D, 0);
					currentTexture_ = 0;
				}
			}

			void GLState::enableBlend() {
				if (!blendEnabled_) {
					glEnable(GL_BLEND);
					blendEnabled_ = true;
				}
			}

			void GLState::disableBlend() {
				if (blendEnabled_) {
					glDisable(GL_BLEND);
					blendEnabled_ = false;
				}
			}

			void GLState::enableTexture2D() {
				if (!texture2DEnabled_) {
					glEnable(GL_TEXTURE_2D);
					texture2DEnabled_ = true;
				}
			}

			void GLState::disableTexture2D() {
				if (texture2DEnabled_) {
					glDisable(GL_TEXTURE_2D);
					texture2DEnabled_ = false;
				}
			}

			void GLState::enableLineStipple() {
				if (!lineStippleEnabled_) {
					glEnable(GL_LINE_STIPPLE);
					lineStippleEnabled_ = true;
				}
			}

			void GLState::disableLineStipple() {
				if (lineStippleEnabled_) {
					glDisable(GL_LINE_STIPPLE);
					lineStippleEnabled_ = false;
				}
			}

			void GLState::setBlendAlpha() {
				enableBlend();
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}

			void GLState::setBlendAdditive() {
				enableBlend();
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			}

			void GLState::setBlendLight() {
				enableBlend();
				glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
			}

			void GLState::setBlendMultiply() {
				enableBlend();
				glBlendFunc(GL_DST_COLOR, GL_ZERO);
			}

			void GLState::setColor(const Color& color) {
				glColor4ub(color.r, color.g, color.b, color.a);
			}

			void GLState::setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
				glColor4ub(r, g, b, a);
			}

			void GLState::setColorF(float r, float g, float b, float a) {
				glColor4f(r, g, b, a);
			}

			void GLState::pushMatrix() {
				glPushMatrix();
			}

			void GLState::popMatrix() {
				glPopMatrix();
			}

			void GLState::loadIdentity() {
				glLoadIdentity();
			}

			void GLState::translate(float x, float y, float z) {
				glTranslatef(x, y, z);
			}

			void GLState::scale(float x, float y, float z) {
				glScalef(x, y, z);
			}

			void GLState::setLineWidth(float width) {
				if (lineWidth_ != width) {
					glLineWidth(width);
					lineWidth_ = width;
				}
			}

			void GLState::setLineStipple(GLint factor, GLushort pattern) {
				glLineStipple(factor, pattern);
			}

			void GLState::resetCache() {
				currentTexture_ = 0;
				blendEnabled_ = false;
				texture2DEnabled_ = false;
				lineStippleEnabled_ = false;
				lineWidth_ = 1.0f;
			}

		} // namespace gl
	} // namespace render
} // namespace rme
