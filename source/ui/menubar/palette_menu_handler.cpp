#include "ui/menubar/palette_menu_handler.h"
#include "app/main.h"
#include "ui/gui.h"
#include "palette/house/house_palette.h"

namespace {
void ShowDynamicPaletteMenuRemoved() {
	wxMessageBox("Dynamic palettes are defined by palettes.xml. Use the palette dropdown; fixed Terrain/Doodad/Raw menu targets are no longer valid.", "Dynamic Palettes", wxOK | wxICON_INFORMATION, g_gui.root);
}
}

PaletteMenuHandler::PaletteMenuHandler(MainFrame* frame, MainMenuBar* menubar) :
	frame(frame), menubar(menubar) {
}

PaletteMenuHandler::~PaletteMenuHandler() {
}

void PaletteMenuHandler::OnNewPalette(wxCommandEvent& event) {
	g_gui.NewPalette();
}

void PaletteMenuHandler::OnSelectTerrainPalette(wxCommandEvent& WXUNUSED(event)) {
	ShowDynamicPaletteMenuRemoved();
}

void PaletteMenuHandler::OnSelectDoodadPalette(wxCommandEvent& WXUNUSED(event)) {
	ShowDynamicPaletteMenuRemoved();
}

void PaletteMenuHandler::OnSelectItemPalette(wxCommandEvent& WXUNUSED(event)) {
	ShowDynamicPaletteMenuRemoved();
}

void PaletteMenuHandler::OnSelectCollectionPalette(wxCommandEvent& WXUNUSED(event)) {
	ShowDynamicPaletteMenuRemoved();
}

void PaletteMenuHandler::OnSelectHousePalette(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SelectPalettePage("House");
}

void PaletteMenuHandler::OnSelectCreaturePalette(wxCommandEvent& WXUNUSED(event)) {
	ShowDynamicPaletteMenuRemoved();
}

void PaletteMenuHandler::OnSelectWaypointPalette(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SelectPalettePage("Waypoint");
}

void PaletteMenuHandler::OnSelectRawPalette(wxCommandEvent& WXUNUSED(event)) {
	ShowDynamicPaletteMenuRemoved();
}
