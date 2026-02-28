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
#include "util/nanovg_canvas.h"
#include "ui/theme.h"
#include "app/settings.h"
#include "game/sprites.h"
#include <glad/glad.h>
#include <format>
#include <nanovg.h>

// ============================================================================
//

class BrowseTileGridCanvas : public NanoVGCanvas {
public:
	BrowseTileGridCanvas(wxWindow* parent, wxWindowID id, Tile* tile);
	~BrowseTileGridCanvas();

	Item* GetSelectedItem();
	void RemoveSelected();

	int GetSelectedCount() const { return m_selection != -1 ? 1 : 0; }
	size_t GetItemCount() const { return items.size(); }

protected:
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;
	wxSize DoGetBestClientSize() const override;

	void DrawSunkenBorder(NVGcontext* vg, float x, float y, float size_x, float size_y);
	void DrawRaisedBorder(NVGcontext* vg, float x, float y, float size_x, float size_y);
	void DrawHoverEffects(NVGcontext* vg, float x, float y, float size_x, float size_y);

	void OnMotion(wxMouseEvent& event);
	void OnLeftDown(wxMouseEvent& event);
	void OnMouseLeave(wxMouseEvent& event);

	int HitTest(int x, int y) const;
	void RecalculateLayout();
	void UpdateItems();

	using ItemsMap = std::vector<Item*>;
	ItemsMap items;
	Tile* edit_tile;

	int m_hover_index;
	int m_selection; // Single selection

	int m_cols;
	int m_rows;
	float m_slot_size;
	float m_padding;
};

BrowseTileGridCanvas::BrowseTileGridCanvas(wxWindow* parent, wxWindowID id, Tile* tile) :
	NanoVGCanvas(parent, id, wxWANTS_CHARS), edit_tile(tile),
	m_hover_index(-1), m_selection(-1),
	m_cols(6), m_rows(1),
	m_slot_size(38.0f), m_padding(1.0f) {

	Bind(wxEVT_MOTION, &BrowseTileGridCanvas::OnMotion, this);
	Bind(wxEVT_LEFT_DOWN, &BrowseTileGridCanvas::OnLeftDown, this);
	Bind(wxEVT_LEAVE_WINDOW, &BrowseTileGridCanvas::OnMouseLeave, this);

	UpdateItems();
}

BrowseTileGridCanvas::~BrowseTileGridCanvas() {
}

void BrowseTileGridCanvas::RecalculateLayout() {
	if (items.empty()) {
		m_rows = 1;
	} else {
		m_rows = (items.size() + m_cols - 1) / m_cols;
	}

	SetMinSize(DoGetBestClientSize());
	if (GetParent()) {
		GetParent()->Layout();
	}
}

wxSize BrowseTileGridCanvas::DoGetBestClientSize() const {
	int width = static_cast<int>(std::ceil(m_cols * m_slot_size));
	int height = static_cast<int>(std::ceil(m_rows * m_slot_size));
	return FromDIP(wxSize(width, height));
}

int BrowseTileGridCanvas::HitTest(int x, int y) const {
	if (items.empty()) {
		return -1;
	}

	float dip_x = x / GetDPIScaleFactor();
	float dip_y = y / GetDPIScaleFactor();

	int col = static_cast<int>(std::floor(dip_x / m_slot_size));
	int row = static_cast<int>(std::floor(dip_y / m_slot_size));

	if (col >= 0 && col < m_cols && row >= 0 && row < m_rows) {
		int index = row * m_cols + col;
		if (index < static_cast<int>(items.size())) {
			return index;
		}
	}

	return -1;
}

void BrowseTileGridCanvas::OnMotion(wxMouseEvent& event) {
	int hit = HitTest(event.GetX(), event.GetY());
	if (hit != m_hover_index) {
		m_hover_index = hit;

		if (hit != -1 && hit < static_cast<int>(items.size())) {
			Item* item = items[hit];
			SetToolTip(std::format("{} - {}", item->getID(), item->getName()));
		} else {
			SetToolTip("");
		}

		Refresh();
	}
}

void BrowseTileGridCanvas::OnMouseLeave(wxMouseEvent& WXUNUSED(event)) {
	if (m_hover_index != -1) {
		m_hover_index = -1;
		Refresh();
	}
}

void BrowseTileGridCanvas::OnLeftDown(wxMouseEvent& event) {
	int hit = HitTest(event.GetX(), event.GetY());
	if (hit != -1) {
		if (m_selection != -1 && m_selection < static_cast<int>(items.size())) {
			items[m_selection]->deselect();
		}

		m_selection = hit;

		if (m_selection != -1 && m_selection < static_cast<int>(items.size())) {
			items[m_selection]->select();
		}

		Refresh();

		wxCommandEvent clickEvent(wxEVT_LISTBOX, GetId());
		clickEvent.SetEventObject(this);
		clickEvent.SetInt(hit);
		ProcessWindowEvent(clickEvent);
	}
}

void BrowseTileGridCanvas::DrawSunkenBorder(NVGcontext* vg, float x, float y, float size_x, float size_y) {
	NVGcolor dark_highlight = nvgRGBA(212, 208, 200, 255);
	NVGcolor light_shadow = nvgRGBA(128, 128, 128, 255);
	NVGcolor highlight = nvgRGBA(255, 255, 255, 255);
	NVGcolor shadow = nvgRGBA(64, 64, 64, 255);

	nvgStrokeWidth(vg, 1.0f);

	nvgBeginPath(vg);
	nvgMoveTo(vg, x + 0.5f, y + size_y - 0.5f);
	nvgLineTo(vg, x + 0.5f, y + 0.5f);
	nvgLineTo(vg, x + size_x - 0.5f, y + 0.5f);
	nvgStrokeColor(vg, shadow);
	nvgStroke(vg);

	nvgBeginPath(vg);
	nvgMoveTo(vg, x + 1.5f, y + size_y - 1.5f);
	nvgLineTo(vg, x + 1.5f, y + 1.5f);
	nvgLineTo(vg, x + size_x - 1.5f, y + 1.5f);
	nvgStrokeColor(vg, light_shadow);
	nvgStroke(vg);

	nvgBeginPath(vg);
	nvgMoveTo(vg, x + size_x - 1.5f, y + 1.5f);
	nvgLineTo(vg, x + size_x - 1.5f, y + size_y - 1.5f);
	nvgLineTo(vg, x + 1.5f, y + size_y - 1.5f);
	nvgStrokeColor(vg, dark_highlight);
	nvgStroke(vg);

	nvgBeginPath(vg);
	nvgMoveTo(vg, x + size_x - 0.5f, y + 0.5f);
	nvgLineTo(vg, x + size_x - 0.5f, y + size_y - 0.5f);
	nvgLineTo(vg, x + 0.5f, y + size_y - 0.5f);
	nvgStrokeColor(vg, highlight);
	nvgStroke(vg);
}

void BrowseTileGridCanvas::DrawRaisedBorder(NVGcontext* vg, float x, float y, float size_x, float size_y) {
	NVGcolor dark_highlight = nvgRGBA(212, 208, 200, 255);
	NVGcolor light_shadow = nvgRGBA(128, 128, 128, 255);
	NVGcolor highlight = nvgRGBA(255, 255, 255, 255);
	NVGcolor shadow = nvgRGBA(64, 64, 64, 255);

	nvgStrokeWidth(vg, 1.0f);

	nvgBeginPath(vg);
	nvgMoveTo(vg, x + 0.5f, y + size_y - 0.5f);
	nvgLineTo(vg, x + 0.5f, y + 0.5f);
	nvgLineTo(vg, x + size_x - 0.5f, y + 0.5f);
	nvgStrokeColor(vg, highlight);
	nvgStroke(vg);

	nvgBeginPath(vg);
	nvgMoveTo(vg, x + 1.5f, y + size_y - 1.5f);
	nvgLineTo(vg, x + 1.5f, y + 1.5f);
	nvgLineTo(vg, x + size_x - 1.5f, y + 1.5f);
	nvgStrokeColor(vg, dark_highlight);
	nvgStroke(vg);

	nvgBeginPath(vg);
	nvgMoveTo(vg, x + size_x - 1.5f, y + 1.5f);
	nvgLineTo(vg, x + size_x - 1.5f, y + size_y - 1.5f);
	nvgLineTo(vg, x + 1.5f, y + size_y - 1.5f);
	nvgStrokeColor(vg, light_shadow);
	nvgStroke(vg);

	nvgBeginPath(vg);
	nvgMoveTo(vg, x + size_x - 0.5f, y + 0.5f);
	nvgLineTo(vg, x + size_x - 0.5f, y + size_y - 0.5f);
	nvgLineTo(vg, x + 0.5f, y + size_y - 0.5f);
	nvgStrokeColor(vg, shadow);
	nvgStroke(vg);
}

void BrowseTileGridCanvas::DrawHoverEffects(NVGcontext* vg, float x, float y, float size_x, float size_y) {
	nvgBeginPath(vg);
	nvgRect(vg, x, y, size_x, size_y);
	wxColour hoverCol = Theme::Get(Theme::Role::AccentHover);
	nvgFillColor(vg, nvgRGBA(hoverCol.Red(), hoverCol.Green(), hoverCol.Blue(), 64)); // Add some transparency
	nvgFill(vg);
}


void BrowseTileGridCanvas::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	if (g_gui.gfx.isUnloaded()) {
		return;
	}

	int capacity = static_cast<int>(items.size());

	// Fill background
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, width, height);
	// We use the panel background color to blend seamlessly
	wxColour bgColour = GetParent()->GetBackgroundColour();
	nvgFillColor(vg, nvgRGBA(bgColour.Red(), bgColour.Green(), bgColour.Blue(), 255));
	nvgFill(vg);

	int img_size = 32;
	float btn_size_x = m_slot_size - m_padding * 2;
	float btn_size_y = m_slot_size - m_padding * 2;

	float offset_x = (btn_size_x - img_size) / 2.0f;
	float offset_y = (btn_size_y - img_size) / 2.0f;

	for (int index = 0; index < capacity; ++index) {
		int col = index % m_cols;
		int row = index / m_cols;

		float slot_x = col * m_slot_size + m_padding;
		float slot_y = row * m_slot_size + m_padding;

		// 1. Draw Slot Background (Black DCButton style backdrop)
		nvgBeginPath(vg);
		nvgRect(vg, slot_x, slot_y, btn_size_x, btn_size_y);
		nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
		nvgFill(vg);

		bool is_selected = (index == m_selection);
		bool is_hovered = (index == m_hover_index);

		// 2. Draw Borders (Raised unselected, Sunken selected)
		if (is_selected) {
			DrawSunkenBorder(vg, slot_x, slot_y, btn_size_x, btn_size_y);
		} else {
			DrawRaisedBorder(vg, slot_x, slot_y, btn_size_x, btn_size_y);
		}

		if (is_hovered && !is_selected) {
			DrawHoverEffects(vg, slot_x, slot_y, btn_size_x, btn_size_y);
		}

		// 3. Draw Sprite content if available
		Item* item = items[index];
		if (item) {
			Sprite* sprite = g_gui.gfx.getSprite(item->getClientID());
			if (sprite) {
				int tex = GetOrCreateSpriteTexture(vg, sprite);
				if (tex > 0) {
					NVGpaint imgPaint = nvgImagePattern(vg, slot_x + offset_x, slot_y + offset_y, img_size, img_size, 0, tex, 1.0f);
					nvgBeginPath(vg);
					nvgRect(vg, slot_x + offset_x, slot_y + offset_y, img_size, img_size);
					nvgFillPaint(vg, imgPaint);
					nvgFill(vg);

					// Overlays for selected
					if (is_selected && g_settings.getInteger(Config::USE_GUI_SELECTION_SHADOW)) {
						Sprite* overlay = g_gui.gfx.getSprite(EDITOR_SPRITE_SELECTION_MARKER);
						if (overlay) {
							int overlayTex = GetOrCreateSpriteTexture(vg, overlay);
							if (overlayTex > 0) {
								NVGpaint ovPaint = nvgImagePattern(vg, slot_x + offset_x, slot_y + offset_y, img_size, img_size, 0, overlayTex, 1.0f);
								nvgBeginPath(vg);
								nvgRect(vg, slot_x + offset_x, slot_y + offset_y, img_size, img_size);
								nvgFillPaint(vg, ovPaint);
								nvgFill(vg);
							}
						}
					}

					// Draw Count if > 1
					if (item->getCount() > 1 && item->isStackable()) {
						std::string countStr = std::to_string(item->getCount());
						nvgFontSize(vg, 10.0f);
						nvgFontFace(vg, "sans");
						nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);

						float text_x = slot_x + btn_size_x - offset_x + 1.0f;
						float text_y = slot_y + btn_size_y - offset_y + 1.0f;

						// Shadow
						nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
						nvgText(vg, text_x + 1.0f, text_y + 1.0f, countStr.c_str(), nullptr);

						// Text
						wxColour countCol = Theme::Get(Theme::Role::TooltipCountText);
						nvgFillColor(vg, nvgRGBA(countCol.Red(), countCol.Green(), countCol.Blue(), 255));
						nvgText(vg, text_x, text_y, countStr.c_str(), nullptr);
					}
				}
			}
		}
	}
}

Item* BrowseTileGridCanvas::GetSelectedItem() {
	if (items.empty() || m_selection == -1) {
		return nullptr;
	}

	return edit_tile->getTopSelectedItem();
}

void BrowseTileGridCanvas::RemoveSelected() {
	if (items.empty() || m_selection == -1) {
		return;
	}

	m_selection = -1;
	items.clear();

	// Delete the items from the tile
	auto tile_selection = edit_tile->popSelectedItems(true);
	// items are automatically deleted when tile_selection goes out of scope

	UpdateItems();
	Refresh();
}


void BrowseTileGridCanvas::UpdateItems() {
	items.clear();
	items.reserve(edit_tile->items.size() + (edit_tile->ground ? 1 : 0));
	for (auto it = edit_tile->items.rbegin(); it != edit_tile->items.rend(); ++it) {
		items.push_back(it->get());
	}

	if (edit_tile->ground) {
		items.push_back(edit_tile->ground.get());
	}

	RecalculateLayout();
}

// ============================================================================
//

BrowseTileWindow::BrowseTileWindow(wxWindow* parent, Tile* tile, wxPoint position /* = wxDefaultPosition */) :
	wxDialog(parent, wxID_ANY, "Browse Field", position, FROM_DIP(parent, wxSize(600, 400)), wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER) {

	wxSizer* mainSizer = newd wxBoxSizer(wxVERTICAL);
	wxSizer* contentSizer = newd wxBoxSizer(wxHORIZONTAL);

	// Left: Scrolled Grid
	wxScrolledWindow* scrollWindow = newd wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxHSCROLL);
	scrollWindow->SetScrollRate(FromDIP(10), FromDIP(10));

	wxSizer* gridSizer = newd wxBoxSizer(wxVERTICAL);
	item_grid = newd BrowseTileGridCanvas(scrollWindow, wxID_ANY, tile);
	gridSizer->Add(item_grid, wxSizerFlags(1).Expand());
	scrollWindow->SetSizer(gridSizer);

	contentSizer->Add(scrollWindow, wxSizerFlags(1).Expand().Border(wxALL, FromDIP(5)));

	// Right: Info
	wxStaticBoxSizer* infoBoxSizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Tile Information");

	wxString pos;
	pos << "x=" << tile->getX() << ",  y=" << tile->getY() << ",  z=" << tile->getZ();

	infoBoxSizer->Add(newd wxStaticText(infoBoxSizer->GetStaticBox(), wxID_ANY, "Position:  " + pos), wxSizerFlags(0).Left().Border(wxBOTTOM, FromDIP(5)));
	infoBoxSizer->Add(item_count_txt = newd wxStaticText(infoBoxSizer->GetStaticBox(), wxID_ANY, "Item count:  " + i2ws(item_grid->GetItemCount())), wxSizerFlags(0).Left().Border(wxBOTTOM, FromDIP(5)));
	infoBoxSizer->Add(newd wxStaticText(infoBoxSizer->GetStaticBox(), wxID_ANY, "Protection zone:  " + b2yn(tile->isPZ())), wxSizerFlags(0).Left().Border(wxBOTTOM, FromDIP(5)));
	infoBoxSizer->Add(newd wxStaticText(infoBoxSizer->GetStaticBox(), wxID_ANY, "No PvP:  " + b2yn(tile->getMapFlags() & TILESTATE_NOPVP)), wxSizerFlags(0).Left().Border(wxBOTTOM, FromDIP(5)));
	infoBoxSizer->Add(newd wxStaticText(infoBoxSizer->GetStaticBox(), wxID_ANY, "No logout:  " + b2yn(tile->getMapFlags() & TILESTATE_NOLOGOUT)), wxSizerFlags(0).Left().Border(wxBOTTOM, FromDIP(5)));
	infoBoxSizer->Add(newd wxStaticText(infoBoxSizer->GetStaticBox(), wxID_ANY, "PvP zone:  " + b2yn(tile->getMapFlags() & TILESTATE_PVPZONE)), wxSizerFlags(0).Left().Border(wxBOTTOM, FromDIP(5)));
	infoBoxSizer->Add(newd wxStaticText(infoBoxSizer->GetStaticBox(), wxID_ANY, "House:  " + b2yn(tile->isHouseTile())), wxSizerFlags(0).Left().Border(wxBOTTOM, FromDIP(5)));

	infoBoxSizer->AddStretchSpacer(1);

	// Actions
	wxBoxSizer* buttons = newd wxBoxSizer(wxHORIZONTAL);
	delete_button = newd wxButton(infoBoxSizer->GetStaticBox(), wxID_REMOVE, "Delete");
	delete_button->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_TRASH_CAN, wxSize(16, 16)));
	delete_button->SetToolTip("Delete selected item");
	delete_button->Enable(false);
	buttons->Add(delete_button, wxSizerFlags(0).Border(wxRIGHT, FromDIP(5)));

	select_raw_button = newd wxButton(infoBoxSizer->GetStaticBox(), wxID_FIND, "Select RAW");
	select_raw_button->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_SEARCH, wxSize(16, 16)));
	select_raw_button->SetToolTip("Select this item in RAW palette");
	select_raw_button->Enable(false);
	buttons->Add(select_raw_button, wxSizerFlags(0));

	infoBoxSizer->Add(buttons, wxSizerFlags(0).Expand().Border(wxTOP, FromDIP(10)));

	contentSizer->Add(infoBoxSizer, wxSizerFlags(0).Expand().Border(wxALL, FromDIP(5)));

	mainSizer->Add(contentSizer, wxSizerFlags(1).Expand());

	// OK/Cancel buttons
	wxSizer* btnSizer = newd wxBoxSizer(wxHORIZONTAL);
	auto okBtn = newd wxButton(this, wxID_OK, "OK");
	okBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_CHECK, wxSize(16, 16)));
	okBtn->SetToolTip("Confirm selection");
	btnSizer->Add(okBtn, wxSizerFlags(0).Center().Border(wxRIGHT, FromDIP(5)));
	auto cancelBtn = newd wxButton(this, wxID_CANCEL, "Cancel");
	cancelBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_XMARK, wxSize(16, 16)));
	cancelBtn->SetToolTip("Cancel");
	btnSizer->Add(cancelBtn, wxSizerFlags(0).Center());

	mainSizer->Add(btnSizer, wxSizerFlags(0).Center().Border(wxALL, FromDIP(10)));

	SetSizerAndFit(mainSizer);

	// Connect Events
	item_grid->Bind(wxEVT_LISTBOX, &BrowseTileWindow::OnItemSelected, this);
	delete_button->Bind(wxEVT_BUTTON, &BrowseTileWindow::OnClickDelete, this);
	select_raw_button->Bind(wxEVT_BUTTON, &BrowseTileWindow::OnClickSelectRaw, this);
	Bind(wxEVT_BUTTON, &BrowseTileWindow::OnClickOK, this, wxID_OK);
	Bind(wxEVT_BUTTON, &BrowseTileWindow::OnClickCancel, this, wxID_CANCEL);
}

BrowseTileWindow::~BrowseTileWindow() {
}

void BrowseTileWindow::OnItemSelected(wxCommandEvent& WXUNUSED(event)) {
	const size_t count = item_grid->GetSelectedCount();
	delete_button->Enable(count != 0);
	select_raw_button->Enable(count == 1);
}

void BrowseTileWindow::OnClickDelete(wxCommandEvent& WXUNUSED(event)) {
	item_grid->RemoveSelected();
	item_count_txt->SetLabelText("Item count:  " + i2ws(item_grid->GetItemCount()));
}

void BrowseTileWindow::OnClickSelectRaw(wxCommandEvent& WXUNUSED(event)) {
	Item* item = item_grid->GetSelectedItem();
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
