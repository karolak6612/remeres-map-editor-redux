#ifndef RME_UI_MENUBAR_PALETTE_MENU_HANDLER_H
#define RME_UI_MENUBAR_PALETTE_MENU_HANDLER_H

#include <wx/wx.h>

class MainFrame;
class MainMenuBar;

class PaletteMenuHandler : public wxEvtHandler {
public:
	PaletteMenuHandler(MainFrame* frame, MainMenuBar* menubar);
	~PaletteMenuHandler();

	void OnNewPalette(wxCommandEvent& event);
	void OnShowPalette(wxCommandEvent& event);
	void OnSelectHousePalette(wxCommandEvent& event);
	void OnSelectSpawnPalette(wxCommandEvent& event);
	void OnSelectWaypointPalette(wxCommandEvent& event);

protected:
	MainFrame* frame;
	MainMenuBar* menubar;
};

#endif
