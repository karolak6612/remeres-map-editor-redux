#ifndef RME_UI_CONTROLS_NANOVG_LISTBOX_H_
#define RME_UI_CONTROLS_NANOVG_LISTBOX_H_

#include "util/nanovg_canvas.h"
#include <vector>
#include <unordered_set>

class NanoVGListBox : public NanoVGCanvas {
public:
	NanoVGListBox(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0);
	virtual ~NanoVGListBox();

	// ListBox interface
	void SetItemCount(size_t count);
	size_t GetItemCount() const { return m_itemCount; }

	void SetSelection(int selection); // Selects single item, clears others
	int GetSelection() const; // Returns first selected item or wxNOT_FOUND
	bool IsSelected(int n) const;
	void Select(int n);
	void Deselect(int n);
	void DeselectAll();

	size_t GetSelectedCount() const { return m_selectedItems.size(); }

	// Virtual methods to be implemented by derived classes
	virtual void OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t index) = 0;
	virtual wxCoord OnMeasureItem(size_t index) const = 0;

	// Helper to scroll to item
	void ScrollToLine(int line);
	void EnsureVisible(int line);

protected:
	virtual void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;
	virtual void OnKeyDown(wxKeyEvent& event);
	virtual void OnLeftDown(wxMouseEvent& event);
	virtual void OnLeftDClick(wxMouseEvent& event);

	// Internal
	int HitTest(int y) const;
	void RecalculateContentHeight();

	size_t m_itemCount;
	std::unordered_set<int> m_selectedItems;
	int m_focusedItem; // Current item for keyboard navigation

	// Cache item heights to avoid recalculating constantly?
	// wxVListBox doesn't cache by default but assumes fast OnMeasureItem.
	// We will just call OnMeasureItem during paint for visible items, and for total height calculation.
	// For variable height items, total height calculation is O(N).
	// We can cache cumulative heights if performance is an issue.
	// For now, simple O(N) scan for height is fine for small lists.

	std::vector<int> m_itemYCache; // Cache Y position of each item for faster lookup
	int m_totalHeight;

	bool m_multipleSelection;
};

#endif
