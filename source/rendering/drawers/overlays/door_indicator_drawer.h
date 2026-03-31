#ifndef RME_DOOR_INDICATOR_DRAWER_H_
#define RME_DOOR_INDICATOR_DRAWER_H_

#include <vector>
#include "map/position.h"

class AtlasManager;
class SpriteBatch;
struct RenderView;

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
	void draw(SpriteBatch& sprite_batch, const AtlasManager& atlas_manager, const RenderView& view) const;

private:
	std::vector<DoorRequest> requests;
};

#endif
