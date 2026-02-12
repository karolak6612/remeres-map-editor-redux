#ifndef RME_UI_CONTROLS_NANOVG_LISTBOX_H_
#define RME_UI_CONTROLS_NANOVG_LISTBOX_H_

#include "util/nanovg_canvas.h"
#include <vector>
#include <set>

class NanoVGListBox : public NanoVGCanvas {
public:
	NanoVGListBox(wxWindow* parent, wxWindowID id, long style = 0);
	virtual ~NanoVGListBox();

	void SetItemCount(size_t count);
	size_t GetItemCount() const {
		return m_itemCount;
	}

	void SetItemHeight(int height);
	int GetItemHeight() const {
		return m_itemHeight;
	}

	// Selection
	void SetSelection(int n); // Selects item n, deselects others if single selection
	bool IsSelected(int n) const;
	void Select(int n); // Adds n to selection
	void Deselect(int n); // Removes n from selection
	void DeselectAll();
	void SelectAll(); // Only if multiple selection is allowed

	int GetSelection() const; // Returns first selected item or -1
	int GetSelectedCount() const;

	// Scroll to ensure item is visible
	void EnsureVisible(int n);

protected:
	virtual void OnSelectionChanged(int n, bool selected);

	virtual void OnDrawItem(struct NVGcontext* vg, const wxRect& rect, size_t n) const = 0;
	virtual int OnMeasureItem(size_t n) const; // Default implementation returns m_itemHeight

	void OnNanoVGPaint(struct NVGcontext* vg, int width, int height) override;

	// Event handlers
	void OnLeftDown(wxMouseEvent& event);
	void OnLeftUp(wxMouseEvent& event);
	void OnMotion(wxMouseEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnSize(wxSizeEvent& event);

	size_t m_itemCount;
	int m_itemHeight; // Fixed height for optimization
	std::set<int> m_selections;

	// For hit testing
	int HitTest(const wxPoint& pt) const;

	bool m_multipleSelection;
};

#endif
