#ifndef RME_UI_CONTROLS_NANOVG_LISTBOX_H_
#define RME_UI_CONTROLS_NANOVG_LISTBOX_H_

#include "util/nanovg_canvas.h"
#include <vector>
#include <set>

class NanoVGListBox : public NanoVGCanvas {
public:
	NanoVGListBox(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0);
	virtual ~NanoVGListBox();

	// wxVListBox-like interface
	void SetItemCount(size_t count);
	size_t GetItemCount() const { return m_itemCount; }

	int GetSelection() const;
	bool IsSelected(size_t n) const;
	bool Select(size_t n, bool select = true);
	size_t GetSelectedCount() const;
	void DeselectAll();
	bool SetSelection(int n); // Selects only this one, deselects others. Returns true if changed.

	// Helper
	void RefreshItem(size_t n);
	void RefreshAll();

	// Virtuals to implement by subclasses
	virtual void OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t n) const = 0;
	virtual wxCoord OnMeasureItem(size_t n) const = 0;

	// Optional virtuals
	virtual void OnDrawBackground(NVGcontext* vg, const wxRect& rect, size_t n) const;
	virtual void OnDrawSeparator(NVGcontext* vg, const wxRect& rect, size_t n) const {}

protected:
	// Event handling
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;
	void OnMouseDown(wxMouseEvent& event);
	void OnMotion(wxMouseEvent& event);
	void OnLeave(wxMouseEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnChar(wxKeyEvent& event);

	void RecalculateHeights();
	int HitTest(int x, int y) const;
	wxRect GetItemRect(size_t n) const;
	void EnsureVisible(size_t n);

	// Selection helpers
	void HandleClick(int index, int flags);

	size_t m_itemCount;
	std::set<size_t> m_selections;
	int m_hoverIndex;
	long m_style;
	int m_lastFocusIndex; // For shift-click range selection

	// Layout cache
	std::vector<int> m_itemY; // Y position of each item. Size = m_itemCount + 1.
	int m_totalHeight;
};

#endif // RME_UI_CONTROLS_NANOVG_LISTBOX_H_
