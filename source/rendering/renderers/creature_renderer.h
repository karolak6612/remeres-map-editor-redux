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

#ifndef RME_CREATURE_RENDERER_H_
#define RME_CREATURE_RENDERER_H_

#include "renderer_base.h"

// Forward declarations
class Creature;
class Outfit;
class GameSprite;
enum Direction; // Forward declaration without explicit type to match creature.h

namespace rme {
	namespace render {

		/// Handles rendering of creatures on the map
		/// Extracts creature drawing logic from MapDrawer::BlitCreature
		class CreatureRenderer : public IRenderer {
		public:
			CreatureRenderer();
			~CreatureRenderer() override;

			// IRenderer interface
			void initialize() override;
			void shutdown() override;
			[[nodiscard]] bool isInitialized() const noexcept override {
				return initialized_;
			}

			/// Render a creature at the given screen position
			/// @param creature The creature to render
			/// @param screenX Screen X position in pixels
			/// @param screenY Screen Y position in pixels
			/// @param tint Color tint to apply
			void renderCreature(const Creature* creature, int screenX, int screenY, const Color& tint = colors::White);

			/// Render a creature outfit at the given position
			/// @param outfit The outfit to render
			/// @param direction Direction the creature is facing
			/// @param screenX Screen X position in pixels
			/// @param screenY Screen Y position in pixels
			/// @param tint Color tint to apply
			void renderOutfit(const Outfit& outfit, Direction direction, int screenX, int screenY, const Color& tint = colors::White);

			/// Render a creature sprite by lookup ID
			/// @param creatureLookId Creature look type ID
			/// @param direction Direction to render
			/// @param screenX Screen X position
			/// @param screenY Screen Y position
			/// @param tint Color tint to apply
			void renderCreatureLook(int creatureLookId, Direction direction, int screenX, int screenY, const Color& tint = colors::White);

		private:
			bool initialized_ = false;
			GLuint lastBoundTexture_ = 0;

			void bindTexture(GLuint textureId);
			void renderOutfitLayers(const Outfit& outfit, Direction direction, int screenX, int screenY, const Color& tint);
		};

	} // namespace render
} // namespace rme

#endif // RME_CREATURE_RENDERER_H_
