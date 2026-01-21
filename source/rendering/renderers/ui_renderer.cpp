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
#include "ui_renderer.h"
#include "../opengl/gl_state.h"
#include "../opengl/gl_primitives.h"
#include "../../gui.h"
#include "../../editor.h"
#include "../../live_socket.h"
#include "../interfaces/i_font_renderer.h"
#include <sstream>

namespace rme {
	namespace render {

		UIRenderer::UIRenderer() = default;

		UIRenderer::~UIRenderer() {
			shutdown();
		}

		void UIRenderer::initialize() {
			if (initialized_) {
				return;
			}
			initialized_ = true;
		}

		void UIRenderer::shutdown() {
			if (!initialized_) {
				return;
			}
			initialized_ = false;
		}

		void UIRenderer::renderUI(const RenderContext& ctx, const RenderOptions& opts) {
			if (opts.showIngameBox) {
				renderIngameBox(ctx.viewportWidth / 2, ctx.viewportHeight / 2, ctx.viewportWidth, ctx.viewportHeight); // This needs more logic from legacy
			}

			// Render indicators
			int text_y = 10;
			renderPositionIndicator(ctx.mouseMapX, ctx.mouseMapY, ctx.currentFloor, 10, text_y);
			text_y += 15;
			renderZoomIndicator(ctx.zoom, 10, text_y);
			text_y += 15;
			renderFloorIndicator(ctx.currentFloor, 10, text_y);
		}

		void UIRenderer::renderIngameBox(int centerX, int centerY, int width, int height) {
			// Port logic from MapDrawer::DrawIngameBox
			// (Simplified implementation for now)
			gl::GLState::instance().disableTexture2D();
			gl::GLState::instance().enableBlend();

			Color side_color(0, 0, 0, 200);
			// ... draw actual boxes based on centerX/centerY and MapWidth/Height

			gl::GLState::instance().enableTexture2D();
		}

		void UIRenderer::renderLiveCursor(int cursorX, int cursorY, const std::string& userName, const Color& color) {
			gl::GLState::instance().disableTexture2D();
			gl::GLState::instance().enableBlend();

			gl::Primitives::drawFilledQuad(cursorX, cursorY, kTileSize, kTileSize, color);

			renderText(cursorX + 2, cursorY - 2, userName, color);

			gl::GLState::instance().enableTexture2D();
		}

		void UIRenderer::renderZoomIndicator(float zoom, int x, int y) {
			std::ostringstream ss;
			ss << "Zoom: " << static_cast<int>(zoom * 100) << "%";
			renderText(x, y, ss.str(), colors::White);
		}

		void UIRenderer::renderFloorIndicator(int floor, int x, int y) {
			std::ostringstream ss;
			ss << "Floor: " << floor;
			renderText(x, y, ss.str(), colors::White);
		}

		void UIRenderer::renderPositionIndicator(int mapX, int mapY, int mapZ, int x, int y) {
			std::ostringstream ss;
			ss << mapX << ", " << mapY << ", " << mapZ;
			renderText(x, y, ss.str(), colors::White);
		}

		void UIRenderer::renderText(int x, int y, const std::string& text, const Color& color) {
			if (!fontRenderer_) {
				return;
			}
			fontRenderer_->drawString(x, y, text, color);
		}

	} // namespace render
} // namespace rme
