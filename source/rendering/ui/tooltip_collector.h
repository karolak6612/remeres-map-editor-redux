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

#ifndef RME_TOOLTIP_COLLECTOR_H_
#define RME_TOOLTIP_COLLECTOR_H_

#include "rendering/ui/tooltip_data.h"
#include <span>

// Owns the per-frame tooltip data pool.
// Collects tooltip data during tile rendering with pool allocation semantics.
// Has no NanoVG or rendering dependency.
class TooltipCollector {
public:
	TooltipCollector() = default;

	// Add a structured tooltip for an item
	void addItemTooltip(const TooltipData& data) {
		if (!data.hasVisibleFields()) {
			return;
		}
		TooltipData& dest = requestTooltipData();
		dest = data;
		commitTooltip();
	}

	void addItemTooltip(TooltipData&& data) {
		if (!data.hasVisibleFields()) {
			return;
		}
		TooltipData& dest = requestTooltipData();
		dest = std::move(data);
		commitTooltip();
	}

	// Request a tooltip object from the pool. Call commitTooltip() to finalize.
	TooltipData& requestTooltipData() {
		if (active_count_ >= tooltips_.size()) {
			tooltips_.emplace_back();
		}
		TooltipData& data = tooltips_[active_count_];
		data.clear();
		return data;
	}

	void commitTooltip() {
		active_count_++;
	}

	// Add a waypoint tooltip
	void addWaypointTooltip(Position pos, std::string_view name) {
		if (name.empty()) {
			return;
		}
		TooltipData& data = requestTooltipData();
		data.pos = pos;
		data.category = TooltipCategory::WAYPOINT;
		data.waypointName = name;
		commitTooltip();
	}

	// Clear all tooltips for the next frame
	void clear() {
		active_count_ = 0;
	}

	// Get the collected tooltips for rendering
	std::span<const TooltipData> getTooltips() const {
		return std::span<const TooltipData>(tooltips_.data(), active_count_);
	}

	size_t count() const { return active_count_; }

private:
	std::vector<TooltipData> tooltips_;
	size_t active_count_ = 0;
};

#endif
