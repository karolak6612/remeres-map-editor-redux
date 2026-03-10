#ifndef RME_HOOK_INDICATOR_DRAWER_H_
#define RME_HOOK_INDICATOR_DRAWER_H_

#include <vector>
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

	void addHook(const Position& pos, bool south, bool east);
	void clear();
	void draw(NVGcontext* vg, const ViewState& view);

private:
	std::vector<HookRequest> requests;
};

#endif
