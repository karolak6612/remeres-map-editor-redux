#ifndef RME_RENDERING_GRID_DRAWER_H_
#define RME_RENDERING_GRID_DRAWER_H_

#include <wx/colour.h>

struct DrawContext;
struct ViewState;
struct ViewBounds;
class SpriteBatch;
class AtlasManager;

class GridDrawer {
public:
	void DrawGrid(const DrawContext& ctx, const ViewBounds& bounds);
	void DrawIngameBox(const DrawContext& ctx, const ViewBounds& bounds);
	void DrawNodeLoadingPlaceholder(SpriteBatch& sprite_batch, const AtlasManager& atlas, int nd_map_x, int nd_map_y, const ViewState& view);

private:
	void drawRect(SpriteBatch& sprite_batch, const AtlasManager& atlas, int x, int y, int w, int h, const wxColor& color, int width = 1);
	void drawFilledRect(SpriteBatch& sprite_batch, const AtlasManager& atlas, int x, int y, int w, int h, const wxColor& color);
};

#endif
