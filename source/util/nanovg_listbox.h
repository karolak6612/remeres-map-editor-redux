#ifndef RME_UTIL_NANOVG_LISTBOX_H_
#define RME_UTIL_NANOVG_LISTBOX_H_

#include "app/main.h"
#include "util/nanovg_canvas.h"
#include <vector>

/**
 * @class NanoVGListBox
 * @brief High-performance virtual list box using NanoVG rendering.
 * Mimics wxVListBox behavior but with hardware acceleration.
 * @note This implementation assumes all list items have a uniform height for performance reasons.
 */
class NanoVGListBox : public NanoVGCanvas {
public:
	NanoVGListBox(wxWindow* parent, wxWindowID id, long style = 0);
	virtual ~NanoVGListBox();

	// Data Handling
	void SetItemCount(size_t count);
	size_t GetItemCount() const {
		return m_count;
	}

	// Selection
	virtual void SetSelection(int index);
	int GetSelection() const;
	bool IsSelected(int index) const;
	virtual void Select(int index, bool select = true);
	int GetSelectedCount() const;
	virtual void ClearSelection();

	// Drawing (Virtuals)
	virtual void OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t index) = 0;
	virtual int OnMeasureItem(size_t index) const = 0;

	// Refresh
	void RefreshItem(size_t index);
	void RefreshAll();

	// Ensure Visible
	void ScrollToLine(int line);
	void EnsureVisible(int line);

protected:
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;
	wxSize DoGetBestClientSize() const override;

	void UpdateScrollbar();
	void OnSize(wxSizeEvent& event);
	void OnMouseDown(wxMouseEvent& event);
	void OnDoubleClick(wxMouseEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnMotion(wxMouseEvent& event);
	void OnLeave(wxMouseEvent& event);

	void SendSelectionEvent();
	int HitTest(int x, int y) const;
	wxRect GetItemRect(int index) const;

	size_t m_count;
	int m_selection; // For single selection (-1 if none)
	std::vector<bool> m_multiSelection; // For multiple selection
	long m_style; // wxLB_SINGLE, wxLB_MULTIPLE, etc.

	int m_hoverIndex;
	int m_focusIndex;
};

#endif
