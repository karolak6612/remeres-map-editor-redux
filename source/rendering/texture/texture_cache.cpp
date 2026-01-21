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

#include "main.h"
#include "texture_cache.h"
#include "texture_manager.h"
#include "../graphics.h"

namespace rme {
	namespace render {

		TextureCache::TextureCache(TextureManager& textureManager) : textureManager_(textureManager) {
			lastClean_ = static_cast<int>(std::time(nullptr));
		}

		TextureCache::~TextureCache() {
			clear();
		}

		void TextureCache::collectGarbage() {
			if (!config_.enabled) {
				return;
			}

			int currentTime = static_cast<int>(std::time(nullptr));
			int loadedTextures = textureManager_.getLoadedTextureCount();

			// Only clean if we have enough textures and enough time has passed
			if (loadedTextures > config_.cleanThreshold && currentTime - lastClean_ > config_.cleanPulse) {

				// Note: Image cleanup must be done through GameSprite's public interface
				// since GameSprite::Image is protected. We only clean GameSprite instances.

				// Clean game sprites in the sprite space
				auto& spriteSpace = textureManager_.getSpriteSpace();
				for (auto& pair : spriteSpace) {
					GameSprite* gs = dynamic_cast<GameSprite*>(pair.second);
					if (gs) {
						gs->clean(currentTime);
					}
				}

				lastClean_ = currentTime;
			}
		}

		void TextureCache::addToCleanupQueue(GameSprite* sprite) {
			if (!sprite) {
				return;
			}

			// Check if sprite is already in queue
			for (auto* existing : cleanupQueue_) {
				if (existing == sprite) {
					return;
				}
			}

			cleanupQueue_.push_back(sprite);
		}

		void TextureCache::clear() {
			cleanupQueue_.clear();
			lastClean_ = static_cast<int>(std::time(nullptr));
		}

	} // namespace render
} // namespace rme
