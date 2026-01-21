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

#ifndef RME_I_SPRITE_PROVIDER_H_
#define RME_I_SPRITE_PROVIDER_H_

#include <cstdint>

// Forward declaration - avoid including graphics.h
class GameSprite;

namespace rme {
	namespace render {

		/// Interface for accessing sprite data
		/// Abstracts the GraphicManager to allow for testing and alternative implementations
		class ISpriteProvider {
		public:
			virtual ~ISpriteProvider() = default;

			/// Get a game sprite by ID
			/// @param id The sprite ID
			/// @return Pointer to the sprite, or nullptr if not found
			[[nodiscard]] virtual GameSprite* getSprite(uint32_t id) = 0;

			/// Get a creature sprite by ID
			/// @param id The creature sprite ID
			/// @return Pointer to the sprite, or nullptr if not found
			[[nodiscard]] virtual GameSprite* getCreatureSprite(uint32_t id) = 0;

			/// Get the maximum item sprite ID loaded
			[[nodiscard]] virtual uint32_t getItemSpriteMaxID() const noexcept = 0;

			/// Get the maximum creature sprite ID loaded
			[[nodiscard]] virtual uint32_t getCreatureSpriteMaxID() const noexcept = 0;

			/// Check if a sprite ID is valid
			[[nodiscard]] virtual bool isValidSpriteID(uint32_t id) const noexcept = 0;

			// Prevent copying
			ISpriteProvider(const ISpriteProvider&) = delete;
			ISpriteProvider& operator=(const ISpriteProvider&) = delete;

		protected:
			ISpriteProvider() = default;
			ISpriteProvider(ISpriteProvider&&) = default;
			ISpriteProvider& operator=(ISpriteProvider&&) = default;
		};

	} // namespace render
} // namespace rme

#endif // RME_I_SPRITE_PROVIDER_H_
