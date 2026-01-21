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

// Forward declarations
class Editor;

namespace rme {
	namespace render {

		// Forward declarations
		struct RenderContext;

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
			RenderContext() :
				viewportWidth(0),
				viewportHeight(0),
				zoom(1.0f),
				scrollX(0),
				scrollY(0),
				currentFloor(7),
				startX(0), startY(0), endX(0), endY(0),
				tileSize(kTileSize),
				mouseMapX(0), mouseMapY(0),
				dragOffsetX(0), dragOffsetY(0), dragOffsetZ(0),
				boundBoxSelection(false),
				currentHouseId(0),
				editor(nullptr) { }

			// Viewport information
			union {
				int viewportWidth;
				int screensizeX;
			};
			union {
				int viewportHeight;
				int screensizeY;
			};
			float zoom;

			// View position (scroll offset)
			union {
				int scrollX;
				int viewScrollX;
			};
			union {
				int scrollY;
				int viewScrollY;
			};

			// Current floor being rendered
			int currentFloor;

			// Visible tile range
			int startX;
			int startY;
			int endX;
			int endY;

			// Tile size in pixels (accounting for zoom)
			int tileSize;

			// Mouse position in map coordinates
			int mouseMapX;
			int mouseMapY;

			// Dragging information
			int dragOffsetX;
			int dragOffsetY;
			int dragOffsetZ;

			// Selection box state
			bool boundBoxSelection;

			// House rendering state
			uint32_t currentHouseId;

			// Pointer to editor (for access to map, etc.)
			::Editor* editor;

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
				dragOffsetX = dragOffsetY = dragOffsetZ = 0;
				boundBoxSelection = false;
				currentHouseId = 0;
				editor = nullptr;
			}
		};

		/// Drawing options that control what gets rendered
		/// Mirrors the existing DrawingOptions struct for compatibility
		struct RenderOptions {
			bool ingame = false;
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
