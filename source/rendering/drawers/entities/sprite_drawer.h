//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_SPRITE_DRAWER_H_
#define RME_RENDERING_SPRITE_DRAWER_H_

#include "app/main.h"

#include <wx/colour.h>
#include "app/definitions.h"
#include "rendering/core/draw_context.h"

// Forward declarations
class GameSprite;
class AtlasManager;
struct AtlasRegion;

class SpriteDrawer {
public:
	SpriteDrawer();
	~SpriteDrawer();

	void glBlitAtlasQuad(const DrawContext& ctx, int sx, int sy, const AtlasRegion* region, DrawColor color = {});
	void glBlitSquare(const DrawContext& ctx, int sx, int sy, DrawColor color, int size = 0);
	void glDrawBox(const DrawContext& ctx, int sx, int sy, int width, int height, DrawColor color);
	void glSetColor(wxColor color);

	void BlitSprite(const DrawContext& ctx, int screenx, int screeny, uint32_t spriteid, DrawColor color = {});
	void BlitSprite(const DrawContext& ctx, int screenx, int screeny, GameSprite* spr, DrawColor color = {});
};

#endif
