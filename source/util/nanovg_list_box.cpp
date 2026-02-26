#include "app/main.h"
#include "util/nanovg_list_box.h"
#include <wx/dcclient.h>

NanoVGListBox::NanoVGListBox(wxWindow* parent, wxWindowID id) :
	NanoVGCanvas(parent, id, wxVSCROLL | wxWANTS_CHARS) {
	Bind(wxEVT_LEFT_DOWN, &NanoVGListBox::OnMouseDown, this);
	Bind(wxEVT_LEFT_DCLICK, &NanoVGListBox::OnMouseDouble, this);
	Bind(wxEVT_KEY_DOWN, &NanoVGListBox::OnKeyDown, this);
}

NanoVGListBox::~NanoVGListBox() {
}

void NanoVGListBox::SetItemCount(size_t count) {
	m_itemCount = count;
	SetScrollStep(GetItemHeight());
	UpdateScrollbar(static_cast<int>(m_itemCount * GetItemHeight()));
	Refresh();
}

void NanoVGListBox::SetSelection(int index) {
	if (index >= (int)m_itemCount) {
		index = wxNOT_FOUND;
	}
	if (m_selection != index) {
		m_selection = index;
		Refresh();
	}
}

void NanoVGListBox::SendSelectionEvent() {
	wxCommandEvent event(wxEVT_LISTBOX, GetId());
	event.SetEventObject(this);
	event.SetInt(m_selection);
	GetEventHandler()->ProcessEvent(event);
}

void NanoVGListBox::RefreshList() {
	UpdateScrollbar(static_cast<int>(m_itemCount * GetItemHeight()));
	Refresh();
}

void NanoVGListBox::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	int itemHeight = GetItemHeight();
	if (itemHeight <= 0) {
		return;
	}

	// Calculate visible range
	int scrollPos = GetScrollPosition();
	// Calculate start index based on scroll position
	int startItem = scrollPos / itemHeight;
	// Calculate how many items fit in the visible height + 1 for partial
	int count = (height + itemHeight - 1) / itemHeight + 1;

	// Draw background
	nvgBeginPath(vg);
	// We fill the visible area (or whole canvas, since we are translated)
	// Actually, clearing the background is usually done by caller or by clearing the window.
	// But let's fill with default color.
	nvgRect(vg, 0, scrollPos, width, height);
	nvgFillColor(vg, nvgRGBf(m_bgRed, m_bgGreen, m_bgBlue));
	nvgFill(vg);

	for (int i = 0; i < count; ++i) {
		int index = startItem + i;
		if (index >= (int)m_itemCount) {
			break;
		}

		int y = index * itemHeight;
		wxRect rect(0, y, width, itemHeight);

		bool selected = (index == m_selection);
		if (selected) {
			nvgBeginPath(vg);
			nvgRect(vg, rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
			nvgFillColor(vg, nvgRGBA(0, 120, 215, 255)); // Selection blue
			nvgFill(vg);
		}

		OnDrawItem(vg, index, rect, selected);
	}
}

wxSize NanoVGListBox::DoGetBestClientSize() const {
	return wxSize(200, 300); // Default size
}

void NanoVGListBox::OnMouseDown(wxMouseEvent& event) {
	SetFocus();
	int y = event.GetY() + GetScrollPosition();
	int itemHeight = GetItemHeight();
	if (itemHeight > 0) {
		int index = y / itemHeight;

		if (index >= 0 && index < (int)m_itemCount) {
			if (m_selection != index) {
				SetSelection(index);
				SendSelectionEvent();
			}
		}
	}
}

void NanoVGListBox::OnMouseDouble(wxMouseEvent& event) {
	SetFocus();
	int y = event.GetY() + GetScrollPosition();
	int itemHeight = GetItemHeight();
	if (itemHeight > 0) {
		int index = y / itemHeight;

		if (index >= 0 && index < (int)m_itemCount) {
			if (m_selection != index) {
				SetSelection(index);
				SendSelectionEvent();
			}

			wxCommandEvent doubleClickEvent(wxEVT_LISTBOX_DCLICK, GetId());
			doubleClickEvent.SetEventObject(this);
			doubleClickEvent.SetInt(m_selection);
			GetEventHandler()->ProcessEvent(doubleClickEvent);
		}
	}
}

void NanoVGListBox::OnKeyDown(wxKeyEvent& event) {
	int key = event.GetKeyCode();
	int itemHeight = GetItemHeight();

	if (itemHeight <= 0) {
		event.Skip();
		return;
	}

	if (key == WXK_DOWN) {
		if (m_selection < (int)m_itemCount - 1) {
			SetSelection(m_selection + 1);
			SendSelectionEvent();
			// Ensure visible
			int y = m_selection * itemHeight;
			int clientH = GetClientSize().GetHeight();
			int scrollPos = GetScrollPosition();

			if (y + itemHeight > scrollPos + clientH) {
				SetScrollPosition(y + itemHeight - clientH);
			} else if (y < scrollPos) {
				SetScrollPosition(y);
			}
		}
	} else if (key == WXK_UP) {
		if (m_selection > 0) {
			SetSelection(m_selection - 1);
			SendSelectionEvent();
			// Ensure visible
			int y = m_selection * itemHeight;
			int scrollPos = GetScrollPosition();

			if (y < scrollPos) {
				SetScrollPosition(y);
			} else if (y + itemHeight > scrollPos + GetClientSize().GetHeight()) {
				SetScrollPosition(y + itemHeight - GetClientSize().GetHeight());
			}
		}
	} else {
		event.Skip();
	}
}
