#ifndef RME_RENDERING_SELECTION_DRAWER_H_
#define RME_RENDERING_SELECTION_DRAWER_H_

struct ViewState;
class MapCanvas;
struct DrawingOptions;
class PrimitiveRenderer;

class SelectionDrawer {
public:
	void draw(PrimitiveRenderer& primitive_renderer, const ViewState& view, const MapCanvas* canvas, const DrawingOptions& options);
};

#endif
