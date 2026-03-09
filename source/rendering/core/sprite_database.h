//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_SPRITE_DATABASE_H_
#define RME_SPRITE_DATABASE_H_

#include <memory>
#include <vector>
#include <unordered_map>

class Sprite;
class Image;
class GameSprite;

/// Owns all sprite and image objects indexed by ID.
/// Extracted from GraphicManager to separate storage from lifecycle management.
class SpriteDatabase {
public:
	SpriteDatabase() = default;
	~SpriteDatabase() = default;

	SpriteDatabase(const SpriteDatabase&) = delete;
	SpriteDatabase& operator=(const SpriteDatabase&) = delete;

	// Lookup
	Sprite* getSprite(int id) const;
	GameSprite* getCreatureSprite(int id) const;
	uint16_t getItemSpriteMaxID() const { return item_count; }
	uint16_t getCreatureSpriteMaxID() const { return creature_count; }

	// Insertion
	void insertSprite(int id, std::unique_ptr<Sprite> sprite);

	// Clear all sprite/image storage
	void clear();

	// Direct access for assembler and preloader
	// (These provide controlled access to internal containers)
	std::vector<std::unique_ptr<Sprite>>& spriteSpace() { return sprite_space; }
	const std::vector<std::unique_ptr<Sprite>>& spriteSpace() const { return sprite_space; }

	std::vector<std::unique_ptr<Image>>& imageSpace() { return image_space; }
	const std::vector<std::unique_ptr<Image>>& imageSpace() const { return image_space; }

	void setItemCount(uint16_t count) { item_count = count; }
	void setCreatureCount(uint16_t count) { creature_count = count; }

private:
	// Indexed by ID for O(1) access
	std::vector<std::unique_ptr<Sprite>> sprite_space;
	std::vector<std::unique_ptr<Image>> image_space;

	// Editor sprites use negative IDs
	std::unordered_map<int, std::unique_ptr<Sprite>> editor_sprite_space;

	uint16_t item_count = 0;
	uint16_t creature_count = 0;
};

#endif
