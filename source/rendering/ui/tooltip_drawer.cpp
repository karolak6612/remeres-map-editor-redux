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
#include "game/item.h"
#include "game/sprites.h"
#include "ui/gui.h"

// FNV-1a Hash Implementation for Caching
constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
constexpr uint64_t FNV_PRIME = 1099511628211ULL;

inline void hash_combine(uint64_t& seed, uint64_t value) {
	seed ^= value;
	seed *= FNV_PRIME;
}

inline void hash_combine(uint64_t& seed, std::string_view s) {
	for (char c : s) {
		seed ^= static_cast<uint8_t>(c);
		seed *= FNV_PRIME;
	}
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
	// Clear frame lists
	drawList.clear();
	legacy_tooltips.clear();
	legacy_active_count = 0;

	// Garbage collect cache occasionally (e.g., every 60 frames)
	// For simplicity, we check every frame but only act on old entries
	garbageCollect();
}

void TooltipDrawer::garbageCollect() {
	// Remove entries not rendered for 120 frames (2 seconds at 60fps)
	constexpr uint64_t CACHE_TTL = 120;
	if (current_frame < CACHE_TTL) return;

	uint64_t threshold = current_frame - CACHE_TTL;

	// Use erase_if (C++20) or standard loop
	for (auto it = layoutCache.begin(); it != layoutCache.end(); ) {
		if (it->second->last_rendered_frame < threshold) {
			it = layoutCache.erase(it);
		} else {
			++it;
		}
	}
}

TooltipData& TooltipDrawer::requestTooltipData() {
	if (legacy_active_count >= legacy_tooltips.size()) {
		legacy_tooltips.emplace_back();
	}
	TooltipData& data = legacy_tooltips[legacy_active_count];
	data.clear();
	return data;
}

void TooltipDrawer::commitTooltip() {
	legacy_active_count++;
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

void TooltipDrawer::generateTooltipData(TooltipData& data, Item* item, const Position& pos, bool isHouseTile, float zoom) {
	data.clear();

	if (!item) {
		return;
	}

	const uint16_t id = item->getID();
	if (id < 100) {
		return;
	}

	bool is_complex = item->isComplex();
	// Early exit for simple items
	if (!is_complex && !g_items[id].isTooltipable()) {
		return;
	}

	bool is_container = g_items[id].isContainer();
	bool is_door = isHouseTile && item->isDoor();
	bool is_teleport = item->isTeleport();

	if (is_complex) {
		data.uniqueId = item->getUniqueID();
		data.actionId = item->getActionID();
		data.text = item->getText();
		data.description = item->getDescription();
	}

	// Check if it's a door
	if (is_door) {
		if (const Door* door = item->asDoor()) {
			if (door->isRealDoor()) {
				data.doorId = door->getDoorID();
			}
		}
	}

	// Check if it's a teleport
	if (is_teleport) {
		if (const Teleport* tp = item->asTeleport()) {
			if (tp->hasDestination()) {
				data.destination = tp->getDestination();
			}
		}
	}

	// Check if container has content
	bool hasContent = false;
	if (is_container) {
		if (const Container* container = item->asContainer()) {
			hasContent = container->getItemCount() > 0;
		}
	}

	// Only create tooltip if there's something to show
	if (!data.hasVisibleFields() && !hasContent && data.text.empty() && data.description.empty()) {
		return; // Nothing to show
	}

	// Get item name from database
	std::string_view itemName = g_items[id].name;
	if (itemName.empty()) {
		itemName = "Item";
	}

	data.pos = pos;
	data.itemId = id;
	data.itemName = itemName;

	// Populate container items
	if (is_container && zoom <= 1.5f) {
		if (const Container* container = item->asContainer()) {
			// Set capacity for rendering empty slots
			data.containerCapacity = static_cast<uint8_t>(container->getVolume());

			const auto& items = container->getVector();
			data.containerItems.reserve(items.size());
			for (const auto& subItem : items) {
				if (subItem) {
					ContainerItem ci;
					ci.id = subItem->getID();
					ci.subtype = subItem->getSubtype();
					ci.count = subItem->getCount();
					// Sanity check for count
					if (ci.count == 0) {
						ci.count = 1;
					}

					data.containerItems.push_back(ci);

					// Limit preview items to avoid massive tooltips
					if (data.containerItems.size() >= 32) {
						break;
					}
				}
			}
		}
	}

	data.updateCategory();
}

uint64_t TooltipDrawer::computeDataHash(const TooltipData& data) const {
	uint64_t seed = FNV_OFFSET_BASIS;

	// Hash Scalars
	hash_combine(seed, static_cast<uint64_t>(data.itemId));
	hash_combine(seed, static_cast<uint64_t>(data.actionId));
	hash_combine(seed, static_cast<uint64_t>(data.uniqueId));
	hash_combine(seed, static_cast<uint64_t>(data.doorId));
	hash_combine(seed, static_cast<uint64_t>(data.destination.x));
	hash_combine(seed, static_cast<uint64_t>(data.destination.y));
	hash_combine(seed, static_cast<uint64_t>(data.destination.z));
	hash_combine(seed, static_cast<uint64_t>(data.containerCapacity));

	// Hash Strings
	hash_combine(seed, data.itemName);
	hash_combine(seed, data.text);
	hash_combine(seed, data.description);
	hash_combine(seed, data.waypointName);

	// Hash Container Contents
	for (const auto& item : data.containerItems) {
		hash_combine(seed, static_cast<uint64_t>(item.id));
		hash_combine(seed, static_cast<uint64_t>(item.count));
		hash_combine(seed, static_cast<uint64_t>(item.subtype));
	}

	return seed;
}

void TooltipDrawer::addItemTooltip(Item* item, const Position& pos, bool isHouseTile, float zoom) {
	if (!item) return;

	TooltipData data;
	generateTooltipData(data, item, pos, isHouseTile, zoom);

	if (!data.hasVisibleFields()) {
		return;
	}

	uint64_t hash = computeDataHash(data);

	// Check cache
	auto it = layoutCache.find(hash);
	CachedTooltipEntry* entry = nullptr;

	if (it != layoutCache.end()) {
		entry = it->second.get();
	} else {
		// Cache miss - compute layout
		auto newEntry = std::make_unique<CachedTooltipEntry>();
		std::string scratch;
		prepareFields(data, newEntry->fields, scratch); // Populate newEntry->fields with owning strings
		newEntry->layout = calculateLayout(nullptr, data, newEntry->fields, 220.0f, 120.0f, 10.0f, 11.0f); // Context null for measurement if possible? No, we need context.
		// Wait, calculateLayout needs VG context for measurement.
		// If we don't have VG context here, we can't measure.
		// We are in addItemTooltip which is called during Update loop, potentially before Draw?
		// No, DrawTile is called inside MapDrawer::Draw, which has no VG context passed to it?
		// MapDrawer::Draw calls DrawMap -> DrawMapLayer -> DrawTile.
		// The VG context is usually available in DrawTooltips, which is a separate pass.

		// CRITICAL: We cannot measure text in addItemTooltip because we don't have the NVGcontext!
		// But we can store the data and defer measurement to draw().
		// And cache the result *after* drawing.

		// So: addItemTooltip adds to drawList.
		// draw() checks if cache entry exists and is valid (has layout).
		// If not, it computes it using the VG context available in draw().

		// So here we just create the entry if missing, but it will be empty.
		layoutCache[hash] = std::move(newEntry);
		entry = layoutCache[hash].get();
	}

	entry->last_rendered_frame = current_frame;

	RenderableTooltip rt;
	rt.data = std::move(data);
	rt.cacheEntry = entry;
	drawList.push_back(std::move(rt));
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

void TooltipDrawer::prepareFields(const TooltipData& tooltip, std::vector<FieldLine>& out_fields, std::string& scratch_buffer) {
	// Build content lines with word wrapping support
	using namespace TooltipColors;
	out_fields.clear();
	scratch_buffer.clear();
	if (scratch_buffer.capacity() < 4096) {
		scratch_buffer.reserve(4096);
	}

	auto addField = [&](std::string_view label, std::string_view value, uint8_t r, uint8_t g, uint8_t b) {
		FieldLine field;
		field.label = std::string(label);
		field.value = std::string(value);
		field.r = r;
		field.g = g;
		field.b = b;
		out_fields.push_back(std::move(field));
	};

	if (tooltip.category == TooltipCategory::WAYPOINT) {
		addField("Waypoint", tooltip.waypointName, WAYPOINT_HEADER_R, WAYPOINT_HEADER_G, WAYPOINT_HEADER_B);
	} else {
		if (tooltip.actionId > 0) {
			size_t start = scratch_buffer.size();
			std::format_to(std::back_inserter(scratch_buffer), "{}", tooltip.actionId);
			addField("Action ID", std::string_view(scratch_buffer.data() + start, scratch_buffer.size() - start), ACTION_ID_R, ACTION_ID_G, ACTION_ID_B);
		}
		if (tooltip.uniqueId > 0) {
			size_t start = scratch_buffer.size();
			std::format_to(std::back_inserter(scratch_buffer), "{}", tooltip.uniqueId);
			addField("Unique ID", std::string_view(scratch_buffer.data() + start, scratch_buffer.size() - start), UNIQUE_ID_R, UNIQUE_ID_G, UNIQUE_ID_B);
		}
		if (tooltip.doorId > 0) {
			size_t start = scratch_buffer.size();
			std::format_to(std::back_inserter(scratch_buffer), "{}", tooltip.doorId);
			addField("Door ID", std::string_view(scratch_buffer.data() + start, scratch_buffer.size() - start), DOOR_ID_R, DOOR_ID_G, DOOR_ID_B);
		}
		if (tooltip.destination.x > 0) {
			size_t start = scratch_buffer.size();
			std::format_to(std::back_inserter(scratch_buffer), "{}, {}, {}", tooltip.destination.x, tooltip.destination.y, tooltip.destination.z);
			addField("Destination", std::string_view(scratch_buffer.data() + start, scratch_buffer.size() - start), TELEPORT_DEST_R, TELEPORT_DEST_G, TELEPORT_DEST_B);
		}
		if (!tooltip.description.empty()) {
			addField("Description", tooltip.description, BODY_TEXT_R, BODY_TEXT_G, BODY_TEXT_B);
		}
		if (!tooltip.text.empty()) {
			size_t start = scratch_buffer.size();
			std::format_to(std::back_inserter(scratch_buffer), "\"{}\"", tooltip.text);
			addField("Text", std::string_view(scratch_buffer.data() + start, scratch_buffer.size() - start), TEXT_R, TEXT_G, TEXT_B);
		}
	}
}

TooltipDrawer::LayoutMetrics TooltipDrawer::calculateLayout(NVGcontext* vg, const TooltipData& tooltip, const std::vector<FieldLine>& fields, float maxWidth, float minWidth, float padding, float fontSize) {
	LayoutMetrics lm = {};
	if (!vg) return lm;

	// Set up font for measurements
	nvgFontSize(vg, fontSize);
	nvgFontFace(vg, "sans");

	// Measure label widths
	float maxLabelWidth = 0.0f;
	for (const auto& field : fields) {
		float labelBounds[4];
		nvgTextBounds(vg, 0, 0, field.label.c_str(), nullptr, labelBounds);
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

	// We can't modify const fields, so we need to store wrap results in fields
	// But calculateLayout is called on CachedTooltipEntry creation time or update time.
	// The fields passed in are likely from CachedTooltipEntry which is mutable in the caller.
	// But here it is const reference.
	// Actually, we should allow modifying fields to store wrapped lines.
	// So calculateLayout should take non-const reference or we assume wrappedLines are already cleared?
	// But `prepareFields` clears wrappedLines? No, `prepareFields` creates `FieldLine`s.
	// `calculateLayout` populates `wrappedLines`.

	// Let's cast away constness or change signature. Changing signature is better.
	// But I declared it const in header? Let's check header.
	// `LayoutMetrics calculateLayout(NVGcontext* vg, const TooltipData& tooltip, const std::vector<FieldLine>& fields, ...)`
	// Yes, const.
	// This means `calculateLayout` should NOT modify fields.
	// But `wrappedLines` are part of `FieldLine`.
	// So `calculateLayout` should populate them.
	// I will use `const_cast` or update header in next step if verification fails.
	// Actually, `wrappedLines` are part of `FieldLine`, so `FieldLine` must be mutable.

	// I'll assume `std::vector<FieldLine>& fields` (non-const) in implementation and update header if needed.
	// But I already wrote the header with `const`.
	// I'll use `const_cast<std::vector<FieldLine>&>(fields)` for now to avoid re-writing header immediately,
	// or I can return the wrapped lines structure? No, simpler to modify in place.
	// It's a "calculate layout" phase, so populating `wrappedLines` is appropriate.

	std::vector<FieldLine>& mutable_fields = const_cast<std::vector<FieldLine>&>(fields);

	for (auto& field : mutable_fields) {
		field.wrappedLines.clear();
		const char* start = field.value.c_str();
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

void TooltipDrawer::drawFields(NVGcontext* vg, float x, float y, float valueStartX, const std::vector<FieldLine>& fields, float lineHeight, float padding, float fontSize) {
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
				nvgText(vg, contentX, cursorY, field.label.c_str(), nullptr);
				firstLine = false;
			}

			// Draw value line in semantic color
			nvgFillColor(vg, nvgRGBA(field.r, field.g, field.b, 255));
			nvgText(vg, contentX + valueStartX, cursorY, line.c_str(), nullptr);

			cursorY += lineHeight;
		}
	}
}

void TooltipDrawer::drawContainerGrid(NVGcontext* vg, float x, float y, const TooltipData& tooltip, const LayoutMetrics& layout) {
	using namespace TooltipColors;

	if (layout.totalContainerSlots <= 0) {
		return;
	}

	// Calculate cursorY after text fields
	// We need to re-calculate text height or pass it, but simpler to deduce from logic:
	// The grid is at the bottom. We can just use the bottom of the box minus grid height minus padding.
	// But let's calculate exact start Y based on text content for precision

	// Issue: layout does not store fields line count explicitly, but we have fields in draw()
	// No, we don't have fields in this function arg.
	// But we passed layout.
	// We can deduce text block height from layout.height - containerHeight - padding*2
	// layout.height = totalLines * lineHeight + padding * 2 + containerHeight + 4.0f
	// So textBlockHeight = layout.height - (containerHeight + 4.0f) - padding * 2

	float fontSize = 11.0f; // Assuming constant
	// float lineHeight = fontSize * 1.4f;

	float textBlockHeight = layout.height - (layout.containerHeight + 4.0f) - 20.0f; // 20 = padding*2

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

	current_frame++; // Increment frame counter

	// Draw cached tooltips (from drawList)
	for (const auto& rt : drawList) {
		const auto& tooltip = rt.data;
		CachedTooltipEntry* entry = rt.cacheEntry;

		int unscaled_x, unscaled_y;
		view.getScreenPosition(tooltip.pos.x, tooltip.pos.y, tooltip.pos.z, unscaled_x, unscaled_y);

		float zoom = view.zoom < 0.01f ? 1.0f : view.zoom;

		float screen_x = unscaled_x / zoom;
		float screen_y = unscaled_y / zoom;
		float tile_size_screen = 32.0f / zoom;

		// Center on tile
		screen_x += tile_size_screen / 2.0f;
		screen_y += tile_size_screen / 2.0f;

		// If entry has no layout (was created without VG context), compute it now
		if (entry && entry->layout.width == 0.0f) {
			// Constants
			float fontSize = 11.0f;
			float padding = 10.0f;
			float minWidth = 120.0f;
			float maxWidth = 220.0f;

			entry->layout = calculateLayout(vg, tooltip, entry->fields, maxWidth, minWidth, padding, fontSize);
		}

		const auto& layout = entry->layout;
		const auto& fields = entry->fields;

		// Position tooltip above tile
		float tooltipX = screen_x - (layout.width / 2.0f);
		float tooltipY = screen_y - layout.height - 12.0f;

		// Draw
		drawBackground(vg, tooltipX, tooltipY, layout.width, layout.height, 4.0f, tooltip);
		drawFields(vg, tooltipX, tooltipY, layout.valueStartX, fields, 11.0f * 1.4f, 10.0f, 11.0f);
		drawContainerGrid(vg, tooltipX, tooltipY, tooltip, layout);
	}

	// Draw legacy tooltips (rebuild every frame)
	for (size_t i = 0; i < legacy_active_count; ++i) {
		const auto& tooltip = legacy_tooltips[i];
		int unscaled_x, unscaled_y;
		view.getScreenPosition(tooltip.pos.x, tooltip.pos.y, tooltip.pos.z, unscaled_x, unscaled_y);

		float zoom = view.zoom < 0.01f ? 1.0f : view.zoom;

		float screen_x = unscaled_x / zoom;
		float screen_y = unscaled_y / zoom;
		float tile_size_screen = 32.0f / zoom;

		// Center on tile
		screen_x += tile_size_screen / 2.0f;
		screen_y += tile_size_screen / 2.0f;

		float fontSize = 11.0f;
		float padding = 10.0f;
		float minWidth = 120.0f;
		float maxWidth = 220.0f;

		// Re-use scratch fields
		std::vector<FieldLine> tempFields;
		std::string tempStorage;
		prepareFields(tooltip, tempFields, tempStorage);

		// Skip if nothing to show
		if (tempFields.empty() && tooltip.containerItems.empty()) {
			continue;
		}

		// Calculate Layout
		LayoutMetrics layout = calculateLayout(vg, tooltip, tempFields, maxWidth, minWidth, padding, fontSize);

		// Position tooltip above tile
		float tooltipX = screen_x - (layout.width / 2.0f);
		float tooltipY = screen_y - layout.height - 12.0f;

		// Draw
		drawBackground(vg, tooltipX, tooltipY, layout.width, layout.height, 4.0f, tooltip);
		drawFields(vg, tooltipX, tooltipY, layout.valueStartX, tempFields, fontSize * 1.4f, padding, fontSize);
		drawContainerGrid(vg, tooltipX, tooltipY, tooltip, layout);
	}
}
