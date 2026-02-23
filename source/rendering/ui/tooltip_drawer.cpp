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

#include "rendering/ui/tooltip_drawer.h"
#include "rendering/core/graphics.h"
#include "rendering/core/text_renderer.h"
#include "ui/theme.h"
#include <nanovg.h>
#include <format>
#include "rendering/core/coordinate_mapper.h"
#include <wx/wx.h>
#include "game/items.h"
#include "game/sprites.h"
#include "ui/gui.h"
#include <functional>

// Helper for hashing
inline void hash_combine(std::size_t& seed, const auto& v) {
	seed ^= std::hash<std::decay_t<decltype(v)>>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

static uint64_t computeTooltipHash(const TooltipData& data) {
	size_t hash = 0;
	hash_combine(hash, data.itemId);
	hash_combine(hash, data.actionId);
	hash_combine(hash, data.uniqueId);
	hash_combine(hash, data.doorId);
	hash_combine(hash, data.containerCapacity);
	hash_combine(hash, data.category);

	if (!data.itemName.empty()) hash_combine(hash, data.itemName);
	if (!data.text.empty()) hash_combine(hash, data.text);
	if (!data.description.empty()) hash_combine(hash, data.description);
	if (!data.waypointName.empty()) hash_combine(hash, data.waypointName);

	// Position destination
	if (data.destination.x > 0) {
		hash_combine(hash, data.destination.x);
		hash_combine(hash, data.destination.y);
		hash_combine(hash, data.destination.z);
	}

	// Container items
	for (const auto& item : data.containerItems) {
		hash_combine(hash, item.id);
		hash_combine(hash, item.count);
		hash_combine(hash, item.subtype);
	}

	return static_cast<uint64_t>(hash);
}

TooltipDrawer::TooltipDrawer() {
}

TooltipDrawer::~TooltipDrawer() {
	clear();
	if (lastContext) {
		for (auto& pair : spriteCache) {
			if (pair.second > 0) {
				nvgDeleteImage(lastContext, pair.second);
			}
		}
		spriteCache.clear();
	}
}

void TooltipDrawer::clear() {
	active_count = 0;
}

void TooltipDrawer::pruneCache() {
	if (current_frame_counter < 120) {
		return;
	}
	uint32_t threshold = current_frame_counter - 120;
	std::erase_if(layoutCache, [threshold](const auto& item) {
		return item.second.last_frame_seen < threshold;
	});
}

TooltipData& TooltipDrawer::requestTooltipData() {
	if (active_count >= tooltips.size()) {
		tooltips.emplace_back();
	}
	TooltipData& data = tooltips[active_count];
	data.clear();
	return data;
}

void TooltipDrawer::commitTooltip() {
	active_count++;
}

void TooltipDrawer::addItemTooltip(const TooltipData& data) {
	if (!data.hasVisibleFields()) {
		return;
	}
	TooltipData& dest = requestTooltipData();
	dest = data;
	commitTooltip();
}

void TooltipDrawer::addItemTooltip(TooltipData&& data) {
	if (!data.hasVisibleFields()) {
		return;
	}
	TooltipData& dest = requestTooltipData();
	dest = std::move(data);
	commitTooltip();
}

void TooltipDrawer::addWaypointTooltip(Position pos, std::string_view name) {
	if (name.empty()) {
		return;
	}
	TooltipData& data = requestTooltipData();
	data.pos = pos;
	data.category = TooltipCategory::WAYPOINT;
	data.waypointName = name;
	commitTooltip();
}

void TooltipDrawer::getHeaderColor(TooltipCategory cat, uint8_t& r, uint8_t& g, uint8_t& b) const {
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

int TooltipDrawer::getSpriteImage(NVGcontext* vg, uint16_t itemId) {
	if (itemId == 0) {
		return 0;
	}

	// Detect context change and clear cache
	if (vg != lastContext) {
		if (lastContext) {
			for (auto& pair : spriteCache) {
				if (pair.second > 0) {
					nvgDeleteImage(lastContext, pair.second);
				}
			}
		}
		spriteCache.clear();
		lastContext = vg;
	}

	// Resolve Item ID
	ItemType& it = g_items[itemId];
	if (!it.sprite) {
		return 0;
	}

	auto itCache = spriteCache.find(itemId);
	if (itCache != spriteCache.end()) {
		return itCache->second;
	}

	GameSprite* gameSprite = it.sprite;

	if (gameSprite && !gameSprite->spriteList.empty()) {
		GameSprite::NormalImage* img = gameSprite->spriteList[0];
		if (img) {
			std::unique_ptr<uint8_t[]> rgba;

			if (!g_gui.gfx.hasTransparency()) {
				std::unique_ptr<uint8_t[]> rgb = img->getRGBData();
				if (rgb) {
					rgba = std::make_unique<uint8_t[]>(32 * 32 * 4);
					for (int i = 0; i < 32 * 32; ++i) {
						uint8_t r = rgb[i * 3 + 0];
						uint8_t g = rgb[i * 3 + 1];
						uint8_t b = rgb[i * 3 + 2];

						if (r == 0xFF && g == 0x00 && b == 0xFF) {
							rgba[i * 4 + 0] = 0;
							rgba[i * 4 + 1] = 0;
							rgba[i * 4 + 2] = 0;
							rgba[i * 4 + 3] = 0;
						} else {
							rgba[i * 4 + 0] = r;
							rgba[i * 4 + 1] = g;
							rgba[i * 4 + 2] = b;
							rgba[i * 4 + 3] = 255;
						}
					}
				}
			}

			if (!rgba) {
				rgba = img->getRGBAData();
			}

			if (rgba) {
				int image = nvgCreateImageRGBA(vg, 32, 32, 0, rgba.get());
				if (image > 0) {
					if (spriteCache.contains(itemId) && spriteCache[itemId] > 0) {
						nvgDeleteImage(vg, spriteCache[itemId]);
					}
					spriteCache[itemId] = image;
					return image;
				}
			}
		}
	}
	return 0;
}

void TooltipDrawer::prepareFields(const TooltipData& tooltip, CachedTooltip& cached) {
	cached.fields.clear();
	std::string storage;
	storage.reserve(256);

	auto addField = [&](std::string label, std::string value, Theme::Role colorRole) {
		CachedTooltip::Field field;
		field.label = std::move(label);
		field.value = std::move(value);
		wxColour c = Theme::Get(colorRole);
		field.r = c.Red();
		field.g = c.Green();
		field.b = c.Blue();
		cached.fields.push_back(std::move(field));
	};

	if (tooltip.category == TooltipCategory::WAYPOINT) {
		addField("Waypoint", std::string(tooltip.waypointName), Theme::Role::TooltipWaypoint);
	} else {
		if (tooltip.actionId > 0) {
			storage.clear();
			std::format_to(std::back_inserter(storage), "{}", tooltip.actionId);
			addField("Action ID", storage, Theme::Role::TooltipActionId);
		}
		if (tooltip.uniqueId > 0) {
			storage.clear();
			std::format_to(std::back_inserter(storage), "{}", tooltip.uniqueId);
			addField("Unique ID", storage, Theme::Role::TooltipUniqueId);
		}
		if (tooltip.doorId > 0) {
			storage.clear();
			std::format_to(std::back_inserter(storage), "{}", tooltip.doorId);
			addField("Door ID", storage, Theme::Role::TooltipDoorId);
		}
		if (tooltip.destination.x > 0) {
			storage.clear();
			std::format_to(std::back_inserter(storage), "{}, {}, {}", tooltip.destination.x, tooltip.destination.y, tooltip.destination.z);
			addField("Destination", storage, Theme::Role::TooltipTeleport);
		}
		if (!tooltip.description.empty()) {
			addField("Description", std::string(tooltip.description), Theme::Role::TooltipBodyText);
		}
		if (!tooltip.text.empty()) {
			storage.clear();
			std::format_to(std::back_inserter(storage), "\"{}\"", tooltip.text);
			addField("Text", storage, Theme::Role::TooltipTextValue);
		}
	}
}

void TooltipDrawer::calculateLayout(NVGcontext* vg, const TooltipData& tooltip, CachedTooltip& cached, float maxWidth, float minWidth, float padding, float fontSize) {
	auto& lm = cached.metrics;
	lm = {};

	// Set up font for measurements
	nvgFontSize(vg, fontSize);
	nvgFontFace(vg, "sans");

	// Measure label widths
	float maxLabelWidth = 0.0f;
	for (const auto& field : cached.fields) {
		float labelBounds[4];
		nvgTextBounds(vg, 0, 0, field.label.data(), field.label.data() + field.label.size(), labelBounds);
		float lw = labelBounds[2] - labelBounds[0];
		if (lw > maxLabelWidth) {
			maxLabelWidth = lw;
		}
	}

	lm.valueStartX = maxLabelWidth + 12.0f; // Gap between label and value
	float maxValueWidth = maxWidth - lm.valueStartX - padding * 2;

	// Word wrap values that are too long
	int totalLines = 0;
	float actualMaxWidth = minWidth;

	for (auto& field : cached.fields) {
		const char* start = field.value.data();
		const char* end = start + field.value.length();

		// Check if value fits on one line
		float valueBounds[4];
		nvgTextBounds(vg, 0, 0, start, end, valueBounds);
		float valueWidth = valueBounds[2] - valueBounds[0];

		if (valueWidth <= maxValueWidth) {
			// Single line
			field.wrappedLines.push_back(field.value);
			totalLines++;
			float lineWidth = lm.valueStartX + valueWidth + padding * 2;
			if (lineWidth > actualMaxWidth) {
				actualMaxWidth = lineWidth;
			}
		} else {
			// Need to wrap - use NanoVG text breaking
			NVGtextRow rows[16];
			int nRows = nvgTextBreakLines(vg, start, end, maxValueWidth, rows, 16);

			for (int i = 0; i < nRows; i++) {
				std::string line(rows[i].start, rows[i].end - rows[i].start);
				field.wrappedLines.push_back(std::move(line));
				totalLines++;

				float lineWidth = lm.valueStartX + rows[i].width + padding * 2;
				if (lineWidth > actualMaxWidth) {
					actualMaxWidth = lineWidth;
				}
			}

			if (nRows == 0) {
				// Fallback if breaking failed
				field.wrappedLines.push_back(field.value);
				totalLines++;
			}
		}
	}

	// Calculate container grid dimensions
	lm.containerCols = 0;
	lm.containerRows = 0;
	lm.gridSlotSize = 34.0f; // 32px + padding
	lm.containerHeight = 0.0f;

	lm.numContainerItems = static_cast<int>(tooltip.containerItems.size());
	int capacity = static_cast<int>(tooltip.containerCapacity);
	lm.emptyContainerSlots = std::max(0, capacity - lm.numContainerItems);
	lm.totalContainerSlots = lm.numContainerItems;

	if (lm.emptyContainerSlots > 0) {
		lm.totalContainerSlots++; // Add one slot for the summary
	}

	// Apply a hard cap for visual safety
	if (lm.totalContainerSlots > 33) {
		lm.totalContainerSlots = 33;
	}

	if (capacity > 0 || lm.numContainerItems > 0) {
		// Heuristic: try to keep it somewhat square but matching width
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
			lm.containerHeight = lm.containerRows * lm.gridSlotSize + 4.0f; // + top margin
		}
	}

	// Calculate final box dimensions
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
}

void TooltipDrawer::drawBackground(NVGcontext* vg, float x, float y, float width, float height, float cornerRadius, const TooltipData& tooltip) {

	// Get border color based on category
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

	// Main background - use theme
	wxColour bgCol = Theme::Get(Theme::Role::TooltipBg);
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, width, height, cornerRadius);
	nvgFillColor(vg, nvgRGBA(bgCol.Red(), bgCol.Green(), bgCol.Blue(), 250));
	nvgFill(vg);

	// Full colored border around entire frame
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, width, height, cornerRadius);
	nvgStrokeColor(vg, nvgRGBA(borderR, borderG, borderB, 255));
	nvgStrokeWidth(vg, 1.0f);
	nvgStroke(vg);
}

void TooltipDrawer::drawFields(NVGcontext* vg, float x, float y, float valueStartX, float lineHeight, float padding, float fontSize, const CachedTooltip& cached) {

	float contentX = x + padding;
	float cursorY = y + padding;

	nvgFontSize(vg, fontSize);
	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

	for (const auto& field : cached.fields) {
		bool firstLine = true;
		for (const auto& line : field.wrappedLines) {
			if (firstLine) {
				// Draw label - use theme tooltip label color
				wxColour labelCol = Theme::Get(Theme::Role::TooltipLabel);
				nvgFillColor(vg, nvgRGBA(labelCol.Red(), labelCol.Green(), labelCol.Blue(), 200));
				nvgText(vg, contentX, cursorY, field.label.data(), field.label.data() + field.label.size());
				firstLine = false;
			}

			// Draw value line in theme-defined semantic color
			nvgFillColor(vg, nvgRGBA(field.r, field.g, field.b, 255));
			nvgText(vg, contentX + valueStartX, cursorY, line.data(), line.data() + line.size());

			cursorY += lineHeight;
		}
	}
}

void TooltipDrawer::drawContainerGrid(NVGcontext* vg, float x, float y, const TooltipData& tooltip, const CachedTooltip::LayoutMetrics& layout) {

	if (layout.totalContainerSlots <= 0) {
		return;
	}

	float textBlockHeight = layout.height - 20.0f; // Subtract padding * 2
	if (layout.totalContainerSlots > 0) {
		textBlockHeight -= (layout.containerHeight + 4.0f);
	}

	// Safety clamp
	if (textBlockHeight < 0) textBlockHeight = 0;

	float startY = y + 10.0f + textBlockHeight + 8.0f;
	float startX = x + 10.0f; // Padding

	for (int idx = 0; idx < layout.totalContainerSlots; ++idx) {
		int col = idx % layout.containerCols;
		int row = idx / layout.containerCols;

		float itemX = startX + col * layout.gridSlotSize;
		float itemY = startY + row * layout.gridSlotSize;

		// Draw slot background (always)
		nvgBeginPath(vg);
		wxColour baseCol = Theme::Get(Theme::Role::CardBase);
		wxColour borderCol = Theme::Get(Theme::Role::Border);

		nvgFillColor(vg, nvgRGBA(baseCol.Red(), baseCol.Green(), baseCol.Blue(), 100)); // Dark slot placeholder
		nvgStrokeColor(vg, nvgRGBA(borderCol.Red(), borderCol.Green(), borderCol.Blue(), 100)); // Light border
		nvgStrokeWidth(vg, 1.0f);
		nvgFill(vg);
		nvgStroke(vg);

		// Check if this is the summary info slot (last slot if we have empty spaces)
		bool isSummarySlot = (layout.emptyContainerSlots > 0 && idx == layout.totalContainerSlots - 1);

		if (isSummarySlot) {
			// Draw empty slots count: "+N"
			std::string summary = "+" + std::to_string(layout.emptyContainerSlots);

			nvgFontSize(vg, 12.0f);
			nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

			// Text shadow
			nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
			nvgText(vg, itemX + 17, itemY + 17, summary.c_str(), nullptr);

			// Text
			wxColour countCol = Theme::Get(Theme::Role::TooltipCountText);
			nvgFillColor(vg, nvgRGBA(countCol.Red(), countCol.Green(), countCol.Blue(), 255));
			nvgText(vg, itemX + 16, itemY + 16, summary.c_str(), nullptr);

		} else if (idx < layout.numContainerItems) {
			// Draw Actual Item
			const auto& item = tooltip.containerItems[idx];

			// Draw item sprite
			int img = getSpriteImage(vg, item.id);
			if (img > 0) {
				nvgBeginPath(vg);
				nvgRect(vg, itemX, itemY, 32, 32);
				nvgFillPaint(vg, nvgImagePattern(vg, itemX, itemY, 32, 32, 0, img, 1.0f));
				nvgFill(vg);
			}

			// Draw Count
			if (item.count > 1) {
				std::string countStr = std::to_string(item.count);
				nvgFontSize(vg, 10.0f);
				nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);

				// Text shadow
				nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
				nvgText(vg, itemX + 33, itemY + 33, countStr.c_str(), nullptr);

				// Text
				wxColour countCol = Theme::Get(Theme::Role::TooltipCountText);
				nvgFillColor(vg, nvgRGBA(countCol.Red(), countCol.Green(), countCol.Blue(), 255));
				nvgText(vg, itemX + 32, itemY + 32, countStr.c_str(), nullptr);
			}
		}
	}
}

void TooltipDrawer::draw(NVGcontext* vg, const RenderView& view) {
	if (!vg) {
		return;
	}

	current_frame_counter++;
	if (current_frame_counter % 60 == 0) {
		pruneCache();
	}

	for (size_t i = 0; i < active_count; ++i) {
		const auto& tooltip = tooltips[i];
		int unscaled_x, unscaled_y;
		view.getScreenPosition(tooltip.pos.x, tooltip.pos.y, tooltip.pos.z, unscaled_x, unscaled_y);

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

		uint64_t hash = computeTooltipHash(tooltip);

		auto it = layoutCache.find(hash);
		if (it == layoutCache.end()) {
			CachedTooltip cached;
			prepareFields(tooltip, cached);

			calculateLayout(vg, tooltip, cached, maxWidth, minWidth, padding, fontSize);
			it = layoutCache.insert({hash, std::move(cached)}).first;
		}

		CachedTooltip& cached = it->second;
		cached.last_frame_seen = current_frame_counter;

		// Skip if nothing to show
		if (cached.fields.empty() && tooltip.containerItems.empty()) {
			continue;
		}

		// Use cached layout metrics
		const auto& layout = cached.metrics;

		// Position tooltip above tile
		float tooltipX = screen_x - (layout.width / 2.0f);
		float tooltipY = screen_y - layout.height - 12.0f;

		// 3. Draw Background & Shadow
		drawBackground(vg, tooltipX, tooltipY, layout.width, layout.height, 4.0f, tooltip);

		// 4. Draw Text Fields
		drawFields(vg, tooltipX, tooltipY, layout.valueStartX, fontSize * 1.4f, padding, fontSize, cached);

		// 5. Draw Container Grid
		drawContainerGrid(vg, tooltipX, tooltipY, tooltip, layout);
	}
}
