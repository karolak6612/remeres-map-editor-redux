//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "ui/tile_properties/creature_property_panel.h"
#include "game/creature.h"
#include "map/tile.h"
#include "map/map.h"
#include "ui/gui.h"

CreaturePropertyPanel::CreaturePropertyPanel(wxWindow* parent) :
	CustomPropertyPanel(parent), current_creature(nullptr), current_tile(nullptr), current_map(nullptr) {

	wxBoxSizer* main_sizer = newd wxBoxSizer(wxVERTICAL);

	// Spawn time
	time_sizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Spawn Time");
	spawntime_spin = newd wxSpinCtrl(this, wxID_ANY);
	spawntime_spin->SetRange(10, 86400); // Max 24 hours
	time_sizer->Add(spawntime_spin, 1, wxEXPAND | wxALL, 5);
	main_sizer->Add(time_sizer, 0, wxEXPAND | wxALL, 5);

	// Direction
	dir_sizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Direction");
	direction_choice = newd wxChoice(this, wxID_ANY);
	for (Direction dir = DIRECTION_FIRST; dir <= DIRECTION_LAST; ++dir) {
		direction_choice->Append(wxstr(Creature::DirID2Name(dir)), (void*)(intptr_t)(dir));
	}
	dir_sizer->Add(direction_choice, 1, wxEXPAND | wxALL, 5);
	main_sizer->Add(dir_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

	SetSizer(main_sizer);

	spawntime_spin->Bind(wxEVT_SPINCTRL, &CreaturePropertyPanel::OnSpawnTimeChange, this);
	direction_choice->Bind(wxEVT_CHOICE, &CreaturePropertyPanel::OnDirectionChange, this);
}

CreaturePropertyPanel::~CreaturePropertyPanel() {
}

void CreaturePropertyPanel::SetItem(Item* item, Tile* tile, Map* map) {
	// Not used for items
	SetCreature(nullptr, tile, map);
}

void CreaturePropertyPanel::SetCreature(Creature* creature, Tile* tile, Map* map) {
	current_creature = creature;
	current_tile = tile;
	current_map = map;

	if (creature) {
		GetSizer()->Show(time_sizer, true);
		GetSizer()->Show(dir_sizer, true);
		spawntime_spin->SetValue(creature->getSpawnTime());
		direction_choice->SetSelection(creature->getDirection());
	} else {
		GetSizer()->Show(time_sizer, false);
		GetSizer()->Show(dir_sizer, false);
	}
	Layout();
	GetParent()->Layout();
}

void CreaturePropertyPanel::OnSpawnTimeChange(wxSpinEvent& event) {
	if (current_creature && current_map) {
		current_creature->setSpawnTime(spawntime_spin->GetValue());
		current_map->doChange();
	}
}

void CreaturePropertyPanel::OnDirectionChange(wxCommandEvent& event) {
	if (current_creature && current_map) {
		int new_dir = (int)(intptr_t)direction_choice->GetClientData(direction_choice->GetSelection());
		current_creature->setDirection((Direction)new_dir);
		current_map->doChange();
		g_gui.RefreshView(); // Directions alter drawing of creatures
	}
}
