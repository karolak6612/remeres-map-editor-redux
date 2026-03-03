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
#include "ui/find_item_window.h"
#include "ui/dialogs/find_dialog.h"
#include "ui/controls/sortable_list_box.h"
#include "ui/gui.h"
#include "game/items.h"
#include "brushes/brush.h"
#include "brushes/raw/raw_brush.h"
#include "util/image_manager.h"
#include <wx/choice.h>

FindItemDialog::FindItemDialog(wxWindow* parent, const wxString& title, bool onlyPickupables /* = false*/) :
	wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600), wxDEFAULT_DIALOG_STYLE),
	input_timer(this),
	result_brush(nullptr),
	result_id(0),
	only_pickupables(onlyPickupables) {
	this->SetSizeHints(wxDefaultSize, wxDefaultSize);

	wxBoxSizer* box_sizer = newd wxBoxSizer(wxHORIZONTAL);

	wxBoxSizer* left_sizer = newd wxBoxSizer(wxVERTICAL);

	notebook = newd wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP);

	// Server ID Page
	wxPanel* server_id_panel = newd wxPanel(notebook);
	wxBoxSizer* server_id_sizer = newd wxBoxSizer(wxVERTICAL);
	server_id_spin = newd wxSpinCtrl(server_id_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 100, g_items.getMaxID(), 100);
	server_id_spin->SetToolTip("Search by server ID");
	server_id_sizer->Add(server_id_spin, 0, wxALL | wxEXPAND, 5);
	invalid_item = newd wxCheckBox(server_id_panel, wxID_ANY, "Force select");
	invalid_item->SetToolTip("Force choose item ID that does not appear on the list.");
	server_id_sizer->Add(invalid_item, 0, wxALL | wxEXPAND, 5);
	server_id_panel->SetSizer(server_id_sizer);
	notebook->AddPage(server_id_panel, "Server ID", true);

	// Client ID Page
	wxPanel* client_id_panel = newd wxPanel(notebook);
	wxBoxSizer* client_id_sizer = newd wxBoxSizer(wxVERTICAL);
	client_id_spin = newd wxSpinCtrl(client_id_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 100, g_gui.gfx.getItemSpriteMaxID(), 100);
	client_id_spin->SetToolTip("Search by client ID");
	client_id_sizer->Add(client_id_spin, 0, wxALL | wxEXPAND, 5);
	client_id_panel->SetSizer(client_id_sizer);
	notebook->AddPage(client_id_panel, "Client ID");

	// Name Page
	wxPanel* name_panel = newd wxPanel(notebook);
	wxBoxSizer* name_sizer = newd wxBoxSizer(wxVERTICAL);
	name_text_input = newd wxTextCtrl(name_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	name_text_input->SetToolTip("Search by item name (requires 2+ characters)");
	name_sizer->Add(name_text_input, 0, wxALL | wxEXPAND, 5);
	name_panel->SetSizer(name_sizer);
	notebook->AddPage(name_panel, "Name");

	// Types Page
	wxPanel* types_panel = newd wxPanel(notebook);
	wxBoxSizer* types_sizer = newd wxBoxSizer(wxVERTICAL);
	wxString types_choices[] = { "Depot", "Mailbox", "Trash Holder", "Container", "Door", "Magic Field", "Teleport", "Bed", "Key", "Podium" };
	int types_choices_count = sizeof(types_choices) / sizeof(wxString);
	types_choice = newd wxChoice(types_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, types_choices_count, types_choices);
	types_choice->SetSelection(0);
	types_sizer->Add(types_choice, 0, wxALL | wxEXPAND, 5);
	types_panel->SetSizer(types_sizer);
	notebook->AddPage(types_panel, "Types");

	// Properties Page
	wxPanel* props_panel = newd wxPanel(notebook);
	wxFlexGridSizer* props_sizer = newd wxFlexGridSizer(0, 3, FromDIP(5), FromDIP(5));

	unpassable = newd wxCheckBox(props_panel, wxID_ANY, "Unpassable");
	unmovable = newd wxCheckBox(props_panel, wxID_ANY, "Unmovable");
	block_missiles = newd wxCheckBox(props_panel, wxID_ANY, "Block Missiles");
	block_pathfinder = newd wxCheckBox(props_panel, wxID_ANY, "Block Pathfinder");
	readable = newd wxCheckBox(props_panel, wxID_ANY, "Readable");
	writeable = newd wxCheckBox(props_panel, wxID_ANY, "Writeable");
	pickupable = newd wxCheckBox(props_panel, wxID_ANY, "Pickupable");
	stackable = newd wxCheckBox(props_panel, wxID_ANY, "Stackable");
	rotatable = newd wxCheckBox(props_panel, wxID_ANY, "Rotatable");
	hangable = newd wxCheckBox(props_panel, wxID_ANY, "Hangable");
	hook_east = newd wxCheckBox(props_panel, wxID_ANY, "Hook East");
	hook_south = newd wxCheckBox(props_panel, wxID_ANY, "Hook South");
	has_elevation = newd wxCheckBox(props_panel, wxID_ANY, "Has Elevation");
	ignore_look = newd wxCheckBox(props_panel, wxID_ANY, "Ignore Look");
	floor_change = newd wxCheckBox(props_panel, wxID_ANY, "Floor Change");

	pickupable->SetValue(only_pickupables);
	if (only_pickupables) pickupable->Enable(false);

	props_sizer->Add(unpassable);
	props_sizer->Add(unmovable);
	props_sizer->Add(block_missiles);
	props_sizer->Add(block_pathfinder);
	props_sizer->Add(readable);
	props_sizer->Add(writeable);
	props_sizer->Add(pickupable);
	props_sizer->Add(stackable);
	props_sizer->Add(rotatable);
	props_sizer->Add(hangable);
	props_sizer->Add(hook_east);
	props_sizer->Add(hook_south);
	props_sizer->Add(has_elevation);
	props_sizer->Add(ignore_look);
	props_sizer->Add(floor_change);

	props_panel->SetSizer(props_sizer);
	notebook->AddPage(props_panel, "Properties");

	left_sizer->Add(notebook, 1, wxEXPAND | wxALL, 5);

	// Buttons
	buttons_box_sizer = newd wxStdDialogButtonSizer();
	ok_button = newd wxButton(this, wxID_OK);
	ok_button->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_CHECK));
	ok_button->SetToolTip("Select an item to confirm");
	buttons_box_sizer->AddButton(ok_button);
	cancel_button = newd wxButton(this, wxID_CANCEL);
	cancel_button->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_XMARK));
	cancel_button->SetToolTip("Cancel selection");
	buttons_box_sizer->AddButton(cancel_button);
	buttons_box_sizer->Realize();
	left_sizer->Add(buttons_box_sizer, 0, wxALIGN_CENTER | wxALL, 5);

	box_sizer->Add(left_sizer, 0, wxEXPAND | wxALL, 5);

	// --------------- Items list ---------------

	wxStaticBoxSizer* result_box_sizer = newd wxStaticBoxSizer(newd wxStaticBox(this, wxID_ANY, "Result"), wxVERTICAL);
	items_list = newd FindDialogListBox(result_box_sizer->GetStaticBox(), wxID_ANY);
	items_list->SetMinSize(FromDIP(wxSize(300, 512)));
	items_list->EnableGridMode(FromDIP(40)); // Enable grid mode with 40px item width
	result_box_sizer->Add(items_list, 1, wxEXPAND | wxALL, 5);
	box_sizer->Add(result_box_sizer, 1, wxEXPAND | wxALL, 5);

	this->SetSizer(box_sizer);
	this->Layout();
	this->Centre(wxBOTH);
	this->EnableProperties(false);
	this->RefreshContentsInternal();

	SetIcons(IMAGE_MANAGER.GetIconBundle(ICON_SEARCH));

	// Connect Events
	notebook->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, &FindItemDialog::OnTabChange, this);
	server_id_spin->Bind(wxEVT_COMMAND_SPINCTRL_UPDATED, &FindItemDialog::OnServerIdChange, this);
	server_id_spin->Bind(wxEVT_COMMAND_TEXT_UPDATED, &FindItemDialog::OnServerIdChange, this);
	client_id_spin->Bind(wxEVT_COMMAND_SPINCTRL_UPDATED, &FindItemDialog::OnClientIdChange, this);
	client_id_spin->Bind(wxEVT_COMMAND_TEXT_UPDATED, &FindItemDialog::OnClientIdChange, this);
	name_text_input->Bind(wxEVT_COMMAND_TEXT_UPDATED, &FindItemDialog::OnText, this);

	types_choice->Bind(wxEVT_CHOICE, &FindItemDialog::OnTypeChange, this);

	unpassable->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindItemDialog::OnPropertyChange, this);
	unmovable->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindItemDialog::OnPropertyChange, this);
	block_missiles->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindItemDialog::OnPropertyChange, this);
	block_pathfinder->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindItemDialog::OnPropertyChange, this);
	readable->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindItemDialog::OnPropertyChange, this);
	writeable->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindItemDialog::OnPropertyChange, this);
	pickupable->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindItemDialog::OnPropertyChange, this);
	stackable->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindItemDialog::OnPropertyChange, this);
	rotatable->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindItemDialog::OnPropertyChange, this);
	hangable->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindItemDialog::OnPropertyChange, this);
	hook_east->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindItemDialog::OnPropertyChange, this);
	hook_south->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindItemDialog::OnPropertyChange, this);
	has_elevation->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindItemDialog::OnPropertyChange, this);
	ignore_look->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindItemDialog::OnPropertyChange, this);
	floor_change->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindItemDialog::OnPropertyChange, this);
	invalid_item->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &FindItemDialog::OnPropertyChange, this);

	input_timer.Bind(wxEVT_TIMER, &FindItemDialog::OnInputTimer, this);
	ok_button->Bind(wxEVT_BUTTON, &FindItemDialog::OnClickOK, this);
	cancel_button->Bind(wxEVT_BUTTON, &FindItemDialog::OnClickCancel, this);
}

FindItemDialog::~FindItemDialog() {
	// Events are automatically unbound when the object is destroyed or when Bind target is destroyed.
}

FindItemDialog::SearchMode FindItemDialog::getSearchMode() const {
	return (SearchMode)notebook->GetSelection();
}

void FindItemDialog::setSearchMode(FindItemDialog::SearchMode mode) {
	if ((SearchMode)notebook->GetSelection() != mode) {
		notebook->SetSelection(mode);
	}

	RefreshContentsInternal();

	if (mode == SearchMode::ServerIDs) {
		server_id_spin->SetFocus();
		server_id_spin->SetSelection(-1, -1);
	} else if (mode == SearchMode::ClientIDs) {
		client_id_spin->SetFocus();
		client_id_spin->SetSelection(-1, -1);
	} else if (mode == SearchMode::Names) {
		name_text_input->SetFocus();
	}
}

void FindItemDialog::EnableProperties(bool enable) {
	unpassable->Enable(enable);
	unmovable->Enable(enable);
	block_missiles->Enable(enable);
	block_pathfinder->Enable(enable);
	readable->Enable(enable);
	writeable->Enable(enable);
	pickupable->Enable(!only_pickupables && enable);
	stackable->Enable(enable);
	rotatable->Enable(enable);
	hangable->Enable(enable);
	hook_east->Enable(enable);
	hook_south->Enable(enable);
	has_elevation->Enable(enable);
	ignore_look->Enable(enable);
	floor_change->Enable(enable);
}

void FindItemDialog::RefreshContentsInternal() {
	items_list->Clear();
	ok_button->Enable(false);
	ok_button->SetToolTip("Select an item to continue");

	SearchMode selection = (SearchMode)notebook->GetSelection();
	bool found_search_results = false;

	if (selection == SearchMode::ServerIDs) {
		result_id = std::min(server_id_spin->GetValue(), 0xFFFF);
		uint16_t serverID = static_cast<uint16_t>(result_id);
		if (serverID <= g_items.getMaxID()) {
			ItemType& item = g_items.getItemType(serverID);
			RAWBrush* raw_brush = item.raw_brush;
			if (raw_brush) {
				if (only_pickupables) {
					if (item.pickupable) {
						found_search_results = true;
						items_list->AddBrush(raw_brush);
					}
				} else {
					found_search_results = true;
					items_list->AddBrush(raw_brush);
				}
			}
		}

		if (invalid_item->GetValue()) {
			found_search_results = true;
		}
	} else if (selection == SearchMode::ClientIDs) {
		uint16_t clientID = (uint16_t)client_id_spin->GetValue();
		for (int id = 100; id <= g_items.getMaxID(); ++id) {
			ItemType& item = g_items.getItemType(id);
			if (item.id == 0 || item.clientID != clientID) {
				continue;
			}

			RAWBrush* raw_brush = item.raw_brush;
			if (!raw_brush) {
				continue;
			}

			if (only_pickupables && !item.pickupable) {
				continue;
			}

			found_search_results = true;
			items_list->AddBrush(raw_brush);
		}
	} else if (selection == SearchMode::Names) {
		std::string search_string = as_lower_str(nstr(name_text_input->GetValue()));
		if (search_string.size() >= 2) {
			for (int id = 100; id <= g_items.getMaxID(); ++id) {
				ItemType& item = g_items.getItemType(id);
				if (item.id == 0) {
					continue;
				}

				RAWBrush* raw_brush = item.raw_brush;
				if (!raw_brush) {
					continue;
				}

				if (only_pickupables && !item.pickupable) {
					continue;
				}

				if (as_lower_str(raw_brush->getName()).find(search_string) == std::string::npos) {
					continue;
				}

				found_search_results = true;
				items_list->AddBrush(raw_brush);
			}
		}
	} else if (selection == SearchMode::Types) {
		for (int id = 100; id <= g_items.getMaxID(); ++id) {
			ItemType& item = g_items.getItemType(id);
			if (item.id == 0) {
				continue;
			}

			RAWBrush* raw_brush = item.raw_brush;
			if (!raw_brush) {
				continue;
			}

			if (only_pickupables && !item.pickupable) {
				continue;
			}

			SearchItemType selection = (SearchItemType)types_choice->GetSelection();
			if ((selection == SearchItemType::Depot && !item.isDepot()) || (selection == SearchItemType::Mailbox && !item.isMailbox()) || (selection == SearchItemType::TrashHolder && !item.isTrashHolder()) || (selection == SearchItemType::Container && !item.isContainer()) || (selection == SearchItemType::Door && !item.isDoor()) || (selection == SearchItemType::MagicField && !item.isMagicField()) || (selection == SearchItemType::Teleport && !item.isTeleport()) || (selection == SearchItemType::Bed && !item.isBed()) || (selection == SearchItemType::Key && !item.isKey()) || (selection == SearchItemType::Podium && !item.isPodium())) {
				continue;
			}

			found_search_results = true;
			items_list->AddBrush(raw_brush);
		}
	} else if (selection == SearchMode::Properties) {
		bool has_selected = (unpassable->GetValue() || unmovable->GetValue() || block_missiles->GetValue() || block_pathfinder->GetValue() || readable->GetValue() || writeable->GetValue() || pickupable->GetValue() || stackable->GetValue() || rotatable->GetValue() || hangable->GetValue() || hook_east->GetValue() || hook_south->GetValue() || has_elevation->GetValue() || ignore_look->GetValue() || floor_change->GetValue());

		if (has_selected) {
			for (int id = 100; id <= g_items.getMaxID(); ++id) {
				ItemType& item = g_items.getItemType(id);
				if (item.id == 0) {
					continue;
				}

				RAWBrush* raw_brush = item.raw_brush;
				if (!raw_brush) {
					continue;
				}

				if ((unpassable->GetValue() && !item.unpassable) || (unmovable->GetValue() && item.moveable) || (block_missiles->GetValue() && !item.blockMissiles) || (block_pathfinder->GetValue() && !item.blockPathfinder) || (readable->GetValue() && !item.canReadText) || (writeable->GetValue() && !item.canWriteText) || (pickupable->GetValue() && !item.pickupable) || (stackable->GetValue() && !item.stackable) || (rotatable->GetValue() && !item.rotable) || (hangable->GetValue() && !item.isHangable) || (hook_east->GetValue() && !item.hookEast) || (hook_south->GetValue() && !item.hookSouth) || (has_elevation->GetValue() && !item.hasElevation) || (ignore_look->GetValue() && !item.ignoreLook) || (floor_change->GetValue() && !item.isFloorChange())) {
					continue;
				}

				found_search_results = true;
				items_list->AddBrush(raw_brush);
			}
		}
	}

	if (found_search_results) {
		items_list->SetSelection(0);
		ok_button->Enable(true);
		ok_button->SetToolTip("Confirm selection");
	} else {
		items_list->SetNoMatches();
	}

	items_list->Refresh();
}

void FindItemDialog::OnTabChange(wxBookCtrlEvent& WXUNUSED(event)) {
	setSearchMode((SearchMode)notebook->GetSelection());
}

void FindItemDialog::OnServerIdChange(wxCommandEvent& WXUNUSED(event)) {
	RefreshContentsInternal();
}

void FindItemDialog::OnClientIdChange(wxCommandEvent& WXUNUSED(event)) {
	RefreshContentsInternal();
}

void FindItemDialog::OnText(wxCommandEvent& WXUNUSED(event)) {
	input_timer.Start(800, true);
}

void FindItemDialog::OnTypeChange(wxCommandEvent& WXUNUSED(event)) {
	RefreshContentsInternal();
}

void FindItemDialog::OnPropertyChange(wxCommandEvent& WXUNUSED(event)) {
	RefreshContentsInternal();
}

void FindItemDialog::OnInputTimer(wxTimerEvent& WXUNUSED(event)) {
	RefreshContentsInternal();
}

void FindItemDialog::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	if (invalid_item->GetValue() && (SearchMode)notebook->GetSelection() == SearchMode::ServerIDs && result_id != 0) {
		EndModal(wxID_OK);
		return;
	}

	if (items_list->GetItemCount() != 0) {
		Brush* brush = items_list->GetSelectedBrush();
		if (brush) {
			result_brush = brush;
			result_id = brush->as<RAWBrush>()->getItemID();
			EndModal(wxID_OK);
		}
	}
}

void FindItemDialog::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	EndModal(wxID_CANCEL);
}
