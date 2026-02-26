#ifndef RME_UTIL_NANOVG_LIST_BOX_H_
#define RME_UTIL_NANOVG_LIST_BOX_H_

#include "util/nanovg_canvas.h"

class NanoVGListBox : public NanoVGCanvas {
public:
	NanoVGListBox(wxWindow* parent, wxWindowID id = wxID_ANY);
	virtual ~NanoVGListBox();

	// Data management
	void SetItemCount(size_t count);
	size_t GetItemCount() const {
		return m_itemCount;
	}

	// Selection
	void SetSelection(int index);
	int GetSelection() const {
		return m_selection;
	}
	bool IsSelected(int index) const {
		return m_selection == index;
	}

	// Rendering
	void RefreshList();

protected:
	// To be implemented by subclasses
	virtual void OnDrawItem(NVGcontext* vg, int index, const wxRect& rect, bool selected) = 0;
	virtual int GetItemHeight() const {
		return 24;
	}

	// NanoVGCanvas overrides
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;
	wxSize DoGetBestClientSize() const override;

	// Event handlers
	void OnMouseDown(wxMouseEvent& event);
	void OnMouseDouble(wxMouseEvent& event);
	void OnKeyDown(wxKeyEvent& event);

protected:
	void SendSelectionEvent();

private:
	size_t m_itemCount = 0;
	int m_selection = wxNOT_FOUND;
};

#endif
