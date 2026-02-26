//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

// ============================================================================
// Waypoint palette

#include "app/main.h"

#include "ui/gui.h"
#include "brushes/managers/brush_manager.h"
#include "editor/hotkey_manager.h"
#include "palette/palette_waypoints.h"
#include "brushes/waypoint/waypoint_brush.h"
#include "map/map.h"
#include "util/image_manager.h"
#include "palette/controls/waypoint_list_box.h"
#include <wx/textdlg.h>
#include <wx/menu.h>

WaypointPalettePanel::WaypointPalettePanel(wxWindow* parent, wxWindowID id) :
	PalettePanel(parent, id),
	map(nullptr) {
	wxSizer* sidesizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Waypoints");

	waypoint_list = newd WaypointListBox(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), PALETTE_WAYPOINT_LISTBOX);
	sidesizer->Add(waypoint_list, 1, wxEXPAND);

	wxSizer* tmpsizer = newd wxBoxSizer(wxHORIZONTAL);
	add_waypoint_button = newd wxButton(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), PALETTE_WAYPOINT_ADD_WAYPOINT, "Add", wxDefaultPosition, wxSize(50, -1));
	add_waypoint_button->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_PLUS, wxSize(16, 16)));
	tmpsizer->Add(add_waypoint_button, 1, wxEXPAND);
	remove_waypoint_button = newd wxButton(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), PALETTE_WAYPOINT_REMOVE_WAYPOINT, "Remove", wxDefaultPosition, wxSize(70, -1));
	remove_waypoint_button->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_MINUS, wxSize(16, 16)));
	tmpsizer->Add(remove_waypoint_button, 1, wxEXPAND);
	sidesizer->Add(tmpsizer, 0, wxEXPAND);

	SetSizerAndFit(sidesizer);

	Bind(wxEVT_BUTTON, &WaypointPalettePanel::OnClickAddWaypoint, this, PALETTE_WAYPOINT_ADD_WAYPOINT);
	Bind(wxEVT_BUTTON, &WaypointPalettePanel::OnClickRemoveWaypoint, this, PALETTE_WAYPOINT_REMOVE_WAYPOINT);

	waypoint_list->Bind(wxEVT_LISTBOX, &WaypointPalettePanel::OnClickWaypoint, this);
	waypoint_list->Bind(wxEVT_CONTEXT_MENU, [this](wxContextMenuEvent& event) {
		if (waypoint_list->GetSelection() != -1) {
			wxMenu menu;
			menu.Append(wxID_EDIT, "Rename")->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_PEN_TO_SQUARE, wxSize(16, 16)));
			menu.Bind(wxEVT_MENU, [this](wxCommandEvent&) {
				RenameSelected();
			}, wxID_EDIT);
			PopupMenu(&menu);
		}
	});

	waypoint_list->Bind(wxEVT_KEY_DOWN, [this](wxKeyEvent& event) {
		if (event.GetKeyCode() == WXK_F2) {
			RenameSelected();
		} else {
			event.Skip();
		}
	});

	waypoint_list->Bind(wxEVT_LISTBOX_DCLICK, [this](wxCommandEvent& event) {
		RenameSelected();
	});
}

void WaypointPalettePanel::OnSwitchIn() {
	PalettePanel::OnSwitchIn();
}

void WaypointPalettePanel::OnSwitchOut() {
	PalettePanel::OnSwitchOut();
}

void WaypointPalettePanel::SetMap(Map* m) {
	map = m;
	this->Enable(m);
}

void WaypointPalettePanel::SelectFirstBrush() {
	// SelectWaypointBrush();
}

Brush* WaypointPalettePanel::GetSelectedBrush() const {
	wxString name = waypoint_list->GetSelectedWaypoint();
	if (name.IsEmpty()) {
		return g_brush_manager.waypoint_brush;
	}
	g_brush_manager.waypoint_brush->setWaypoint(map->waypoints.getWaypoint(nstr(name)));
	return g_brush_manager.waypoint_brush;
}

bool WaypointPalettePanel::SelectBrush(const Brush* whatbrush) {
	ASSERT(whatbrush == g_brush_manager.waypoint_brush);
	return false;
}

int WaypointPalettePanel::GetSelectedBrushSize() const {
	return 0;
}

PaletteType WaypointPalettePanel::GetType() const {
	return TILESET_WAYPOINT;
}

wxString WaypointPalettePanel::GetName() const {
	return "Waypoint Palette";
}

void WaypointPalettePanel::OnUpdate() {
	if (!map) {
		waypoint_list->Enable(false);
		add_waypoint_button->Enable(false);
		remove_waypoint_button->Enable(false);
		waypoint_list->SetWaypoints({});
		return;
	}

	waypoint_list->Enable(true);
	add_waypoint_button->Enable(true);
	remove_waypoint_button->Enable(true);

	Waypoints& waypoints = map->waypoints;
	std::vector<std::string> names;
	names.reserve(waypoints.size());

	for (const auto& [name, wp] : waypoints) {
		names.push_back(wp->name);
	}
	waypoint_list->SetWaypoints(names);
}

void WaypointPalettePanel::OnClickWaypoint(wxCommandEvent& event) {
	if (!map) {
		return;
	}

	std::string wpname = nstr(waypoint_list->GetSelectedWaypoint());
	Waypoint* wp = map->waypoints.getWaypoint(wpname);
	if (wp) {
		g_gui.SetScreenCenterPosition(wp->pos);
		g_brush_manager.waypoint_brush->setWaypoint(wp);
	}
}

void WaypointPalettePanel::RenameSelected() {
	if (!map) {
		return;
	}

	wxString oldName = waypoint_list->GetSelectedWaypoint();
	if (oldName.IsEmpty()) {
		return;
	}

	std::string oldwpname = nstr(oldName);
	Waypoint* wp = map->waypoints.getWaypoint(oldwpname);
	if (!wp) {
		return;
	}

	g_hotkeys.DisableHotkeys();

	wxTextEntryDialog dialog(this, "Enter new name for waypoint:", "Rename Waypoint", oldName);
	if (dialog.ShowModal() == wxID_OK) {
		wxString newName = dialog.GetValue();
		std::string wpname = nstr(newName);

		if (wpname == "") {
			map->waypoints.removeWaypoint(oldwpname);
			refresh_timer.Start(300, true);
		} else if (wpname != oldwpname) {
			if (map->waypoints.getWaypoint(wpname)) {
				g_gui.SetStatusText("There already is a waypoint with this name.");
			} else {
				auto nwp_ptr = std::make_unique<Waypoint>(*wp);
				nwp_ptr->name = wpname;

				if (map->getTile(wp->pos)) {
					map->getTileL(wp->pos)->decreaseWaypointCount();
				}
				map->waypoints.removeWaypoint(oldwpname);

				Waypoint* nwp = nwp_ptr.get();
				map->waypoints.addWaypoint(std::move(nwp_ptr));
				g_brush_manager.waypoint_brush->setWaypoint(nwp);

				refresh_timer.Start(300, true);
			}
		}
	}

	g_hotkeys.EnableHotkeys();
}


void WaypointPalettePanel::OnClickAddWaypoint(wxCommandEvent& event) {
	if (map) {
		std::string baseName = "Waypoint";
		std::string name = baseName;
		int counter = 1;
		while (map->waypoints.getWaypoint(name)) {
			name = baseName + " " + std::to_string(counter++);
		}

		auto wp = std::make_unique<Waypoint>();
		wp->name = name;
		map->waypoints.addWaypoint(std::move(wp));

		refresh_timer.Start(100, true);
	}
}

void WaypointPalettePanel::OnClickRemoveWaypoint(wxCommandEvent& event) {
	if (!map) {
		return;
	}

	wxString name = waypoint_list->GetSelectedWaypoint();
	if (!name.IsEmpty()) {
		Waypoint* wp = map->waypoints.getWaypoint(nstr(name));
		if (wp) {
			if (map->getTile(wp->pos)) {
				map->getTileL(wp->pos)->decreaseWaypointCount();
			}
			map->waypoints.removeWaypoint(wp->name);
		}
		refresh_timer.Start(100, true);
	}
}
