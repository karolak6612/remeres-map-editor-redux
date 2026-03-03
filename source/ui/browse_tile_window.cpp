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
	UpdateItems();
}

BrowseTileListBox::~BrowseTileListBox() {
	////
}

void BrowseTileListBox::OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t n) {
	Item* item = items[n];

	int icon_size = std::min(rect.width, rect.height) - FromDIP(4);
	int icon_x = rect.x + (rect.width - icon_size) / 2;
	int icon_y = rect.y + (rect.height - icon_size) / 2;

	Sprite* sprite = g_gui.gfx.getSprite(item->getClientID());
	if (sprite) {
		int tex = GetOrCreateSpriteTexture(vg, sprite);
		if (tex > 0) {
			NVGpaint imgPaint = nvgImagePattern(vg, icon_x, icon_y, icon_size, icon_size, 0, tex, 1.0f);
			nvgBeginPath(vg);
			nvgRect(vg, icon_x, icon_y, icon_size, icon_size);
			nvgFillPaint(vg, imgPaint);
			nvgFill(vg);
		}
	}

	if (!IsGridMode()) {
		if (IsSelected(n)) {
			wxColour textColour = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);
			nvgFillColor(vg, nvgRGBA(textColour.Red(), textColour.Green(), textColour.Blue(), 255));
		} else {
			wxColour textColour = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT);
			nvgFillColor(vg, nvgRGBA(textColour.Red(), textColour.Green(), textColour.Blue(), 255));
		}

		std::string label = std::format("{} - {}", item->getID(), item->getName());

		nvgFontSize(vg, 12.0f);
		nvgFontFace(vg, "sans");
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgText(vg, rect.x + 40, rect.y + rect.height / 2.0f, label.c_str(), nullptr);
	}
}

int BrowseTileListBox::OnMeasureItem(size_t n) const {
	return m_gridMode ? m_itemWidth : FromDIP(32);
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

	wxBoxSizer* main_sizer = newd wxBoxSizer(wxHORIZONTAL);

	// Left Side: List and item buttons
	wxBoxSizer* left_sizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* items_box = newd wxStaticBoxSizer(wxVERTICAL, this, "Items on Tile");
	item_list = newd BrowseTileListBox(items_box->GetStaticBox(), wxID_ANY, tile);
	item_list->EnableGridMode(FromDIP(48)); // Enable grid layout with thumbnails
	items_box->Add(item_list, 1, wxEXPAND | wxALL, 2);

	wxBoxSizer* items_buttons = newd wxBoxSizer(wxHORIZONTAL);
	delete_button = newd wxButton(items_box->GetStaticBox(), wxID_REMOVE, "Delete");
	delete_button->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_TRASH_CAN));
	delete_button->SetToolTip("Delete selected item");
	delete_button->Enable(false);
	items_buttons->Add(delete_button, 1, wxEXPAND | wxRIGHT, 2);

	select_raw_button = newd wxButton(items_box->GetStaticBox(), wxID_FIND, "Select RAW");
	select_raw_button->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_SEARCH));
	select_raw_button->SetToolTip("Select this item in RAW palette");
	select_raw_button->Enable(false);
	items_buttons->Add(select_raw_button, 1, wxEXPAND | wxLEFT, 2);

	items_box->Add(items_buttons, 0, wxEXPAND | wxALL, 2);
	left_sizer->Add(items_box, 1, wxEXPAND | wxALL, 5);

	main_sizer->Add(left_sizer, 1, wxEXPAND | wxALL, 5);

	// Right Side: Info and Dialog Buttons
	wxBoxSizer* right_sizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* info_box = newd wxStaticBoxSizer(wxVERTICAL, this, "Tile Information");
	wxFlexGridSizer* info_grid = newd wxFlexGridSizer(2, FromDIP(5), FromDIP(15));

	wxString pos;
	pos << "x=" << tile->getX() << ", y=" << tile->getY() << ", z=" << tile->getZ();

	info_grid->Add(newd wxStaticText(info_box->GetStaticBox(), wxID_ANY, "Position:"), 0, wxALIGN_CENTER_VERTICAL);
	info_grid->Add(newd wxStaticText(info_box->GetStaticBox(), wxID_ANY, pos), 0, wxALIGN_CENTER_VERTICAL);

	info_grid->Add(newd wxStaticText(info_box->GetStaticBox(), wxID_ANY, "Item count:"), 0, wxALIGN_CENTER_VERTICAL);
	item_count_txt = newd wxStaticText(info_box->GetStaticBox(), wxID_ANY, i2ws(item_list->GetItemCount()));
	info_grid->Add(item_count_txt, 0, wxALIGN_CENTER_VERTICAL);

	info_grid->Add(newd wxStaticText(info_box->GetStaticBox(), wxID_ANY, "Protection zone:"), 0, wxALIGN_CENTER_VERTICAL);
	info_grid->Add(newd wxStaticText(info_box->GetStaticBox(), wxID_ANY, b2yn(tile->isPZ())), 0, wxALIGN_CENTER_VERTICAL);

	info_grid->Add(newd wxStaticText(info_box->GetStaticBox(), wxID_ANY, "No PvP:"), 0, wxALIGN_CENTER_VERTICAL);
	info_grid->Add(newd wxStaticText(info_box->GetStaticBox(), wxID_ANY, b2yn(tile->getMapFlags() & TILESTATE_NOPVP)), 0, wxALIGN_CENTER_VERTICAL);

	info_grid->Add(newd wxStaticText(info_box->GetStaticBox(), wxID_ANY, "No logout:"), 0, wxALIGN_CENTER_VERTICAL);
	info_grid->Add(newd wxStaticText(info_box->GetStaticBox(), wxID_ANY, b2yn(tile->getMapFlags() & TILESTATE_NOLOGOUT)), 0, wxALIGN_CENTER_VERTICAL);

	info_grid->Add(newd wxStaticText(info_box->GetStaticBox(), wxID_ANY, "PvP zone:"), 0, wxALIGN_CENTER_VERTICAL);
	info_grid->Add(newd wxStaticText(info_box->GetStaticBox(), wxID_ANY, b2yn(tile->getMapFlags() & TILESTATE_PVPZONE)), 0, wxALIGN_CENTER_VERTICAL);

	info_grid->Add(newd wxStaticText(info_box->GetStaticBox(), wxID_ANY, "House:"), 0, wxALIGN_CENTER_VERTICAL);
	info_grid->Add(newd wxStaticText(info_box->GetStaticBox(), wxID_ANY, b2yn(tile->isHouseTile())), 0, wxALIGN_CENTER_VERTICAL);

	info_box->Add(info_grid, 1, wxEXPAND | wxALL, 5);
	right_sizer->Add(info_box, 0, wxEXPAND | wxALL, 5);

	right_sizer->AddStretchSpacer(1);

	// OK/Cancel buttons
	wxStdDialogButtonSizer* btnSizer = newd wxStdDialogButtonSizer();
	auto okBtn = newd wxButton(this, wxID_OK, "OK");
	okBtn->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_CHECK));
	okBtn->SetToolTip("Confirm selection");
	btnSizer->AddButton(okBtn);
	auto cancelBtn = newd wxButton(this, wxID_CANCEL, "Cancel");
	cancelBtn->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_XMARK));
	cancelBtn->SetToolTip("Cancel");
	btnSizer->AddButton(cancelBtn);
	btnSizer->Realize();

	right_sizer->Add(btnSizer, 0, wxALIGN_RIGHT | wxALL, 5);

	main_sizer->Add(right_sizer, 0, wxEXPAND | wxALL, 5);

	SetSizer(main_sizer);
	Layout();

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
	item_count_txt->SetLabelText(i2ws(item_list->GetItemCount()));
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
