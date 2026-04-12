//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_SPRITE_DRAWER_H_
#define RME_RENDERING_SPRITE_DRAWER_H_

#include "app/main.h"

#include <wx/colour.h>
#include "app/definitions.h"
#include "item_definitions/core/item_definition_types.h"

// Forward declarations
class GameSprite;
struct AtlasRegion;
class SpriteBatch;
class AtlasManager;

class SpriteDrawer {
public:
	SpriteDrawer();
	~SpriteDrawer();

	void // gl API removed
	void // gl API removed
	void // gl API removed
	void // gl API removed

	void BlitSprite(SpriteBatch& sprite_batch, int screenx, int screeny, ServerItemId server_item_id, DrawColor color = {});
	void BlitSprite(SpriteBatch& sprite_batch, int screenx, int screeny, GameSprite* spr, DrawColor color = {});
};

#endif
