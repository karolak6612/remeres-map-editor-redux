#include "../../main.h"
#include "brush_renderer.h"
#include "../../brushes/brush.h"
#include "item_renderer.h"
#include "creature_renderer.h"
#include "../../position.h"
#include "../../gui.h"
#include "../../editor.h"
#include "../../brushes/raw_brush.h"
#include "../../brushes/creature_brush.h"
#include "../../brushes/house_brush.h"
#include "../../brushes/house_exit_brush.h"
#include "../graphics.h"
#include "../opengl/gl_includes.h"
#include "../opengl/gl_state.h"
#include "../opengl/gl_primitives.h"
#include "tile_renderer.h"
#include <memory>
#include <cmath>

namespace rme {
	namespace render {

		BrushRenderer::BrushRenderer() = default;

		BrushRenderer::~BrushRenderer() {
			shutdown();
		}

		void BrushRenderer::initialize() {
			if (initialized_) {
				return;
			}
			tileRenderer_ = std::unique_ptr<TileRenderer>(new TileRenderer());
			initialized_ = true;
		}

		void BrushRenderer::shutdown() {
			if (!initialized_) {
				return;
			}
			initialized_ = false;
		}

		void BrushRenderer::renderBrushIndicator(Brush* brush, int x, int y, const Color& color) {
			if (!brush) {
				return;
			}

			x += (kTileSize / 2);
			y += (kTileSize / 2);

			// 7----0----1
			// |         |
			// 6--5  3--2
			//     \/
			//     4
			static int vertexes[9][2] = {
				{ -15, -20 }, // 0
				{ 15, -20 }, // 1
				{ 15, -5 }, // 2
				{ 5, -5 }, // 3
				{ 0, 0 }, // 4
				{ -5, -5 }, // 5
				{ -15, -5 }, // 6
				{ -15, -20 }, // 7
				{ -15, -20 }, // 0
			};

			gl::GLState::instance().disableTexture2D();
			gl::GLState::instance().enableBlend();

			// circle shadow
			glBegin(GL_TRIANGLE_FAN);
			glColor4ub(0x00, 0x00, 0x00, 0x50);
			glVertex2i(x, y);
			for (int i = 0; i <= 30; i++) {
				float angle = i * 2.0f * 3.14159265f / 30;
				glVertex2f(cos(angle) * (kTileSize / 2) + x, sin(angle) * (kTileSize / 2) + y);
			}
			glEnd();

			// background
			glColor4ub(color.r, color.g, color.b, 0xB4);
			glBegin(GL_POLYGON);
			for (int i = 0; i < 8; ++i) {
				glVertex2i(vertexes[i][0] + x, vertexes[i][1] + y);
			}
			glEnd();

			// borders
			glColor4ub(0x00, 0x00, 0x00, 0xB4);
			glLineWidth(1.0);
			glBegin(GL_LINES);
			for (int i = 0; i < 8; ++i) {
				glVertex2i(vertexes[i][0] + x, vertexes[i][1] + y);
				glVertex2i(vertexes[i + 1][0] + x, vertexes[i + 1][1] + y);
			}
			glEnd();

			gl::GLState::instance().enableTexture2D();
		}

		void BrushRenderer::renderRawBrush(ItemType* itemType, int screenX, int screenY, const Color& color) {
			if (!itemType || !itemType->sprite || !initialized_) {
				return;
			}

			tileRenderer_->drawRawBrush(screenX, screenY, itemType, color.r, color.g, color.b, color.a);
		}

		void BrushRenderer::renderBrushSize(int centerX, int centerY, int radius, int tileSize, const Color& color) {
			gl::GLState::instance().disableTexture2D();
			gl::GLState::instance().enableBlend();

			int x = centerX - radius * tileSize;
			int y = centerY - radius * tileSize;
			int size = (radius * 2 + 1) * tileSize;

			gl::Primitives::drawDashedRect(x, y, size, size, color, 1.0f);

			gl::GLState::instance().enableTexture2D();
		}

		void BrushRenderer::renderBrushPreview(Brush* brush, const Position& pos, const RenderContext& ctx, const RenderOptions& opts) {
			if (!brush || !initialized_) {
				return;
			}

			// Interaction states are now in opts (isDrawing, isDragging, brushSize, brushShape)

			// Set colors based on brush type
			uint8_t r = 255, g = 255, b = 255, alpha = 160;

			if (brush->isTerrain() || brush->isTable() || brush->isCarpet()) {
				r = 0;
				g = 255;
				b = 0; // Greenish brush
			} else if (brush->isHouse()) {
				r = 0;
				g = 0;
				b = 255;
			} else if (brush->isSpawn()) {
				r = 255;
				g = 0;
				b = 0;
			} else if (brush->isEraser()) {
				r = 255;
				g = 255;
				b = 255;
			}

			// Wall brush special handle
			if (brush->isWall()) {
				// Ported logic for wall drawing (lines, corners)
				// Simplified for ahora: draw a cross or box
				int startX = (pos.x - opts.brushSize) * kTileSize - ctx.scrollX;
				int startY = (pos.y - opts.brushSize) * kTileSize - ctx.scrollY;
				int endX = (pos.x + opts.brushSize + 1) * kTileSize - ctx.scrollX;
				int endY = (pos.y + opts.brushSize + 1) * kTileSize - ctx.scrollY;

				gl::GLState::instance().disableTexture2D();
				gl::GLState::instance().enableBlend();
				glColor4ub(r, g, b, 80);
				gl::Primitives::drawFilledQuad(startX, startY, endX - startX, kTileSize, Color(r, g, b, 80));
				gl::Primitives::drawFilledQuad(startX, startY, kTileSize, endY - startY, Color(r, g, b, 80));
				gl::GLState::instance().enableTexture2D();
			} else if (brush->isRaw()) {
				// Iterate area and draw raw brush images
				RAWBrush* rb = brush->asRaw();
				if (!rb) {
					return;
				}

				for (int dy = -opts.brushSize; dy <= opts.brushSize; ++dy) {
					for (int dx = -opts.brushSize; dx <= opts.brushSize; ++dx) {
						if (opts.brushShape == BRUSHSHAPE_CIRCLE) {
							if (std::sqrt(dx * dx + dy * dy) > opts.brushSize + 0.5) {
								continue;
							}
						}

						int sx = (pos.x + dx) * kTileSize - ctx.scrollX;
						int sy = (pos.y + dy) * kTileSize - ctx.scrollY;
						tileRenderer_->drawRawBrush(sx, sy, rb->getItemType(), 255, 255, 255, 160);
					}
				}
			} else {
				// Basic tinted box for other brushes
				for (int dy = -opts.brushSize; dy <= opts.brushSize; ++dy) {
					for (int dx = -opts.brushSize; dx <= opts.brushSize; ++dx) {
						if (opts.brushShape == BRUSHSHAPE_CIRCLE) {
							if (std::sqrt(dx * dx + dy * dy) > opts.brushSize + 0.5) {
								continue;
							}
						}

						int sx = (pos.x + dx) * kTileSize - ctx.scrollX;
						int sy = (pos.y + dy) * kTileSize - ctx.scrollY;

						gl::GLState::instance().disableTexture2D();
						gl::Primitives::drawFilledQuad(sx, sy, kTileSize, kTileSize, Color(r, g, b, 60));
						gl::GLState::instance().enableTexture2D();
					}
				}
			}
		}

	} // namespace render
} // namespace rme
