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
#include "ui/theme.h"
#include <format>
#include <nanovg.h>

// ============================================================================
//

class BrowseTileListBox : public NanoVGCanvas {
public:
	BrowseTileListBox(wxWindow* parent, wxWindowID id, Tile* tile);
	~BrowseTileListBox();

	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;
	void OnSize(wxSizeEvent& event);
	void OnMouse(wxMouseEvent& event);

	Item* GetSelectedItem();
	void RemoveSelected();

	void SetSelection(int index);
	void ClearSelection();

	int GetItemCount() const { return items.size(); }
	int GetSelectedCount() const { return m_selection != -1 ? 1 : 0; }

protected:
	void UpdateItems();

	using ItemsMap = std::vector<Item*>;
	ItemsMap items;
	Tile* edit_tile;

	int m_selection = -1;
	int m_columns = 1;
	int m_item_width;
	int m_item_height;
};

BrowseTileListBox::BrowseTileListBox(wxWindow* parent, wxWindowID id, Tile* tile) :
	NanoVGCanvas(parent, id), edit_tile(tile) {
	SetMinSize(FromDIP(wxSize(200, 180)));

	m_item_width = FromDIP(80);
	m_item_height = FromDIP(80);

	Bind(wxEVT_SIZE, &BrowseTileListBox::OnSize, this);
	Bind(wxEVT_LEFT_DOWN, &BrowseTileListBox::OnMouse, this);
	Bind(wxEVT_LEFT_DCLICK, &BrowseTileListBox::OnMouse, this);

	UpdateItems();
}

BrowseTileListBox::~BrowseTileListBox() {
	////
}

void BrowseTileListBox::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	// NanoVGCanvas automatically translates by -GetScrollPosition() on Y axis
	// so we don't need to manually translate it.

	for (size_t n = 0; n < items.size(); ++n) {
		int col = n % m_columns;
		int row = n / m_columns;
		wxRect rect(col * m_item_width, row * m_item_height, m_item_width, m_item_height);

		if (m_selection == n) {
			wxColour textColour = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
			nvgBeginPath(vg);
			nvgRect(vg, rect.x, rect.y, rect.width, rect.height);
			nvgFillColor(vg, nvgRGBA(textColour.Red(), textColour.Green(), textColour.Blue(), 255));
			nvgFill(vg);
		}

		Item* item = items[n];
		Sprite* sprite = g_gui.gfx.getSprite(item->getClientID());
		if (sprite) {
			int tex = GetOrCreateSpriteTexture(vg, sprite);
			if (tex > 0) {
				int icon_size = 32;
				int dx = rect.x + (rect.width - icon_size) / 2;
				int dy = rect.y + FromDIP(8);
				NVGpaint imgPaint = nvgImagePattern(vg, dx, dy, icon_size, icon_size, 0, tex, 1.0f);
				nvgBeginPath(vg);
				nvgRect(vg, dx, dy, icon_size, icon_size);
				nvgFillPaint(vg, imgPaint);
				nvgFill(vg);
			}
		}

		wxColour textColour = (m_selection == n) ? wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT) : Theme::Get(Theme::Role::Text);
		nvgFillColor(vg, nvgRGBA(textColour.Red(), textColour.Green(), textColour.Blue(), 255));

		std::string label = std::format("{} - {}", item->getID(), item->getName());

		nvgFontSize(vg, 11.0f);
		nvgFontFace(vg, "sans");
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
		nvgTextBox(vg, rect.x + FromDIP(4), rect.y + FromDIP(44), rect.width - FromDIP(8), label.c_str(), nullptr);
	}
}

void BrowseTileListBox::OnSize(wxSizeEvent& event) {
	int width = GetClientSize().x;
	m_columns = std::max(1, width / m_item_width);
	int rows = std::ceil((float)items.size() / m_columns);
	SetVirtualSize(wxSize(width, rows * m_item_height));
	event.Skip();
}

void BrowseTileListBox::OnMouse(wxMouseEvent& event) {
	if (event.LeftDown() || event.LeftDClick()) {
		int x = event.GetX();
		int y = event.GetY();

		int scroll_y = GetScrollPosition();
		y += scroll_y;

		int col = x / m_item_width;
		int row = y / m_item_height;
		int index = row * m_columns + col;

		if (col < m_columns && index < items.size()) {
			SetSelection(index);
			if (event.LeftDClick()) {
				wxCommandEvent ev(wxEVT_BUTTON, wxID_FIND);
				ev.SetEventObject(this);
				ProcessWindowEvent(ev);
			} else {
				wxCommandEvent ev(wxEVT_LISTBOX, GetId());
				ev.SetEventObject(this);
				ev.SetInt(index);
				ProcessWindowEvent(ev);
			}
		}
	}
}

Item* BrowseTileListBox::GetSelectedItem() {
	if (items.empty() || m_selection == -1) {
		return nullptr;
	}

	return edit_tile->getTopSelectedItem();
}

void BrowseTileListBox::RemoveSelected() {
	if (items.empty() || m_selection == -1) {
		return;
	}

	ClearSelection();
	items.clear();

	// Delete the items from the tile
	auto tile_selection = edit_tile->popSelectedItems(true);
	// items are automatically deleted when tile_selection goes out of scope

	UpdateItems();
	Refresh();
}

void BrowseTileListBox::SetSelection(int index) {
	if (m_selection != -1 && (size_t)m_selection < items.size()) {
		items[m_selection]->deselect();
	}
	m_selection = index;
	if (m_selection != -1 && (size_t)m_selection < items.size()) {
		items[m_selection]->select();
	}
	Refresh();
}

void BrowseTileListBox::ClearSelection() {
	for (Item* item : items) {
		item->deselect();
	}
	m_selection = -1;
	Refresh();
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

	// Update Virtual Size
	int width = GetClientSize().x;
	m_columns = std::max(1, width / m_item_width);
	int rows = std::ceil((float)items.size() / m_columns);
	SetVirtualSize(wxSize(width, rows * m_item_height));
	SetScrollStep(FromDIP(20));
}

// ============================================================================
//

BrowseTileWindow::BrowseTileWindow(wxWindow* parent, Tile* tile, wxPoint position /* = wxDefaultPosition */) :
	wxDialog(parent, wxID_ANY, "Browse Field", position, FROM_DIP(parent, wxSize(600, 400)), wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER) {
	wxSizer* topSizer = newd wxBoxSizer(wxVERTICAL);
	wxSizer* sizer = newd wxBoxSizer(wxHORIZONTAL);

	item_list = newd BrowseTileListBox(this, wxID_ANY, tile);
	sizer->Add(item_list, wxSizerFlags(1).Expand().Border(wxRIGHT, 5));

	wxString pos;
	pos << "x=" << tile->getX() << ",  y=" << tile->getY() << ",  z=" << tile->getZ();

	wxSizer* infoSizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* actionsBox = newd wxStaticBoxSizer(wxVERTICAL, this, "Actions");
	wxBoxSizer* buttons = newd wxBoxSizer(wxHORIZONTAL);
	delete_button = newd wxButton(actionsBox->GetStaticBox(), wxID_REMOVE, "Delete");
	delete_button->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_TRASH_CAN, wxSize(16, 16)));
	delete_button->SetToolTip("Delete selected item");
	delete_button->Enable(false);
	buttons->Add(delete_button, wxSizerFlags(1).Expand().Border(wxRIGHT, 5));

	select_raw_button = newd wxButton(actionsBox->GetStaticBox(), wxID_FIND, "Select RAW");
	select_raw_button->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_SEARCH, wxSize(16, 16)));
	select_raw_button->SetToolTip("Select this item in RAW palette");
	select_raw_button->Enable(false);
	buttons->Add(select_raw_button, wxSizerFlags(1).Expand());
	actionsBox->Add(buttons, wxSizerFlags(1).Expand().Border(wxALL, 5));
	infoSizer->Add(actionsBox, wxSizerFlags(0).Expand().Border(wxBOTTOM, 10));

	wxStaticBoxSizer* tileInfoBox = newd wxStaticBoxSizer(wxVERTICAL, this, "Tile Information");
	tileInfoBox->Add(newd wxStaticText(tileInfoBox->GetStaticBox(), wxID_ANY, "Position:  " + pos), wxSizerFlags(0).Left().Border(wxALL, 5));
	tileInfoBox->Add(item_count_txt = newd wxStaticText(tileInfoBox->GetStaticBox(), wxID_ANY, "Item count:  " + i2ws(item_list->GetItemCount())), wxSizerFlags(0).Left().Border(wxALL, 5));
	infoSizer->Add(tileInfoBox, wxSizerFlags(0).Expand().Border(wxBOTTOM, 10));

	wxStaticBoxSizer* flagsBox = newd wxStaticBoxSizer(wxVERTICAL, this, "Flags");
	flagsBox->Add(newd wxStaticText(flagsBox->GetStaticBox(), wxID_ANY, "Protection zone:  " + b2yn(tile->isPZ())), wxSizerFlags(0).Left().Border(wxALL, 2));
	flagsBox->Add(newd wxStaticText(flagsBox->GetStaticBox(), wxID_ANY, "No PvP:  " + b2yn(tile->getMapFlags() & TILESTATE_NOPVP)), wxSizerFlags(0).Left().Border(wxALL, 2));
	flagsBox->Add(newd wxStaticText(flagsBox->GetStaticBox(), wxID_ANY, "No logout:  " + b2yn(tile->getMapFlags() & TILESTATE_NOLOGOUT)), wxSizerFlags(0).Left().Border(wxALL, 2));
	flagsBox->Add(newd wxStaticText(flagsBox->GetStaticBox(), wxID_ANY, "PvP zone:  " + b2yn(tile->getMapFlags() & TILESTATE_PVPZONE)), wxSizerFlags(0).Left().Border(wxALL, 2));
	flagsBox->Add(newd wxStaticText(flagsBox->GetStaticBox(), wxID_ANY, "House:  " + b2yn(tile->isHouseTile())), wxSizerFlags(0).Left().Border(wxALL, 2));
	infoSizer->Add(flagsBox, wxSizerFlags(0).Expand());

	sizer->Add(infoSizer, wxSizerFlags(0).Expand().Border(wxALL, 5));
	topSizer->Add(sizer, wxSizerFlags(1).Expand().Border(wxALL, 5));

	// OK/Cancel buttons
	wxSizer* btnSizer = newd wxBoxSizer(wxHORIZONTAL);
	auto okBtn = newd wxButton(this, wxID_OK, "OK");
	okBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_CHECK, wxSize(16, 16)));
	okBtn->SetToolTip("Confirm selection");
	btnSizer->Add(okBtn, wxSizerFlags(0).Center().Border(wxRIGHT, 5));
	auto cancelBtn = newd wxButton(this, wxID_CANCEL, "Cancel");
	cancelBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_XMARK, wxSize(16, 16)));
	cancelBtn->SetToolTip("Cancel");
	btnSizer->Add(cancelBtn, wxSizerFlags(0).Center());
	topSizer->Add(btnSizer, wxSizerFlags(0).Center().DoubleBorder());

	SetSizerAndFit(topSizer);

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
