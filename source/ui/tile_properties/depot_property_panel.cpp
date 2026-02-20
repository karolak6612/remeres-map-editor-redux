//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "ui/tile_properties/depot_property_panel.h"
#include "game/complexitem.h"
#include "game/town.h"
#include "map/map.h"

DepotPropertyPanel::DepotPropertyPanel(wxWindow* parent) :
	ItemPropertyPanel(parent) {

	depot_id_label = newd wxStaticText(this, wxID_ANY, "Depot Town:");
	GetSizer()->Add(depot_id_label, 0, wxLEFT | wxTOP, 5);

	depot_id_field = newd wxChoice(this, wxID_ANY);
	GetSizer()->Add(depot_id_field, 0, wxEXPAND | wxALL, 5);

	depot_id_field->Bind(wxEVT_CHOICE, &DepotPropertyPanel::OnDepotIdChange, this);
}

DepotPropertyPanel::~DepotPropertyPanel() {
}

void DepotPropertyPanel::SetItem(Item* item, Tile* tile, Map* map) {
	ItemPropertyPanel::SetItem(item, tile, map);

	Depot* depot = item ? dynamic_cast<Depot*>(item) : nullptr;
	if (depot && map) {
		depot_id_label->Show();
		depot_id_field->Show();

		depot_id_field->Clear();
		const Towns& towns = map->towns;
		int to_select_index = 0;

		if (towns.count() > 0) {
			bool found = false;
			for (TownMap::const_iterator town_iter = towns.begin();
				 town_iter != towns.end();
				 ++town_iter) {
				if (town_iter->second->getID() == depot->getDepotID()) {
					found = true;
				}
				depot_id_field->Append(wxstr(town_iter->second->getName()), (void*)(intptr_t)(town_iter->second->getID()));
				if (!found) {
					++to_select_index;
				}
			}
			if (!found) {
				if (depot->getDepotID() != 0) {
					depot_id_field->Append("Undefined Town (id:" + i2ws(depot->getDepotID()) + ")", (void*)(intptr_t)(depot->getDepotID()));
				}
			}
		}
		depot_id_field->Append("No Town", (void*)(intptr_t)(0));
		if (depot->getDepotID() == 0) {
			to_select_index = depot_id_field->GetCount() - 1;
		}
		depot_id_field->SetSelection(to_select_index);
	} else {
		depot_id_label->Hide();
		depot_id_field->Hide();
	}
	Layout();
}

void DepotPropertyPanel::OnDepotIdChange(wxCommandEvent& event) {
	if (current_item && current_map) {
		Depot* depot = dynamic_cast<Depot*>(current_item);
		if (depot) {
			int new_depotid = (int)(intptr_t)depot_id_field->GetClientData(depot_id_field->GetSelection());
			depot->setDepotID(new_depotid);
			current_map->doChange();
		}
	}
}
