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
#include "texture_manager.h"
#include "../graphics.h"

namespace rme {
	namespace render {

		TextureManager::TextureManager() = default;

		TextureManager::~TextureManager() {
			clear();
		}

		Sprite* TextureManager::getSprite(int id) {
			auto it = spriteSpace_.find(id);
			if (it != spriteSpace_.end()) {
				return it->second;
			}
			return nullptr;
		}

		GameSprite* TextureManager::getCreatureSprite(int id) {
			if (id < 0) {
				return nullptr;
			}

			auto it = spriteSpace_.find(id + itemCount_);
			if (it != spriteSpace_.end()) {
				return dynamic_cast<GameSprite*>(it->second);
			}
			return nullptr;
		}

		void TextureManager::registerSprite(int id, Sprite* sprite) {
			// Clean up any existing sprite at this ID
			auto it = spriteSpace_.find(id);
			if (it != spriteSpace_.end()) {
				delete it->second;
			}
			spriteSpace_[id] = sprite;
		}

		void TextureManager::registerImage(int id, void* image) {
			imageSpace_[id] = image;
		}

		GLuint TextureManager::getFreeTextureID() {
			return nextTextureId_++;
		}

		void TextureManager::clear() {
			for (auto& pair : spriteSpace_) {
				delete pair.second;
			}
			spriteSpace_.clear();
			imageSpace_.clear();

			itemCount_ = 0;
			creatureCount_ = 0;
			loadedTextures_ = 0;
			unloaded_ = true;
		}

		void TextureManager::cleanSoftwareSprites() {
			for (auto& pair : spriteSpace_) {
				if (pair.second) {
					pair.second->unloadDC();
				}
			}
		}

	} // namespace render
} // namespace rme
