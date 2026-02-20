//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "ui/tile_properties/teleport_property_panel.h"
#include "game/complexitem.h"
#include "map/tile.h"
#include "map/map.h"
#include "ui/properties/teleport_service.h"
#include "ui/gui.h"

TeleportPropertyPanel::TeleportPropertyPanel(wxWindow* parent) :
	ItemPropertyPanel(parent) {

	dest_sizer = newd wxBoxSizer(wxHORIZONTAL);
	dest_sizer->Add(newd wxStaticText(this, wxID_ANY, "Destination:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

	x_spin = newd wxSpinCtrl(this, wxID_ANY);
	x_spin->SetRange(0, 65535);
	dest_sizer->Add(x_spin, 1, wxEXPAND | wxRIGHT, 2);

	y_spin = newd wxSpinCtrl(this, wxID_ANY);
	y_spin->SetRange(0, 65535);
	dest_sizer->Add(y_spin, 1, wxEXPAND | wxRIGHT, 2);

	z_spin = newd wxSpinCtrl(this, wxID_ANY);
	z_spin->SetRange(0, 15);
	dest_sizer->Add(z_spin, 0, wxEXPAND);

	GetSizer()->Add(dest_sizer, 0, wxEXPAND | wxALL, 5);

	x_spin->Bind(wxEVT_SPINCTRL, &TeleportPropertyPanel::OnDestChange, this);
	y_spin->Bind(wxEVT_SPINCTRL, &TeleportPropertyPanel::OnDestChange, this);
	z_spin->Bind(wxEVT_SPINCTRL, &TeleportPropertyPanel::OnDestChange, this);
}

TeleportPropertyPanel::~TeleportPropertyPanel() {
}

void TeleportPropertyPanel::SetItem(Item* item, Tile* tile, Map* map) {
	ItemPropertyPanel::SetItem(item, tile, map);

	if (item && item->asTeleport()) {
		Teleport* teleport = static_cast<Teleport*>(item);
		GetSizer()->Show(dest_sizer, true);
		x_spin->SetValue(teleport->getX());
		y_spin->SetValue(teleport->getY());
		z_spin->SetValue(teleport->getZ());
	} else {
		GetSizer()->Show(dest_sizer, false);
	}
	Layout();
	GetParent()->Layout();
}

void TeleportPropertyPanel::OnDestChange(wxSpinEvent& event) {
	if (current_item && current_map && current_item->asTeleport()) {
		Teleport* teleport = static_cast<Teleport*>(current_item);
		Position dest(x_spin->GetValue(), y_spin->GetValue(), z_spin->GetValue());
		teleport->setDestination(dest);
		current_map->doChange();
	}
}
