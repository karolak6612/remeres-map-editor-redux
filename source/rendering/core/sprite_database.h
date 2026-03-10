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

#ifndef RME_RENDERING_CORE_SPRITE_DATABASE_H_
#define RME_RENDERING_CORE_SPRITE_DATABASE_H_

#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>

class Sprite;
class GameSprite;
class Image;

class SpriteDatabase {
public:
	using SpriteVector = std::vector<std::unique_ptr<Sprite>>;
	using ImageVector = std::vector<std::unique_ptr<Image>>;

	SpriteDatabase();
	~SpriteDatabase();

	SpriteDatabase(SpriteDatabase&&) noexcept;
	SpriteDatabase& operator=(SpriteDatabase&&) noexcept;

	Sprite* getSprite(int id) const;
	void insertSprite(int id, std::unique_ptr<Sprite> sprite);
	GameSprite* getCreatureSprite(int id, uint16_t item_count) const;

	void clear();
	void resize(size_t sprite_size, size_t image_size);

	SpriteVector& sprites() { return sprite_space_; }
	const SpriteVector& sprites() const { return sprite_space_; }

	ImageVector& images() { return image_space_; }
	const ImageVector& images() const { return image_space_; }

	std::unordered_map<int, std::unique_ptr<Sprite>>& editorSprites() { return editor_sprite_space_; }

	std::vector<void*>& residentImages() { return resident_images_; }
	std::vector<GameSprite*>& residentGameSprites() { return resident_game_sprites_; }

private:
	SpriteVector sprite_space_;
	ImageVector image_space_;
	std::unordered_map<int, std::unique_ptr<Sprite>> editor_sprite_space_;

	std::vector<void*> resident_images_;
	std::vector<GameSprite*> resident_game_sprites_;
};

#endif
