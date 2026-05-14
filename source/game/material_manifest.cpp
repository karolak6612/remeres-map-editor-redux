#include "game/material_manifest.h"

#include "ext/pugixml.hpp"
#include "util/common.h"

#include <wx/dir.h>

namespace {

[[nodiscard]] std::vector<FileName> collectXmlFiles(const FileName& directory, bool recursive) {
	wxArrayString files;
	const int flags = wxDIR_FILES | (recursive ? wxDIR_DIRS : 0);
	wxDir::GetAllFiles(directory.GetFullPath(), &files, "*.xml", flags);

	std::vector<FileName> result;
	result.reserve(files.size());
	for (const auto& file : files) {
		result.emplace_back(file);
	}
	std::ranges::sort(result, [](const FileName& lhs, const FileName& rhs) {
		return lhs.GetFullPath().CmpNoCase(rhs.GetFullPath()) < 0;
	});
	return result;
}

[[nodiscard]] FileName resolveRelativePath(const FileName& baseFile, const std::string& relative) {
	wxString normalized = wxString(relative);
	normalized.Replace("\\", "/");
	FileName path(baseFile.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + normalized);
	path.Normalize(wxPATH_NORM_ALL);
	return path;
}

[[nodiscard]] FileName resolveRelativeFolder(const FileName& baseFile, const std::string& relative) {
	wxString normalized = wxString(relative);
	normalized.Replace("\\", "/");
	if (!normalized.empty() && !normalized.EndsWith("/")) {
		normalized += "/";
	}
	FileName path(baseFile.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + normalized);
	path.Normalize(wxPATH_NORM_ALL);
	return path;
}

[[nodiscard]] bool collectSectionFiles(const FileName& manifestFile, pugi::xml_node section, std::string_view sectionName, std::vector<FileName>& out, wxString& error) {
	bool loadedAny = false;
	for (pugi::xml_node includeNode = section.child("include"); includeNode; includeNode = includeNode.next_sibling("include")) {
		if (const auto fileAttribute = includeNode.attribute("file")) {
			FileName includeFile = resolveRelativePath(manifestFile, fileAttribute.as_string());
			if (!includeFile.FileExists()) {
				error = "Missing modular include file: " + includeFile.GetFullPath();
				return false;
			}
			out.push_back(includeFile);
			loadedAny = true;
			continue;
		}

		if (const auto folderAttribute = includeNode.attribute("folder")) {
			FileName folder = resolveRelativeFolder(manifestFile, folderAttribute.as_string());
			if (!folder.DirExists()) {
				error = "Missing modular include folder: " + folder.GetFullPath();
				return false;
			}
			std::vector<FileName> folderFiles = collectXmlFiles(folder, includeNode.attribute("subfolders").as_bool(false));
			loadedAny = loadedAny || !folderFiles.empty();
			out.insert(out.end(), folderFiles.begin(), folderFiles.end());
		}
	}

	if (!loadedAny) {
		error = "Modular section has no include entries: " + wxString(std::string(sectionName));
		return false;
	}
	return true;
}

} // namespace

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
			if (!collectSectionFiles(manifestFile, childNode, "borders", files.borders, error)) {
				return false;
			}
		} else if (childName == "brushes") {
			sawBrushes = true;
			if (!collectSectionFiles(manifestFile, childNode, "brushes", files.brushes, error)) {
				return false;
			}
		} else if (childName == "creatures") {
			sawCreatures = true;
			if (!collectSectionFiles(manifestFile, childNode, "creatures", files.creatures, error)) {
				return false;
			}
		} else if (childName == "items") {
			sawItems = true;
			if (!collectSectionFiles(manifestFile, childNode, "items", files.items, error)) {
				return false;
			}
		} else if (childName == "tilesets") {
			sawTilesets = true;
			if (!collectSectionFiles(manifestFile, childNode, "tilesets", files.tilesets, error)) {
				return false;
			}
		} else if (childName == "palettes") {
			sawPalettes = true;
			if (!collectSectionFiles(manifestFile, childNode, "palettes", files.palettes, error)) {
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
