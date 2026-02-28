#ifndef RME_RENDERING_GRID_DRAWER_H_
#define RME_RENDERING_GRID_DRAWER_H_

#include <wx/colour.h>

struct RenderView;
struct DrawingOptions;
struct ViewBounds;
class SpriteBatch;

class GridDrawer {
public:
	void DrawGrid(SpriteBatch& sprite_batch, const RenderView& view, const DrawingOptions& options, const ViewBounds& bounds);
	void DrawIngameBox(SpriteBatch& sprite_batch, const RenderView& view, const DrawingOptions& options, const ViewBounds& bounds);
	void DrawNodeLoadingPlaceholder(SpriteBatch& sprite_batch, int nd_map_x, int nd_map_y, const RenderView& view);

private:
	void drawRect(SpriteBatch& sprite_batch, int x, int y, int w, int h, const wxColor& color, int width = 1);
	void drawFilledRect(SpriteBatch& sprite_batch, int x, int y, int w, int h, const wxColor& color);
};

#endif
