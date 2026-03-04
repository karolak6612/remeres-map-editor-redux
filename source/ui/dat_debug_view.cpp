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

#include "rendering/core/atlas_lifecycle.h"
#include "rendering/core/sprite_database.h"
#include "rendering/core/texture_gc.h"
#include "rendering/io/sprite_loader.h"
#include "ui/gui.h"

// ============================================================================
//

class DatDebugViewListBox : public wxVListBox {
public:
    DatDebugViewListBox(wxWindow* parent, wxWindowID id);
    ~DatDebugViewListBox();

    void OnDrawItem(wxDC& dc, const wxRect& rect, size_t index) const;
    wxCoord OnMeasureItem(size_t index) const;

protected:
    using SpriteIDs = std::vector<uint32_t>;
    SpriteIDs sprite_ids;
};

DatDebugViewListBox::DatDebugViewListBox(wxWindow* parent, wxWindowID id) :
    wxVListBox(parent, id, wxDefaultPosition, wxDefaultSize, wxLB_SINGLE)
{
    const int max_id = g_gui.sprites.getItemSpriteMaxID();
    sprite_ids.reserve(max_id);
    for (int id = 1; id <= max_id; ++id) {
        sprite_ids.push_back(id);
    }
    SetItemCount(sprite_ids.size());
}

DatDebugViewListBox::~DatDebugViewListBox()
{
    ////
}

void DatDebugViewListBox::OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const
{
    if (n < sprite_ids.size()) {
        g_gui.sprites.DrawItemSprite(sprite_ids[n], &dc, SPRITE_SIZE_32x32, rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
    }

    if (IsSelected(n)) {
        if (HasFocus()) {
            dc.SetTextForeground(wxColor(0xFF, 0xFF, 0xFF));
        } else {
            dc.SetTextForeground(wxColor(0x00, 0x00, 0xFF));
        }
    } else {
        dc.SetTextForeground(wxColor(0x00, 0x00, 0x00));
    }

    if (n < sprite_ids.size()) {
        dc.DrawText(wxString() << sprite_ids[n], rect.GetX() + 40, rect.GetY() + 6);
    }
}

wxCoord DatDebugViewListBox::OnMeasureItem(size_t n) const
{
    return 32;
}

// ============================================================================
//

DatDebugView::DatDebugView(wxWindow* parent) : wxPanel(parent)
{
    wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

    search_field = newd wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize);
    search_field->SetFocus();
    sizer->Add(search_field, 0, wxEXPAND, 2);

    item_list = newd DatDebugViewListBox(this, wxID_ANY);
    item_list->SetMinSize(wxSize(470, 400));
    sizer->Add(item_list, 1, wxEXPAND | wxALL, 2);

    SetSizerAndFit(sizer);
    Centre(wxBOTH);

    search_field->Bind(wxEVT_TEXT, &DatDebugView::OnTextChange, this);
    item_list->Bind(wxEVT_LISTBOX_DCLICK, &DatDebugView::OnClickList, this);
}

DatDebugView::~DatDebugView()
{
    ////
}

void DatDebugView::OnTextChange(wxCommandEvent& evt)
{
    ////
}

void DatDebugView::OnClickList(wxCommandEvent& evt)
{
    ////
}
