//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "palette/controls/waypoint_list_box.h"
#include "ui/theme.h"
#include "util/nvg_utils.h"
#include <glad/glad.h>
#include <nanovg.h>
#include <nanovg_gl.h>

WaypointListBox::WaypointListBox(wxWindow* parent, wxWindowID id) :
	NanoVGListBox(parent, id, wxLB_SINGLE) {

	Bind(wxEVT_LEFT_DCLICK, [this](wxMouseEvent& event) {
		if (GetSelection() != -1) {
			wxCommandEvent evt(wxEVT_LISTBOX_DCLICK, GetId());
			evt.SetEventObject(this);
			evt.SetInt(GetSelection());
			ProcessWindowEvent(evt);
		}
	});
}

WaypointListBox::~WaypointListBox() {
}

void WaypointListBox::SetWaypoints(const std::vector<std::string>& names) {
	m_items = names;
	SetItemCount(m_items.size());
}

wxString WaypointListBox::GetSelectedWaypoint() const {
	int selection = GetSelection();
	if (selection >= 0 && selection < static_cast<int>(m_items.size())) {
		return wxstr(m_items[selection]);
	}
	return "";
}

void WaypointListBox::OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t index) {
	if (index >= m_items.size()) {
		return;
	}

	const std::string& name = m_items[index];

	int x = rect.GetX();
	int y = rect.GetY();
	int w = rect.GetWidth();
	int h = rect.GetHeight();

	// Text
	nvgFontSize(vg, 14.0f);
	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

	if (IsSelected(index)) {
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	} else {
		nvgFillColor(vg, NvgUtils::ToNvColor(Theme::Get(Theme::Role::Text)));
	}

	nvgText(vg, x + 10, y + h / 2.0f, name.c_str(), nullptr);
}

int WaypointListBox::OnMeasureItem(size_t index) const {
	return 24;
}
