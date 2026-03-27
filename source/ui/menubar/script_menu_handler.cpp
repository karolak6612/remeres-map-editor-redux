#include "script_menu_handler.h"
#include "lua/lua_script_manager.h"
#include "lua/lua_scripts_window.h"
#include "ui/main_frame.h"
#include "ui/gui.h"
#include "app/application.h"
#include <algorithm>
#include <wx/msgdlg.h>
#include <wx/aui/framemanager.h>

ScriptMenuHandler::ScriptMenuHandler(MainFrame* frame) : m_frame(frame) {
	// Connect dynamic events
	for (int i = SCRIPTS_FIRST; i <= SCRIPTS_LAST; ++i) {
		m_frame->Bind(wxEVT_COMMAND_MENU_SELECTED, &ScriptMenuHandler::OnScriptExecute, this, MAIN_FRAME_MENU + i);
	}
	for (int i = SHOW_CUSTOM_FIRST; i <= SHOW_CUSTOM_LAST; ++i) {
		m_frame->Bind(wxEVT_COMMAND_MENU_SELECTED, &ScriptMenuHandler::OnShowOverlayToggle, this, MAIN_FRAME_MENU + i);
	}
}

ScriptMenuHandler::~ScriptMenuHandler() {
	// m_frame is expected to clean up bindings
}

void ScriptMenuHandler::OnScriptsOpenFolder(wxCommandEvent& WXUNUSED(event)) {
	g_luaScripts.openScriptsFolder();
}

void ScriptMenuHandler::OnScriptsReload(wxCommandEvent& WXUNUSED(event)) {
	g_luaScripts.reloadScripts();
	RebuildScriptsMenu();
	g_gui.SetStatusText("Scripts reloaded");

	LuaScriptsWindow* scriptsWindow = LuaScriptsWindow::Get();
	if (scriptsWindow) {
		scriptsWindow->RefreshScriptList();
	}
}

void ScriptMenuHandler::OnScriptsManager(wxCommandEvent& WXUNUSED(event)) {
	wxAuiPaneInfo& pane = g_gui.aui_manager->GetPane("ScriptManager");
	if (pane.IsOk()) {
		pane.Show(!pane.IsShown());
		g_gui.aui_manager->Update();
	}
}

void ScriptMenuHandler::OnScriptExecute(wxCommandEvent& event) {
	const auto it = m_scriptMenuIds.find(event.GetId());
	if (it == m_scriptMenuIds.end()) {
		return;
	}

	g_luaScripts.setScriptEnabled(it->second, event.IsChecked());
	RebuildScriptsMenu();

	if (LuaScriptsWindow* scriptsWindow = LuaScriptsWindow::Get()) {
		scriptsWindow->RefreshScriptList();
	}
}

void ScriptMenuHandler::OnShowOverlayToggle(wxCommandEvent& event) {
	const auto it = m_overlayMenuIds.find(event.GetId());
	if (it == m_overlayMenuIds.end()) return;

	g_luaScripts.setMapOverlayShowEnabled(it->second, event.IsChecked());
	RebuildScriptsMenu();
	g_gui.RefreshView();
}

void ScriptMenuHandler::LoadScriptsMenu(wxMenu* menu) {
	m_scriptsMenu = menu;
	if (!m_scriptsMenu) {
		return;
	}

	if (m_staticMenuItemCount == 0) {
		m_staticMenuItemCount = m_scriptsMenu->GetMenuItemCount();
	}

	RebuildScriptsMenu();
}

void ScriptMenuHandler::RebuildScriptsMenu() {
	if (!m_scriptsMenu) {
		return;
	}

	while (m_scriptsMenu->GetMenuItemCount() > m_staticMenuItemCount) {
		m_scriptsMenu->Destroy(m_scriptsMenu->FindItemByPosition(m_staticMenuItemCount));
	}

	m_scriptMenuIds.clear();
	m_overlayMenuIds.clear();

	const auto& scripts = g_luaScripts.getScripts();
	const auto& shows = g_luaScripts.getMapOverlayShows();
	const size_t maxScriptCount = static_cast<size_t>(SCRIPTS_LAST - SCRIPTS_FIRST);
	const size_t maxShowCount = static_cast<size_t>(SHOW_CUSTOM_LAST - SHOW_CUSTOM_FIRST);

	if (!scripts.empty()) {
		m_scriptsMenu->AppendSeparator();
		for (size_t scriptIndex = 0; scriptIndex < scripts.size() && scriptIndex < maxScriptCount; ++scriptIndex) {
			const auto& script = scripts[scriptIndex];
			wxString label = wxString::FromUTF8(script->getDisplayName());
			wxString shortcut = wxString::FromUTF8(script->getShortcut());
			if (!shortcut.IsEmpty()) {
				label += "\t" + shortcut;
			}

			const int menuId = MAIN_FRAME_MENU + SCRIPTS_FIRST + static_cast<int>(scriptIndex);
			wxMenuItem* item = m_scriptsMenu->AppendCheckItem(
				menuId,
				label,
				wxString::FromUTF8(script->getDescription())
			);
			if (item) {
				item->Check(script->isEnabled());
			}
			m_scriptMenuIds.emplace(menuId, scriptIndex);
		}
	}

	if (!shows.empty()) {
		m_scriptsMenu->AppendSeparator();
		for (size_t showIndex = 0; showIndex < shows.size() && showIndex < maxShowCount; ++showIndex) {
			const auto& show = shows[showIndex];
			const int menuId = MAIN_FRAME_MENU + SHOW_CUSTOM_FIRST + static_cast<int>(showIndex);
			wxMenuItem* item = m_scriptsMenu->AppendCheckItem(
				menuId,
				wxString::Format("Overlay: %s", wxString::FromUTF8(show.label)),
				wxString()
			);
			if (item) {
				item->Check(g_luaScripts.isMapOverlayEnabled(show.overlayId));
			}
			m_overlayMenuIds.emplace(menuId, show.overlayId);
		}
	}
}

void ScriptMenuHandler::UpdateShowMenu(wxMenu* menu) {
	if (menu) {
		m_scriptsMenu = menu;
	}
	RebuildScriptsMenu();
}
