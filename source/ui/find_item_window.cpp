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

FindItemDialog::FindItemDialog(wxWindow* parent, const wxString& title, bool onlyPickupables /* = false*/) :
	wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600), wxDEFAULT_DIALOG_STYLE),
	input_timer(this),
	result_brush(nullptr),
	result_id(0),
	only_pickupables(onlyPickupables) {
	this->SetSizeHints(wxDefaultSize, wxDefaultSize);

	wxBoxSizer* box_sizer = newd wxBoxSizer(wxHORIZONTAL);

	wxBoxSizer* options_box_sizer = newd wxBoxSizer(wxVERTICAL);

	wxString radio_boxChoices[] = { "Find by Server ID",
									"Find by Client ID",
									"Find by Name",
									"Find by Types",
									"Find by Properties" };

	int radio_boxNChoices = sizeof(radio_boxChoices) / sizeof(wxString);
	options_radio_box = newd wxRadioBox(this, wxID_ANY, "Search Mode", wxDefaultPosition, wxDefaultSize, radio_boxNChoices, radio_boxChoices, 1, wxRA_SPECIFY_COLS);
	options_radio_box->SetSelection(SearchMode::ServerIDs);
	options_box_sizer->Add(options_radio_box, 0, wxALL | wxEXPAND, 5);

	wxNotebook* notebook = newd wxNotebook(this, wxID_ANY);
	options_box_sizer->Add(notebook, 1, wxALL | wxEXPAND, 5);

	// ID & Name Page
	wxPanel* basic_page = newd wxPanel(notebook, wxID_ANY);
	wxBoxSizer* basic_sizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* server_id_box_sizer = newd wxStaticBoxSizer(newd wxStaticBox(basic_page, wxID_ANY, "Server ID"), wxVERTICAL);
	server_id_spin = newd wxSpinCtrl(server_id_box_sizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 100, g_items.getMaxID(), 100);
	server_id_spin->SetToolTip("Search by server ID");
	server_id_box_sizer->Add(server_id_spin, 0, wxALL | wxEXPAND, 5);

	invalid_item = newd wxCheckBox(server_id_box_sizer->GetStaticBox(), wxID_ANY, "Force select", wxDefaultPosition, wxDefaultSize, 0);
	invalid_item->SetToolTip("Force choose item ID that does not appear on the list.");
	server_id_box_sizer->Add(invalid_item, 1, wxALL | wxEXPAND, 5);

	basic_sizer->Add(server_id_box_sizer, 0, wxALL | wxEXPAND, 5);

	wxStaticBoxSizer* client_id_box_sizer = newd wxStaticBoxSizer(newd wxStaticBox(basic_page, wxID_ANY, "Client ID"), wxVERTICAL);
	client_id_spin = newd wxSpinCtrl(client_id_box_sizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 100, g_gui.gfx.getItemSpriteMaxID(), 100);
	client_id_spin->SetToolTip("Search by client ID");
	client_id_spin->Enable(false);
	client_id_box_sizer->Add(client_id_spin, 0, wxALL | wxEXPAND, 5);
	basic_sizer->Add(client_id_box_sizer, 0, wxALL | wxEXPAND, 5);

	wxStaticBoxSizer* name_box_sizer = newd wxStaticBoxSizer(newd wxStaticBox(basic_page, wxID_ANY, "Name"), wxVERTICAL);
	name_text_input = newd wxTextCtrl(name_box_sizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	name_text_input->SetToolTip("Search by item name (requires 2+ characters)");
	name_text_input->Enable(false);
	name_box_sizer->Add(name_text_input, 0, wxALL | wxEXPAND, 5);
	basic_sizer->Add(name_box_sizer, 0, wxALL | wxEXPAND, 5);

	basic_page->SetSizer(basic_sizer);
	notebook->AddPage(basic_page, "Basic");

	// Types Page
	wxPanel* types_page = newd wxPanel(notebook, wxID_ANY);
	wxBoxSizer* types_sizer = newd wxBoxSizer(wxVERTICAL);

	wxString types_choices[] = { "Depot",
								 "Mailbox",
								 "Trash Holder",
								 "Container",
								 "Door",
								 "Magic Field",
								 "Teleport",
								 "Bed",
								 "Key",
								 "Podium" };

	int types_choices_count = sizeof(types_choices) / sizeof(wxString);
	types_radio_box = newd wxRadioBox(types_page, wxID_ANY, "Select Type", wxDefaultPosition, wxDefaultSize, types_choices_count, types_choices, 1, wxRA_SPECIFY_COLS);
	types_radio_box->SetSelection(0);
	types_radio_box->Enable(false);
	types_sizer->Add(types_radio_box, 1, wxALL | wxEXPAND, 5);

	types_page->SetSizer(types_sizer);
	notebook->AddPage(types_page, "Types");

	// Properties Page
	wxPanel* properties_page = newd wxPanel(notebook, wxID_ANY);
	wxBoxSizer* prop_page_sizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* properties_box_sizer = newd wxStaticBoxSizer(newd wxStaticBox(properties_page, wxID_ANY, "Properties"), wxVERTICAL);
	wxFlexGridSizer* prop_grid = newd wxFlexGridSizer(3, 5, 5); // 3 cols, 5px gap

	unpassable = newd wxCheckBox(properties_box_sizer->GetStaticBox(), wxID_ANY, "Unpassable");
	unpassable->SetToolTip("Item blocks movement");
	prop_grid->Add(unpassable, 0, wxALL, 5);

	unmovable = newd wxCheckBox(properties_box_sizer->GetStaticBox(), wxID_ANY, "Unmovable");
	unmovable->SetToolTip("Item cannot be moved");
	prop_grid->Add(unmovable, 0, wxALL, 5);

	block_missiles = newd wxCheckBox(properties_box_sizer->GetStaticBox(), wxID_ANY, "Block Missiles");
	block_missiles->SetToolTip("Item blocks projectiles");
	prop_grid->Add(block_missiles, 0, wxALL, 5);

	block_pathfinder = newd wxCheckBox(properties_box_sizer->GetStaticBox(), wxID_ANY, "Block Path");
	block_pathfinder->SetToolTip("Item blocks pathfinding");
	prop_grid->Add(block_pathfinder, 0, wxALL, 5);

	readable = newd wxCheckBox(properties_box_sizer->GetStaticBox(), wxID_ANY, "Readable");
	readable->SetToolTip("Item has text");
	prop_grid->Add(readable, 0, wxALL, 5);

	writeable = newd wxCheckBox(properties_box_sizer->GetStaticBox(), wxID_ANY, "Writeable");
	writeable->SetToolTip("Item can be written on");
	prop_grid->Add(writeable, 0, wxALL, 5);

	pickupable = newd wxCheckBox(properties_box_sizer->GetStaticBox(), wxID_ANY, "Pickupable");
	pickupable->SetToolTip("Item can be picked up");
	pickupable->SetValue(only_pickupables);
	pickupable->Enable(!only_pickupables);
	prop_grid->Add(pickupable, 0, wxALL, 5);

	stackable = newd wxCheckBox(properties_box_sizer->GetStaticBox(), wxID_ANY, "Stackable");
	stackable->SetToolTip("Item is stackable");
	prop_grid->Add(stackable, 0, wxALL, 5);

	rotatable = newd wxCheckBox(properties_box_sizer->GetStaticBox(), wxID_ANY, "Rotatable");
	rotatable->SetToolTip("Item can be rotated");
	prop_grid->Add(rotatable, 0, wxALL, 5);

	hangable = newd wxCheckBox(properties_box_sizer->GetStaticBox(), wxID_ANY, "Hangable");
	hangable->SetToolTip("Item can be hung on walls");
	prop_grid->Add(hangable, 0, wxALL, 5);

	hook_east = newd wxCheckBox(properties_box_sizer->GetStaticBox(), wxID_ANY, "Hook East");
	hook_east->SetToolTip("Item hooks to the east");
	prop_grid->Add(hook_east, 0, wxALL, 5);

	hook_south = newd wxCheckBox(properties_box_sizer->GetStaticBox(), wxID_ANY, "Hook South");
	hook_south->SetToolTip("Item hooks to the south");
	prop_grid->Add(hook_south, 0, wxALL, 5);

	has_elevation = newd wxCheckBox(properties_box_sizer->GetStaticBox(), wxID_ANY, "Has Elevation");
	has_elevation->SetToolTip("Item has height (elevation)");
	prop_grid->Add(has_elevation, 0, wxALL, 5);

	ignore_look = newd wxCheckBox(properties_box_sizer->GetStaticBox(), wxID_ANY, "Ignore Look");
	ignore_look->SetToolTip("Item is ignored when looking");
	prop_grid->Add(ignore_look, 0, wxALL, 5);

	floor_change = newd wxCheckBox(properties_box_sizer->GetStaticBox(), wxID_ANY, "Floor Change");
	floor_change->SetToolTip("Item causes floor change");
	prop_grid->Add(floor_change, 0, wxALL, 5);

	properties_box_sizer->Add(prop_grid, 1, wxALL | wxEXPAND, 0);
	prop_page_sizer->Add(properties_box_sizer, 1, wxALL | wxEXPAND, 5);

	properties_page->SetSizer(prop_page_sizer);
	notebook->AddPage(properties_page, "Properties");

	// Add buttons to bottom of options sizer
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
	options_box_sizer->Add(buttons_box_sizer, 0, wxALIGN_CENTER | wxALL, 5);

	box_sizer->Add(options_box_sizer, 1, wxALL | wxEXPAND, 5);

	// --------------- Items list ---------------

	wxStaticBoxSizer* result_box_sizer = newd wxStaticBoxSizer(newd wxStaticBox(this, wxID_ANY, "Result"), wxVERTICAL);
	items_list = newd FindDialogListBox(result_box_sizer->GetStaticBox(), wxID_ANY);
	items_list->SetMinSize(wxSize(230, 512));
	result_box_sizer->Add(items_list, 0, wxALL, 5);
	box_sizer->Add(result_box_sizer, 1, wxALL | wxEXPAND, 5);

	this->SetSizer(box_sizer);
	this->Layout();
	this->Centre(wxBOTH);
	this->EnableProperties(false);
	this->RefreshContentsInternal();

	SetIcons(IMAGE_MANAGER.GetIconBundle(ICON_SEARCH));

	// Connect Events
	options_radio_box->Bind(wxEVT_COMMAND_RADIOBOX_SELECTED, &FindItemDialog::OnOptionChange, this);
	server_id_spin->Bind(wxEVT_COMMAND_SPINCTRL_UPDATED, &FindItemDialog::OnServerIdChange, this);
	server_id_spin->Bind(wxEVT_COMMAND_TEXT_UPDATED, &FindItemDialog::OnServerIdChange, this);
	client_id_spin->Bind(wxEVT_COMMAND_SPINCTRL_UPDATED, &FindItemDialog::OnClientIdChange, this);
	client_id_spin->Bind(wxEVT_COMMAND_TEXT_UPDATED, &FindItemDialog::OnClientIdChange, this);
	name_text_input->Bind(wxEVT_COMMAND_TEXT_UPDATED, &FindItemDialog::OnText, this);

	types_radio_box->Bind(wxEVT_COMMAND_RADIOBOX_SELECTED, &FindItemDialog::OnTypeChange, this);

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
	return (SearchMode)options_radio_box->GetSelection();
}

void FindItemDialog::setSearchMode(FindItemDialog::SearchMode mode) {
	if ((SearchMode)options_radio_box->GetSelection() != mode) {
		options_radio_box->SetSelection(mode);
	}

	server_id_spin->Enable(mode == SearchMode::ServerIDs);
	invalid_item->Enable(mode == SearchMode::ServerIDs);
	client_id_spin->Enable(mode == SearchMode::ClientIDs);
	name_text_input->Enable(mode == SearchMode::Names);
	types_radio_box->Enable(mode == SearchMode::Types);
	EnableProperties(mode == SearchMode::Properties);
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

	SearchMode selection = (SearchMode)options_radio_box->GetSelection();
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

			SearchItemType selection = (SearchItemType)types_radio_box->GetSelection();
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

void FindItemDialog::OnOptionChange(wxCommandEvent& WXUNUSED(event)) {
	setSearchMode((SearchMode)options_radio_box->GetSelection());
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
	if (invalid_item->GetValue() && (SearchMode)options_radio_box->GetSelection() == SearchMode::ServerIDs && result_id != 0) {
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
