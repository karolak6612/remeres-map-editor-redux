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

#include "../../logging/logger.h"
#include "main.h"
#include "item_renderer.h"
#include "../opengl/gl_state.h"
#include "../opengl/gl_primitives.h"

namespace rme {
	namespace render {

		ItemRenderer::ItemRenderer() = default;

		ItemRenderer::~ItemRenderer() {
			shutdown();
		}

		void ItemRenderer::initialize() {
			LOG_RENDER_INFO("[INIT] Initializing ItemRenderer...");
			if (initialized_) {
				return;
			}
			lastBoundTexture_ = 0;
			initialized_ = true;
		}

		void ItemRenderer::shutdown() {
			LOG_RENDER_INFO("[INIT] Shutting down ItemRenderer...");
			if (!initialized_) {
				return;
			}
			lastBoundTexture_ = 0;
			initialized_ = false;
		}

		void ItemRenderer::bindTexture(GLuint textureId) {
			if (lastBoundTexture_ != textureId) {
				LOG_RENDER_TRACE("[RESOURCE] Binding item texture: {}", textureId);
				gl::GLState::instance().bindTexture2D(textureId);
				lastBoundTexture_ = textureId;
			}
		}

		void ItemRenderer::renderItem(Item* item, int screenX, int screenY, const Color& tint) {
			if (!item) {
				return;
			}
			// Note: Full implementation will be migrated from MapDrawer::BlitItem
			// This delegates to the sprite rendering system
		}

		void ItemRenderer::renderItemAt(Item* item, const Position& pos, int screenX, int screenY, const Tile* tile, const Color& tint) {
			if (!item) {
				return;
			}
			// Note: Full implementation with position-aware rendering
			// Handles animations based on position
		}

		void ItemRenderer::renderItemType(ItemType* itemType, int screenX, int screenY, const Color& tint) {
			if (!itemType) {
				return;
			}
			// Note: Renders preview of item type
			// Used by brush system and palettes
		}

		void ItemRenderer::renderSprite(uint32_t spriteId, int screenX, int screenY, const Color& tint) {
			// Note: Full implementation will use TextureManager
			// renderGameSprite(g_graphics.getSprite(spriteId), screenX, screenY, tint);
		}

		void ItemRenderer::renderGameSprite(GameSprite* sprite, int screenX, int screenY, const Color& tint) {
			if (!sprite) {
				return;
			}
			// Note: Full implementation will:
			// 1. Get texture from sprite
			// 2. Bind texture
			// 3. Draw textured quad with tint
		}

	} // namespace render
} // namespace rme
