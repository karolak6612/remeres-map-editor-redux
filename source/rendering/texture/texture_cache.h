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

#ifndef RME_TEXTURE_CACHE_H_
#define RME_TEXTURE_CACHE_H_

#include <cstdint>
#include <deque>
#include <ctime>

// Forward declarations
class GameSprite;

namespace rme {
	namespace render {

		class TextureManager;

		/// Configuration for texture garbage collection
		struct TextureCacheConfig {
			bool enabled = true;
			int cleanThreshold = 100; // Number of loaded textures before cleaning
			int cleanPulse = 30; // Seconds between cleanup attempts
		};

		/// Cache for managing texture lifecycle
		/// Handles garbage collection of unused textures
		/// Owns the cleanup logic previously in GraphicManager
		class TextureCache {
		public:
			explicit TextureCache(TextureManager& textureManager);
			~TextureCache();

			/// Set cache configuration
			void setConfig(const TextureCacheConfig& config) {
				config_ = config;
			}

			/// Get current configuration
			[[nodiscard]] const TextureCacheConfig& getConfig() const noexcept {
				return config_;
			}

			/// Perform garbage collection on unused textures
			/// Should be called periodically to free GPU memory
			void collectGarbage();

			/// Add a sprite to the cleanup queue
			/// @param sprite Sprite to mark for potential cleanup
			void addToCleanupQueue(GameSprite* sprite);

			/// Clear all cached data
			void clear();

			/// Get the last cleanup timestamp
			[[nodiscard]] int getLastCleanTime() const noexcept {
				return lastClean_;
			}

			/// Get the cleanup queue size
			[[nodiscard]] size_t getCleanupQueueSize() const noexcept {
				return cleanupQueue_.size();
			}

			// Prevent copying
			TextureCache(const TextureCache&) = delete;
			TextureCache& operator=(const TextureCache&) = delete;

		private:
			TextureManager& textureManager_;
			TextureCacheConfig config_;
			std::deque<GameSprite*> cleanupQueue_;
			int lastClean_ = 0;
		};

	} // namespace render
} // namespace rme

#endif // RME_TEXTURE_CACHE_H_
