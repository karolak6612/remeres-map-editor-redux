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
#include <nanovg.h>
#include <format>
#include "rendering/core/coordinate_mapper.h"
#include <wx/wx.h>
#include "game/items.h"
#include "game/sprites.h"
#include "ui/gui.h"
#include <functional>

static inline void hash_combine(size_t& seed, size_t value) {
	seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

size_t TooltipData::computeHash() const {
	size_t seed = 0;
	hash_combine(seed, std::hash<int>{}(static_cast<int>(category)));
	hash_combine(seed, std::hash<uint16_t>{}(itemId));
	hash_combine(seed, std::hash<uint16_t>{}(actionId));
	hash_combine(seed, std::hash<uint16_t>{}(uniqueId));
	hash_combine(seed, std::hash<uint8_t>{}(doorId));
	hash_combine(seed, std::hash<uint8_t>{}(containerCapacity));

	if (!itemName.empty()) hash_combine(seed, std::hash<std::string_view>{}(itemName));
	if (!text.empty()) hash_combine(seed, std::hash<std::string_view>{}(text));
	if (!description.empty()) hash_combine(seed, std::hash<std::string_view>{}(description));
	if (!waypointName.empty()) hash_combine(seed, std::hash<std::string_view>{}(waypointName));

	hash_combine(seed, std::hash<int>{}(destination.x));
	hash_combine(seed, std::hash<int>{}(destination.y));
	hash_combine(seed, std::hash<int>{}(destination.z));

	for (const auto& item : containerItems) {
		hash_combine(seed, std::hash<uint16_t>{}(item.id));
		hash_combine(seed, std::hash<uint8_t>{}(item.count));
		hash_combine(seed, std::hash<uint8_t>{}(item.subtype));
	}
	return seed;
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
	using namespace TooltipColors;
	switch (cat) {
		case TooltipCategory::WAYPOINT:
			r = WAYPOINT_HEADER_R;
			g = WAYPOINT_HEADER_G;
			b = WAYPOINT_HEADER_B;
			break;
		case TooltipCategory::DOOR:
			r = DOOR_HEADER_R;
			g = DOOR_HEADER_G;
			b = DOOR_HEADER_B;
			break;
		case TooltipCategory::TELEPORT:
			r = TELEPORT_HEADER_R;
			g = TELEPORT_HEADER_G;
			b = TELEPORT_HEADER_B;
			break;
		case TooltipCategory::TEXT:
			r = TEXT_HEADER_R;
			g = TEXT_HEADER_G;
			b = TEXT_HEADER_B;
			break;
		case TooltipCategory::ITEM:
		default:
			r = ITEM_HEADER_R;
			g = ITEM_HEADER_G;
			b = ITEM_HEADER_B;
			break;
	}
}

int TooltipDrawer::getSpriteImage(NVGcontext* vg, uint16_t itemId) {
	if (itemId == 0) {
		return 0;
	}

	// Detect context change and clear cache
	if (vg != lastContext) {
		// If we had a previous context, we'd ideally delete images from it,
		// but if the context pointer changed, the old one might be invalid.
		// However, adhering to the request to clear cache with nvgDeleteImage:
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

	// We use the item ID as the cache key since it's unique and stable
	auto itCache = spriteCache.find(itemId);
	if (itCache != spriteCache.end()) {
		return itCache->second;
	}

	// Use the sprite directly from ItemType
	GameSprite* gameSprite = it.sprite;

	if (gameSprite && !gameSprite->spriteList.empty()) {
		// Use the first frame/part of the sprite
		GameSprite::NormalImage* img = gameSprite->spriteList[0];
		if (img) {
			std::unique_ptr<uint8_t[]> rgba;

			// For legacy sprites (no transparency), use getRGBData + Magenta Masking
			// This matches how WxWidgets/SpriteIconGenerator renders icons
			if (!g_gui.gfx.hasTransparency()) {
				std::unique_ptr<uint8_t[]> rgb = img->getRGBData();
				if (rgb) {
					rgba = std::make_unique<uint8_t[]>(32 * 32 * 4);
					for (int i = 0; i < 32 * 32; ++i) {
						uint8_t r = rgb[i * 3 + 0];
						uint8_t g = rgb[i * 3 + 1];
						uint8_t b = rgb[i * 3 + 2];

						// Magic Pink (Magenta) is transparent for legacy sprites
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
				} else {
					// getRGBData failed, should ideally not happen if sprite exists logic is correct
				}
			}

			// Fallback/Standard path for alpha sprites or if RGB failed
			if (!rgba) {
				rgba = img->getRGBAData();
				if (!rgba) {
					// getRGBAData failed
				}
			}

			if (rgba) {
				int image = nvgCreateImageRGBA(vg, 32, 32, 0, rgba.get());
				if (image == 0) {
					// nvgCreateImageRGBA failed
				} else {
					// Success
					// Check if we are overwriting an existing valid image (shouldn't happen given find() above, but safest)
					if (spriteCache.contains(itemId) && spriteCache[itemId] > 0) {
						nvgDeleteImage(vg, spriteCache[itemId]);
					}
					spriteCache[itemId] = image;
					return image;
				}
			}
		}
	} else {
		// GameSprite missing or empty list
	}

	return 0;
}

void TooltipDrawer::prepareFields(const TooltipData& tooltip) {
	// Build content lines with word wrapping support
	using namespace TooltipColors;
	scratch_fields_count = 0;
	storage.clear();
	if (storage.capacity() < 4096) {
		storage.reserve(4096);
	}

	auto addField = [&](std::string_view label, std::string_view value, uint8_t r, uint8_t g, uint8_t b) {
		if (scratch_fields_count >= scratch_fields.size()) {
			scratch_fields.emplace_back();
		}
		FieldLine& field = scratch_fields[scratch_fields_count++];
		field.label = label;
		field.value = value;
		field.r = r;
		field.g = g;
		field.b = b;
		field.wrappedLines.clear();
	};

	if (tooltip.category == TooltipCategory::WAYPOINT) {
		addField("Waypoint", tooltip.waypointName, WAYPOINT_HEADER_R, WAYPOINT_HEADER_G, WAYPOINT_HEADER_B);
	} else {
		if (tooltip.actionId > 0) {
			size_t start = storage.size();
			std::format_to(std::back_inserter(storage), "{}", tooltip.actionId);
			addField("Action ID", std::string_view(storage.data() + start, storage.size() - start), ACTION_ID_R, ACTION_ID_G, ACTION_ID_B);
		}
		if (tooltip.uniqueId > 0) {
			size_t start = storage.size();
			std::format_to(std::back_inserter(storage), "{}", tooltip.uniqueId);
			addField("Unique ID", std::string_view(storage.data() + start, storage.size() - start), UNIQUE_ID_R, UNIQUE_ID_G, UNIQUE_ID_B);
		}
		if (tooltip.doorId > 0) {
			size_t start = storage.size();
			std::format_to(std::back_inserter(storage), "{}", tooltip.doorId);
			addField("Door ID", std::string_view(storage.data() + start, storage.size() - start), DOOR_ID_R, DOOR_ID_G, DOOR_ID_B);
		}
		if (tooltip.destination.x > 0) {
			size_t start = storage.size();
			std::format_to(std::back_inserter(storage), "{}, {}, {}", tooltip.destination.x, tooltip.destination.y, tooltip.destination.z);
			addField("Destination", std::string_view(storage.data() + start, storage.size() - start), TELEPORT_DEST_R, TELEPORT_DEST_G, TELEPORT_DEST_B);
		}
		if (!tooltip.description.empty()) {
			addField("Description", tooltip.description, BODY_TEXT_R, BODY_TEXT_G, BODY_TEXT_B);
		}
		if (!tooltip.text.empty()) {
			size_t start = storage.size();
			std::format_to(std::back_inserter(storage), "\"{}\"", tooltip.text);
			addField("Text", std::string_view(storage.data() + start, storage.size() - start), TEXT_R, TEXT_G, TEXT_B);
		}
	}
}

TooltipDrawer::LayoutMetrics TooltipDrawer::calculateLayout(NVGcontext* vg, const TooltipData& tooltip, float maxWidth, float minWidth, float padding, float fontSize) {
	LayoutMetrics lm = {};

	// Set up font for measurements
	nvgFontSize(vg, fontSize);
	nvgFontFace(vg, "sans");

	// Measure label widths
	float maxLabelWidth = 0.0f;
	for (size_t i = 0; i < scratch_fields_count; ++i) {
		const auto& field = scratch_fields[i];
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

	for (size_t i = 0; i < scratch_fields_count; ++i) {
		auto& field = scratch_fields[i];
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
				std::string_view line(rows[i].start, rows[i].end - rows[i].start);
				field.wrappedLines.push_back(line);
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

	return lm;
}

void TooltipDrawer::drawBackground(NVGcontext* vg, float x, float y, float width, float height, float cornerRadius, const TooltipData& tooltip) {
	using namespace TooltipColors;

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

	// Main background
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, width, height, cornerRadius);
	nvgFillColor(vg, nvgRGBA(BODY_BG_R, BODY_BG_G, BODY_BG_B, 250));
	nvgFill(vg);

	// Full colored border around entire frame
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, width, height, cornerRadius);
	nvgStrokeColor(vg, nvgRGBA(borderR, borderG, borderB, 255));
	nvgStrokeWidth(vg, 1.0f);
	nvgStroke(vg);
}

void TooltipDrawer::drawFields(NVGcontext* vg, const std::vector<FieldLine>& fields, float x, float y, float valueStartX, float lineHeight, float padding, float fontSize) {
	using namespace TooltipColors;

	float contentX = x + padding;
	float cursorY = y + padding;

	nvgFontSize(vg, fontSize);
	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

	for (const auto& field : fields) {
		bool firstLine = true;
		for (const auto& line : field.wrappedLines) {
			if (firstLine) {
				// Draw label on first line
				nvgFillColor(vg, nvgRGBA(BODY_TEXT_R, BODY_TEXT_G, BODY_TEXT_B, 160));
				nvgText(vg, contentX, cursorY, field.label.data(), field.label.data() + field.label.size());
				firstLine = false;
			}

			// Draw value line in semantic color
			nvgFillColor(vg, nvgRGBA(field.r, field.g, field.b, 255));
			nvgText(vg, contentX + valueStartX, cursorY, line.data(), line.data() + line.size());

			cursorY += lineHeight;
		}
	}
}

void TooltipDrawer::drawContainerGrid(NVGcontext* vg, const std::vector<FieldLine>& fields, float x, float y, const TooltipData& tooltip, const LayoutMetrics& layout) {
	using namespace TooltipColors;

	if (layout.totalContainerSlots <= 0) {
		return;
	}

	// Calculate cursorY after text fields
	// We need to re-calculate text height or pass it, but simpler to deduce from logic:
	// The grid is at the bottom. We can just use the bottom of the box minus grid height minus padding.
	// But let's calculate exact start Y based on text content for precision
	float fontSize = 11.0f;
	float lineHeight = fontSize * 1.4f;
	float textBlockHeight = 0.0f;
	for (const auto& field : fields) {
		textBlockHeight += field.wrappedLines.size() * lineHeight;
	}

	float startY = y + 10.0f + textBlockHeight + 8.0f; // Padding + Text + Spacer
	float startX = x + 10.0f; // Padding

	for (int idx = 0; idx < layout.totalContainerSlots; ++idx) {
		int col = idx % layout.containerCols;
		int row = idx / layout.containerCols;

		float itemX = startX + col * layout.gridSlotSize;
		float itemY = startY + row * layout.gridSlotSize;

		// Draw slot background (always)
		nvgBeginPath(vg);
		nvgRect(vg, itemX, itemY, 32, 32);
		nvgFillColor(vg, nvgRGBA(60, 60, 60, 100)); // Dark slot placeholder
		nvgStrokeColor(vg, nvgRGBA(100, 100, 100, 100)); // Light border
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
			nvgFillColor(vg, nvgRGBA(COUNT_TEXT_R, COUNT_TEXT_G, COUNT_TEXT_B, 255));
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
				nvgFillColor(vg, nvgRGBA(COUNT_TEXT_R, COUNT_TEXT_G, COUNT_TEXT_B, 255));
				nvgText(vg, itemX + 32, itemY + 32, countStr.c_str(), nullptr);
			}
		}
	}
}

void TooltipDrawer::draw(NVGcontext* vg, const RenderView& view) {
	if (!vg) {
		return;
	}

	frame_counter++;

	// Garbage collect every 600 frames
	if (frame_counter % 600 == 0) {
		std::erase_if(layoutCache, [this](const auto& pair) {
			return (frame_counter - pair.second.last_frame_used) > 600;
		});
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

		size_t hash = tooltip.computeHash();
		LayoutMetrics layout;
		std::vector<FieldLine> draw_fields;
		bool cached = false;

		auto it = layoutCache.find(hash);
		if (it != layoutCache.end()) {
			it->second.last_frame_used = frame_counter;
			const auto& entry = it->second;
			layout = entry.layout;
			cached = true;

			// Construct lightweight FieldLines pointing to cached strings
			draw_fields.reserve(entry.fields.size());
			for (const auto& cf : entry.fields) {
				FieldLine fl;
				fl.label = cf.label;
				fl.value = cf.value;
				fl.r = cf.r;
				fl.g = cf.g;
				fl.b = cf.b;
				for (const auto& wl : cf.wrappedLines) {
					fl.wrappedLines.push_back(wl);
				}
				draw_fields.push_back(std::move(fl));
			}
		}

		if (!cached) {
			// 1. Prepare Content
			prepareFields(tooltip);

			// Skip if nothing to show
			if (scratch_fields_count == 0 && tooltip.containerItems.empty()) {
				continue;
			}

			// 2. Calculate Layout
			layout = calculateLayout(vg, tooltip, maxWidth, minWidth, padding, fontSize);

			// Cache it
			CachedTooltipEntry newEntry;
			newEntry.layout = layout;
			newEntry.last_frame_used = frame_counter;

			newEntry.fields.reserve(scratch_fields_count);
			draw_fields.reserve(scratch_fields_count);

			for (size_t k = 0; k < scratch_fields_count; ++k) {
				const auto& src = scratch_fields[k];

				// Populate Cache Entry (Deep Copy)
				CachedFieldLine dst;
				dst.label = std::string(src.label);
				dst.value = std::string(src.value);
				dst.r = src.r;
				dst.g = src.g;
				dst.b = src.b;
				for (const auto& wl : src.wrappedLines) {
					dst.wrappedLines.emplace_back(wl);
				}
				newEntry.fields.push_back(std::move(dst));

				// Populate Draw Fields (from scratch, referencing scratch strings which are valid this frame)
				draw_fields.push_back(src);
			}

			layoutCache.emplace(hash, std::move(newEntry));
		}

		// Position tooltip above tile
		float tooltipX = screen_x - (layout.width / 2.0f);
		float tooltipY = screen_y - layout.height - 12.0f;

		// 3. Draw Background & Shadow
		drawBackground(vg, tooltipX, tooltipY, layout.width, layout.height, 4.0f, tooltip);

		// 4. Draw Text Fields
		drawFields(vg, draw_fields, tooltipX, tooltipY, layout.valueStartX, fontSize * 1.4f, padding, fontSize);

		// 5. Draw Container Grid
		drawContainerGrid(vg, draw_fields, tooltipX, tooltipY, tooltip, layout);
	}
}
