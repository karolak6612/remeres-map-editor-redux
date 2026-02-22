//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_SPRITE_DRAWER_H_
#define RME_RENDERING_SPRITE_DRAWER_H_

#include "app/main.h"

#include <wx/colour.h>
#include "app/definitions.h"

// Forward declarations
class GameSprite;
struct AtlasRegion;
class SpriteBatch;
class AtlasManager;

class SpriteDrawer {
public:
	SpriteDrawer();
	~SpriteDrawer();

	void ResetCache();

	void glBlitAtlasQuad(SpriteBatch& sprite_batch, int sx, int sy, const AtlasRegion* region, DrawColor color = {});
	void glBlitSquare(SpriteBatch& sprite_batch, int sx, int sy, int red, int green, int blue, int alpha, int size = 0);
	void glDrawBox(SpriteBatch& sprite_batch, int sx, int sy, int width, int height, int red, int green, int blue, int alpha);
	void glSetColor(wxColor color);

	void BlitSprite(SpriteBatch& sprite_batch, int screenx, int screeny, uint32_t spriteid, DrawColor color = {});
	void BlitSprite(SpriteBatch& sprite_batch, int screenx, int screeny, GameSprite* spr, DrawColor color = {});

private:
	// Texture bind caching for performance
	GLuint last_bound_texture_ = 0;
};

#endif
