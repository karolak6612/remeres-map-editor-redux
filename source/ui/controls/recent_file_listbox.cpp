#include "ui/controls/recent_file_listbox.h"
#include "ui/theme.h"
#include "util/nvg_utils.h"
#include <wx/datetime.h>
#include <algorithm>

RecentFileListBox::RecentFileListBox(wxWindow* parent, wxWindowID id) :
	NanoVGListBox(parent, id, wxLB_SINGLE) {
	Bind(wxEVT_LEFT_DCLICK, &RecentFileListBox::OnMouseDoubleClick, this);
}

RecentFileListBox::~RecentFileListBox() {
}

void RecentFileListBox::SetRecentFiles(const std::vector<wxString>& files) {
	m_items.clear();
	for (const auto& file : files) {
		RecentFileItem item;
		item.fullPath = file;
		wxFileName fn(file);
		item.mapName = fn.GetName();
		item.path = fn.GetPath();

		wxDateTime dt;
		if (fn.GetTimes(nullptr, &dt, nullptr)) {
			item.date = dt.Format("%Y-%m-%d");
		} else {
			item.date = "-";
		}
		m_items.push_back(item);
	}
	SetItemCount(m_items.size());
	Refresh();
}

wxString RecentFileListBox::GetSelectedFile() const {
	int sel = GetSelection();
	if (sel >= 0 && sel < (int)m_items.size()) {
		return m_items[sel].fullPath;
	}
	return "";
}

int RecentFileListBox::OnMeasureItem(size_t index) const {
	return 60; // Height of each card row
}

void RecentFileListBox::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	if (m_count == 0) {
		return;
	}

	int scrollPos = GetScrollPosition();
	int itemHeight = std::max(1, OnMeasureItem(0));

	int startRow = scrollPos / itemHeight;
	int endRow = (scrollPos + height + itemHeight - 1) / itemHeight;

	startRow = std::max(0, startRow);
	endRow = std::min(static_cast<int>(m_count), endRow);

	for (int i = startRow; i < endRow; ++i) {
		wxRect rect(0, i * itemHeight, width, itemHeight);
		OnDrawItem(vg, rect, i);
	}
}

void RecentFileListBox::OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t index) {
	if (index >= m_items.size()) return;

	const RecentFileItem& item = m_items[index];
	bool isSelected = IsSelected(index);
	bool isHover = ((int)index == m_hoverIndex);

	float x = rect.x + 4;
	float y = rect.y + 2;
	float w = rect.width - 8;
	float h = rect.height - 4;

	// Shadow
	NVGpaint shadowPaint = nvgBoxGradient(vg, x, y + 2, w, h, 4, 10, nvgRGBA(0, 0, 0, 128), nvgRGBA(0, 0, 0, 0));
	nvgBeginPath(vg);
	nvgRect(vg, x - 10, y - 10, w + 20, h + 20);
	nvgRoundedRect(vg, x, y, w, h, 4);
	nvgPathWinding(vg, NVG_HOLE);
	nvgFillPaint(vg, shadowPaint);
	nvgFill(vg);

	// Card Background
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, w, h, 4);
	NVGcolor bgColor;
	if (isSelected) {
		bgColor = NvgUtils::ToNvColor(Theme::Get(Theme::Role::Selected));
	} else if (isHover) {
		bgColor = NvgUtils::ToNvColor(Theme::Get(Theme::Role::CardBaseHover));
	} else {
		bgColor = NvgUtils::ToNvColor(Theme::Get(Theme::Role::CardBase));
	}
	nvgFillColor(vg, bgColor);
	nvgFill(vg);

	if (isSelected) {
		nvgStrokeColor(vg, NvgUtils::ToNvColor(Theme::Get(Theme::Role::Accent)));
		nvgStrokeWidth(vg, 2.0f);
		nvgStroke(vg);
	}

	// Icon (Vector Map)
	float iconX = x + 16;
	float iconY = y + h/2;

	nvgSave(vg);
	nvgTranslate(vg, iconX, iconY);

	// Draw a simple map icon
	nvgBeginPath(vg);
	nvgMoveTo(vg, -10, -8);
	nvgLineTo(vg, -4, -12); // Fold 1 top
	nvgLineTo(vg, 4, -8);   // Fold 2 top
	nvgLineTo(vg, 10, -12); // Fold 3 top
	nvgLineTo(vg, 10, 8);   // Right bottom
	nvgLineTo(vg, 4, 12);   // Fold 2 bottom
	nvgLineTo(vg, -4, 8);   // Fold 1 bottom
	nvgLineTo(vg, -10, 12); // Left bottom
	nvgClosePath(vg);

	nvgFillColor(vg, nvgRGBA(200, 200, 200, isSelected ? 255 : 180));
	nvgFill(vg);

	// Fold lines
	nvgBeginPath(vg);
	nvgMoveTo(vg, -4, -12);
	nvgLineTo(vg, -4, 8);
	nvgMoveTo(vg, 4, -8);
	nvgLineTo(vg, 4, 12);
	nvgStrokeColor(vg, nvgRGBA(100, 100, 100, 100));
	nvgStrokeWidth(vg, 1.0f);
	nvgStroke(vg);

	nvgRestore(vg);

	// Text
	float textX = x + 40;

	// Map Name
	nvgFontSize(vg, 18.0f);
	nvgFontFace(vg, "sans-bold");
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);
	nvgFillColor(vg, NvgUtils::ToNvColor(Theme::Get(Theme::Role::Text)));
	nvgText(vg, textX, y + h/2 - 2, item.mapName.ToUTF8(), nullptr);

	// Path
	nvgFontSize(vg, 12.0f);
	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
	nvgFillColor(vg, NvgUtils::ToNvColor(Theme::Get(Theme::Role::TextSubtle)));

	nvgText(vg, textX, y + h/2 + 2, item.path.ToUTF8(), nullptr);

	// Date
	nvgFontSize(vg, 12.0f);
	nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
	nvgText(vg, x + w - 10, y + h/2, item.date.ToUTF8(), nullptr);
}

void RecentFileListBox::OnMouseDoubleClick(wxMouseEvent& event) {
	int scrollPos = GetScrollPosition();
	int index = HitTest(event.GetX(), event.GetY() + scrollPos);

	if (index != -1) {
		wxCommandEvent event(wxEVT_LISTBOX_DCLICK, GetId());
		event.SetEventObject(this);
		event.SetInt(index);
		ProcessWindowEvent(event);
	}
}
