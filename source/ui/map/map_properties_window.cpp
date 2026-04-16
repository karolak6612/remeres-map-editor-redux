#include "ui/map/map_properties_window.h"

#include "editor/editor.h"
#include "map/map.h"
#include "editor/operations/map_version_changer.h"
#include "ui/gui.h"
#include "app/managers/version_manager.h"
#include "ui/dialog_util.h"
#include "ui/map_tab.h"
#include "util/image_manager.h"

#include <array>

namespace {
struct VersionChoiceEntry {
	const char* label;
	MapVersionID version;
};

constexpr auto kVersionChoices = std::to_array<VersionChoiceEntry>({
	VersionChoiceEntry { "OTServ 0.5.0", MAP_OTBM_1 },
	VersionChoiceEntry { "OTServ 0.6.0", MAP_OTBM_2 },
	VersionChoiceEntry { "OTServ 0.6.1", MAP_OTBM_3 },
	VersionChoiceEntry { "OTServ 0.7.0 (revscriptsys)", MAP_OTBM_4 },
	VersionChoiceEntry { "Canary (OTBM 5)", MAP_OTBM_5 },
	VersionChoiceEntry { "Canary (OTBM 6)", MAP_OTBM_6 },
});

[[nodiscard]] size_t versionChoiceIndex(MapVersionID version) {
	for (size_t index = 0; index < kVersionChoices.size(); ++index) {
		if (kVersionChoices[index].version == version) {
			return index;
		}
	}
	return 0;
}

[[nodiscard]] MapVersionID versionChoiceValue(int selection) {
	if (selection >= 0 && static_cast<size_t>(selection) < kVersionChoices.size()) {
		return kVersionChoices[selection].version;
	}
	return MAP_OTBM_1;
}
}

MapPropertiesWindow::MapPropertiesWindow(wxWindow* parent, MapTab* view, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Map Properties", wxDefaultPosition, FROM_DIP(parent, wxSize(300, 200)), wxRESIZE_BORDER | wxCAPTION),
	view(view),
	editor(editor) {
	// Setup data variabels
	Map& map = editor.map;

	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	wxFlexGridSizer* grid_sizer = newd wxFlexGridSizer(2, 10, 10);
	grid_sizer->AddGrowableCol(1);

	// Description
	grid_sizer->Add(newd wxStaticText(this, wxID_ANY, "Map Description"));
	description_ctrl = newd wxTextCtrl(this, wxID_ANY, wxstr(map.getMapDescription()), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	description_ctrl->SetToolTip("Enter a description for the map");
	grid_sizer->Add(description_ctrl, wxSizerFlags(1).Expand());

	// Map version
	grid_sizer->Add(newd wxStaticText(this, wxID_ANY, "Map Version"));
	version_choice = newd wxChoice(this, MAP_PROPERTIES_VERSION);
	version_choice->SetToolTip("Select the OTBM version (Determines feature support)");
	for (const auto& entry : kVersionChoices) {
		version_choice->Append(entry.label);
	}
	version_choice->SetSelection(static_cast<int>(versionChoiceIndex(map.getVersion().otbm)));

	grid_sizer->Add(version_choice, wxSizerFlags(1).Expand());

	// Version
	grid_sizer->Add(newd wxStaticText(this, wxID_ANY, "Client Version"));
	protocol_choice = newd wxChoice(this, wxID_ANY);
	protocol_choice->SetToolTip("Select the target client version");

	protocol_choice->SetStringSelection(wxstr(g_version.GetCurrentVersion().getName()));

	grid_sizer->Add(protocol_choice, wxSizerFlags(1).Expand());

	// Dimensions
	grid_sizer->Add(newd wxStaticText(this, wxID_ANY, "Map Dimensions"));
	{
		wxSizer* subsizer = newd wxBoxSizer(wxHORIZONTAL);
		subsizer->Add(
			width_spin = newd wxSpinCtrl(this, wxID_ANY, wxstr(i2s(map.getWidth())), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 256, MAP_MAX_WIDTH), wxSizerFlags(1).Expand()
		);
		width_spin->SetToolTip("Map width in tiles");
		subsizer->Add(
			height_spin = newd wxSpinCtrl(this, wxID_ANY, wxstr(i2s(map.getHeight())), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 256, MAP_MAX_HEIGHT), wxSizerFlags(1).Expand()
		);
		height_spin->SetToolTip("Map height in tiles");
		grid_sizer->Add(subsizer, 1, wxEXPAND);
	}

	// External files
	grid_sizer->Add(
		newd wxStaticText(this, wxID_ANY, "External Housefile")
	);

	grid_sizer->Add(
		house_filename_ctrl = newd wxTextCtrl(this, wxID_ANY, wxstr(map.getHouseFilename())), 1, wxEXPAND
	);
	house_filename_ctrl->SetToolTip("External house XML file (leave empty for internal)");

	grid_sizer->Add(
		newd wxStaticText(this, wxID_ANY, "External Spawnfile")
	);

	grid_sizer->Add(
		spawn_filename_ctrl = newd wxTextCtrl(this, wxID_ANY, wxstr(map.getSpawnFilename())), 1, wxEXPAND
	);
	spawn_filename_ctrl->SetToolTip("External spawn XML file (leave empty for internal)");

	grid_sizer->Add(
		newd wxStaticText(this, wxID_ANY, "External NPC Spawnfile")
	);

	grid_sizer->Add(
		spawn_npc_filename_ctrl = newd wxTextCtrl(this, wxID_ANY, wxstr(map.getSpawnNpcFilename())), 1, wxEXPAND
	);
	spawn_npc_filename_ctrl->SetToolTip("External NPC spawn XML file (leave empty for internal)");

	grid_sizer->Add(
		newd wxStaticText(this, wxID_ANY, "External Zonefile")
	);

	grid_sizer->Add(
		zone_filename_ctrl = newd wxTextCtrl(this, wxID_ANY, wxstr(map.getZoneFilename())), 1, wxEXPAND
	);
	zone_filename_ctrl->SetToolTip("External zone XML file (leave empty for internal)");

	grid_sizer->Add(
		newd wxStaticText(this, wxID_ANY, "External Waypointfile")
	);

	grid_sizer->Add(
		waypoint_filename_ctrl = newd wxTextCtrl(this, wxID_ANY, wxstr(map.getWaypointFilename())), 1, wxEXPAND
	);
	waypoint_filename_ctrl->SetToolTip("External waypoint XML file (leave empty for internal)");

	topsizer->Add(grid_sizer, wxSizerFlags(1).Expand().Border(wxALL, 20));

	wxSizer* subsizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* okBtn = newd wxButton(this, wxID_OK, "OK");
	okBtn->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_CHECK));
	okBtn->SetToolTip("Confirm changes");
	subsizer->Add(okBtn, wxSizerFlags(1).Center());

	wxButton* cancelBtn = newd wxButton(this, wxID_CANCEL, "Cancel");
	cancelBtn->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_XMARK));
	cancelBtn->SetToolTip("Discard changes");
	subsizer->Add(cancelBtn, wxSizerFlags(1).Center());
	topsizer->Add(subsizer, wxSizerFlags(0).Center().Border(wxLEFT | wxRIGHT | wxBOTTOM, 20));

	SetSizerAndFit(topsizer);
	Centre(wxBOTH);
	UpdateProtocolList();

	ClientVersion* current_version = ClientVersion::getBestMatch(map.getVersion().client);
	if (current_version) {
		protocol_choice->SetStringSelection(wxstr(current_version->getName()));
	}

	version_choice->Bind(wxEVT_CHOICE, &MapPropertiesWindow::OnChangeVersion, this);
	okBtn->Bind(wxEVT_BUTTON, &MapPropertiesWindow::OnClickOK, this);
	cancelBtn->Bind(wxEVT_BUTTON, &MapPropertiesWindow::OnClickCancel, this);

	SetIcons(IMAGE_MANAGER.GetIconBundle(ICON_GEAR));
}

void MapPropertiesWindow::UpdateProtocolList() {
	wxString ver = version_choice->GetStringSelection();
	wxString client = protocol_choice->GetStringSelection();

	protocol_choice->Clear();

	ClientVersionList versions;
	if (g_settings.getInteger(Config::USE_OTBM_4_FOR_ALL_MAPS)) {
		versions = ClientVersion::getAllVisible();
	} else {
		MapVersionID map_version = versionChoiceValue(version_choice->GetSelection());
		ClientVersionList protocols = ClientVersion::getAllForOTBMVersion(map_version);
		for (ClientVersionList::const_iterator p = protocols.begin(); p != protocols.end(); ++p) {
			protocol_choice->Append(wxstr((*p)->getName()));
		}
	}
	protocol_choice->SetSelection(0);
	protocol_choice->SetStringSelection(client);
}

void MapPropertiesWindow::OnChangeVersion(wxCommandEvent&) {
	UpdateProtocolList();
}

void MapPropertiesWindow::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	Map& map = editor.map;

	MapVersion old_ver = map.getVersion();
	MapVersion new_ver;

	new_ver.client = ClientVersion::get(nstr(protocol_choice->GetStringSelection()))->getProtocolID();
	new_ver.otbm = versionChoiceValue(version_choice->GetSelection());

	if (!MapVersionChanger::changeMapVersion(this, editor, new_ver)) {
		return;
	}

	map.setMapDescription(nstr(description_ctrl->GetValue()));
	map.setHouseFilename(nstr(house_filename_ctrl->GetValue()));
	map.setSpawnFilename(nstr(spawn_filename_ctrl->GetValue()));
	map.setSpawnNpcFilename(nstr(spawn_npc_filename_ctrl->GetValue()));
	map.setZoneFilename(nstr(zone_filename_ctrl->GetValue()));
	map.setWaypointFilename(nstr(waypoint_filename_ctrl->GetValue()));

	// Only resize if we have to
	int new_map_width = width_spin->GetValue();
	int new_map_height = height_spin->GetValue();
	if (new_map_width != map.getWidth() || new_map_height != map.getHeight()) {
		map.setWidth(new_map_width);
		map.setHeight(new_map_height);
		g_gui.FitViewToMap(view);
	}
	g_gui.RefreshPalettes();

	EndModal(1);
}

void MapPropertiesWindow::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	// Just close this window
	EndModal(1);
}

MapPropertiesWindow::~MapPropertiesWindow() = default;
