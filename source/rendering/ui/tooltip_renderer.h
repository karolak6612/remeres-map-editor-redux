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

#ifndef RME_TOOLTIP_RENDERER_H_
#define RME_TOOLTIP_RENDERER_H_

#include "rendering/ui/tooltip_data.h"
#include "rendering/core/render_view.h"
#include <span>
#include <string>
#include <string_view>
#include <vector>

struct NVGcontext;
class NVGImageCache;

// Renders tooltip cards using NanoVG.
// Accepts collected tooltip data and an image cache, knows nothing
// about how the data was gathered.
class TooltipRenderer {
public:
	TooltipRenderer() = default;

	// Draw all tooltips from the collected data
	void draw(NVGcontext* vg, const ViewState& view, std::span<const TooltipData> tooltips, NVGImageCache& imageCache);

private:
	// Layout scratch buffers (reused across frames)
	struct FieldLine {
		std::string_view label;
		std::string_view value;
		uint8_t r, g, b;
		std::vector<std::string_view> wrappedLines;
	};
	std::vector<FieldLine> scratch_fields_;
	size_t scratch_fields_count_ = 0;
	std::string storage_; // Scratch buffer for text generation

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
	void drawContainerGrid(NVGcontext* vg, float x, float y, const TooltipData& tooltip, const LayoutMetrics& layout, NVGImageCache& imageCache);

	static void getHeaderColor(TooltipCategory cat, uint8_t& r, uint8_t& g, uint8_t& b);
};

#endif
