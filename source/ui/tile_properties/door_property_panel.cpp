//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "ui/tile_properties/door_property_panel.h"
#include "game/complexitem.h"
#include "map/tile.h"
#include "map/map.h"
#include "ui/gui.h"

DoorPropertyPanel::DoorPropertyPanel(wxWindow* parent) :
	ItemPropertyPanel(parent) {

	door_sizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Door ID");
	door_id_spin = newd wxSpinCtrl(this, wxID_ANY);
	door_id_spin->SetRange(0, 255);
	door_sizer->Add(door_id_spin, 1, wxEXPAND | wxALL, 5);

	GetSizer()->Add(door_sizer, 0, wxEXPAND | wxALL, 5);

	door_id_spin->Bind(wxEVT_SPINCTRL, &DoorPropertyPanel::OnDoorIdChange, this);
}

DoorPropertyPanel::~DoorPropertyPanel() {
}

void DoorPropertyPanel::SetItem(Item* item, Tile* tile, Map* map) {
	ItemPropertyPanel::SetItem(item, tile, map);

	if (item && item->asDoor()) {
		Door* door = static_cast<Door*>(item);
		GetSizer()->Show(door_sizer, true);
		door_id_spin->SetValue(door->getDoorID());
		if (tile && tile->isHouseTile() && door->isRealDoor()) {
			door_id_spin->Enable(true);
		} else {
			door_id_spin->Enable(false);
		}
	} else {
		GetSizer()->Show(door_sizer, false);
	}
	Layout();
	GetParent()->Layout();
}

void DoorPropertyPanel::OnDoorIdChange(wxSpinEvent& event) {
	if (current_item && current_map && current_item->asDoor()) {
		Door* door = static_cast<Door*>(current_item);
		door->setDoorID(door_id_spin->GetValue());
		current_map->doChange();
	}
}
