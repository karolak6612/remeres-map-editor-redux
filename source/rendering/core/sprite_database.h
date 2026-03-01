#ifndef RME_RENDERING_CORE_SPRITE_DATABASE_H_
#define RME_RENDERING_CORE_SPRITE_DATABASE_H_

#include "rendering/core/game_sprite.h"
#include "rendering/core/image.h"
#include <vector>
#include <unordered_map>
#include <memory>

class SpriteDatabase {
public:
	SpriteDatabase() = default;
	~SpriteDatabase() = default;

	// Non-copyable/movable to prevent accidental copies of the god object replacement
	SpriteDatabase(const SpriteDatabase&) = delete;
	SpriteDatabase& operator=(const SpriteDatabase&) = delete;

	void clear();

	Sprite* getSprite(int id);
	GameSprite* getCreatureSprite(int id);
	void insertSprite(int id, std::unique_ptr<Sprite> sprite);


	uint16_t getItemSpriteMaxID() const { return item_count; }
	uint16_t getCreatureSpriteMaxID() const { return creature_count; }

	void setItemSpriteMaxID(uint16_t item_count) { this->item_count = item_count; }
	void setCreatureSpriteMaxID(uint16_t creature_count) { this->creature_count = creature_count; }

	// Access for Loader/Preloader/GC
	std::vector<std::unique_ptr<Sprite>>& getSpriteSpace() { return sprite_space; }
	std::unordered_map<int, std::unique_ptr<Sprite>>& getEditorSpriteSpace() { return editor_sprite_space; }
	std::vector<std::unique_ptr<Image>>& getImageSpace() { return image_space; }

private:
	std::vector<std::unique_ptr<Sprite>> sprite_space;
	std::vector<std::unique_ptr<Image>> image_space;
	std::unordered_map<int, std::unique_ptr<Sprite>> editor_sprite_space;

	uint16_t item_count = 0;
	uint16_t creature_count = 0;
};

#endif
