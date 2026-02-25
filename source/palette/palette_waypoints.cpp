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
#include "util/nanovg_listbox.h"
#include "ui/theme.h"
#include "util/nvg_utils.h"
#include <wx/textdlg.h>
#include <wx/msgdlg.h>
#include <vector>
#include <string>

class WaypointListBox : public NanoVGListBox {
public:
	WaypointListBox(wxWindow* parent, wxWindowID id) :
		NanoVGListBox(parent, id, wxLB_SINGLE) {
	}

	void SetWaypoints(const std::vector<std::string>& waypoints) {
		m_waypoints = waypoints;
		SetItemCount(m_waypoints.size());
		Refresh();
	}

	wxString GetItemText(int index) const {
		if (index >= 0 && index < (int)m_waypoints.size()) {
			return wxstr(m_waypoints[index]);
		}
		return "";
	}

	void OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t index) override {
		if (index >= m_waypoints.size()) {
			return;
		}

		// Background for selection
		if (IsSelected(static_cast<int>(index))) {
			nvgFillColor(vg, NvgUtils::ToNvColor(Theme::Get(Theme::Role::Accent)));
			nvgBeginPath(vg);
			nvgRect(vg, static_cast<float>(rect.x), static_cast<float>(rect.y), static_cast<float>(rect.width), static_cast<float>(rect.height));
			nvgFill(vg);
			nvgFillColor(vg, NvgUtils::ToNvColor(Theme::Get(Theme::Role::TextOnAccent)));
		} else {
			nvgFillColor(vg, NvgUtils::ToNvColor(Theme::Get(Theme::Role::Text)));
		}

		nvgFontSize(vg, 14.0f);
		nvgFontFace(vg, "sans");
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgText(vg, static_cast<float>(rect.x + 5), rect.y + rect.height / 2.0f, m_waypoints[index].c_str(), nullptr);
	}

	int OnMeasureItem(size_t index) const override {
		return 20;
	}

private:
	std::vector<std::string> m_waypoints;
};

WaypointPalettePanel::WaypointPalettePanel(wxWindow* parent, wxWindowID id) :
	PalettePanel(parent, id),
	map(nullptr) {
	wxSizer* sidesizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Waypoints");

	waypoint_list = newd WaypointListBox(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), PALETTE_WAYPOINT_LISTBOX);
	sidesizer->Add(waypoint_list, 1, wxEXPAND);

	wxSizer* tmpsizer = newd wxBoxSizer(wxHORIZONTAL);
	add_waypoint_button = newd wxButton(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), PALETTE_WAYPOINT_ADD_WAYPOINT, "Add", wxDefaultPosition, wxSize(40, -1));
	add_waypoint_button->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_PLUS, wxSize(16, 16)));
	tmpsizer->Add(add_waypoint_button, 1, wxEXPAND);

	remove_waypoint_button = newd wxButton(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), PALETTE_WAYPOINT_REMOVE_WAYPOINT, "Remove", wxDefaultPosition, wxSize(60, -1));
	remove_waypoint_button->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_MINUS, wxSize(16, 16)));
	tmpsizer->Add(remove_waypoint_button, 1, wxEXPAND);

	rename_waypoint_button = newd wxButton(static_cast<wxStaticBoxSizer*>(sidesizer)->GetStaticBox(), wxID_ANY, "Rename", wxDefaultPosition, wxSize(60, -1));
	rename_waypoint_button->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_PEN_TO_SQUARE, wxSize(16, 16)));
	rename_waypoint_button->Bind(wxEVT_BUTTON, &WaypointPalettePanel::OnClickRenameWaypoint, this);
	tmpsizer->Add(rename_waypoint_button, 1, wxEXPAND);

	sidesizer->Add(tmpsizer, 0, wxEXPAND);

	SetSizerAndFit(sidesizer);

	Bind(wxEVT_BUTTON, &WaypointPalettePanel::OnClickAddWaypoint, this, PALETTE_WAYPOINT_ADD_WAYPOINT);
	Bind(wxEVT_BUTTON, &WaypointPalettePanel::OnClickRemoveWaypoint, this, PALETTE_WAYPOINT_REMOVE_WAYPOINT);

	// NanoVGListBox sends wxEVT_LISTBOX for selection changes
	waypoint_list->Bind(wxEVT_LISTBOX, &WaypointPalettePanel::OnClickWaypoint, this);
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
	int item = waypoint_list->GetSelection();
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
	if (!map) {
		waypoint_list->Enable(false);
		add_waypoint_button->Enable(false);
		remove_waypoint_button->Enable(false);
		rename_waypoint_button->Enable(false);
		return;
	}

	waypoint_list->Enable(true);
	add_waypoint_button->Enable(true);
	remove_waypoint_button->Enable(true);
	rename_waypoint_button->Enable(true);

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

	int item = waypoint_list->GetSelection();
	if (item != -1) {
		std::string wpname = nstr(waypoint_list->GetItemText(item));
		Waypoint* wp = map->waypoints.getWaypoint(wpname);
		if (wp) {
			g_gui.SetScreenCenterPosition(wp->pos);
			g_brush_manager.waypoint_brush->setWaypoint(wp);
		}
	}
}

void WaypointPalettePanel::OnClickAddWaypoint(wxCommandEvent& event) {
	if (!map) {
		return;
	}

	wxString name = wxGetTextFromUser("Enter waypoint name:", "New Waypoint");
	if (name.IsEmpty()) {
		return;
	}

	std::string wpname = nstr(name);
	if (map->waypoints.getWaypoint(wpname)) {
		wxMessageBox("A waypoint with that name already exists.", "Error", wxICON_ERROR);
		return;
	}

	auto nwp = std::make_unique<Waypoint>();
	nwp->name = wpname;
	map->waypoints.addWaypoint(std::move(nwp));

	OnUpdate();

	// Find and select the new waypoint
	for(int i = 0; i < (int)waypoint_list->GetItemCount(); ++i) {
		if (nstr(waypoint_list->GetItemText(i)) == wpname) {
			waypoint_list->SetSelection(i);
			waypoint_list->EnsureVisible(i);
			break;
		}
	}
}

void WaypointPalettePanel::OnClickRemoveWaypoint(wxCommandEvent& event) {
	if (!map) {
		return;
	}

	int item = waypoint_list->GetSelection();
	if (item != -1) {
		wxString name = waypoint_list->GetItemText(item);
		Waypoint* wp = map->waypoints.getWaypoint(nstr(name));
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

void WaypointPalettePanel::OnClickRenameWaypoint(wxCommandEvent& event) {
	if (!map) {
		return;
	}

	int item = waypoint_list->GetSelection();
	if (item == -1) {
		return;
	}

	wxString oldName = waypoint_list->GetItemText(item);
	wxString newName = wxGetTextFromUser("Enter new name:", "Rename Waypoint", oldName);

	if (newName.IsEmpty() || newName == oldName) {
		return;
	}

	std::string sOldName = nstr(oldName);
	std::string sNewName = nstr(newName);

	if (map->waypoints.getWaypoint(sNewName)) {
		wxMessageBox("A waypoint with that name already exists.", "Error", wxICON_ERROR);
		return;
	}

	Waypoint* wp = map->waypoints.getWaypoint(sOldName);
	if (wp) {
		auto nwp_ptr = std::make_unique<Waypoint>(*wp);
		nwp_ptr->name = sNewName;

		if (map->getTile(wp->pos)) {
			map->getTileL(wp->pos)->decreaseWaypointCount();
		}
		map->waypoints.removeWaypoint(sOldName);

		map->waypoints.addWaypoint(std::move(nwp_ptr));

		OnUpdate();
		refresh_timer.Start(300, true);

		// Reselect
		for(int i = 0; i < (int)waypoint_list->GetItemCount(); ++i) {
			if (nstr(waypoint_list->GetItemText(i)) == sNewName) {
				waypoint_list->SetSelection(i);
				waypoint_list->EnsureVisible(i);
				break;
			}
		}
	}
}
