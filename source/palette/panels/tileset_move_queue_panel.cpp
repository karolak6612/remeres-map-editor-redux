#include "palette/panels/tileset_move_queue_panel.h"

#include "brushes/raw/raw_brush.h"
#include "game/materials.h"
#include "item_definitions/core/item_definition_store.h"
#include "palette/palette_window.h"
#include "ui/gui.h"
#include "util/image_manager.h"

#include <algorithm>
#include <cctype>
#include <unordered_set>

#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/tglbtn.h>
#include <wx/msgdlg.h>

namespace {
	constexpr int TOOL_ICON_SIZE = 16;

	void ConfigureToggleIcon(wxBitmapToggleButton* button, const wxBitmap& bitmap) {
		if (!button) {
			return;
		}
		button->SetBitmap(bitmap);
		button->SetBitmapCurrent(bitmap);
		button->SetBitmapFocus(bitmap);
		button->SetBitmapPressed(bitmap);
	}

	[[nodiscard]] std::string lowerCopy(std::string value) {
		for (char& ch : value) {
			ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
		}
		return value;
	}

	[[nodiscard]] wxString paletteLabel(PaletteType palette) {
		return palette == TILESET_RAW ? "RAW" : "Items";
	}
}

TilesetMoveQueuePanel::TilesetMoveQueuePanel(wxWindow* parent, wxWindowID id) :
	wxPanel(parent, id, wxDefaultPosition, wxSize(225, 250)) {
	SetMinSize(wxSize(225, 250));

	auto* top_sizer = newd wxBoxSizer(wxVERTICAL);

	auto* palette_sizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Palette");
	palette_choice = newd wxChoice(palette_sizer->GetStaticBox(), wxID_ANY);
	palette_choice->Append("RAW");
	palette_choice->Append("Items");
	palette_choice->SetSelection(0);
	palette_sizer->Add(palette_choice, 0, wxEXPAND | wxALL, FromDIP(4));
	top_sizer->Add(palette_sizer, 0, wxEXPAND);

	auto* tileset_sizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Tileset");
	tileset_choice = newd wxChoice(tileset_sizer->GetStaticBox(), wxID_ANY);
	tileset_sizer->Add(tileset_choice, 0, wxEXPAND | wxALL, FromDIP(4));
	top_sizer->Add(tileset_sizer, 0, wxEXPAND);

	auto* toolbar_panel = newd wxPanel(this, wxID_ANY);
	auto* toolbar_sizer = newd wxBoxSizer(wxVERTICAL);

	search_text = newd wxTextCtrl(toolbar_panel, wxID_ANY);
	search_mode_button = newd wxBitmapButton(
		toolbar_panel,
		wxID_ANY,
		IMAGE_MANAGER.GetBitmap(ICON_ANGLE_DOWN, wxSize(TOOL_ICON_SIZE, TOOL_ICON_SIZE)),
		wxDefaultPosition,
		wxDefaultSize,
		wxBU_EXACTFIT
	);

	auto* search_sizer = newd wxBoxSizer(wxHORIZONTAL);
	search_sizer->Add(search_text, 1, wxEXPAND);
	search_sizer->Add(search_mode_button, 0, wxLEFT, FromDIP(4));
	toolbar_sizer->Add(search_sizer, 0, wxEXPAND);

	list_toggle = newd wxBitmapToggleButton(toolbar_panel, wxID_ANY, wxBitmap(), wxDefaultPosition, FromDIP(wxSize(28, 28)), wxBU_EXACTFIT | wxBORDER_NONE);
	grid_toggle = newd wxBitmapToggleButton(toolbar_panel, wxID_ANY, wxBitmap(), wxDefaultPosition, FromDIP(wxSize(28, 28)), wxBU_EXACTFIT | wxBORDER_NONE);
	ConfigureToggleIcon(list_toggle, IMAGE_MANAGER.GetBitmap(ICON_RECTANGLE_LIST, wxSize(TOOL_ICON_SIZE, TOOL_ICON_SIZE)));
	ConfigureToggleIcon(grid_toggle, IMAGE_MANAGER.GetBitmap(ICON_TABLE_CELLS, wxSize(TOOL_ICON_SIZE, TOOL_ICON_SIZE)));

	auto* view_sizer = newd wxBoxSizer(wxHORIZONTAL);
	view_sizer->Add(list_toggle, 0, wxRIGHT, FromDIP(2));
	view_sizer->Add(grid_toggle, 0);
	toolbar_sizer->Add(view_sizer, 0, wxTOP, FromDIP(4));

	toolbar_panel->SetSizer(toolbar_sizer);
	top_sizer->Add(toolbar_panel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(4));

	results_grid = newd VirtualBrushGrid(this, &visible_entries, TILESET_RAW, RENDER_SIZE_32x32);
	top_sizer->Add(results_grid, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(4));

	auto* action_sizer = newd wxBoxSizer(wxHORIZONTAL);
	discard_button = newd wxButton(this, wxID_ANY, "Discard");
	apply_button = newd wxButton(this, wxID_ANY, "Apply");
	action_sizer->Add(discard_button, 1, wxRIGHT, FromDIP(4));
	action_sizer->Add(apply_button, 1);
	top_sizer->Add(action_sizer, 0, wxEXPAND | wxALL, FromDIP(4));

	SetSizer(top_sizer);

	palette_choice->Bind(wxEVT_CHOICE, &TilesetMoveQueuePanel::OnPaletteChoice, this);
	tileset_choice->Bind(wxEVT_CHOICE, &TilesetMoveQueuePanel::OnTilesetChoice, this);
	search_text->Bind(wxEVT_TEXT, &TilesetMoveQueuePanel::OnSearchText, this);
	search_text->Bind(wxEVT_CHAR_HOOK, &TilesetMoveQueuePanel::OnSearchCharHook, this);
	search_text->Bind(wxEVT_SET_FOCUS, &TilesetMoveQueuePanel::OnSearchFocus, this);
	search_text->Bind(wxEVT_KILL_FOCUS, &TilesetMoveQueuePanel::OnSearchBlur, this);
	search_mode_button->Bind(wxEVT_BUTTON, &TilesetMoveQueuePanel::OnSearchModeButton, this);
	list_toggle->Bind(wxEVT_TOGGLEBUTTON, &TilesetMoveQueuePanel::OnViewToggle, this);
	grid_toggle->Bind(wxEVT_TOGGLEBUTTON, &TilesetMoveQueuePanel::OnViewToggle, this);
	apply_button->Bind(wxEVT_BUTTON, &TilesetMoveQueuePanel::OnApply, this);
	discard_button->Bind(wxEVT_BUTTON, &TilesetMoveQueuePanel::OnDiscard, this);

	UpdateSearchHint();
	UpdateViewMode();
	ReloadFromQueue();
}

TilesetMoveQueuePanel::~TilesetMoveQueuePanel() {
	if (search_hotkeys_suspended) {
		g_hotkeys.EnableHotkeys();
		search_hotkeys_suspended = false;
	}
}

void TilesetMoveQueuePanel::ReloadFromQueue() {
	RebuildTilesetChoices();
	ApplyFilter();
}

void TilesetMoveQueuePanel::RebuildTilesetChoices() {
	const PaletteType palette = SelectedPalette();
	const auto queue_entries = g_gui.GetTilesetMoveQueue().Entries();

	std::unordered_set<std::string> seen_tilesets;
	std::vector<std::string> names;
	for (const auto& entry : queue_entries) {
		if (entry.target.palette != palette) {
			continue;
		}

		if (seen_tilesets.insert(entry.target.tileset_name).second) {
			names.push_back(entry.target.tileset_name);
		}
	}

	std::ranges::sort(names);
	const std::string current = SelectedTilesetName();

	tileset_options.clear();
	tileset_choice->Clear();
	for (const auto& name : names) {
		tileset_options.push_back(TilesetOption { .tileset_name = name });
		tileset_choice->Append(wxstr(name));
	}

	if (tileset_options.empty()) {
		tileset_choice->SetSelection(wxNOT_FOUND);
		tileset_choice->Enable(false);
		return;
	}

	tileset_choice->Enable(true);
	int selection = 0;
	for (size_t index = 0; index < tileset_options.size(); ++index) {
		if (tileset_options[index].tileset_name == current) {
			selection = static_cast<int>(index);
			break;
		}
	}
	tileset_choice->SetSelection(selection);
}

void TilesetMoveQueuePanel::ApplyFilter() {
	const PaletteType palette = SelectedPalette();
	const std::string tileset_name = SelectedTilesetName();
	const std::string query = lowerCopy(nstr(search_text ? search_text->GetValue() : wxString()));

	visible_entries.clear();
	if (tileset_name.empty()) {
		results_grid->SetEntryList(&visible_entries, palette);
		apply_button->Enable(false);
		discard_button->Enable(!g_gui.GetTilesetMoveQueue().Empty());
		return;
	}

	auto queued_entries = g_gui.GetTilesetMoveQueue().EntriesForTarget(palette, tileset_name);
	for (const auto& entry : queued_entries) {
		const auto definition = g_item_definitions.get(entry.item_id);
		RAWBrush* brush = definition ? definition.editorData().raw_brush : nullptr;
		if (!brush) {
			continue;
		}

		bool matches = true;
		if (!query.empty()) {
			if (search_mode == SearchMode::Id) {
				matches = std::to_string(entry.item_id) == query;
			} else {
				matches = lowerCopy(brush->getName()).find(query) != std::string::npos;
			}
		}

		if (matches) {
			visible_entries.push_back(VirtualBrushGrid::Entry { .brush = brush, .tileset_index = -1 });
		}
	}

	results_grid->SetEntryList(&visible_entries, palette);
	apply_button->Enable(!g_gui.GetTilesetMoveQueue().Empty());
	discard_button->Enable(!g_gui.GetTilesetMoveQueue().Empty());
}

PaletteType TilesetMoveQueuePanel::SelectedPalette() const {
	return (palette_choice && palette_choice->GetSelection() == 1) ? TILESET_ITEM : TILESET_RAW;
}

std::string TilesetMoveQueuePanel::SelectedTilesetName() const {
	const int selection = tileset_choice ? tileset_choice->GetSelection() : wxNOT_FOUND;
	if (selection == wxNOT_FOUND || selection < 0 || static_cast<size_t>(selection) >= tileset_options.size()) {
		return {};
	}
	return tileset_options[static_cast<size_t>(selection)].tileset_name;
}

void TilesetMoveQueuePanel::UpdateSearchHint() {
	if (!search_text) {
		return;
	}

	search_text->SetHint(search_mode == SearchMode::Id ? "Search ID..." : "Search name...");
}

void TilesetMoveQueuePanel::UpdateViewMode() {
	results_grid->SetDisplayMode(view_mode == ViewMode::List ? VirtualBrushGrid::DisplayMode::List : VirtualBrushGrid::DisplayMode::Grid);
	list_toggle->SetValue(view_mode == ViewMode::List);
	grid_toggle->SetValue(view_mode == ViewMode::Grid);
}

void TilesetMoveQueuePanel::OnPaletteChoice(wxCommandEvent&) {
	RebuildTilesetChoices();
	ApplyFilter();
}

void TilesetMoveQueuePanel::OnTilesetChoice(wxCommandEvent&) {
	ApplyFilter();
}

void TilesetMoveQueuePanel::OnSearchText(wxCommandEvent&) {
	ApplyFilter();
}

void TilesetMoveQueuePanel::OnSearchCharHook(wxKeyEvent& event) {
	event.StopPropagation();
	event.Skip();
}

void TilesetMoveQueuePanel::OnSearchFocus(wxFocusEvent& event) {
	if (!search_hotkeys_suspended) {
		g_hotkeys.DisableHotkeys();
		search_hotkeys_suspended = true;
	}
	event.Skip();
}

void TilesetMoveQueuePanel::OnSearchBlur(wxFocusEvent& event) {
	if (search_hotkeys_suspended) {
		g_hotkeys.EnableHotkeys();
		search_hotkeys_suspended = false;
	}
	event.Skip();
}

void TilesetMoveQueuePanel::OnSearchModeButton(wxCommandEvent&) {
	wxMenu menu;
	const int id_name = wxWindow::NewControlId();
	const int id_id = wxWindow::NewControlId();

	menu.AppendRadioItem(id_name, "Name");
	menu.AppendRadioItem(id_id, "ID");
	menu.Check(search_mode == SearchMode::Name ? id_name : id_id, true);

	menu.Bind(wxEVT_MENU, [this, id_id](wxCommandEvent& event) {
		search_mode = event.GetId() == id_id ? SearchMode::Id : SearchMode::Name;
		UpdateSearchHint();
		ApplyFilter();
	}, id_name);
	menu.Bind(wxEVT_MENU, [this, id_id](wxCommandEvent& event) {
		search_mode = event.GetId() == id_id ? SearchMode::Id : SearchMode::Name;
		UpdateSearchHint();
		ApplyFilter();
	}, id_id);

	search_mode_button->PopupMenu(&menu, 0, search_mode_button->GetSize().y);
}

void TilesetMoveQueuePanel::OnViewToggle(wxCommandEvent& event) {
	if (event.GetEventObject() == list_toggle) {
		view_mode = ViewMode::List;
	} else if (event.GetEventObject() == grid_toggle) {
		view_mode = ViewMode::Grid;
	}
	UpdateViewMode();
}

void TilesetMoveQueuePanel::OnApply(wxCommandEvent&) {
	const auto result = g_gui.GetTilesetMoveQueue().Apply(g_materials);
	if (!result.success) {
		wxMessageBox(wxstr(result.error), "Move Queue", wxOK | wxICON_ERROR, this);
		return;
	}

	for (PaletteWindow* palette : g_gui.GetPalettes()) {
		if (!palette) {
			continue;
		}
		palette->InvalidatePage(TILESET_RAW);
		palette->InvalidatePage(TILESET_ITEM);
	}

	g_gui.RefreshTilesetMoveQueuePanel(false);

	wxString message = wxString::Format("Applied: %d\nSkipped: %d", result.applied, result.skipped);
	if (!result.warnings.empty()) {
		message << wxString::Format("\nWarnings: %zu", result.warnings.size());
		const size_t preview_count = std::min<size_t>(3, result.warnings.size());
		for (size_t index = 0; index < preview_count; ++index) {
			message << "\n- " << wxstr(result.warnings[index]);
		}
		if (result.warnings.size() > preview_count) {
			message << wxString::Format("\n- ...and %zu more", result.warnings.size() - preview_count);
		}
	}
	wxMessageBox(message, "Move Queue", wxOK | wxICON_INFORMATION, this);
}

void TilesetMoveQueuePanel::OnDiscard(wxCommandEvent&) {
	g_gui.GetTilesetMoveQueue().Clear();
	g_gui.RefreshTilesetMoveQueuePanel(false);

	for (PaletteWindow* palette : g_gui.GetPalettes()) {
		if (!palette) {
			continue;
		}
		palette->Refresh();
	}
}
