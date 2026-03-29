#ifndef RME_VISUAL_OVERLAY_DRAWER_H_
#define RME_VISUAL_OVERLAY_DRAWER_H_

#include <string>
#include <vector>

#include <wx/colour.h>

#include "map/position.h"

struct NVGcontext;
struct RenderView;

enum class VisualOverlayPlacement {
	TileFill,
	TileInset,
	TileCenter,
};

struct VisualOverlayRequest {
	Position pos;
	std::string asset_path;
	wxColour color = wxColour(255, 255, 255, 255);
	VisualOverlayPlacement placement = VisualOverlayPlacement::TileInset;
};

class VisualOverlayDrawer {
public:
	void add(VisualOverlayRequest request);
	void clear();
	void draw(NVGcontext* vg, const RenderView& view);

private:
	std::vector<VisualOverlayRequest> requests;
};

#endif
