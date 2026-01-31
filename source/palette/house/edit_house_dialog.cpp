//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "palette/house/edit_house_dialog.h"

#include "ui/dialog_util.h"
#include "app/settings.h"
#include "ui/gui.h"

#include "map/map.h"
#include "game/house.h"
#include "game/town.h"

#include <sstream>

EditHouseDialog::EditHouseDialog(wxWindow* parent, Map* map, House* house) :
	wxDialog(parent, wxID_ANY, "House Properties", wxDefaultPosition, wxSize(250, 160)),
	map(map),
	what_house(house) {
	Bind(wxEVT_SET_FOCUS, &EditHouseDialog::OnFocusChange, this);
	Bind(wxEVT_BUTTON, &EditHouseDialog::OnClickOK, this, wxID_OK);
	Bind(wxEVT_BUTTON, &EditHouseDialog::OnClickCancel, this, wxID_CANCEL);

	ASSERT(map);
	ASSERT(house);

	wxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
	wxSizer* boxsizer = new wxStaticBoxSizer(wxVERTICAL, this, "House Properties");
	wxFlexGridSizer* housePropContainer = new wxFlexGridSizer(2, 10, 10);
	housePropContainer->AddGrowableCol(1);

	wxFlexGridSizer* subsizer = new wxFlexGridSizer(2, 10, 10);
	subsizer->AddGrowableCol(1);

	house_name = wxstr(house->name);
	house_id = i2ws(house->getID());
	house_rent = i2ws(house->rent);

	subsizer->Add(new wxStaticText(this, wxID_ANY, "Name:"), wxSizerFlags(0).Border(wxLEFT, 5));
	name_field = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(160, 20), 0, wxTextValidator(wxFILTER_ASCII, &house_name));
	name_field->SetToolTip("Enter the name of the house.");
	subsizer->Add(name_field, wxSizerFlags(1).Expand());

	subsizer->Add(new wxStaticText(this, wxID_ANY, "Town:"), wxSizerFlags(0).Border(wxLEFT, 5));

	const Towns& towns = map->towns;
	town_id_field = new wxChoice(this, wxID_ANY);
	int to_select_index = 0;
	uint32_t houseTownId = house->townid;

	if (towns.count() > 0) {
		bool found = false;
		for (TownMap::const_iterator town_iter = towns.begin(); town_iter != towns.end(); ++town_iter) {
			if (town_iter->second->getID() == houseTownId) {
				found = true;
			}
			town_id_field->Append(wxstr(town_iter->second->getName()));
			town_ids_.push_back(town_iter->second->getID());
			if (!found) {
				++to_select_index;
			}
		}

		if (!found) {
			if (houseTownId != 0) {
				town_id_field->Append("Undefined Town (id:" + i2ws(houseTownId) + ")");
				town_ids_.push_back(houseTownId);
				++to_select_index;
			}
		}
	}
	town_id_field->SetSelection(to_select_index);
	town_id_field->SetToolTip("Select the town this house belongs to.");
	subsizer->Add(town_id_field, wxSizerFlags(1).Expand());

	subsizer->Add(new wxStaticText(this, wxID_ANY, "Rent:"), wxSizerFlags(0).Border(wxLEFT, 5));
	rent_field = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(160, 20), 0, wxTextValidator(wxFILTER_NUMERIC, &house_rent));
	rent_field->SetToolTip("Enter the rent for the house.");
	subsizer->Add(rent_field, wxSizerFlags(1).Expand());

	wxFlexGridSizer* subsizerRight = new wxFlexGridSizer(1, 10, 10);
	wxFlexGridSizer* houseSizer = new wxFlexGridSizer(2, 10, 10);

	houseSizer->Add(new wxStaticText(this, wxID_ANY, "ID:"), wxSizerFlags(0).Center());
	id_field = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(40, 20), wxSP_ARROW_KEYS, 1, 0xFFFF, house->getID());
	id_field->SetToolTip("Set the unique ID of the house.");
	houseSizer->Add(id_field, wxSizerFlags(1).Expand());
	subsizerRight->Add(houseSizer, wxSizerFlags(1).Expand());

	wxSizer* checkbox_sub_sizer = new wxBoxSizer(wxVERTICAL);
	checkbox_sub_sizer->AddSpacer(4);
	guildhall_field = new wxCheckBox(this, wxID_ANY, "Guildhall");
	guildhall_field->SetToolTip("Check if this house is a guildhall.");
	checkbox_sub_sizer->Add(guildhall_field);
	subsizerRight->Add(checkbox_sub_sizer);
	guildhall_field->SetValue(house->guildhall);

	housePropContainer->Add(subsizer, wxSizerFlags(5).Expand());
	housePropContainer->Add(subsizerRight, wxSizerFlags(5).Expand());
	boxsizer->Add(housePropContainer, wxSizerFlags(5).Expand().Border(wxTOP | wxBOTTOM, 10));
	topsizer->Add(boxsizer, wxSizerFlags(0).Expand().Border(wxRIGHT | wxLEFT, 20));

	wxSizer* buttonsSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonsSizer->Add(new wxButton(this, wxID_OK, "OK"), wxSizerFlags(1).Center().Border(wxTOP | wxBOTTOM, 10));
	buttonsSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Center().Border(wxTOP | wxBOTTOM, 10));
	topsizer->Add(buttonsSizer, wxSizerFlags(0).Center().Border(wxLEFT | wxRIGHT, 20));

	SetSizerAndFit(topsizer);
}

EditHouseDialog::~EditHouseDialog() {
}

void EditHouseDialog::OnFocusChange(wxFocusEvent& event) {
	wxWindow* win = event.GetWindow();
	if (wxSpinCtrl* spin = dynamic_cast<wxSpinCtrl*>(win)) {
		spin->SetSelection(-1, -1);
	} else if (wxTextCtrl* text = dynamic_cast<wxTextCtrl*>(win)) {
		text->SetSelection(-1, -1);
	}
}

void EditHouseDialog::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	if (Validate() && TransferDataFromWindow()) {
		long new_house_rent;
		house_rent.ToLong(&new_house_rent);
		if (new_house_rent < 0) {
			DialogUtil::PopupDialog(this, "Error", "House rent cannot be less than 0.", wxOK);
			return;
		}

		uint32_t new_house_id = id_field->GetValue();
		if (new_house_id < 1) {
			DialogUtil::PopupDialog(this, "Error", "House id cannot be less than 1.", wxOK);
			return;
		}

		if (house_name.length() == 0) {
			DialogUtil::PopupDialog(this, "Error", "House name cannot be empty.", wxOK);
			return;
		}

		if (g_settings.getInteger(Config::WARN_FOR_DUPLICATE_ID)) {
			Houses& houses = map->houses;
			for (HouseMap::const_iterator house_iter = houses.begin(); house_iter != houses.end(); ++house_iter) {
				House* house = house_iter->second;
				ASSERT(house);

				if (house->getID() == new_house_id && new_house_id != what_house->getID()) {
					DialogUtil::PopupDialog(this, "Error", "This house id is already in use.", wxOK);
					return;
				}

				if (wxstr(house->name) == house_name && house->getID() != what_house->getID()) {
					int ret = DialogUtil::PopupDialog(this, "Warning", "This house name is already in use, are you sure you want to continue?", wxYES | wxNO);
					if (ret == wxID_NO) {
						return;
					}
				}
			}
		}

		if (new_house_id != what_house->getID()) {
			int ret = DialogUtil::PopupDialog(this, "Warning", "Changing existing house ids on a production server WILL HAVE DATABASE CONSEQUENCES such as potential item loss, house owner change or invalidating guest lists.\nYou are doing it at own risk!\n\nAre you ABSOLUTELY sure you want to continue?", wxYES | wxNO);
			if (ret == wxID_NO) {
				return;
			}

			uint32_t old_house_id = what_house->getID();
			map->convertHouseTiles(old_house_id, new_house_id);
			map->houses.changeId(what_house, new_house_id);
		}

		int selection = town_id_field->GetSelection();
		if (selection == wxNOT_FOUND || selection < 0 || (size_t)selection >= town_ids_.size()) {
			DialogUtil::PopupDialog(this, "Error", "Invalid town selected.", wxOK);
			return;
		}
		int new_town_id = town_ids_[selection];

		what_house->name = nstr(house_name);
		what_house->rent = new_house_rent;
		what_house->guildhall = guildhall_field->GetValue();
		what_house->townid = new_town_id;

		EndModal(1);
	}
}

void EditHouseDialog::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	EndModal(0);
}
