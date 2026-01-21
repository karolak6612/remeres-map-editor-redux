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

#ifndef RME_TEXTURE_MANAGER_H_
#define RME_TEXTURE_MANAGER_H_

#include <cstdint>
#include <map>
#include <deque>
#include <glad/glad.h>

// Forward declarations
class Sprite;
class GameSprite;

namespace rme {
	namespace render {

		/// Manager for accessing sprites and textures
		/// Owns the sprite storage and provides access methods
		/// Implements ISpriteProvider interface conceptually
		class TextureManager {
		public:
			TextureManager();
			~TextureManager();

			// Sprite storage types
			using SpriteMap = std::map<int, Sprite*>;
			using ImageMap = std::map<int, void*>; // Actually GameSprite::Image* but avoiding header dependency

			/// Get a sprite by ID
			/// @param id Sprite ID
			/// @return Sprite pointer, or nullptr if not found
			[[nodiscard]] Sprite* getSprite(int id);

			/// Get a creature sprite by ID
			/// @param id Creature sprite ID (offset by item count)
			/// @return GameSprite pointer, or nullptr if not found
			[[nodiscard]] GameSprite* getCreatureSprite(int id);

			/// Register a sprite in the manager
			/// @param id Sprite ID
			/// @param sprite Sprite pointer (ownership transferred)
			void registerSprite(int id, Sprite* sprite);

			/// Register an image in the image space
			/// @param id Image ID
			/// @param image Image pointer
			void registerImage(int id, void* image);

			/// Get the maximum item sprite ID
			[[nodiscard]] uint16_t getItemSpriteMaxID() const noexcept {
				return itemCount_;
			}

			/// Set the item count (for creature ID offset calculation)
			void setItemCount(uint16_t count) {
				itemCount_ = count;
			}

			/// Get the maximum creature sprite ID
			[[nodiscard]] uint16_t getCreatureSpriteMaxID() const noexcept {
				return creatureCount_;
			}

			/// Set the creature count
			void setCreatureCount(uint16_t count) {
				creatureCount_ = count;
			}

			/// Get a free texture ID for new textures
			[[nodiscard]] GLuint getFreeTextureID();

			/// Check if sprites have transparency support
			[[nodiscard]] bool hasTransparency() const noexcept {
				return hasTransparency_;
			}

			/// Set transparency support flag
			void setHasTransparency(bool value) {
				hasTransparency_ = value;
			}

			/// Check if manager is unloaded (no sprites loaded)
			[[nodiscard]] bool isUnloaded() const noexcept {
				return unloaded_;
			}

			/// Set unloaded state
			void setUnloaded(bool value) {
				unloaded_ = value;
			}

			/// Clear all sprites
			void clear();

			/// Clean software-only sprite data (wxDC resources)
			void cleanSoftwareSprites();

			/// Get direct access to sprite space (for migration compatibility)
			[[nodiscard]] SpriteMap& getSpriteSpace() {
				return spriteSpace_;
			}
			[[nodiscard]] const SpriteMap& getSpriteSpace() const {
				return spriteSpace_;
			}

			/// Get direct access to image space (for migration compatibility)
			[[nodiscard]] ImageMap& getImageSpace() {
				return imageSpace_;
			}

			/// Track loaded texture count
			void incrementLoadedTextures() {
				++loadedTextures_;
			}
			void decrementLoadedTextures() {
				if (loadedTextures_ > 0) {
					--loadedTextures_;
				}
			}
			[[nodiscard]] int getLoadedTextureCount() const noexcept {
				return loadedTextures_;
			}

			// Prevent copying
			TextureManager(const TextureManager&) = delete;
			TextureManager& operator=(const TextureManager&) = delete;

		private:
			SpriteMap spriteSpace_;
			ImageMap imageSpace_;

			uint16_t itemCount_ = 0;
			uint16_t creatureCount_ = 0;
			int loadedTextures_ = 0;
			GLuint nextTextureId_ = 0x10000000;
			bool hasTransparency_ = false;
			bool unloaded_ = true;
		};

	} // namespace render
} // namespace rme

#endif // RME_TEXTURE_MANAGER_H_
