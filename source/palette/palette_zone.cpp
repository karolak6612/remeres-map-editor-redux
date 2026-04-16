#include "app/main.h"

#include "palette/palette_zone.h"

#include "brushes/managers/brush_manager.h"
#include "brushes/zone/zone_brush.h"
#include "editor/editor.h"
#include "map/map.h"
#include "map/tile.h"
#include "ui/gui.h"
#include "util/image_manager.h"

#include <wx/bmpbuttn.h>
#include <wx/listbox.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>

namespace {
constexpr const char* DEFAULT_ZONE_NAME = "Zone 1";
constexpr int BUTTON_ICON_SIZE = 16;
}

ZonePalettePanel::ZonePalettePanel(wxWindow* parent, wxWindowID id) :
	PalettePanel(parent, id),
	map(nullptr),
	zone_list(nullptr),
	zone_text(nullptr),
	add_button(nullptr),
	rename_button(nullptr),
	remove_button(nullptr),
	mutating_ui(false) {
	auto* top_sizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Zones");

	zone_list = newd wxListBox(top_sizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE);
	top_sizer->Add(zone_list, 1, wxEXPAND | wxALL, FromDIP(5));

	zone_text = newd wxTextCtrl(top_sizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	zone_text->SetToolTip("Select an existing zone or type a new zone name.");
	top_sizer->Add(zone_text, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));

	auto* first_row = newd wxBoxSizer(wxHORIZONTAL);
	const wxSize button_size = FromDIP(wxSize(30, 30));
	add_button = newd wxBitmapButton(top_sizer->GetStaticBox(), wxID_ADD, IMAGE_MANAGER.GetBitmap(ICON_PLUS, wxSize(BUTTON_ICON_SIZE, BUTTON_ICON_SIZE)), wxDefaultPosition, button_size);
	add_button->SetToolTip("Add zone");
	first_row->Add(add_button, 1, wxEXPAND | wxRIGHT, FromDIP(4));

	rename_button = newd wxBitmapButton(top_sizer->GetStaticBox(), wxID_ANY, IMAGE_MANAGER.GetBitmap(ICON_PENCIL, wxSize(BUTTON_ICON_SIZE, BUTTON_ICON_SIZE)), wxDefaultPosition, button_size);
	rename_button->SetToolTip("Rename zone");
	first_row->Add(rename_button, 1, wxEXPAND | wxRIGHT, FromDIP(4));

	remove_button = newd wxBitmapButton(top_sizer->GetStaticBox(), wxID_REMOVE, IMAGE_MANAGER.GetBitmap(ICON_MINUS, wxSize(BUTTON_ICON_SIZE, BUTTON_ICON_SIZE)), wxDefaultPosition, button_size);
	remove_button->SetToolTip("Remove zone");
	first_row->Add(remove_button, 1, wxEXPAND);
	top_sizer->Add(first_row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));

	SetSizerAndFit(top_sizer);

	zone_list->Bind(wxEVT_LISTBOX, &ZonePalettePanel::OnSelectZone, this);
	zone_text->Bind(wxEVT_TEXT, &ZonePalettePanel::OnZoneTextChanged, this);
	zone_text->Bind(wxEVT_TEXT_ENTER, &ZonePalettePanel::OnAddZone, this);
	add_button->Bind(wxEVT_BUTTON, &ZonePalettePanel::OnAddZone, this);
	rename_button->Bind(wxEVT_BUTTON, &ZonePalettePanel::OnRenameZone, this);
	remove_button->Bind(wxEVT_BUTTON, &ZonePalettePanel::OnRemoveZone, this);

	RefreshZoneList();
}

PaletteType ZonePalettePanel::GetType() const {
	return TILESET_ZONES;
}

wxString ZonePalettePanel::GetName() const {
	return "Zone Palette";
}

Brush* ZonePalettePanel::GetSelectedBrush() const {
	return g_brush_manager.zone_brush;
}

int ZonePalettePanel::GetSelectedBrushSize() const {
	return 0;
}

bool ZonePalettePanel::SelectBrush(const Brush* whatbrush) {
	if (whatbrush != g_brush_manager.zone_brush) {
		return false;
	}

	RefreshZoneList();
	return true;
}

void ZonePalettePanel::OnSwitchIn() {
	PalettePanel::OnSwitchIn();
	RefreshZoneList();
}

void ZonePalettePanel::OnUpdate() {
	RefreshZoneList();
}

void ZonePalettePanel::SetMap(Map* new_map) {
	map = new_map;
	UpdateButtonState();
	RefreshZoneList();
}

void ZonePalettePanel::OnSelectZone(wxCommandEvent& event) {
	(void)event;
	if (IsMutatingUi() || !zone_list) {
		return;
	}

	const auto selection = zone_list->GetSelection();
	if (selection == wxNOT_FOUND) {
		return;
	}

	const std::string zone_name = nstr(zone_list->GetString(selection));
	SyncZoneText(zone_name);
	g_brush_manager.SetSelectedZone(zone_name);
	UpdateButtonState();
}

void ZonePalettePanel::OnAddZone(wxCommandEvent& event) {
	(void)event;
	if (IsMutatingUi() || !HasMap()) {
		return;
	}

	const std::string zone_name = nstr(zone_text->GetValue());
	if (zone_name.empty()) {
		return;
	}

	if (map->zones.ensureZone(zone_name) == 0) {
		wxMessageBox("Failed to create the selected zone.", "Add Zone", wxOK | wxICON_ERROR, this);
		return;
	}
	g_brush_manager.SetSelectedZone(zone_name);
	g_gui.RefreshPalettes(map);
	RefreshZoneList();
}

void ZonePalettePanel::OnRenameZone(wxCommandEvent& event) {
	(void)event;
	if (IsMutatingUi() || !HasMap()) {
		return;
	}

	const auto zone_id = GetSelectedZoneId();
	if (!zone_id) {
		return;
	}

	const std::string old_name = map->zones.findName(*zone_id);
	if (old_name.empty()) {
		return;
	}

	wxTextEntryDialog dialog(wxGetTopLevelParent(this), "Rename the selected zone.", "Rename Zone", wxstr(old_name));
	if (dialog.ShowModal() != wxID_OK) {
		return;
	}

	const std::string new_name = nstr(dialog.GetValue());
	if (new_name.empty() || new_name == old_name) {
		return;
	}

	if (const auto existing = map->zones.findId(new_name); existing && *existing != *zone_id) {
		wxMessageBox("A zone with that name already exists.", "Rename Zone", wxOK | wxICON_WARNING, this);
		return;
	}

	map->zones.removeZone(*zone_id);
	if (!map->zones.addZone(new_name, *zone_id)) {
		map->zones.addZone(old_name, *zone_id);
		wxMessageBox("Failed to rename the selected zone.", "Rename Zone", wxOK | wxICON_ERROR, this);
		return;
	}

	g_brush_manager.SetSelectedZone(new_name);
	g_gui.RefreshPalettes(map);
	RefreshZoneList();
	SyncSelection(new_name);
}

void ZonePalettePanel::OnRemoveZone(wxCommandEvent& event) {
	(void)event;
	if (IsMutatingUi() || !HasMap()) {
		return;
	}

	const auto zone_id = GetSelectedZoneId();
	if (!zone_id) {
		return;
	}

	for (auto& tile_location : map->tiles()) {
		if (Tile* tile = tile_location.get(); tile && tile->hasZone(*zone_id)) {
			tile->removeZone(*zone_id);
		}
	}

	map->zones.removeZone(*zone_id);
	const std::string fallback_zone_name = map->zones.empty() ? std::string { DEFAULT_ZONE_NAME } : map->zones.begin()->first;
	g_brush_manager.SetSelectedZone(fallback_zone_name);
	g_gui.RefreshPalettes(map);
	RefreshZoneList();
}

void ZonePalettePanel::OnZoneTextChanged(wxCommandEvent& event) {
	(void)event;
	if (IsMutatingUi()) {
		return;
	}

	const std::string zone_name = nstr(zone_text->GetValue());
	g_brush_manager.SetSelectedZone(zone_name);
	SyncSelection(zone_name);
	UpdateButtonState();
}

std::string ZonePalettePanel::GetSelectedZoneName() const {
	if (!zone_list || zone_list->GetSelection() == wxNOT_FOUND || !HasMap()) {
		return {};
	}

	const std::string zone_name = nstr(zone_list->GetString(zone_list->GetSelection()));
	return map->zones.findId(zone_name) ? zone_name : std::string {};
}

std::optional<uint16_t> ZonePalettePanel::GetSelectedZoneId() const {
	const std::string zone_name = GetSelectedZoneName();
	if (zone_name.empty() || !HasMap()) {
		return std::nullopt;
	}

	return map->zones.findId(zone_name);
}

std::string ZonePalettePanel::ResolvePreferredZoneName() const {
	if (!map) {
		return DEFAULT_ZONE_NAME;
	}

	const std::string brush_zone_name = std::string { g_brush_manager.GetSelectedZone() };
	if (!brush_zone_name.empty()) {
		return brush_zone_name;
	}

	const std::string typed_zone_name = zone_text && !zone_text->GetValue().IsEmpty() ? nstr(zone_text->GetValue()) : std::string {};
	if (!typed_zone_name.empty()) {
		return typed_zone_name;
	}

	if (const auto& zones = map->zones; !zones.empty()) {
		return zones.begin()->first;
	}

	return DEFAULT_ZONE_NAME;
}

void ZonePalettePanel::RefreshZoneList() {
	if (!zone_list || !zone_text) {
		return;
	}

	const std::string preferred_name = ResolvePreferredZoneName();
	SetMutatingUi(true);
	zone_list->Freeze();
	zone_list->Clear();

	if (map) {
		for (const auto& [name, id] : map->zones) {
			(void)id;
			zone_list->Append(wxstr(name));
		}
	}

	if (!preferred_name.empty() && zone_list->FindString(wxstr(preferred_name)) == wxNOT_FOUND) {
		zone_list->Append(wxstr(preferred_name));
	}

	SyncZoneText(preferred_name);
	SyncSelection(preferred_name);
	UpdateButtonState();

	zone_list->Thaw();
	SetMutatingUi(false);
}

void ZonePalettePanel::SyncSelection(const std::string& zone_name) {
	if (!zone_list) {
		return;
	}

	const wxString wx_zone_name = wxstr(zone_name);
	const int selection = zone_list->FindString(wx_zone_name);
	if (selection != wxNOT_FOUND) {
		zone_list->SetSelection(selection);
	}
}

void ZonePalettePanel::SyncZoneText(const std::string& zone_name) {
	if (!zone_text) {
		return;
	}

	zone_text->ChangeValue(wxstr(zone_name));
}

void ZonePalettePanel::UpdateButtonState() {
	const bool has_map = HasMap();
	const bool has_selection = GetSelectedZoneId().has_value();
	add_button->Enable(has_map);
	rename_button->Enable(has_map && has_selection);
	remove_button->Enable(has_map && has_selection);
}

void ZonePalettePanel::SetMutatingUi(bool value) {
	mutating_ui = value;
}

bool ZonePalettePanel::IsMutatingUi() const {
	return mutating_ui;
}

bool ZonePalettePanel::HasMap() const {
	return map != nullptr;
}
