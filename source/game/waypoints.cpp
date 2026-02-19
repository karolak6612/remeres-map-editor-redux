//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include "game/waypoints.h"
#include "map/map.h"

void Waypoints::addWaypoint(std::unique_ptr<Waypoint> wp, bool replace) {
	if (getWaypoint(wp->name) && !replace) {
		return;
	}
	removeWaypoint(wp->name);

	Waypoint* raw_wp = wp.get();
	if (raw_wp->pos != Position()) {
		Tile* t = map.getTile(raw_wp->pos);
		if (!t) {
			t = map.createTile(raw_wp->pos.x, raw_wp->pos.y, raw_wp->pos.z);
		}
		t->getLocation()->increaseWaypointCount();
		position_index.emplace(raw_wp->pos, raw_wp);
	}
	waypoints.insert(std::make_pair(as_lower_str(raw_wp->name), std::move(wp)));
}

Waypoint* Waypoints::getWaypoint(std::string name) {
	to_lower_str(name);
	WaypointMap::iterator iter = waypoints.find(name);
	if (iter == waypoints.end()) {
		return nullptr;
	}
	return iter->second.get();
}

Waypoint* Waypoints::getWaypoint(TileLocation* location) {
	if (!location) {
		return nullptr;
	}
	auto it = position_index.find(location->getPosition());
	if (it != position_index.end()) {
		return it->second;
	}
	return nullptr;
}

void Waypoints::removeWaypoint(std::string name) {
	to_lower_str(name);
	WaypointMap::iterator iter = waypoints.find(name);
	if (iter == waypoints.end()) {
		return;
	}

	Waypoint* wp = iter->second.get();
	if (wp->pos != Position()) {
		TileLocation* loc = map.getTileL(wp->pos);
		if (loc) {
			loc->decreaseWaypointCount();
		}

		auto range = position_index.equal_range(wp->pos);
		for (auto it = range.first; it != range.second; ++it) {
			if (it->second == wp) {
				position_index.erase(it);
				break;
			}
		}
	}

	waypoints.erase(iter);
}

void Waypoints::updateWaypointPosition(Waypoint* wp, const Position& newPos) {
	if (!wp || wp->pos == newPos) {
		return;
	}

	// Remove from old position
	if (wp->pos != Position()) {
		TileLocation* oldLoc = map.getTileL(wp->pos);
		if (oldLoc) {
			oldLoc->decreaseWaypointCount();
		}

		auto range = position_index.equal_range(wp->pos);
		for (auto it = range.first; it != range.second; ++it) {
			if (it->second == wp) {
				position_index.erase(it);
				break;
			}
		}
	}

	// Update position
	wp->pos = newPos;

	// Add to new position
	if (wp->pos != Position()) {
		Tile* t = map.getTile(wp->pos);
		if (!t) {
			t = map.createTile(wp->pos.x, wp->pos.y, wp->pos.z);
		}
		t->getLocation()->increaseWaypointCount();
		position_index.emplace(wp->pos, wp);
	}
}
