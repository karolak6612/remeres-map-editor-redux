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

#ifndef RME_ITEM_RENDERER_H_
#define RME_ITEM_RENDERER_H_

#include "renderer_base.h"

// Forward declarations
class Item;
class ItemType;
class GameSprite;
class Tile;
struct Position;

namespace rme {
	namespace render {

		/// Handles rendering of items on the map
		/// Extracts item drawing logic from MapDrawer::BlitItem
		class ItemRenderer : public IRenderer {
		public:
			ItemRenderer();
			~ItemRenderer() override;

			// IRenderer interface
			void initialize() override;
			void shutdown() override;
			[[nodiscard]] bool isInitialized() const noexcept override {
				return initialized_;
			}

			/// Render an item at the given screen position
			/// @param item The item to render
			/// @param screenX Screen X position in pixels
			/// @param screenY Screen Y position in pixels
			/// @param tint Color tint to apply (default: white/no tint)
			void renderItem(Item* item, int screenX, int screenY, const Color& tint = colors::White);

			/// Render an item with position context (for animations)
			/// @param item The item to render
			/// @param pos Position on the map
			/// @param screenX Screen X position in pixels
			/// @param screenY Screen Y position in pixels
			/// @param tile Optional tile context for stacking
			/// @param tint Color tint to apply
			void renderItemAt(Item* item, const Position& pos, int screenX, int screenY, const Tile* tile = nullptr, const Color& tint = colors::White);

			/// Render an item type preview (for brushes/palettes)
			/// @param itemType The item type to render
			/// @param screenX Screen X position
			/// @param screenY Screen Y position
			/// @param tint Color tint to apply
			void renderItemType(ItemType* itemType, int screenX, int screenY, const Color& tint = colors::White);

			/// Render a sprite by ID
			/// @param spriteId Sprite ID to render
			/// @param screenX Screen X position
			/// @param screenY Screen Y position
			/// @param tint Color tint to apply
			void renderSprite(uint32_t spriteId, int screenX, int screenY, const Color& tint = colors::White);

			/// Render a raw game sprite
			/// @param sprite The game sprite to render
			/// @param screenX Screen X position
			/// @param screenY Screen Y position
			/// @param tint Color tint to apply
			void renderGameSprite(GameSprite* sprite, int screenX, int screenY, const Color& tint = colors::White);

		private:
			bool initialized_ = false;
			GLuint lastBoundTexture_ = 0;

			void bindTexture(GLuint textureId);
		};

	} // namespace render
} // namespace rme

#endif // RME_ITEM_RENDERER_H_
