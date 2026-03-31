#ifndef RME_VISUAL_OVERLAY_DRAWER_H_
#define RME_VISUAL_OVERLAY_DRAWER_H_

#include <vector>

#include <wx/colour.h>

#include "map/position.h"

class AtlasManager;
class SpriteBatch;
struct RenderView;

enum class VisualOverlayPlacement {
	TileFill,
	TileInset,
	TileCenter,
};

struct VisualOverlayRequest {
	Position pos;
	uint32_t atlas_sprite_id = 0;
	wxColour color = wxColour(255, 255, 255, 255);
	VisualOverlayPlacement placement = VisualOverlayPlacement::TileInset;
};

class VisualOverlayDrawer {
public:
	void add(VisualOverlayRequest request);
	void clear();
	void draw(SpriteBatch& sprite_batch, const AtlasManager& atlas_manager, const RenderView& view) const;

private:
	std::vector<VisualOverlayRequest> requests;
};

#endif
