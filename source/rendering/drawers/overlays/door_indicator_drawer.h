#ifndef RME_DOOR_INDICATOR_DRAWER_H_
#define RME_DOOR_INDICATOR_DRAWER_H_

#include <vector>
#include "map/position.h"

#include "rendering/core/draw_context.h"

struct NVGcontext;

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

	void addDoor(const Position& pos, bool locked, bool south, bool east);
	void clear();
	void draw(NVGcontext* vg, const DrawContext& ctx);

private:
	std::vector<DoorRequest> requests;
};

#endif
