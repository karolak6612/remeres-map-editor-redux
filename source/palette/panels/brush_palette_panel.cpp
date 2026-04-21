#include "palette/panels/brush_palette_panel.h"

#include "app/settings.h"
#include "game/materials.h"
#include "brushes/raw/raw_brush.h"
#include "item_definitions/core/item_definition_store.h"
#include "palette/controls/virtual_brush_grid.h"
#include "palette/palette_window.h"
#include "tileset_move_queue/tileset_xml_rewriter.h"
#include "ui/add_item_window.h"
#include "ui/add_tileset_window.h"
#include "ui/gui.h"
#include "util/image_manager.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <string_view>
#include <unordered_set>

#include <wx/bmpbuttn.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/tglbtn.h>
#include <wx/timer.h>

namespace {
	constexpr int TOOL_ICON_SIZE = 16;
	bool g_globalBrushSearchById = false;
	bool g_globalBrushViewModeOverride = false;
	bool g_globalBrushViewModeIsGrid = false;

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

	[[nodiscard]] std::vector<std::string_view> tokenizeWords(std::string_view text) {
		std::vector<std::string_view> tokens;

		size_t index = 0;
		while (index < text.size()) {
			while (index < text.size() && !std::isalnum(static_cast<unsigned char>(text[index]))) {
				++index;
			}

			const size_t start = index;
			while (index < text.size() && std::isalnum(static_cast<unsigned char>(text[index]))) {
				++index;
			}

			if (start < index) {
				tokens.push_back(text.substr(start, index - start));
			}
		}

		return tokens;
	}

	[[nodiscard]] bool matchesWholeWordSequence(const std::string& name_lower, const std::string& query_lower) {
		const auto query_tokens = tokenizeWords(query_lower);
		if (query_tokens.empty()) {
			return false;
		}

		const auto name_tokens = tokenizeWords(name_lower);
		if (name_tokens.size() < query_tokens.size()) {
			return false;
		}

		for (size_t start = 0; start + query_tokens.size() <= name_tokens.size(); ++start) {
			bool matched = true;
			for (size_t offset = 0; offset < query_tokens.size(); ++offset) {
				if (name_tokens[start + offset] != query_tokens[offset]) {
					matched = false;
					break;
				}
			}

			if (matched) {
				return true;
			}
		}

		return false;
	}

	[[nodiscard]] bool matchesExactWordSequence(const std::string& name_lower, const std::string& query_lower) {
		const auto query_tokens = tokenizeWords(query_lower);
		if (query_tokens.empty()) {
			return false;
		}

		const auto name_tokens = tokenizeWords(name_lower);
		if (name_tokens.size() != query_tokens.size()) {
			return false;
		}

		for (size_t index = 0; index < query_tokens.size(); ++index) {
			if (name_tokens[index] != query_tokens[index]) {
				return false;
			}
		}

		return true;
	}

	[[nodiscard]] bool isXmlBackedMoveTarget(
		const TilesetMoveQueue::Target& target,
		const std::unordered_set<std::string>& xml_tileset_names
	) {
		if (!target.isValid()) {
			return false;
		}

		if (TilesetXmlRewriter::IsVirtualRuntimeTilesetName(target.tileset_name)) {
			return false;
		}

		return xml_tileset_names.contains(target.tileset_name);
	}

	[[nodiscard]] bool supportsMoveQueue(PaletteType paletteType) {
		return paletteType == TILESET_RAW || paletteType == TILESET_ITEM;
	}

	[[nodiscard]] wxString paletteLabel(PaletteType paletteType) {
		return paletteType == TILESET_RAW ? "RAW" : "Items";
	}

	[[nodiscard]] constexpr auto globalSearchPalettes() {
		return std::array<PaletteType, 5> {
			TILESET_TERRAIN,
			TILESET_DOODAD,
			TILESET_COLLECTION,
			TILESET_ITEM,
			TILESET_RAW,
		};
	}
}

BrushPalettePanel::BrushPalettePanel(wxWindow* parent, const TilesetContainer& tilesets, TilesetCategoryType category, wxWindowID id) :
	PalettePanel(parent, id),
	palette_type(category) {
	search_mode = g_globalBrushSearchById ? SearchMode::Id : SearchMode::Name;

	BuildTilesetPages(tilesets);

	auto* topsizer = newd wxBoxSizer(wxVERTICAL);

	// Tileset selector
	auto* ts_sizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Tileset");
	tileset_choice = newd wxChoice(ts_sizer->GetStaticBox(), wxID_ANY);
	for (const auto& page : tileset_pages) {
		tileset_choice->Append(wxstr(page.name));
	}
	tileset_choice->SetSelection(tileset_pages.empty() ? wxNOT_FOUND : 0);
	tileset_choice->Enable(!tileset_pages.empty());
	ts_sizer->Add(tileset_choice, 0, wxEXPAND | wxALL, FromDIP(4));

	if (g_settings.getBoolean(Config::SHOW_TILESET_EDITOR)) {
		auto* row = newd wxBoxSizer(wxHORIZONTAL);

		auto* buttonAddTileset = newd wxButton(ts_sizer->GetStaticBox(), wxID_NEW, "Add new Tileset");
		buttonAddTileset->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_PLUS, wxSize(TOOL_ICON_SIZE, TOOL_ICON_SIZE)));
		buttonAddTileset->SetToolTip("Create a new custom tileset");
		row->Add(buttonAddTileset, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(6));

		auto* buttonAddItemToTileset = newd wxButton(ts_sizer->GetStaticBox(), wxID_ADD, "Add new Item");
		buttonAddItemToTileset->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_PLUS, wxSize(TOOL_ICON_SIZE, TOOL_ICON_SIZE)));
		buttonAddItemToTileset->SetToolTip("Add a new item to the current tileset");
		row->Add(buttonAddItemToTileset, 0, wxALIGN_CENTER_VERTICAL);

		ts_sizer->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(4));

		Bind(wxEVT_BUTTON, &BrushPalettePanel::OnClickAddItemToTileset, this, wxID_ADD);
		Bind(wxEVT_BUTTON, &BrushPalettePanel::OnClickAddTileset, this, wxID_NEW);
	}

	topsizer->Add(ts_sizer, 0, wxEXPAND);

	// Fixed toolbar row
	toolbar_panel = newd wxPanel(this, wxID_ANY);
	auto* toolbar_sizer = newd wxBoxSizer(wxVERTICAL);

	search_text = newd wxTextCtrl(toolbar_panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	search_text->SetMinSize(FromDIP(wxSize(150, -1)));
	search_mode_button = newd wxBitmapButton(
		toolbar_panel,
		wxID_ANY,
		IMAGE_MANAGER.GetBitmap(ICON_ANGLE_DOWN, wxSize(TOOL_ICON_SIZE, TOOL_ICON_SIZE)),
		wxDefaultPosition,
		wxDefaultSize,
		wxBU_EXACTFIT
	);
	search_mode_button->SetToolTip("Search mode");

	auto* search_sizer = newd wxBoxSizer(wxHORIZONTAL);
	search_sizer->Add(search_text, 1, wxEXPAND);
	search_sizer->Add(search_mode_button, 0, wxLEFT, FromDIP(4));
	toolbar_sizer->Add(search_sizer, 0, wxEXPAND);

	list_toggle = newd wxBitmapToggleButton(toolbar_panel, wxID_ANY, wxBitmap(), wxDefaultPosition, FromDIP(wxSize(28, 28)), wxBU_EXACTFIT | wxBORDER_NONE);
	grid_toggle = newd wxBitmapToggleButton(toolbar_panel, wxID_ANY, wxBitmap(), wxDefaultPosition, FromDIP(wxSize(28, 28)), wxBU_EXACTFIT | wxBORDER_NONE);
	ConfigureToggleIcon(list_toggle, IMAGE_MANAGER.GetBitmap(ICON_RECTANGLE_LIST, wxSize(TOOL_ICON_SIZE, TOOL_ICON_SIZE)));
	ConfigureToggleIcon(grid_toggle, IMAGE_MANAGER.GetBitmap(ICON_TABLE_CELLS, wxSize(TOOL_ICON_SIZE, TOOL_ICON_SIZE)));
	list_toggle->SetToolTip("List view");
	grid_toggle->SetToolTip("Grid view");

	auto* controls_sizer = newd wxBoxSizer(wxHORIZONTAL);
	controls_sizer->Add(list_toggle, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(2));
	controls_sizer->Add(grid_toggle, 0, wxALIGN_CENTER_VERTICAL);

	if (palette_type == TILESET_RAW) {
		raw_unused_checkbox = newd wxCheckBox(toolbar_panel, wxID_ANY, "Only unused");
		raw_unused_checkbox->SetToolTip("Hide RAW items already used by other brush systems.");
		controls_sizer->AddStretchSpacer(1);
		controls_sizer->Add(raw_unused_checkbox, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(8));
	}

	toolbar_sizer->Add(controls_sizer, 0, wxEXPAND | wxTOP, FromDIP(4));

	toolbar_panel->SetSizer(toolbar_sizer);
	topsizer->Add(toolbar_panel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(4));

	// Results (scrollable)
	results_grid = newd VirtualBrushGrid(this, &tileset_entries, palette_type, RENDER_SIZE_32x32);
	topsizer->Add(results_grid, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(4));

	SetSizerAndFit(topsizer);

	// Bind events
	tileset_choice->Bind(wxEVT_CHOICE, &BrushPalettePanel::OnTilesetChoice, this);
	search_text->Bind(wxEVT_TEXT, &BrushPalettePanel::OnSearchText, this);
	search_text->Bind(wxEVT_TEXT_ENTER, &BrushPalettePanel::OnSearchEnter, this);
	search_text->Bind(wxEVT_CHAR_HOOK, &BrushPalettePanel::OnSearchCharHook, this);
	search_text->Bind(wxEVT_SET_FOCUS, &BrushPalettePanel::OnSearchFocus, this);
	search_text->Bind(wxEVT_KILL_FOCUS, &BrushPalettePanel::OnSearchBlur, this);
	search_mode_button->Bind(wxEVT_BUTTON, &BrushPalettePanel::OnSearchModeButton, this);
	list_toggle->Bind(wxEVT_TOGGLEBUTTON, &BrushPalettePanel::OnViewToggle, this);
	grid_toggle->Bind(wxEVT_TOGGLEBUTTON, &BrushPalettePanel::OnViewToggle, this);
	if (raw_unused_checkbox) {
		raw_unused_checkbox->Bind(wxEVT_CHECKBOX, &BrushPalettePanel::OnRawUnusedToggle, this);
	}
	results_grid->Bind(EVT_VIRTUAL_BRUSH_GRID_SELECTED, &BrushPalettePanel::OnSearchGridSelected, this);
	results_grid->Bind(wxEVT_CONTEXT_MENU, &BrushPalettePanel::OnResultsGridContextMenu, this);

	UpdateSearchHint();

	// Apply initial state
	ApplyViewModeToViews();
	ApplySearchNow();
}

BrushPalettePanel::~BrushPalettePanel() {
	if (search_hotkeys_suspended) {
		g_hotkeys.EnableHotkeys();
		search_hotkeys_suspended = false;
	}
}

void BrushPalettePanel::BuildTilesetPages(const TilesetContainer& tilesets) {
	tileset_pages.clear();
	for (const auto& tileset : GetSortedTilesets(tilesets)) {
		const TilesetCategory* tcg = tileset->getCategory(palette_type);
		if (tcg && tcg->size() > 0) {
			tileset_pages.push_back(TilesetPage { .name = tileset->name, .category = tcg });
		}
	}
}

void BrushPalettePanel::InvalidateContents() {
	raw_used_built = false;
	raw_used_flags.clear();
	all_entries.clear();
	tileset_entries.clear();
	filtered_entries.clear();
	last_search_selected_brush = nullptr;
	last_search_selected_tileset_index = -1;
	last_search_selected_palette = TILESET_UNKNOWN;
	pending_switch_selection = nullptr;
	pending_switch_align_to_top = false;

	ApplySearchNow();
	PalettePanel::InvalidateContents();
}

void BrushPalettePanel::LoadCurrentContents() {
	SyncSearchModeFromGlobal();
	SyncViewModeFromGlobal();
	ApplySearchNow();
	PalettePanel::LoadCurrentContents();
}

void BrushPalettePanel::LoadAllContents() {
	SyncSearchModeFromGlobal();
	SyncViewModeFromGlobal();
	ApplySearchNow();
	PalettePanel::LoadAllContents();
}

PaletteType BrushPalettePanel::GetType() const {
	return palette_type;
}

void BrushPalettePanel::SetListType(BrushListType ltype) {
	style_list_type = ltype;

	if (!view_mode_user_override) {
		view_mode = (ltype == BRUSHLIST_LISTBOX || ltype == BRUSHLIST_TEXT_LISTBOX) ? ViewMode::List : ViewMode::Grid;
	}

	ApplyViewModeToViews();
	ApplySearchNow();
}

void BrushPalettePanel::SetListType(wxString ltype) {
	if (ltype == "small icons") {
		SetListType(BRUSHLIST_SMALL_ICONS);
	} else if (ltype == "large icons") {
		SetListType(BRUSHLIST_LARGE_ICONS);
	} else if (ltype == "listbox") {
		SetListType(BRUSHLIST_LISTBOX);
	} else if (ltype == "textlistbox") {
		SetListType(BRUSHLIST_TEXT_LISTBOX);
	}
}

Brush* BrushPalettePanel::GetSelectedBrush() const {
	for (const auto& toolBar : tool_bars) {
		if (Brush* res = toolBar->GetSelectedBrush()) {
			return res;
		}
	}

	return results_grid ? results_grid->GetSelectedBrush() : nullptr;
}

void BrushPalettePanel::SelectFirstBrush() {
	if (results_grid) {
		results_grid->SelectFirstBrush();
	}
}

bool BrushPalettePanel::SelectBrush(const Brush* whatbrush) {
	return SelectBrushWithOptions(whatbrush, false);
}

bool BrushPalettePanel::SelectBrushWithOptions(const Brush* whatbrush, bool align_to_top) {
	if (!results_grid) {
		return false;
	}

	pending_switch_selection = const_cast<Brush*>(whatbrush);
	pending_switch_align_to_top = align_to_top;

	// If a toolbar selects something, clear the main selection.
	for (PalettePanel* toolBar : tool_bars) {
		if (toolBar->SelectBrush(whatbrush)) {
			results_grid->SelectBrush(nullptr);
			return true;
		}
	}

	// If search is active, clear it so selection can jump tilesets.
	if (search_text && (!search_text->GetValue().IsEmpty() || !applied_search_query.IsEmpty())) {
		search_text->ChangeValue("");
		applied_search_query.Clear();
		ApplySearchNow();
	}

	if (results_grid->SelectBrush(whatbrush)) {
		if (tileset_choice) {
			const int sel = tileset_choice->GetSelection();
			if (sel != wxNOT_FOUND && sel >= 0 && static_cast<size_t>(sel) < tileset_pages.size()) {
				remembered_brushes[tileset_pages[static_cast<size_t>(sel)].name] = const_cast<Brush*>(whatbrush);
			}
		}
		for (PalettePanel* toolBar : tool_bars) {
			toolBar->SelectBrush(nullptr);
		}
		return true;
	}

	// Try other tilesets.
	for (size_t idx = 0; idx < tileset_pages.size(); ++idx) {
		const auto* category = tileset_pages[idx].category;
		if (!category) {
			continue;
		}
		for (const auto* b : category->brushlist) {
			if (b == whatbrush) {
				tileset_choice->SetSelection(static_cast<int>(idx));
				last_tileset_selection = static_cast<int>(idx);
				remembered_brushes[tileset_pages[idx].name] = const_cast<Brush*>(whatbrush);
				ApplySearchNow();
				return results_grid->SelectBrush(whatbrush);
			}
		}
	}

	return false;
}

bool BrushPalettePanel::SelectBrushInTileset(const Brush* whatbrush, int tileset_index) {
	return SelectBrushInTileset(whatbrush, tileset_index, false);
}

bool BrushPalettePanel::SelectBrushInTileset(const Brush* whatbrush, int tileset_index, bool align_to_top) {
	if (!whatbrush) {
		return false;
	}

	pending_switch_selection = const_cast<Brush*>(whatbrush);
	pending_switch_align_to_top = align_to_top;

	if (search_text && (!search_text->GetValue().IsEmpty() || !applied_search_query.IsEmpty())) {
		search_text->ChangeValue("");
		applied_search_query.Clear();
	}

	if (tileset_index >= 0 && static_cast<size_t>(tileset_index) < tileset_pages.size() && tileset_choice) {
		tileset_choice->SetSelection(tileset_index);
		last_tileset_selection = tileset_index;
		remembered_brushes[tileset_pages[static_cast<size_t>(tileset_index)].name] = const_cast<Brush*>(whatbrush);
	}

	ApplySearchNow();
	return results_grid ? results_grid->SelectBrush(whatbrush) : false;
}

void BrushPalettePanel::OnSwitchIn() {
	g_palettes.ActivatePalette(GetParentPalette());
	g_gui.RestoreBrushSizeState(last_brush_size_state);
	SyncSearchModeFromGlobal();
	SyncViewModeFromGlobal();
	LoadCurrentContents();
	if (pending_switch_selection && results_grid) {
		results_grid->SelectBrush(
			pending_switch_selection,
			pending_switch_align_to_top ? VirtualBrushGrid::SelectionScrollBehavior::AlignToTop : VirtualBrushGrid::SelectionScrollBehavior::EnsureVisible
		);
		pending_switch_selection = nullptr;
		pending_switch_align_to_top = false;
	}
	SyncSelectedBrushToGui();
}

void BrushPalettePanel::UpdateSearchHint() {
	if (!search_text) {
		return;
	}
	if (search_mode == SearchMode::Id) {
		search_text->SetHint("Search ID...");
	} else {
		search_text->SetHint("Search name...");
	}
}

void BrushPalettePanel::ApplyViewModeToViews() {
	// Icon size follows the style; list/grid follows runtime toggle.
	const RenderSize iconSize = (style_list_type == BRUSHLIST_SMALL_ICONS) ? RENDER_SIZE_16x16 : RENDER_SIZE_32x32;
	if (results_grid) {
		results_grid->SetIconSize(iconSize);
		results_grid->SetDisplayMode(view_mode == ViewMode::List ? VirtualBrushGrid::DisplayMode::List : VirtualBrushGrid::DisplayMode::Grid);
	}

	if (list_toggle && grid_toggle) {
		list_toggle->SetValue(view_mode == ViewMode::List);
		grid_toggle->SetValue(view_mode == ViewMode::Grid);
	}
}

void BrushPalettePanel::SyncSearchModeFromGlobal() {
	const SearchMode global_mode = g_globalBrushSearchById ? SearchMode::Id : SearchMode::Name;
	if (search_mode == global_mode) {
		return;
	}

	search_mode = global_mode;
	UpdateSearchHint();

	if (!applied_search_query.IsEmpty()) {
		ApplySearchNow();
	}
}

void BrushPalettePanel::SyncViewModeFromGlobal() {
	if (!g_globalBrushViewModeOverride) {
		return;
	}

	const ViewMode global_mode = g_globalBrushViewModeIsGrid ? ViewMode::Grid : ViewMode::List;
	if (view_mode == global_mode && view_mode_user_override) {
		return;
	}

	view_mode = global_mode;
	view_mode_user_override = true;
	ApplyViewModeToViews();
}

bool BrushPalettePanel::IsActivePalettePage() const {
	const auto* palette = GetParentPalette();
	return palette && palette->GetSelectedPage() == palette_type;
}

void BrushPalettePanel::SyncSelectedBrushToGui() {
	if (!IsActivePalettePage()) {
		return;
	}

	Brush* current = results_grid ? results_grid->GetSelectedBrush() : nullptr;
	if (current) {
		g_gui.SelectBrush(current, palette_type);
	} else {
		g_gui.SelectBrush();
	}
}

void BrushPalettePanel::EnsureRawUsedFlags() {
	if (raw_used_built) {
		return;
	}
	raw_used_built = true;
	raw_used_flags.assign(65536, 0);

	for (ServerItemId id : g_item_definitions.allIds()) {
		const auto definition = g_item_definitions.get(id);
		if (!definition) {
			continue;
		}

		const ItemEditorData& data = definition.editorData();
		if (data.brush != nullptr || data.doodad_brush != nullptr || data.collection_brush != nullptr) {
			raw_used_flags[id] = 1;
		}
	}
}

bool BrushPalettePanel::IsRawBrushUnused(const Brush* brush) const {
	if (palette_type != TILESET_RAW) {
		return true;
	}
	if (!raw_unused_checkbox || !raw_unused_checkbox->GetValue()) {
		return true;
	}
	if (!brush || !brush->is<RAWBrush>()) {
		return true;
	}
	const uint16_t id = brush->as<RAWBrush>()->getItemID();
	if (id >= raw_used_flags.size()) {
		return true;
	}
	return raw_used_flags[id] == 0;
}

bool BrushPalettePanel::SupportsMoveQueue() const {
	return supportsMoveQueue(palette_type);
}

std::vector<TilesetMoveQueue::Target> BrushPalettePanel::CollectMoveTargets(PaletteType palette) const {
	std::vector<TilesetMoveQueue::Target> targets;
	const auto xml_tileset_names = TilesetXmlRewriter::LoadXmlTilesetNames();
	for (Tileset* tileset : GetSortedTilesets(g_materials.tilesets)) {
		if (!tileset) {
			continue;
		}

		const TilesetCategory* category = tileset->getCategory(palette);
		if (!category || category->size() == 0) {
			continue;
		}

		TilesetMoveQueue::Target target {
			.palette = palette,
			.tileset_name = tileset->name,
		};
		if (!isXmlBackedMoveTarget(target, xml_tileset_names)) {
			continue;
		}

		targets.push_back(std::move(target));
	}
	return targets;
}

std::optional<TilesetMoveQueue::Target> BrushPalettePanel::ResolveSourceTarget(int tileset_index) const {
	if (tileset_index < 0 || static_cast<size_t>(tileset_index) >= tileset_pages.size()) {
		return std::nullopt;
	}

	return TilesetMoveQueue::Target {
		.palette = palette_type,
		.tileset_name = tileset_pages[static_cast<size_t>(tileset_index)].name,
	};
}

void BrushPalettePanel::QueueSelectedItemsTo(const TilesetMoveQueue::Target& target) {
	if (!results_grid || !target.isValid()) {
		return;
	}

	auto selected_entries = results_grid->GetSelectedEntries();
	if (selected_entries.empty()) {
		return;
	}

	auto& move_queue = g_gui.GetTilesetMoveQueue();
	for (const auto& entry : selected_entries) {
		if (!entry.brush || !entry.brush->is<RAWBrush>()) {
			continue;
		}

		int source_tileset_index = entry.tileset_index;
		if (source_tileset_index < 0 && tileset_choice) {
			source_tileset_index = tileset_choice->GetSelection();
		}

		const auto source = ResolveSourceTarget(source_tileset_index);
		if (!source.has_value()) {
			continue;
		}

		const ServerItemId item_id = entry.brush->as<RAWBrush>()->getItemID();
		if (*source == target) {
			move_queue.Remove(item_id);
			continue;
		}

		move_queue.QueueMove(item_id, *source, target);
	}

	RefreshQueueVisuals();
}

void BrushPalettePanel::RefreshQueueVisuals() {
	if (results_grid) {
		results_grid->Refresh();
	}

	for (PaletteWindow* palette : g_gui.GetPalettes()) {
		if (!palette) {
			continue;
		}
		palette->Refresh();
	}

	g_gui.RefreshTilesetMoveQueuePanel(true);
}

void BrushPalettePanel::RebuildAllEntries() {
	all_entries.clear();

	for (const PaletteType search_palette : globalSearchPalettes()) {
		int tileset_index = 0;
		for (Tileset* tileset : GetSortedTilesets(g_materials.tilesets)) {
			if (!tileset) {
				continue;
			}

			const auto* category = tileset->getCategory(search_palette);
			if (!category || category->size() == 0) {
				continue;
			}

			for (Brush* brush : category->brushlist) {
				if (!brush) {
					continue;
				}

				SearchEntry entry;
				entry.brush = brush;
				entry.palette_type = search_palette;
				entry.tileset_index = tileset_index;
				if (brush->is<RAWBrush>()) {
					entry.id = brush->as<RAWBrush>()->getItemID();
				}
				entry.name_lower = lowerCopy(brush->getName());
				all_entries.push_back(std::move(entry));
			}

			++tileset_index;
		}
	}
}

void BrushPalettePanel::ApplySearchNow() {
	if (!results_grid || !tileset_choice || !search_text) {
		return;
	}

	wxString query = applied_search_query;
	query.Trim(true).Trim(false);
	const bool searching = !query.IsEmpty();

	if (palette_type == TILESET_RAW && raw_unused_checkbox && raw_unused_checkbox->GetValue()) {
		EnsureRawUsedFlags();
	}

	if (!searching) {
		tileset_choice->Enable(!tileset_pages.empty());

		int sel = tileset_choice->GetSelection();
		const int restore_tileset_index = (last_search_selected_palette == palette_type) ? last_search_selected_tileset_index : -1;
		if (restore_tileset_index != -1) {
			sel = restore_tileset_index;
			tileset_choice->SetSelection(sel);
		}

		if (sel == wxNOT_FOUND && !tileset_pages.empty()) {
			sel = 0;
			tileset_choice->SetSelection(0);
		}

		last_tileset_selection = sel == wxNOT_FOUND ? 0 : sel;
		tileset_entries.clear();

		if (sel != wxNOT_FOUND && sel >= 0 && static_cast<size_t>(sel) < tileset_pages.size()) {
			const auto* category = tileset_pages[static_cast<size_t>(sel)].category;
			if (category) {
				tileset_entries.reserve(category->brushlist.size());
				for (Brush* brush : category->brushlist) {
					if (!brush) {
						continue;
					}
					if (!IsRawBrushUnused(brush)) {
						continue;
					}
					tileset_entries.push_back(VirtualBrushGrid::Entry {
						.brush = brush,
						.palette_type = palette_type,
						.tileset_index = sel,
					});
				}
			}
		}

		results_grid->SetEntryList(&tileset_entries, palette_type);

		// If user selected something while searching, keep that selection when returning.
		if (last_search_selected_brush && last_search_selected_palette == palette_type && restore_tileset_index == sel) {
			results_grid->SelectBrush(last_search_selected_brush);
			remembered_brushes[tileset_pages[static_cast<size_t>(sel)].name] = last_search_selected_brush;
		}

		if (restore_tileset_index != -1) {
			last_search_selected_tileset_index = -1;
		}

		// Restore remembered selection for this tileset if possible.
		if (sel != wxNOT_FOUND && sel >= 0 && static_cast<size_t>(sel) < tileset_pages.size()) {
			const std::string& name = tileset_pages[static_cast<size_t>(sel)].name;
			auto it = remembered_brushes.find(name);
			if (it != remembered_brushes.end()) {
				results_grid->SelectBrush(it->second);
			} else {
				results_grid->SelectFirstBrush();
			}
		}

		return;
	}

	tileset_choice->Enable(false);

	if (all_entries.empty()) {
		RebuildAllEntries();
	}

	filtered_entries.clear();

	if (search_mode == SearchMode::Id) {
		long id_val = -1;
		if (query.ToLong(&id_val) && id_val >= 0 && id_val <= 65535) {
			for (const auto& entry : all_entries) {
				if (!entry.id.has_value()) {
					continue;
				}
				if (static_cast<long>(*entry.id) != id_val) {
					continue;
				}
				if (!IsRawBrushUnused(entry.brush)) {
					continue;
				}
				filtered_entries.push_back(VirtualBrushGrid::Entry {
					.brush = entry.brush,
					.palette_type = entry.palette_type,
					.tileset_index = entry.tileset_index,
				});
			}
		}
	} else {
		const std::string q = lowerCopy(nstr(query));
		if (!q.empty()) {
			bool found_exact_name_match = false;
			for (const auto& entry : all_entries) {
				if (!matchesExactWordSequence(entry.name_lower, q)) {
					continue;
				}
				if (!IsRawBrushUnused(entry.brush)) {
					continue;
				}
				filtered_entries.push_back(VirtualBrushGrid::Entry {
					.brush = entry.brush,
					.palette_type = entry.palette_type,
					.tileset_index = entry.tileset_index,
				});
				found_exact_name_match = true;
			}

			if (!found_exact_name_match) {
			for (const auto& entry : all_entries) {
				if (!matchesWholeWordSequence(entry.name_lower, q)) {
					continue;
				}
				if (!IsRawBrushUnused(entry.brush)) {
					continue;
				}
				filtered_entries.push_back(VirtualBrushGrid::Entry {
					.brush = entry.brush,
					.palette_type = entry.palette_type,
					.tileset_index = entry.tileset_index,
				});
			}
			}
		}
	}

	results_grid->SetEntryList(&filtered_entries, TILESET_UNKNOWN);
	if (last_search_selected_brush) {
		results_grid->SelectBrush(last_search_selected_brush);
	} else {
		results_grid->SelectFirstBrush();
	}
}

void BrushPalettePanel::OnTilesetChoice(wxCommandEvent&) {
	// Remember selection for previous tileset.
	const int old_sel = last_tileset_selection;
	if (old_sel != wxNOT_FOUND && old_sel >= 0 && static_cast<size_t>(old_sel) < tileset_pages.size()) {
		if (Brush* b = results_grid->GetSelectedBrush()) {
			remembered_brushes[tileset_pages[static_cast<size_t>(old_sel)].name] = b;
		}
	}

	last_tileset_selection = tileset_choice->GetSelection();
	ApplySearchNow();
	SyncSelectedBrushToGui();
}

void BrushPalettePanel::OnSearchText(wxCommandEvent&) {
	// Search is committed explicitly with Enter so typing doesn't reshuffle results mid-input.
}

void BrushPalettePanel::OnSearchEnter(wxCommandEvent&) {
	if (!search_text) {
		return;
	}

	wxString next_query = search_text->GetValue();
	next_query.Trim(true).Trim(false);
	if (next_query != applied_search_query) {
		last_search_selected_brush = nullptr;
		last_search_selected_tileset_index = -1;
		last_search_selected_palette = TILESET_UNKNOWN;
	}

	applied_search_query = next_query;
	ApplySearchNow();
	SyncSelectedBrushToGui();
}

void BrushPalettePanel::OnSearchCharHook(wxKeyEvent& event) {
	// Keep palette/menu hotkeys from firing while the user is typing in the filter.
	event.StopPropagation();
	event.Skip();
}

void BrushPalettePanel::OnSearchFocus(wxFocusEvent& event) {
	if (!search_hotkeys_suspended) {
		g_hotkeys.DisableHotkeys();
		search_hotkeys_suspended = true;
	}
	event.Skip();
}

void BrushPalettePanel::OnSearchBlur(wxFocusEvent& event) {
	if (search_hotkeys_suspended) {
		g_hotkeys.EnableHotkeys();
		search_hotkeys_suspended = false;
	}
	event.Skip();
}

void BrushPalettePanel::OnSearchModeButton(wxCommandEvent&) {
	wxMenu menu;
	const int id_name = wxNewId();
	const int id_id = wxNewId();

	menu.AppendRadioItem(id_name, "Name");
	menu.AppendRadioItem(id_id, "ID");
	menu.Check(search_mode == SearchMode::Name ? id_name : id_id, true);

	menu.Bind(wxEVT_MENU, [this, id_name, id_id](wxCommandEvent& evt) {
		search_mode = (evt.GetId() == id_id) ? SearchMode::Id : SearchMode::Name;
		g_globalBrushSearchById = (search_mode == SearchMode::Id);
		UpdateSearchHint();
		ApplySearchNow();
		SyncSelectedBrushToGui();
	}, id_name);
	menu.Bind(wxEVT_MENU, [this, id_name, id_id](wxCommandEvent& evt) {
		search_mode = (evt.GetId() == id_id) ? SearchMode::Id : SearchMode::Name;
		g_globalBrushSearchById = (search_mode == SearchMode::Id);
		UpdateSearchHint();
		ApplySearchNow();
		SyncSelectedBrushToGui();
	}, id_id);

	search_mode_button->PopupMenu(&menu, 0, search_mode_button->GetSize().y);
}

void BrushPalettePanel::OnViewToggle(wxCommandEvent& event) {
	Brush* selected_brush = results_grid ? results_grid->GetSelectedBrush() : nullptr;

	if (event.GetEventObject() == list_toggle) {
		view_mode = ViewMode::List;
	} else if (event.GetEventObject() == grid_toggle) {
		view_mode = ViewMode::Grid;
	} else {
		return;
	}

	view_mode_user_override = true;
	g_globalBrushViewModeOverride = true;
	g_globalBrushViewModeIsGrid = (view_mode == ViewMode::Grid);
	ApplyViewModeToViews();
	if (selected_brush && results_grid) {
		results_grid->SelectBrush(selected_brush, VirtualBrushGrid::SelectionScrollBehavior::EnsureVisible);
	}
}

void BrushPalettePanel::OnRawUnusedToggle(wxCommandEvent&) {
	// RAW-only toggle affects both tileset browsing and search results.
	raw_used_built = false;
	ApplySearchNow();
	SyncSelectedBrushToGui();
}

void BrushPalettePanel::OnSearchGridSelected(wxCommandEvent& event) {
	if (!results_grid || !tileset_choice) {
		return;
	}

	const bool searching = !applied_search_query.IsEmpty();

	if (searching) {
		const auto selected_entry = results_grid->GetSelectedEntry();
		if (!selected_entry.has_value() || !selected_entry->brush) {
			return;
		}

		last_search_selected_brush = selected_entry->brush;
		last_search_selected_tileset_index = selected_entry->tileset_index;
		last_search_selected_palette = selected_entry->palette_type;

		if (auto* palette = GetParentPalette()) {
			palette->OnSelectBrush(selected_entry->brush, selected_entry->palette_type, selected_entry->tileset_index, true);
		}
	} else {
		const int sel = tileset_choice->GetSelection();
		if (sel != wxNOT_FOUND && sel >= 0 && static_cast<size_t>(sel) < tileset_pages.size()) {
			if (Brush* b = results_grid->GetSelectedBrush()) {
				remembered_brushes[tileset_pages[static_cast<size_t>(sel)].name] = b;
			}
		}
	}
}

void BrushPalettePanel::OnResultsGridContextMenu(wxContextMenuEvent& event) {
	if (!SupportsMoveQueue() || !results_grid || !results_grid->HasSelection()) {
		return;
	}

	wxMenu menu;
	auto* move_menu = newd wxMenu();

	const auto appendTargetItem = [this](wxMenu* target_menu, const TilesetMoveQueue::Target& target, const wxString& label) {
		const int menu_id = wxWindow::NewControlId();
		target_menu->Append(menu_id, label);
		target_menu->Bind(wxEVT_MENU, [this, target](wxCommandEvent&) {
			QueueSelectedItemsTo(target);
		}, menu_id);
	};

	const auto xml_tileset_names = TilesetXmlRewriter::LoadXmlTilesetNames();
	const auto& recent_targets = g_gui.GetTilesetMoveQueue().RecentTargets();
	size_t recent_target_count = 0;
	for (const auto& target : recent_targets) {
		if (!isXmlBackedMoveTarget(target, xml_tileset_names)) {
			continue;
		}
		appendTargetItem(move_menu, target, wxString::Format("(Last used) %s -> %s", paletteLabel(target.palette), wxstr(target.tileset_name)));
		++recent_target_count;
	}

	if (recent_target_count > 0) {
		move_menu->AppendSeparator();
	}

	auto* raw_targets_menu = newd wxMenu();
	for (const auto& target : CollectMoveTargets(TILESET_RAW)) {
		appendTargetItem(raw_targets_menu, target, wxstr(target.tileset_name));
	}
	if (raw_targets_menu->GetMenuItemCount() > 0) {
		move_menu->AppendSubMenu(raw_targets_menu, "RAW");
	} else {
		delete raw_targets_menu;
	}

	auto* item_targets_menu = newd wxMenu();
	for (const auto& target : CollectMoveTargets(TILESET_ITEM)) {
		appendTargetItem(item_targets_menu, target, wxstr(target.tileset_name));
	}
	if (item_targets_menu->GetMenuItemCount() > 0) {
		move_menu->AppendSubMenu(item_targets_menu, "Items");
	} else {
		delete item_targets_menu;
	}

	if (move_menu->GetMenuItemCount() == 0) {
		delete move_menu;
		return;
	}

	menu.AppendSubMenu(move_menu, "Move to");

	wxPoint menu_position = event.GetPosition();
	if (menu_position == wxDefaultPosition) {
		menu_position = wxPoint(8, 8);
	} else {
		menu_position = results_grid->ScreenToClient(menu_position);
	}

	results_grid->PopupMenu(&menu, menu_position);
}

void BrushPalettePanel::OnClickAddTileset(wxCommandEvent& WXUNUSED(event)) {
	wxDialog* w = newd AddTilesetWindow(g_gui.root, palette_type);
	const int ret = w->ShowModal();
	w->Destroy();

	if (ret != 0) {
		g_gui.DestroyPalettes();
		g_gui.NewPalette();
	}
}

void BrushPalettePanel::OnClickAddItemToTileset(wxCommandEvent& WXUNUSED(event)) {
	const int selection = tileset_choice ? tileset_choice->GetSelection() : wxNOT_FOUND;
	if (selection == wxNOT_FOUND || selection < 0 || static_cast<size_t>(selection) >= tileset_pages.size()) {
		return;
	}

	const std::string tilesetName = tileset_pages[static_cast<size_t>(selection)].name;
	auto it = g_materials.tilesets.find(tilesetName);
	if (it == g_materials.tilesets.end()) {
		return;
	}

	wxDialog* w = newd AddItemWindow(g_gui.root, palette_type, it->second);
	const int ret = w->ShowModal();
	w->Destroy();

	if (ret != 0) {
		g_gui.RebuildPalettes();
	}
}
