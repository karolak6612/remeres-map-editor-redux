#include "ui/controls/recent_file_listbox.h"
#include "util/image_manager.h"
#include "ui/theme.h"
#include <wx/filename.h>
#include <nanovg.h>

RecentFileListBox::RecentFileListBox(wxWindow* parent, wxWindowID id) :
	NanoVGListBox(parent, id, wxLB_SINGLE) {
}

RecentFileListBox::~RecentFileListBox() {
}

void RecentFileListBox::SetRecentFiles(const std::vector<wxString>& files) {
	m_files.clear();
	for (const auto& file : files) {
		RecentFile rf;
		rf.path = file;
		wxFileName fn(file);
		rf.name = fn.GetFullName();

		wxDateTime dt;
		if (fn.GetTimes(nullptr, &dt, nullptr)) {
			rf.date = dt.Format("%Y-%m-%d %H:%M");
		} else {
			rf.date = "-";
		}
		m_files.push_back(rf);
	}
	SetItemCount(m_files.size());
}

wxString RecentFileListBox::GetFilePath(int index) const {
	if (index >= 0 && index < static_cast<int>(m_files.size())) {
		return m_files[index].path;
	}
	return "";
}

int RecentFileListBox::OnMeasureItem(size_t index) const {
	return FromDIP(50);
}

void RecentFileListBox::OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t index) {
	if (index >= m_files.size()) {
		return;
	}

	const RecentFile& file = m_files[index];
	bool selected = IsSelected(static_cast<int>(index));

	wxColour textCol = Theme::Get(Theme::Role::Text);
	wxColour subTextCol = Theme::Get(Theme::Role::TextSubtle);

	if (selected) {
		textCol = Theme::Get(Theme::Role::TextOnAccent);
		subTextCol = textCol.ChangeLightness(180); // Lighter for subtitle on selection
	}

	int x = rect.x + FromDIP(8);
	int y = rect.y + rect.height / 2;

	// Draw Icon
	int iconSize = FromDIP(32);
	int iconY = rect.y + (rect.height - iconSize) / 2;

	// Use ICON_MAP (svg/regular/map.svg)
	int img = IMAGE_MANAGER.GetNanoVGImage(vg, ICON_MAP, selected ? textCol : wxNullColour);
	if (img > 0) {
		NVGpaint imgPaint = nvgImagePattern(vg, x, iconY, iconSize, iconSize, 0, img, 1.0f);
		nvgBeginPath(vg);
		nvgRect(vg, x, iconY, iconSize, iconSize);
		nvgFillPaint(vg, imgPaint);
		nvgFill(vg);
	}

	int textX = x + iconSize + FromDIP(12);

	// Draw Name (Top)
	nvgFontSize(vg, 16.0f);
	nvgFontFace(vg, "sans-bold");
	nvgFillColor(vg, nvgRGBA(textCol.Red(), textCol.Green(), textCol.Blue(), 255));
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);
	nvgText(vg, textX, y - FromDIP(2), file.name.ToUTF8().data(), nullptr);

	// Draw Path (Bottom)
	nvgFontSize(vg, 12.0f);
	nvgFontFace(vg, "sans");
	nvgFillColor(vg, nvgRGBA(subTextCol.Red(), subTextCol.Green(), subTextCol.Blue(), 255));
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
	nvgText(vg, textX, y, file.path.ToUTF8().data(), nullptr);

	// Draw Date (Right Aligned)
	nvgFontSize(vg, 12.0f);
	nvgFontFace(vg, "sans");
	nvgFillColor(vg, nvgRGBA(subTextCol.Red(), subTextCol.Green(), subTextCol.Blue(), 255));
	nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
	nvgText(vg, rect.width - FromDIP(16), rect.y + rect.height / 2.0f, file.date.ToUTF8().data(), nullptr);
}
