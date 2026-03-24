#include "script_menu_handler.h"
#include "lua/lua_script_manager.h"
#include "lua/lua_scripts_window.h"
#include "ui/main_frame.h"
#include "ui/gui.h"
#include "app/application.h"
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
	if (m_scriptsMenu) {
		LoadScriptsMenu(m_scriptsMenu);
	}
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
	int scriptIndex = event.GetId() - MAIN_FRAME_MENU - SCRIPTS_FIRST;
	const auto& scripts = g_luaScripts.getScripts();

	if (scriptIndex >= 0 && scriptIndex < (int)scripts.size()) {
		LuaScript* script = scripts[scriptIndex].get();
		if (!g_luaScripts.executeScript(script)) {
			wxMessageBox(
				wxString("Script error:\n") + g_luaScripts.getLastError(),
				"Lua Script Error",
				wxOK | wxICON_ERROR
			);
		}
	}
}

void ScriptMenuHandler::OnShowOverlayToggle(wxCommandEvent& event) {
	int showIndex = event.GetId() - MAIN_FRAME_MENU - SHOW_CUSTOM_FIRST;
	const auto& shows = g_luaScripts.getMapOverlayShows();
	if (showIndex >= 0 && showIndex < static_cast<int>(shows.size())) {
		const auto& showItem = shows[showIndex];
		g_luaScripts.setMapOverlayShowEnabled(showItem.overlayId, event.IsChecked());
		g_gui.RefreshView();
	}
}

void ScriptMenuHandler::LoadScriptsMenu(wxMenu* menu) {
	m_scriptsMenu = menu;
	if (!m_scriptsMenu) return;

	// Find the last separator. Any items after it are dynamic script entries.
	int lastSeparatorIdx = -1;
	for (size_t i = 0; i < m_scriptsMenu->GetMenuItemCount(); ++i) {
		wxMenuItem* item = m_scriptsMenu->FindItemByPosition(i);
		if (item && item->IsSeparator()) {
			lastSeparatorIdx = static_cast<int>(i);
		}
	}

	// If no separator found, we'll start appending from the end or a reasonable default.
	// But according to menubar.xml, there should be at least two.
	size_t clearFrom = (lastSeparatorIdx != -1) ? (lastSeparatorIdx + 1) : m_scriptsMenu->GetMenuItemCount();

	// Remove existing dynamic script items
	while (m_scriptsMenu->GetMenuItemCount() > clearFrom) {
		m_scriptsMenu->Destroy(m_scriptsMenu->FindItemByPosition(clearFrom));
	}

	const auto& scripts = g_luaScripts.getScripts();
	if (!scripts.empty()) {
		int scriptIndex = 0;
		for (const auto& script : scripts) {
			if (scriptIndex >= (SCRIPTS_LAST - SCRIPTS_FIRST)) break;
			// We only show enabled scripts in the main menu
			// (Disabled scripts can still be managed in the Script Manager)
			if (!script->isEnabled()) {
				scriptIndex++; // Keep index aligned with Event ID mapping
				continue;
			}

			wxString label = wxString::FromUTF8(script->getDisplayName());
			wxString shortcut = wxString::FromUTF8(script->getShortcut());
			if (!shortcut.IsEmpty()) {
				label += "\t" + shortcut;
			}

			m_scriptsMenu->Append(
				MAIN_FRAME_MENU + SCRIPTS_FIRST + scriptIndex,
				label,
				wxString::FromUTF8(script->getDescription())
			);
			scriptIndex++;
		}
	}
}

void ScriptMenuHandler::LoadShowMenu(wxMenu* menu) {
	m_showMenu = menu;
	if (!m_showMenu) return;

	size_t total = m_showMenu->GetMenuItemCount();
	if (m_showMenuCount > 0) {
		for (size_t i = 0; i < m_showMenuCount && total > 0; ++i) {
			m_showMenu->Destroy(m_showMenu->FindItemByPosition(total - 1));
			--total;
		}
	}
	if (m_showMenuHasSeparator && total > 0) {
		m_showMenu->Destroy(m_showMenu->FindItemByPosition(total - 1));
		--total;
	}

	m_showMenuCount = 0;
	m_showMenuHasSeparator = false;

	const auto& shows = g_luaScripts.getMapOverlayShows();
	if (shows.empty()) return;

	m_showMenu->AppendSeparator();
	m_showMenuHasSeparator = true;

	const size_t maxCount = static_cast<size_t>(SHOW_CUSTOM_LAST - SHOW_CUSTOM_FIRST);
	size_t count = 0;
	for (const auto& show : shows) {
		if (count >= maxCount) break;

		wxMenuItem* item = m_showMenu->Append(
			MAIN_FRAME_MENU + SHOW_CUSTOM_FIRST + static_cast<int>(count),
			wxString::FromUTF8(show.label),
			wxString(),
			wxITEM_CHECK
		);
		if (item) {
			item->Check(show.enabled);
		}
		++count;
	}

	m_showMenuCount = count;
}

void ScriptMenuHandler::UpdateShowMenu(wxMenu* menu) {
	if (!menu || !m_showMenu) return;

	const auto& shows = g_luaScripts.getMapOverlayShows();
	const size_t maxCount = static_cast<size_t>(SHOW_CUSTOM_LAST - SHOW_CUSTOM_FIRST);
	const size_t desiredCount = std::min(shows.size(), maxCount);
	bool needsReload = m_showMenuCount != desiredCount;

	if (!needsReload) {
		for (size_t i = 0; i < desiredCount; ++i) {
			wxMenuItem* item = m_showMenu->FindItem(MAIN_FRAME_MENU + SHOW_CUSTOM_FIRST + static_cast<int>(i));
			if (!item) {
				needsReload = true;
				break;
			}
			wxString label = wxString::FromUTF8(shows[i].label);
			if (item->GetItemLabelText() != label) {
				needsReload = true;
				break;
			}
		}
	}

	if (needsReload) {
		LoadShowMenu(m_showMenu);
	} else {
		for (size_t i = 0; i < desiredCount; ++i) {
			wxMenuItem* item = m_showMenu->FindItem(MAIN_FRAME_MENU + SHOW_CUSTOM_FIRST + static_cast<int>(i));
			if (item) {
				item->Check(g_luaScripts.isMapOverlayEnabled(shows[i].overlayId));
			}
		}
	}
}
