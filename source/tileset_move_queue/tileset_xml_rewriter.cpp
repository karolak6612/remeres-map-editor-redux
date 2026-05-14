#include "app/main.h"
#include "tileset_move_queue/tileset_xml_rewriter.h"

#include "ext/pugixml.hpp"
#include "util/file_system.h"

#include <algorithm>
#include <string_view>
#include <unordered_set>

namespace {
	struct ItemRange {
		uint16_t start = 0;
		uint16_t end = 0;
		pugi::xml_node node;
	};

	[[nodiscard]] bool supportsPalette(const std::string& section_name, PaletteType palette) {
		if (palette == TILESET_RAW) {
			return section_name == "raw"
				|| section_name == "terrain_and_raw"
				|| section_name == "collections_and_raw"
				|| section_name == "doodad_and_raw"
				|| section_name == "items_and_raw";
		}

		if (palette == TILESET_ITEM) {
			return section_name == "items" || section_name == "items_and_raw";
		}

		return false;
	}

	[[nodiscard]] std::string directSectionName(PaletteType palette) {
		return palette == TILESET_ITEM ? "items" : "raw";
	}

	[[nodiscard]] ItemRange readRange(const pugi::xml_node& node) {
		const auto id_attribute = node.attribute("id");
		if (id_attribute) {
			const uint16_t id = id_attribute.as_ushort();
			return ItemRange { .start = id, .end = id, .node = node };
		}

		const uint16_t from_id = node.attribute("fromid").as_ushort();
		const uint16_t to_id = std::max<uint16_t>(from_id, node.attribute("toid").as_ushort());
		return ItemRange { .start = from_id, .end = to_id, .node = node };
	}

	void assignNodeRange(pugi::xml_node node, uint16_t start, uint16_t end) {
		node.remove_attribute("id");
		node.remove_attribute("fromid");
		node.remove_attribute("toid");

		if (start == end) {
			node.append_attribute("id").set_value(start);
			return;
		}

		node.append_attribute("fromid").set_value(start);
		node.append_attribute("toid").set_value(end);
	}

	[[nodiscard]] pugi::xml_node findTilesetNode(pugi::xml_node root, const std::string& tileset_name) {
		for (pugi::xml_node tileset = root.child("tileset"); tileset; tileset = tileset.next_sibling("tileset")) {
			if (tileset.attribute("name").as_string() == tileset_name) {
				return tileset;
			}
		}
		return {};
	}

	[[nodiscard]] std::vector<pugi::xml_node> findSectionNodes(pugi::xml_node tileset_node, PaletteType palette) {
		std::vector<pugi::xml_node> nodes;
		for (pugi::xml_node child = tileset_node.first_child(); child; child = child.next_sibling()) {
			if (supportsPalette(child.name(), palette)) {
				nodes.push_back(child);
			}
		}
		return nodes;
	}

	[[nodiscard]] pugi::xml_node ensureTargetSection(pugi::xml_node tileset_node, PaletteType palette) {
		const std::string direct_name = directSectionName(palette);
		for (pugi::xml_node child = tileset_node.first_child(); child; child = child.next_sibling()) {
			if (direct_name == child.name()) {
				return child;
			}
		}

		pugi::xml_node section = tileset_node.append_child(direct_name.c_str());
		tileset_node.append_child(pugi::node_pcdata).set_value("\n\t");
		return section;
	}

	[[nodiscard]] bool sectionContainsItem(pugi::xml_node section, uint16_t item_id) {
		for (pugi::xml_node item = section.child("item"); item; item = item.next_sibling("item")) {
			const ItemRange range = readRange(item);
			if (range.start <= item_id && item_id <= range.end) {
				return true;
			}
		}
		return false;
	}

	bool removeItemFromSection(pugi::xml_node section, uint16_t item_id) {
		bool removed = false;
		for (pugi::xml_node item = section.child("item"); item;) {
			pugi::xml_node next = item.next_sibling("item");
			const ItemRange range = readRange(item);
			if (range.start <= item_id && item_id <= range.end) {
				removed = true;
				if (range.start == range.end) {
					section.remove_child(item);
				} else if (item_id == range.start) {
					assignNodeRange(item, static_cast<uint16_t>(range.start + 1), range.end);
				} else if (item_id == range.end) {
					assignNodeRange(item, range.start, static_cast<uint16_t>(range.end - 1));
				} else {
					assignNodeRange(item, range.start, static_cast<uint16_t>(item_id - 1));
					pugi::xml_node tail = section.insert_child_after("item", item);
					assignNodeRange(tail, static_cast<uint16_t>(item_id + 1), range.end);
				}
			}
			item = next;
		}
		return removed;
	}

	void insertItemSorted(pugi::xml_node section, uint16_t item_id) {
		for (pugi::xml_node item = section.child("item"); item; item = item.next_sibling("item")) {
			const ItemRange range = readRange(item);
			if (range.start <= item_id && item_id <= range.end) {
				return;
			}

			if (range.start > item_id) {
				pugi::xml_node inserted = section.insert_child_before("item", item);
				assignNodeRange(inserted, item_id, item_id);
				return;
			}
		}

		pugi::xml_node inserted = section.append_child("item");
		assignNodeRange(inserted, item_id, item_id);
	}
}

std::unordered_set<std::string> TilesetXmlRewriter::LoadXmlTilesetNames() {
	std::unordered_set<std::string> names;

	pugi::xml_document document;
	const std::string path = ResolveTilesetsPath();
	if (!document.load_file(path.c_str())) {
		return names;
	}

	pugi::xml_node materials = document.child("materials");
	if (!materials) {
		return names;
	}

	for (pugi::xml_node tileset = materials.child("tileset"); tileset; tileset = tileset.next_sibling("tileset")) {
		const auto name = tileset.attribute("name");
		if (!name) {
			continue;
		}

		names.emplace(name.as_string());
	}

	return names;
}

bool TilesetXmlRewriter::IsVirtualRuntimeTilesetName(std::string_view tileset_name) {
	return tileset_name == "Others" || tileset_name == "NPCs";
}

TilesetXmlRewriter::Result TilesetXmlRewriter::Apply(const std::vector<TilesetMoveQueue::Entry>& entries) const {
	Result result;

	pugi::xml_document document;
	const std::string path = ResolveTilesetsPath();
	const pugi::xml_parse_result parse_result = document.load_file(path.c_str());
	if (!parse_result) {
		result.error = "Could not open tilesets.xml.";
		return result;
	}

	pugi::xml_node materials = document.child("materials");
	if (!materials) {
		result.error = "Invalid materials root in tilesets.xml.";
		return result;
	}

	for (const auto& entry : entries) {
		pugi::xml_node source_tileset = findTilesetNode(materials, entry.source.tileset_name);
		pugi::xml_node target_tileset = findTilesetNode(materials, entry.target.tileset_name);

		if (!target_tileset) {
			result.warnings.push_back(
				"Skipped item " + std::to_string(entry.item_id)
				+ " because target tileset '" + entry.target.tileset_name + "' was not found in tilesets.xml."
			);
			++result.skipped;
			continue;
		}

		if (!source_tileset && !IsVirtualRuntimeTilesetName(entry.source.tileset_name)) {
			result.warnings.push_back(
				"Skipped item " + std::to_string(entry.item_id)
				+ " because source tileset '" + entry.source.tileset_name + "' was not found in tilesets.xml."
			);
			++result.skipped;
			continue;
		}

		if (source_tileset) {
			for (const pugi::xml_node& section : findSectionNodes(source_tileset, entry.source.palette)) {
				removeItemFromSection(section, entry.item_id);
			}
		}

		pugi::xml_node target_section = ensureTargetSection(target_tileset, entry.target.palette);
		if (sectionContainsItem(target_section, entry.item_id)) {
			result.warnings.push_back("Skipped item " + std::to_string(entry.item_id) + " because the target tileset already contains it.");
			++result.skipped;
			continue;
		}

		insertItemSorted(target_section, entry.item_id);
		++result.applied;
	}

	if (!document.save_file(path.c_str(), PUGIXML_TEXT("\t"))) {
		result.error = "Could not save tilesets.xml.";
		return result;
	}

	result.success = true;
	return result;
}

std::string TilesetXmlRewriter::ResolveTilesetsPath() {
	wxString base = FileSystem::GetFoundDataDirectory();
	if (base.empty()) {
		base = FileSystem::GetDataDirectory();
	}

	FileName file;
	file.Assign(base);
	file.AppendDir("1310");
	file.SetFullName("tilesets.xml");
	return nstr(file.GetFullPath());
}
