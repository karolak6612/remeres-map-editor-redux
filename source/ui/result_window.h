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

#ifndef RME_RESULT_WINDOW_H_
#define RME_RESULT_WINDOW_H_

#include "app/main.h"

#include <functional>
#include <vector>

struct SearchResultRow {
	uint32_t index = 0;
	wxString name;
	Position position;
};

class SearchResultWindow : public wxPanel {
public:
	SearchResultWindow(wxWindow* parent);
	~SearchResultWindow() override;

	void Clear();
	void SetResults(std::vector<SearchResultRow> rows, uint32_t total_count = 0, uint32_t page_offset = 0, uint32_t page_limit = 0, std::function<void(uint32_t)> page_loader = {});

	void OnClickExport(wxCommandEvent& event);
	void OnClickClear(wxCommandEvent& event);
	void OnClickPreviousPage(wxCommandEvent& event);
	void OnClickNextPage(wxCommandEvent& event);

protected:
	class ResultCanvas;

	void updateSummary();
	void focusFirstResult();
	void activateRow(int index);

	ResultCanvas* result_list = nullptr;
	wxStaticText* summary_label = nullptr;
	wxButton* previous_page_button_ = nullptr;
	wxButton* next_page_button_ = nullptr;
	wxButton* export_button_ = nullptr;
	wxButton* clear_button_ = nullptr;
	std::vector<SearchResultRow> rows_;
	uint32_t total_count_ = 0;
	uint32_t page_offset_ = 0;
	uint32_t page_limit_ = 0;
	std::function<void(uint32_t)> page_loader_;
};

#endif
