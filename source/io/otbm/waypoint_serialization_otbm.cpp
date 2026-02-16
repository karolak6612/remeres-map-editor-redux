#include "waypoint_serialization_otbm.h"

#include "map/map.h"
#include <spdlog/spdlog.h>
#include <format>

void WaypointSerializationOTBM::readWaypoints(Map& map, BinaryNode* mapNode) {
	spdlog::debug("Reading OTBM_WAYPOINTS...");
	for (BinaryNode* waypointNode = mapNode->getChild(); waypointNode != nullptr; waypointNode = waypointNode->advance()) {
		uint8_t waypoint_type;
		if (!waypointNode->getByte(waypoint_type)) {
			spdlog::warn("Invalid waypoint node: failed to read type byte");
			continue;
		}
		if (waypoint_type != OTBM_WAYPOINT) {
			spdlog::warn("Invalid waypoint node type: {} (expected {})", static_cast<int>(waypoint_type), static_cast<int>(OTBM_WAYPOINT));
			continue;
		}

		Waypoint wp;
		if (!waypointNode->getString(wp.name)) {
			spdlog::warn("Failed to read waypoint name");
			continue;
		}

		uint16_t x, y;
		uint8_t z;
		if (!waypointNode->getU16(x) || !waypointNode->getU16(y) || !waypointNode->getU8(z)) {
			spdlog::warn("Invalid position for waypoint '{}'", wp.name);
			continue;
		}
		wp.pos = { x, y, z };

		map.waypoints.addWaypoint(std::make_unique<Waypoint>(wp));
	}
}

bool WaypointSerializationOTBM::writeWaypoints(const Map& map, NodeFileWriteHandle& f, MapVersion mapVersion) {
	bool waypointsWarning = false;
	const bool supportWaypoints = mapVersion.otbm >= MAP_OTBM_3;

	if (!map.waypoints.waypoints.empty()) {
		if (!supportWaypoints) {
			waypointsWarning = true;
		}

		f.addNode(OTBM_WAYPOINTS);
		for (const auto& [name, waypoint_ptr] : map.waypoints) {
			const Waypoint* waypoint = waypoint_ptr.get();
			f.addNode(OTBM_WAYPOINT);
			f.addString(waypoint->name);
			f.addU16(waypoint->pos.x);
			f.addU16(waypoint->pos.y);
			f.addU8(waypoint->pos.z);
			f.endNode();
		}
		f.endNode();
	}
	return waypointsWarning;
}
