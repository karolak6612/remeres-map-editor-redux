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

#ifndef RME_BRUSH_RENDERER_H_
#define RME_BRUSH_RENDERER_H_

#include "renderer_base.h"
#include <memory>

// Forward declarations
class Brush;
class ItemType;
struct Position;

namespace rme {
	namespace render {
		class TileRenderer;
	}
}

namespace rme {
	namespace render {

		/// Handles rendering of brush previews and indicators
		/// Extracts brush drawing logic from MapDrawer::DrawBrush
		class BrushRenderer : public IRenderer {
		public:
			BrushRenderer();
			~BrushRenderer() override;

			// IRenderer interface
			void initialize() override;
			void shutdown() override;
			[[nodiscard]] bool isInitialized() const noexcept override {
				return initialized_;
			}

			/// Render brush indicator at the given position
			/// @param brush The brush to render
			/// @param screenX Screen X position
			/// @param screenY Screen Y position
			/// @param color Indicator color
			void renderBrushIndicator(Brush* brush, int screenX, int screenY, const Color& color);

			/// Render a raw item type preview
			/// @param itemType Item type to render
			/// @param screenX Screen X position
			/// @param screenY Screen Y position
			/// @param color Tint color
			void renderRawBrush(ItemType* itemType, int screenX, int screenY, const Color& color);

			/// Render brush size indicator circle
			/// @param centerX Center X in screen coordinates
			/// @param centerY Center Y in screen coordinates
			/// @param radius Radius in tiles
			/// @param tileSize Tile size in pixels
			/// @param color Indicator color
			void renderBrushSize(int centerX, int centerY, int radius, int tileSize, const Color& color);

			/// Render brush preview for the entire brush area
			/// @param brush The brush
			/// @param pos Center position on map
			/// @param ctx Render context
			/// @param opts Render options
			void renderBrushPreview(Brush* brush, const Position& pos, const RenderContext& ctx, const RenderOptions& opts);

		private:
			bool initialized_ = false;
			std::unique_ptr<TileRenderer> tileRenderer_;
		};

	} // namespace render
} // namespace rme

#endif // RME_BRUSH_RENDERER_H_
