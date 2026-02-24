#include "app/managers/crash_recovery_manager.h"
#include "app/main.h"
#include "util/common.h"
#include "util/file_system.h"
#include "ui/dialog_util.h"
#include "ui/gui.h"
#include <fstream>
#include <cstdio>
#include <string>

bool CrashRecoveryManager::CheckAndRecover() {
	FileName save_failed_file = FileSystem::GetLocalDataDirectory();
	save_failed_file.SetName(".saving.txt");

	if (save_failed_file.FileExists()) {
		std::ifstream f(nstr(save_failed_file.GetFullPath()).c_str(), std::ios::in);

		std::string backup_otbm, backup_house, backup_spawn;

		std::getline(f, backup_otbm);
		std::getline(f, backup_house);
		std::getline(f, backup_spawn);

		// Remove the file
		f.close();
		std::remove(nstr(save_failed_file.GetFullPath()).c_str());

		// Query file retrieval if possible
		if (!backup_otbm.empty()) {
			long ret = DialogUtil::PopupDialog(
				"Editor Crashed",
				wxString(
					"IMPORTANT! THE EDITOR CRASHED WHILE SAVING!\n\n"
					"Do you want to recover the lost map? (it will be opened immediately):\n"
				) << wxstr(backup_otbm)
				  << "\n"
				  << wxstr(backup_house) << "\n"
				  << wxstr(backup_spawn) << "\n",
				wxYES | wxNO
			);

			if (ret == wxID_YES) {
				// Recover if the user so wishes
				// Original code assumed backup_otbm has at least 1 char to strip (newline?)
				// We replicate the logic: substr(0, size - 1)
				if (backup_otbm.empty()) {
					return false;
				}

				std::string final_otbm = backup_otbm.substr(0, backup_otbm.size() - 1);

				std::remove(final_otbm.c_str());
				std::rename(backup_otbm.c_str(), final_otbm.c_str());

				if (!backup_house.empty()) {
					std::string final_house = backup_house.substr(0, backup_house.size() - 1);
					std::remove(final_house.c_str());
					std::rename(backup_house.c_str(), final_house.c_str());
				}
				if (!backup_spawn.empty()) {
					std::string final_spawn = backup_spawn.substr(0, backup_spawn.size() - 1);
					std::remove(final_spawn.c_str());
					std::rename(backup_spawn.c_str(), final_spawn.c_str());
				}

				// Load the map
				g_gui.LoadMap(wxstr(final_otbm));
				return true;
			}
		}
	}
	return false;
}
