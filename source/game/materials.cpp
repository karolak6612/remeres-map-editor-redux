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

#include "editor/editor.h"
#include "item_definitions/core/item_definition_store.h"
#include "game/creatures.h"
#include "game/material_include_resolver.h"

#include "ui/gui.h"
#include "game/materials.h"
#include "brushes/brush.h"
#include "brushes/creature/creature_brush.h"
#include "brushes/raw/raw_brush.h"

Materials g_materials;

namespace {
	[[nodiscard]] std::string normalizedPathKey(const FileName& file) {
		return file.GetFullPath().ToStdString();
	}
}

Materials::Materials() {
	////
}

Materials::~Materials() {
	clear();
}

void Materials::clear() {
	for (MaterialsExtensionList::iterator iter = extensions.begin(); iter != extensions.end(); ++iter) {
		delete *iter;
	}

	extensions.clear();
	database.clear();
}

const MaterialsExtensionList& Materials::getExtensions() {
	return extensions;
}

MaterialsExtensionList Materials::getExtensionsByVersion(const ClientVersionID& version_id) {
	MaterialsExtensionList ret_list;
	for (MaterialsExtensionList::iterator iter = extensions.begin(); iter != extensions.end(); ++iter) {
		if ((*iter)->isForVersion(version_id)) {
			ret_list.push_back(*iter);
		}
	}
	return ret_list;
}

bool Materials::loadMaterials(const FileName& identifier, wxString& error, std::vector<std::string>& warnings) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(identifier.GetFullPath().mb_str());
	if (!result) {
		warnings.push_back((wxString("Could not open ") + identifier.GetFullName() + " (file not found or syntax error)").ToStdString());
		return false;
	}

	pugi::xml_node node = doc.child("materials");
	if (!node) {
		warnings.push_back((identifier.GetFullName() + ": Invalid rootheader.").ToStdString());
		return false;
	}

	database.clear();
	database.bindSourceTruth(g_brushes, g_item_definitions, g_creatures);
	if (!unserializeMaterials(identifier, node, error, warnings)) {
		return false;
	}
	return true;
}

bool Materials::unserializeMaterials(const FileName& filename, pugi::xml_node node, wxString& error, std::vector<std::string>& warnings) {
	bool sawBorders = false;
	bool sawBrushes = false;
	bool sawCreatures = false;
	bool sawItems = false;
	bool sawTilesets = false;
	bool sawPalettes = false;

	for (pugi::xml_node childNode = node.first_child(); childNode; childNode = childNode.next_sibling()) {
		const std::string childName = as_lower_str(childNode.name());
		if (childName == "metaitem") {
			if (const auto attribute = childNode.attribute("id")) {
				g_item_definitions.ensureMetaItem(attribute.as_ushort());
			}
		} else if (childName == "borders") {
			sawBorders = true;
			if (!loadModuleIncludes(filename, childNode, "border", error, warnings)) {
				return false;
			}
		} else if (childName == "brushes") {
			sawBrushes = true;
			if (!loadModuleIncludes(filename, childNode, "brush", error, warnings)) {
				return false;
			}
		} else if (childName == "creatures") {
			sawCreatures = true;
			if (!loadModuleIncludes(filename, childNode, {}, error, warnings)) {
				return false;
			}
		} else if (childName == "items") {
			sawItems = true;
			if (!loadModuleIncludes(filename, childNode, {}, error, warnings)) {
				return false;
			}
		} else if (childName == "tileset") {
			error = "Legacy <tileset> nodes are not supported by the modular material loader.";
			return false;
		} else if (childName == "tilesets") {
			sawTilesets = true;
			std::vector<FileName> tilesetFiles;
			if (!MaterialIncludeResolver::collectSectionFiles(filename, childNode, "tilesets", tilesetFiles, error)) {
				return false;
			}
			std::vector<std::string> sourceKeys;
			sourceKeys.reserve(tilesetFiles.size());
			for (const FileName& tilesetFile : tilesetFiles) {
				sourceKeys.push_back(normalizedPathKey(tilesetFile));
			}
			database.setTilesetSources(std::move(sourceKeys));
		} else if (childName == "palettes") {
			sawPalettes = true;
			if (!loadPaletteIncludes(filename, childNode, error, warnings)) {
				return false;
			}
		}
	}

	if (!sawBorders || !sawBrushes || !sawCreatures || !sawItems || !sawTilesets || !sawPalettes) {
		error = "Modular materials.xml must define borders, brushes, creatures, items, tilesets, and palettes sections.";
		return false;
	}
	return true;
}

bool Materials::loadModuleIncludes(const FileName& manifest, pugi::xml_node section, std::string_view expectedNode, wxString& error, std::vector<std::string>& warnings) {
	bool loadedAny = false;
	for (pugi::xml_node includeNode = section.child("include"); includeNode; includeNode = includeNode.next_sibling("include")) {
		std::vector<FileName> files;
		if (const auto fileAttribute = includeNode.attribute("file")) {
			FileName includeFile = MaterialIncludeResolver::resolveRelativePath(manifest, fileAttribute.as_string());
			if (!includeFile.FileExists()) {
				error = "Missing modular include file: " + includeFile.GetFullPath();
				return false;
			}
			files.push_back(includeFile);
		} else if (const auto folderAttribute = includeNode.attribute("folder")) {
			FileName folder = MaterialIncludeResolver::resolveRelativeFolder(manifest, folderAttribute.as_string());
			if (!folder.DirExists()) {
				error = "Missing modular include folder: " + folder.GetFullPath();
				return false;
			}
			files = MaterialIncludeResolver::collectXmlFiles(folder, includeNode.attribute("subfolders").as_bool(false));
		}

		for (const FileName& file : files) {
			loadedAny = true;
			if (expectedNode.empty()) {
				continue;
			}

			pugi::xml_document doc;
			pugi::xml_parse_result result = doc.load_file(file.GetFullPath().mb_str());
			if (!result) {
				error = "Could not parse modular include file: " + file.GetFullPath();
				return false;
			}

			pugi::xml_node root = doc.document_element();
			for (pugi::xml_node childNode = root.first_child(); childNode; childNode = childNode.next_sibling()) {
				const std::string childName = as_lower_str(childNode.name());
				if (childName != expectedNode) {
					continue;
				}
				if (expectedNode == "border") {
					g_brushes.unserializeBorder(childNode, warnings);
				} else if (expectedNode == "brush") {
					g_brushes.unserializeBrush(childNode, warnings);
				}
			}
		}
	}

	if (!loadedAny) {
		error = "Modular section has no include entries: " + wxString(section.name());
		return false;
	}
	return true;
}

bool Materials::loadPaletteIncludes(const FileName& manifest, pugi::xml_node section, wxString& error, std::vector<std::string>& warnings) {
	bool loadedAny = false;
	for (pugi::xml_node includeNode = section.child("include"); includeNode; includeNode = includeNode.next_sibling("include")) {
		if (const auto fileAttribute = includeNode.attribute("file")) {
			FileName includeFile = MaterialIncludeResolver::resolveRelativePath(manifest, fileAttribute.as_string());
			if (!includeFile.FileExists()) {
				error = "Missing palettes include file: " + includeFile.GetFullPath();
				return false;
			}
			loadedAny = true;
			if (!loadPaletteFile(includeFile, error, warnings)) {
				return false;
			}
		} else if (const auto folderAttribute = includeNode.attribute("folder")) {
			FileName folder = MaterialIncludeResolver::resolveRelativeFolder(manifest, folderAttribute.as_string());
			if (!folder.DirExists()) {
				error = "Missing palettes include folder: " + folder.GetFullPath();
				return false;
			}
			for (const FileName& file : MaterialIncludeResolver::collectXmlFiles(folder, includeNode.attribute("subfolders").as_bool(false))) {
				loadedAny = true;
				if (!loadPaletteFile(file, error, warnings)) {
					return false;
				}
			}
		}
	}

	if (!loadedAny) {
		error = "Modular palettes section has no include entries.";
		return false;
	}
	return true;
}

static RAWBrush* ensureRawBrush(ServerItemId item_id) {
	ItemEditorData& editor_data = g_item_definitions.mutableEditorData(item_id);
	if (editor_data.raw_brush == nullptr) {
		auto raw_brush = std::make_unique<RAWBrush>(item_id);
		editor_data.raw_brush = raw_brush.get();
		g_item_definitions.setFlag(item_id, ItemFlag::HasRaw, true);
		g_brushes.addBrush(std::move(raw_brush));
	}
	return editor_data.raw_brush;
}

static CreatureBrush* ensureCreatureBrush(CreatureType* type) {
	if (!type) {
		return nullptr;
	}
	if (type->brush) {
		return type->brush;
	}
	auto creature_brush = std::make_unique<CreatureBrush>(type);
	type->brush = creature_brush.get();
	CreatureBrush* brush = creature_brush.get();
	g_brushes.addBrush(std::move(creature_brush));
	return brush;
}

bool Materials::loadPaletteFile(const FileName& filename, wxString& error, std::vector<std::string>& warnings) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(filename.GetFullPath().mb_str());
	if (!result) {
		error = "Could not parse palettes file: " + filename.GetFullPath();
		return false;
	}

	pugi::xml_node root = doc.child("palettes");
	if (!root) {
		error = "Invalid palettes file root: " + filename.GetFullPath();
		return false;
	}

	for (pugi::xml_node paletteNode = root.child("palette"); paletteNode; paletteNode = paletteNode.next_sibling("palette")) {
		const auto nameAttribute = paletteNode.attribute("name");
		if (!nameAttribute || std::string(nameAttribute.as_string()).empty()) {
			warnings.push_back("palettes: skipped palette without name in " + filename.GetFullPath().ToStdString());
			continue;
		}

		DynamicPaletteDefinition palette;
		palette.name = nameAttribute.as_string();

		for (pugi::xml_node tilesetNode = paletteNode.child("tileset"); tilesetNode; tilesetNode = tilesetNode.next_sibling("tileset")) {
			for (pugi::xml_node includeNode = tilesetNode.child("include"); includeNode; includeNode = includeNode.next_sibling("include")) {
				if (!loadTilesetInclude(filename, includeNode, palette, error, warnings)) {
					return false;
				}
			}
		}

		database.paletteCatalog().addDynamicPalette(std::move(palette));
	}
	return true;
}

bool Materials::loadTilesetInclude(const FileName& manifest, pugi::xml_node includeNode, DynamicPaletteDefinition& palette, wxString& error, std::vector<std::string>& warnings) {
	if (const auto fileAttribute = includeNode.attribute("file")) {
		FileName includeFile = MaterialIncludeResolver::resolveRelativePath(manifest, fileAttribute.as_string());
		if (!includeFile.FileExists()) {
			error = "Missing tileset include file: " + includeFile.GetFullPath();
			return false;
		}
		if (!database.isKnownTilesetSource(normalizedPathKey(includeFile))) {
			error = "Tileset include is not declared in materials.xml <tilesets>: " + includeFile.GetFullPath();
			return false;
		}
		return loadDynamicTilesetFile(includeFile, palette, error, warnings);
	}

	if (const auto folderAttribute = includeNode.attribute("folder")) {
		FileName folder = MaterialIncludeResolver::resolveRelativeFolder(manifest, folderAttribute.as_string());
		if (!folder.DirExists()) {
			error = "Missing tileset include folder: " + folder.GetFullPath();
			return false;
		}
		for (const FileName& file : MaterialIncludeResolver::collectXmlFiles(folder, includeNode.attribute("subfolders").as_bool(false))) {
			if (!database.isKnownTilesetSource(normalizedPathKey(file))) {
				error = "Tileset include is not declared in materials.xml <tilesets>: " + file.GetFullPath();
				return false;
			}
			if (!loadDynamicTilesetFile(file, palette, error, warnings)) {
				return false;
			}
		}
	}
	return true;
}

bool Materials::loadDynamicTilesetFile(const FileName& filename, DynamicPaletteDefinition& palette, wxString& error, std::vector<std::string>& warnings) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(filename.GetFullPath().mb_str());
	if (!result) {
		error = "Could not parse tileset file: " + filename.GetFullPath();
		return false;
	}

	pugi::xml_node root = doc.child("tileset");
	if (!root) {
		error = "Invalid tileset file root: " + filename.GetFullPath();
		return false;
	}

	const auto nameAttribute = root.attribute("name");
	if (!nameAttribute || std::string(nameAttribute.as_string()).empty()) {
		error = "Tileset file is missing name attribute: " + filename.GetFullPath();
		return false;
	}

	DynamicTilesetDefinition tileset;
	tileset.name = nameAttribute.as_string();

	for (pugi::xml_node childNode = root.first_child(); childNode; childNode = childNode.next_sibling()) {
		const std::string childName = as_lower_str(childNode.name());
		if (childName == "brush") {
			const auto brushName = childNode.attribute("name");
			if (!brushName) {
				continue;
			}
			if (Brush* brush = g_brushes.getBrush(brushName.as_string())) {
				brush->flagAsVisible();
				tileset.brushes.push_back(brush);
			} else {
				warnings.push_back("tileset_brush_references: tileset=\"" + tileset.name + "\" brush=\"" + std::string(brushName.as_string()) + "\"");
			}
		} else if (childName == "item") {
			uint16_t fromId = 0;
			uint16_t toId = 0;
			if (const auto idAttribute = childNode.attribute("id")) {
				fromId = idAttribute.as_ushort();
				toId = fromId;
			} else if (const auto fromAttribute = childNode.attribute("fromid")) {
				fromId = fromAttribute.as_ushort();
				toId = childNode.attribute("toid").as_ushort(fromId);
			} else {
				continue;
			}
			if (toId < fromId) {
				std::swap(fromId, toId);
			}
			for (uint16_t id = fromId; id <= toId; ++id) {
				const auto definition = g_item_definitions.get(id);
				if (!definition) {
					warnings.push_back("tileset_item_references: tileset=\"" + tileset.name + "\" missing_id=" + std::to_string(id));
					continue;
				}
				RAWBrush* brush = ensureRawBrush(id);
				brush->flagAsVisible();
				tileset.brushes.push_back(brush);
			}
		} else if (childName == "creature") {
			const auto creatureName = childNode.attribute("name");
			if (!creatureName) {
				continue;
			}
			CreatureType* type = g_creatures[creatureName.as_string()];
			if (!type) {
				warnings.push_back("tileset_creature_references: tileset=\"" + tileset.name + "\" creature=\"" + std::string(creatureName.as_string()) + "\"");
				continue;
			}
			CreatureBrush* brush = ensureCreatureBrush(type);
			brush->flagAsVisible();
			tileset.brushes.push_back(brush);
		}
	}

	palette.tilesets.push_back(std::move(tileset));
	return true;
}
