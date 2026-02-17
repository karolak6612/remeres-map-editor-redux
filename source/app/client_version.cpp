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

#include <toml++/toml.h>
#include "app/main.h"

#include "app/settings.h"
#include "io/filehandle.h"

#include "ui/gui.h"

#include "util/file_system.h"
#include "ui/dialog_util.h"
#include "app/client_version.h"

#include <wx/dir.h>
#include <toml++/toml.h>

// Static methods to load/save

ClientVersion::VersionList ClientVersion::client_versions;
ClientVersion* ClientVersion::latest_version = nullptr;
ClientVersion::OtbMap ClientVersion::otb_versions;

void ClientVersion::loadVersions() {
	// Clean up old stuff
	ClientVersion::unloadVersions();

	loadVersionsFromTOML("clients.toml");

	// Assign a default if there isn't one.
	if (!latest_version && !client_versions.empty()) {
		latest_version = client_versions.front().get();
	}

	// Load the user-configured data directory paths from g_settings
	toml::table& table = g_settings.getTable();
	if (auto clients = table["clients"].as_array()) {
		for (auto&& node : *clients) {
			if (auto client = node.as_table()) {
				std::string name = (*client)["name"].value_or("");
				std::string path = (*client)["path"].value_or("");
				if (!name.empty() && !path.empty()) {
					ClientVersion* version = get(name);
					if (version) {
						version->setClientPath(wxstr(path));
					}
				}
			}
		}
	}
}

void ClientVersion::loadVersionsFromTOML(const std::string& configName) {
	wxFileName file_to_load;

	wxFileName exec_dir_client_toml;
	exec_dir_client_toml.Assign(FileSystem::GetExecDirectory());
	exec_dir_client_toml.SetFullName(configName);

	wxFileName data_dir_client_toml;
	data_dir_client_toml.Assign(FileSystem::GetDataDirectory());
	data_dir_client_toml.SetFullName(configName);

	wxFileName work_dir_client_toml;
	work_dir_client_toml.Assign(FileSystem::GetFoundDataDirectory());
	work_dir_client_toml.SetFullName(configName);

	file_to_load = exec_dir_client_toml;
	if (!file_to_load.FileExists()) {
		file_to_load = data_dir_client_toml;
		if (!file_to_load.FileExists()) {
			file_to_load = work_dir_client_toml;
			if (!file_to_load.FileExists()) {
				file_to_load.Clear();
			}
		}
	}

	if (!file_to_load.FileExists()) {
		wxLogError(wxString() + "Could not load " + configName + ", editor will NOT be able to load any client data files.\n" + "Checked paths:\n" + exec_dir_client_toml.GetFullPath() + "\n" + data_dir_client_toml.GetFullPath() + "\n" + work_dir_client_toml.GetFullPath());
		return;
	}

	toml::table config;
	try {
		config = toml::parse_file(file_to_load.GetFullPath().ToStdString());
	} catch (const toml::parse_error& err) {
		wxLogError("Parsing failed: %s", err.description());
		return;
	}

	if (config["clients"].as_array()) {
		auto& clients = *config["clients"].as_array();
		for (auto&& elem : clients) {
			if (!elem.is_table()) {
				continue;
			}
			toml::table& client = *elem.as_table();

			int version = client["version"].value_or(0);
			std::string name = client["name"].value_or("");

			// OTB Info
			int otbId = client["otbId"].value_or(0);
			int otbMajor = client["otbMajor"].value_or(0);

			OtbVersion otb;
			otb.name = name;
			otb.id = static_cast<ClientVersionID>(otbId);
			otb.format_version = static_cast<OtbFormatVersion>(otbMajor);
			if (otb.format_version < OTB_VERSION_1) {
				otb.format_version = OTB_VERSION_1;
			}
			if (otb.format_version > OTB_VERSION_3) {
				otb.format_version = OTB_VERSION_3;
			}

			// Data Directory
			std::string dataDirectory = client["dataDirectory"].value_or("");

			std::unique_ptr<ClientVersion> cv = std::unique_ptr<ClientVersion>(newd ClientVersion(otb, name, wxstr(dataDirectory)));
			cv->setClientPath(cv->getDataPath());

			// Description & Config Type
			cv->description = client["description"].value_or("");
			cv->config_type = client["configType"].value_or("");

			// OTBM Versions
			cv->preferred_map_version = MAP_OTBM_UNKNOWN;
			if (client["otbmVersions"].as_array()) {
				auto& otbmVers = *client["otbmVersions"].as_array();
				for (auto&& v : otbmVers) {
					int ver = v.value_or(-1);
					// Adjust 1-based TOML version to 0-based enum
					if (ver >= 1 && ver <= 4) {
						int enumVer = ver - 1;
						cv->map_versions_supported.push_back(static_cast<MapVersionID>(enumVer));
						if (cv->preferred_map_version == MAP_OTBM_UNKNOWN) {
							cv->preferred_map_version = static_cast<MapVersionID>(enumVer);
						}
					}
				}
			}

			// Data / Signatures
			std::string datSigStr = client["datSignature"].value_or("0");
			std::string sprSigStr = client["sprSignature"].value_or("0");

			ClientData client_data;
			client_data.datSignature = std::strtoul(datSigStr.c_str(), nullptr, 16);
			client_data.sprSignature = std::strtoul(sprSigStr.c_str(), nullptr, 16);
			client_data.datFormat = getDatFormatForVersion(version);

			// OTFI Replacement Fields
			cv->metadata_file = client["metadataFile"].value_or("Tibia.dat");
			cv->sprites_file = client["spritesFile"].value_or("Tibia.spr");

			cv->is_transparent = client["transparency"].value_or(version >= 1010);
			cv->is_extended = client["extended"].value_or(version >= 860);
			cv->has_frame_durations = client["frameDurations"].value_or(version >= 1050);
			cv->has_frame_groups = client["frameGroups"].value_or(version >= 1057);

			cv->data_versions.push_back(client_data);
			cv->visible = true;

			bool isDefault = client["default"].value_or(false);
			if (isDefault) {
				latest_version = cv.get();
			}

			client_versions.push_back(std::move(cv));
		}
	}
}

void ClientVersion::unloadVersions() {
	client_versions.clear();
	latest_version = nullptr;
	otb_versions.clear();
}

DatFormat ClientVersion::getDatFormatForVersion(int version) {
	// Logic to infer DatFormat from version integer
	if (version >= 1077) {
		return DAT_FORMAT_1057;
	}
	if (version >= 1010) {
		return DAT_FORMAT_1010;
	}
	if (version >= 960) {
		return DAT_FORMAT_96;
	}
	if (version >= 860) {
		return DAT_FORMAT_86;
	}
	if (version >= 780) {
		return DAT_FORMAT_78;
	}
	if (version >= 750) {
		return DAT_FORMAT_755;
	}
	if (version >= 710) {
		return DAT_FORMAT_74;
	}

	return DAT_FORMAT_UNKNOWN;
}

bool ClientVersion::isDefaultPath() const {
	wxFileName clientPath = getClientPath();
	wxFileName dataPath = getDataPath();

	// Normalize both to ensure matching comparison and clean strings
	clientPath.Normalize();
	dataPath.Normalize();

	return clientPath.GetFullPath() == dataPath.GetFullPath();
}

void ClientVersion::saveVersions() {
	toml::table& table = g_settings.getTable();
	toml::array clients_array;

	bool hasOverrides = false;
	for (const auto& version : client_versions) {
		// Only save if the path is actually different from the internal data/ directory
		if (!version->isDefaultPath()) {
			toml::table client_obj;
			client_obj.insert_or_assign("name", version->getName());
			// Use normalized path for saving
			wxFileName cp = version->getClientPath();
			cp.Normalize();
			client_obj.insert_or_assign("path", nstr(cp.GetFullPath()));
			clients_array.push_back(std::move(client_obj));
			hasOverrides = true;
		}
	}

	if (hasOverrides) {
		table.insert_or_assign("clients", std::move(clients_array));
	} else {
		table.erase("clients");
	}
	g_settings.save();
}

// Client version class

ClientVersion::ClientVersion(OtbVersion otb, std::string versionName, wxString path) :
	otb(otb),
	name(versionName),
	visible(false),
	is_transparent(false),
	is_extended(false),
	has_frame_durations(false),
	has_frame_groups(false),
	preferred_map_version(MAP_OTBM_UNKNOWN),
	data_path(path) {
	////
	// Default transparency check (can be updated later or in load)
}

ClientVersion* ClientVersion::get(ClientVersionID id) {
	for (const auto& cv : client_versions) {
		if (cv->getID() == id) {
			return cv.get();
		}
	}
	return nullptr;
}

ClientVersion* ClientVersion::get(std::string id) {
	for (const auto& cv : client_versions) {
		if (cv->getName() == id) {
			return cv.get();
		}
	}
	return nullptr;
}

ClientVersionList ClientVersion::getAll() {
	ClientVersionList l;
	for (const auto& cv : client_versions) {
		l.push_back(cv.get());
	}
	return l;
}

ClientVersionList ClientVersion::getAllVisible() {
	ClientVersionList l;
	for (const auto& cv : client_versions) {
		if (cv->isVisible()) {
			l.push_back(cv.get());
		}
	}
	return l;
}

ClientVersionList ClientVersion::getAllForOTBMVersion(MapVersionID id) {
	ClientVersionList list;
	for (const auto& cv : client_versions) {
		if (cv->isVisible()) {
			for (const auto& v : cv->map_versions_supported) {
				if (v == id) {
					list.push_back(cv.get());
					break;
				}
			}
		}
	}
	return list;
}

ClientVersion* ClientVersion::getLatestVersion() {
	return latest_version;
}

FileName ClientVersion::getDataPath() const {
	wxString basePath = FileSystem::GetDataDirectory();
	if (!wxFileName(basePath).DirExists()) {
		basePath = FileSystem::GetFoundDataDirectory();
	}
	return basePath + data_path + FileName::GetPathSeparator();
}

FileName ClientVersion::getLocalDataPath() const {
	FileName f = FileSystem::GetLocalDataDirectory() + data_path + FileName::GetPathSeparator();
	f.Mkdir(0755, wxPATH_MKDIR_FULL);
	return f;
}

bool ClientVersion::hasValidPaths() {
	if (!client_path.DirExists()) {
		return false;
	}

	wxDir dir(client_path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR));
	wxString otfi_file;

	// OTFI loading removed (deprecated)
	// Metadata and sprites paths are now set in loadVersionsFromTOML or defaulted.
	// We just verify they exist here.
	metadata_path = wxFileName(client_path.GetFullPath(), wxString(metadata_file));
	sprites_path = wxFileName(client_path.GetFullPath(), wxString(sprites_file));

	if (!metadata_path.FileExists() || !sprites_path.FileExists()) {
		// Fallback to "Tibia.dat" / "Tibia.spr" if the configured files don't exist
		// This maintains some backward compatibility if the toml config is slightly off but files are standard
		metadata_path = wxFileName(client_path.GetFullPath(), wxString(ASSETS_NAME) + ".dat");
		sprites_path = wxFileName(client_path.GetFullPath(), wxString(ASSETS_NAME) + ".spr");
	}

	if (!metadata_path.FileExists() || !sprites_path.FileExists()) {
		return false;
	}

	if (!g_settings.getInteger(Config::CHECK_SIGNATURES)) {
		return true;
	}

	// Peek the version of the files
	FileReadHandle dat_file(static_cast<const char*>(metadata_path.GetFullPath().mb_str()));
	if (!dat_file.isOk()) {
		wxLogError("Could not open metadata file.");
		return false;
	}

	uint32_t datSignature;
	dat_file.getU32(datSignature);
	dat_file.close();

	FileReadHandle spr_file(static_cast<const char*>(sprites_path.GetFullPath().mb_str()));
	if (!spr_file.isOk()) {
		wxLogError("Could not open sprites file.");
		return false;
	}

	uint32_t sprSignature;
	spr_file.getU32(sprSignature);
	spr_file.close();

	for (const auto& clientData : data_versions) {
		if (clientData.sprSignature == sprSignature && clientData.datSignature == datSignature) {
			return true;
		}
	}

	wxString message = "Signatures are incorrect.\n";
	message << "Metadata signature: %X\n";
	message << "Sprites signature: %X";
	wxLogError(wxString::Format(message, datSignature, sprSignature));
	return false;
}

bool ClientVersion::loadValidPaths() {
	while (!hasValidPaths()) {
		wxString message = "Could not locate metadata and/or sprite files, please navigate to your client assets %s installation folder.\n";
		message << "Attempted metadata file: %s\n";
		message << "Attempted sprites file: %s\n";

		DialogUtil::PopupDialog("Error", wxString::Format(message, name, metadata_path.GetFullPath(), sprites_path.GetFullPath()), wxOK);

		wxString dirHelpText("Select assets directory.");
		wxDirDialog file_dlg(nullptr, dirHelpText, "", wxDD_DIR_MUST_EXIST);
		int ok = file_dlg.ShowModal();
		if (ok == wxID_CANCEL) {
			return false;
		}

		client_path.Assign(file_dlg.GetPath() + FileName::GetPathSeparator());
	}

	ClientVersion::saveVersions();

	return true;
}

DatFormat ClientVersion::getDatFormatForSignature(uint32_t signature) const {
	for (std::vector<ClientData>::const_iterator iter = data_versions.begin(); iter != data_versions.end(); ++iter) {
		if (iter->datSignature == signature) {
			return iter->datFormat;
		}
	}

	return DAT_FORMAT_UNKNOWN;
}

std::string ClientVersion::getName() const {
	return name;
}

ClientVersionID ClientVersion::getID() const {
	return otb.id;
}

bool ClientVersion::isVisible() const {
	return visible;
}

void ClientVersion::setClientPath(const FileName& dir) {
	client_path.Assign(dir);
}

MapVersionID ClientVersion::getPrefferedMapVersionID() const {
	return preferred_map_version;
}

OtbVersion ClientVersion::getOTBVersion() const {
	return otb;
}

ClientVersionList ClientVersion::getExtensionsSupported() const {
	return extension_versions;
}

ClientVersionList ClientVersion::getAllVersionsSupportedForClientVersion(ClientVersion* clientVersion) {
	ClientVersionList versionList;
	for (const auto& versionEntry : client_versions) {
		ClientVersion* version = versionEntry.get();
		for (ClientVersion* checkVersion : version->getExtensionsSupported()) {
			if (clientVersion == checkVersion) {
				versionList.push_back(version);
			}
		}
	}
	std::sort(versionList.begin(), versionList.end(), VersionComparisonPredicate);
	return versionList;
}
