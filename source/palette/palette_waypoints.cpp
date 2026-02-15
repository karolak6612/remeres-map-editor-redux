//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
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
#include <wx/textdlg.h>

class WaypointListCtrl : public wxListCtrl {
public:
	WaypointListCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
		: wxListCtrl(parent, id, pos, size, style | wxLC_VIRTUAL) { }

	void UpdateWaypoints(Map* map) {
		cache.clear();
		if (map) {
			for (auto& pair : map->waypoints) {
				cache.push_back(pair.second);
			}
		}
		SetItemCount(cache.size());
		Refresh();
	}

	wxString OnGetItemText(long item, long column) const override {
		if (item >= 0 && item < (long)cache.size()) {
			return wxstr(cache[item]->name);
		}
		return "";
	}

	std::vector<Waypoint*> cache;
};

WaypointPalettePanel::WaypointPalettePanel(wxWindow* parent, wxWindowID id) :
	PalettePanel(parent, id),
	map(nullptr) {
	wxSizer* sidesizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Waypoints");

	waypoint_list = newd WaypointListCtrl(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), PALETTE_WAYPOINT_LISTBOX, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_NO_HEADER);
	waypoint_list->InsertColumn(0, "UNNAMED", wxLIST_FORMAT_LEFT, 200);
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

	// Virtual list doesn't support inline editing easily
	// Bind(wxEVT_LIST_BEGIN_LABEL_EDIT, &WaypointPalettePanel::OnBeginEditWaypointLabel, this, PALETTE_WAYPOINT_LISTBOX);
	// Bind(wxEVT_LIST_END_LABEL_EDIT, &WaypointPalettePanel::OnEditWaypointLabel, this, PALETTE_WAYPOINT_LISTBOX);
	Bind(wxEVT_LIST_ITEM_SELECTED, &WaypointPalettePanel::OnClickWaypoint, this, PALETTE_WAYPOINT_LISTBOX);
	Bind(wxEVT_LIST_ITEM_ACTIVATED, &WaypointPalettePanel::OnEditWaypointLabel, this, PALETTE_WAYPOINT_LISTBOX);
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
	long item = waypoint_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	g_brush_manager.waypoint_brush->setWaypoint(
		item == -1 ? nullptr : map->waypoints.getWaypoint(nstr(waypoint_list->GetItemText(item)))
	);
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
	/*
	if (wxTextCtrl* tc = waypoint_list->GetEditControl()) {
		Waypoint* wp = map->waypoints.getWaypoint(nstr(tc->GetValue()));
		if (wp && wp->pos == Position()) {
			if (map->getTile(wp->pos)) {
				map->getTileL(wp->pos)->decreaseWaypointCount();
			}
			map->waypoints.removeWaypoint(wp->name);
		}
	}
	*/

	if (!map) {
		waypoint_list->Enable(false);
		add_waypoint_button->Enable(false);
		remove_waypoint_button->Enable(false);
		waypoint_list->UpdateWaypoints(nullptr);
		return;
	}

	waypoint_list->Enable(true);
	add_waypoint_button->Enable(true);
	remove_waypoint_button->Enable(true);

	waypoint_list->UpdateWaypoints(map);
}

void WaypointPalettePanel::OnClickWaypoint(wxListEvent& event) {
	if (!map) {
		return;
	}

	std::string wpname = nstr(event.GetText());
	Waypoint* wp = map->waypoints.getWaypoint(wpname);
	if (wp) {
		g_gui.SetScreenCenterPosition(wp->pos);
		g_brush_manager.waypoint_brush->setWaypoint(wp);
	}
}

void WaypointPalettePanel::OnBeginEditWaypointLabel(wxListEvent& event) {
	// Virtual list doesn't support this
}

void WaypointPalettePanel::OnEditWaypointLabel(wxListEvent& event) {
	long index = event.GetIndex();
	wxString oldName = waypoint_list->GetItemText(index);

	wxTextEntryDialog dlg(this, "Enter new name for waypoint:", "Rename Waypoint", oldName);
	if (dlg.ShowModal() != wxID_OK) return;

	std::string wpname = nstr(dlg.GetValue());
	std::string oldwpname = nstr(oldName);
	Waypoint* wp = map->waypoints.getWaypoint(oldwpname);

	if (wpname.empty()) {
		map->waypoints.removeWaypoint(oldwpname);
		g_gui.RefreshPalettes();
	} else if (wp) {
		if (wpname == oldwpname) {
			; // do nothing
		} else {
			if (map->waypoints.getWaypoint(wpname)) {
				g_gui.SetStatusText("There already is a waypoint with this name.");
			} else {
				Waypoint* nwp = newd Waypoint(*wp);
				nwp->name = wpname;

				Waypoint* rwp = map->waypoints.getWaypoint(oldwpname);
				if (rwp) {
					if (map->getTile(rwp->pos)) {
						map->getTileL(rwp->pos)->decreaseWaypointCount();
					}
					map->waypoints.removeWaypoint(rwp->name);
				}

				map->waypoints.addWaypoint(nwp);
				g_brush_manager.waypoint_brush->setWaypoint(nwp);

				refresh_timer.Start(300, true);
			}
		}
	}
}

void WaypointPalettePanel::OnClickAddWaypoint(wxCommandEvent& event) {
	if (map) {
		map->waypoints.addWaypoint(newd Waypoint());
		// Refresh needed to show new item
		refresh_timer.Start(10, true);
	}
}

void WaypointPalettePanel::OnClickRemoveWaypoint(wxCommandEvent& event) {
	if (!map) {
		return;
	}

	long item = waypoint_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (item != -1) {
		Waypoint* wp = map->waypoints.getWaypoint(nstr(waypoint_list->GetItemText(item)));
		if (wp) {
			if (map->getTile(wp->pos)) {
				map->getTileL(wp->pos)->decreaseWaypointCount();
			}
			map->waypoints.removeWaypoint(wp->name);
		}
		waypoint_list->DeleteItem(item);
		refresh_timer.Start(300, true);
	}
}
