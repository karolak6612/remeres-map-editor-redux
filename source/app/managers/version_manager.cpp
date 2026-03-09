//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "app/managers/version_manager.h"

#include <spdlog/spdlog.h>

#include "ui/gui.h"
#include "util/file_system.h"
#include "game/sprites.h"
#include "game/materials.h"
#include "brushes/brush.h"
#include "brushes/managers/brush_manager.h"
#include "ui/managers/loading_manager.h"
#include "ui/tool_options_window.h"
#include "item_definitions/core/asset_bundle_loader.h"
#include "item_definitions/core/item_definition_store.h"

VersionManager g_version;

namespace {
	wxFileName resolveVersionDataFile(const wxString& base_data_path, const std::initializer_list<const char*>& candidates) {
		for (const auto* candidate : candidates) {
			wxFileName path(base_data_path + wxString::FromUTF8(candidate));
			if (path.FileExists()) {
				return path;
			}
		}
		return {};
	}

	std::string displayName(const wxFileName& filename, const char* fallback_name) {
		return filename.IsOk() && !filename.GetFullPath().empty() ? filename.GetFullPath().ToStdString() : std::string(fallback_name);
	}
}

VersionManager::VersionManager() :
	loaded_version(CLIENT_VERSION_NONE) {
}

VersionManager::~VersionManager() {
	// UnloadVersion is handled explicitly by Application::Unload() during graceful shutdown.
	// Calling it here during static deinitialization causes use-after-free for singletons
	// like SpritePreloader that may have already been destructed.
}

bool VersionManager::LoadVersion(ClientVersionID version, wxString& error, std::vector<std::string>& warnings, bool force) {
	if (ClientVersion::get(version) == nullptr) {
		error = "Unsupported client version! (8)";
		return false;
	}

	if (version != loaded_version || force) {
		if (getLoadedVersion() != nullptr) {
			// There is another version loaded right now, save window layout
			g_gui.SavePerspective();
		}

		// Disable all rendering so the data is not accessed while reloading
		UnnamedRenderingLock();
		g_gui.DestroyPalettes();
		g_gui.DestroyMinimap();

		// Destroy the previous version
		UnloadVersion();

		loaded_version = version;
		if (!getLoadedVersion()->hasValidPaths()) {
			if (!getLoadedVersion()->loadValidPaths()) {
				error = "Couldn't load relevant asset files";
				loaded_version = CLIENT_VERSION_NONE;
				return false;
			}
		}

		bool ret = LoadDataFiles(error, warnings);
		if (ret) {
			g_gui.LoadPerspective();
		} else {
			loaded_version = CLIENT_VERSION_NONE;
		}

		return ret;
	}
	return true;
}

ClientVersionID VersionManager::GetCurrentVersionID() const {
	if (!loaded_version.empty()) {
		return getLoadedVersion()->getID();
	}
	return CLIENT_VERSION_NONE;
}

const ClientVersion& VersionManager::GetCurrentVersion() const {
	assert(!loaded_version.empty());
	return *getLoadedVersion();
}

bool VersionManager::LoadDataFiles(wxString& error, std::vector<std::string>& warnings) {
	FileName data_path = getLoadedVersion()->getDataPath();
	FileName extension_path = FileSystem::GetExtensionsDirectory();

	g_gui.gfx.client_version = getLoadedVersion();

	// OTFI loading removed. Metadata and sprite files are configured via clients.toml or defaults in ClientVersion.

	g_loading.CreateLoadBar("Loading asset files");
	g_loading.SetLoadDone(0, "Loading canonical asset bundle...");

	wxFileName metadata_path = getLoadedVersion()->getMetadataPath();
	wxFileName sprites_path = getLoadedVersion()->getSpritesPath();
	wxString base_data_path = data_path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
	const wxFileName items_xml_path = resolveVersionDataFile(base_data_path, { "items/items.xml", "items.xml" });
	const wxFileName items_otb_path = resolveVersionDataFile(base_data_path, { "items.otb" });
	const wxFileName monsters_xml_path = resolveVersionDataFile(base_data_path, { "creatures/monsters.xml" });
	const wxFileName npcs_xml_path = resolveVersionDataFile(base_data_path, { "creatures/npcs.xml" });
	const wxFileName creatures_xml_path = resolveVersionDataFile(base_data_path, { "creatures.xml" });
	const wxFileName materials_xml_path = resolveVersionDataFile(base_data_path, { "materials/materials.xml", "materials.xml" });

	AssetLoadRequest asset_request;
	asset_request.mode = getLoadedVersion()->getItemDefinitionMode();
	asset_request.client_version = getLoadedVersion();
	asset_request.dat_path = metadata_path;
	asset_request.spr_path = sprites_path;
	asset_request.otb_path = items_otb_path;
	asset_request.xml_path = items_xml_path;

	if (asset_request.xml_path.GetFullPath().empty()) {
		error = "Couldn't locate items.xml for the selected client version.";
		g_loading.DestroyLoadBar();
		UnloadVersion();
		return false;
	}

	AssetBundle bundle;
	AssetBundleLoader bundle_loader;
	if (!bundle_loader.load(asset_request, bundle, error, warnings)) {
		error = "Couldn't load canonical asset bundle: " + error;
		g_loading.DestroyLoadBar();
		UnloadVersion();
		return false;
	}

	g_loading.SetLoadDone(20, "Installing graphics...");
	if (!bundle_loader.install(bundle, g_gui.gfx, g_item_definitions, error, warnings)) {
		error = "Couldn't install canonical asset bundle: " + error;
		g_loading.DestroyLoadBar();
		UnloadVersion();
		return false;
	}

	g_loading.SetLoadDone(35, "Loading creatures...");
	wxString creatures_error;
	if (monsters_xml_path.FileExists() || npcs_xml_path.FileExists()) {
		if (monsters_xml_path.FileExists() && !g_creatures.importXMLFromOT(monsters_xml_path, creatures_error, warnings)) {
			warnings.push_back(std::format("Couldn't load {}: {}", monsters_xml_path.GetFullPath().ToStdString(), creatures_error.ToStdString()));
		}
		if (npcs_xml_path.FileExists() && !g_creatures.importXMLFromOT(npcs_xml_path, creatures_error, warnings)) {
			warnings.push_back(std::format("Couldn't load {}: {}", npcs_xml_path.GetFullPath().ToStdString(), creatures_error.ToStdString()));
		}
	} else if (creatures_xml_path.FileExists()) {
		if (!g_creatures.loadFromXML(creatures_xml_path, true, creatures_error, warnings)) {
			warnings.push_back(std::format("Couldn't load {}: {}", creatures_xml_path.GetFullPath().ToStdString(), creatures_error.ToStdString()));
		}
	} else {
		warnings.push_back("Couldn't locate creatures XML for the selected client version.");
	}

	// g_loading.SetLoadDone(45, "Loading user creatures.xml ...");
	// {
	// 	FileName cdb = getLoadedVersion()->getLocalDataPath();
	// 	cdb.SetFullName("creatures.xml");
	// 	wxString nerr;
	// 	wxArrayString nwarn;
	// 	if (!g_creatures.loadFromXML(cdb, false, nerr, nwarn)) {
	// 		warnings.push_back("Couldn't load user creatures.xml: " + nerr);
	// 		spdlog::error("Couldn't load user creatures.xml: {}", nerr.ToStdString());
	// 	}
	// 	for (const auto& warn : nwarn) {
	// 		warnings.push_back(warn);
	// 		spdlog::warn("User creature XML warning: {}", warn.ToStdString());
	// 	}
	// }

	g_loading.SetLoadDone(50, "Loading materials...");
	if (materials_xml_path.FileExists()) {
		if (!g_materials.loadMaterials(materials_xml_path, error, warnings)) {
			warnings.push_back("Couldn't load materials XML: " + std::string(error.mb_str()));
		}
	} else {
		warnings.push_back("Couldn't locate materials XML for the selected client version.");
	}

	g_loading.SetLoadDone(70, "Loading extensions...");
	if (!g_materials.loadExtensions(extension_path, error, warnings)) {
		warnings.push_back("Couldn't load extensions: " + std::string(error.mb_str()));
		spdlog::warn("Couldn't load extensions: {}", error.ToStdString());
	}

	g_loading.SetLoadDone(70, "Finishing...");
	g_brushes.init();
	g_materials.createOtherTileset();

	g_loading.DestroyLoadBar();
	return true;
}

void VersionManager::UnloadVersion() {
	UnnamedRenderingLock();
	g_gui.gfx.clear();
	if (g_gui.tool_options) {
		g_gui.tool_options->Clear();
	}
	g_brush_manager.Clear();

	if (!loaded_version.empty()) {
		g_materials.clear();
		g_brushes.clear();
		g_item_definitions.clear();
		g_gui.gfx.clear();

		// FileName cdb = getLoadedVersion()->getLocalDataPath();
		// cdb.SetFullName("creatures.xml");
		// g_creatures.saveToXML(cdb); // Disabled to prevent dual source of truth
		g_creatures.clear();

		loaded_version = CLIENT_VERSION_NONE;
	}
}
