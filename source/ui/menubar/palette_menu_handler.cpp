#include "ui/menubar/palette_menu_handler.h"
#include "app/main.h"
#include "ui/gui.h"
#include "palette/house/house_palette.h"

PaletteMenuHandler::PaletteMenuHandler(MainFrame* frame, MainMenuBar* menubar) :
	frame(frame), menubar(menubar) {
}

PaletteMenuHandler::~PaletteMenuHandler() {
}

void PaletteMenuHandler::OnNewPalette(wxCommandEvent& event) {
	g_gui.NewPalette();
}

void PaletteMenuHandler::OnShowPalette(wxCommandEvent& event) {
	g_gui.ShowPalette();
}

void PaletteMenuHandler::OnSelectHousePalette(wxCommandEvent& event) {
	g_gui.SelectPalettePage("House");
}

void PaletteMenuHandler::OnSelectWaypointPalette(wxCommandEvent& event) {
	g_gui.SelectPalettePage("Waypoint");
}
