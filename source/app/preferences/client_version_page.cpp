#include "app/preferences/client_version_page.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <map>

#include <wx/menu.h>
#include <wx/tokenzr.h>

#include "app/main.h"
#include "app/settings.h"
#include "ui/theme.h"
#include "util/image_manager.h"

namespace {
constexpr char kDefaultDataDirectory[] = "1287";

std::string ConfigTypeFromSelection(long selection) {
	switch (selection) {
		case 0:
			return "dat_otb";
		case 1:
			return "dat_only";
		case 2:
			return "dat_srv";
		case 3:
			return "protobuf";
		default:
			return "dat_otb";
	}
}

int SelectionFromConfigType(const std::string& value) {
	if (value == "dat_only") {
		return 1;
	}
	if (value == "dat_srv") {
		return 2;
	}
	if (value == "protobuf") {
		return 3;
	}
	return 0;
}

std::string LowercaseCopy(std::string value) {
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
		return static_cast<char>(std::tolower(character));
	});
	return value;
}
}

ClientVersionPage::ClientVersionPage(wxWindow* parent) : PreferencesPage(parent) {
	auto* main_sizer = new wxBoxSizer(wxVERTICAL);

	client_splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);
	client_splitter->SetBackgroundColour(GetBackgroundColour());

	auto* left_panel = new wxPanel(client_splitter, wxID_ANY);
	left_panel->SetBackgroundColour(GetBackgroundColour());
	auto* left_sizer = new wxBoxSizer(wxVERTICAL);

	auto* library_section = new PreferencesSectionPanel(
		left_panel,
		"Client Library",
		"Browse configured clients, filter the list by name or version, and manage entries without leaving Preferences."
	);
	client_search_ctrl = new wxSearchCtrl(library_section, wxID_ANY);
	client_search_ctrl->ShowSearchButton(true);
	client_search_ctrl->ShowCancelButton(true);
	PreferencesLayout::AddControlRow(
		library_section,
		"Search clients",
		"Filter by display name, description, protocol version, or OTB id.",
		client_search_ctrl,
		true
	);

	client_tree_ctrl = new wxTreeCtrl(library_section, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HIDE_ROOT | wxTR_HAS_BUTTONS | wxTR_SINGLE | wxTR_LINES_AT_ROOT);
	client_tree_ctrl->SetBackgroundColour(Theme::Get(Theme::Role::Background));
	client_tree_ctrl->SetForegroundColour(Theme::Get(Theme::Role::Text));
	library_section->GetBodySizer()->Add(client_tree_ctrl, 1, wxEXPAND | wxBOTTOM, FromDIP(12));

	auto* action_sizer = new wxBoxSizer(wxHORIZONTAL);
	add_client_btn = new wxButton(library_section, wxID_ANY, "Add");
	add_client_btn->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_PLUS));
	action_sizer->Add(add_client_btn, 1, wxRIGHT, FromDIP(6));
	duplicate_client_btn = new wxButton(library_section, wxID_ANY, "Duplicate");
	duplicate_client_btn->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_COPY));
	action_sizer->Add(duplicate_client_btn, 1, wxRIGHT, FromDIP(6));
	delete_client_btn = new wxButton(library_section, wxID_ANY, "Remove");
	delete_client_btn->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_MINUS));
	action_sizer->Add(delete_client_btn, 1);
	library_section->GetBodySizer()->Add(action_sizer, 0, wxEXPAND);
	left_sizer->Add(library_section, 1, wxEXPAND | wxALL, FromDIP(10));

	auto* defaults_section = new PreferencesSectionPanel(
		left_panel,
		"Client Defaults",
		"These options apply globally to client loading and validation."
	);
	default_version_choice = new wxChoice(defaults_section, wxID_ANY);
	PreferencesLayout::AddControlRow(
		defaults_section,
		"Default client version",
		"Client selected by default in flows that need a preferred version.",
		default_version_choice,
		true
	);
	check_sigs_chkbox = PreferencesLayout::AddCheckBoxRow(
		defaults_section,
		"Check file signatures",
		"Validate DAT and SPR signatures when loading client assets to catch mismatched folders early.",
		g_settings.getBoolean(Config::CHECK_SIGNATURES)
	);
	left_sizer->Add(defaults_section, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(10));
	left_panel->SetSizer(left_sizer);

	auto* right_panel = new wxPanel(client_splitter, wxID_ANY);
	right_panel->SetBackgroundColour(GetBackgroundColour());
	auto* right_sizer = new wxBoxSizer(wxVERTICAL);

	detail_book = new wxSimplebook(right_panel, wxID_ANY);
	detail_book->SetBackgroundColour(GetBackgroundColour());

	auto* empty_panel = new wxPanel(detail_book, wxID_ANY);
	empty_panel->SetBackgroundColour(GetBackgroundColour());
	auto* empty_sizer = new wxBoxSizer(wxVERTICAL);
	auto* empty_section = new PreferencesSectionPanel(
		empty_panel,
		"No Client Selected",
		"Choose a client from the library to edit its compatibility, paths, signatures, and feature flags."
	);
	empty_section->GetBodySizer()->Add(
		PreferencesLayout::CreateBodyText(empty_section, "Use the search box on the left to narrow the list, then select a client entry to begin editing.", false),
		0,
		wxEXPAND
	);
	empty_sizer->AddStretchSpacer();
	empty_sizer->Add(empty_section, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(10));
	empty_sizer->AddStretchSpacer();
	empty_panel->SetSizer(empty_sizer);
	detail_book->AddPage(empty_panel, "Empty");

	auto* editor_panel = new wxPanel(detail_book, wxID_ANY);
	editor_panel->SetBackgroundColour(GetBackgroundColour());
	auto* editor_sizer = new wxBoxSizer(wxVERTICAL);

	auto* summary_section = new PreferencesSectionPanel(
		editor_panel,
		"Selected Client",
		"Review the active client at a glance before making detailed changes below."
	);
	summary_name_label = PreferencesLayout::CreateBodyText(summary_section, "", true);
	summary_name_label->SetFont(Theme::GetFont(12, true));
	summary_section->GetBodySizer()->Add(summary_name_label, 0, wxBOTTOM, FromDIP(4));
	summary_meta_label = PreferencesLayout::CreateBodyText(summary_section, "", false);
	summary_meta_label->SetForegroundColour(Theme::Get(Theme::Role::TextSubtle));
	summary_section->GetBodySizer()->Add(summary_meta_label, 0, wxBOTTOM, FromDIP(4));
	summary_dirty_label = PreferencesLayout::CreateBodyText(summary_section, "", true);
	summary_section->GetBodySizer()->Add(summary_dirty_label, 0);
	editor_sizer->Add(summary_section, 0, wxEXPAND | wxALL, FromDIP(10));

	auto* editor_section = new PreferencesSectionPanel(
		editor_panel,
		"Client Editor",
		"Use the property pages to edit identity, paths, compatibility, signatures, and feature flags."
	);
	client_prop_grid = new wxPropertyGridManager(
		editor_section,
		wxID_ANY,
		wxDefaultPosition,
		wxDefaultSize,
		wxPG_SPLITTER_AUTO_CENTER | wxPG_BOLD_MODIFIED | wxPG_DESCRIPTION | wxPG_TOOLBAR
	);
	client_prop_grid->SetBackgroundColour(Theme::Get(Theme::Role::Background));
	client_prop_grid->SetForegroundColour(Theme::Get(Theme::Role::Text));
	identity_page = client_prop_grid->AddPage("Identity");
	paths_page = client_prop_grid->AddPage("Paths & Files");
	compatibility_page = client_prop_grid->AddPage("Compatibility");
	signatures_page = client_prop_grid->AddPage("Signatures");
	features_page = client_prop_grid->AddPage("Features");
	editor_section->GetBodySizer()->Add(client_prop_grid, 1, wxEXPAND);
	editor_sizer->Add(editor_section, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(10));
	editor_panel->SetSizer(editor_sizer);
	detail_book->AddPage(editor_panel, "Editor");

	right_sizer->Add(detail_book, 1, wxEXPAND);
	right_panel->SetSizer(right_sizer);

	client_splitter->SplitVertically(left_panel, right_panel, FromDIP(320));
	client_splitter->SetSashGravity(0.30);
	client_splitter->SetMinimumPaneSize(FromDIP(250));
	main_sizer->Add(client_splitter, 1, wxEXPAND | wxALL, FromDIP(5));

	SetSizer(main_sizer);

	PopulateDefaultVersionChoice();
	PopulateClientTree();
	active_client = GetSelectedClient();
	if (active_client) {
		active_client->backup();
	}
	RefreshClientEditor();

	client_tree_ctrl->Bind(wxEVT_TREE_SEL_CHANGED, &ClientVersionPage::OnClientSelected, this);
	client_tree_ctrl->Bind(wxEVT_TREE_ITEM_MENU, &ClientVersionPage::OnTreeContextMenu, this);
	client_search_ctrl->Bind(wxEVT_TEXT, &ClientVersionPage::OnSearchChanged, this);
	client_search_ctrl->Bind(wxEVT_SEARCHCTRL_CANCEL_BTN, &ClientVersionPage::OnSearchChanged, this);
	client_prop_grid->Bind(wxEVT_PG_CHANGED, &ClientVersionPage::OnPropertyChanged, this);
	add_client_btn->Bind(wxEVT_BUTTON, &ClientVersionPage::OnAddClient, this);
	duplicate_client_btn->Bind(wxEVT_BUTTON, &ClientVersionPage::OnDuplicateClient, this);
	delete_client_btn->Bind(wxEVT_BUTTON, &ClientVersionPage::OnDeleteClient, this);
}

void ClientVersionPage::PopulateDefaultVersionChoice() {
	default_version_choice->Clear();
	ClientVersion* latest_version = ClientVersion::getLatestVersion();
	int latest_index = wxNOT_FOUND;
	int index = 0;
	for (auto* version : ClientVersion::getAll()) {
		default_version_choice->Append(wxstr(version->getName()));
		if (version == latest_version) {
			latest_index = index;
		}
		++index;
	}
	if (latest_index != wxNOT_FOUND) {
		default_version_choice->SetSelection(latest_index);
	} else if (default_version_choice->GetCount() > 0) {
		default_version_choice->SetSelection(0);
	}
}

bool ClientVersionPage::MatchesFilter(const ClientVersion& version) const {
	if (client_filter.empty()) {
		return true;
	}

	std::string haystack = LowercaseCopy(version.getName());
	haystack += " ";
	haystack += LowercaseCopy(version.getDescription());
	haystack += " ";
	haystack += std::to_string(version.getVersion());
	haystack += " ";
	haystack += std::to_string(version.getOtbId());
	return haystack.find(client_filter) != std::string::npos;
}

int ClientVersionPage::GetMajorGroup(const ClientVersion& version) const {
	const int protocol = static_cast<int>(version.getVersion());
	if (protocol >= 10000) {
		return protocol / 1000;
	}
	if (protocol >= 1000) {
		return protocol / 100;
	}
	if (protocol >= 100) {
		return protocol / 100;
	}
	return std::max(protocol / 10, 0);
}

void ClientVersionPage::PopulateClientTree() {
	ClientVersion* preferred_selection = active_client;
	client_tree_ctrl->DeleteAllItems();
	auto root = client_tree_ctrl->AddRoot("Clients");

	std::map<int, std::vector<ClientVersion*>> grouped_versions;
	for (auto* version : ClientVersion::getAll()) {
		if (MatchesFilter(*version)) {
			grouped_versions[GetMajorGroup(*version)].push_back(version);
		}
	}

	wxTreeItemId item_to_select;
	wxTreeItemId first_item;
	wxTreeItemId group_to_expand;

	for (const auto& [major_version, versions] : grouped_versions) {
		auto group = client_tree_ctrl->AppendItem(root, wxString::Format("%d.x", major_version));
		for (auto* version : versions) {
			auto item = client_tree_ctrl->AppendItem(group, wxstr(version->getName()), -1, -1, new TreeItemData(version));
			if (!first_item.IsOk()) {
				first_item = item;
			}
			if (version == preferred_selection) {
				item_to_select = item;
				group_to_expand = group;
			}
		}
		if (!client_filter.empty()) {
			client_tree_ctrl->Expand(group);
		} else {
			client_tree_ctrl->Collapse(group);
		}
	}

	if (!group_to_expand.IsOk()) {
		wxTreeItemIdValue cookie;
		group_to_expand = client_tree_ctrl->GetFirstChild(root, cookie);
	}
	if (group_to_expand.IsOk()) {
		client_tree_ctrl->Expand(group_to_expand);
	}

	ignore_tree_selection = true;
	if (item_to_select.IsOk()) {
		client_tree_ctrl->SelectItem(item_to_select);
		client_tree_ctrl->EnsureVisible(item_to_select);
	} else if (first_item.IsOk()) {
		client_tree_ctrl->SelectItem(first_item);
		client_tree_ctrl->EnsureVisible(first_item);
	} else {
		client_tree_ctrl->UnselectAll();
	}
	ignore_tree_selection = false;
}

void ClientVersionPage::SelectClient(ClientVersion* version) {
	if (!version) {
		ignore_tree_selection = true;
		client_tree_ctrl->UnselectAll();
		ignore_tree_selection = false;
		return;
	}

	auto root = client_tree_ctrl->GetRootItem();
	if (!root.IsOk()) {
		return;
	}

	wxTreeItemIdValue group_cookie;
	for (auto group = client_tree_ctrl->GetFirstChild(root, group_cookie); group.IsOk(); group = client_tree_ctrl->GetNextChild(root, group_cookie)) {
		wxTreeItemIdValue child_cookie;
		for (auto item = client_tree_ctrl->GetFirstChild(group, child_cookie); item.IsOk(); item = client_tree_ctrl->GetNextChild(group, child_cookie)) {
			auto* data = dynamic_cast<TreeItemData*>(client_tree_ctrl->GetItemData(item));
			if (data && data->cv == version) {
				ignore_tree_selection = true;
				client_tree_ctrl->Expand(group);
				client_tree_ctrl->SelectItem(item);
				client_tree_ctrl->EnsureVisible(item);
				ignore_tree_selection = false;
				return;
			}
		}
	}
}

ClientVersion* ClientVersionPage::GetSelectedClient() {
	auto selection = client_tree_ctrl->GetSelection();
	if (!selection.IsOk()) {
		return nullptr;
	}
	auto* data = dynamic_cast<TreeItemData*>(client_tree_ctrl->GetItemData(selection));
	return data ? data->cv : nullptr;
}

void ClientVersionPage::RefreshSummary() {
	if (!active_client) {
		summary_name_label->SetLabel("");
		summary_meta_label->SetLabel("");
		summary_dirty_label->SetLabel("");
		return;
	}

	summary_name_label->SetLabel(wxstr(active_client->getName()));
	summary_meta_label->SetLabel(wxString::Format(
		"Version %u | Config %s | OTB %u / %u",
		active_client->getVersion(),
		wxstr(active_client->getConfigType()),
		active_client->getOtbMajor(),
		active_client->getOtbId()
	));
	if (active_client->isDirty()) {
		summary_dirty_label->SetLabel("Unsaved changes");
		summary_dirty_label->SetForegroundColour(Theme::Get(Theme::Role::Warning));
	} else {
		summary_dirty_label->SetLabel("Saved");
		summary_dirty_label->SetForegroundColour(Theme::Get(Theme::Role::Success));
	}
	summary_name_label->GetParent()->Layout();
}

void ClientVersionPage::RefreshClientEditor() {
	identity_page->Clear();
	paths_page->Clear();
	compatibility_page->Clear();
	signatures_page->Clear();
	features_page->Clear();

	if (!active_client) {
		detail_book->SetSelection(0);
		RefreshSummary();
		return;
	}

	detail_book->SetSelection(1);
	RefreshSummary();

	auto* version_property = identity_page->Append(new wxIntProperty("Version ID", "Version", active_client->getVersion()));
	version_property->SetHelpString("Internal numeric client version, for example 860 or 1287.");
	auto* name_property = identity_page->Append(new wxStringProperty("Display Name", "Name", wxstr(active_client->getName())));
	name_property->SetHelpString("Name shown in version selectors and compatibility prompts.");
	auto* description_property = identity_page->Append(new wxStringProperty("Description", "description", wxstr(active_client->getDescription())));
	description_property->SetHelpString("Optional note describing the client, data source, or intended use.");
	wxPGChoices config_choices;
	config_choices.Add("dat_otb");
	config_choices.Add("dat_only");
	config_choices.Add("dat_srv");
	config_choices.Add("protobuf");
	auto* config_property = identity_page->Append(new wxEnumProperty("Configuration Type", "configType", config_choices, SelectionFromConfigType(active_client->getConfigType())));
	config_property->SetHelpString("How item definitions are loaded for this client.");

	auto* client_path_property = paths_page->Append(new wxDirProperty("Client Path", "clientPath", active_client->getClientPath().GetFullPath()));
	client_path_property->SetHelpString("Folder containing the configured client DAT and SPR files.");
	auto* data_directory_property = paths_page->Append(new wxStringProperty("Data Directory", "dataDirectory", wxstr(active_client->getDataDirectory())));
	data_directory_property->SetHelpString("Editor data folder used for this client version.");
	auto* metadata_property = paths_page->Append(new wxStringProperty("Metadata File (.dat)", "metadataFile", wxstr(active_client->getMetadataFile())));
	metadata_property->SetHelpString("File name of the DAT metadata file inside the client path.");
	auto* sprites_property = paths_page->Append(new wxStringProperty("Sprites File (.spr)", "spritesFile", wxstr(active_client->getSpritesFile())));
	sprites_property->SetHelpString("File name of the SPR sprite archive inside the client path.");

	auto* otb_id_property = compatibility_page->Append(new wxIntProperty("OTB ID", "otbId", active_client->getOtbId()));
	otb_id_property->SetHelpString("Minor items version used when comparing maps against this client.");
	auto* otb_major_property = compatibility_page->Append(new wxIntProperty("OTB Major Version", "otbMajor", active_client->getOtbMajor()));
	otb_major_property->SetHelpString("Major OTB format version for this client.");
	wxString otbm_versions_text;
	for (const auto version_id : active_client->getMapVersionsSupported()) {
		if (!otbm_versions_text.IsEmpty()) {
			otbm_versions_text += ", ";
		}
		otbm_versions_text += wxString::Format("%d", static_cast<int>(version_id) + 1);
	}
	auto* otbm_property = compatibility_page->Append(new wxStringProperty("Supported OTBM Versions", "otbmVersions", otbm_versions_text));
	otbm_property->SetHelpString("Comma-separated list of supported OTBM versions, usually 1 through 4.");

	auto* dat_signature_property = signatures_page->Append(new wxStringProperty("DAT Signature", "datSignature", wxString::Format("%X", active_client->getDatSignature())));
	dat_signature_property->SetHelpString("Expected DAT signature written in hexadecimal.");
	auto* spr_signature_property = signatures_page->Append(new wxStringProperty("SPR Signature", "sprSignature", wxString::Format("%X", active_client->getSprSignature())));
	spr_signature_property->SetHelpString("Expected SPR signature written in hexadecimal.");

	features_page->Append(new wxBoolProperty("Transparency", "transparency", active_client->isTransparent()))->SetHelpString("Enable transparency handling for this client.");
	features_page->Append(new wxBoolProperty("Extended", "extended", active_client->isExtended()))->SetHelpString("Use the extended sprite/item feature set for this client.");
	features_page->Append(new wxBoolProperty("Frame Durations", "frameDurations", active_client->hasFrameDurations()))->SetHelpString("Treat animations as having per-frame durations.");
	features_page->Append(new wxBoolProperty("Frame Groups", "frameGroups", active_client->hasFrameGroups()))->SetHelpString("Treat animations as having separate frame groups.");

	for (auto* page : { identity_page, paths_page, compatibility_page, signatures_page, features_page }) {
		wxPropertyGridIterator iterator = page->GetIterator();
		for (; !iterator.AtEnd(); ++iterator) {
			UpdatePropertyValidation(*iterator);
		}
	}

	client_prop_grid->SelectPage(0);
	client_prop_grid->Refresh();
}

bool ClientVersionPage::ResolvePendingChanges(ClientVersion* client) {
	if (!client || !client->isDirty()) {
		return true;
	}

	const int result = wxMessageBox(
		"Save changes to " + wxstr(client->getName()) + "?",
		"Unsaved Changes",
		wxYES_NO | wxCANCEL | wxICON_QUESTION,
		this
	);
	if (result == wxYES) {
		if (!ClientVersion::saveVersions()) {
			wxMessageBox("Failed to save client versions. Changes were not saved.", "Save Error", wxOK | wxICON_ERROR, this);
			return false;
		}
		for (auto* version : ClientVersion::getAll()) {
			version->clearDirty();
			version->backup();
		}
		return true;
	}
	if (result == wxNO) {
		client->restore();
		client->clearDirty();
		return true;
	}
	return false;
}

void ClientVersionPage::UpdatePropertyValidation(wxPGProperty* prop) {
	if (!prop) {
		return;
	}

	bool invalid = false;
	if (prop->GetName() == "Name") {
		const auto name = nstr(prop->GetValueAsString());
		invalid = name.empty();
		if (!invalid && active_client) {
			for (auto* other : ClientVersion::getAll()) {
				if (other != active_client && other->getName() == name) {
					invalid = true;
					break;
				}
			}
		}
	}

	prop->SetBackgroundColour(invalid ? wxColour(255, 210, 210) : Theme::Get(Theme::Role::Background));
}

void ClientVersionPage::OnClientSelected(wxTreeEvent& WXUNUSED(event)) {
	if (ignore_tree_selection) {
		return;
	}

	ClientVersion* requested_client = GetSelectedClient();
	if (requested_client == active_client) {
		RefreshClientEditor();
		return;
	}

	const bool had_dirty_changes = active_client && active_client->isDirty();
	ClientVersion* previous_client = active_client;
	if (!ResolvePendingChanges(previous_client)) {
		ignore_tree_selection = true;
		SelectClient(previous_client);
		ignore_tree_selection = false;
		return;
	}

	if (had_dirty_changes) {
		PopulateDefaultVersionChoice();
		PopulateClientTree();
		ignore_tree_selection = true;
		SelectClient(requested_client);
		ignore_tree_selection = false;
		requested_client = GetSelectedClient();
	}

	active_client = requested_client;
	if (active_client) {
		active_client->backup();
	}
	RefreshClientEditor();
}

void ClientVersionPage::OnSearchChanged(wxCommandEvent& WXUNUSED(event)) {
	ClientVersion* previous_client = active_client;
	client_filter = LowercaseCopy(nstr(client_search_ctrl->GetValue()));
	PopulateClientTree();
	active_client = GetSelectedClient();
	if (active_client && active_client != previous_client) {
		active_client->backup();
	}
	RefreshClientEditor();
}

void ClientVersionPage::OnTreeContextMenu(wxTreeEvent& event) {
	auto* data = dynamic_cast<TreeItemData*>(client_tree_ctrl->GetItemData(event.GetItem()));
	if (!data || !data->cv) {
		return;
	}

	wxMenu menu;
	menu.Append(wxID_COPY, "Duplicate")->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_COPY));
	menu.Bind(wxEVT_MENU, &ClientVersionPage::OnDuplicateClient, this, wxID_COPY);
	PopupMenu(&menu);
}

void ClientVersionPage::OnDuplicateClient(wxCommandEvent& WXUNUSED(event)) {
	ClientVersion* current = active_client ? active_client : GetSelectedClient();
	if (!current) {
		return;
	}

	auto duplicate = current->clone();
	duplicate->setName(current->getName() + " (Copy)");
	duplicate->markDirty();

	ClientVersion* duplicate_ptr = duplicate.get();
	ClientVersion::addVersion(std::move(duplicate));
	PopulateDefaultVersionChoice();
	PopulateClientTree();
	active_client = duplicate_ptr;
	active_client->backup();
	active_client->markDirty();
	SelectClient(active_client);
	RefreshClientEditor();
}

void ClientVersionPage::OnPropertyChanged(wxPropertyGridEvent& event) {
	ClientVersion* client = active_client;
	if (!client) {
		return;
	}

	auto* prop = event.GetProperty();
	const auto prop_name = prop->GetName();
	const wxAny value = prop->GetValue();

	if (prop_name == "Version") {
		client->setVersion(value.As<int>());
	} else if (prop_name == "Name") {
		client->setName(nstr(value.As<wxString>()));
	} else if (prop_name == "description") {
		client->setDescription(nstr(value.As<wxString>()));
	} else if (prop_name == "configType") {
		client->setConfigType(ConfigTypeFromSelection(prop->GetValue().GetLong()));
	} else if (prop_name == "clientPath") {
		client->setClientPath(FileName(value.As<wxString>()));
	} else if (prop_name == "dataDirectory") {
		client->setDataDirectory(nstr(value.As<wxString>()));
	} else if (prop_name == "metadataFile") {
		client->setMetadataFile(nstr(value.As<wxString>()));
	} else if (prop_name == "spritesFile") {
		client->setSpritesFile(nstr(value.As<wxString>()));
	} else if (prop_name == "otbId") {
		client->setOtbId(value.As<int>());
	} else if (prop_name == "otbMajor") {
		client->setOtbMajor(value.As<int>());
	} else if (prop_name == "otbmVersions") {
		client->getMapVersionsSupported().clear();
		wxStringTokenizer tokenizer(value.As<wxString>(), ", ");
		while (tokenizer.HasMoreTokens()) {
			long version = 0;
			if (tokenizer.GetNextToken().ToLong(&version) && version >= 1 && version <= 4) {
				client->getMapVersionsSupported().push_back(static_cast<MapVersionID>(version - 1));
			}
		}
	} else if (prop_name == "datSignature") {
		uint32_t signature = 0;
		auto text = nstr(value.As<wxString>());
		if (std::from_chars(text.data(), text.data() + text.size(), signature, 16).ec == std::errc()) {
			client->setDatSignature(signature);
		} else {
			prop->SetValue(wxString::Format("%X", client->getDatSignature()));
		}
	} else if (prop_name == "sprSignature") {
		uint32_t signature = 0;
		auto text = nstr(value.As<wxString>());
		if (std::from_chars(text.data(), text.data() + text.size(), signature, 16).ec == std::errc()) {
			client->setSprSignature(signature);
		} else {
			prop->SetValue(wxString::Format("%X", client->getSprSignature()));
		}
	} else if (prop_name == "transparency") {
		client->setTransparent(value.As<bool>());
	} else if (prop_name == "extended") {
		client->setExtended(value.As<bool>());
	} else if (prop_name == "frameDurations") {
		client->setFrameDurations(value.As<bool>());
	} else if (prop_name == "frameGroups") {
		client->setFrameGroups(value.As<bool>());
	}

	client->markDirty();
	UpdatePropertyValidation(prop);
	PopulateDefaultVersionChoice();
	if (prop_name == "Name" || prop_name == "Version") {
		PopulateClientTree();
		SelectClient(client);
	}
	RefreshSummary();
}

void ClientVersionPage::OnAddClient(wxCommandEvent& WXUNUSED(event)) {
	std::string new_name = "New Client";
	int counter = 1;
	while (ClientVersion::get(new_name)) {
		new_name = "New Client " + std::to_string(counter++);
	}

	OtbVersion otb;
	otb.name = new_name;
	otb.id = PROTOCOL_VERSION_NONE;
	otb.format_version = OTB_VERSION_1;

	auto client = std::make_unique<ClientVersion>(otb, new_name, kDefaultDataDirectory);
	client->setMetadataFile("Tibia.dat");
	client->setSpritesFile("Tibia.spr");
	client->markDirty();

	ClientVersion* client_ptr = client.get();
	ClientVersion::addVersion(std::move(client));
	PopulateDefaultVersionChoice();
	PopulateClientTree();
	active_client = client_ptr;
	active_client->backup();
	active_client->markDirty();
	SelectClient(active_client);
	RefreshClientEditor();
}

void ClientVersionPage::OnDeleteClient(wxCommandEvent& WXUNUSED(event)) {
	ClientVersion* client = active_client ? active_client : GetSelectedClient();
	if (!client) {
		return;
	}

	if (wxMessageBox("Are you sure you want to delete " + wxstr(client->getName()) + "?", "Confirm Delete", wxYES_NO | wxICON_WARNING, this) != wxYES) {
		return;
	}

	ClientVersion::removeVersion(client->getName());
	if (!ClientVersion::saveVersions()) {
		wxMessageBox("Could not save client versions to disk. The changes will be reverted.", "Error", wxOK | wxICON_ERROR, this);
		ClientVersion::loadVersions();
	}

	PopulateDefaultVersionChoice();
	active_client = nullptr;
	PopulateClientTree();
	active_client = GetSelectedClient();
	if (active_client) {
		active_client->backup();
	}
	RefreshClientEditor();
}

bool ClientVersionPage::ValidateData() {
	for (auto* client : ClientVersion::getAll()) {
		if (!client->isValid()) {
			wxMessageBox("Client '" + wxstr(client->getName()) + "' has invalid data. The name cannot be empty.", "Invalid Client Data", wxOK | wxICON_ERROR, this);
			SelectClient(client);
			active_client = client;
			RefreshClientEditor();
			return false;
		}
		for (auto* other : ClientVersion::getAll()) {
			if (client != other && client->getName() == other->getName()) {
				wxMessageBox("Client '" + wxstr(client->getName()) + "' has a duplicate name. Rename one of them before saving.", "Duplicate Client Name", wxOK | wxICON_ERROR, this);
				SelectClient(client);
				active_client = client;
				RefreshClientEditor();
				return false;
			}
		}
	}
	return true;
}

void ClientVersionPage::Apply() {
	g_settings.setInteger(Config::CHECK_SIGNATURES, check_sigs_chkbox->GetValue());

	if (default_version_choice->GetSelection() != wxNOT_FOUND) {
		const auto default_name = nstr(default_version_choice->GetStringSelection());
		if (auto* default_client = ClientVersion::get(default_name)) {
			ClientVersion::setLatestVersion(default_client);
			g_settings.setInteger(Config::DEFAULT_CLIENT_VERSION, default_client->getProtocolID());
		}
	}

	bool any_dirty = false;
	for (auto* client : ClientVersion::getAll()) {
		if (client->isDirty()) {
			any_dirty = true;
			break;
		}
	}
	if (!any_dirty) {
		return;
	}

	if (ClientVersion::saveVersions()) {
		for (auto* client : ClientVersion::getAll()) {
			client->clearDirty();
			client->backup();
		}
		RefreshSummary();
	} else {
		wxMessageBox("Failed to save client versions. Check the log for details.", "Save Error", wxOK | wxICON_ERROR, this);
	}
}



