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

#include "map/tile_operations.h"
#include "app/main.h"

#include "map/map.h"
#include "ui/gui.h"
#include "brushes/raw/raw_brush.h"
#include "map/tile.h"
#include "rendering/core/graphics.h"
#include "ui/gui.h"
#include <wx/listbox.h>
#include "ui/browse_tile_window.h"
#include "util/image_manager.h"
#include "util/nanovg_listbox.h"
#include <glad/glad.h>
#include <format>
#include <nanovg.h>

// ============================================================================
//

class BrowseTileListBox : public NanoVGListBox {
public:
	BrowseTileListBox(wxWindow* parent, wxWindowID id, Tile* tile);
	~BrowseTileListBox();

	void OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t index) override;
	int OnMeasureItem(size_t index) const override;
	Item* GetSelectedItem();
	void RemoveSelected();

	void SetSelection(int index) override;
	void Select(int index, bool select = true) override;
	void ClearSelection() override;

protected:
	void UpdateItems();

	using ItemsMap = std::vector<Item*>;
	ItemsMap items;
	Tile* edit_tile;
};

BrowseTileListBox::BrowseTileListBox(wxWindow* parent, wxWindowID id, Tile* tile) :
	NanoVGListBox(parent, id, wxLB_SINGLE), edit_tile(tile) {
	SetMinSize(FromDIP(wxSize(260, 200)));
	SetGridMode(true, FromDIP(64), FromDIP(64));
	UpdateItems();
}

BrowseTileListBox::~BrowseTileListBox() {
	////
}

void BrowseTileListBox::OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t n) {
	Item* item = items[n];

	Sprite* sprite = g_gui.gfx.getSprite(item->getClientID());
	if (sprite) {
		int tex = GetOrCreateSpriteTexture(vg, sprite);
		if (tex > 0) {
			int icon_size = std::min(32, std::min(rect.width, rect.height));
			int imgX = rect.x + (rect.width - icon_size) / 2;
			int imgY = rect.y + (rect.height - icon_size) / 2 - FromDIP(6);

			NVGpaint imgPaint = nvgImagePattern(vg, imgX, imgY, icon_size, icon_size, 0, tex, 1.0f);
			nvgBeginPath(vg);
			nvgRect(vg, imgX, imgY, icon_size, icon_size);
			nvgFillPaint(vg, imgPaint);
			nvgFill(vg);
		}
	}

	if (IsSelected(n)) {
		wxColour textColour = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);
		nvgFillColor(vg, nvgRGBA(textColour.Red(), textColour.Green(), textColour.Blue(), 255));
	} else {
		wxColour textColour = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT);
		nvgFillColor(vg, nvgRGBA(textColour.Red(), textColour.Green(), textColour.Blue(), 255));
	}

	std::string label = std::format("{}", item->getID());

	nvgFontSize(vg, 11.0f);
	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
	nvgText(vg, rect.x + rect.width / 2.0f, rect.y + rect.height - FromDIP(4), label.c_str(), nullptr);
}

int BrowseTileListBox::OnMeasureItem(size_t n) const {
	return FromDIP(64);
}

Item* BrowseTileListBox::GetSelectedItem() {
	if (GetItemCount() == 0 || GetSelectedCount() == 0) {
		return nullptr;
	}

	return TileOperations::getTopSelectedItem(edit_tile);
}

void BrowseTileListBox::RemoveSelected() {
	if (GetItemCount() == 0 || GetSelectedCount() == 0) {
		return;
	}

	ClearSelection();
	items.clear();

	// Delete the items from the tile
	auto tile_selection = TileOperations::popSelectedItems(edit_tile, true);
	// items are automatically deleted when tile_selection goes out of scope

	UpdateItems();
	Refresh();
}

void BrowseTileListBox::SetSelection(int index) {
	if (m_selection != -1 && (size_t)m_selection < items.size()) {
		items[m_selection]->deselect();
	}
	NanoVGListBox::SetSelection(index);
	if (m_selection != -1 && (size_t)m_selection < items.size()) {
		items[m_selection]->select();
	}
}

void BrowseTileListBox::Select(int index, bool select) {
	if (index >= 0 && (size_t)index < items.size()) {
		if (select) {
			items[index]->select();
		} else {
			items[index]->deselect();
		}
	}
	NanoVGListBox::Select(index, select);
}

void BrowseTileListBox::ClearSelection() {
	for (Item* item : items) {
		item->deselect();
	}
	NanoVGListBox::ClearSelection();
}

void BrowseTileListBox::UpdateItems() {
	items.clear();
	items.reserve(edit_tile->items.size() + (edit_tile->ground ? 1 : 0));
	for (auto it = edit_tile->items.rbegin(); it != edit_tile->items.rend(); ++it) {
		items.push_back(it->get());
	}

	if (edit_tile->ground) {
		items.push_back(edit_tile->ground.get());
	}

	SetItemCount(items.size());
}

// ============================================================================
//

BrowseTileWindow::BrowseTileWindow(wxWindow* parent, Tile* tile, wxPoint position /* = wxDefaultPosition */) :
	wxDialog(parent, wxID_ANY, "Browse Field", position, FROM_DIP(parent, wxSize(640, 480)), wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER) {

	wxSizer* mainSizer = newd wxBoxSizer(wxVERTICAL);
	wxSizer* topSizer = newd wxBoxSizer(wxHORIZONTAL);

	// Left Side: Grid List
	item_list = newd BrowseTileListBox(this, wxID_ANY, tile);
	topSizer->Add(item_list, wxSizerFlags(1).Expand().Border(wxALL, 10));

	// Right Side: Info and Actions
	wxSizer* rightSizer = newd wxBoxSizer(wxVERTICAL);

	// Actions Box
	wxStaticBoxSizer* actionSizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Actions");
	wxBoxSizer* buttons = newd wxBoxSizer(wxHORIZONTAL);
	delete_button = newd wxButton(actionSizer->GetStaticBox(), wxID_REMOVE, "Delete");
	delete_button->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_TRASH_CAN));
	delete_button->SetToolTip("Delete selected item");
	delete_button->Enable(false);
	buttons->Add(delete_button, wxSizerFlags(1).Expand().Border(wxRIGHT, 5));
	select_raw_button = newd wxButton(actionSizer->GetStaticBox(), wxID_FIND, "Select RAW");
	select_raw_button->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_SEARCH));
	select_raw_button->SetToolTip("Select this item in RAW palette");
	select_raw_button->Enable(false);
	buttons->Add(select_raw_button, wxSizerFlags(1).Expand());
	actionSizer->Add(buttons, wxSizerFlags(0).Expand().Border(wxALL, 5));
	rightSizer->Add(actionSizer, wxSizerFlags(0).Expand().Border(wxALL, 5));

	// Tile Information Box
	wxString posStr = std::format("x={}, y={}, z={}", tile->getX(), tile->getY(), tile->getZ());
	wxStaticBoxSizer* infoSizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Tile Information");
	wxFlexGridSizer* infoGrid = newd wxFlexGridSizer(2, FromDIP(5), FromDIP(10));

	infoGrid->Add(newd wxStaticText(infoSizer->GetStaticBox(), wxID_ANY, "Position:"), wxSizerFlags().Right());
	infoGrid->Add(newd wxStaticText(infoSizer->GetStaticBox(), wxID_ANY, posStr), wxSizerFlags().Left());

	infoGrid->Add(newd wxStaticText(infoSizer->GetStaticBox(), wxID_ANY, "Item count:"), wxSizerFlags().Right());
	infoGrid->Add(item_count_txt = newd wxStaticText(infoSizer->GetStaticBox(), wxID_ANY, i2ws(item_list->GetItemCount())), wxSizerFlags().Left());

	infoSizer->Add(infoGrid, wxSizerFlags(1).Expand().Border(wxALL, 5));
	rightSizer->Add(infoSizer, wxSizerFlags(0).Expand().Border(wxALL, 5));

	// Tile Flags Box
	wxStaticBoxSizer* flagsSizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Tile Flags");
	wxFlexGridSizer* flagsGrid = newd wxFlexGridSizer(2, FromDIP(5), FromDIP(10));

	auto addFlag = [&](const wxString& label, bool value, std::string_view icon) {
		wxBoxSizer* row = newd wxBoxSizer(wxHORIZONTAL);
		wxStaticBitmap* sbmp = newd wxStaticBitmap(flagsSizer->GetStaticBox(), wxID_ANY, IMAGE_MANAGER.GetBitmapBundle(icon, wxSize(14, 14)));
		row->Add(sbmp, wxSizerFlags(0).Center().Border(wxRIGHT, 4));
		row->Add(newd wxStaticText(flagsSizer->GetStaticBox(), wxID_ANY, label), wxSizerFlags(0).Center());
		flagsGrid->Add(row, wxSizerFlags().Right());

		wxStaticText* valText = newd wxStaticText(flagsSizer->GetStaticBox(), wxID_ANY, value ? "Yes" : "No");
		if (value) {
			valText->SetForegroundColour(wxColour(0, 150, 0)); // Green for Yes
			wxFont f = valText->GetFont();
			f.SetWeight(wxFONTWEIGHT_BOLD);
			valText->SetFont(f);
		}
		flagsGrid->Add(valText, wxSizerFlags().Left().CenterVertical());
	};

	addFlag("Protection zone:", tile->isPZ(), ICON_SHIELD_HALVED);
	addFlag("No PvP:", tile->getMapFlags() & TILESTATE_NOPVP, ICON_HAND_PEACE);
	addFlag("No logout:", tile->getMapFlags() & TILESTATE_NOLOGOUT, ICON_BAN);
	addFlag("PvP zone:", tile->getMapFlags() & TILESTATE_PVPZONE, ICON_SKULL);
	addFlag("House:", tile->isHouseTile(), ICON_HOUSE);

	flagsSizer->Add(flagsGrid, wxSizerFlags(1).Expand().Border(wxALL, 5));
	rightSizer->Add(flagsSizer, wxSizerFlags(1).Expand().Border(wxALL, 5));

	topSizer->Add(rightSizer, wxSizerFlags(0).Expand().Border(wxALL, 5));
	mainSizer->Add(topSizer, wxSizerFlags(1).Expand());

	// OK/Cancel buttons
	wxSizer* btnSizer = newd wxBoxSizer(wxHORIZONTAL);
	auto okBtn = newd wxButton(this, wxID_OK, "OK");
	okBtn->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_CHECK));
	okBtn->SetToolTip("Confirm selection");
	btnSizer->Add(okBtn, wxSizerFlags(0).Center().Border(wxRIGHT, 5));
	auto cancelBtn = newd wxButton(this, wxID_CANCEL, "Cancel");
	cancelBtn->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_XMARK));
	cancelBtn->SetToolTip("Cancel");
	btnSizer->Add(cancelBtn, wxSizerFlags(0).Center().Border(wxLEFT, 5));
	mainSizer->Add(btnSizer, wxSizerFlags(0).Right().Border(wxALL, 10));

	SetSizerAndFit(mainSizer);

	// Connect Events
	item_list->Bind(wxEVT_LISTBOX, &BrowseTileWindow::OnItemSelected, this);
	delete_button->Bind(wxEVT_BUTTON, &BrowseTileWindow::OnClickDelete, this);
	select_raw_button->Bind(wxEVT_BUTTON, &BrowseTileWindow::OnClickSelectRaw, this);
	Bind(wxEVT_BUTTON, &BrowseTileWindow::OnClickOK, this, wxID_OK);
	Bind(wxEVT_BUTTON, &BrowseTileWindow::OnClickCancel, this, wxID_CANCEL);
}

BrowseTileWindow::~BrowseTileWindow() {
}

void BrowseTileWindow::OnItemSelected(wxCommandEvent& WXUNUSED(event)) {
	const size_t count = item_list->GetSelectedCount();
	delete_button->Enable(count != 0);
	select_raw_button->Enable(count == 1);
}

void BrowseTileWindow::OnClickDelete(wxCommandEvent& WXUNUSED(event)) {
	item_list->RemoveSelected();
	item_count_txt->SetLabelText("Item count:  " + i2ws(item_list->GetItemCount()));
}

void BrowseTileWindow::OnClickSelectRaw(wxCommandEvent& WXUNUSED(event)) {
	Item* item = item_list->GetSelectedItem();
	if (item && item->getRAWBrush()) {
		g_gui.SelectBrush(item->getRAWBrush(), TILESET_RAW);
	}

	EndModal(1);
}

void BrowseTileWindow::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	EndModal(1);
}

void BrowseTileWindow::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	EndModal(0);
}
