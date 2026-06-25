#ifndef RME_PALETTE_CONTROLS_VIRTUAL_BRUSH_GRID_H_
#define RME_PALETTE_CONTROLS_VIRTUAL_BRUSH_GRID_H_

#include "app/main.h"
#include "util/nanovg_canvas.h"
#include "palette/panels/brush_panel.h"
#include "ui/dcbutton.h" // For RenderSize
#include "item_definitions/core/item_definition_types.h"

#include <unordered_set>
#include <vector>

wxDECLARE_EVENT(EVT_VIRTUAL_BRUSH_GRID_SELECTED, wxCommandEvent);

/**
 * @class VirtualBrushGrid
 * @brief High-performance brush grid using NanoVG for GPU-accelerated rendering.
 *
 * This control displays a grid of brush icons with virtual scrolling,
 * supporting thousands of brushes at 60fps. Uses texture caching for
 * efficient sprite rendering.
 */
class VirtualBrushGrid : public NanoVGCanvas, public BrushBoxInterface {
public:
	struct Entry {
		Brush* brush = nullptr;
		PaletteType palette_type = TILESET_UNKNOWN;
		int tileset_index = -1; // Used by global-search to restore tileset selection.
	};
	using EntryList = std::vector<Entry>;

	/**
	 * @brief Constructs a VirtualBrushGrid.
	 * @param parent Parent window
	 * @param _tileset The tileset category containing brushes
	 * @param rsz Icon render size (16x16 or 32x32)
	 */
	VirtualBrushGrid(wxWindow* parent, const TilesetCategory* _tileset, RenderSize rsz);
	VirtualBrushGrid(wxWindow* parent, const EntryList* entries, TilesetCategoryType palette_type, RenderSize rsz);
	~VirtualBrushGrid() override;

	wxWindow* GetSelfWindow() override {
		return this;
	}

	// BrushBoxInterface
	void SelectFirstBrush() override;
	Brush* GetSelectedBrush() const override;
	enum class SelectionScrollBehavior {
		EnsureVisible,
		AlignToTop,
	};
	bool SelectBrush(const Brush* brush) override;
	bool SelectBrush(const Brush* brush, SelectionScrollBehavior scroll_behavior);
	std::optional<Entry> GetSelectedEntry() const;
	std::vector<Entry> GetSelectedEntries() const;
	std::vector<Brush*> GetSelectedBrushes() const;
	std::vector<ServerItemId> GetSelectedItemIds() const;
	bool HasSelection() const;
	size_t GetSelectionCount() const;

	enum class DisplayMode {
		Grid,
		List
	};

	static constexpr int LIST_ROW_HEIGHT = 36;
	static constexpr int GRID_PADDING = 4;
	static constexpr int GRID_ITEM_SIZE_BASE = 32;
	static constexpr int ICON_OFFSET = 2;

	void SetDisplayMode(DisplayMode mode);
	void SetEntryList(const EntryList* entries, TilesetCategoryType palette_type);
	void SetIconSize(RenderSize size);

protected:
	/**
	 * @brief Performs NanoVG rendering of the brush grid.
	 * @param vg NanoVG context
	 * @param width Canvas width
	 * @param height Canvas height
	 */
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;

	wxSize DoGetBestClientSize() const override;

	// Event Handlers
	void OnMouseDown(wxMouseEvent& event);
	void OnMouseRightDown(wxMouseEvent& event);
	void OnMotion(wxMouseEvent& event);
	void OnSize(wxSizeEvent& event);

	// Internal helpers
	void UpdateLayout();
	int HitTest(int x, int y) const;
	wxRect GetItemRect(int index) const;
	void DrawBrushItem(NVGcontext* vg, int index, const wxRect& rect);

	int ItemCount() const;
	Brush* ItemBrush(int index) const;
	TilesetCategoryType ItemPaletteType(int index) const;
	int ItemTilesetIndex(int index) const;
	bool ShowRawIdLabel(int index) const;
	int GridCellHeight() const;
	bool IsIndexSelected(int index) const;
	bool IsQueuedBrush(const Brush* brush) const;
	void ClearSelection();
	void SelectSingleIndex(int index);
	void ToggleIndexSelection(int index);
	void SelectRangeTo(int index);
	void NotifySelectionChanged();

	DisplayMode display_mode = DisplayMode::Grid;
	RenderSize icon_size;
	int selected_index;
	int selection_anchor = -1;
	int hover_index;
	int columns;
	int item_size;
	int padding;
	std::unordered_set<int> selected_indices;

	const TilesetCategory* tileset = nullptr;
	const EntryList* entries = nullptr;
	TilesetCategoryType palette_type = TILESET_UNKNOWN;

	// Optimization: UTF8 name cache
	mutable std::unordered_map<const Brush*, std::string> m_utf8NameCache;

	// Animation state
	wxTimer m_animTimer;
	float hover_anim = 0.0f;
	void OnTimer(wxTimerEvent& event);
};

#endif
