#ifndef RME_UI_CONTROLS_VIRTUAL_LIST_CANVAS_H_
#define RME_UI_CONTROLS_VIRTUAL_LIST_CANVAS_H_

#include "app/main.h"
#include "util/nanovg_canvas.h"
#include <vector>
#include <unordered_set>

/**
 * @class VirtualListCanvas
 * @brief A high-performance list control base class using NanoVG for rendering.
 *
 * This class replaces wxVListBox and wxListBox for cases requiring custom drawing
 * (e.g., sprites, formatted text) with hardware acceleration.
 *
 * It supports:
 * - Virtual data source (via GetItemCount/OnDrawItem)
 * - Single or Multiple selection modes
 * - Keyboard navigation
 * - Mouse interaction
 */
class VirtualListCanvas : public NanoVGCanvas {
public:
	enum SelectionMode {
		SINGLE,
		MULTIPLE
	};

	VirtualListCanvas(wxWindow* parent, wxWindowID id = wxID_ANY, SelectionMode mode = SINGLE);
	virtual ~VirtualListCanvas();

	// Data Provider Interface
	virtual size_t GetItemCount() const = 0;
	virtual int GetItemHeight() const { return FromDIP(32); } // Default height

	// Selection
	void SetSelection(int index);
	int GetSelection() const; // Returns first selected index or wxNOT_FOUND
	bool IsSelected(size_t index) const;
	void Select(size_t index, bool select = true);
	void DeselectAll();
	void SelectAll(); // Only for MULTIPLE mode
	size_t GetSelectedCount() const;
	std::vector<int> GetSelections() const;

	// View Control
	void RefreshList(); // Recalculates layout and repaints
	void EnsureVisible(int index);

protected:
	// Drawing Hook
	virtual void OnDrawItem(NVGcontext* vg, int index, const wxRect& rect) = 0;

	// NanoVGCanvas Overrides
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;
	wxSize DoGetBestClientSize() const override;

	// Event Handlers
	void OnLeftDown(wxMouseEvent& event);
	void OnLeftDClick(wxMouseEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnSize(wxSizeEvent& event);

	// Internal Helpers
	int HitTest(int y) const;
	void UpdateScrollbar();

	SelectionMode m_selectionMode;
	std::unordered_set<int> m_selections;
	int m_hoverIndex = wxNOT_FOUND;
	int m_anchorIndex = wxNOT_FOUND; // For shift-selection

private:
	// Cached metrics
	int m_itemHeight = 32;
};

#endif // RME_UI_CONTROLS_VIRTUAL_LIST_CANVAS_H_
