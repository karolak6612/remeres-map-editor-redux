#ifndef RME_RENDERING_SELECTION_DRAWER_H_
#define RME_RENDERING_SELECTION_DRAWER_H_

struct ViewState;
struct ViewSnapshot;
struct RenderSettings;
class PrimitiveRenderer;

class SelectionDrawer {
public:
	void draw(PrimitiveRenderer& primitive_renderer, const ViewState& view, const ViewSnapshot& snapshot, const RenderSettings& settings);
};

#endif
