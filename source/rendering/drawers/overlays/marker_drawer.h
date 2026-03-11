#ifndef RME_MARKER_DRAWER_H_
#define RME_MARKER_DRAWER_H_

#include <cstdint>

struct MarkerRenderSnapshot;
struct RenderSettings;
class SpriteDrawer;
class SpriteBatch;

class MarkerDrawer {
public:
	MarkerDrawer();
	~MarkerDrawer();

	void draw(
		SpriteBatch& sprite_batch, SpriteDrawer* drawer, int draw_x, int draw_y, const MarkerRenderSnapshot& marker, const RenderSettings& settings
	);
};

#endif
