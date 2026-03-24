#ifndef RME_HOOK_INDICATOR_DRAWER_H_
#define RME_HOOK_INDICATOR_DRAWER_H_

#include <span>
#include "map/position.h"

struct NVGcontext;
struct ViewState;

class HookIndicatorDrawer {
public:
	HookIndicatorDrawer();
	~HookIndicatorDrawer();

	struct HookRequest {
		Position pos;
		bool south;
		bool east;
	};

	void draw(NVGcontext* vg, const ViewState& view, std::span<const HookRequest> hooks);
};

#endif
