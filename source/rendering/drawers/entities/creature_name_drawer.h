#ifndef RME_RENDERING_DRAWERS_ENTITIES_CREATURE_NAME_DRAWER_H_
#define RME_RENDERING_DRAWERS_ENTITIES_CREATURE_NAME_DRAWER_H_

#include "app/definitions.h"
#include <span>
#include <string>

struct ViewState;
struct NVGcontext;
#include "map/position.h"

struct CreatureLabel {
	Position pos;
	std::string name;
};

class CreatureNameDrawer {
public:
	CreatureNameDrawer();
	~CreatureNameDrawer();

	void draw(NVGcontext* vg, const ViewState& view, std::span<const CreatureLabel> labels);
};

#endif
