#include "app/managers/update_manager.h"
#include "app/updater.h"
#include "app/settings.h"
#include "ui/dialog_util.h"
#include <wx/wx.h>

UpdateManager::UpdateManager() {
}

UpdateManager::~UpdateManager() {
}

void UpdateManager::Initialize(wxWindow* parent) {
#ifdef _USE_UPDATER_
	if (g_settings.getInteger(Config::USE_UPDATER) == -1) {
		long ret = DialogUtil::PopupDialog(
			"Notice",
			"Do you want the editor to automatically check for updates?\n"
			"It will connect to the internet if you choose yes.\n"
			"You can change this setting in the preferences later.",
			wxYES | wxNO
		);
		if (ret == wxID_YES) {
			g_settings.setInteger(Config::USE_UPDATER, 1);
		} else {
			g_settings.setInteger(Config::USE_UPDATER, 0);
		}
	}
	if (g_settings.getInteger(Config::USE_UPDATER) == 1) {
		m_updater = std::make_unique<UpdateChecker>();
		m_updater->connect(parent);
	}
#endif
}

void UpdateManager::Unload() {
#ifdef _USE_UPDATER_
	m_updater.reset();
#endif
}
