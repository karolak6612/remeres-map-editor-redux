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
#include "rendering/core/sprite_database.h"
#include "ui/tool_options_window.h"

VersionManager g_version;

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
	FileName client_path = getLoadedVersion()->getClientPath();
	FileName extension_path = FileSystem::GetExtensionsDirectory();

	g_gui.sprite_database.client_version = getLoadedVersion();

	// OTFI loading removed. Metadata and sprite files are configured via clients.toml or defaults in ClientVersion.

	g_loading.CreateLoadBar("Loading asset files");
	g_loading.SetLoadDone(0, "Loading metadata file...");

	wxFileName metadata_path = getLoadedVersion()->getMetadataPath();
	if (!g_gui.sprite_database.loadSpriteMetadata(metadata_path, error, warnings)) {
		error = "Couldn't load metadata: " + error;
		g_loading.DestroyLoadBar();
		UnloadVersion();
		return false;
	}

	g_loading.SetLoadDone(10, "Loading sprites file...");

	wxFileName sprites_path = getLoadedVersion()->getSpritesPath();
	if (!g_gui.sprite_database.loadSpriteData(sprites_path, error, warnings)) {
		error = "Couldn't load sprites: " + error;
		g_loading.DestroyLoadBar();
		UnloadVersion();
		return false;
	}

	g_loading.SetLoadDone(20, "Loading items.otb file...");
	wxString base_data_path = data_path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
	if (!g_items.loadFromOtb(base_data_path + "items.otb", error, warnings)) {
		error = "Couldn't load items.otb: " + error;
		g_loading.DestroyLoadBar();
		UnloadVersion();
		return false;
	}

	g_loading.SetLoadDone(30, "Loading items.xml ...");
	if (!g_items.loadFromGameXml(base_data_path + "items.xml", error, warnings)) {
		warnings.push_back(std::format("Couldn't load items.xml: {}", error.ToStdString()));
	}

	g_loading.SetLoadDone(45, "Loading creatures.xml ...");
	if (!g_creatures.loadFromXML(base_data_path + "creatures.xml", true, error, warnings)) {
		warnings.push_back(std::format("Couldn't load creatures.xml: {}", error.ToStdString()));
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

	g_loading.SetLoadDone(50, "Loading materials.xml ...");
	if (!g_materials.loadMaterials(base_data_path + "materials.xml", error, warnings)) {
		warnings.push_back("Couldn't load materials.xml: " + std::string(error.mb_str()));
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
	g_gui.sprite_database.clear();
	if (g_gui.tool_options) {
		g_gui.tool_options->Clear();
	}
	g_brush_manager.Clear();

	if (!loaded_version.empty()) {
		g_materials.clear();
		g_brushes.clear();
		g_items.clear();
		g_gui.sprite_database.clear();

		// FileName cdb = getLoadedVersion()->getLocalDataPath();
		// cdb.SetFullName("creatures.xml");
		// g_creatures.saveToXML(cdb); // Disabled to prevent dual source of truth
		g_creatures.clear();

		loaded_version = CLIENT_VERSION_NONE;
	}
}
