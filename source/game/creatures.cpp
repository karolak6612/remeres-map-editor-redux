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

#include "app/main.h"

#include "ui/gui.h"
#include "game/materials.h"
#include "brushes/brush.h"
#include "game/creatures.h"
#include "brushes/creature/creature_brush.h"
#include "io/xml_file_loader.h"

#include <wx/dir.h>

#include <algorithm>
#include <cctype>
#include <format>
#include <fstream>
#include <optional>

CreatureDatabase g_creatures;

CreatureType::CreatureType() :
	isNpc(false),
	missing(false),
	in_other_tileset(false),
	standard(false),
	name(""),
	brush(nullptr) {
	////
}

CreatureType::CreatureType(const CreatureType& ct) :
	isNpc(ct.isNpc),
	missing(ct.missing),
	in_other_tileset(ct.in_other_tileset),
	standard(ct.standard),
	name(ct.name),
	outfit(ct.outfit),
	brush(ct.brush) {
	////
}

CreatureType& CreatureType::operator=(const CreatureType& ct) {
	isNpc = ct.isNpc;
	missing = ct.missing;
	in_other_tileset = ct.in_other_tileset;
	standard = ct.standard;
	name = ct.name;
	outfit = ct.outfit;
	brush = ct.brush;
	return *this;
}

CreatureType::~CreatureType() {
	////
}

void CreatureType::preserve_assign_creature_fields(CreatureType* dest, const CreatureType& src) {
	CreatureBrush* oldBrush = dest->brush;
	bool oldInTileset = dest->in_other_tileset;
	bool oldStandard = dest->standard;

	*dest = src;

	dest->brush = oldBrush;
	dest->in_other_tileset = oldInTileset;
	dest->standard = oldStandard;
}

CreatureType* CreatureType::loadFromXML(pugi::xml_node node, std::vector<std::string>& warnings) {
	pugi::xml_attribute attribute;
	if (!(attribute = node.attribute("type"))) {
		warnings.push_back("Couldn't read type tag of creature node.");
		return nullptr;
	}

	const std::string& tmpType = attribute.as_string();
	if (tmpType != "monster" && tmpType != "npc") {
		warnings.push_back((wxString("Invalid type tag of creature node \"") + wxstr(tmpType) + "\"").ToStdString());
		return nullptr;
	}

	if (!(attribute = node.attribute("name"))) {
		warnings.push_back("Couldn't read name tag of creature node.");
		return nullptr;
	}

	CreatureType* ct = newd CreatureType();
	ct->name = attribute.as_string();
	ct->isNpc = tmpType == "npc";

	if ((attribute = node.attribute("looktype"))) {
		ct->outfit.lookType = attribute.as_int();

		if (g_gui.gfx.getCreatureSprite(ct->outfit.lookType) == nullptr) {
			warnings.push_back((wxString("Invalid creature \"") + wxstr(ct->name) + "\" look type #" + std::to_string(ct->outfit.lookType)).ToStdString());
		}
	} else {
		// Log if no looktype is present (defaulting to 0)
	}

	if ((attribute = node.attribute("lookitem"))) {
		ct->outfit.lookItem = attribute.as_int();
	}

	if ((attribute = node.attribute("lookmount"))) {
		ct->outfit.lookMount = attribute.as_int();
	}

	if ((attribute = node.attribute("lookaddon"))) {
		ct->outfit.lookAddon = attribute.as_int();
	}

	if ((attribute = node.attribute("lookhead"))) {
		ct->outfit.lookHead = attribute.as_int();
	}

	if ((attribute = node.attribute("lookbody"))) {
		ct->outfit.lookBody = attribute.as_int();
	}

	if ((attribute = node.attribute("looklegs"))) {
		ct->outfit.lookLegs = attribute.as_int();
	}

	if ((attribute = node.attribute("lookfeet"))) {
		ct->outfit.lookFeet = attribute.as_int();
	}

	if ((attribute = node.attribute("lookmounthead"))) {
		ct->outfit.lookMountHead = attribute.as_int();
	}

	if ((attribute = node.attribute("lookmountbody"))) {
		ct->outfit.lookMountBody = attribute.as_int();
	}

	if ((attribute = node.attribute("lookmountlegs"))) {
		ct->outfit.lookMountLegs = attribute.as_int();
	}

	if ((attribute = node.attribute("lookmountfeet"))) {
		ct->outfit.lookMountFeet = attribute.as_int();
	}

	return ct;
}

CreatureType* CreatureType::loadFromOTXML(const FileName& filename, pugi::xml_document& doc, std::vector<std::string>& warnings) {
	ASSERT(doc != nullptr);

	bool isNpc;
	pugi::xml_node node;
	if ((node = doc.child("monster"))) {
		isNpc = false;
	} else if ((node = doc.child("npc"))) {
		isNpc = true;
	} else {
		warnings.push_back("This file is not a monster/npc file");
		return nullptr;
	}

	pugi::xml_attribute attribute;
	if (!(attribute = node.attribute("name"))) {
		warnings.push_back("Couldn't read name tag of creature node.");
		return nullptr;
	}

	CreatureType* ct = newd CreatureType();
	if (isNpc) {
		ct->name = nstr(filename.GetName());
	} else {
		ct->name = attribute.as_string();
	}
	ct->isNpc = isNpc;

	for (pugi::xml_node optionNode = node.first_child(); optionNode; optionNode = optionNode.next_sibling()) {
		if (as_lower_str(optionNode.name()) != "look") {
			continue;
		}

		if ((attribute = optionNode.attribute("type"))) {
			ct->outfit.lookType = attribute.as_int();
		}

		if ((attribute = optionNode.attribute("item")) || (attribute = optionNode.attribute("lookex")) || (attribute = optionNode.attribute("typeex"))) {
			ct->outfit.lookItem = attribute.as_int();
		}

		if ((attribute = optionNode.attribute("mount"))) {
			ct->outfit.lookMount = attribute.as_int();
		}

		if ((attribute = optionNode.attribute("addon"))) {
			ct->outfit.lookAddon = attribute.as_int();
		}

		if ((attribute = optionNode.attribute("head"))) {
			ct->outfit.lookHead = attribute.as_int();
		}

		if ((attribute = optionNode.attribute("body"))) {
			ct->outfit.lookBody = attribute.as_int();
		}

		if ((attribute = optionNode.attribute("legs"))) {
			ct->outfit.lookLegs = attribute.as_int();
		}

		if ((attribute = optionNode.attribute("feet"))) {
			ct->outfit.lookFeet = attribute.as_int();
		}

		if ((attribute = optionNode.attribute("mounthead"))) {
			ct->outfit.lookMountHead = attribute.as_int();
		}

		if ((attribute = optionNode.attribute("mountbody"))) {
			ct->outfit.lookMountBody = attribute.as_int();
		}

		if ((attribute = optionNode.attribute("mountlegs"))) {
			ct->outfit.lookMountLegs = attribute.as_int();
		}

		if ((attribute = optionNode.attribute("mountfeet"))) {
			ct->outfit.lookMountFeet = attribute.as_int();
		}
	}
	return ct;
}

CreatureDatabase::CreatureDatabase() {
	////
}

CreatureDatabase::~CreatureDatabase() {
	clear();
}

void CreatureDatabase::clear() {
	for (auto& [_, creature] : creature_map) {
		delete creature;
	}
	creature_map.clear();
}

CreatureType* CreatureDatabase::operator[](const std::string& name) {
	auto iter = creature_map.find(as_lower_str(name));
	if (iter != creature_map.end()) {
		return iter->second;
	}
	return nullptr;
}

CreatureType* CreatureDatabase::addMissingCreatureType(const std::string& name, bool isNpc) {
	assert((*this)[name] == nullptr);

	CreatureType* ct = newd CreatureType();
	ct->name = name;
	ct->isNpc = isNpc;
	ct->missing = true;
	ct->outfit.lookType = 128;
	ct->outfit.lookHead = 78;
	ct->outfit.lookBody = 69;
	ct->outfit.lookLegs = 58;
	ct->outfit.lookFeet = 76;
	ct->outfit.lookAddon = 0;

	creature_map.insert(std::make_pair(as_lower_str(name), ct));
	return ct;
}

CreatureType* CreatureDatabase::addCreatureType(const std::string& name, bool isNpc, const Outfit& outfit) {
	assert((*this)[name] == nullptr);

	CreatureType* ct = newd CreatureType();
	ct->name = name;
	ct->isNpc = isNpc;
	ct->missing = false;
	ct->outfit = outfit;

	creature_map.insert(std::make_pair(as_lower_str(name), ct));
	return ct;
}

bool CreatureDatabase::hasMissing() const {
	return std::ranges::any_of(creature_map, [](const auto& pair) {
		return pair.second->missing;
	});
}

bool CreatureDatabase::loadFromXML(const FileName& filename, bool standard, wxString& error, std::vector<std::string>& warnings) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(filename.GetFullPath().mb_str());
	if (!result) {
		error = "Couldn't open file \"" + filename.GetFullName() + "\", invalid format?";
		return false;
	}

	pugi::xml_node node = doc.child("creatures");
	if (!node) {
		error = "Invalid file signature, this file is not a valid creatures file.";
		return false;
	}

	for (pugi::xml_node creatureNode = node.first_child(); creatureNode; creatureNode = creatureNode.next_sibling()) {
		if (as_lower_str(creatureNode.name()) != "creature") {
			continue;
		}

		CreatureType* creatureType = CreatureType::loadFromXML(creatureNode, warnings);
		if (creatureType) {
			creatureType->standard = standard;
			if ((*this)[creatureType->name]) {
				warnings.push_back((wxString("Duplicate creature type name \"") + wxstr(creatureType->name) + "\"! Discarding...").ToStdString());
				delete creatureType;
			} else {
				creature_map[as_lower_str(creatureType->name)] = creatureType;
			}
		}
	}
	return true;
}

static void ensureCreatureBrush(CreatureType* creatureType) {
	if (creatureType->brush) {
		return;
	}

	Tileset* tileSet = nullptr;
	if (creatureType->isNpc) {
		tileSet = g_materials.tilesets["NPCs"];
	} else {
		tileSet = g_materials.tilesets["Monsters"];
	}
	ASSERT(tileSet != nullptr);

	auto brush = std::make_unique<CreatureBrush>(creatureType);
	creatureType->brush = brush.get();
	g_brushes.addBrush(std::move(brush));
	creatureType->in_other_tileset = true;

	TilesetCategory* tileSetCategory = tileSet->getCategory(TILESET_CREATURE);
	tileSetCategory->brushlist.push_back(creatureType->brush);
}

struct ParsedLuaCreature {
	std::string name;
	Outfit outfit;
};

static std::string readLuaCreatureFileContent(const std::string& file_path) {
	std::ifstream stream(file_path, std::ios::binary | std::ios::ate);
	if (!stream.is_open()) {
		return {};
	}

	const auto size = stream.tellg();
	if (size < 0) {
		return {};
	}

	stream.seekg(0, std::ios::beg);
	const auto content_size = static_cast<size_t>(size);
	std::string content(content_size, '\0');
	if (!stream.read(content.data(), static_cast<std::streamsize>(content_size))) {
		return {};
	}

	return content;
}

static std::string parseLuaCreateCall(std::string_view content, std::string_view func_name) {
	size_t position = content.find(func_name);
	if (position == std::string::npos) {
		return {};
	}

	position += func_name.size();
	while (position < content.size() && std::isspace(static_cast<unsigned char>(content[position]))) {
		++position;
	}
	if (position >= content.size() || content[position] != '(') {
		return {};
	}

	++position;
	while (position < content.size() && std::isspace(static_cast<unsigned char>(content[position]))) {
		++position;
	}
	if (position >= content.size() || content[position] != '"') {
		return {};
	}

	++position;
	const size_t name_start = position;
	while (position < content.size() && content[position] != '"') {
		++position;
	}
	if (position >= content.size()) {
		return {};
	}

	return std::string(content.substr(name_start, position - name_start));
}

static std::string parseLuaLocalString(std::string_view content, std::string_view variable_name) {
	const std::string pattern = "local " + std::string { variable_name };
	size_t position = content.find(pattern);
	if (position == std::string::npos) {
		return {};
	}

	position += pattern.size();
	while (position < content.size() && std::isspace(static_cast<unsigned char>(content[position]))) {
		++position;
	}
	if (position >= content.size() || content[position] != '=') {
		return {};
	}

	++position;
	while (position < content.size() && std::isspace(static_cast<unsigned char>(content[position]))) {
		++position;
	}
	if (position >= content.size() || content[position] != '"') {
		return {};
	}

	++position;
	const size_t name_start = position;
	while (position < content.size() && content[position] != '"') {
		++position;
	}
	if (position >= content.size()) {
		return {};
	}

	return std::string(content.substr(name_start, position - name_start));
}

static int parseLuaOutfitField(std::string_view block, std::string_view key) {
	size_t search_from = 0;
	while (search_from < block.size()) {
		size_t position = block.find(key, search_from);
		if (position == std::string::npos) {
			return -1;
		}

		const size_t after_key = position + key.size();
		if (after_key < block.size()) {
			const unsigned char next_char = static_cast<unsigned char>(block[after_key]);
			if (std::isalnum(next_char) || next_char == '_') {
				search_from = after_key;
				continue;
			}
		}

		position = after_key;
		while (position < block.size() && std::isspace(static_cast<unsigned char>(block[position]))) {
			++position;
		}
		if (position >= block.size() || block[position] != '=') {
			search_from = position;
			continue;
		}

		++position;
		while (position < block.size() && std::isspace(static_cast<unsigned char>(block[position]))) {
			++position;
		}

		int value = 0;
		bool found_digit = false;
		while (position < block.size() && std::isdigit(static_cast<unsigned char>(block[position]))) {
			value = value * 10 + (block[position] - '0');
			found_digit = true;
			++position;
		}
		if (found_digit) {
			return value;
		}

		search_from = position;
	}

	return -1;
}

static bool parseLuaOutfit(std::string_view content, Outfit& outfit) {
	size_t outfit_position = content.find(".outfit");
	if (outfit_position == std::string::npos) {
		outfit_position = content.find(":outfit(");
	}
	if (outfit_position == std::string::npos) {
		return false;
	}

	const size_t brace_start = content.find('{', outfit_position);
	if (brace_start == std::string::npos) {
		return false;
	}

	int depth = 1;
	size_t brace_end = brace_start + 1;
	while (brace_end < content.size() && depth > 0) {
		if (content[brace_end] == '{') {
			++depth;
		} else if (content[brace_end] == '}') {
			--depth;
		}
		++brace_end;
	}
	if (depth != 0) {
		return false;
	}

	--brace_end;
	const std::string block(content.substr(brace_start, brace_end - brace_start + 1));

	int value = -1;
	bool has_base_look = false;
	if ((value = parseLuaOutfitField(block, "lookType")) >= 0) {
		outfit.lookType = value;
		has_base_look = true;
	}
	if ((value = parseLuaOutfitField(block, "lookTypeEx")) >= 0) {
		outfit.lookItem = value;
		has_base_look = true;
	}
	if ((value = parseLuaOutfitField(block, "lookHead")) >= 0) {
		outfit.lookHead = value;
	}
	if ((value = parseLuaOutfitField(block, "lookBody")) >= 0) {
		outfit.lookBody = value;
	}
	if ((value = parseLuaOutfitField(block, "lookLegs")) >= 0) {
		outfit.lookLegs = value;
	}
	if ((value = parseLuaOutfitField(block, "lookFeet")) >= 0) {
		outfit.lookFeet = value;
	}
	if ((value = parseLuaOutfitField(block, "lookAddons")) >= 0) {
		outfit.lookAddon = value;
	}
	if ((value = parseLuaOutfitField(block, "lookMount")) >= 0) {
		outfit.lookMount = value;
	}
	if ((value = parseLuaOutfitField(block, "lookMountHead")) >= 0) {
		outfit.lookMountHead = value;
	}
	if ((value = parseLuaOutfitField(block, "lookMountBody")) >= 0) {
		outfit.lookMountBody = value;
	}
	if ((value = parseLuaOutfitField(block, "lookMountLegs")) >= 0) {
		outfit.lookMountLegs = value;
	}
	if ((value = parseLuaOutfitField(block, "lookMountFeet")) >= 0) {
		outfit.lookMountFeet = value;
	}

	return has_base_look;
}

static std::optional<ParsedLuaCreature> parseLuaCreature(
	std::string_view content,
	std::string_view create_call_name,
	std::string_view fallback_local_name
) {
	std::string name = parseLuaCreateCall(content, create_call_name);
	if (name.empty()) {
		name = parseLuaLocalString(content, fallback_local_name);
	}
	if (name.empty()) {
		return std::nullopt;
	}

	ParsedLuaCreature result;
	result.name = std::move(name);
	if (!parseLuaOutfit(content, result.outfit)) {
		return std::nullopt;
	}

	return result;
}

static bool looksLikeLuaCreatureDefinition(
	std::string_view content,
	std::string_view create_call_name,
	std::string_view fallback_local_name
) {
	return content.find(create_call_name) != std::string::npos || content.find(fallback_local_name) != std::string::npos;
}

static void mergeCreatureType(CreatureDatabase& database, CreatureType* creature_type) {
	CreatureType* current = database[creature_type->name];
	if (current) {
		CreatureType::preserve_assign_creature_fields(current, *creature_type);
		delete creature_type;
		if (!current->brush) {
			ensureCreatureBrush(current);
		}
		return;
	}

	CreatureType* inserted = database.addCreatureType(creature_type->name, creature_type->isNpc, creature_type->outfit);
	inserted->missing = creature_type->missing;
	inserted->standard = creature_type->standard;
	delete creature_type;
	ensureCreatureBrush(inserted);
}

static bool importLuaDirectory(
	CreatureDatabase& database,
	const wxString& directory,
	bool is_npc,
	size_t& imported_count,
	wxString& error,
	std::vector<std::string>& warnings
) {
	imported_count = 0;
	if (directory.empty()) {
		return true;
	}
	if (!wxDir::Exists(directory)) {
		error = "Lua creature directory does not exist: " + directory;
		return false;
	}

	wxArrayString lua_files;
	wxDir::GetAllFiles(directory, &lua_files, "*.lua", wxDIR_FILES | wxDIR_DIRS);

	int processed_files = 0;
	for (const auto& file_path : lua_files) {
		if (++processed_files % 50 == 0) {
			wxSafeYield();
		}

		const std::string content = readLuaCreatureFileContent(file_path.ToStdString());
		if (content.empty()) {
			warnings.push_back("Couldn't open Lua creature file: " + file_path.ToStdString());
			continue;
		}

		const auto create_call_name = is_npc ? "Game.createNpcType" : "Game.createMonsterType";
		const auto fallback_local_name = is_npc ? "internalNpcName" : "internalMonsterName";
		const auto parsed = parseLuaCreature(
			content,
			create_call_name,
			fallback_local_name
		);
		if (!parsed) {
			if (looksLikeLuaCreatureDefinition(content, create_call_name, fallback_local_name)) {
				warnings.push_back("Skipping invalid Lua creature file: " + file_path.ToStdString());
			}
			continue;
		}

		auto* creature_type = newd CreatureType();
		creature_type->name = parsed->name;
		creature_type->isNpc = is_npc;
		creature_type->standard = false;
		creature_type->outfit = parsed->outfit;

		mergeCreatureType(database, creature_type);
		++imported_count;
	}

	return true;
}

bool CreatureDatabase::importXMLFromOT(const FileName& filename, wxString& error, std::vector<std::string>& warnings) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(filename.GetFullPath().mb_str());
	if (!result) {
		error = "Couldn't open file \"" + filename.GetFullName() + "\", invalid format?";
		return false;
	}

	pugi::xml_node node;
	if ((node = doc.child("monsters"))) {
		for (pugi::xml_node monsterNode = node.first_child(); monsterNode; monsterNode = monsterNode.next_sibling()) {
			if (as_lower_str(monsterNode.name()) != "monster") {
				continue;
			}

			pugi::xml_attribute attribute;
			if (!(attribute = monsterNode.attribute("file"))) {
				warnings.push_back(std::format(
					"Skipping <monster> entry in {} because it is missing a file attribute.",
					filename.GetFullPath().ToStdString()
				));
				continue;
			}

			FileName monsterFile = XmlFileLoader::resolveRelative(filename, wxString::FromUTF8(attribute.as_string()));

			pugi::xml_document monsterDoc;
			pugi::xml_parse_result monsterResult = monsterDoc.load_file(monsterFile.GetFullPath().mb_str());
			if (!monsterResult) {
				warnings.push_back(std::format(
					"Skipping <monster file=\"{}\"> in {} because {} could not be loaded: {}.",
					attribute.as_string(),
					filename.GetFullPath().ToStdString(),
					monsterFile.GetFullPath().ToStdString(),
					monsterResult.description()
				));
				continue;
			}

			CreatureType* creatureType = CreatureType::loadFromOTXML(monsterFile, monsterDoc, warnings);
			if (creatureType) {
				mergeCreatureType(*this, creatureType);
			}
		}
	} else if ((node = doc.child("npcs"))) {
		for (pugi::xml_node npcNode = node.first_child(); npcNode; npcNode = npcNode.next_sibling()) {
			if (as_lower_str(npcNode.name()) != "npc") {
				continue;
			}

			pugi::xml_attribute attribute;
			if (!(attribute = npcNode.attribute("file"))) {
				warnings.push_back(std::format(
					"Skipping <npc> entry in {} because it is missing a file attribute.",
					filename.GetFullPath().ToStdString()
				));
				continue;
			}

			FileName npcFile = XmlFileLoader::resolveRelative(filename, wxString::FromUTF8(attribute.as_string()));

			pugi::xml_document npcDoc;
			pugi::xml_parse_result npcResult = npcDoc.load_file(npcFile.GetFullPath().mb_str());
			if (!npcResult) {
				warnings.push_back(std::format(
					"Skipping <npc file=\"{}\"> in {} because {} could not be loaded: {}.",
					attribute.as_string(),
					filename.GetFullPath().ToStdString(),
					npcFile.GetFullPath().ToStdString(),
					npcResult.description()
				));
				continue;
			}

			CreatureType* creatureType = CreatureType::loadFromOTXML(npcFile, npcDoc, warnings);
			if (creatureType) {
				mergeCreatureType(*this, creatureType);
			}
		}
	} else if ((node = doc.child("monster")) || (node = doc.child("npc"))) {
		CreatureType* creatureType = CreatureType::loadFromOTXML(filename, doc, warnings);
		if (creatureType) {
			mergeCreatureType(*this, creatureType);
		}
	} else {
		error = "This is not valid OT npc/monster data file.";
		return false;
	}
	return true;
}

bool CreatureDatabase::importLuaMonsters(const wxString& directory, size_t& imported_count, wxString& error, std::vector<std::string>& warnings) {
	return importLuaDirectory(*this, directory, false, imported_count, error, warnings);
}

bool CreatureDatabase::importLuaNpcs(const wxString& directory, size_t& imported_count, wxString& error, std::vector<std::string>& warnings) {
	return importLuaDirectory(*this, directory, true, imported_count, error, warnings);
}

bool CreatureDatabase::saveToXML(const FileName& filename) {
	pugi::xml_document doc;

	pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
	decl.append_attribute("version") = "1.0";

	pugi::xml_node creatureNodes = doc.append_child("creatures");
	for (const auto& creatureEntry : creature_map) {
		CreatureType* creatureType = creatureEntry.second;
		if (!creatureType->standard) {
			pugi::xml_node creatureNode = creatureNodes.append_child("creature");

			creatureNode.append_attribute("name") = creatureType->name.c_str();
			creatureNode.append_attribute("type") = creatureType->isNpc ? "npc" : "monster";

			const Outfit& outfit = creatureType->outfit;
			creatureNode.append_attribute("looktype") = outfit.lookType;
			creatureNode.append_attribute("lookitem") = outfit.lookItem;
			creatureNode.append_attribute("lookmount") = outfit.lookMount;
			creatureNode.append_attribute("lookaddon") = outfit.lookAddon;
			creatureNode.append_attribute("lookhead") = outfit.lookHead;
			creatureNode.append_attribute("lookbody") = outfit.lookBody;
			creatureNode.append_attribute("looklegs") = outfit.lookLegs;
			creatureNode.append_attribute("lookfeet") = outfit.lookFeet;
			creatureNode.append_attribute("lookmounthead") = outfit.lookMountHead;
			creatureNode.append_attribute("lookmountbody") = outfit.lookMountBody;
			creatureNode.append_attribute("lookmountlegs") = outfit.lookMountLegs;
			creatureNode.append_attribute("lookmountfeet") = outfit.lookMountFeet;
		}
	}
	return doc.save_file(filename.GetFullPath().mb_str(), "\t", pugi::format_default, pugi::encoding_utf8);
}
