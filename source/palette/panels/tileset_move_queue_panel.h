#ifndef RME_PALETTE_PANELS_TILESET_MOVE_QUEUE_PANEL_H_
#define RME_PALETTE_PANELS_TILESET_MOVE_QUEUE_PANEL_H_

#include "palette/controls/virtual_brush_grid.h"
#include "tileset_move_queue/tileset_move_queue.h"

#include <string>
#include <vector>

class wxBitmapButton;
class wxBitmapToggleButton;
class wxButton;
class wxChoice;
class wxTextCtrl;

class TilesetMoveQueuePanel : public wxPanel {
public:
	TilesetMoveQueuePanel(wxWindow* parent, wxWindowID id = wxID_ANY);
	~TilesetMoveQueuePanel() override;

	void ReloadFromQueue();

private:
	enum class SearchMode {
		Id,
		Name,
	};

	enum class ViewMode {
		List,
		Grid,
	};

	struct TilesetOption {
		std::string tileset_name;
	};

	wxChoice* palette_choice = nullptr;
	wxChoice* tileset_choice = nullptr;
	wxTextCtrl* search_text = nullptr;
	wxBitmapButton* search_mode_button = nullptr;
	wxBitmapToggleButton* list_toggle = nullptr;
	wxBitmapToggleButton* grid_toggle = nullptr;
	wxButton* apply_button = nullptr;
	wxButton* discard_button = nullptr;
	VirtualBrushGrid* results_grid = nullptr;

	SearchMode search_mode = SearchMode::Name;
	ViewMode view_mode = ViewMode::Grid;
	bool search_hotkeys_suspended = false;

	std::vector<TilesetOption> tileset_options;
	VirtualBrushGrid::EntryList visible_entries;

	void RebuildTilesetChoices();
	void ApplyFilter();
	PaletteType SelectedPalette() const;
	std::string SelectedTilesetName() const;
	void UpdateSearchHint();
	void UpdateViewMode();

	void OnPaletteChoice(wxCommandEvent& event);
	void OnTilesetChoice(wxCommandEvent& event);
	void OnSearchText(wxCommandEvent& event);
	void OnSearchCharHook(wxKeyEvent& event);
	void OnSearchFocus(wxFocusEvent& event);
	void OnSearchBlur(wxFocusEvent& event);
	void OnSearchModeButton(wxCommandEvent& event);
	void OnViewToggle(wxCommandEvent& event);
	void OnApply(wxCommandEvent& event);
	void OnDiscard(wxCommandEvent& event);
};

#endif
