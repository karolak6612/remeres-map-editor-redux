#include "app/preferences/client_version_page.h"
#include "app/main.h"
#include "app/settings.h"
#include "ui/core/gui.h"
#include "ui/util/dialog_util.h"
#include "ui/core/theme.h"
#include "util/image_manager.h"
#include <wx/tokenzr.h>
#include <charconv>

namespace {
	constexpr char kDefaultDataDirectory[] = "1287";
}

ClientVersionPage::ClientVersionPage(wxWindow* parent) : PreferencesPage(parent) {
	wxSizer* main_sizer = newd wxBoxSizer(wxVERTICAL);

	// Splitter for Tree and Property Grid
	client_splitter = newd wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);

	// Left side: List and Buttons
	wxPanel* left_panel = newd wxPanel(client_splitter, wxID_ANY);
	wxStaticBoxSizer* left_static_sizer = newd wxStaticBoxSizer(wxVERTICAL, left_panel, "Client List");
	wxSizer* left_inner_sizer = newd wxBoxSizer(wxVERTICAL);

	client_tree_ctrl = newd wxTreeCtrl(left_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HIDE_ROOT | wxTR_HAS_BUTTONS | wxTR_SINGLE);
	client_tree_ctrl->SetBackgroundColour(Theme::Get(Theme::Role::Background));
	client_tree_ctrl->SetForegroundColour(Theme::Get(Theme::Role::Text));

	left_inner_sizer->Add(client_tree_ctrl, wxSizerFlags(1).Expand().Border(wxBOTTOM, FROM_DIP(this, 5)));

	wxSizer* btn_outer_sizer = newd wxBoxSizer(wxHORIZONTAL);
	add_client_btn = newd wxButton(left_panel, wxID_ANY, "+ Add");
	add_client_btn->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_PLUS));
	delete_client_btn = newd wxButton(left_panel, wxID_ANY, "- Remove");
	delete_client_btn->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_MINUS));

	// Equal width for buttons
	btn_outer_sizer->AddStretchSpacer(1);
	btn_outer_sizer->Add(add_client_btn, wxSizerFlags(4).Expand().Border(wxRIGHT, FROM_DIP(this, 2)));
	btn_outer_sizer->Add(delete_client_btn, wxSizerFlags(4).Expand().Border(wxLEFT, FROM_DIP(this, 2)));
	btn_outer_sizer->AddStretchSpacer(1);

	left_inner_sizer->Add(btn_outer_sizer, wxSizerFlags().Expand());
	left_static_sizer->Add(left_inner_sizer, wxSizerFlags(1).Expand().Border(wxALL, 5));
	left_panel->SetSizer(left_static_sizer);

	// Right side: Property Grid
	wxPanel* right_panel = newd wxPanel(client_splitter, wxID_ANY);
	wxStaticBoxSizer* right_static_sizer = newd wxStaticBoxSizer(wxVERTICAL, right_panel, "Client Properties");

	client_prop_grid = newd wxPropertyGrid(right_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPG_SPLITTER_AUTO_CENTER | wxPG_BOLD_MODIFIED | wxPG_DESCRIPTION);
	client_prop_grid->SetBackgroundColour(Theme::Get(Theme::Role::Background));
	client_prop_grid->SetForegroundColour(Theme::Get(Theme::Role::Text));
	client_prop_grid->SetCaptionBackgroundColour(Theme::Get(Theme::Role::Header));
	client_prop_grid->SetCaptionTextColour(Theme::Get(Theme::Role::Text));
	client_prop_grid->SetMarginColour(Theme::Get(Theme::Role::Background));
	client_prop_grid->SetCellBackgroundColour(Theme::Get(Theme::Role::Background));
	client_prop_grid->SetCellTextColour(Theme::Get(Theme::Role::Text));
	client_prop_grid->SetEmptySpaceColour(Theme::Get(Theme::Role::Background));

	right_static_sizer->Add(client_prop_grid, wxSizerFlags(1).Expand().Border(wxALL, 5));
	right_panel->SetSizer(right_static_sizer);

	client_splitter->SplitVertically(left_panel, right_panel, 250);
	client_splitter->SetSashGravity(0.25);
	client_splitter->SetMinimumPaneSize(150);

	main_sizer->Add(client_splitter, wxSizerFlags(1).Expand().Border(wxALL, 5));

	// General Settings (at the bottom)
	wxStaticBoxSizer* settings_sizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Global Options");

	wxBoxSizer* def_version_sizer = newd wxBoxSizer(wxHORIZONTAL);
	def_version_sizer->Add(newd wxStaticText(this, wxID_ANY, "Default Client Version:"), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL).Border(wxRIGHT, 5));
	default_version_choice = newd wxChoice(this, wxID_ANY);
	def_version_sizer->Add(default_version_choice, wxSizerFlags(1).Expand());

	settings_sizer->Add(def_version_sizer, wxSizerFlags().Expand().Border(wxALL, 5));

	check_sigs_chkbox = newd wxCheckBox(this, wxID_ANY, "Check file signatures");
	check_sigs_chkbox->SetValue(g_settings.getInteger(Config::CHECK_SIGNATURES));
	settings_sizer->Add(check_sigs_chkbox, wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, 5));

	main_sizer->Add(settings_sizer, wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, 10));

	SetSizer(main_sizer);

	PopulateClientTree();

	// Binds
	client_tree_ctrl->Bind(wxEVT_TREE_SEL_CHANGING, [this](wxTreeEvent& event) {
		ClientVersion* current = GetSelectedClient();
		if (current && current->isDirty()) {
			int res = wxMessageBox("Save changes to " + current->getName() + "?", "Unsaved Changes", wxYES_NO | wxCANCEL | wxICON_QUESTION);
			if (res == wxYES) {
				if (ClientVersion::saveVersions()) {
					current->clearDirty();
				} else {
					wxMessageBox("Failed to save client versions. Changes not saved.", "Save Error", wxOK | wxICON_ERROR);
					event.Veto();
					return;
				}
			} else if (res == wxNO) {
				current->restore();
				current->clearDirty();
			} else {
				event.Veto();
				return;
			}
		}
	});
	client_tree_ctrl->Bind(wxEVT_TREE_SEL_CHANGED, &ClientVersionPage::OnClientSelected, this);
	client_tree_ctrl->Bind(wxEVT_TREE_ITEM_MENU, &ClientVersionPage::OnTreeContextMenu, this);
	client_prop_grid->Bind(wxEVT_PG_CHANGED, &ClientVersionPage::OnPropertyChanged, this);
	add_client_btn->Bind(wxEVT_BUTTON, &ClientVersionPage::OnAddClient, this);
	delete_client_btn->Bind(wxEVT_BUTTON, &ClientVersionPage::OnDeleteClient, this);
}

void ClientVersionPage::PopulateClientTree() {
	// Store the currently selected client to re-select it after repopulating
	ClientVersion* current_selected_client = GetSelectedClient();

	client_tree_ctrl->DeleteAllItems();
	wxTreeItemId root = client_tree_ctrl->AddRoot("Clients");

	wxTreeItemId item_to_select;

	// Group clients by major version
	std::map<int, std::vector<ClientVersion*>> grouped_versions;
	ClientVersionList all_versions = ClientVersion::getAll();
	for (auto* version : all_versions) {
		int protocol = version->getVersion();
		int major;
		if (protocol >= 10000) { // e.g., 12850 -> 12
			major = protocol / 1000;
		} else if (protocol >= 1000) { // e.g., 1098 -> 10
			major = protocol / 100;
		} else if (protocol >= 100) { // e.g., 860 -> 8
			major = protocol / 100;
		} else { // For very old versions if any, e.g., 740 -> 7
			major = protocol / 10;
		}
		grouped_versions[major].push_back(version);
	}

	default_version_choice->Clear();

	// Populate the tree and default version choice
	for (const auto& [major_version, versions_in_group] : grouped_versions) {
		wxTreeItemId group = client_tree_ctrl->AppendItem(root, wxString::Format("%d.x", major_version));
		for (auto* version : versions_in_group) {
			default_version_choice->Append(wxstr(version->getName()));
			if (version->getProtocolID() == g_settings.getInteger(Config::DEFAULT_CLIENT_VERSION)) {
				default_version_choice->SetSelection(default_version_choice->GetCount() - 1);
			}

			wxTreeItemId item = client_tree_ctrl->AppendItem(group, wxstr(version->getName()), -1, -1, new TreeItemData(version));
			if (version == current_selected_client) {
				item_to_select = item;
			}
		}
		// Collapse the group by default
		client_tree_ctrl->Collapse(group);
	}

	if (item_to_select.IsOk()) {
		client_tree_ctrl->EnsureVisible(item_to_select);
		client_tree_ctrl->SelectItem(item_to_select);
	} else {
		// If nothing specific selected, maybe expand the first group so it's not empty
		wxTreeItemIdValue cookie;
		wxTreeItemId firstGroup = client_tree_ctrl->GetFirstChild(root, cookie);
		if (firstGroup.IsOk()) {
			client_tree_ctrl->Expand(firstGroup);
		}
	}
}

ClientVersion* ClientVersionPage::GetSelectedClient() {
	wxTreeItemId selection = client_tree_ctrl->GetSelection();
	if (!selection.IsOk()) {
		return nullptr;
	}

	TreeItemData* data = (TreeItemData*)client_tree_ctrl->GetItemData(selection);
	return data ? data->cv : nullptr;
}

void ClientVersionPage::SelectClient(ClientVersion* version) {
	if (!version) {
		client_tree_ctrl->UnselectAll();
		return;
	}

	wxTreeItemId root = client_tree_ctrl->GetRootItem();
	if (!root.IsOk()) {
		return;
	}

	wxTreeItemIdValue cookie;
	wxTreeItemId group_item = client_tree_ctrl->GetFirstChild(root, cookie);
	while (group_item.IsOk()) {
		wxTreeItemIdValue child_cookie;
		wxTreeItemId client_item = client_tree_ctrl->GetFirstChild(group_item, child_cookie);
		while (client_item.IsOk()) {
			TreeItemData* data = (TreeItemData*)client_tree_ctrl->GetItemData(client_item);
			if (data && data->cv == version) {
				client_tree_ctrl->Expand(group_item); // Expand the group to make it visible
				client_tree_ctrl->SelectItem(client_item);
				client_tree_ctrl->EnsureVisible(client_item);
				return;
			}
			client_item = client_tree_ctrl->GetNextChild(group_item, child_cookie);
		}
		group_item = client_tree_ctrl->GetNextSibling(group_item);
	}
}

void ClientVersionPage::OnClientSelected(wxTreeEvent& WXUNUSED(event)) {
	// Note: Validation of previous selection is handled by EVT_TREE_SEL_CHANGING now (to some extent)
	// or in this method in original code. The lambda for SEL_CHANGING covers the dirty check.
	// But we need to handle the populate.

	ClientVersion* cv = GetSelectedClient();
	if (!cv) {
		client_prop_grid->Clear();
		return;
	}

	cv->backup(); // Cache current values for "Discard"

	client_prop_grid->Clear();

	// Group: General
	client_prop_grid->Append(new wxPropertyCategory("General Info", "General"));
	client_prop_grid->Append(new wxIntProperty("Version ID", "Version", cv->getVersion()))
		->SetHelpString("The internal numeric version of the client (e.g., 860, 1077, 1285).");
	client_prop_grid->Append(new wxStringProperty("Display Name", "Name", cv->getName()))
		->SetHelpString("The name displayed in lists and menus.");
	client_prop_grid->Append(new wxStringProperty("Description", "description", cv->getDescription()))
		->SetHelpString("Provide an optional description for this client.");

	// Group: Files & Paths
	client_prop_grid->Append(new wxPropertyCategory("Files & Paths", "Files"));

	client_prop_grid->Append(new wxDirProperty("Client Path", "clientPath", cv->getClientPath().GetFullPath()))
		->SetHelpString("Selection of the client folder (where Tibia.dat and Tibia.spr are located).");

	client_prop_grid->Append(new wxStringProperty("Data Directory", "dataDirectory", cv->getDataDirectory()))
		->SetHelpString("RME's internal folder for this client's data.");
	client_prop_grid->Append(new wxStringProperty("Metadata File (.dat)", "metadataFile", cv->getMetadataFile()));
	client_prop_grid->Append(new wxStringProperty("Sprites File (.spr)", "spritesFile", cv->getSpritesFile()));

	// Group: Signatures
	client_prop_grid->Append(new wxPropertyCategory("Signatures", "Signatures"));

	wxString datSig = wxString::Format("%X", cv->getDatSignature());
	wxString sprSig = wxString::Format("%X", cv->getSprSignature());

	client_prop_grid->Append(new wxStringProperty("DAT Signature", "datSignature", datSig))
		->SetHelpString("Hex signature of the Tibia.dat file.");
	client_prop_grid->Append(new wxStringProperty("SPR Signature", "sprSignature", sprSig))
		->SetHelpString("Hex signature of the Tibia.spr file.");

	// Group: OTB Settings
	client_prop_grid->Append(new wxPropertyCategory("OTB & Map Compatibility", "OTB"));
	client_prop_grid->Append(new wxIntProperty("OTB ID", "otbId", cv->getOtbId()));
	client_prop_grid->Append(new wxIntProperty("OTB Major Version", "otbMajor", cv->getOtbMajor()));

	wxString otbmVersStr;
	for (auto v : cv->getMapVersionsSupported()) {
		if (!otbmVersStr.IsEmpty()) {
			otbmVersStr += ", ";
		}
		otbmVersStr += std::to_string((int)v + 1);
	}
	client_prop_grid->Append(new wxStringProperty("Supported OTBM Versions", "otbmVersions", otbmVersStr))
		->SetHelpString("Comma separated list of OTBM versions supporting this client (1, 2, 3, 4).");

	// Group: Config / Flags
	client_prop_grid->Append(new wxPropertyCategory("Configuration & Flags", "Config"));
	client_prop_grid->Append(new wxStringProperty("Configuration Type", "configType", cv->getConfigType()));

	client_prop_grid->Append(new wxBoolProperty("Transparency", "transparency", cv->isTransparent()));
	client_prop_grid->Append(new wxBoolProperty("Extended", "extended", cv->isExtended()));
	client_prop_grid->Append(new wxBoolProperty("Frame Durations", "frameDurations", cv->hasFrameDurations()));
	client_prop_grid->Append(new wxBoolProperty("Frame Groups", "frameGroups", cv->hasFrameGroups()));

	// Perform initial validation coloring
	wxPropertyGridIterator it = client_prop_grid->GetIterator();
	for (; !it.AtEnd(); it++) {
		UpdatePropertyValidation(*it);
	}
}

void ClientVersionPage::UpdatePropertyValidation(wxPGProperty* prop) {
	if (!prop) {
		return;
	}
	wxString name = prop->GetName();
	wxAny value = prop->GetValue();
	bool invalid = false;

	if (name == "Name") {
		wxString valStr = value.As<wxString>();
		if (valStr.IsEmpty()) {
			invalid = true;
		} else {
			ClientVersion* current = GetSelectedClient();
			if (current) {
				for (auto* other : ClientVersion::getAll()) {
					if (other != current && other->getName() == nstr(valStr)) {
						invalid = true;
						break;
					}
				}
			}
		}
	}

	if (invalid) {
		prop->SetBackgroundColour(wxColour(255, 200, 200)); // Light Red
	} else {
		// Reset to default
		prop->SetBackgroundColour(Theme::Get(Theme::Role::Background));
	}
}

void ClientVersionPage::OnTreeContextMenu(wxTreeEvent& event) {
	wxTreeItemId item = event.GetItem();
	if (!item.IsOk()) {
		return;
	}

	// Check if it is a client item (has data)
	TreeItemData* data = (TreeItemData*)client_tree_ctrl->GetItemData(item);
	if (!data || !data->cv) {
		return;
	}

	wxMenu menu;
	menu.Append(wxID_COPY, "Duplicate")->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_COPY));
	menu.Bind(wxEVT_MENU, &ClientVersionPage::OnDuplicateClient, this, wxID_COPY);

	PopupMenu(&menu);
}

void ClientVersionPage::OnDuplicateClient(wxCommandEvent& WXUNUSED(event)) {
	ClientVersion* current = GetSelectedClient();
	if (!current) {
		return;
	}

	std::unique_ptr<ClientVersion> new_cv = current->clone();
	new_cv->setName(current->getName() + " (Copy)");

	ClientVersion* ptr = new_cv.get();
	ClientVersion::addVersion(std::move(new_cv));

	PopulateClientTree();
	SelectClient(ptr);
}

void ClientVersionPage::OnPropertyChanged(wxPropertyGridEvent& event) {
	ClientVersion* cv = GetSelectedClient();
	if (!cv) {
		return;
	}

	wxPGProperty* prop = event.GetProperty();
	wxString propName = prop->GetName();
	wxAny value = prop->GetValue();

	if (propName == "Version") {
		cv->setVersion(value.As<int>());
	} else if (propName == "Name") {
		cv->setName(nstr(value.As<wxString>()));
		wxTreeItemId sel = client_tree_ctrl->GetSelection();
		if (sel.IsOk()) {
			client_tree_ctrl->SetItemText(sel, value.As<wxString>());
		}
	} else if (propName == "otbId") {
		cv->setOtbId(value.As<int>());
	} else if (propName == "otbMajor") {
		cv->setOtbMajor(value.As<int>());
	} else if (propName == "otbmVersions") {
		wxString s = value.As<wxString>();
		cv->getMapVersionsSupported().clear();
		wxStringTokenizer tokenizer(s, ", ");
		while (tokenizer.HasMoreTokens()) {
			long v;
			if (tokenizer.GetNextToken().ToLong(&v)) {
				if (v >= 1 && v <= 4) {
					cv->getMapVersionsSupported().push_back(static_cast<MapVersionID>(v - 1));
				}
			}
		}
	} else if (propName == "dataDirectory") {
		cv->setDataDirectory(nstr(value.As<wxString>()));
	} else if (propName == "clientPath") {
		cv->setClientPath(FileName(value.As<wxString>()));
	} else if (propName == "datSignature") {
		uint32_t sig;
		std::string s = nstr(value.As<wxString>());
		if (std::from_chars(s.data(), s.data() + s.size(), sig, 16).ec == std::errc()) {
			cv->setDatSignature(sig);
		} else {
			prop->SetValue(wxString::Format("%X", cv->getDatSignature()));
			UpdatePropertyValidation(prop);
		}
	} else if (propName == "sprSignature") {
		uint32_t sig;
		std::string s = nstr(value.As<wxString>());
		if (std::from_chars(s.data(), s.data() + s.size(), sig, 16).ec == std::errc()) {
			cv->setSprSignature(sig);
		} else {
			prop->SetValue(wxString::Format("%X", cv->getSprSignature()));
			UpdatePropertyValidation(prop);
		}
	} else if (propName == "description") {
		cv->setDescription(nstr(value.As<wxString>()));
	} else if (propName == "configType") {
		cv->setConfigType(nstr(prop->GetValueAsString()));
	} else if (propName == "metadataFile") {
		cv->setMetadataFile(nstr(value.As<wxString>()));
	} else if (propName == "spritesFile") {
		cv->setSpritesFile(nstr(value.As<wxString>()));
	} else if (propName == "transparency") {
		cv->setTransparent(value.As<bool>());
	} else if (propName == "extended") {
		cv->setExtended(value.As<bool>());
	} else if (propName == "frameDurations") {
		cv->setFrameDurations(value.As<bool>());
	} else if (propName == "frameGroups") {
		cv->setFrameGroups(value.As<bool>());
	}

	cv->markDirty();
	UpdatePropertyValidation(prop);
}

void ClientVersionPage::OnAddClient(wxCommandEvent& WXUNUSED(event)) {
	std::string newName = "New Client";
	int counter = 1;
	while (ClientVersion::get(newName)) {
		newName = "New Client " + std::to_string(counter++);
	}

	OtbVersion otb;
	otb.name = newName;
	otb.id = PROTOCOL_VERSION_NONE;
	otb.format_version = OTB_VERSION_1;

	auto cv = std::make_unique<ClientVersion>(otb, newName, kDefaultDataDirectory);
	// cv->setName(newName); // Redundant, set in constructor
	cv->setMetadataFile("Tibia.dat");
	cv->setSpritesFile("Tibia.spr");
	cv->markDirty();

	ClientVersion* cv_ptr = cv.get();
	ClientVersion::addVersion(std::move(cv));

	PopulateClientTree();
	SelectClient(cv_ptr);
}

void ClientVersionPage::OnDeleteClient(wxCommandEvent& WXUNUSED(event)) {
	ClientVersion* cv = GetSelectedClient();
	if (!cv) {
		return;
	}

	if (wxMessageBox("Are you sure you want to delete " + cv->getName() + "?", "Confirm Delete", wxYES_NO | wxICON_WARNING) == wxYES) {
		ClientVersion::removeVersion(cv->getName());
		if (!ClientVersion::saveVersions()) {
			wxMessageBox("Could not save client versions to disk.\nThe changes will be reverted.", "Error", wxOK | wxICON_ERROR);
			// Revert by reloading from disk
			ClientVersion::loadVersions();
		}
		PopulateClientTree();
	}
}

bool ClientVersionPage::ValidateData() {
	for (auto* cv : ClientVersion::getAll()) {
		if (!cv->isValid()) {
			wxMessageBox("Client '" + cv->getName() + "' has invalid data (Name cannot be empty).\nPlease fix it before saving.", "Invalid Client Data", wxOK | wxICON_ERROR);
			SelectClient(cv);
			return false;
		}

		// Check for duplicates
		for (auto* other : ClientVersion::getAll()) {
			if (cv != other && cv->getName() == other->getName()) {
				wxMessageBox("Client '" + cv->getName() + "' has a duplicate name.\nPlease rename one of them before saving.", "Duplicate Client Name", wxOK | wxICON_ERROR);
				SelectClient(cv);
				return false;
			}
		}
	}
	return true;
}

void ClientVersionPage::Apply() {
	g_settings.setInteger(Config::CHECK_SIGNATURES, check_sigs_chkbox->GetValue());

	if (default_version_choice->GetSelection() != wxNOT_FOUND) {
		std::string defName = nstr(default_version_choice->GetStringSelection());
		ClientVersion* defCv = ClientVersion::get(defName);
		if (defCv) {
			g_settings.setInteger(Config::DEFAULT_CLIENT_VERSION, defCv->getProtocolID());
		}
	}

	bool anyDirty = false;
	for (auto* cv : ClientVersion::getAll()) {
		if (cv->isDirty()) {
			anyDirty = true;
			break;
		}
	}
	if (anyDirty) {
		if (ClientVersion::saveVersions()) {
			for (auto* cv : ClientVersion::getAll()) {
				cv->clearDirty();
			}
		} else {
			wxMessageBox("Failed to save client versions. Check log for details.", "Save Error", wxOK | wxICON_ERROR);
		}
	}
}
