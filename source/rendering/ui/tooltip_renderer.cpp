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

#include "rendering/ui/tooltip_renderer.h"
#include "rendering/ui/nvg_image_cache.h"
#include "ui/theme.h"
#include <nanovg.h>
#include <format>

void TooltipRenderer::draw(NVGcontext* vg, const ViewState& view, std::span<const TooltipData> tooltips, NVGImageCache& imageCache) {
	if (!vg) {
		return;
	}

	for (const auto& tooltip : tooltips) {
		const auto [unscaled_x, unscaled_y] = view.getScreenPosition(tooltip.pos.x, tooltip.pos.y, tooltip.pos.z);

		float zoom = view.zoom < 0.01f ? 1.0f : view.zoom;

		float screen_x = unscaled_x / zoom;
		float screen_y = unscaled_y / zoom;
		float tile_size_screen = 32.0f / zoom;

		// Center on tile
		screen_x += tile_size_screen / 2.0f;
		screen_y += tile_size_screen / 2.0f;

		// Constants
		float fontSize = 11.0f;
		float padding = 10.0f;
		float minWidth = 120.0f;
		float maxWidth = 220.0f;

		// 1. Prepare Content
		prepareFields(tooltip);

		// Skip if nothing to show
		if (scratch_fields_count_ == 0 && tooltip.containerItems.empty()) {
			continue;
		}

		// 2. Calculate Layout
		LayoutMetrics layout = calculateLayout(vg, tooltip, maxWidth, minWidth, padding, fontSize);

		// Position tooltip above tile
		float tooltipX = screen_x - (layout.width / 2.0f);
		float tooltipY = screen_y - layout.height - 12.0f;

		// 3. Draw Background & Shadow
		drawBackground(vg, tooltipX, tooltipY, layout.width, layout.height, 4.0f, tooltip);

		// 4. Draw Text Fields
		drawFields(vg, tooltipX, tooltipY, layout.valueStartX, fontSize * 1.4f, padding, fontSize);

		// 5. Draw Container Grid
		drawContainerGrid(vg, tooltipX, tooltipY, tooltip, layout, imageCache);
	}
}

void TooltipRenderer::getHeaderColor(TooltipCategory cat, uint8_t& r, uint8_t& g, uint8_t& b) {
	wxColour color;
	switch (cat) {
		case TooltipCategory::WAYPOINT:
			color = Theme::Get(Theme::Role::TooltipBorderWaypoint);
			break;
		case TooltipCategory::DOOR:
			color = Theme::Get(Theme::Role::TooltipBorderDoor);
			break;
		case TooltipCategory::TELEPORT:
			color = Theme::Get(Theme::Role::TooltipBorderTeleport);
			break;
		case TooltipCategory::TEXT:
			color = Theme::Get(Theme::Role::TooltipBorderText);
			break;
		case TooltipCategory::ITEM:
		default:
			color = Theme::Get(Theme::Role::TooltipBorderItem);
			break;
	}
	r = color.Red();
	g = color.Green();
	b = color.Blue();
}

void TooltipRenderer::prepareFields(const TooltipData& tooltip) {
	scratch_fields_count_ = 0;
	storage_.clear();
	if (storage_.capacity() < 4096) {
		storage_.reserve(4096);
	}

	// Two-phase approach to avoid string_view invalidation from storage_ reallocation.
	// Phase 1: Format all values into storage_, recording offsets.
	// Phase 2: Create FieldLines with string_views into the now-stable storage_ buffer.

	struct PendingField {
		std::string_view label;
		size_t value_offset; // SIZE_MAX = external string_view
		size_t value_length;
		std::string_view external_value;
		Theme::Role colorRole;
	};
	PendingField pending[8];
	size_t pending_count = 0;

	if (tooltip.category == TooltipCategory::WAYPOINT) {
		auto& pf = pending[pending_count++];
		pf.label = "Waypoint";
		pf.value_offset = SIZE_MAX;
		pf.external_value = tooltip.waypointName;
		pf.colorRole = Theme::Role::TooltipWaypoint;
	} else {
		if (tooltip.actionId > 0) {
			auto& pf = pending[pending_count++];
			pf.label = "Action ID";
			pf.value_offset = storage_.size();
			std::format_to(std::back_inserter(storage_), "{}", tooltip.actionId);
			pf.value_length = storage_.size() - pf.value_offset;
			pf.colorRole = Theme::Role::TooltipActionId;
		}
		if (tooltip.uniqueId > 0) {
			auto& pf = pending[pending_count++];
			pf.label = "Unique ID";
			pf.value_offset = storage_.size();
			std::format_to(std::back_inserter(storage_), "{}", tooltip.uniqueId);
			pf.value_length = storage_.size() - pf.value_offset;
			pf.colorRole = Theme::Role::TooltipUniqueId;
		}
		if (tooltip.doorId > 0) {
			auto& pf = pending[pending_count++];
			pf.label = "Door ID";
			pf.value_offset = storage_.size();
			std::format_to(std::back_inserter(storage_), "{}", tooltip.doorId);
			pf.value_length = storage_.size() - pf.value_offset;
			pf.colorRole = Theme::Role::TooltipDoorId;
		}
		if (tooltip.has_destination) {
			auto& pf = pending[pending_count++];
			pf.label = "Destination";
			pf.value_offset = storage_.size();
			std::format_to(std::back_inserter(storage_), "{}, {}, {}", tooltip.destination.x, tooltip.destination.y, tooltip.destination.z);
			pf.value_length = storage_.size() - pf.value_offset;
			pf.colorRole = Theme::Role::TooltipTeleport;
		}
		if (!tooltip.description.empty()) {
			auto& pf = pending[pending_count++];
			pf.label = "Description";
			pf.value_offset = SIZE_MAX;
			pf.external_value = tooltip.description;
			pf.colorRole = Theme::Role::TooltipBodyText;
		}
		if (!tooltip.text.empty()) {
			auto& pf = pending[pending_count++];
			pf.label = "Text";
			pf.value_offset = storage_.size();
			std::format_to(std::back_inserter(storage_), "\"{}\"", tooltip.text);
			pf.value_length = storage_.size() - pf.value_offset;
			pf.colorRole = Theme::Role::TooltipTextValue;
		}
	}

	// Phase 2: storage_ is now stable — create FieldLines with safe string_views
	for (size_t i = 0; i < pending_count; ++i) {
		const auto& pf = pending[i];
		if (scratch_fields_count_ >= scratch_fields_.size()) {
			scratch_fields_.emplace_back();
		}
		FieldLine& field = scratch_fields_[scratch_fields_count_++];
		field.label = pf.label;
		field.value = (pf.value_offset == SIZE_MAX)
			? pf.external_value
			: std::string_view(storage_.data() + pf.value_offset, pf.value_length);
		wxColour c = Theme::Get(pf.colorRole);
		field.r = c.Red();
		field.g = c.Green();
		field.b = c.Blue();
		field.wrappedLines.clear();
	}
}

TooltipRenderer::LayoutMetrics TooltipRenderer::calculateLayout(NVGcontext* vg, const TooltipData& tooltip, float maxWidth, float minWidth, float padding, float fontSize) {
	LayoutMetrics lm = {};

	nvgFontSize(vg, fontSize);
	nvgFontFace(vg, "sans");

	// Measure label widths
	float maxLabelWidth = 0.0f;
	for (size_t i = 0; i < scratch_fields_count_; ++i) {
		const auto& field = scratch_fields_[i];
		float labelBounds[4];
		nvgTextBounds(vg, 0, 0, field.label.data(), field.label.data() + field.label.size(), labelBounds);
		float lw = labelBounds[2] - labelBounds[0];
		if (lw > maxLabelWidth) {
			maxLabelWidth = lw;
		}
	}

	lm.valueStartX = maxLabelWidth + 12.0f;
	float maxValueWidth = maxWidth - lm.valueStartX - padding * 2;

	int totalLines = 0;
	float actualMaxWidth = minWidth;

	for (size_t i = 0; i < scratch_fields_count_; ++i) {
		auto& field = scratch_fields_[i];
		const char* start = field.value.data();
		const char* end = start + field.value.length();

		float valueBounds[4];
		nvgTextBounds(vg, 0, 0, start, end, valueBounds);
		float valueWidth = valueBounds[2] - valueBounds[0];

		if (valueWidth <= maxValueWidth) {
			field.wrappedLines.push_back(field.value);
			totalLines++;
			float lineWidth = lm.valueStartX + valueWidth + padding * 2;
			if (lineWidth > actualMaxWidth) {
				actualMaxWidth = lineWidth;
			}
		} else {
			NVGtextRow rows[16];
			int nRows = nvgTextBreakLines(vg, start, end, maxValueWidth, rows, 16);

			for (int j = 0; j < nRows; j++) {
				std::string_view line(rows[j].start, rows[j].end - rows[j].start);
				field.wrappedLines.push_back(line);
				totalLines++;

				float lineWidth = lm.valueStartX + rows[j].width + padding * 2;
				if (lineWidth > actualMaxWidth) {
					actualMaxWidth = lineWidth;
				}
			}

			if (nRows == 0) {
				field.wrappedLines.push_back(field.value);
				totalLines++;
			}
		}
	}

	// Calculate container grid dimensions
	lm.containerCols = 0;
	lm.containerRows = 0;
	lm.gridSlotSize = 34.0f;
	lm.containerHeight = 0.0f;

	lm.numContainerItems = static_cast<int>(tooltip.containerItems.size());
	int capacity = static_cast<int>(tooltip.containerCapacity);
	lm.emptyContainerSlots = std::max(0, capacity - lm.numContainerItems);
	lm.totalContainerSlots = lm.numContainerItems;

	if (lm.emptyContainerSlots > 0) {
		lm.totalContainerSlots++;
	}

	if (lm.totalContainerSlots > 33) {
		lm.totalContainerSlots = 33;
	}

	if (capacity > 0 || lm.numContainerItems > 0) {
		lm.containerCols = std::min(4, lm.totalContainerSlots);
		if (lm.totalContainerSlots > 4) {
			lm.containerCols = 5;
		}
		if (lm.totalContainerSlots > 10) {
			lm.containerCols = 6;
		}
		if (lm.totalContainerSlots > 15) {
			lm.containerCols = 8;
		}

		if (lm.containerCols == 0 && lm.totalContainerSlots > 0) {
			lm.containerCols = 1;
		}

		if (lm.containerCols > 0) {
			lm.containerRows = (lm.totalContainerSlots + lm.containerCols - 1) / lm.containerCols;
			lm.containerHeight = lm.containerRows * lm.gridSlotSize + 4.0f;
		}
	}

	lm.width = std::min(maxWidth + padding * 2, std::max(minWidth, actualMaxWidth));

	bool hasContainer = lm.totalContainerSlots > 0;
	if (hasContainer) {
		float gridWidth = lm.containerCols * lm.gridSlotSize;
		lm.width = std::max(lm.width, gridWidth + padding * 2);
	}

	float lineHeight = fontSize * 1.4f;
	lm.height = totalLines * lineHeight + padding * 2;
	if (hasContainer) {
		lm.height += lm.containerHeight + 4.0f;
	}

	return lm;
}

void TooltipRenderer::drawBackground(NVGcontext* vg, float x, float y, float width, float height, float cornerRadius, const TooltipData& tooltip) {
	uint8_t borderR, borderG, borderB;
	getHeaderColor(tooltip.category, borderR, borderG, borderB);

	// Shadow (multi-layer soft shadow)
	for (int i = 3; i >= 0; i--) {
		float alpha = 35.0f + (3 - i) * 20.0f;
		float spread = i * 2.0f;
		float offsetY = 3.0f + i * 1.0f;
		nvgBeginPath(vg);
		nvgRoundedRect(vg, x - spread, y + offsetY - spread, width + spread * 2, height + spread * 2, cornerRadius + spread);
		nvgFillColor(vg, nvgRGBA(0, 0, 0, static_cast<unsigned char>(alpha)));
		nvgFill(vg);
	}

	// Main background
	wxColour bgCol = Theme::Get(Theme::Role::TooltipBg);
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, width, height, cornerRadius);
	nvgFillColor(vg, nvgRGBA(bgCol.Red(), bgCol.Green(), bgCol.Blue(), 250));
	nvgFill(vg);

	// Colored border
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, width, height, cornerRadius);
	nvgStrokeColor(vg, nvgRGBA(borderR, borderG, borderB, 255));
	nvgStrokeWidth(vg, 1.0f);
	nvgStroke(vg);
}

void TooltipRenderer::drawFields(NVGcontext* vg, float x, float y, float valueStartX, float lineHeight, float padding, float fontSize) {
	float contentX = x + padding;
	float cursorY = y + padding;

	nvgFontSize(vg, fontSize);
	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

	for (size_t i = 0; i < scratch_fields_count_; ++i) {
		const auto& field = scratch_fields_[i];
		bool firstLine = true;
		for (const auto& line : field.wrappedLines) {
			if (firstLine) {
				wxColour labelCol = Theme::Get(Theme::Role::TooltipLabel);
				nvgFillColor(vg, nvgRGBA(labelCol.Red(), labelCol.Green(), labelCol.Blue(), 200));
				nvgText(vg, contentX, cursorY, field.label.data(), field.label.data() + field.label.size());
				firstLine = false;
			}

			nvgFillColor(vg, nvgRGBA(field.r, field.g, field.b, 255));
			nvgText(vg, contentX + valueStartX, cursorY, line.data(), line.data() + line.size());

			cursorY += lineHeight;
		}
	}
}

void TooltipRenderer::drawContainerGrid(NVGcontext* vg, float x, float y, const TooltipData& tooltip, const LayoutMetrics& layout, NVGImageCache& imageCache) {
	if (layout.totalContainerSlots <= 0) {
		return;
	}

	float fontSize = 11.0f;
	float lineHeight = fontSize * 1.4f;
	float textBlockHeight = 0.0f;
	for (size_t i = 0; i < scratch_fields_count_; ++i) {
		const auto& field = scratch_fields_[i];
		textBlockHeight += field.wrappedLines.size() * lineHeight;
	}

	float startY = y + 10.0f + textBlockHeight + 8.0f;
	float startX = x + 10.0f;

	for (int idx = 0; idx < layout.totalContainerSlots; ++idx) {
		int col = idx % layout.containerCols;
		int row = idx / layout.containerCols;

		float itemX = startX + col * layout.gridSlotSize;
		float itemY = startY + row * layout.gridSlotSize;

		// Draw slot background
		nvgBeginPath(vg);
		nvgRect(vg, itemX, itemY, layout.gridSlotSize, layout.gridSlotSize);
		wxColour baseCol = Theme::Get(Theme::Role::CardBase);
		wxColour borderCol = Theme::Get(Theme::Role::Border);

		nvgFillColor(vg, nvgRGBA(baseCol.Red(), baseCol.Green(), baseCol.Blue(), 100));
		nvgStrokeColor(vg, nvgRGBA(borderCol.Red(), borderCol.Green(), borderCol.Blue(), 100));
		nvgStrokeWidth(vg, 1.0f);
		nvgFill(vg);
		nvgStroke(vg);

		bool isSummarySlot = (layout.emptyContainerSlots > 0 && idx == layout.totalContainerSlots - 1);

		if (isSummarySlot) {
			std::string summary = "+" + std::to_string(layout.emptyContainerSlots);

			nvgFontSize(vg, 12.0f);
			nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

			nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
			nvgText(vg, itemX + 17, itemY + 17, summary.c_str(), nullptr);

			wxColour countCol = Theme::Get(Theme::Role::TooltipCountText);
			nvgFillColor(vg, nvgRGBA(countCol.Red(), countCol.Green(), countCol.Blue(), 255));
			nvgText(vg, itemX + 16, itemY + 16, summary.c_str(), nullptr);

		} else if (idx < layout.numContainerItems) {
			const auto& item = tooltip.containerItems[idx];

			int img = imageCache.getSpriteImage(vg, item.id);
			if (img > 0) {
				nvgBeginPath(vg);
				nvgRect(vg, itemX, itemY, 32, 32);
				nvgFillPaint(vg, nvgImagePattern(vg, itemX, itemY, 32, 32, 0, img, 1.0f));
				nvgFill(vg);
			}

			if (item.count > 1) {
				std::string countStr = std::to_string(item.count);
				nvgFontSize(vg, 10.0f);
				nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);

				nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
				nvgText(vg, itemX + 33, itemY + 33, countStr.c_str(), nullptr);

				wxColour countCol = Theme::Get(Theme::Role::TooltipCountText);
				nvgFillColor(vg, nvgRGBA(countCol.Red(), countCol.Green(), countCol.Blue(), 255));
				nvgText(vg, itemX + 32, itemY + 32, countStr.c_str(), nullptr);
			}
		}
	}
}
