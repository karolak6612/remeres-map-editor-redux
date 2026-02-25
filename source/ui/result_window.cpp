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

#include "app/main.h"

#include "ui/result_window.h"
#include "ui/gui.h"
#include "map/position.h"
#include "util/image_manager.h"
#include "util/nanovg_listbox.h"
#include "ui/theme.h"
#include "util/nvg_utils.h"

struct SearchResultItem {
	wxString description;
	Position pos;
};

class SearchResultListBox : public NanoVGListBox {
public:
	SearchResultListBox(wxWindow* parent, wxWindowID id) :
		NanoVGListBox(parent, id, wxLB_SINGLE) {
	}

	void AddPosition(wxString description, Position pos) {
		m_items.push_back({description, pos});
		SetItemCount(m_items.size());
		Refresh();
	}

	void Clear() {
		m_items.clear();
		SetItemCount(0);
		Refresh();
	}

	Position GetPosition(int index) const {
		if (index >= 0 && index < (int)m_items.size()) {
			return m_items[index].pos;
		}
		return Position();
	}

	wxString GetString(int index) const {
		if (index >= 0 && index < (int)m_items.size()) {
			return m_items[index].description;
		}
		return "";
	}

	size_t GetCount() const {
		return m_items.size();
	}

	void OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t index) override {
		if (index >= m_items.size()) {
			return;
		}

		// Selection background
		if (IsSelected(static_cast<int>(index))) {
			nvgFillColor(vg, NvgUtils::ToNvColor(Theme::Get(Theme::Role::Accent)));
			nvgBeginPath(vg);
			nvgRect(vg, static_cast<float>(rect.x), static_cast<float>(rect.y), static_cast<float>(rect.width), static_cast<float>(rect.height));
			nvgFill(vg);
			nvgFillColor(vg, NvgUtils::ToNvColor(Theme::Get(Theme::Role::TextOnAccent)));
		} else {
			nvgFillColor(vg, NvgUtils::ToNvColor(Theme::Get(Theme::Role::Text)));
		}

		nvgFontSize(vg, 12.0f);
		nvgFontFace(vg, "sans");
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgText(vg, static_cast<float>(rect.x + 5), rect.y + rect.height / 2.0f, m_items[index].description.ToUTF8().data(), nullptr);
	}

	int OnMeasureItem(size_t index) const override {
		return 20;
	}

private:
	std::vector<SearchResultItem> m_items;
};

SearchResultWindow::SearchResultWindow(wxWindow* parent) :
	wxPanel(parent, wxID_ANY) {
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	result_list = newd SearchResultListBox(this, wxID_ANY);
	result_list->SetMinSize(FromDIP(wxSize(200, 330)));
	sizer->Add(result_list, wxSizerFlags(1).Expand());

	wxSizer* buttonsSizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* exportBtn = newd wxButton(this, wxID_FILE, "Export");
	exportBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_FILE_EXPORT, wxSize(16, 16)));
	exportBtn->SetToolTip("Export results to text file");
	buttonsSizer->Add(exportBtn, wxSizerFlags(0).Center());

	wxButton* clearBtn = newd wxButton(this, wxID_CLEAR, "Clear");
	clearBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_TRASH_CAN, wxSize(16, 16)));
	clearBtn->SetToolTip("Clear search results");
	buttonsSizer->Add(clearBtn, wxSizerFlags(0).Center());
	sizer->Add(buttonsSizer, wxSizerFlags(0).Center().DoubleBorder());
	SetSizerAndFit(sizer);

	result_list->Bind(wxEVT_LISTBOX, &SearchResultWindow::OnClickResult, this);
	exportBtn->Bind(wxEVT_BUTTON, &SearchResultWindow::OnClickExport, this);
	clearBtn->Bind(wxEVT_BUTTON, &SearchResultWindow::OnClickClear, this);
}

SearchResultWindow::~SearchResultWindow() {
	Clear();
}

void SearchResultWindow::Clear() {
	result_list->Clear();
}

void SearchResultWindow::AddPosition(wxString description, Position pos) {
	result_list->AddPosition(description << " (" << pos.x << "," << pos.y << "," << pos.z << ")", pos);
}

void SearchResultWindow::OnClickResult(wxCommandEvent& event) {
	int selection = result_list->GetSelection();
	if (selection != -1) {
		Position pos = result_list->GetPosition(selection);
		if (pos != Position()) {
			g_gui.SetScreenCenterPosition(pos);
		}
	}
}

void SearchResultWindow::OnClickExport(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog dialog(this, "Save file...", "", "", "Text Documents (*.txt) | *.txt", wxFD_SAVE);
	if (dialog.ShowModal() == wxID_OK) {
		wxFile file(dialog.GetPath(), wxFile::write);
		if (file.IsOpened()) {
			g_gui.CreateLoadBar("Exporting search result...");

			file.Write("Generated by Remere's Map Editor " + __RME_VERSION__);
			file.Write("\n=============================================\n\n");
			size_t count = result_list->GetCount();
			for (size_t i = 0; i < count; ++i) {
				file.Write(result_list->GetString(i) + "\n");
				g_gui.SetLoadScale((int32_t)i, (int32_t)count);
			}
			file.Close();

			g_gui.DestroyLoadBar();
		}
	}
}

void SearchResultWindow::OnClickClear(wxCommandEvent& WXUNUSED(event)) {
	Clear();
}
