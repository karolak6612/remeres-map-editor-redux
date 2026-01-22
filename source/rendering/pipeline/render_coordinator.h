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

#ifndef RME_RENDER_COORDINATOR_H_
#define RME_RENDER_COORDINATOR_H_

#include <memory>
#include <array>
#include <functional>
#include "render_state.h"
#include "render_pass.h"
#include "../renderers/renderer_base.h"

// Forward declarations for renderers
namespace rme {
	namespace render {
		class TileRenderer;
		class ItemRenderer;
		class CreatureRenderer;
		class SelectionRenderer;
		class BrushRenderer;
		class GridRenderer;
		class LightRenderer;
		class TooltipRenderer;
		class UIRenderer;
		class FontRenderer;
	} // namespace render
} // namespace rme

namespace rme {
	namespace render {

		/// Central coordinator for the rendering pipeline
		/// Manages all renderers and orchestrates the render passes
		class RenderCoordinator {
		public:
			/// Constructor with dependency injection
			RenderCoordinator(
				std::unique_ptr<TileRenderer> tileRenderer,
				std::unique_ptr<ItemRenderer> itemRenderer,
				std::unique_ptr<CreatureRenderer> creatureRenderer,
				std::unique_ptr<SelectionRenderer> selectionRenderer,
				std::unique_ptr<BrushRenderer> brushRenderer,
				std::unique_ptr<GridRenderer> gridRenderer,
				std::unique_ptr<LightRenderer> lightRenderer,
				std::unique_ptr<TooltipRenderer> tooltipRenderer,
				std::unique_ptr<UIRenderer> uiRenderer,
				std::unique_ptr<FontRenderer> fontRenderer
			);
			~RenderCoordinator();

			/// Initialize all renderers
			void initialize();

			/// Shutdown all renderers
			void shutdown();

			/// Check if coordinator is ready
			[[nodiscard]] bool isInitialized() const noexcept {
				return initialized_;
			}

			/// Execute a complete render frame
			/// @param state The render state with context and options
			void render(RenderState& state);

			/// Execute a single render pass
			/// @param pass The pass to execute
			/// @param state The render state
			void executePass(RenderPass pass, RenderState& state);

			// Renderer accessors
			TileRenderer& tileRenderer() {
				return *tileRenderer_;
			}
			ItemRenderer& itemRenderer() {
				return *itemRenderer_;
			}
			CreatureRenderer& creatureRenderer() {
				return *creatureRenderer_;
			}
			SelectionRenderer& selectionRenderer() {
				return *selectionRenderer_;
			}
			BrushRenderer& brushRenderer() {
				return *brushRenderer_;
			}
			GridRenderer& gridRenderer() {
				return *gridRenderer_;
			}
			LightRenderer& lightRenderer() {
				return *lightRenderer_;
			}
			TooltipRenderer& tooltipRenderer() {
				return *tooltipRenderer_;
			}
			UIRenderer& uiRenderer() {
				return *uiRenderer_;
			}

			/// Set callback for pass completion (for debugging/profiling)
			using PassCallback = std::function<void(RenderPass, double)>;
			void setPassCallback(PassCallback callback) {
				passCallback_ = std::move(callback);
			}

		private:
			bool initialized_ = false;

			// Owned renderers
			std::unique_ptr<TileRenderer> tileRenderer_;
			std::unique_ptr<ItemRenderer> itemRenderer_;
			std::unique_ptr<CreatureRenderer> creatureRenderer_;
			std::unique_ptr<SelectionRenderer> selectionRenderer_;
			std::unique_ptr<BrushRenderer> brushRenderer_;
			std::unique_ptr<GridRenderer> gridRenderer_;
			std::unique_ptr<LightRenderer> lightRenderer_;
			std::unique_ptr<TooltipRenderer> tooltipRenderer_;
			std::unique_ptr<UIRenderer> uiRenderer_;
			std::unique_ptr<FontRenderer> fontRenderer_;

			// Optional callback for profiling
			PassCallback passCallback_;

			// Pass execution helpers (matching RenderPass enum in render_types.h)
			void executeBackgroundPass(RenderState& state);
			void executeTilesPass(RenderState& state);
			void executeSelectionPass(RenderState& state);
			void executeDraggingShadowPass(RenderState& state);
			void executeHigherFloorsPass(RenderState& state);
			void executeBrushPass(RenderState& state);
			void executeGridPass(RenderState& state);
			void executeLightPass(RenderState& state);
			void executeUIPass(RenderState& state);
			void executeTooltipsPass(RenderState& state);
		};

	} // namespace render
} // namespace rme

#endif // RME_RENDER_COORDINATOR_H_
