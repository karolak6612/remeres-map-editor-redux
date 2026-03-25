//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_SPRITE_DRAWER_H_
#define RME_RENDERING_SPRITE_DRAWER_H_

#include "app/main.h"

#include "app/definitions.h"
#include "item_definitions/core/item_definition_types.h"

// Forward declarations
class GameSprite;
struct AtlasRegion;
class SpriteBatch;
class AtlasManager;

class ISpriteResolver;

class SpriteDrawer {
public:
    SpriteDrawer();
    ~SpriteDrawer();

    void glBlitAtlasQuad(SpriteBatch& sprite_batch, int sx, int sy, const AtlasRegion* region, DrawColor color = {});
    void glBlitSquare(SpriteBatch& sprite_batch, const AtlasManager& atlas, int sx, int sy, DrawColor color, int size = 0);
    void glDrawBox(SpriteBatch& sprite_batch, const AtlasManager& atlas, int sx, int sy, int width, int height, DrawColor color);
    void BlitSprite(SpriteBatch& sprite_batch, int screenx, int screeny, ServerItemId server_item_id, DrawColor color = {});
    void BlitSprite(SpriteBatch& sprite_batch, int screenx, int screeny, GameSprite* spr, DrawColor color = {});

    void SetSpriteResolver(ISpriteResolver* resolver)
    {
        sprite_resolver = resolver;
    }

private:
    ISpriteResolver* sprite_resolver = nullptr;
};

#endif
