#ifndef RME_RENDERING_GRID_DRAWER_H_
#define RME_RENDERING_GRID_DRAWER_H_

#include <wx/colour.h>

#include "rendering/core/draw_context.h"

struct ViewBounds;

class AtlasManager;
class GridDrawer {
public:
	void DrawGrid(const DrawContext& ctx, const ViewBounds& bounds);
	void DrawIngameBox(const DrawContext& ctx, const ViewBounds& bounds);
	void DrawNodeLoadingPlaceholder(const DrawContext& ctx, int nd_map_x, int nd_map_y);

private:
	void drawRect(SpriteBatch& sprite_batch, int x, int y, int w, int h, const wxColor& color, const AtlasManager& atlas, int width = 1);
	void drawFilledRect(SpriteBatch& sprite_batch, int x, int y, int w, int h, const wxColor& color, const AtlasManager& atlas);

	const AtlasManager* ensureAtlasManager() const;
};

#endif
