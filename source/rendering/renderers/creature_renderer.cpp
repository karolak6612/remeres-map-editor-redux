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
#include "creature_renderer.h"
#include "../opengl/gl_state.h"
#include "../opengl/gl_primitives.h"

namespace rme {
	namespace render {

		CreatureRenderer::CreatureRenderer() = default;

		CreatureRenderer::~CreatureRenderer() {
			shutdown();
		}

		void CreatureRenderer::initialize() {
			LOG_RENDER_INFO("[INIT] Initializing CreatureRenderer...");
			if (initialized_) {
				return;
			}
			lastBoundTexture_ = 0;
			initialized_ = true;
		}

		void CreatureRenderer::shutdown() {
			LOG_RENDER_INFO("[INIT] Shutting down CreatureRenderer...");
			if (!initialized_) {
				return;
			}
			lastBoundTexture_ = 0;
			initialized_ = false;
		}

		void CreatureRenderer::bindTexture(GLuint textureId) {
			if (lastBoundTexture_ != textureId) {
				LOG_RENDER_TRACE("[RESOURCE] Binding creature texture: {}", textureId);
				gl::GLState::instance().bindTexture2D(textureId);
				lastBoundTexture_ = textureId;
			}
		}

		void CreatureRenderer::renderCreature(const Creature* creature, int screenX, int screenY, const Color& tint) {
			if (!creature) {
				return;
			}
			// Note: Full implementation will be migrated from MapDrawer::BlitCreature
			// renderOutfit(creature->getOutfit(), creature->getDirection(), screenX, screenY, tint);
		}

		void CreatureRenderer::renderOutfit(const Outfit& outfit, Direction direction, int screenX, int screenY, const Color& tint) {
			// Note: Full implementation will render layered outfit sprites
			// Including head, body, legs, feet colors and addons
			renderOutfitLayers(outfit, direction, screenX, screenY, tint);
		}

		void CreatureRenderer::renderCreatureLook(int creatureLookId, Direction direction, int screenX, int screenY, const Color& tint) {
			// Note: Full implementation will get creature sprite from TextureManager
			// GameSprite* sprite = textureManager.getCreatureSprite(creatureLookId);
			// if (sprite) { ... render sprite ... }
		}

		void CreatureRenderer::renderOutfitLayers(const Outfit& outfit, Direction direction, int screenX, int screenY, const Color& tint) {
			// Note: Full implementation will:
			// 1. Render base sprite
			// 2. Apply head color
			// 3. Apply body color
			// 4. Apply legs color
			// 5. Apply feet color
			// 6. Render addons if present
		}

	} // namespace render
} // namespace rme
