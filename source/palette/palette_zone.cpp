#include "app/main.h"

#include "palette/palette_zone.h"

#include "brushes/managers/brush_manager.h"
#include "brushes/zone/zone_brush.h"
#include "editor/editor.h"
#include "map/map.h"
#include "map/tile.h"
#include "rendering/drawers/tiles/tile_color_calculator.h"
#include "ui/gui.h"

#include <wx/button.h>
#include <wx/dataview.h>
#include <wx/dcmemory.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>

#include <algorithm>
#include <array>
#include <vector>

namespace {
constexpr const char* DEFAULT_ZONE_NAME = "Zone 1";

[[nodiscard]] std::vector<std::pair<uint16_t, std::string>> sortedZones(const ZoneRegistry& zones) {
	std::vector<std::pair<uint16_t, std::string>> entries;
	entries.reserve(std::distance(zones.begin(), zones.end()));
	for (const auto& [name, id] : zones) {
		entries.emplace_back(id, name);
	}

	std::ranges::sort(entries, {}, &std::pair<uint16_t, std::string>::first);
	return entries;
}

[[nodiscard]] wxBitmap makeZoneColorBitmap(uint16_t zone_id) {
	uint8_t red = 255;
	uint8_t green = 255;
	uint8_t blue = 255;
	TileColorCalculator::GetZoneColor(zone_id, red, green, blue);

	wxBitmap bitmap(16, 16);
	wxMemoryDC dc;
	dc.SelectObject(bitmap);
	dc.SetBackground(*wxTRANSPARENT_BRUSH);
	dc.Clear();
	dc.SetPen(wxPen(wxColour(32, 32, 32)));
	dc.SetBrush(wxBrush(wxColour(red, green, blue)));
	dc.DrawRoundedRectangle(1, 1, 14, 14, 3);
	dc.SelectObject(wxNullBitmap);
	return bitmap;
}
}

ZonePalettePanel::ZonePalettePanel(wxWindow* parent, wxWindowID id) :
	PalettePanel(parent, id),
	map(nullptr),
	zone_list(nullptr),
	add_button(nullptr),
	edit_button(nullptr),
	remove_button(nullptr),
	mutating_ui(false) {
	auto* main_sizer = newd wxBoxSizer(wxVERTICAL);
	auto* list_box = newd wxStaticBoxSizer(wxVERTICAL, this, "Zones");

	zone_list = newd wxDataViewListCtrl(list_box->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_SINGLE | wxDV_ROW_LINES);
	zone_list->AppendIconTextColumn("", wxDATAVIEW_CELL_INERT, 28, wxALIGN_CENTER, wxDATAVIEW_COL_RESIZABLE);
	zone_list->AppendTextColumn("ID", wxDATAVIEW_CELL_INERT, 52, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE);
	zone_list->AppendTextColumn("Name", wxDATAVIEW_CELL_INERT, 180, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE);
	list_box->Add(zone_list, 1, wxEXPAND | wxALL, FromDIP(5));
	main_sizer->Add(list_box, 1, wxEXPAND | wxALL, FromDIP(5));

	auto* action_sizer = newd wxBoxSizer(wxHORIZONTAL);
	add_button = newd wxButton(this, wxID_ADD, "Add", wxDefaultPosition, wxSize(60, -1));
	edit_button = newd wxButton(this, wxID_EDIT, "Edit", wxDefaultPosition, wxSize(60, -1));
	remove_button = newd wxButton(this, wxID_REMOVE, "Remove", wxDefaultPosition, wxSize(70, -1));

	action_sizer->Add(add_button, 1, wxEXPAND | wxRIGHT, FromDIP(4));
	action_sizer->Add(edit_button, 1, wxEXPAND | wxRIGHT, FromDIP(4));
	action_sizer->Add(remove_button, 1, wxEXPAND);
	main_sizer->Add(action_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(5));

	SetSizerAndFit(main_sizer);

	zone_list->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &ZonePalettePanel::OnSelectZone, this);
	zone_list->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &ZonePalettePanel::OnActivateZone, this);
	add_button->Bind(wxEVT_BUTTON, &ZonePalettePanel::OnAddZone, this);
	edit_button->Bind(wxEVT_BUTTON, &ZonePalettePanel::OnEditZone, this);
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

void ZonePalettePanel::OnSelectZone(wxDataViewEvent& event) {
	(void)event;
	if (IsMutatingUi() || !zone_list || !HasMap()) {
		return;
	}

	const auto zone_id = GetSelectedZoneId();
	if (!zone_id) {
		return;
	}

	const std::string zone_name = map->zones.findName(*zone_id);
	g_brush_manager.SetSelectedZone(zone_name);
	g_gui.SelectBrush();
	g_gui.RefreshView();
	UpdateButtonState();
}

void ZonePalettePanel::OnActivateZone(wxDataViewEvent& event) {
	(void)event;
	if (const auto zone_id = GetSelectedZoneId()) {
		JumpToZone(*zone_id);
	}
}

void ZonePalettePanel::OnAddZone(wxCommandEvent& event) {
	(void)event;
	if (IsMutatingUi() || !HasMap()) {
		return;
	}

	const auto zone_name = PromptForZoneName("Add Zone", DEFAULT_ZONE_NAME);
	if (!zone_name) {
		return;
	}

	if (map->zones.ensureZone(*zone_name) == 0) {
		wxMessageBox("Failed to create the selected zone.", "Add Zone", wxOK | wxICON_ERROR, this);
		return;
	}

	map->doChange();
	g_brush_manager.SetSelectedZone(*zone_name);
	g_gui.RefreshPalettes(map);
	g_gui.RefreshView();
	RefreshZoneList();
}

void ZonePalettePanel::OnEditZone(wxCommandEvent& event) {
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

	const auto new_name = PromptForZoneName("Edit Zone", old_name);
	if (!new_name) {
		return;
	}
	if (*new_name == old_name) {
		return;
	}

	if (const auto existing = map->zones.findId(*new_name); existing && *existing != *zone_id) {
		wxMessageBox("A zone with that name already exists.", "Rename Zone", wxOK | wxICON_WARNING, this);
		return;
	}

	map->zones.removeZone(*zone_id);
	if (!map->zones.addZone(*new_name, *zone_id)) {
		map->zones.addZone(old_name, *zone_id);
		wxMessageBox("Failed to rename the selected zone.", "Rename Zone", wxOK | wxICON_ERROR, this);
		return;
	}

	map->doChange();
	g_brush_manager.SetSelectedZone(*new_name);
	g_gui.RefreshPalettes(map);
	g_gui.RefreshView();
	RefreshZoneList();
	SyncSelection(*new_name);
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
	map->doChange();
	const std::string fallback_zone_name = map->zones.empty() ? std::string {} : sortedZones(map->zones).front().second;
	g_brush_manager.SetSelectedZone(fallback_zone_name);
	g_gui.RefreshPalettes(map);
	g_gui.RefreshView();
	RefreshZoneList();
}

std::string ZonePalettePanel::GetSelectedZoneName() const {
	const auto zone_id = GetSelectedZoneId();
	if (!zone_id || !HasMap()) {
		return {};
	}

	return map->zones.findName(*zone_id);
}

std::optional<uint16_t> ZonePalettePanel::GetSelectedZoneId() const {
	if (!zone_list || !HasMap()) {
		return std::nullopt;
	}

	const wxDataViewItem item = zone_list->GetSelection();
	if (!item.IsOk()) {
		return std::nullopt;
	}

	const auto zone_id = static_cast<uint16_t>(zone_list->GetItemData(item));
	return zone_id == 0 ? std::nullopt : std::optional<uint16_t> { zone_id };
}

std::string ZonePalettePanel::ResolvePreferredZoneName() const {
	if (!map) {
		return DEFAULT_ZONE_NAME;
	}

	const std::string brush_zone_name = std::string { g_brush_manager.GetSelectedZone() };
	if (!brush_zone_name.empty()) {
		return brush_zone_name;
	}

	if (const auto entries = sortedZones(map->zones); !entries.empty()) {
		return entries.front().second;
	}

	return {};
}

void ZonePalettePanel::JumpToZone(uint16_t zone_id) {
	if (!HasMap()) {
		return;
	}

	for (auto& tile_location : map->tiles()) {
		if (Tile* tile = tile_location.get(); tile && tile->hasZone(zone_id)) {
			g_gui.SetScreenCenterPosition(tile->getPosition());
			return;
		}
	}
}

std::optional<std::string> ZonePalettePanel::PromptForZoneName(const wxString& title, const std::string& initial_value) const {
	wxTextEntryDialog dialog(wxGetTopLevelParent(const_cast<ZonePalettePanel*>(this)), "Zone name:", title, wxstr(initial_value));
	if (dialog.ShowModal() != wxID_OK) {
		return std::nullopt;
	}

	const std::string zone_name = nstr(dialog.GetValue());
	if (zone_name.empty()) {
		return std::nullopt;
	}

	return zone_name;
}

void ZonePalettePanel::RefreshZoneList() {
	if (!zone_list) {
		return;
	}

	const std::string preferred_name = ResolvePreferredZoneName();
	SetMutatingUi(true);
	zone_list->Freeze();
	zone_list->DeleteAllItems();

	if (map) {
		for (const auto& [id, name] : sortedZones(map->zones)) {
			wxVector<wxVariant> row;
			wxIcon icon;
			icon.CopyFromBitmap(makeZoneColorBitmap(id));
			row.push_back(wxVariant(wxDataViewIconText("", icon)));
			row.push_back(wxVariant(wxstr(std::to_string(id))));
			row.push_back(wxVariant(wxstr(name)));
			zone_list->AppendItem(row, static_cast<wxUIntPtr>(id));
		}
	}

	SyncSelection(preferred_name);
	UpdateButtonState();

	zone_list->Thaw();
	SetMutatingUi(false);
}

void ZonePalettePanel::SyncSelection(const std::string& zone_name) {
	if (!zone_list || !map) {
		return;
	}

	const auto zone_id = map->zones.findId(zone_name);
	if (!zone_id) {
		zone_list->UnselectAll();
		return;
	}

	for (int row = 0; row < zone_list->GetItemCount(); ++row) {
		const wxDataViewItem item = zone_list->RowToItem(row);
		if (static_cast<uint16_t>(zone_list->GetItemData(item)) == *zone_id) {
			zone_list->Select(item);
			return;
		}
	}

	zone_list->UnselectAll();
}

void ZonePalettePanel::UpdateButtonState() {
	const bool has_map = HasMap();
	const bool has_selection = GetSelectedZoneId().has_value();
	add_button->Enable(has_map);
	edit_button->Enable(has_map && has_selection);
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
