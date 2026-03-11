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

#ifndef RME_TEXTURE_GARBAGE_COLLECTOR_H
#define RME_TEXTURE_GARBAGE_COLLECTOR_H

#include <cstdint>
#include <deque>
#include <memory>
#include <time.h>
#include <vector>

class GameSprite;
class Image;
class Sprite;

class TextureGarbageCollector {
public:
	TextureGarbageCollector();
	~TextureGarbageCollector();

	void GarbageCollect(std::vector<GameSprite*>& resident_game_sprites, std::vector<Image*>& resident_images, time_t current_time);
	void AddSpriteToCleanup(const std::vector<GameSprite*>& resident_game_sprites, uint32_t sprite_id);
	void CleanSoftwareSprites(std::vector<std::unique_ptr<Sprite>>& sprite_space);
	void Clear();

	void NotifyTextureLoaded();
	void NotifyTextureUnloaded();

	int GetLoadedTexturesCount() const {
		return loaded_textures;
	}

private:
	int loaded_textures;
	time_t lastclean;
	std::deque<uint32_t> cleanup_list_;
};

#endif
