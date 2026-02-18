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
#include "game/item.h"
#include "game/complexitem.h"
#include "ui/gui.h"

// Helper for hashing
static inline void hash_combine(uint64_t& seed, uint64_t value) {
	seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

TooltipDrawer::CachedTooltipData::CachedTooltipData(const TooltipData& other) :
	category(other.category),
	itemId(other.itemId),
	itemName(other.itemName),
	actionId(other.actionId),
	uniqueId(other.uniqueId),
	doorId(other.doorId),
	text(other.text),
	description(other.description),
	destination(other.destination),
	waypointName(other.waypointName),
	containerItems(other.containerItems),
	containerCapacity(other.containerCapacity) {
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
	active_tooltips.clear();
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

uint64_t TooltipDrawer::computeHash(const Item* item, bool isHouseTile, float zoom) {
	if (!item) {
		return 0;
	}

	uint64_t hash = 0;
	uint16_t id = item->getID();
	hash_combine(hash, id);

	if (isHouseTile) {
		hash_combine(hash, 1);
	}

	// Zoom level affects container visibility (threshold 1.5)
	if (zoom <= 1.5f) {
		hash_combine(hash, 1);
	} else {
		hash_combine(hash, 0);
	}

	if (item->isComplex()) {
		hash_combine(hash, item->getActionID());
		hash_combine(hash, item->getUniqueID());

		// Text/Desc hashing
		std::string_view text = item->getText();
		for (char c : text) {
			hash_combine(hash, c);
		}

		std::string_view desc = item->getDescription();
		for (char c : desc) {
			hash_combine(hash, c);
		}

		// Door hashing
		if (isHouseTile && item->isDoor()) {
			if (const Door* door = item->asDoor()) {
				if (door->isRealDoor()) {
					hash_combine(hash, door->getDoorID());
				}
			}
		}

		// Teleport hashing
		if (item->isTeleport()) {
			Teleport* tp = static_cast<Teleport*>(const_cast<Item*>(item));
			if (tp->hasDestination()) {
				const Position& dest = tp->getDestination();
				hash_combine(hash, dest.x);
				hash_combine(hash, dest.y);
				hash_combine(hash, dest.z);
			}
		}

		// Container hashing
		if (zoom <= 1.5f && g_items[id].isContainer()) {
			if (const Container* container = item->asContainer()) {
				const auto& items = container->getVector();
				size_t count = 0;
				for (const auto& subItem : items) {
					if (subItem) {
						hash_combine(hash, subItem->getID());
						hash_combine(hash, subItem->getSubtype());
						hash_combine(hash, subItem->getCount());
						count++;
						if (count >= 32) {
							break;
						}
					}
				}
				hash_combine(hash, container->getVolume());
			}
		}
	}

	return hash;
}

bool TooltipDrawer::hasTooltip(uint64_t hash) const {
	return cache.find(hash) != cache.end();
}

void TooltipDrawer::addActiveTooltip(uint64_t hash, const Position& pos) {
	active_tooltips.push_back({ hash, pos });
}

void TooltipDrawer::defineTooltip(uint64_t hash, const TooltipData& data) {
	if (hasTooltip(hash)) {
		// Move to front of LRU
		lru_list.remove(hash);
		lru_list.push_front(hash);
		return;
	}

	// Create new entry
	CachedTooltip entry;
	entry.data = CachedTooltipData(data); // Deep copy strings

	// Pre-calculate fields
	prepareCachedFields(entry);

	// Layout will be calculated on first draw since we need NVGcontext
	// Initialize with zero size to signal recalculation needed
	entry.layout.width = 0;

	// Cache management
	if (cache.size() >= MAX_CACHE_SIZE) {
		uint64_t victim = lru_list.back();
		lru_list.pop_back();
		cache.erase(victim);
	}

	cache[hash] = std::move(entry);
	lru_list.push_front(hash);
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

void TooltipDrawer::prepareFields(const TooltipData& tooltip) {
	// Legacy path for non-cached tooltips (e.g. Waypoints if not hashed)
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

void TooltipDrawer::prepareCachedFields(CachedTooltip& cached) {
	using namespace TooltipColors;
	const auto& data = cached.data;
	cached.fields.clear();

	auto addField = [&](std::string label, std::string value, uint8_t r, uint8_t g, uint8_t b) {
		CachedFieldLine field;
		field.label = std::move(label);
		field.value = std::move(value);
		field.r = r;
		field.g = g;
		field.b = b;
		cached.fields.push_back(std::move(field));
	};

	if (data.category == TooltipCategory::WAYPOINT) {
		addField("Waypoint", data.waypointName, WAYPOINT_HEADER_R, WAYPOINT_HEADER_G, WAYPOINT_HEADER_B);
	} else {
		if (data.actionId > 0) {
			addField("Action ID", std::to_string(data.actionId), ACTION_ID_R, ACTION_ID_G, ACTION_ID_B);
		}
		if (data.uniqueId > 0) {
			addField("Unique ID", std::to_string(data.uniqueId), UNIQUE_ID_R, UNIQUE_ID_G, UNIQUE_ID_B);
		}
		if (data.doorId > 0) {
			addField("Door ID", std::to_string(data.doorId), DOOR_ID_R, DOOR_ID_G, DOOR_ID_B);
		}
		if (data.destination.x > 0) {
			std::string dest = std::format("{}, {}, {}", data.destination.x, data.destination.y, data.destination.z);
			addField("Destination", std::move(dest), TELEPORT_DEST_R, TELEPORT_DEST_G, TELEPORT_DEST_B);
		}
		if (!data.description.empty()) {
			addField("Description", data.description, BODY_TEXT_R, BODY_TEXT_G, BODY_TEXT_B);
		}
		if (!data.text.empty()) {
			addField("Text", "\"" + data.text + "\"", TEXT_R, TEXT_G, TEXT_B);
		}
	}
}

TooltipDrawer::LayoutMetrics TooltipDrawer::calculateLayout(NVGcontext* vg, const TooltipData& tooltip, float maxWidth, float minWidth, float padding, float fontSize) {
	// Legacy layout calculation using scratch_fields
	LayoutMetrics lm = {};
	nvgFontSize(vg, fontSize);
	nvgFontFace(vg, "sans");

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

	lm.valueStartX = maxLabelWidth + 12.0f;
	float maxValueWidth = maxWidth - lm.valueStartX - padding * 2;
	int totalLines = 0;
	float actualMaxWidth = minWidth;

	for (size_t i = 0; i < scratch_fields_count; ++i) {
		auto& field = scratch_fields[i];
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

TooltipDrawer::LayoutMetrics TooltipDrawer::calculateCachedLayout(NVGcontext* vg, const CachedTooltip& cached, float maxWidth, float minWidth, float padding, float fontSize) {
	// Similar to calculateLayout but uses cached fields and updates wrappedLines in-place
	LayoutMetrics lm = {};
	nvgFontSize(vg, fontSize);
	nvgFontFace(vg, "sans");

	float maxLabelWidth = 0.0f;
	for (const auto& field : cached.fields) {
		float labelBounds[4];
		nvgTextBounds(vg, 0, 0, field.label.c_str(), nullptr, labelBounds);
		float lw = labelBounds[2] - labelBounds[0];
		if (lw > maxLabelWidth) {
			maxLabelWidth = lw;
		}
	}

	lm.valueStartX = maxLabelWidth + 12.0f;
	float maxValueWidth = maxWidth - lm.valueStartX - padding * 2;
	int totalLines = 0;
	float actualMaxWidth = minWidth;

	// Mutable access needed to update wrappedLines
	CachedTooltip& mutableCached = const_cast<CachedTooltip&>(cached);

	for (auto& field : mutableCached.fields) {
		field.wrappedLines.clear();
		const char* start = field.value.c_str();
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
				std::string line(rows[j].start, rows[j].end - rows[j].start);
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

	// Container metrics
	lm.gridSlotSize = 34.0f;
	lm.numContainerItems = static_cast<int>(cached.data.containerItems.size());
	int capacity = static_cast<int>(cached.data.containerCapacity);
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

void TooltipDrawer::drawBackground(NVGcontext* vg, float x, float y, float width, float height, float cornerRadius, const CachedTooltipData& tooltip) {
	// Re-use existing logic but with CachedTooltipData type
	// Ideally we'd template this or extract category
	using namespace TooltipColors;
	uint8_t borderR, borderG, borderB;
	getHeaderColor(tooltip.category, borderR, borderG, borderB);

	for (int i = 3; i >= 0; i--) {
		float alpha = 35.0f + (3 - i) * 20.0f;
		float spread = i * 2.0f;
		float offsetY = 3.0f + i * 1.0f;
		nvgBeginPath(vg);
		nvgRoundedRect(vg, x - spread, y + offsetY - spread, width + spread * 2, height + spread * 2, cornerRadius + spread);
		nvgFillColor(vg, nvgRGBA(0, 0, 0, static_cast<unsigned char>(alpha)));
		nvgFill(vg);
	}

	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, width, height, cornerRadius);
	nvgFillColor(vg, nvgRGBA(BODY_BG_R, BODY_BG_G, BODY_BG_B, 250));
	nvgFill(vg);

	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, width, height, cornerRadius);
	nvgStrokeColor(vg, nvgRGBA(borderR, borderG, borderB, 255));
	nvgStrokeWidth(vg, 1.0f);
	nvgStroke(vg);
}

void TooltipDrawer::drawCachedFields(NVGcontext* vg, float x, float y, const CachedTooltip& cached, float lineHeight, float padding, float fontSize) {
	using namespace TooltipColors;
	float contentX = x + padding;
	float cursorY = y + padding;
	float valueStartX = cached.layout.valueStartX;

	nvgFontSize(vg, fontSize);
	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

	for (const auto& field : cached.fields) {
		bool firstLine = true;
		for (const auto& line : field.wrappedLines) {
			if (firstLine) {
				nvgFillColor(vg, nvgRGBA(BODY_TEXT_R, BODY_TEXT_G, BODY_TEXT_B, 160));
				nvgText(vg, contentX, cursorY, field.label.c_str(), nullptr);
				firstLine = false;
			}
			nvgFillColor(vg, nvgRGBA(field.r, field.g, field.b, 255));
			nvgText(vg, contentX + valueStartX, cursorY, line.c_str(), nullptr);
			cursorY += lineHeight;
		}
	}
}

// Redefined to take full cached object
void TooltipDrawer::drawContainerGrid(NVGcontext* vg, float x, float y, const CachedTooltip& cached) {
	using namespace TooltipColors;
	const auto& layout = cached.layout;
	const auto& tooltip = cached.data;

	if (layout.totalContainerSlots <= 0) {
		return;
	}

	float fontSize = 11.0f;
	float lineHeight = fontSize * 1.4f;
	float textBlockHeight = 0.0f;
	for (const auto& field : cached.fields) {
		textBlockHeight += field.wrappedLines.size() * lineHeight;
	}

	float startY = y + 10.0f + textBlockHeight + 8.0f;
	float startX = x + 10.0f;

	for (int idx = 0; idx < layout.totalContainerSlots; ++idx) {
		int col = idx % layout.containerCols;
		int row = idx / layout.containerCols;

		float itemX = startX + col * layout.gridSlotSize;
		float itemY = startY + row * layout.gridSlotSize;

		nvgBeginPath(vg);
		nvgRect(vg, itemX, itemY, 32, 32);
		nvgFillColor(vg, nvgRGBA(60, 60, 60, 100));
		nvgStrokeColor(vg, nvgRGBA(100, 100, 100, 100));
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
			nvgFillColor(vg, nvgRGBA(COUNT_TEXT_R, COUNT_TEXT_G, COUNT_TEXT_B, 255));
			nvgText(vg, itemX + 16, itemY + 16, summary.c_str(), nullptr);
		} else if (idx < layout.numContainerItems) {
			const auto& item = tooltip.containerItems[idx];
			int img = getSpriteImage(vg, item.id);
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

	// Constants
	float fontSize = 11.0f;
	float padding = 10.0f;
	float minWidth = 120.0f;
	float maxWidth = 220.0f;

	// 1. Draw Active Cached Tooltips
	for (const auto& active : active_tooltips) {
		auto it = cache.find(active.hash);
		if (it == cache.end()) {
			continue;
		}

		CachedTooltip& cached = it->second;

		// Calculate layout if needed (first run or context change)
		if (cached.layout.width == 0) {
			cached.layout = calculateCachedLayout(vg, cached, maxWidth, minWidth, padding, fontSize);
		}

		int unscaled_x, unscaled_y;
		view.getScreenPosition(active.pos.x, active.pos.y, active.pos.z, unscaled_x, unscaled_y);
		float zoom = view.zoom < 0.01f ? 1.0f : view.zoom;
		float screen_x = unscaled_x / zoom;
		float screen_y = unscaled_y / zoom;
		float tile_size_screen = 32.0f / zoom;

		screen_x += tile_size_screen / 2.0f;
		screen_y += tile_size_screen / 2.0f;

		float tooltipX = screen_x - (cached.layout.width / 2.0f);
		float tooltipY = screen_y - cached.layout.height - 12.0f;

		drawBackground(vg, tooltipX, tooltipY, cached.layout.width, cached.layout.height, 4.0f, cached.data);
		drawCachedFields(vg, tooltipX, tooltipY, cached, fontSize * 1.4f, padding, fontSize);
		drawContainerGrid(vg, tooltipX, tooltipY, cached);
	}

	// 2. Draw Legacy Tooltips (if any)
	for (size_t i = 0; i < active_count; ++i) {
		const auto& tooltip = tooltips[i];
		int unscaled_x, unscaled_y;
		view.getScreenPosition(tooltip.pos.x, tooltip.pos.y, tooltip.pos.z, unscaled_x, unscaled_y);
		float zoom = view.zoom < 0.01f ? 1.0f : view.zoom;
		float screen_x = unscaled_x / zoom;
		float screen_y = unscaled_y / zoom;
		float tile_size_screen = 32.0f / zoom;
		screen_x += tile_size_screen / 2.0f;
		screen_y += tile_size_screen / 2.0f;

		prepareFields(tooltip);
		if (scratch_fields_count == 0 && tooltip.containerItems.empty()) {
			continue;
		}

		LayoutMetrics layout = calculateLayout(vg, tooltip, maxWidth, minWidth, padding, fontSize);
		float tooltipX = screen_x - (layout.width / 2.0f);
		float tooltipY = screen_y - layout.height - 12.0f;

		drawBackground(vg, tooltipX, tooltipY, layout.width, layout.height, 4.0f, tooltip);
		drawFields(vg, tooltipX, tooltipY, layout.valueStartX, fontSize * 1.4f, padding, fontSize);
		// Legacy drawContainerGrid needs the old signature with TooltipData
		// We can define it inline or add back the method, but let's overload
		// Actually I defined overload in header but implementation was missing above.
		// Let's just implement the legacy one locally here or duplicated.
		// For brevity, I'll implement legacy drawContainerGrid here

		auto legacyDrawGrid = [&](float x, float y, const TooltipData& t, const LayoutMetrics& l) {
			using namespace TooltipColors;
			if (l.totalContainerSlots <= 0) return;

			float textBlockHeight = 0.0f;
			float lh = fontSize * 1.4f;
			for (size_t k = 0; k < scratch_fields_count; ++k) {
				textBlockHeight += scratch_fields[k].wrappedLines.size() * lh;
			}
			float startY = y + 10.0f + textBlockHeight + 8.0f;
			float startX = x + 10.0f;

			for (int idx = 0; idx < l.totalContainerSlots; ++idx) {
				int col = idx % l.containerCols;
				int row = idx / l.containerCols;
				float itemX = startX + col * l.gridSlotSize;
				float itemY = startY + row * l.gridSlotSize;

				nvgBeginPath(vg);
				nvgRect(vg, itemX, itemY, 32, 32);
				nvgFillColor(vg, nvgRGBA(60, 60, 60, 100));
				nvgStrokeColor(vg, nvgRGBA(100, 100, 100, 100));
				nvgStrokeWidth(vg, 1.0f);
				nvgFill(vg);
				nvgStroke(vg);

				if (l.emptyContainerSlots > 0 && idx == l.totalContainerSlots - 1) {
					std::string s = "+" + std::to_string(l.emptyContainerSlots);
					nvgFontSize(vg, 12.0f);
					nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
					nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
					nvgText(vg, itemX + 17, itemY + 17, s.c_str(), nullptr);
					nvgFillColor(vg, nvgRGBA(COUNT_TEXT_R, COUNT_TEXT_G, COUNT_TEXT_B, 255));
					nvgText(vg, itemX + 16, itemY + 16, s.c_str(), nullptr);
				} else if (idx < l.numContainerItems) {
					const auto& item = t.containerItems[idx];
					int img = getSpriteImage(vg, item.id);
					if (img > 0) {
						nvgBeginPath(vg);
						nvgRect(vg, itemX, itemY, 32, 32);
						nvgFillPaint(vg, nvgImagePattern(vg, itemX, itemY, 32, 32, 0, img, 1.0f));
						nvgFill(vg);
					}
					if (item.count > 1) {
						std::string c = std::to_string(item.count);
						nvgFontSize(vg, 10.0f);
						nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);
						nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
						nvgText(vg, itemX + 33, itemY + 33, c.c_str(), nullptr);
						nvgFillColor(vg, nvgRGBA(COUNT_TEXT_R, COUNT_TEXT_G, COUNT_TEXT_B, 255));
						nvgText(vg, itemX + 32, itemY + 32, c.c_str(), nullptr);
					}
				}
			}
		};

		legacyDrawGrid(tooltipX, tooltipY, tooltip, layout);
	}
}
