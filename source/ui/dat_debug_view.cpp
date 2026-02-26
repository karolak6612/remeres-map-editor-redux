#include "app/main.h"

#include "ui/dat_debug_view.h"

#include "rendering/core/graphics.h"
#include "ui/gui.h"
#include "util/nanovg_list_box.h"

// ============================================================================
//

class DatDebugViewListBox : public NanoVGListBox {
public:
	DatDebugViewListBox(wxWindow* parent, wxWindowID id);
	~DatDebugViewListBox();

	void OnDrawItem(NVGcontext* vg, int index, const wxRect& rect, bool selected) override;
	int GetItemHeight() const override {
		return 32;
	}

protected:
	using SpriteMap = std::vector<Sprite*>;
	SpriteMap sprites;
};

DatDebugViewListBox::DatDebugViewListBox(wxWindow* parent, wxWindowID id) :
	NanoVGListBox(parent, id) {
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

void DatDebugViewListBox::OnDrawItem(NVGcontext* vg, int index, const wxRect& rect, bool selected) {
	if (index < 0 || index >= (int)sprites.size()) {
		return;
	}

	Sprite* spr = sprites[index];
	if (spr) {
		int img = GetOrCreateSpriteTexture(vg, spr);
		if (img > 0) {
			int w, h;
			nvgImageSize(vg, img, &w, &h);

			// Fit to 32x32 if larger, center if smaller
			float scale = 1.0f;
			if (w > 32 || h > 32) {
				scale = 32.0f / std::max(w, h);
			}

			float drawW = w * scale;
			float drawH = h * scale;
			float x = rect.GetX() + (32 - drawW) / 2; // Center in first 32px slot
			float y = rect.GetY() + (32 - drawH) / 2;

			nvgBeginPath(vg);
			nvgRect(vg, x, y, drawW, drawH);
			nvgFillPaint(vg, nvgImagePattern(vg, x, y, drawW, drawH, 0, img, 1.0f));
			nvgFill(vg);
		}
	}

	// Draw text
	nvgFillColor(vg, selected ? nvgRGB(255, 255, 255) : nvgRGB(0, 0, 0));
	nvgFontSize(vg, 16.0f);
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

	std::string text = std::to_string(index);
	nvgText(vg, rect.GetX() + 40, rect.GetY() + rect.GetHeight() / 2, text.c_str(), nullptr);
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
