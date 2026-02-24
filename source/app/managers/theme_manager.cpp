#include "app/managers/theme_manager.h"
#include "app/settings.h"
#include "ui/theme.h"
#include <wx/wx.h>

void ThemeManager::ApplyTheme() {
	int rawTheme = g_settings.getInteger(Config::THEME);
	Theme::Type theme = Theme::Type::System;
	if (rawTheme >= static_cast<int>(Theme::Type::System) && rawTheme <= static_cast<int>(Theme::Type::Light)) {
		theme = static_cast<Theme::Type>(rawTheme);
	}
	Theme::setType(theme);

	wxApp* app = wxTheApp;
	if (!app) return;

#if wxCHECK_VERSION(3, 3, 0)
	switch (theme) {
		case Theme::Type::Dark:
			app->SetAppearance(wxApp::Appearance::Dark);
			break;
		case Theme::Type::Light:
			app->SetAppearance(wxApp::Appearance::Light);
			break;
		case Theme::Type::System:
		default:
			app->SetAppearance(wxApp::Appearance::System);
			break;
	}
#endif

#ifdef __WXMSW__
	#if wxCHECK_VERSION(3, 3, 0)
	// Enable dark mode support for Windows
	// Note: SetAppearance() above handles this internally in newer versions,
	// but explicit calls here ensure improved behavior on some system configurations.
	switch (theme) {
		case Theme::Type::Dark:
			app->MSWEnableDarkMode(wxApp::DarkMode_Always);
			break;
		case Theme::Type::Light:
			// "DarkMode_Never" is not available in wxWidgets 3.3.1 API.
			// Light mode is the default on MSW, so no action is required here.
			break;
		case Theme::Type::System:
		default:
			app->MSWEnableDarkMode(wxApp::DarkMode_Auto);
			break;
	}
	#endif
#endif
}
