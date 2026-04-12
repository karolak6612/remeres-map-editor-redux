//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_VERSION_MANAGER_H_
#define RME_VERSION_MANAGER_H_

#include "app/main.h"
#include "app/client_version.h"
#include "item_definitions/core/missing_item_report.h"

class VersionManager {
public:
	VersionManager();
	~VersionManager();

	void UnloadVersion();
	bool LoadVersion(ClientVersionID ver, wxString& error, std::vector<std::string>& warnings, bool force = false);

	// The current version loaded (returns CLIENT_VERSION_NONE if no version is loaded)
	const ClientVersion& GetCurrentVersion() const;
	ClientVersionID GetCurrentVersionID() const;

	// If any version is loaded at all
	bool IsVersionLoaded() const {
		return !loaded_version.empty();
	}

	ClientVersion* getLoadedVersion() const {
		return loaded_version.empty() ? nullptr : ClientVersion::get(loaded_version);
	}

	// Access the missing items report from the last version load
	const MissingItemReport& getLastMissingItems() const {
		return last_missing_items;
	}

	// Check if the last loaded version uses OTB
	bool lastLoadHasOtb() const {
		return last_load_has_otb;
	}

private:
	bool LoadDataFiles(wxString& error, std::vector<std::string>& warnings);

	ClientVersionID loaded_version;
	MissingItemReport last_missing_items;
	bool last_load_has_otb = true;
};

extern VersionManager g_version;

#endif
