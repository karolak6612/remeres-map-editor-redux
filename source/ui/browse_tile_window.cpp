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
	SetMinSize(FromDIP(wxSize(200, 180)));
	SetGridMode(true, FromDIP(80), FromDIP(80)); // Use grid for thumbnails
	UpdateItems();
}

BrowseTileListBox::~BrowseTileListBox() {
	////
}

void BrowseTileListBox::OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t n) {
	Item* item = items[n];

	Sprite* sprite = g_gui.gfx.getSprite(item->getClientID());

	int icon_size = 32;
	int x_offset = rect.x + (rect.width - icon_size) / 2;
	int y_offset = rect.y + 10;

	if (sprite) {
		int tex = GetOrCreateSpriteTexture(vg, sprite);
		if (tex > 0) {
			NVGpaint imgPaint = nvgImagePattern(vg, x_offset, y_offset, icon_size, icon_size, 0, tex, 1.0f);
			nvgBeginPath(vg);
			nvgRect(vg, x_offset, y_offset, icon_size, icon_size);
			nvgFillPaint(vg, imgPaint);
			nvgFill(vg);
		}
	}

	if (IsSelected(n)) {
		wxColour textColour = Theme::Get(Theme::Role::TextOnAccent);
		nvgFillColor(vg, nvgRGBA(textColour.Red(), textColour.Green(), textColour.Blue(), 255));
	} else {
		wxColour textColour = Theme::Get(Theme::Role::Text);
		nvgFillColor(vg, nvgRGBA(textColour.Red(), textColour.Green(), textColour.Blue(), 255));
	}

	nvgFontSize(vg, 11.0f);
	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);

	std::string nameStr = item->getName();
	if (nameStr.length() > 15) {
		nameStr = nameStr.substr(0, 12) + "...";
	}
	nvgText(vg, rect.x + rect.width / 2.0f, y_offset + icon_size + 5, nameStr.c_str(), nullptr);

	nvgFontSize(vg, 9.0f);
	nvgFillColor(vg, nvgRGBA(Theme::Get(Theme::Role::TextSubtle).Red(), Theme::Get(Theme::Role::TextSubtle).Green(), Theme::Get(Theme::Role::TextSubtle).Blue(), 180));
	std::string idText = "ID: " + std::to_string(item->getID());
	nvgText(vg, rect.x + rect.width / 2.0f, y_offset + icon_size + 18, idText.c_str(), nullptr);
}

int BrowseTileListBox::OnMeasureItem(size_t n) const {
	return FromDIP(80);
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
	wxDialog(parent, wxID_ANY, "Browse Field", position, FROM_DIP(parent, wxSize(600, 400)), wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER) {
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	// Create horizontal split layout
	wxBoxSizer* mainSplit = newd wxBoxSizer(wxHORIZONTAL);

	// Left side: Items list
	wxStaticBoxSizer* listGroup = newd wxStaticBoxSizer(wxVERTICAL, this, "Tile Contents");
	item_list = newd BrowseTileListBox(listGroup->GetStaticBox(), wxID_ANY, tile);
	listGroup->Add(item_list, 1, wxEXPAND | wxALL, 4);
	mainSplit->Add(listGroup, 1, wxEXPAND | wxALL, 4);

	// Right side: Tile properties and Actions
	wxBoxSizer* rightSplit = newd wxBoxSizer(wxVERTICAL);

	// Properties Box
	wxStaticBoxSizer* infoSizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Properties");

	wxString pos;
	pos << "x=" << tile->getX() << ",  y=" << tile->getY() << ",  z=" << tile->getZ();

	infoSizer->Add(newd wxStaticText(infoSizer->GetStaticBox(), wxID_ANY, "Position:  " + pos), 0, wxALL, 4);
	infoSizer->Add(item_count_txt = newd wxStaticText(infoSizer->GetStaticBox(), wxID_ANY, "Item count:  " + i2ws(item_list->GetItemCount())), 0, wxALL, 4);
	infoSizer->Add(newd wxStaticText(infoSizer->GetStaticBox(), wxID_ANY, "Protection zone:  " + b2yn(tile->isPZ())), 0, wxALL, 4);
	infoSizer->Add(newd wxStaticText(infoSizer->GetStaticBox(), wxID_ANY, "No PvP:  " + b2yn(tile->getMapFlags() & TILESTATE_NOPVP)), 0, wxALL, 4);
	infoSizer->Add(newd wxStaticText(infoSizer->GetStaticBox(), wxID_ANY, "No logout:  " + b2yn(tile->getMapFlags() & TILESTATE_NOLOGOUT)), 0, wxALL, 4);
	infoSizer->Add(newd wxStaticText(infoSizer->GetStaticBox(), wxID_ANY, "PvP zone:  " + b2yn(tile->getMapFlags() & TILESTATE_PVPZONE)), 0, wxALL, 4);
	infoSizer->Add(newd wxStaticText(infoSizer->GetStaticBox(), wxID_ANY, "House:  " + b2yn(tile->isHouseTile())), 0, wxALL, 4);

	rightSplit->Add(infoSizer, 0, wxEXPAND | wxALL, 4);

	// Actions Box
	wxStaticBoxSizer* actionsSizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Actions");

	delete_button = newd wxButton(actionsSizer->GetStaticBox(), wxID_REMOVE, "Delete Selected");
	delete_button->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_TRASH_CAN));
	delete_button->SetToolTip("Delete selected item");
	delete_button->Enable(false);
	actionsSizer->Add(delete_button, 0, wxEXPAND | wxALL, 4);

	select_raw_button = newd wxButton(actionsSizer->GetStaticBox(), wxID_FIND, "Select RAW");
	select_raw_button->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_SEARCH));
	select_raw_button->SetToolTip("Select this item in RAW palette");
	select_raw_button->Enable(false);
	actionsSizer->Add(select_raw_button, 0, wxEXPAND | wxALL, 4);

	rightSplit->Add(actionsSizer, 0, wxEXPAND | wxALL, 4);

	mainSplit->Add(rightSplit, 0, wxEXPAND | wxALL, 0);

	sizer->Add(mainSplit, 1, wxEXPAND | wxALL, 4);

	// OK/Cancel buttons
	wxSizer* btnSizer = newd wxBoxSizer(wxHORIZONTAL);
	auto okBtn = newd wxButton(this, wxID_OK, "OK");
	okBtn->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_CHECK));
	okBtn->SetToolTip("Confirm selection");
	btnSizer->Add(okBtn, 0, wxALL, 4);
	auto cancelBtn = newd wxButton(this, wxID_CANCEL, "Cancel");
	cancelBtn->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_XMARK));
	cancelBtn->SetToolTip("Cancel");
	btnSizer->Add(cancelBtn, 0, wxALL, 4);
	sizer->Add(btnSizer, 0, wxALIGN_RIGHT | wxALL, 4);

	SetSizerAndFit(sizer);

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
