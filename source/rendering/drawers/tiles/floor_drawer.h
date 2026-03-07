//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_FLOOR_DRAWER_H_
#define RME_RENDERING_FLOOR_DRAWER_H_

#include "rendering/core/draw_context.h"
#include "rendering/core/floor_view_params.h"

class Editor;
class ItemDrawer;
class SpriteDrawer;
class CreatureDrawer;

struct TileRenderContext;

class FloorDrawer {
public:
	FloorDrawer();
	~FloorDrawer();

	void draw(const DrawContext& ctx, const FloorViewParams& floor_params, const TileRenderContext& render_ctx);
};

#endif
