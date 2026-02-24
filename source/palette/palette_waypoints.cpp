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
#include "util/nvg_utils.h"
#include "ui/theme.h"

#include <glad/glad.h>
#include <nanovg.h>

class WaypointListBox : public NanoVGListBox {
public:
	WaypointListBox(wxWindow* parent, wxWindowID id) :
		NanoVGListBox(parent, id, wxLB_SINGLE) {}

	void SetWaypoints(const std::vector<std::string>& wps) {
		waypoints = wps;
		SetItemCount(waypoints.size());
		Refresh();
	}

	std::string GetSelectedWaypoint() const {
		int sel = GetSelection();
		if (sel != -1 && sel < (int)waypoints.size()) {
			return waypoints[sel];
		}
		return "";
	}

	void OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t index) override {
		if (index >= waypoints.size()) {
			return;
		}

		// Text Color
		NVGcolor textColor;
		if (IsSelected(index)) {
			wxColour c = Theme::Get(Theme::Role::TextOnAccent);
			textColor = NvgUtils::ToNvColor(c);
		} else {
			wxColour c = Theme::Get(Theme::Role::Text);
			textColor = NvgUtils::ToNvColor(c);
		}

		nvgFillColor(vg, textColor);
		nvgFontSize(vg, 12.0f);
		nvgFontFace(vg, "sans");
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

		nvgText(vg, rect.x + 5, rect.y + rect.height / 2.0f, waypoints[index].c_str(), nullptr);
	}

	int OnMeasureItem(size_t index) const override {
		return 20;
	}

private:
	std::vector<std::string> waypoints;
};

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

	waypoint_list->Bind(wxEVT_LISTBOX, &WaypointPalettePanel::OnClickWaypoint, this, PALETTE_WAYPOINT_LISTBOX);
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
	std::string name = waypoint_list->GetSelectedWaypoint();
	if (name.empty()) {
		g_brush_manager.waypoint_brush->setWaypoint(nullptr);
	} else {
		g_brush_manager.waypoint_brush->setWaypoint(map->waypoints.getWaypoint(name));
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
	// NanoVGListBox doesn't have inline editing, so we don't need to check GetEditControl

	// waypoint_list->Freeze(); // NanoVGListBox doesn't support Freeze/Thaw in same way (it's wxWindow though)
	// waypoint_list->DeleteAllItems(); // Replaced by SetWaypoints

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

	for (const auto& [name, wp] : waypoints) {
		names.push_back(name);
	}
	// Original code inserted at 0, effectively reversing the map order (which is sorted by key).
	// To match that, we should reverse.
	std::reverse(names.begin(), names.end());

	waypoint_list->SetWaypoints(names);
}

void WaypointPalettePanel::OnClickWaypoint(wxCommandEvent& event) {
	if (!map) {
		return;
	}

	std::string wpname = waypoint_list->GetSelectedWaypoint();
	Waypoint* wp = map->waypoints.getWaypoint(wpname);
	if (wp) {
		g_gui.SetScreenCenterPosition(wp->pos);
		g_brush_manager.waypoint_brush->setWaypoint(wp);
	}
}

void WaypointPalettePanel::OnClickAddWaypoint(wxCommandEvent& event) {
	if (map) {
		wxTextEntryDialog dialog(this, "Enter waypoint name:", "Add Waypoint");
		if (dialog.ShowModal() == wxID_OK) {
			wxString val = dialog.GetValue();
			if (!val.IsEmpty()) {
				std::string wpname = nstr(val);
				if (map->waypoints.getWaypoint(wpname)) {
					g_gui.SetStatusText("There already is a waypoint with this name.");
				} else {
					auto nwp = std::make_unique<Waypoint>();
					nwp->name = wpname;
					// Default position is (0,0,0) which is fine for new waypoint
					// Default waypoint constructor sets pos to (0,0,0).
					// But we probably want to set it to something useful if we are adding it?
					// The original code seemingly just added it and relied on user setting position via brush?
					// WaypointBrush sets the position when you click on map.
					// So adding a waypoint just creates a named handle.
					map->waypoints.addWaypoint(std::move(nwp));
					OnUpdate(); // Refresh list
				}
			}
		}
	}
}

void WaypointPalettePanel::OnClickRemoveWaypoint(wxCommandEvent& event) {
	if (!map) {
		return;
	}

	std::string name = waypoint_list->GetSelectedWaypoint();
	if (!name.empty()) {
		Waypoint* wp = map->waypoints.getWaypoint(name);
		if (wp) {
			if (map->getTile(wp->pos)) {
				map->getTileL(wp->pos)->decreaseWaypointCount();
			}
			map->waypoints.removeWaypoint(wp->name);
		}
		OnUpdate();
	}
}
