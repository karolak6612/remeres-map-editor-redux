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

#include "rendering/core/graphics.h"
#include "ui/gui.h"
#include "util/nanovg_listbox.h"
#include "util/nvg_utils.h"
#include "ui/theme.h"

#include <glad/glad.h>
#include <nanovg.h>

// ============================================================================
//

class DatDebugViewListBox : public NanoVGListBox {
public:
	DatDebugViewListBox(wxWindow* parent, wxWindowID id);
	~DatDebugViewListBox();

	void OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t index) override;
	int OnMeasureItem(size_t index) const override;

protected:
	using SpriteMap = std::vector<Sprite*>;
	SpriteMap sprites;
};

DatDebugViewListBox::DatDebugViewListBox(wxWindow* parent, wxWindowID id) :
	NanoVGListBox(parent, id, wxLB_SINGLE) {
	sprites.reserve(g_gui.gfx.getItemSpriteMaxID());
	for (int id = 0; id < g_gui.gfx.getItemSpriteMaxID(); ++id) {
		Sprite* spr = g_gui.gfx.getSprite(id);
		if (spr) {
			sprites.push_back(spr);
		}
	}
	SetItemCount(sprites.size());
}

DatDebugViewListBox::~DatDebugViewListBox() {
	////
}

void DatDebugViewListBox::OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t n) {
	if (n < sprites.size()) {
		Sprite* spr = sprites[n];
		if (spr) {
			int tex = GetOrCreateSpriteTexture(vg, spr);
			if (tex > 0) {
				float imgSize = 32.0f;
				NVGpaint imgPaint = nvgImagePattern(vg, rect.x, rect.y, imgSize, imgSize, 0, tex, 1.0f);
				nvgBeginPath(vg);
				nvgRect(vg, rect.x, rect.y, imgSize, imgSize);
				nvgFillPaint(vg, imgPaint);
				nvgFill(vg);
			}
		}
	}

	NVGcolor textColor;
	if (IsSelected(n)) {
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

	std::string text = std::to_string(n);
	nvgText(vg, rect.x + 40, rect.y + rect.height / 2.0f, text.c_str(), nullptr);
}

int DatDebugViewListBox::OnMeasureItem(size_t n) const {
	return 32;
}

// ============================================================================
//

DatDebugView::DatDebugView(wxWindow* parent) :
	wxPanel(parent) {
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

DatDebugView::~DatDebugView() {
	////
}

void DatDebugView::OnTextChange(wxCommandEvent& evt) {
	////
}

void DatDebugView::OnClickList(wxCommandEvent& evt) {
	////
}
