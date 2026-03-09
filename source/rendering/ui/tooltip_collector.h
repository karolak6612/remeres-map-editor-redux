//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_TOOLTIP_COLLECTOR_H_
#define RME_TOOLTIP_COLLECTOR_H_

#include "rendering/ui/tooltip_data.h"

/// Collects tooltip data during the tile rendering pass.
/// Uses a pooled vector to avoid per-frame allocations.
class TooltipCollector {
public:
	TooltipCollector() = default;
	~TooltipCollector() = default;

	// Request a tooltip object from the pool. Call commitTooltip() to finalize.
	TooltipData& requestTooltipData() {
		if (active_count >= tooltips.size()) {
			tooltips.emplace_back();
		}
		tooltips[active_count].clear();
		return tooltips[active_count];
	}

	void commitTooltip() {
		if (active_count < tooltips.size()) {
			tooltips[active_count].updateCategory();
			++active_count;
		}
	}

	// Add a structured tooltip for an item
	void addItemTooltip(const TooltipData& data) {
		if (active_count >= tooltips.size()) {
			tooltips.push_back(data);
		} else {
			tooltips[active_count] = data;
		}
		tooltips[active_count].updateCategory();
		++active_count;
	}

	void addItemTooltip(TooltipData&& data) {
		if (active_count >= tooltips.size()) {
			tooltips.push_back(std::move(data));
		} else {
			tooltips[active_count] = std::move(data);
		}
		tooltips[active_count].updateCategory();
		++active_count;
	}

	// Add a waypoint tooltip
	void addWaypointTooltip(Position pos, std::string_view name) {
		TooltipData data(pos, name);
		addItemTooltip(std::move(data));
	}

	// Clear all tooltips for next frame
	void clear() {
		active_count = 0;
	}

	// Read access for the renderer
	const std::vector<TooltipData>& getTooltips() const { return tooltips; }
	size_t getActiveCount() const { return active_count; }

private:
	std::vector<TooltipData> tooltips;
	size_t active_count = 0;
};

#endif
