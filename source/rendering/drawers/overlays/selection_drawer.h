#ifndef RME_RENDERING_SELECTION_DRAWER_H_
#define RME_RENDERING_SELECTION_DRAWER_H_

struct RenderView;
class MapCanvas;
struct DrawingOptions;
class PrimitiveRenderer;

class SelectionDrawer {
public:
	void draw(PrimitiveRenderer& primitive_renderer, const RenderView& view, const MapCanvas* canvas, const DrawingOptions& options);
};

#endif
