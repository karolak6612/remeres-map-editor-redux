#include "game/material_manifest.h"

#include "ext/pugixml.hpp"
#include "game/material_include_resolver.h"
#include "util/common.h"

bool LoadMaterialManifestFiles(const FileName& manifestFile, MaterialManifestFiles& files, wxString& error) {
	files = {};

	pugi::xml_document doc;
	const pugi::xml_parse_result result = doc.load_file(manifestFile.GetFullPath().mb_str());
	if (!result) {
		error = "Could not parse materials manifest: " + manifestFile.GetFullPath();
		return false;
	}

	pugi::xml_node root = doc.child("materials");
	if (!root) {
		error = "Invalid materials manifest root: " + manifestFile.GetFullPath();
		return false;
	}

	bool sawBorders = false;
	bool sawBrushes = false;
	bool sawCreatures = false;
	bool sawItems = false;
	bool sawTilesets = false;
	bool sawPalettes = false;

	for (pugi::xml_node childNode = root.first_child(); childNode; childNode = childNode.next_sibling()) {
		const std::string childName = as_lower_str(childNode.name());
		if (childName == "borders") {
			sawBorders = true;
			if (!MaterialIncludeResolver::collectSectionFiles(manifestFile, childNode, "borders", files.borders, error)) {
				return false;
			}
		} else if (childName == "brushes") {
			sawBrushes = true;
			if (!MaterialIncludeResolver::collectSectionFiles(manifestFile, childNode, "brushes", files.brushes, error)) {
				return false;
			}
		} else if (childName == "creatures") {
			sawCreatures = true;
			if (!MaterialIncludeResolver::collectSectionFiles(manifestFile, childNode, "creatures", files.creatures, error)) {
				return false;
			}
		} else if (childName == "items") {
			sawItems = true;
			if (!MaterialIncludeResolver::collectSectionFiles(manifestFile, childNode, "items", files.items, error)) {
				return false;
			}
		} else if (childName == "tilesets") {
			sawTilesets = true;
			if (!MaterialIncludeResolver::collectSectionFiles(manifestFile, childNode, "tilesets", files.tilesets, error)) {
				return false;
			}
		} else if (childName == "palettes") {
			sawPalettes = true;
			if (!MaterialIncludeResolver::collectSectionFiles(manifestFile, childNode, "palettes", files.palettes, error)) {
				return false;
			}
		} else if (childName == "tileset") {
			error = "Legacy <tileset> nodes are not supported by the modular material loader.";
			return false;
		}
	}

	if (!sawBorders || !sawBrushes || !sawCreatures || !sawItems || !sawTilesets || !sawPalettes) {
		error = "Modular materials.xml must define borders, brushes, creatures, items, tilesets, and palettes sections.";
		return false;
	}
	return true;
}
