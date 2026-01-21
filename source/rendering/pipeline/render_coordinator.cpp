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

#include "../../map.h"
#include "../../basemap.h"
#include "../../main.h"
#include "../../map_region.h"
#include "../../gui.h"
#include "../../editor.h"
#include "render_coordinator.h"
#include <chrono>

// Include all renderers
#include "../renderers/tile_renderer.h"
#include "../renderers/item_renderer.h"
#include "../renderers/creature_renderer.h"
#include "../renderers/selection_renderer.h"
#include "../renderers/brush_renderer.h"
#include "../renderers/grid_renderer.h"
#include "../renderers/light_renderer.h"
#include "../renderers/tooltip_renderer.h"
#include "../renderers/ui_renderer.h"
#include "../renderers/font_renderer.h"

// GL abstraction
#include "../opengl/gl_state.h"
#include "../opengl/gl_primitives.h"

#include <chrono>

#if defined(_MSVC_LANG) && _MSVC_LANG < 201402L
	#define MAKE_UNIQUE(T, ...) std::unique_ptr<T>(new T(__VA_ARGS__))
#else
	#define MAKE_UNIQUE(T, ...) std::make_unique<T>(__VA_ARGS__)
#endif

namespace rme {
	namespace render {

		RenderCoordinator::RenderCoordinator() {
			// Create all renderers
			tileRenderer_ = std::unique_ptr<TileRenderer>(new TileRenderer());
			itemRenderer_ = std::unique_ptr<ItemRenderer>(new ItemRenderer());
			creatureRenderer_ = std::unique_ptr<CreatureRenderer>(new CreatureRenderer());
			selectionRenderer_ = std::unique_ptr<SelectionRenderer>(new SelectionRenderer());
			brushRenderer_ = std::unique_ptr<BrushRenderer>(new BrushRenderer());
			gridRenderer_ = std::unique_ptr<GridRenderer>(new GridRenderer());
			lightRenderer_ = std::unique_ptr<LightRenderer>(new LightRenderer());
			tooltipRenderer_ = std::unique_ptr<TooltipRenderer>(new TooltipRenderer());
			uiRenderer_ = std::unique_ptr<UIRenderer>(new UIRenderer());
			fontRenderer_ = std::unique_ptr<FontRenderer>(new FontRenderer());
		}

		RenderCoordinator::~RenderCoordinator() {
			shutdown();
		}

		void RenderCoordinator::initialize() {
			if (initialized_) {
				return;
			}

			// Initialize renderers that implement IRenderer interface
			// Note: TileRenderer no longer uses IRenderer, so no initialize() call
			itemRenderer_->initialize();
			creatureRenderer_->initialize();
			selectionRenderer_->initialize();
			brushRenderer_->initialize();
			gridRenderer_->initialize();
			lightRenderer_->initialize();
			tooltipRenderer_->initialize();
			uiRenderer_->initialize();
			fontRenderer_->initialize();

			// Font renderer must be set before use
			tooltipRenderer_->setFontRenderer(fontRenderer_.get());
			uiRenderer_->setFontRenderer(fontRenderer_.get());

			initialized_ = true;
		}

		void RenderCoordinator::shutdown() {
			if (!initialized_) {
				return;
			}

			// Shutdown renderers that implement IRenderer interface
			// Note: TileRenderer no longer uses IRenderer, so no shutdown() call
			if (itemRenderer_) {
				itemRenderer_->shutdown();
			}
			if (creatureRenderer_) {
				creatureRenderer_->shutdown();
			}
			if (selectionRenderer_) {
				selectionRenderer_->shutdown();
			}
			if (brushRenderer_) {
				brushRenderer_->shutdown();
			}
			if (gridRenderer_) {
				gridRenderer_->shutdown();
			}
			if (lightRenderer_) {
				lightRenderer_->shutdown();
			}
			if (tooltipRenderer_) {
				tooltipRenderer_->shutdown();
			}
			if (uiRenderer_) {
				uiRenderer_->shutdown();
			}
			if (fontRenderer_) {
				fontRenderer_->shutdown();
			}

			// Release ownership
			tileRenderer_.reset();
			itemRenderer_.reset();
			creatureRenderer_.reset();
			selectionRenderer_.reset();
			brushRenderer_.reset();
			gridRenderer_.reset();
			lightRenderer_.reset();
			tooltipRenderer_.reset();
			uiRenderer_.reset();

			initialized_ = false;
		}

		void RenderCoordinator::render(RenderState& state) {
			if (!initialized_) {
				return;
			}

			state.beginFrame();

			// Execute all passes in order (using existing RenderPass enum)
			for (uint8_t i = 0; i < static_cast<uint8_t>(RenderPass::Count); ++i) {
				executePass(static_cast<RenderPass>(i), state);
			}

			state.endFrame();
		}

		void RenderCoordinator::executePass(RenderPass pass, RenderState& state) {
			auto startTime = std::chrono::high_resolution_clock::now();

			switch (pass) {
				case RenderPass::Background:
					executeBackgroundPass(state);
					break;
				case RenderPass::Tiles:
					executeTilesPass(state);
					break;
				case RenderPass::Selection:
					executeSelectionPass(state);
					break;
				case RenderPass::DraggingShadow:
					executeDraggingShadowPass(state);
					break;
				case RenderPass::HigherFloors:
					executeHigherFloorsPass(state);
					break;
				case RenderPass::Brush:
					executeBrushPass(state);
					break;
				case RenderPass::Grid:
					if (state.options.showGrid > 0) {
						executeGridPass(state);
					}
					break;
				case RenderPass::Light:
					if (state.options.showLights) {
						executeLightPass(state);
					}
					break;
				case RenderPass::UI:
					executeUIPass(state);
					break;
				case RenderPass::Tooltips:
					if (state.options.showTooltips) {
						executeTooltipsPass(state);
					}
					break;
				default:
					break;
			}

			// Callback for profiling
			if (passCallback_) {
				auto endTime = std::chrono::high_resolution_clock::now();
				auto elapsed = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(endTime - startTime).count();
				passCallback_(pass, static_cast<double>(elapsed));
			}
		}

		void RenderCoordinator::executeBackgroundPass(RenderState& state) {
			// Reset GL state for new frame
			gl::GLState::instance().resetCache();
			gl::GLState::instance().setBlendAlpha();
			gl::GLState::instance().enableBlend();
		}

		void RenderCoordinator::executeTilesPass(RenderState& state) {
			Map* map = state.map();
			if (!map) {
				return;
			}

			// Logic ported from MapDrawer::DrawMap
			int start_z, end_z, superend_z;
			int floor = state.context.currentFloor;

			if (state.options.showAllFloors) {
				if (floor <= 7) { // GROUND_LAYER
					start_z = 7;
				} else {
					start_z = std::min(15, floor + 2); // MAP_MAX_LAYER
				}
			} else {
				start_z = floor;
			}

			end_z = floor;
			superend_z = (floor > 7 ? 8 : 0);

			// Iterative z-layer rendering (top to bottom)
			for (int map_z = start_z; map_z >= superend_z; map_z--) {
				// Handle floor shade
				if (map_z == end_z && start_z != end_z && state.options.showShade) {
					gl::GLState::instance().disableTexture2D();
					gl::Primitives::drawFilledQuad(
						0, 0,
						static_cast<int>(state.context.viewportWidth * state.context.zoom),
						static_cast<int>(state.context.viewportHeight * state.context.zoom),
						Color(0, 0, 0, 128)
					);
					gl::GLState::instance().enableTexture2D();
				}

				if (map_z >= end_z) {
					// Iterate over visible tiles
					for (int map_x = state.context.startX; map_x < state.context.endX; ++map_x) {
						for (int map_y = state.context.startY; map_y < state.context.endY; ++map_y) {
							Tile* tile = map->getTile(map_x, map_y, map_z);
							if (tile && tile->location) {
								tileRenderer_->drawTile(tile->location, state.context, state.options);

								// Light pass integration (simplified for now, moved to executeLightPass later)
								// In legacy, it was called here: if (location && options.isDrawLight() && zoom <= 10.0) AddLight(location);
							}
						}
					}
				}
			}
		}

		void RenderCoordinator::executeSelectionPass(RenderState& state) {
			Editor* editor = state.editor();
			if (!editor) {
				return;
			}

			// Render selection box
			if (state.options.isDrawing && state.context.boundBoxSelection) {
				selectionRenderer_->renderSelectionBox(state.context);
			}
		}

		void RenderCoordinator::executeDraggingShadowPass(RenderState& state) {
			Editor* editor = state.editor();
			if (!editor || !state.options.isDragging) {
				return;
			}

			selectionRenderer_->renderDragShadow(
				&editor->selection,
				state.context.dragOffsetX,
				state.context.dragOffsetY,
				state.context
			);
		}

		void RenderCoordinator::executeHigherFloorsPass(RenderState& state) {
			Map* map = state.map();
			if (!map || !state.options.transparentFloors) {
				return;
			}

			int floor = state.context.currentFloor;
			if (floor == 8 || floor == 0) {
				return;
			}

			int map_z = floor - 1;
			for (int map_x = state.context.startX; map_x < state.context.endX; ++map_x) {
				for (int map_y = state.context.startY; map_y < state.context.endY; ++map_y) {
					Tile* tile = map->getTile(map_x, map_y, map_z);
					if (tile && tile->location) {
						tileRenderer_->drawTile(tile->location, state.context, state.options, 255, 255, 255, 96);
					}
				}
			}
		}

		void RenderCoordinator::executeBrushPass(RenderState& state) {
			Brush* brush = g_gui.GetCurrentBrush();
			if (!brush || !state.options.isDrawing) {
				return;
			}

			Position mousePos(state.context.mouseMapX, state.context.mouseMapY, state.context.currentFloor);
			brushRenderer_->renderBrushPreview(brush, mousePos, state.context, state.options);
		}

		void RenderCoordinator::executeGridPass(RenderState& state) {
			// Grid rendering
			gridRenderer_->renderGrid(state.context, state.options.showGrid);
		}

		void RenderCoordinator::executeLightPass(RenderState& state) {
			lightRenderer_->clearLights();

			Map* map = state.map();
			if (!map) {
				return;
			}

			// Add lights from visible tiles
			for (int map_z = 0; map_z < MAP_LAYERS; ++map_z) {
				for (int map_x = state.context.startX; map_x < state.context.endX; ++map_x) {
					for (int map_y = state.context.startY; map_y < state.context.endY; ++map_y) {
						Tile* tile = map->getTile(map_x, map_y, map_z);
						if (tile && tile->location) {
							lightRenderer_->addLight(tile->location);
						}
					}
				}
			}

			lightRenderer_->renderLights(state.context, state.options);
		}

		void RenderCoordinator::executeUIPass(RenderState& state) {
			uiRenderer_->renderUI(state.context, state.options);
		}

		void RenderCoordinator::executeTooltipsPass(RenderState& state) {
			// Tooltip rendering
			tooltipRenderer_->renderTooltips();
			tooltipRenderer_->clearTooltips();
		}

	} // namespace render
} // namespace rme
