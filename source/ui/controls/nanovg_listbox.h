#ifndef RME_UI_CONTROLS_NANOVG_LISTBOX_H_
#define RME_UI_CONTROLS_NANOVG_LISTBOX_H_

#include "util/nanovg_canvas.h"
#include <unordered_set>

class NanoVGListBox : public NanoVGCanvas {
public:
	NanoVGListBox(wxWindow* parent, wxWindowID id, long style = 0);
	virtual ~NanoVGListBox();

	// ListBox Interface
	void SetItemCount(size_t count);
	size_t GetItemCount() const { return m_itemCount; }

	void SetSelection(int index); // Deselects others if single selection
	int GetSelection() const; // Returns first selected or wxNOT_FOUND
	bool IsSelected(int index) const;
	void Select(int index, bool select = true);
	void DeselectAll();
	size_t GetSelectedCount() const;

	void SetRowHeight(int height);
	void ScrollTo(int index);

	void SetFocusItem(int index);
	int GetFocusItem() const { return m_focusedIndex; }

protected:
	// Virtuals to be implemented by subclass
	virtual void OnDrawItem(NVGcontext* vg, int index, const wxRect& rect, bool selected) = 0;

	// Optional callbacks
	virtual void OnSelectionChanged() {}
	virtual void OnItemDoubleClicked(int index) {}

	// NanoVGCanvas overrides
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;

private:
	void OnMouse(wxMouseEvent& evt);
	void OnKey(wxKeyEvent& evt);
	void RecalculateHeight();

	size_t m_itemCount = 0;
	std::unordered_set<int> m_selectedIndices;
	bool m_multipleSelection = false;

	// Cache item heights to avoid frequent recalculation if variable
	// For now, we assume fixed height or fast OnMeasureItem.
	// We optimize by assuming fixed height if a flag is set?
	// wxVListBox allows variable height. We should support it.
	// To support scrolling properly with variable height, we need cumulative heights.
	// For simplicity and performance, if OnMeasureItem is O(1), we can compute.
	// But calculating total height requires O(N).
	// We will cache total height.

	// Optimization: If all items have same height (hint), we can compute fast.
	// But let's stick to iterating for correctness first.
	// Actually, `wxVListBox` caches heights.

	// Simplification: We will call OnMeasureItem for ALL items when SetItemCount is called or size changes?
	// If N is large (thousands), this is slow.
	// `BrowseTileListBox` has few items (items on a tile).
	// `DatDebugViewListBox` has thousands.
	// So we need efficient scrolling.
	// We can use a "fixed height" optimization if the subclass returns a constant.

	// We will assume fixed height of 32 for now as most sprites are 32x32?
	// `DatDebugViewListBox` returns 32.
	// `BrowseTileListBox` returns 32.
	// `FindDialogListBox` returns 32.
	// So we can assume fixed height for these specific cases!
	// I will add `SetRowHeight(int height)` to enforce fixed height optimization.
	int m_rowHeight = 32;
	bool m_fixedHeight = true;

	int m_focusedIndex = wxNOT_FOUND;
	int m_anchorIndex = wxNOT_FOUND;

	int GetItemAt(int y);
	wxRect GetItemRect(int index);
};

#endif
