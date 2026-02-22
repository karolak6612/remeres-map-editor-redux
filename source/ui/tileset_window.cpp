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

#include <wx/grid.h>

#include "map/tile.h"
#include "game/item.h"
#include "game/complexitem.h"
#include "game/town.h"
#include "game/house.h"
#include "map/map.h"
#include "editor/editor.h"
#include "game/creature.h"
#include "game/materials.h"
#include "map/tileset.h"

#include "ui/gui.h"
#include "ui/dialog_util.h"
#include "app/application.h"
#include "ui/tileset_window.h"
#include "ui/properties/container_properties_window.h"
#include "util/image_manager.h"

// ============================================================================
// Tileset Window

static constexpr int OUTFIT_COLOR_MAX = 133;

TilesetWindow::TilesetWindow(wxWindow* win_parent, const Map* map, const Tile* tile_parent, Item* item, wxPoint pos) :
	ObjectPropertiesWindowBase(win_parent, "Move to Tileset", map, tile_parent, item, pos),
	palette_field(nullptr),
	tileset_field(nullptr) {
	ASSERT(edit_item);

	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);
	wxString description = "Move to Tileset";

	wxStaticBoxSizer* boxsizer = newd wxStaticBoxSizer(wxVERTICAL, this, description);
	wxWindow* boxParent = boxsizer->GetStaticBox();

	wxFlexGridSizer* subsizer = newd wxFlexGridSizer(2, 10, 10);
	subsizer->AddGrowableCol(1);

	subsizer->Add(newd wxStaticText(boxParent, wxID_ANY, "ID " + i2ws(item->getID())));
	subsizer->Add(newd wxStaticText(boxParent, wxID_ANY, "\"" + wxstr(item->getName()) + "\""));

	subsizer->Add(newd wxStaticText(boxParent, wxID_ANY, "Palette"));
	palette_field = newd wxChoice(boxParent, wxID_ANY);
	palette_field->SetToolTip("Select the palette category");

	palette_field->Append("Terrain", reinterpret_cast<void*>(static_cast<intptr_t>(TILESET_TERRAIN)));
	palette_field->Append("Collections", reinterpret_cast<void*>(static_cast<intptr_t>(TILESET_COLLECTION)));
	palette_field->Append("Doodad", reinterpret_cast<void*>(static_cast<intptr_t>(TILESET_DOODAD)));
	palette_field->Append("Item", reinterpret_cast<void*>(static_cast<intptr_t>(TILESET_ITEM)));
	palette_field->Append("Raw", reinterpret_cast<void*>(static_cast<intptr_t>(TILESET_RAW)));
	palette_field->SetSelection(3);

	palette_field->Bind(wxEVT_CHOICE, &TilesetWindow::OnChangePalette, this);
	subsizer->Add(palette_field, wxSizerFlags(1).Expand());

	subsizer->Add(newd wxStaticText(boxParent, wxID_ANY, "Tileset"));
	tileset_field = newd wxChoice(boxParent, wxID_ANY);
	tileset_field->SetToolTip("Select the target tileset");

	for (const auto& tileset : GetSortedTilesets(g_materials.tilesets)) {
		tileset_field->Append(wxstr(tileset->name));
	}
	tileset_field->SetSelection(0);
	subsizer->Add(tileset_field, wxSizerFlags(1).Expand());

	boxsizer->Add(subsizer, wxSizerFlags(1).Expand());

	topsizer->Add(boxsizer, wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT | wxTOP, 20));

	wxStdDialogButtonSizer* buttonsizer = newd wxStdDialogButtonSizer();
	wxButton* okBtn = newd wxButton(this, wxID_OK, "OK");
	okBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_CHECK, wxSize(16, 16)));
	okBtn->SetToolTip("Confirm changes");
	okBtn->Bind(wxEVT_BUTTON, &TilesetWindow::OnClickOK, this);
	buttonsizer->AddButton(okBtn);
	buttonsizer->SetAffirmativeButton(okBtn);

	wxButton* cancelBtn = newd wxButton(this, wxID_CANCEL, "Cancel");
	cancelBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_XMARK, wxSize(16, 16)));
	cancelBtn->SetToolTip("Discard changes");
	cancelBtn->Bind(wxEVT_BUTTON, &TilesetWindow::OnClickCancel, this);
	buttonsizer->AddButton(cancelBtn);
	buttonsizer->SetCancelButton(cancelBtn);
	buttonsizer->Realize();

	topsizer->Add(buttonsizer, wxSizerFlags(0).Center().Border(wxALL, 20));

	SetSizerAndFit(topsizer);
	Centre(wxBOTH);

	wxIcon icon;
	icon.CopyFromBitmap(IMAGE_MANAGER.GetBitmap(ICON_SHARE_FROM_SQUARE, wxSize(32, 32)));
	SetIcon(icon);
}

void TilesetWindow::OnChangePalette(wxCommandEvent& WXUNUSED(event)) {
	tileset_field->Clear();

	TilesetCategoryType category = static_cast<TilesetCategoryType>(reinterpret_cast<intptr_t>(palette_field->GetClientData(palette_field->GetSelection())));

	for (const auto& tileset : GetSortedTilesets(g_materials.tilesets)) {
		if (!tileset->getCategory(category)->brushlist.empty()) {
			tileset_field->Append(wxstr(tileset->name));
		}
	}

	tileset_field->SetSelection(0);
}

void TilesetWindow::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	if (edit_item) {
		TilesetCategoryType categoryType = static_cast<TilesetCategoryType>(reinterpret_cast<intptr_t>(palette_field->GetClientData(palette_field->GetSelection())));
		std::string tilesetName = tileset_field->GetStringSelection().ToStdString();

		g_materials.addToTileset(tilesetName, edit_item->getID(), categoryType);
		g_gui.SetStatusText("'" + std::string(edit_item->getName()) + "' added to tileset '" + tilesetName + "'");
	}
	EndModal(1);
}

void TilesetWindow::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	// Just close this window
	EndModal(0);
}
