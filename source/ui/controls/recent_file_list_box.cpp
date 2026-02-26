//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "ui/controls/recent_file_list_box.h"
#include "ui/theme.h"
#include "util/image_manager.h"
#include "util/nvg_utils.h"
#include <wx/filename.h>
#include <wx/datetime.h>
#include <glad/glad.h>
#include <nanovg.h>
#include <nanovg_gl.h>

RecentFileListBox::RecentFileListBox(wxWindow* parent, wxWindowID id) :
	NanoVGListBox(parent, id, wxLB_SINGLE),
	m_iconImage(0) {
}

RecentFileListBox::~RecentFileListBox() {
}

void RecentFileListBox::SetRecentFiles(const std::vector<wxString>& files) {
	m_items.clear();
	for (const auto& f : files) {
		RecentFileItem item;
		item.path = f;

		wxFileName fn(f);
		wxDateTime dt;
		if (fn.GetTimes(nullptr, &dt, nullptr)) {
			item.date = dt.Format("%Y-%m-%d %H:%M");
		} else {
			item.date = "-";
		}
		m_items.push_back(item);
	}
	SetItemCount(m_items.size());
}

wxString RecentFileListBox::GetSelectedFile() const {
	int index = GetSelection();
	if (index >= 0 && index < static_cast<int>(m_items.size())) {
		return m_items[index].path;
	}
	return "";
}

void RecentFileListBox::OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t index) {
	if (index >= m_items.size()) {
		return;
	}

	const RecentFileItem& item = m_items[index];

	int x = rect.GetX();
	int y = rect.GetY();
	int w = rect.GetWidth();
	int h = rect.GetHeight();

	// Icon
	if (m_iconImage == 0) {
		m_iconImage = IMAGE_MANAGER.GetNanoVGImage(vg, ICON_MAP);
	}

	if (m_iconImage > 0) {
		NVGpaint imgPaint = nvgImagePattern(vg, x + 8, y + (h - 16) / 2, 16, 16, 0, m_iconImage, 1.0f);
		nvgBeginPath(vg);
		nvgRect(vg, x + 8, y + (h - 16) / 2, 16, 16);
		nvgFillPaint(vg, imgPaint);
		nvgFill(vg);
	}

	// Text setup
	nvgFontSize(vg, 14.0f);
	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

	if (IsSelected(index)) {
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	} else {
		nvgFillColor(vg, NvgUtils::ToNvColor(Theme::Get(Theme::Role::Text)));
	}

	// File Name/Path
	nvgText(vg, x + 32, y + h / 2.0f, item.path.ToUTF8().data(), nullptr);

	// Date (Right aligned)
	nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
	nvgFillColor(vg, NvgUtils::ToNvColor(Theme::Get(Theme::Role::TextSubtle)));
	nvgText(vg, x + w - 10, y + h / 2.0f, item.date.ToUTF8().data(), nullptr);
}

int RecentFileListBox::OnMeasureItem(size_t index) const {
	return 28;
}
