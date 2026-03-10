//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_ITEM_DRAWER_H_
#define RME_RENDERING_ITEM_DRAWER_H_

#include "app/definitions.h"
#include "item_definitions/core/item_definition_store.h"
#include "map/position.h"

// Forward declarations
class SpriteDrawer;
class CreatureDrawer;
class Tile;
class Item;
class ISpriteResolver;
class GameSprite;

struct RenderSettings;
struct FrameOptions;
class SpriteBatch;
class AtlasManager;
struct SpritePatterns;

struct BlitItemParams {
	const Tile* tile = nullptr;
	Position pos;
	Item* item = nullptr;
	ItemDefinitionView item_definition;
	const RenderSettings* settings = nullptr;
	const FrameOptions* frame = nullptr;
	const SpritePatterns* patterns = nullptr;
	bool ephemeral = false;
	int red = 255;
	int green = 255;
	int blue = 255;
	int alpha = 255;

	BlitItemParams(const Tile* t, Item* i, const RenderSettings& s, const FrameOptions& f);
	BlitItemParams(const Position& p, Item* i, const RenderSettings& s, const FrameOptions& f);
};

class ItemDrawer {
public:
	ItemDrawer();
	~ItemDrawer();

	void BlitItem(SpriteBatch& sprite_batch, const AtlasManager& atlas, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer, int& draw_x, int& draw_y, const BlitItemParams& params);

	void DrawRawBrush(SpriteBatch& sprite_batch, SpriteDrawer* sprite_drawer, int screenx, int screeny, ServerItemId item_id, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha);

	void SetSpriteResolver(ISpriteResolver* resolver) {
		sprite_resolver = resolver;
	}

private:
	GameSprite* resolveSprite(const ItemDefinitionView& definition) const;
	GameSprite* resolveSprite(ServerItemId item_id) const;

	ISpriteResolver* sprite_resolver = nullptr;
};

#endif
