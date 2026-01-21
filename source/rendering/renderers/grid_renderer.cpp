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

#include "main.h"
#include "grid_renderer.h"
#include "../opengl/gl_state.h"
#include "../opengl/gl_primitives.h"

namespace rme {
	namespace render {

		GridRenderer::GridRenderer() = default;

		GridRenderer::~GridRenderer() {
			shutdown();
		}

		void GridRenderer::initialize() {
			if (initialized_) {
				return;
			}
			initialized_ = true;
		}

		void GridRenderer::shutdown() {
			if (!initialized_) {
				return;
			}
			initialized_ = false;
		}

		void GridRenderer::renderGrid(const RenderContext& ctx, int gridType, const Color& color) {
			if (gridType == 0) {
				return;
			}

			gridType_ = gridType;

			if (gridType == 1) {
				renderSmallGrid(ctx, color);
			} else if (gridType >= 2) {
				renderLargeGrid(ctx, color);
			}
		}

		void GridRenderer::renderGridCell(int screenX, int screenY, const Color& color) {
			gl::GLState::instance().disableTexture2D();
			gl::Primitives::drawRect(screenX, screenY, kTileSize, kTileSize, color, 1.0f);
			gl::GLState::instance().enableTexture2D();
		}

		void GridRenderer::renderSmallGrid(const RenderContext& ctx, const Color& color) {
			gl::GLState::instance().disableTexture2D();
			gl::GLState::instance().enableBlend();
			gl::GLState::instance().setBlendAlpha();

			// Begin batch line drawing for efficiency
			gl::Primitives::beginLines(color, 1.0f);

			int tileSize = ctx.tileSize;
			int width = ctx.viewportWidth;
			int height = ctx.viewportHeight;

			// Draw vertical lines
			for (int x = 0; x <= width; x += tileSize) {
				gl::Primitives::addLineVertex(x, 0);
				gl::Primitives::addLineVertex(x, height);
			}

			// Draw horizontal lines
			for (int y = 0; y <= height; y += tileSize) {
				gl::Primitives::addLineVertex(0, y);
				gl::Primitives::addLineVertex(width, y);
			}

			gl::Primitives::endLines();
			gl::GLState::instance().enableTexture2D();
		}

		void GridRenderer::renderLargeGrid(const RenderContext& ctx, const Color& color) {
			gl::GLState::instance().disableTexture2D();
			gl::GLState::instance().enableBlend();
			gl::GLState::instance().setBlendAlpha();

			// Large grid is every 8 tiles
			int gridSize = ctx.tileSize * 8;

			gl::Primitives::beginLines(color, 1.0f);

			int width = ctx.viewportWidth;
			int height = ctx.viewportHeight;

			// Calculate offset to align with map grid
			int offsetX = (ctx.scrollX * ctx.tileSize) % gridSize;
			int offsetY = (ctx.scrollY * ctx.tileSize) % gridSize;

			// Draw vertical lines
			for (int x = -offsetX; x <= width; x += gridSize) {
				gl::Primitives::addLineVertex(x, 0);
				gl::Primitives::addLineVertex(x, height);
			}

			// Draw horizontal lines
			for (int y = -offsetY; y <= height; y += gridSize) {
				gl::Primitives::addLineVertex(0, y);
				gl::Primitives::addLineVertex(width, y);
			}

			gl::Primitives::endLines();
			gl::GLState::instance().enableTexture2D();
		}

	} // namespace render
} // namespace rme
