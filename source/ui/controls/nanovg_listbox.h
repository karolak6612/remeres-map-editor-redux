#ifndef RME_UI_CONTROLS_NANOVG_LISTBOX_H_
#define RME_UI_CONTROLS_NANOVG_LISTBOX_H_

#include "util/nanovg_canvas.h"
#include <unordered_set>
#include <functional>

/**
 * @class NanoVGListBox
 * @brief A high-performance list box control using NanoVG for rendering.
 *
 * Replaces wxVListBox for large datasets, providing virtual scrolling,
 * multi-selection, and GPU-accelerated item rendering.
 */
class NanoVGListBox : public NanoVGCanvas {
public:
	/**
	 * @brief Constructs a NanoVGListBox.
	 * @param parent Parent window
	 * @param id Window ID
	 * @param style Window style (e.g., wxLB_MULTIPLE)
	 */
	NanoVGListBox(wxWindow* parent, wxWindowID id = wxID_ANY, long style = 0);
	virtual ~NanoVGListBox();

	// Data Management
	void SetItemCount(int count);
	int GetItemCount() const { return m_count; }

	// Selection
	void SetSelection(int index); // Clears others if not multiple
	void Select(int index, bool select = true);
	void SelectRange(int start, int end, bool select = true);
	void Toggle(int index);
	void DeselectAll();
	bool IsSelected(int index) const;
	int GetSelection() const; // Returns first selected index or -1
	int GetSelectedCount() const;
	std::vector<int> GetSelections() const;

	// Layout
	void SetRowHeight(int height);
	int GetRowHeight() const { return m_rowHeight; }

	// Events
	// Override this to draw list items
	virtual void OnDrawItem(NVGcontext* vg, const wxRect& rect, int index) = 0;

	// Optional callback when selection changes
	virtual void OnSelectionChanged() {}

protected:
	// NanoVGCanvas overrides
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;
	wxSize DoGetBestClientSize() const override;

	// Input handling
	void OnMouseDown(wxMouseEvent& event);
	void OnMotion(wxMouseEvent& event);
	void OnKeyDown(wxKeyEvent& event);

	// Helpers
	int HitTest(int y) const;
	void EnsureVisible(int index);

	int m_count = 0;
	int m_rowHeight = 24;
	int m_hoverIndex = -1;
	int m_anchorIndex = -1; // For shift-selection range

	std::unordered_set<int> m_selections;
	bool m_multipleSelection = false;
};

#endif // RME_UI_CONTROLS_NANOVG_LISTBOX_H_
