#include "../../main.h"
#include "selection_renderer.h"
#include "../../selection.h"
#include "../../tile.h"
#include "../../position.h"
#include "../../editor.h"
#include "../opengl/gl_state.h"
#include "../opengl/gl_primitives.h"
#include "item_renderer.h"
#include "creature_renderer.h"
#include "tile_renderer.h"
#include "../../creature.h"
#include "../opengl/gl_includes.h"
#include <algorithm>
#include <memory>

namespace rme {
	namespace render {

		SelectionRenderer::SelectionRenderer() = default;

		SelectionRenderer::~SelectionRenderer() {
			shutdown();
		}

		void SelectionRenderer::initialize() {
			if (initialized_) {
				return;
			}
			tileRenderer_ = std::unique_ptr<TileRenderer>(new TileRenderer());
			initialized_ = true;
		}

		void SelectionRenderer::shutdown() {
			if (!initialized_) {
				return;
			}
			initialized_ = false;
		}

		void SelectionRenderer::renderSelectionBox(int startX, int startY, int endX, int endY, const Color& color) {
			int x1 = std::min(startX, endX);
			int y1 = std::min(startY, endY);
			int x2 = std::max(startX, endX);
			int y2 = std::max(startY, endY);

			gl::GLState::instance().disableTexture2D();
			gl::GLState::instance().enableBlend();

			glEnable(GL_LINE_STIPPLE);
			glLineStipple(1, 0xf0);
			glLineWidth(1.0);

			glColor4ub(color.r, color.g, color.b, color.a);

			glBegin(GL_LINES);
			// Top
			glVertex2f(x1, y1);
			glVertex2f(x2, y1);
			// Right
			glVertex2f(x2, y1);
			glVertex2f(x2, y2);
			// Bottom
			glVertex2f(x2, y2);
			glVertex2f(x1, y2);
			// Left
			glVertex2f(x1, y2);
			glVertex2f(x1, y1);
			glEnd();

			glDisable(GL_LINE_STIPPLE);
			gl::GLState::instance().enableTexture2D();
		}

		void SelectionRenderer::renderTileSelection(int screenX, int screenY, bool selected) {
			if (!selected) {
				return;
			}

			gl::GLState::instance().disableTexture2D();
			gl::GLState::instance().enableBlend();

			// Draw selection highlight (semi-transparent white)
			gl::Primitives::drawFilledQuad(screenX, screenY, kTileSize, kTileSize, Color(255, 255, 255, 64));

			gl::GLState::instance().enableTexture2D();
		}

		void SelectionRenderer::renderDashedRect(int x, int y, int width, int height, const Color& color) {
			gl::GLState::instance().disableTexture2D();
			gl::Primitives::drawDashedRect(x, y, width, height, color, 1.0f);
			gl::GLState::instance().enableTexture2D();
		}

		void SelectionRenderer::renderDragShadow(const Selection* selection, int offsetX, int offsetY, const RenderContext& ctx) {
			if (!selection || selection->isBusy() || !initialized_) {
				return;
			}

			// Logic ported from MapDrawer::DrawDraggingShadow
			// offsetX/offsetY are map coordinate deltas

			for (auto it = selection->begin(); it != selection->end(); ++it) {
				Tile* tile = *it;
				Position pos = tile->getPosition();

				// Apply offset
				pos.x += offsetX;
				pos.y += offsetY;

				// Skip if out of layers (simplified)
				if (pos.z < 0 || pos.z >= 16) {
					continue;
				}

				// Check if visible (roughly)
				if (pos.x < ctx.startX - 2 || pos.x > ctx.endX + 2 || pos.y < ctx.startY - 2 || pos.y > ctx.endY + 2) {
					continue;
				}

				// Offset for floor elevation rendering
				int elevationOffset = 0;
				if (pos.z <= 7) {
					elevationOffset = (7 - pos.z) * kTileSize;
				} else {
					elevationOffset = kTileSize * (ctx.currentFloor - pos.z);
				}

				int drawX = (pos.x * kTileSize - ctx.scrollX) - elevationOffset;
				int drawY = (pos.y * kTileSize - ctx.scrollY) - elevationOffset;

				// Render selected items
				// (Assuming zoom check or simplified for now)
				auto items = tile->getSelectedItems();
				for (Item* item : items) {
					// We pass simplified RenderOptions here as it's a preview
					RenderOptions opts;
					tileRenderer_->drawItemAt(drawX, drawY, pos, item, true, 160, 160, 160, 160, tile, ctx, opts);
				}

				// Render creature if selected
				if (tile->creature && tile->creature->isSelected()) {
					tileRenderer_->drawCreature(drawX, drawY, tile->creature, 255, 255, 255, 160);
				}
			}
		}

		void SelectionRenderer::renderSelectionBox(const RenderContext& ctx) {
			// (ctx typically provides screen coords from input dispatcher)
			// But we need to make sure RenderContext HAS selection box bounds
			// For now, it might be better to pass them directly from coordinator if they are stored in state
		}

		void SelectionRenderer::renderLiveCursors(const RenderContext& ctx) {
			if (!g_live.isLive()) {
				return;
			}

			for (auto& cursor : g_live.getCursors()) {
				if (cursor.floor != ctx.currentFloor) {
					continue;
				}

				// Calculate screen position
				int offset = (cursor.floor <= 7) ? (7 - cursor.floor) * kTileSize : kTileSize * (ctx.currentFloor - cursor.floor);
				int sx = (cursor.x * kTileSize - ctx.scrollX) - offset;
				int sy = (cursor.y * kTileSize - ctx.scrollY) - offset;

				// draw cursor
				gl::GLState::instance().disableTexture2D();
				gl::Primitives::drawFilledQuad(sx, sy, kTileSize, kTileSize, cursor.color);
				gl::GLState::instance().enableTexture2D();
			}
		}

	} // namespace render
} // namespace rme
