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

#include "map/tile.h"
#include "game/item.h"
#include "map/map.h"

#include "ui/tileset_window.h"
#include "util/image_manager.h"

// ============================================================================
// Tileset Window

TilesetWindow::TilesetWindow(wxWindow* win_parent, const Map* map, const Tile* tile_parent, Item* item, wxPoint pos) :
	ObjectPropertiesWindowBase(win_parent, "Move to Tileset", map, tile_parent, item, pos),
	palette_field(nullptr),
	tileset_field(nullptr) {
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);
	topsizer->Add(newd wxStaticText(this, wxID_ANY, "Move To Tileset targets the removed legacy palette model and is disabled for the modular data runtime."),
	              wxSizerFlags(1).Expand().Border(wxALL, 20));

	wxStdDialogButtonSizer* buttonsizer = newd wxStdDialogButtonSizer();
	wxButton* okBtn = newd wxButton(this, wxID_OK, "OK");
	okBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_CHECK, wxSize(16, 16)));
	okBtn->Bind(wxEVT_BUTTON, &TilesetWindow::OnClickCancel, this);
	buttonsizer->AddButton(okBtn);
	buttonsizer->SetAffirmativeButton(okBtn);

	buttonsizer->Realize();
	topsizer->Add(buttonsizer, wxSizerFlags(0).Center().Border(wxALL, 20));

	SetSizerAndFit(topsizer);
	Centre(wxBOTH);

	wxIcon icon;
	icon.CopyFromBitmap(IMAGE_MANAGER.GetBitmap(ICON_SHARE_FROM_SQUARE, wxSize(32, 32)));
	SetIcon(icon);
}

void TilesetWindow::OnChangePalette(wxCommandEvent& WXUNUSED(event)) {
}

void TilesetWindow::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	EndModal(0);
}

void TilesetWindow::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	EndModal(0);
}
