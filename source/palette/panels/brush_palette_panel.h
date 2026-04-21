#ifndef RME_PALETTE_PANELS_BRUSH_PALETTE_PANEL_H_
#define RME_PALETTE_PANELS_BRUSH_PALETTE_PANEL_H_

#include "palette/palette_common.h"
#include "palette/panels/brush_panel.h"
#include "palette/controls/virtual_brush_grid.h"
#include "app/settings.h"
#include "tileset_move_queue/tileset_move_queue.h"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class wxBitmapButton;
class wxBitmapToggleButton;
class wxCheckBox;
class wxChoice;
class wxPanel;
class wxSimplebook;
class wxTextCtrl;
class wxTimer;

class BrushPalettePanel : public PalettePanel {
public:
	BrushPalettePanel(wxWindow* parent, const TilesetContainer& tilesets, TilesetCategoryType category, wxWindowID id = wxID_ANY);
	~BrushPalettePanel() override;

	// Interface
	// Flushes this panel and consequent views will feature reloaded data
	void InvalidateContents() override;
	// Loads the currently displayed page
	void LoadCurrentContents() override;
	// Loads all content in this panel
	void LoadAllContents() override;

	PaletteType GetType() const override;

	// Sets the display type (list or icons)
	void SetListType(BrushListType ltype);
	void SetListType(wxString ltype);

	// Select the first brush
	void SelectFirstBrush() override;
	// Returns the currently selected brush (first brush if panel is not loaded)
	Brush* GetSelectedBrush() const override;
	// Select the brush in the parameter, this only changes the look of the panel
	bool SelectBrush(const Brush* whatbrush) override;
	bool SelectBrushInTileset(const Brush* whatbrush, int tileset_index);
	bool SelectBrushWithOptions(const Brush* whatbrush, bool align_to_top);
	bool SelectBrushInTileset(const Brush* whatbrush, int tileset_index, bool align_to_top);

	// Called when this page is displayed
	void OnSwitchIn() override;

	void OnClickAddTileset(wxCommandEvent& WXUNUSED(event));
	void OnClickAddItemToTileset(wxCommandEvent& WXUNUSED(event));

protected:
	enum class SearchMode {
		Id,
		Name,
	};

	enum class ViewMode {
		List,
		Grid,
	};

	struct TilesetPage {
		std::string name;
		const TilesetCategory* category = nullptr;
	};

	struct SearchEntry {
		Brush* brush = nullptr;
		PaletteType palette_type = TILESET_UNKNOWN;
		int tileset_index = -1;
		std::optional<uint16_t> id;
		std::string name_lower;
	};

	PaletteType palette_type;

	wxChoice* tileset_choice = nullptr;

	wxPanel* toolbar_panel = nullptr;
	wxTextCtrl* search_text = nullptr;
	wxBitmapButton* search_mode_button = nullptr;
	wxBitmapToggleButton* list_toggle = nullptr;
	wxBitmapToggleButton* grid_toggle = nullptr;
	wxCheckBox* raw_unused_checkbox = nullptr; // RAW palette only

	VirtualBrushGrid* results_grid = nullptr;

	SearchMode search_mode = SearchMode::Name;
	ViewMode view_mode = ViewMode::List;
	bool view_mode_user_override = false;
	BrushListType style_list_type = BRUSHLIST_LISTBOX;
	bool search_hotkeys_suspended = false;
	wxString applied_search_query;

	std::vector<TilesetPage> tileset_pages;
	std::unordered_map<std::string, Brush*> remembered_brushes;
	int last_tileset_selection = 0;

	std::vector<SearchEntry> all_entries;
	std::vector<VirtualBrushGrid::Entry> tileset_entries;
	std::vector<VirtualBrushGrid::Entry> filtered_entries;
	Brush* last_search_selected_brush = nullptr;
	int last_search_selected_tileset_index = -1;
	PaletteType last_search_selected_palette = TILESET_UNKNOWN;
	Brush* pending_switch_selection = nullptr;
	bool pending_switch_align_to_top = false;

	bool raw_used_built = false;
	std::vector<uint8_t> raw_used_flags;

	void BuildTilesetPages(const TilesetContainer& tilesets);
	void RebuildAllEntries();
	void ApplySearchNow();
	void SyncSearchModeFromGlobal();
	void SyncViewModeFromGlobal();

	void UpdateSearchHint();
	void ApplyViewModeToViews();
	void EnsureRawUsedFlags();
	bool IsRawBrushUnused(const Brush* brush) const;
	bool SupportsMoveQueue() const;
	bool IsActivePalettePage() const;
	void SyncSelectedBrushToGui();
	std::vector<TilesetMoveQueue::Target> CollectMoveTargets(PaletteType palette) const;
	std::optional<TilesetMoveQueue::Target> ResolveSourceTarget(int tileset_index) const;
	void QueueSelectedItemsTo(const TilesetMoveQueue::Target& target);
	void RefreshQueueVisuals();

	void OnTilesetChoice(wxCommandEvent& event);
	void OnSearchText(wxCommandEvent& event);
	void OnSearchEnter(wxCommandEvent& event);
	void OnSearchCharHook(wxKeyEvent& event);
	void OnSearchFocus(wxFocusEvent& event);
	void OnSearchBlur(wxFocusEvent& event);
	void OnSearchModeButton(wxCommandEvent& event);
	void OnViewToggle(wxCommandEvent& event);
	void OnRawUnusedToggle(wxCommandEvent& event);
	void OnSearchGridSelected(wxCommandEvent& event);
	void OnResultsGridContextMenu(wxContextMenuEvent& event);
};

#endif
