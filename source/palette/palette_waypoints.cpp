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

#include <algorithm>
#include <vector>

class WaypointListCtrl : public wxListCtrl {
public:
	WaypointListCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style) :
		wxListCtrl(parent, id, pos, size, style | wxLC_VIRTUAL) {
	}

	wxString OnGetItemText(long item, long column) const override {
		if (item >= 0 && item < static_cast<long>(m_waypoints.size())) {
			return wxstr(m_waypoints[item]);
		}
		return "";
	}

	void SetWaypoints(std::vector<std::string> waypoints) {
		m_waypoints = std::move(waypoints);
		SetItemCount(m_waypoints.size());
		Refresh();
	}

	const std::string& GetWaypointName(long item) const {
		return m_waypoints.at(item);
	}

	long FindWaypoint(const std::string& name) const {
		auto it = std::find(m_waypoints.begin(), m_waypoints.end(), name);
		if (it != m_waypoints.end()) {
			return std::distance(m_waypoints.begin(), it);
		}
		return -1;
	}

private:
	std::vector<std::string> m_waypoints;
};

WaypointPalettePanel::WaypointPalettePanel(wxWindow* parent, wxWindowID id) :
	PalettePanel(parent, id),
	map(nullptr) {
	wxSizer* sidesizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Waypoints");

	waypoint_list = newd WaypointListCtrl(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), PALETTE_WAYPOINT_LISTBOX, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_EDIT_LABELS | wxLC_NO_HEADER);
	waypoint_list->InsertColumn(0, "UNNAMED", wxLIST_FORMAT_LEFT, 200);
	sidesizer->Add(waypoint_list, 1, wxEXPAND);

	wxSizer* tmpsizer = newd wxBoxSizer(wxHORIZONTAL);
	add_waypoint_button = newd wxButton(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), PALETTE_WAYPOINT_ADD_WAYPOINT, "Add", wxDefaultPosition, wxSize(50, -1));
	add_waypoint_button->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_PLUS, wxSize(16, 16)));
	add_waypoint_button->SetToolTip("Add Waypoint");
	tmpsizer->Add(add_waypoint_button, 1, wxEXPAND);
	remove_waypoint_button = newd wxButton(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), PALETTE_WAYPOINT_REMOVE_WAYPOINT, "Remove", wxDefaultPosition, wxSize(70, -1));
	remove_waypoint_button->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_MINUS, wxSize(16, 16)));
	remove_waypoint_button->SetToolTip("Remove Waypoint");
	tmpsizer->Add(remove_waypoint_button, 1, wxEXPAND);
	sidesizer->Add(tmpsizer, 0, wxEXPAND);

	SetSizerAndFit(sidesizer);

	Bind(wxEVT_BUTTON, &WaypointPalettePanel::OnClickAddWaypoint, this, PALETTE_WAYPOINT_ADD_WAYPOINT);
	Bind(wxEVT_BUTTON, &WaypointPalettePanel::OnClickRemoveWaypoint, this, PALETTE_WAYPOINT_REMOVE_WAYPOINT);

	Bind(wxEVT_LIST_BEGIN_LABEL_EDIT, &WaypointPalettePanel::OnBeginEditWaypointLabel, this, PALETTE_WAYPOINT_LISTBOX);
	Bind(wxEVT_LIST_END_LABEL_EDIT, &WaypointPalettePanel::OnEditWaypointLabel, this, PALETTE_WAYPOINT_LISTBOX);
	Bind(wxEVT_LIST_ITEM_SELECTED, &WaypointPalettePanel::OnClickWaypoint, this, PALETTE_WAYPOINT_LISTBOX);
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
	if (item != -1) {
		WaypointListCtrl* vlist = static_cast<WaypointListCtrl*>(waypoint_list);
		g_brush_manager.waypoint_brush->setWaypoint(
			map->waypoints.getWaypoint(vlist->GetWaypointName(item))
		);
	} else {
		g_brush_manager.waypoint_brush->setWaypoint(nullptr);
	}
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
	waypoint_list->Freeze();

	if (!map) {
		static_cast<WaypointListCtrl*>(waypoint_list)->SetWaypoints({});
		waypoint_list->Enable(false);
		add_waypoint_button->Enable(false);
		remove_waypoint_button->Enable(false);
		waypoint_list->Thaw();
		return;
	}

	waypoint_list->Enable(true);
	add_waypoint_button->Enable(true);
	remove_waypoint_button->Enable(true);

	std::vector<std::string> wp_names;
	wp_names.reserve(map->waypoints.size());
	for (const auto& [name, wp] : map->waypoints) {
		wp_names.push_back(name);
	}
	// Sort to keep consistent order
	std::sort(wp_names.begin(), wp_names.end());

	static_cast<WaypointListCtrl*>(waypoint_list)->SetWaypoints(std::move(wp_names));
	waypoint_list->Thaw();
}

void WaypointPalettePanel::OnClickWaypoint(wxListEvent& event) {
	if (!map) {
		return;
	}

	WaypointListCtrl* vlist = static_cast<WaypointListCtrl*>(waypoint_list);
	std::string wpname = vlist->GetWaypointName(event.GetIndex());

	Waypoint* wp = map->waypoints.getWaypoint(wpname);
	if (wp) {
		g_gui.SetScreenCenterPosition(wp->pos);
		g_brush_manager.waypoint_brush->setWaypoint(wp);
	}
}

void WaypointPalettePanel::OnBeginEditWaypointLabel(wxListEvent& event) {
	// We need to disable all hotkeys, so we can type properly
	g_hotkeys.DisableHotkeys();
}

void WaypointPalettePanel::OnEditWaypointLabel(wxListEvent& event) {
	WaypointListCtrl* vlist = static_cast<WaypointListCtrl*>(waypoint_list);
	std::string oldwpname = vlist->GetWaypointName(event.GetIndex());
	std::string wpname = nstr(event.GetLabel());

	Waypoint* wp = map->waypoints.getWaypoint(oldwpname);

	if (event.IsEditCancelled()) {
		return;
	}

	if (wpname == "") {
		map->waypoints.removeWaypoint(oldwpname);
		OnUpdate(); // Refresh list
		g_gui.RefreshPalettes();
	} else if (wp) {
		if (wpname == oldwpname) {
			; // do nothing
		} else {
			if (map->waypoints.getWaypoint(wpname)) {
				// Already exists a waypoint with this name!
				g_gui.SetStatusText("There already is a waypoint with this name.");
				event.Veto();
				// If we were creating a new waypoint (empty name originally), and failed to rename,
				// we should probably remove it?
				// But here oldwpname is likely "" if it was just added.
				if (oldwpname == "") {
					map->waypoints.removeWaypoint(oldwpname);
					OnUpdate();
					g_gui.RefreshPalettes();
				}
			} else {
				auto nwp_ptr = std::make_unique<Waypoint>(*wp);
				nwp_ptr->name = wpname;
				Waypoint* nwp = nwp_ptr.get();

				Waypoint* rwp = map->waypoints.getWaypoint(oldwpname);
				if (rwp) {
					if (map->getTile(rwp->pos)) {
						map->getTileL(rwp->pos)->decreaseWaypointCount();
					}
					map->waypoints.removeWaypoint(rwp->name);
				}

				map->waypoints.addWaypoint(std::move(nwp_ptr));
				g_brush_manager.waypoint_brush->setWaypoint(nwp);

				// Refresh list to show new name and resorts
				OnUpdate();

				// Re-select the renamed item
				long new_idx = vlist->FindWaypoint(wpname);
				if (new_idx != -1) {
					waypoint_list->SetItemState(new_idx, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
					waypoint_list->EnsureVisible(new_idx);
				}

				// Refresh other palettes
				refresh_timer.Start(300, true);
			}
		}
	}

	if (event.IsAllowed()) {
		g_hotkeys.EnableHotkeys();
	}
}

void WaypointPalettePanel::OnClickAddWaypoint(wxCommandEvent& event) {
	if (map) {
		// Add empty waypoint
		map->waypoints.addWaypoint(std::make_unique<Waypoint>());

		OnUpdate(); // Refresh list

		// Find the empty waypoint
		WaypointListCtrl* vlist = static_cast<WaypointListCtrl*>(waypoint_list);
		long i = vlist->FindWaypoint("");
		if (i != -1) {
			waypoint_list->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
			waypoint_list->EnsureVisible(i);
			waypoint_list->EditLabel(i);
		}
		// g_gui.RefreshPalettes();
	}
}

void WaypointPalettePanel::OnClickRemoveWaypoint(wxCommandEvent& event) {
	if (!map) {
		return;
	}

	long item = waypoint_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (item != -1) {
		WaypointListCtrl* vlist = static_cast<WaypointListCtrl*>(waypoint_list);
		std::string name = vlist->GetWaypointName(item);

		Waypoint* wp = map->waypoints.getWaypoint(name);
		if (wp) {
			if (map->getTile(wp->pos)) {
				map->getTileL(wp->pos)->decreaseWaypointCount();
			}
			map->waypoints.removeWaypoint(wp->name);
		}

		OnUpdate();
		refresh_timer.Start(300, true);
	}
}
