#ifndef RME_UI_CONTROLS_VIRTUAL_LIST_CANVAS_H_
#define RME_UI_CONTROLS_VIRTUAL_LIST_CANVAS_H_

#include "util/nanovg_canvas.h"
#include <vector>
#include <set>

/**
 * @class VirtualListCanvas
 * @brief A high-performance, virtual list control using NanoVG rendering.
 *
 * Replaces wxVListBox for lists with many items or custom rendering requirements.
 * Supports:
 * - Virtual scrolling (millions of items)
 * - Custom item drawing via NanoVG
 * - Single and Multiple selection modes
 * - Keyboard navigation
 * - Hover effects
 */
class VirtualListCanvas : public NanoVGCanvas {
public:
	enum class SelectionMode {
		Single,
		Multiple
	};

	VirtualListCanvas(wxWindow* parent, wxWindowID id = wxID_ANY, long style = wxVSCROLL | wxWANTS_CHARS);
	virtual ~VirtualListCanvas();

	// Data Provider Interface
	virtual size_t GetItemCount() const = 0;

	// Drawing Interface
	/**
	 * @brief Draw a single item.
	 * @param vg NanoVG context.
	 * @param index Item index.
	 * @param rect Item rectangle in local coordinates (already scrolled).
	 */
	virtual void OnDrawItem(NVGcontext* vg, int index, const wxRect& rect) = 0;

	// Selection Management
	void SetSelectionMode(SelectionMode mode);
	SelectionMode GetSelectionMode() const { return m_selectionMode; }

	void SetSelection(int index); // Clears others if Single
	void Select(int index, bool select = true);
	void Deselect(int index);
	void SelectAll();
	void DeselectAll();
	bool IsSelected(int index) const;
	int GetSelection() const; // Returns first selected index or wxNOT_FOUND
	size_t GetSelectedCount() const;
	std::vector<int> GetSelections() const;

	// Layout & Scrolling
	void SetItemHeight(int height);
	int GetItemHeight(int index) const;
	void RefreshList(); // Recalculate layout and repaint
	void EnsureVisible(int index);

	// Event handling
	void SendSelectionEvent();

protected:
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;
	wxSize DoGetBestClientSize() const override;

	// Event handlers
	void OnMouseDown(wxMouseEvent& event);
	void OnMouseUp(wxMouseEvent& event);
	void OnMotion(wxMouseEvent& event);
	void OnLeave(wxMouseEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnChar(wxKeyEvent& event);

	int HitTest(int y) const;
	void ToggleSelection(int index);
	void RangeSelect(int anchor, int current);

protected:
	int m_itemHeight;
	SelectionMode m_selectionMode;

	std::set<int> m_selectedIndices;
	int m_anchorIndex; // For range selection (Shift+Click)
	int m_hoverIndex;
	int m_focusedIndex; // For keyboard navigation

	// Style colors (can be customized by subclasses or theme)
	NVGcolor m_selectionColor;
	NVGcolor m_hoverColor;
	NVGcolor m_textColor;
};

#endif
