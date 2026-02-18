#ifndef RME_UTIL_NANOVG_LISTBOX_H_
#define RME_UTIL_NANOVG_LISTBOX_H_

#include "util/nanovg_canvas.h"
#include <set>
#include <vector>

class NanoVGListBox : public NanoVGCanvas {
public:
	NanoVGListBox(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0);
	virtual ~NanoVGListBox();

	void SetItemCount(int count);
	int GetItemCount() const;

	int GetSelection() const;
	bool IsSelected(int item) const;
	void SetSelection(int item);
	bool Select(int item);
	bool Deselect(int item);
	void ClearSelection();

	// For multiple selection
	int GetSelectedCount() const;
	int GetSelections(wxArrayInt& selections) const;

	void EnsureVisible(int item);

	// Virtuals to be implemented by derived classes
	virtual int OnMeasureItem(size_t n) const;
	virtual void OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t n) = 0;
	virtual void OnDrawBackground(NVGcontext* vg, const wxRect& rect, size_t n) const;

protected:
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;
	void OnLeftDown(wxMouseEvent& event);
	void OnLeftDClick(wxMouseEvent& event);
	void OnMotion(wxMouseEvent& event);
	void OnLeaveWindow(wxMouseEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnSize(wxSizeEvent& event);

	void UpdateScroll();
	int HitTest(int y) const;
	void SendSelectionEvent(wxEventType type);

	int m_count;
	std::set<int> m_selections;
	bool m_multipleSelection;
	int m_hoverIndex;
	int m_anchorIndex; // For shift-selection
};

#endif // RME_UTIL_NANOVG_LISTBOX_H_
