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

#include "ui/dat_debug_view.h"
#include "ui/controls/dat_debug_list_box.h"

#include "rendering/core/graphics.h"
#include "ui/gui.h"

// ============================================================================
//

DatDebugView::DatDebugView(wxWindow* parent) :
	wxPanel(parent) {
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	search_field = newd wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize);
	search_field->SetFocus();
	sizer->Add(search_field, 0, wxEXPAND, 2);

	item_list = newd DatDebugListBox(this, wxID_ANY);
	item_list->SetMinSize(wxSize(470, 400));
	sizer->Add(item_list, 1, wxEXPAND | wxALL, 2);

	SetSizerAndFit(sizer);
	Centre(wxBOTH);

	search_field->Bind(wxEVT_TEXT, &DatDebugView::OnTextChange, this);
	item_list->Bind(wxEVT_LISTBOX_DCLICK, &DatDebugView::OnClickList, this);
}

DatDebugView::~DatDebugView() {
	////
}

void DatDebugView::OnTextChange(wxCommandEvent& evt) {
	////
}

void DatDebugView::OnClickList(wxCommandEvent& evt) {
	////
}
