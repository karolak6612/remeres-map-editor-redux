#ifndef RME_UTIL_NANOVG_LISTBOX_H_
#define RME_UTIL_NANOVG_LISTBOX_H_

#include "util/nanovg_canvas.h"
#include <unordered_set>
#include <vector>

class NanoVGListBox : public NanoVGCanvas {
public:
	NanoVGListBox(wxWindow* parent, wxWindowID id, long style = 0);
	virtual ~NanoVGListBox();

	// ListBox methods
	void SetItemCount(int count);
	int GetItemCount() const {
		return m_count;
	}

	// Selection
	void SetSelection(int n); // Selects only n, deselects others
	int GetSelection() const; // Returns first selected item, or wxNOT_FOUND
	bool IsSelected(int n) const;
	void Select(int n); // Adds n to selection (if multiple)
	void Deselect(int n);
	void DeselectAll();
	int GetSelectedCount() const {
		return m_selections.size();
	}
	bool HasSelection() const {
		return !m_selections.empty();
	}

	// Virtual methods to implement
	virtual void OnDrawItem(NVGcontext* vg, const wxRect& rect, int index) const = 0;
	virtual int OnMeasureItem(int index) const; // Default returns 20

	// Helper
	int HitTest(const wxPoint& point) const;
	void EnsureVisible(int n);

protected:
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;
	void OnLeftDown(wxMouseEvent& event);
	void OnLeftDClick(wxMouseEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void RecalculateLayout();

	int m_count;
	long m_style;
	std::unordered_set<int> m_selections;

	// Cached Y positions for variable height items.
	// m_itemY[i] is the Y coordinate of the top of item i.
	// m_itemY[m_count] is the total height.
	std::vector<int> m_itemY;
	int m_totalHeight;
	int m_lastSelection; // For Shift-click range selection
};

#endif
