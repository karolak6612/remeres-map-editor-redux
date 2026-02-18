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

#include <toml++/toml.hpp>
#include <charconv>
#include <spdlog/fmt/fmt.h>
#include "app/main.h"

#include "app/settings.h"
#include "io/filehandle.h"

#include "ui/gui.h"

#include "util/file_system.h"
#include "ui/dialog_util.h"
#include "app/client_version.h"

#include <wx/dir.h>

// Static methods to load/save

ClientVersion::VersionList ClientVersion::client_versions;
ClientVersion* ClientVersion::latest_version = nullptr;

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
				if (name.empty()) {
					continue;
				}

				ClientVersion* version = get(name);
				if (!version) {
					// If it doesn't exist, we might be adding a new custom version
					// However, for now let's just update existing ones
					continue;
				}

				// Update properties from config.toml
				version->setClientPath(wxstr((*client)["clientPath"].value_or("")));
				version->version = (*client)["version"].value_or(version->version);
				version->description = (*client)["description"].value_or(version->description);
				version->config_type = (*client)["configType"].value_or(version->config_type);
				version->metadata_file = (*client)["metadataFile"].value_or(version->metadata_file);
				version->sprites_file = (*client)["spritesFile"].value_or(version->sprites_file);
				version->is_transparent = (*client)["transparency"].value_or(version->is_transparent);
				version->is_extended = (*client)["extended"].value_or(version->is_extended);
				version->has_frame_durations = (*client)["frameDurations"].value_or(version->has_frame_durations);
				version->has_frame_groups = (*client)["frameGroups"].value_or(version->has_frame_groups);

				if (auto otbId = (*client)["otbId"].value<int>()) {
					version->otb.id = static_cast<OtbVersionID>(*otbId);
				}
				if (auto otbMajor = (*client)["otbMajor"].value<int>()) {
					version->otb.format_version = static_cast<OtbFormatVersion>(*otbMajor);
				}

				if (auto sig = (*client)["datSignature"].value<std::string>()) {
					if (!version->data_versions.empty()) {
						std::from_chars(sig->data(), sig->data() + sig->size(), version->data_versions[0].datSignature, 16);
					}
				}
				if (auto sig = (*client)["sprSignature"].value<std::string>()) {
					if (!version->data_versions.empty()) {
						std::from_chars(sig->data(), sig->data() + sig->size(), version->data_versions[0].sprSignature, 16);
					}
				}

				if (auto otbmVers = (*client)["otbmVersions"].as_array()) {
					version->map_versions_supported.clear();
					for (auto&& v : *otbmVers) {
						int ver = v.value_or(-1);
						if (ver >= 1 && ver <= 4) {
							version->map_versions_supported.push_back(static_cast<MapVersionID>(ver - 1));
						}
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
			otb.id = static_cast<OtbVersionID>(otbId);
			otb.format_version = static_cast<OtbFormatVersion>(otbMajor);
			if (otb.format_version < OTB_VERSION_1) {
				otb.format_version = OTB_VERSION_1;
			}
			if (otb.format_version > OTB_VERSION_3) {
				otb.format_version = OTB_VERSION_3;
			}

			// Data Directory
			std::string dataDirectory = client["dataDirectory"].value_or("");
			std::unique_ptr<ClientVersion> cv = std::make_unique<ClientVersion>(otb, name, wxstr(dataDirectory));

			// Use a raw pointer for configuration while we still own the unique_ptr
			ClientVersion* cv_ptr = cv.get();
			cv_ptr->version = version;
			// NO default client path here!

			// Description & Config Type
			cv_ptr->description = client["description"].value_or("");
			cv_ptr->config_type = client["configType"].value_or("");

			// OTBM Versions
			cv_ptr->preferred_map_version = MAP_OTBM_UNKNOWN;
			if (client["otbmVersions"].as_array()) {
				auto& otbmVers = *client["otbmVersions"].as_array();
				for (auto&& v : otbmVers) {
					int ver = v.value_or(-1);
					// Adjust 1-based TOML version to 0-based enum
					if (ver >= 1 && ver <= 4) {
						int enumVer = ver - 1;
						cv_ptr->map_versions_supported.push_back(static_cast<MapVersionID>(enumVer));
						if (cv_ptr->preferred_map_version == MAP_OTBM_UNKNOWN) {
							cv_ptr->preferred_map_version = static_cast<MapVersionID>(enumVer);
						}
					}
				}
			}

			// Data / Signatures
			std::string datSigStr = client["datSignature"].value_or("0");
			std::string sprSigStr = client["sprSignature"].value_or("0");

			ClientData client_data;
			if (std::from_chars(datSigStr.data(), datSigStr.data() + datSigStr.size(), client_data.datSignature, 16).ec != std::errc()) {
				client_data.datSignature = 0;
			}
			if (std::from_chars(sprSigStr.data(), sprSigStr.data() + sprSigStr.size(), client_data.sprSignature, 16).ec != std::errc()) {
				client_data.sprSignature = 0;
			}
			client_data.datFormat = getDatFormatForVersion(version);

			// OTFI Replacement Fields
			cv_ptr->metadata_file = client["metadataFile"].value_or("Tibia.dat");
			cv_ptr->sprites_file = client["spritesFile"].value_or("Tibia.spr");

			cv_ptr->is_transparent = client["transparency"].value_or(version >= 1010);
			cv_ptr->is_extended = client["extended"].value_or(version >= 860);
			cv_ptr->has_frame_durations = client["frameDurations"].value_or(version >= 1050);
			cv_ptr->has_frame_groups = client["frameGroups"].value_or(version >= 1057);

			// Update the default ClientData created in constructor instead of pushing a new one
			if (!cv_ptr->data_versions.empty()) {
				cv_ptr->data_versions[0] = client_data;
			} else {
				cv_ptr->data_versions.push_back(client_data);
			}
			cv_ptr->visible = true;

			bool isDefault = client["default"].value_or(false);
			if (isDefault) {
				latest_version = cv_ptr;
			}

			client_versions.push_back(std::move(cv));
		}
	}
}

void ClientVersion::unloadVersions() {
	client_versions.clear();
	latest_version = nullptr;
}

void ClientVersion::addVersion(std::unique_ptr<ClientVersion> version) {
	client_versions.push_back(std::move(version));
}

void ClientVersion::removeVersion(const ClientVersionID& id) {
	std::erase_if(client_versions, [&](const auto& cv) {
		return cv->getName() == id;
	});
}

DatFormat ClientVersion::getDatFormatForVersion(int version) {
	// Logic to infer DatFormat from version integer
	if (version >= 1057) {
		return DAT_FORMAT_1057;
	}
	if (version >= 1050) {
		return DAT_FORMAT_1050;
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

	for (const auto& version : client_versions) {
		toml::table client_obj;
		client_obj.insert_or_assign("name", version->getName());

		// Always save these as they might be modified or added
		client_obj.insert_or_assign("version", (int)version->version);
		client_obj.insert_or_assign("otbId", (int)version->otb.id);
		client_obj.insert_or_assign("otbMajor", (int)version->otb.format_version);
		client_obj.insert_or_assign("description", version->description);
		client_obj.insert_or_assign("configType", version->config_type);
		client_obj.insert_or_assign("metadataFile", version->metadata_file);
		client_obj.insert_or_assign("spritesFile", version->sprites_file);
		client_obj.insert_or_assign("transparency", version->is_transparent);
		client_obj.insert_or_assign("extended", version->is_extended);
		client_obj.insert_or_assign("frameDurations", version->has_frame_durations);
		client_obj.insert_or_assign("frameGroups", version->has_frame_groups);

		if (!version->data_versions.empty()) {
			client_obj.insert_or_assign("datSignature", fmt::format("{:X}", version->data_versions[0].datSignature));
			client_obj.insert_or_assign("sprSignature", fmt::format("{:X}", version->data_versions[0].sprSignature));
		}

		toml::array otbmVers;
		for (auto v : version->map_versions_supported) {
			otbmVers.push_back((int)v + 1);
		}
		client_obj.insert_or_assign("otbmVersions", std::move(otbmVers));

		// Path
		wxFileName cp = version->getClientPath();
		// If path is empty, save as empty string, don't normalize to anything else
		if (cp.IsOk() && !cp.GetFullPath().IsEmpty()) {
			cp.Normalize();
			client_obj.insert_or_assign("clientPath", nstr(cp.GetFullPath()));
		} else {
			client_obj.insert_or_assign("clientPath", "");
		}

		clients_array.push_back(std::move(client_obj));
	}

	table.insert_or_assign("clients", std::move(clients_array));
	g_settings.save();
}

// Client version class

ClientVersion::ClientVersion(OtbVersion otb, std::string versionName, wxString path) :
	otb(otb),
	version(0),
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
	data_versions.push_back(ClientData());
}

void ClientVersion::setVersion(uint32_t v) {
	version = v;
}

ClientVersion* ClientVersion::get(const ClientVersionID& id) {
	for (const auto& cv : client_versions) {
		if (cv->name == id) {
			return cv.get();
		}
	}
	return nullptr;
}

ClientVersion* ClientVersion::getBestMatch(OtbVersionID id) {
	// Try to find a default one first
	for (const auto& cv : client_versions) {
		if (cv->otb.id == id) {
			// We could check for a 'default' flag here if we wanted to be more precise
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
			if (std::ranges::find(cv->map_versions_supported, id) != cv->map_versions_supported.end()) {
				list.push_back(cv.get());
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

	// OTFI loading removed (deprecated)
	// Metadata and sprites paths are now set in loadVersionsFromTOML or defaulted.
	// We just verify they exist here.
	metadata_path = wxFileName(client_path.GetFullPath(), wxString(metadata_file));

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
	return name;
}

OtbVersionID ClientVersion::getProtocolID() const {
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
void ClientVersion::backup() {
	backup_data.version = version;
	backup_data.name = name;
	backup_data.description = description;
	backup_data.config_type = config_type;
	backup_data.metadata_file = metadata_file;
	backup_data.sprites_file = sprites_file;
	backup_data.is_transparent = is_transparent;
	backup_data.is_extended = is_extended;
	backup_data.has_frame_durations = has_frame_durations;
	backup_data.has_frame_groups = has_frame_groups;
	backup_data.client_path = client_path;
	backup_data.otb = otb;
	backup_data.data_versions = data_versions;
	backup_data.map_versions_supported = map_versions_supported;
	is_dirty = false;
}

void ClientVersion::restore() {
	version = backup_data.version;
	name = backup_data.name;
	description = backup_data.description;
	config_type = backup_data.config_type;
	metadata_file = backup_data.metadata_file;
	sprites_file = backup_data.sprites_file;
	is_transparent = backup_data.is_transparent;
	is_extended = backup_data.is_extended;
	has_frame_durations = backup_data.has_frame_durations;
	has_frame_groups = backup_data.has_frame_groups;
	client_path = backup_data.client_path;
	otb = backup_data.otb;
	data_versions = backup_data.data_versions;
	map_versions_supported = backup_data.map_versions_supported;
	is_dirty = false;
}
