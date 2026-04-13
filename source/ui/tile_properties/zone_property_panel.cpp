//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include "brushes/managers/brush_manager.h"
#include "editor/action.h"
#include "editor/action_queue.h"
#include "editor/editor.h"
#include "map/map.h"
#include "map/tile.h"
#include "map/tile_operations.h"
#include "ui/gui.h"
#include "ui/tile_properties/zone_property_panel.h"

ZonePropertyPanel::ZonePropertyPanel(wxWindow* parent) :
	wxPanel(parent, wxID_ANY) {
	auto* main_sizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Zones");

	assigned_zones = newd wxListBox(main_sizer->GetStaticBox(), wxID_ANY);
	main_sizer->Add(assigned_zones, wxSizerFlags(1).Expand().Border(wxALL, 5));

	zone_input = newd wxComboBox(main_sizer->GetStaticBox(), wxID_ANY);
	zone_input->SetToolTip("Select an existing zone or type a new one");
	main_sizer->Add(zone_input, wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, 5));

	auto* button_row = newd wxBoxSizer(wxHORIZONTAL);
	add_zone_button = newd wxButton(main_sizer->GetStaticBox(), wxID_ANY, "Add");
	remove_zone_button = newd wxButton(main_sizer->GetStaticBox(), wxID_ANY, "Remove");
	clear_zones_button = newd wxButton(main_sizer->GetStaticBox(), wxID_ANY, "Clear");
	button_row->Add(add_zone_button, wxSizerFlags(1).Border(wxRIGHT, 5));
	button_row->Add(remove_zone_button, wxSizerFlags(1).Border(wxRIGHT, 5));
	button_row->Add(clear_zones_button, wxSizerFlags(1));
	main_sizer->Add(button_row, wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, 5));

	SetSizer(main_sizer);

	add_zone_button->Bind(wxEVT_BUTTON, &ZonePropertyPanel::OnAddZone, this);
	remove_zone_button->Bind(wxEVT_BUTTON, &ZonePropertyPanel::OnRemoveZone, this);
	clear_zones_button->Bind(wxEVT_BUTTON, &ZonePropertyPanel::OnClearZones, this);
	assigned_zones->Bind(wxEVT_LISTBOX, &ZonePropertyPanel::OnAssignedZoneSelected, this);
}

ZonePropertyPanel::~ZonePropertyPanel() {
}

void ZonePropertyPanel::SetTile(Tile* tile, Map* map) {
	current_tile = tile;
	current_map = map;
	RefreshZoneLists();
}

void ZonePropertyPanel::RefreshZoneLists() {
	assigned_zones->Clear();
	zone_input->Clear();

	if (current_map) {
		for (const auto& [name, id] : current_map->zones) {
			(void)id;
			zone_input->Append(wxstr(name));
		}
	}

	if (current_tile && current_map) {
		for (uint16_t zone_id : current_tile->getZones()) {
			const std::string zone_name = current_map->zones.findName(zone_id);
			assigned_zones->Append(wxstr(zone_name.empty() ? std::to_string(zone_id) : zone_name));
		}
	}

	remove_zone_button->Enable(assigned_zones->GetCount() > 0);
	clear_zones_button->Enable(assigned_zones->GetCount() > 0);
}

void ZonePropertyPanel::OnAddZone(wxCommandEvent& event) {
	if (!current_tile || !current_map) {
		return;
	}

	const std::string zone_name = nstr(zone_input->GetValue());
	if (zone_name.empty()) {
		return;
	}

	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) {
		return;
	}

	const uint16_t zone_id = current_map->zones.ensureZone(zone_name);
	if (zone_id == 0) {
		return;
	}
	auto new_tile = TileOperations::deepCopy(current_tile, *current_map);
	if (new_tile->hasZone(zone_id)) {
		return;
	}
	new_tile->addZone(zone_id);

	const Position position = new_tile->getPosition();
	auto action = editor->actionQueue->createAction(ACTION_CHANGE_PROPERTIES);
	action->addChange(std::make_unique<Change>(std::move(new_tile)));
	editor->addAction(std::move(action));

	current_map = &editor->map;
	current_tile = editor->map.getTile(position);
	g_brush_manager.SetSelectedZone(zone_name);
	zone_input->SetValue(wxstr(zone_name));
	RefreshZoneLists();
	g_gui.RefreshPalettes();
	g_gui.RefreshView();
}

void ZonePropertyPanel::OnRemoveZone(wxCommandEvent& event) {
	if (!current_tile || !current_map || assigned_zones->GetSelection() == wxNOT_FOUND) {
		return;
	}

	const std::string zone_name = nstr(assigned_zones->GetStringSelection());
	const auto zone_id = current_map->zones.findId(zone_name);
	if (!zone_id) {
		return;
	}

	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) {
		return;
	}

	auto new_tile = TileOperations::deepCopy(current_tile, *current_map);
	new_tile->removeZone(*zone_id);

	const Position position = new_tile->getPosition();
	auto action = editor->actionQueue->createAction(ACTION_CHANGE_PROPERTIES);
	action->addChange(std::make_unique<Change>(std::move(new_tile)));
	editor->addAction(std::move(action));

	current_map = &editor->map;
	current_tile = editor->map.getTile(position);
	zone_input->SetValue(wxstr(zone_name));
	RefreshZoneLists();
	g_gui.RefreshView();
}

void ZonePropertyPanel::OnClearZones(wxCommandEvent& event) {
	if (!current_tile || !current_map || current_tile->getZones().empty()) {
		return;
	}

	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) {
		return;
	}

	auto new_tile = TileOperations::deepCopy(current_tile, *current_map);
	new_tile->clearZones();

	const Position position = new_tile->getPosition();
	auto action = editor->actionQueue->createAction(ACTION_CHANGE_PROPERTIES);
	action->addChange(std::make_unique<Change>(std::move(new_tile)));
	editor->addAction(std::move(action));

	current_map = &editor->map;
	current_tile = editor->map.getTile(position);
	RefreshZoneLists();
	g_gui.RefreshView();
}

void ZonePropertyPanel::OnAssignedZoneSelected(wxCommandEvent& event) {
	if (assigned_zones->GetSelection() == wxNOT_FOUND) {
		return;
	}

	zone_input->SetValue(assigned_zones->GetStringSelection());
	g_brush_manager.SetSelectedZone(nstr(zone_input->GetValue()));
}
