#ifndef RME_UI_CONTROLS_NANOVG_LISTBOX_H_
#define RME_UI_CONTROLS_NANOVG_LISTBOX_H_

#include "util/nanovg_canvas.h"
#include <wx/timer.h>
#include <unordered_set>

class NanoVGListBox : public NanoVGCanvas {
public:
	NanoVGListBox(wxWindow* parent, wxWindowID id = wxID_ANY, long style = 0);
	virtual ~NanoVGListBox();

	void SetItemCount(int count);
	int GetItemCount() const { return m_itemCount; }

	void SetSelection(int index); // Clears others if single
	int GetSelection() const { return m_selection; }
	bool IsSelected(int index) const;
	void Select(int index, bool select = true);
	void DeselectAll();
	int GetSelectedCount() const;

	// Drawing interface
	virtual void OnDrawItem(NVGcontext* vg, int index, const wxRect& rect) = 0;
	virtual wxCoord OnMeasureItem(int index) const = 0; // Like wxVListBox

	// Customization
	void SetItemHeight(int height); // Helper for fixed height lists

protected:
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;

	// Events
	void OnMouseDown(wxMouseEvent& event);
	void OnLeftDClick(wxMouseEvent& event);
	void OnMotion(wxMouseEvent& event);
	void OnLeave(wxMouseEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnTimer(wxTimerEvent& event);

	int HitTest(int y) const;
	void UpdateLayout();
	wxRect GetItemRect(int index) const;

	int m_itemCount;
	int m_selection; // Primary selection (for anchor/focus)
	int m_hover;

	bool m_multipleSelection;
	std::unordered_set<int> m_selections;

	// Optimization for fixed height (if OnMeasureItem returns constant)
	int m_itemHeight;

	wxTimer m_animTimer;
};

#endif
