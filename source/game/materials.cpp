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

#include <wx/dir.h>

#include "editor/editor.h"
#include "item_definitions/core/item_definition_store.h"
#include "game/creatures.h"
#include "io/xml_file_loader.h"

#include "ui/gui.h"
#include "game/materials.h"
#include "brushes/brush.h"
#include "brushes/creature/creature_brush.h"
#include "brushes/raw/raw_brush.h"

Materials g_materials;

Materials::Materials() {
	////
}

Materials::~Materials() {
	clear();
}

void Materials::clear() {
	for (TilesetContainer::iterator iter = tilesets.begin(); iter != tilesets.end(); ++iter) {
		delete iter->second;
	}

	for (MaterialsExtensionList::iterator iter = extensions.begin(); iter != extensions.end(); ++iter) {
		delete *iter;
	}

	tilesets.clear();
	extensions.clear();
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
	const auto visitor = [&](const FileName& source_file, pugi::xml_node child_node, wxString& visit_error, std::vector<std::string>& visit_warnings) {
		const auto child_name = as_lower_str(child_node.name());
		if (child_name == "metaitem") {
			if (const auto attribute = child_node.attribute("id")) {
				g_item_definitions.ensureMetaItem(attribute.as_ushort());
			}
			return true;
		}
		if (child_name == "border") {
			g_brushes.unserializeBorder(child_node, visit_warnings);
			return true;
		}
		if (child_name == "brush") {
			g_brushes.unserializeBrush(child_node, visit_warnings);
			return true;
		}
		if (child_name == "tileset") {
			return unserializeTileset(child_node, visit_warnings);
		}
		(void)source_file;
		(void)visit_error;
		return true;
	};
	return XmlFileLoader::visitElements(identifier, "materials", visitor, error, warnings);
}

bool Materials::loadExtensions(FileName directoryName, wxString& error, std::vector<std::string>& warnings) {
	directoryName.Mkdir(0755, wxPATH_MKDIR_FULL); // Create if it doesn't exist

	wxDir ext_dir(directoryName.GetPath());
	if (!ext_dir.IsOpened()) {
		error = "Could not open extensions directory.";
		return false;
	}

	wxString filename;
	if (!ext_dir.GetFirst(&filename)) {
		// No extensions found
		return true;
	}

	StringVector clientVersions;
	do {
		FileName fn;
		fn.SetPath(directoryName.GetPath());
		fn.SetFullName(filename);
		if (fn.GetExt() != "xml") {
			continue;
		}

		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file(fn.GetFullPath().mb_str());
		if (!result) {
			warnings.push_back((wxString("Could not open ") + filename + " (file not found or syntax error)").ToStdString());
			continue;
		}

		pugi::xml_node extensionNode = doc.child("materialsextension");
		if (!extensionNode) {
			warnings.push_back((filename + ": Invalid rootheader.").ToStdString());
			continue;
		}

		pugi::xml_attribute attribute;
		if (!(attribute = extensionNode.attribute("name"))) {
			warnings.push_back((filename + ": Couldn't read extension name.").ToStdString());
			continue;
		}

		const std::string& extensionName = attribute.as_string();
		if (!(attribute = extensionNode.attribute("author"))) {
			warnings.push_back((filename + ": Couldn't read extension author.").ToStdString());
			continue;
		}

		const std::string& extensionAuthor = attribute.as_string();
		if (!(attribute = extensionNode.attribute("description"))) {
			warnings.push_back((filename + ": Couldn't read extension description.").ToStdString());
			continue;
		}

		const std::string& extensionDescription = attribute.as_string();
		if (extensionName.empty() || extensionAuthor.empty() || extensionDescription.empty()) {
			warnings.push_back((filename + ": Couldn't read extension attributes (name, author, description).").ToStdString());
			continue;
		}

		std::string extensionUrl = extensionNode.attribute("url").as_string();
		extensionUrl.erase(std::remove(extensionUrl.begin(), extensionUrl.end(), '\''));

		std::string extensionAuthorLink = extensionNode.attribute("authorurl").as_string();
		extensionAuthorLink.erase(std::remove(extensionAuthorLink.begin(), extensionAuthorLink.end(), '\''));

		MaterialsExtension* materialExtension = newd MaterialsExtension(extensionName, extensionAuthor, extensionDescription);
		materialExtension->url = extensionUrl;
		materialExtension->author_url = extensionAuthorLink;

		if ((attribute = extensionNode.attribute("client"))) {
			clientVersions.clear();
			const std::string& extensionClientString = attribute.as_string();

			size_t lastPosition = 0;
			size_t position = extensionClientString.find(';');
			while (position != std::string::npos) {
				clientVersions.push_back(extensionClientString.substr(lastPosition, position - lastPosition));
				lastPosition = position + 1;
				position = extensionClientString.find(';', lastPosition);
			}

			clientVersions.push_back(extensionClientString.substr(lastPosition));
			for (const std::string& version : clientVersions) {
				materialExtension->addVersion(version);
			}

			std::sort(materialExtension->version_list.begin(), materialExtension->version_list.end(), VersionComparisonPredicate);

			auto duplicate = std::unique(materialExtension->version_list.begin(), materialExtension->version_list.end());
			while (duplicate != materialExtension->version_list.end()) {
				materialExtension->version_list.erase(duplicate);
				duplicate = std::unique(materialExtension->version_list.begin(), materialExtension->version_list.end());
			}
		} else {
			warnings.push_back((filename + ": Extension is not available for any version.").ToStdString());
		}

		extensions.push_back(materialExtension);
		if (materialExtension->isForVersion(g_version.GetCurrentVersionID())) {
			unserializeMaterials(filename, extensionNode, error, warnings);
		}
	} while (ext_dir.GetNext(&filename));

	return true;
}

bool Materials::unserializeMaterials(const FileName& filename, pugi::xml_node node, wxString& error, std::vector<std::string>& warnings) {
	wxString warning;
	pugi::xml_attribute attribute;
	for (pugi::xml_node childNode = node.first_child(); childNode; childNode = childNode.next_sibling()) {
		const std::string& childName = as_lower_str(childNode.name());
		if (childName == "metaitem") {
			if (const auto attribute = childNode.attribute("id")) {
				g_item_definitions.ensureMetaItem(attribute.as_ushort());
			}
		} else if (childName == "border") {
			g_brushes.unserializeBorder(childNode, warnings);
			if (warning.size()) {
				warnings.push_back((wxString("materials.xml: ") + warning).ToStdString());
			}
		} else if (childName == "brush") {
			g_brushes.unserializeBrush(childNode, warnings);
			if (warning.size()) {
				warnings.push_back((wxString("materials.xml: ") + warning).ToStdString());
			}
		} else if (childName == "tileset") {
			unserializeTileset(childNode, warnings);
		}
	}
	(void)filename;
	(void)error;
	(void)attribute;
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

void Materials::createOtherTileset() {
	Tileset* others;
	Tileset* npc_tileset;

	if (tilesets.find("Others") != tilesets.end()) {
		others = tilesets["Others"];
		others->clear();
	} else {
		others = newd Tileset(g_brushes, "Others");
		tilesets["Others"] = others;
	}

	if (tilesets.find("NPCs") != tilesets.end()) {
		npc_tileset = tilesets["NPCs"];
		npc_tileset->clear();
	} else {
		npc_tileset = newd Tileset(g_brushes, "NPCs");
		tilesets["NPCs"] = npc_tileset;
	}

	// There should really be an iterator to do this
	for (ServerItemId id : g_item_definitions.allIds()) {
		const auto definition = g_item_definitions.get(id);
		if (!definition) {
			continue;
		}

		if (!definition.isMetaItem()) {
			Brush* brush;
			const ItemEditorData& editor_data = definition.editorData();
			if (definition.hasFlag(ItemFlag::InOtherTileset)) {
				others->getCategory(TILESET_RAW)->brushlist.push_back(editor_data.raw_brush);
				continue;
			} else if (editor_data.raw_brush == nullptr) {
				brush = ensureRawBrush(id);
			} else if (!definition.hasFlag(ItemFlag::HasRaw)) {
				brush = editor_data.raw_brush;
			} else {
				continue;
			}

			brush->flagAsVisible();
			others->getCategory(TILESET_RAW)->brushlist.push_back(g_item_definitions.editorData(id).raw_brush);
			g_item_definitions.setFlag(id, ItemFlag::InOtherTileset, true);
		}
	}

	for (CreatureMap::iterator iter = g_creatures.begin(); iter != g_creatures.end(); ++iter) {
		CreatureType* type = iter->second;

		if (type->brush == nullptr) {
			auto creature_brush = std::make_unique<CreatureBrush>(type);
			type->brush = creature_brush.get();
			g_brushes.addBrush(std::move(creature_brush));
		}

		type->brush->flagAsVisible();
		type->in_other_tileset = true;

		if (type->isNpc) {
			npc_tileset->getCategory(TILESET_CREATURE)->brushlist.push_back(type->brush);
		} else {
			others->getCategory(TILESET_CREATURE)->brushlist.push_back(type->brush);
		}
	}
}

bool Materials::unserializeTileset(pugi::xml_node node, std::vector<std::string>& warnings) {
	pugi::xml_attribute attribute;
	if (!(attribute = node.attribute("name"))) {
		warnings.push_back("Couldn't read tileset name");
		return false;
	}

	const std::string& name = attribute.as_string();

	Tileset* tileset = nullptr;
	auto it = tilesets.find(name);
	if (it != tilesets.end()) {
		tileset = it->second;
	}

	if (!tileset) {
		tileset = newd Tileset(g_brushes, name);
		if (it != tilesets.end()) {
			it->second = tileset;
		} else {
			tilesets.insert(std::make_pair(name, tileset));
		}
	}

	for (pugi::xml_node childNode = node.first_child(); childNode; childNode = childNode.next_sibling()) {
		tileset->loadCategory(childNode, warnings);
	}
	return true;
}

void Materials::addToTileset(std::string tilesetName, int itemId, TilesetCategoryType categoryType) {
	const auto definition = g_item_definitions.get(itemId);
	if (!definition) {
		return;
	}

	Tileset* tileset;
	auto _it = tilesets.find(tilesetName);
	if (_it != tilesets.end()) {
		tileset = _it->second;
	} else {
		tileset = newd Tileset(g_brushes, tilesetName);
		tilesets.insert(std::make_pair(tilesetName, tileset));
	}

	TilesetCategory* category = tileset->getCategory(categoryType);

	if (!definition.isMetaItem()) {
		Brush* brush;
		const ItemEditorData& editor_data = definition.editorData();
		if (definition.hasFlag(ItemFlag::InOtherTileset)) {
			category->brushlist.push_back(editor_data.raw_brush);
			return;
		} else if (editor_data.raw_brush == nullptr) {
			brush = ensureRawBrush(itemId);
		} else {
			brush = editor_data.raw_brush;
		}

		brush->flagAsVisible();
		category->brushlist.push_back(g_item_definitions.editorData(itemId).raw_brush);
		g_item_definitions.setFlag(itemId, ItemFlag::InOtherTileset, true);
	}
}

bool Materials::isInTileset(Item* item, std::string tilesetName) const {
	const auto definition = g_item_definitions.get(item->getID());
	if (!definition) {
		return false;
	}

	const ItemEditorData& editor_data = definition.editorData();
	return (isInTileset(editor_data.brush, tilesetName) || isInTileset(editor_data.doodad_brush, tilesetName) || isInTileset(editor_data.raw_brush, tilesetName) || isInTileset(editor_data.collection_brush, tilesetName));
}

bool Materials::isInTileset(Brush* brush, std::string tilesetName) const {
	if (!brush) {
		return false;
	}

	TilesetContainer::const_iterator tilesetiter = tilesets.find(tilesetName);
	if (tilesetiter == tilesets.end()) {
		return false;
	}
	Tileset* tileset = tilesetiter->second;

	return tileset->containsBrush(brush);
}
