#ifndef RME_SCRIPT_MENU_HANDLER_H_
#define RME_SCRIPT_MENU_HANDLER_H_

#include "ui/gui_ids.h"
#include <wx/menu.h>
#include <wx/event.h>
#include <unordered_map>

class MainFrame;
class wxMenu;

class ScriptMenuHandler {
public:
	ScriptMenuHandler(MainFrame* frame);
	~ScriptMenuHandler();

	void OnScriptsOpenFolder(wxCommandEvent& event);
	void OnScriptsReload(wxCommandEvent& event);
	void OnScriptsManager(wxCommandEvent& event);
	void OnScriptExecute(wxCommandEvent& event);
	void OnShowOverlayToggle(wxCommandEvent& event);

	void LoadScriptsMenu(wxMenu* menu);
	void UpdateShowMenu(wxMenu* menu);

private:
	void RebuildScriptsMenu();

	MainFrame* m_frame;
	wxMenu* m_scriptsMenu = nullptr;
	size_t m_staticMenuItemCount = 0;
	std::unordered_map<int, size_t> m_scriptMenuIds;
	std::unordered_map<int, std::string> m_overlayMenuIds;
};

#endif
