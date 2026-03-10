#ifndef RME_DOOR_INDICATOR_DRAWER_H_
#define RME_DOOR_INDICATOR_DRAWER_H_

#include <span>
#include "map/position.h"

struct NVGcontext;
struct ViewState;

class DoorIndicatorDrawer {
public:
	DoorIndicatorDrawer();
	~DoorIndicatorDrawer();

	struct DoorRequest {
		Position pos;
		bool locked;
		bool south;
		bool east;
	};

	void draw(NVGcontext* vg, const ViewState& view, std::span<const DoorRequest> doors);
};

#endif
