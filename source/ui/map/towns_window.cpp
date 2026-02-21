#include "ui/map/towns_window.h"

#include "editor/editor.h"
#include "map/map.h"
#include "game/town.h"
#include "ui/positionctrl.h"
#include "ui/gui.h"
#include "ui/dialog_util.h"
#include "util/image_manager.h"

#include <algorithm>
#include <iterator>
#include <vector>

class TownListCtrl : public wxListCtrl {
public:
	TownListCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style) :
		wxListCtrl(parent, id, pos, size, style | wxLC_VIRTUAL) {
	}

	wxString OnGetItemText(long item, long column) const override {
		if (item >= 0 && item < static_cast<long>(m_towns.size())) {
			if (column == 0) return wxString::Format("%d", m_towns[item].first);
			if (column == 1) return wxstr(m_towns[item].second);
		}
		return "";
	}

	void SetTowns(std::vector<std::pair<uint32_t, std::string>> towns) {
		m_towns = std::move(towns);
		SetItemCount(m_towns.size());
		Refresh();
	}

private:
	std::vector<std::pair<uint32_t, std::string>> m_towns;
};

EditTownsDialog::EditTownsDialog(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Towns", wxDefaultPosition, FROM_DIP(parent, wxSize(350, 400))),
	editor(editor) {
	Map& map = editor.map;

	// Create topsizer
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	wxSizer* tmpsizer;

	for (const auto& [id, town] : map.towns) {
		town_list.push_back(std::make_unique<Town>(*town));
		if (max_town_id < town->getID()) {
			max_town_id = town->getID();
		}
	}

	// Town list
	town_list_ctrl = newd TownListCtrl(this, wxID_ANY, wxDefaultPosition, FROM_DIP(this, wxSize(310, 150)), wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES);
	town_list_ctrl->InsertColumn(0, "ID", wxLIST_FORMAT_LEFT, 50);
	town_list_ctrl->InsertColumn(1, "Name", wxLIST_FORMAT_LEFT, 240);

	sizer->Add(town_list_ctrl, 1, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 10);

	tmpsizer = newd wxBoxSizer(wxHORIZONTAL);
	auto addBtn = newd wxButton(this, EDIT_TOWNS_ADD, "Add");
	addBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_PLUS, wxSize(16, 16)));
	addBtn->SetToolTip("Add a new town");
	tmpsizer->Add(addBtn, 0, wxTOP, 5);
	remove_button = newd wxButton(this, EDIT_TOWNS_REMOVE, "Remove");
	remove_button->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_MINUS, wxSize(16, 16)));
	remove_button->SetToolTip("Remove selected town");
	tmpsizer->Add(remove_button, 0, wxRIGHT | wxTOP, 5);
	sizer->Add(tmpsizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

	// House options
	tmpsizer = newd wxStaticBoxSizer(wxHORIZONTAL, this, "Name / ID");
	name_field = newd wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, FROM_DIP(this, wxSize(190, 20)), 0, wxTextValidator(wxFILTER_ASCII, &town_name));
	name_field->SetToolTip("Town name");
	tmpsizer->Add(name_field, 2, wxEXPAND | wxLEFT | wxBOTTOM, 5);

	id_field = newd wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, FROM_DIP(this, wxSize(40, 20)), 0, wxTextValidator(wxFILTER_NUMERIC, &town_id));
	id_field->SetToolTip("Town ID");
	id_field->Enable(false);
	tmpsizer->Add(id_field, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
	sizer->Add(tmpsizer, 0, wxEXPAND | wxALL, 10);

	// Temple position
	temple_position = newd PositionCtrl(this, "Temple Position", 0, 0, 0, map.getWidth(), map.getHeight());
	select_position_button = newd wxButton(this, EDIT_TOWNS_SELECT_TEMPLE, "Go To");
	select_position_button->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_LOCATION_ARROW, wxSize(16, 16)));
	select_position_button->SetToolTip("Jump to temple position");
	temple_position->Add(select_position_button, 0, wxLEFT | wxRIGHT | wxBOTTOM, 5);
	sizer->Add(temple_position, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

	// OK/Cancel buttons
	tmpsizer = newd wxBoxSizer(wxHORIZONTAL);
	auto okBtn = newd wxButton(this, wxID_OK, "OK");
	okBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_CHECK, wxSize(16, 16)));
	okBtn->SetToolTip("Save changes");
	tmpsizer->Add(okBtn, wxSizerFlags(1).Center());
	auto cancelBtn = newd wxButton(this, wxID_CANCEL, "Cancel");
	cancelBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_XMARK, wxSize(16, 16)));
	cancelBtn->SetToolTip("Cancel");
	tmpsizer->Add(cancelBtn, wxSizerFlags(1).Center());
	sizer->Add(tmpsizer, 0, wxCENTER | wxALL, 10);

	SetSizerAndFit(sizer);
	Centre(wxBOTH);
	BuildListBox(true);

	town_list_ctrl->Bind(wxEVT_LIST_ITEM_SELECTED, &EditTownsDialog::OnListSelected, this);
	addBtn->Bind(wxEVT_BUTTON, &EditTownsDialog::OnClickAdd, this);
	remove_button->Bind(wxEVT_BUTTON, &EditTownsDialog::OnClickRemove, this);
	select_position_button->Bind(wxEVT_BUTTON, &EditTownsDialog::OnClickSelectTemplePosition, this);
	okBtn->Bind(wxEVT_BUTTON, &EditTownsDialog::OnClickOK, this);
	cancelBtn->Bind(wxEVT_BUTTON, &EditTownsDialog::OnClickCancel, this);

	wxIcon icon;
	icon.CopyFromBitmap(IMAGE_MANAGER.GetBitmap(ICON_CITY, wxSize(32, 32)));
	SetIcon(icon);
}

EditTownsDialog::~EditTownsDialog() = default;

void EditTownsDialog::BuildListBox(bool doselect) {
	long tmplong = 0;
	max_town_id = 0;
	std::vector<std::pair<uint32_t, std::string>> town_data;
	uint32_t selection_before = 0;

	if (doselect && id_field->GetValue().ToLong(&tmplong)) {
		uint32_t old_town_id = tmplong;

		for (const auto& town : town_list) {
			if (old_town_id == town->getID()) {
				selection_before = town->getID();
				break;
			}
		}
	}

	for (const auto& town : town_list) {
		town_data.push_back({town->getID(), town->getName()});
		if (max_town_id < town->getID()) {
			max_town_id = town->getID();
		}
	}

	static_cast<TownListCtrl*>(town_list_ctrl)->SetTowns(std::move(town_data));

	remove_button->Enable(town_list_ctrl->GetItemCount() != 0);
	select_position_button->Enable(false);

	if (doselect) {
		if (selection_before) {
			int i = 0;
			for (const auto& town : town_list) {
				if (selection_before == town->getID()) {
					// SetSelection(i)
					town_list_ctrl->Unbind(wxEVT_LIST_ITEM_SELECTED, &EditTownsDialog::OnListSelected, this);
					town_list_ctrl->SetItemState(i, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
					town_list_ctrl->Bind(wxEVT_LIST_ITEM_SELECTED, &EditTownsDialog::OnListSelected, this);
					return;
				}
				++i;
			}
		}
		UpdateSelection(0);
	}
}

void EditTownsDialog::UpdateSelection(long new_selection) {
	long tmplong;

	// Save old values
	if (!town_list.empty()) {
		if (id_field->GetValue().ToLong(&tmplong)) {
			uint32_t old_town_id = tmplong;

			Town* old_town = nullptr;

			for (auto& town : town_list) {
				if (old_town_id == town->getID()) {
					old_town = town.get();
					break;
				}
			}

			if (old_town) {
				editor.map.getOrCreateTile(old_town->getTemplePosition())->getLocation()->decreaseTownCount();

				Position templePos = temple_position->GetPosition();

				editor.map.getOrCreateTile(templePos)->getLocation()->increaseTownCount();

				old_town->setTemplePosition(templePos);

				wxString new_name = name_field->GetValue();
				wxString old_name = wxstr(old_town->getName());

				old_town->setName(nstr(new_name));
				if (new_name != old_name) {
					// Name has changed, update list
					BuildListBox(false);
				}
			}
		}
	}

	// Clear fields
	town_name.Clear();
	town_id.Clear();

	if (town_list.size() > size_t(new_selection)) {
		name_field->Enable(true);
		temple_position->Enable(true);
		select_position_button->Enable(true);

		// Change the values to reflect the newd selection
		Town* town = town_list[new_selection].get();
		ASSERT(town);

		town_name << wxstr(town->getName());
		name_field->SetValue(town_name);
		town_id << long(town->getID());
		id_field->SetValue(town_id);
		temple_position->SetPosition(town->getTemplePosition());

		// town_listbox->SetSelection(new_selection);
		town_list_ctrl->Unbind(wxEVT_LIST_ITEM_SELECTED, &EditTownsDialog::OnListSelected, this);
		town_list_ctrl->SetItemState(new_selection, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
		town_list_ctrl->Bind(wxEVT_LIST_ITEM_SELECTED, &EditTownsDialog::OnListSelected, this);
	} else {
		name_field->Enable(false);
		temple_position->Enable(false);
		select_position_button->Enable(false);
	}
	Refresh();
}

void EditTownsDialog::OnListSelected(wxListEvent& event) {
	UpdateSelection(event.GetIndex());
}

void EditTownsDialog::OnClickSelectTemplePosition(wxCommandEvent& WXUNUSED(event)) {
	Position templepos = temple_position->GetPosition();
	g_gui.SetScreenCenterPosition(templepos);
}

void EditTownsDialog::OnClickAdd(wxCommandEvent& WXUNUSED(event)) {
	auto new_town = std::make_unique<Town>(++max_town_id);
	new_town->setName("Unnamed Town");
	new_town->setTemplePosition(Position(0, 0, 0));
	town_list.push_back(std::move(new_town));

	editor.map.getOrCreateTile(Position(0, 0, 0))->getLocation()->increaseTownCount();

	BuildListBox(false);
	UpdateSelection(town_list.size() - 1);
}

void EditTownsDialog::OnClickRemove(wxCommandEvent& WXUNUSED(event)) {
	long tmplong;
	if (id_field->GetValue().ToLong(&tmplong)) {
		uint32_t old_town_id = tmplong;

		Town* town = nullptr;
		int selection_index = 0;

		auto town_iter = std::ranges::find_if(town_list, [old_town_id](const std::unique_ptr<Town>& t) {
			return t->getID() == old_town_id;
		});

		if (town_iter != town_list.end()) {
			town = town_iter->get();
			selection_index = static_cast<int>(std::distance(town_list.begin(), town_iter));
		}

		if (!town) {
			return;
		}

		Map& map = editor.map;
		for (const auto& [id, house] : map.houses) {
			if (house->townid == town->getID()) {
				DialogUtil::PopupDialog(this, "Error", "You cannot delete a town which still has houses associated with it.", wxOK);
				return;
			}
		}

		// remove town flag from tile
		editor.map.getOrCreateTile(town->getTemplePosition())->getLocation()->decreaseTownCount();

		// remove town object
		town_list.erase(town_iter);
		BuildListBox(false);
		UpdateSelection(selection_index - 1);
	}
}

void EditTownsDialog::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	long tmplong = 0;

	if (Validate() && TransferDataFromWindow()) {
		// Save old values
		if (!town_list.empty() && id_field->GetValue().ToLong(&tmplong)) {
			uint32_t old_town_id = tmplong;

			Town* old_town = nullptr;

			for (auto& town : town_list) {
				if (old_town_id == town->getID()) {
					old_town = town.get();
					break;
				}
			}

			if (old_town) {
				editor.map.getOrCreateTile(old_town->getTemplePosition())->getLocation()->decreaseTownCount();

				Position templePos = temple_position->GetPosition();

				editor.map.getOrCreateTile(templePos)->getLocation()->increaseTownCount();

				old_town->setTemplePosition(templePos);

				wxString new_name = name_field->GetValue();
				wxString old_name = wxstr(old_town->getName());

				old_town->setName(nstr(new_name));
				if (new_name != old_name) {
					// Name has changed, update list
					BuildListBox(true);
				}
			}
		}

		Towns& towns = editor.map.towns;

		// Verify the newd information
		for (const auto& town : town_list) {
			if (town->getName() == "") {
				DialogUtil::PopupDialog(this, "Error", "You can't have a town with an empty name.", wxOK);
				return;
			}
			if (!town->getTemplePosition().isValid() || town->getTemplePosition().x > editor.map.getWidth() || town->getTemplePosition().y > editor.map.getHeight()) {
				wxString msg;
				msg << "The town " << wxstr(town->getName()) << " has an invalid temple position.";
				DialogUtil::PopupDialog(this, "Error", msg, wxOK);
				return;
			}
		}

		// Clear old towns
		towns.clear();

		// Build the newd town map
		for (auto& town : town_list) {
			// Movement to towns.addTown() leaves moved-from unique_ptrs in town_list.
			// This is safe as town_list.clear() is called immediately after.
			towns.addTown(std::move(town));
		}
		town_list.clear();
		editor.map.doChange();

		EndModal(1);
		g_gui.RefreshPalettes();
	}
}

void EditTownsDialog::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	// Just close this window
	EndModal(0);
}
