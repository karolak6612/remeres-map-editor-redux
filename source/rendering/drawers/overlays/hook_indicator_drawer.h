#ifndef RME_HOOK_INDICATOR_DRAWER_H_
#define RME_HOOK_INDICATOR_DRAWER_H_

#include <vector>
#include "map/position.h"

class AtlasManager;
class SpriteBatch;
struct RenderView;

class HookIndicatorDrawer {
public:
	HookIndicatorDrawer();
	~HookIndicatorDrawer();

	struct HookRequest {
		Position pos;
		bool south;
		bool east;
	};

	void addHook(const Position& pos, bool south, bool east);
	void clear();
	void draw(SpriteBatch& sprite_batch, const AtlasManager& atlas_manager, const RenderView& view) const;

private:
	std::vector<HookRequest> requests;
};

#endif
