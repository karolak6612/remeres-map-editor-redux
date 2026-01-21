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

#ifndef RME_TILE_RENDERER_H_
#define RME_TILE_RENDERER_H_

#include "renderer_base.h"
#include "../types/color.h"
#include <cstdint>

// Forward declarations
class Tile;
class TileLocation;
class Item;
class Creature;
class GameSprite;
class ItemType;
class Map;
class Editor;
struct Position;
struct Outfit;
enum Direction; // Forward declaration without explicit type to match creature.h

namespace rme {
	namespace render {

		/// Handles rendering of tiles and their contents (items, creatures, overlays)
		/// Extracted from MapDrawer::DrawTile + BlitItem
		class TileRenderer {
		public:
			TileRenderer() = default;
			~TileRenderer() = default;

			// Non-copyable
			TileRenderer(const TileRenderer&) = delete;
			TileRenderer& operator=(const TileRenderer&) = delete;

			/// Draw a complete tile at screen position
			void drawTile(TileLocation* location, const RenderContext& ctx, const RenderOptions& opts, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, uint8_t alpha = 255);

			/// Draw an item at screen position (modifies draw_x, draw_y for stacking)
			void drawItem(int& drawX, int& drawY, const Tile* tile, Item* item, bool ephemeral, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha, const RenderContext& ctx, const RenderOptions& opts);

			/// Draw an item with explicit position
			void drawItemAt(int& drawX, int& drawY, const Position& pos, Item* item, bool ephemeral, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha, const Tile* tile, const RenderContext& ctx, const RenderOptions& opts);

			/// Draw a creature at screen position
			void drawCreature(int screenX, int screenY, const Creature* creature, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, uint8_t alpha = 255);

			/// Draw a creature outfit at screen position
			void drawCreatureOutfit(int screenX, int screenY, const Outfit& outfit, Direction dir, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, uint8_t alpha = 255);

			/// Draw a sprite by ID
			void drawSprite(int screenX, int screenY, uint32_t spriteId, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, uint8_t alpha = 255);

			/// Draw a sprite object
			void drawGameSprite(int screenX, int screenY, GameSprite* sprite, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, uint8_t alpha = 255);

			/// Draw a colored square (for invisible items, selection, etc.)
			void drawSquare(int screenX, int screenY, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha, int size = 32);

			/// Draw raw brush indicator
			void drawRawBrush(int screenX, int screenY, ItemType* itemType, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha);

		private:
			// Texture binding optimization
			GLuint lastBoundTexture_ = 0;
			void bindTexture(GLuint textureId);

			// Low-level GL drawing
			void glBlitTexture(int sx, int sy, int textureNum, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha);
			void glBlitSquare(int sx, int sy, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha, int size);

			// Calculate tile color based on properties
			[[nodiscard]] Color calculateTileColor(const Tile* tile, const RenderOptions& opts, int currentHouseId);

			// Calculate stack count index for sprites
			[[nodiscard]] static int getStackIndex(int count) noexcept;
		};

	} // namespace render
} // namespace rme

#endif // RME_TILE_RENDERER_H_
