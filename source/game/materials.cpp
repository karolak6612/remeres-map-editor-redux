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
#include <memory>
#include <format>
#include <algorithm>

#include "editor/editor.h"
#include "game/items.h"
#include "game/creatures.h"

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
	// Unique pointers handle memory cleanup automatically.
	tilesets.clear();
	extensions.clear();
}

const MaterialsExtensionList& Materials::getExtensions() {
	return extensions;
}

std::vector<MaterialsExtension*> Materials::getExtensionsByVersion(const ClientVersionID& version_id) {
	std::vector<MaterialsExtension*> ret_list;
	for (const auto& extension : extensions) {
		if (extension->isForVersion(version_id)) {
			ret_list.push_back(extension.get());
		}
	}
	return ret_list;
}

bool Materials::loadMaterials(const FileName& identifier, wxString& error, std::vector<std::string>& warnings) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(identifier.GetFullPath().mb_str());
	if (!result) {
		warnings.push_back(std::format("Could not open {} (file not found or syntax error)", identifier.GetFullName().ToStdString()));
		return false;
	}

	pugi::xml_node node = doc.child("materials");
	if (!node) {
		warnings.push_back(std::format("{}: Invalid rootheader.", identifier.GetFullName().ToStdString()));
		return false;
	}

	unserializeMaterials(identifier, node, error, warnings);
	return true;
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
			warnings.push_back(std::format("Could not open {} (file not found or syntax error)", filename.ToStdString()));
			continue;
		}

		pugi::xml_node extensionNode = doc.child("materialsextension");
		if (!extensionNode) {
			warnings.push_back(std::format("{}: Invalid rootheader.", filename.ToStdString()));
			continue;
		}

		pugi::xml_attribute attribute;
		if (!(attribute = extensionNode.attribute("name"))) {
			warnings.push_back(std::format("{}: Couldn't read extension name.", filename.ToStdString()));
			continue;
		}

		const std::string& extensionName = attribute.as_string();
		if (!(attribute = extensionNode.attribute("author"))) {
			warnings.push_back(std::format("{}: Couldn't read extension author.", filename.ToStdString()));
			continue;
		}

		const std::string& extensionAuthor = attribute.as_string();
		if (!(attribute = extensionNode.attribute("description"))) {
			warnings.push_back(std::format("{}: Couldn't read extension description.", filename.ToStdString()));
			continue;
		}

		const std::string& extensionDescription = attribute.as_string();
		if (extensionName.empty() || extensionAuthor.empty() || extensionDescription.empty()) {
			warnings.push_back(std::format("{}: Couldn't read extension attributes (name, author, description).", filename.ToStdString()));
			continue;
		}

		std::string extensionUrl = extensionNode.attribute("url").as_string();
		extensionUrl.erase(std::remove(extensionUrl.begin(), extensionUrl.end(), '\''), extensionUrl.end());

		std::string extensionAuthorLink = extensionNode.attribute("authorurl").as_string();
		extensionAuthorLink.erase(std::remove(extensionAuthorLink.begin(), extensionAuthorLink.end(), '\''), extensionAuthorLink.end());

		auto materialExtension = std::make_unique<MaterialsExtension>(extensionName, extensionAuthor, extensionDescription);
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
				materialExtension->version_list.erase(duplicate, materialExtension->version_list.end());
				duplicate = std::unique(materialExtension->version_list.begin(), materialExtension->version_list.end());
			}
		} else {
			warnings.push_back(std::format("{}: Extension is not available for any version.", filename.ToStdString()));
		}

		bool isForVersion = materialExtension->isForVersion(g_version.GetCurrentVersionID());
		extensions.push_back(std::move(materialExtension));

		if (isForVersion) {
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
		if (childName == "include") {
			if (!(attribute = childNode.attribute("file"))) {
				continue;
			}

			FileName includeName;
			includeName.SetPath(filename.GetPath());
			includeName.SetFullName(wxString(attribute.as_string(), wxConvUTF8));

			wxString subError;
			if (!loadMaterials(includeName, subError, warnings)) {
				warnings.push_back(std::format("Error while loading file \"{}\": {}", includeName.GetFullName().ToStdString(), subError.ToStdString()));
			}
		} else if (childName == "metaitem") {
			g_items.loadMetaItem(childNode);
		} else if (childName == "border") {
			g_brushes.unserializeBorder(childNode, warnings);
			if (warning.size()) {
				warnings.push_back(std::format("materials.xml: {}", warning.ToStdString()));
			}
		} else if (childName == "brush") {
			g_brushes.unserializeBrush(childNode, warnings);
			if (warning.size()) {
				warnings.push_back(std::format("materials.xml: {}", warning.ToStdString()));
			}
		} else if (childName == "tileset") {
			unserializeTileset(childNode, warnings);
		}
	}
	return true;
}

static RAWBrush* ensureRawBrush(ItemType& it) {
	if (it.raw_brush == nullptr) {
		auto raw_brush = std::make_unique<RAWBrush>(it.id);
		it.raw_brush = raw_brush.get();
		it.has_raw = true;
		g_brushes.addBrush(std::move(raw_brush));
	}
	return it.raw_brush;
}

void Materials::createOtherTileset() {
	Tileset* others = nullptr;
	Tileset* npc_tileset = nullptr;

	if (auto it = tilesets.find("Others"); it != tilesets.end()) {
		others = it->second.get();
		others->clear();
	} else {
		auto ptr = std::make_unique<Tileset>(g_brushes, "Others");
		others = ptr.get();
		tilesets.emplace("Others", std::move(ptr));
	}

	if (auto it = tilesets.find("NPCs"); it != tilesets.end()) {
		npc_tileset = it->second.get();
		npc_tileset->clear();
	} else {
		auto ptr = std::make_unique<Tileset>(g_brushes, "NPCs");
		npc_tileset = ptr.get();
		tilesets.emplace("NPCs", std::move(ptr));
	}

	// There should really be an iterator to do this
	for (int32_t id = 0; id <= g_items.getMaxID(); ++id) {
		ItemType& it = g_items[id];
		if (it.id == 0) {
			continue;
		}

		if (!it.isMetaItem()) {
			Brush* brush;
			if (it.in_other_tileset) {
				others->getCategory(TILESET_RAW)->brushlist.push_back(it.raw_brush);
				continue;
			} else if (it.raw_brush == nullptr) {
				brush = ensureRawBrush(it);
			} else if (!it.has_raw) {
				brush = it.raw_brush;
			} else {
				continue;
			}

			brush->flagAsVisible();
			others->getCategory(TILESET_RAW)->brushlist.push_back(it.raw_brush);
			it.in_other_tileset = true;
		}
	}

	for (auto& [id, type] : g_creatures) {
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
		tileset = it->second.get();
	}

	if (!tileset) {
		auto ptr = std::make_unique<Tileset>(g_brushes, name);
		tileset = ptr.get();
		if (it != tilesets.end()) {
			it->second = std::move(ptr);
		} else {
			tilesets.emplace(name, std::move(ptr));
		}
	}

	for (pugi::xml_node childNode = node.first_child(); childNode; childNode = childNode.next_sibling()) {
		tileset->loadCategory(childNode, warnings);
	}
	return true;
}

void Materials::addToTileset(std::string tilesetName, int itemId, TilesetCategoryType categoryType) {
	ItemType& it = g_items[itemId];

	if (it.id == 0) {
		return;
	}

	Tileset* tileset = nullptr;
	auto _it = tilesets.find(tilesetName);
	if (_it != tilesets.end()) {
		tileset = _it->second.get();
	} else {
		auto ptr = std::make_unique<Tileset>(g_brushes, tilesetName);
		tileset = ptr.get();
		tilesets.emplace(tilesetName, std::move(ptr));
	}

	TilesetCategory* category = tileset->getCategory(categoryType);

	if (!it.isMetaItem()) {
		Brush* brush;
		if (it.in_other_tileset) {
			category->brushlist.push_back(it.raw_brush);
			return;
		} else if (it.raw_brush == nullptr) {
			brush = ensureRawBrush(it);
		} else {
			brush = it.raw_brush;
		}

		brush->flagAsVisible();
		category->brushlist.push_back(it.raw_brush);
		it.in_other_tileset = true;
	}
}

bool Materials::isInTileset(Item* item, std::string tilesetName) const {
	const ItemType& it = g_items[item->getID()];

	return it.id != 0 && (isInTileset(it.brush, tilesetName) || isInTileset(it.doodad_brush, tilesetName) || isInTileset(it.raw_brush, tilesetName)) || isInTileset(it.collection_brush, tilesetName);
}

bool Materials::isInTileset(Brush* brush, std::string tilesetName) const {
	if (!brush) {
		return false;
	}

	auto it = tilesets.find(tilesetName);
	if (it == tilesets.end()) {
		return false;
	}
	Tileset* tileset = it->second.get();

	return tileset->containsBrush(brush);
}
