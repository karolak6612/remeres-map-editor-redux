//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "ui/tile_properties/spawn_property_panel.h"
#include "game/spawn.h"
#include "map/tile.h"
#include "map/map.h"
#include "ui/gui.h"

SpawnPropertyPanel::SpawnPropertyPanel(wxWindow* parent) :
	CustomPropertyPanel(parent), current_spawn(nullptr), current_tile(nullptr), current_map(nullptr) {

	wxBoxSizer* main_sizer = newd wxBoxSizer(wxVERTICAL);

	radius_sizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Spawn Radius");
	radius_spin = newd wxSpinCtrl(this, wxID_ANY);
	radius_spin->SetRange(1, 99);
	radius_sizer->Add(radius_spin, 1, wxEXPAND | wxALL, 5);

	main_sizer->Add(radius_sizer, 0, wxEXPAND | wxALL, 5);
	SetSizer(main_sizer);

	radius_spin->Bind(wxEVT_SPINCTRL, &SpawnPropertyPanel::OnRadiusChange, this);
}

SpawnPropertyPanel::~SpawnPropertyPanel() {
}

void SpawnPropertyPanel::SetItem(Item* item, Tile* tile, Map* map) {
	// Not used for items
	SetSpawn(nullptr, tile, map);
}

void SpawnPropertyPanel::SetSpawn(Spawn* spawn, Tile* tile, Map* map) {
	current_spawn = spawn;
	current_tile = tile;
	current_map = map;

	if (spawn) {
		GetSizer()->Show(radius_sizer, true);
		radius_spin->SetValue(spawn->getSize());
	} else {
		GetSizer()->Show(radius_sizer, false);
	}
	Layout();
	GetParent()->Layout();
}

void SpawnPropertyPanel::OnRadiusChange(wxSpinEvent& event) {
	if (current_spawn && current_map) {
		current_spawn->setSize(radius_spin->GetValue());
		current_map->doChange();

		// Refresh GUI selection overlays
		g_gui.RefreshView();
	}
}
