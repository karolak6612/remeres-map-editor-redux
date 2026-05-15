#include "palette/spawn_palette.h"

#include "app/settings.h"
#include "brushes/managers/brush_manager.h"
#include "brushes/spawn/spawn_brush.h"
#include "ui/gui.h"

SpawnPalettePanel::SpawnPalettePanel(wxWindow* parent, wxWindowID id) :
	PalettePanel(parent, id) {
	auto* sizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Spawn");
	auto* label = newd wxStaticText(sizer->GetStaticBox(), wxID_ANY, "Spawn Brush");
	sizer->Add(label, 0, wxEXPAND | wxALL, 6);
	SetSizerAndFit(sizer);
}

wxString SpawnPalettePanel::GetName() const {
	return "Spawn";
}

Brush* SpawnPalettePanel::GetSelectedBrush() const {
	return g_brush_manager.spawn_brush;
}

int SpawnPalettePanel::GetSelectedBrushSize() const {
	return g_settings.getInteger(Config::CURRENT_SPAWN_RADIUS);
}

bool SpawnPalettePanel::SelectBrush(const Brush* whatbrush) {
	return whatbrush == g_brush_manager.spawn_brush;
}

void SpawnPalettePanel::OnSwitchIn() {
	PalettePanel::OnSwitchIn();
	g_gui.SetSpawnTime(g_settings.getInteger(Config::DEFAULT_SPAWNTIME));
	g_gui.SetBrushSize(g_settings.getInteger(Config::CURRENT_SPAWN_RADIUS));
	g_gui.SelectBrushInternal(g_brush_manager.spawn_brush);
}
