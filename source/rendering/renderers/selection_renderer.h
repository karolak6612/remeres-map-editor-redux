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

#ifndef RME_SELECTION_RENDERER_H_
#define RME_SELECTION_RENDERER_H_

#include "renderer_base.h"
#include <memory>

// Forward declarations
class Selection;
class Tile;
struct Position;

namespace rme {
	namespace render {
		class TileRenderer;
	}
}

namespace rme {
	namespace render {

		/// Handles rendering of selection indicators and boxes
		/// Extracts selection drawing logic from MapDrawer
		class SelectionRenderer : public IRenderer {
		public:
			SelectionRenderer();
			~SelectionRenderer() override;

			// IRenderer interface
			void initialize() override;
			void shutdown() override;
			[[nodiscard]] bool isInitialized() const noexcept override {
				return initialized_;
			}

			/// Render selection box during drag operation
			void renderSelectionBox(int startX, int startY, int endX, int endY, const Color& color = Color(255, 255, 255, 128));

			/// Render selection box from context
			void renderSelectionBox(const RenderContext& ctx);

			/// Render selection indicator on a tile
			/// @param screenX Tile screen X position
			/// @param screenY Tile screen Y position
			/// @param selected Whether the tile is selected
			void renderTileSelection(int screenX, int screenY, bool selected);

			/// Render a dashed selection rectangle
			/// @param x Rectangle X
			/// @param y Rectangle Y
			/// @param width Rectangle width
			/// @param height Rectangle height
			/// @param color Line color
			void renderDashedRect(int x, int y, int width, int height, const Color& color);

			/// Render live cursors for multiplayer
			/// @param ctx Render context
			void renderLiveCursors(const RenderContext& ctx);

			/// Render dragging shadow effect
			/// @param selection Current selection
			/// @param offsetX Drag offset X
			/// @param offsetY Drag offset Y
			/// @param ctx Render context
			void renderDragShadow(const Selection* selection, int offsetX, int offsetY, const RenderContext& ctx);

		private:
			bool initialized_ = false;
			std::unique_ptr<TileRenderer> tileRenderer_;
		};

	} // namespace render
} // namespace rme

#endif // RME_SELECTION_RENDERER_H_
