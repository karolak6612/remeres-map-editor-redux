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

#ifndef RME_RENDERER_BASE_H_
#define RME_RENDERER_BASE_H_

#include "../opengl/gl_includes.h"
#include "../types/color.h"
#include "../types/render_types.h"

namespace rme {
	namespace render {

		// Forward declarations
		class RenderContext;

		/// Base interface for all specialized renderers
		/// Following the Strategy pattern for different rendering operations
		class IRenderer {
		public:
			virtual ~IRenderer() = default;

			/// Initialize the renderer (called once when context is ready)
			virtual void initialize() = 0;

			/// Shutdown the renderer (cleanup resources)
			virtual void shutdown() = 0;

			/// Check if renderer is initialized and ready
			[[nodiscard]] virtual bool isInitialized() const noexcept = 0;
		};

		/// Render context provides shared state for all renderers
		/// Passed to renderers during draw calls
		struct RenderContext {
			// Viewport information
			int viewportWidth = 0;
			int viewportHeight = 0;
			float zoom = 1.0f;

			// View position (scroll offset)
			int scrollX = 0;
			int scrollY = 0;

			// Current floor being rendered
			int currentFloor = 7;

			// Visible tile range
			int startX = 0;
			int startY = 0;
			int endX = 0;
			int endY = 0;

			// Tile size in pixels (accounting for zoom)
			int tileSize = kTileSize;

			// Mouse position in map coordinates
			int mouseMapX = 0;
			int mouseMapY = 0;

			// Dragging information
			int dragOffsetX = 0;
			int dragOffsetY = 0;
			int dragOffsetZ = 0;

			// Selection box state
			bool boundBoxSelection = false;

			// Clear the context
			void clear() {
				viewportWidth = 0;
				viewportHeight = 0;
				zoom = 1.0f;
				scrollX = 0;
				scrollY = 0;
				currentFloor = 7;
				startX = startY = endX = endY = 0;
				tileSize = kTileSize;
				mouseMapX = mouseMapY = 0;
				boundBoxSelection = false;
			}
		};

		/// Drawing options that control what gets rendered
		/// Mirrors the existing DrawingOptions struct for compatibility
		struct RenderOptions {
			bool transparentFloors = false;
			bool transparentItems = false;
			bool showIngameBox = false;
			bool showLights = false;
			bool showLightStr = false;
			bool showTechItems = true;
			bool showWaypoints = true;

			int showGrid = 0;
			bool showAllFloors = false;
			bool showCreatures = true;
			bool showSpawns = true;
			bool showHouses = true;
			bool showShade = true;
			bool showSpecialTiles = true;
			bool showItems = true;

			bool highlightItems = false;
			bool highlightLockedDoors = false;
			bool showBlocking = false;
			bool showTooltips = false;
			bool showAsMinimap = false;
			bool showOnlyColors = false;
			bool showOnlyModified = false;
			bool showPreview = false;
			bool showHooks = false;
			bool hideItemsWhenZoomed = false;
			bool showTowns = false;
			bool alwaysShowZones = false;
			bool extendedHouseShader = false;
			bool experimentalFog = false;

			// Interaction states
			bool isDrawing = false;
			bool isDragging = false;
			int brushSize = 0;
			int brushShape = 0;
		};

	} // namespace render
} // namespace rme

#endif // RME_RENDERER_BASE_H_
