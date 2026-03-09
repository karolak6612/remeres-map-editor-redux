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

#ifndef RME_TOOLTIP_DRAWER_H_
#define RME_TOOLTIP_DRAWER_H_

#include "rendering/ui/tooltip_data.h"
#include "rendering/ui/tooltip_collector.h"
#include "rendering/core/render_view.h"
#include <unordered_map>
#include <sstream>

class Item;
class Waypoint;
struct NVGcontext;

/// Renders tooltip cards collected by TooltipCollector.
/// Also exposes collector methods for backward compatibility.
class TooltipDrawer {
public:
	TooltipDrawer();
	~TooltipDrawer();

	// --- Collector pass (delegates to owned collector) ---
	void addItemTooltip(const TooltipData& data) { collector.addItemTooltip(data); }
	void addItemTooltip(TooltipData&& data) { collector.addItemTooltip(std::move(data)); }
	TooltipData& requestTooltipData() { return collector.requestTooltipData(); }
	void commitTooltip() { collector.commitTooltip(); }
	void addWaypointTooltip(Position pos, std::string_view name) { collector.addWaypointTooltip(pos, name); }
	void clear() { collector.clear(); }

	// Direct access to the collector (for callers that want the separated interface)
	TooltipCollector& getCollector() { return collector; }

	// --- Renderer pass ---
	void draw(NVGcontext* vg, const RenderView& view);

protected:
	TooltipCollector collector;

	struct FieldLine {
		std::string_view label;
		std::string_view value;
		uint8_t r, g, b;
		std::vector<std::string_view> wrappedLines; // For multi-line values
	};
	std::vector<FieldLine> scratch_fields;
	size_t scratch_fields_count = 0;
	std::string storage; // Scratch buffer for text generation

	std::unordered_map<uint32_t, int> spriteCache; // sprite_id -> nvg image handle
	NVGcontext* lastContext = nullptr;

	// Helper to get or load sprite image
	int getSpriteImage(NVGcontext* vg, uint16_t itemId);

	// Helper to get header color based on category
	void getHeaderColor(TooltipCategory cat, uint8_t& r, uint8_t& g, uint8_t& b) const;

	// Refactored drawing helpers
	struct LayoutMetrics {
		float width;
		float height;
		float valueStartX;
		float gridSlotSize;
		int containerCols;
		int containerRows;
		float containerHeight;
		int totalContainerSlots;
		int emptyContainerSlots;
		int numContainerItems;
	};

	void prepareFields(const TooltipData& tooltip);
	LayoutMetrics calculateLayout(NVGcontext* vg, const TooltipData& tooltip, float maxWidth, float minWidth, float padding, float fontSize);
	void drawBackground(NVGcontext* vg, float x, float y, float width, float height, float cornerRadius, const TooltipData& tooltip);
	void drawFields(NVGcontext* vg, float x, float y, float valueStartX, float lineHeight, float padding, float fontSize);
	void drawContainerGrid(NVGcontext* vg, float x, float y, const TooltipData& tooltip, const LayoutMetrics& layout);
};

#endif
