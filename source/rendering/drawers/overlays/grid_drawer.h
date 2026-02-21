#ifndef RME_RENDERING_GRID_DRAWER_H_
#define RME_RENDERING_GRID_DRAWER_H_

#include <wx/colour.h>

struct RenderView;
struct DrawingOptions;
class ISpriteSink;

class GridDrawer {
public:
	void DrawGrid(ISpriteSink& sprite_sink, const RenderView& view, const DrawingOptions& options);
	void DrawIngameBox(ISpriteSink& sprite_sink, const RenderView& view, const DrawingOptions& options);
	void DrawNodeLoadingPlaceholder(ISpriteSink& sprite_sink, int nd_map_x, int nd_map_y, const RenderView& view);

private:
	void drawRect(ISpriteSink& sprite_sink, int x, int y, int w, int h, const wxColor& color, int width = 1);
	void drawFilledRect(ISpriteSink& sprite_sink, int x, int y, int w, int h, const wxColor& color);
};

#endif
