#include "util/nanovg_listbox.h"
#include <glad/glad.h>
#include <nanovg.h>
#include <algorithm>

NanoVGListBox::NanoVGListBox(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style) :
	NanoVGCanvas(parent, id, style | wxVSCROLL | wxWANTS_CHARS),
	m_count(0),
	m_multipleSelection((style & wxLB_MULTIPLE) != 0 || (style & wxLB_EXTENDED) != 0),
	m_hoverIndex(-1),
	m_anchorIndex(-1) {

	if (pos != wxDefaultPosition) {
		SetPosition(pos);
	}
	if (size != wxDefaultSize) {
		SetSize(size);
		SetMinSize(size);
	}

	Bind(wxEVT_LEFT_DOWN, &NanoVGListBox::OnLeftDown, this);
	Bind(wxEVT_LEFT_DCLICK, &NanoVGListBox::OnLeftDClick, this);
	Bind(wxEVT_MOTION, &NanoVGListBox::OnMotion, this);
	Bind(wxEVT_LEAVE_WINDOW, &NanoVGListBox::OnLeaveWindow, this);
	Bind(wxEVT_KEY_DOWN, &NanoVGListBox::OnKeyDown, this);
	Bind(wxEVT_SIZE, &NanoVGListBox::OnSize, this);
}

NanoVGListBox::~NanoVGListBox() {
	////
}

void NanoVGListBox::SetItemCount(int count) {
	if (m_count != count) {
		m_count = count;
		m_selections.clear();
		m_anchorIndex = -1;
		UpdateScroll();
		Refresh();
	}
}

int NanoVGListBox::GetItemCount() const {
	return m_count;
}

int NanoVGListBox::GetSelection() const {
	if (m_selections.empty()) {
		return wxNOT_FOUND;
	}
	return *m_selections.begin();
}

bool NanoVGListBox::IsSelected(int item) const {
	return m_selections.find(item) != m_selections.end();
}

void NanoVGListBox::SetSelection(int item) {
	if (item >= 0 && item < m_count) {
		m_selections.clear();
		m_selections.insert(item);
		m_anchorIndex = item;
		EnsureVisible(item);
		Refresh();
	} else if (item == wxNOT_FOUND) {
		ClearSelection();
	}
}

bool NanoVGListBox::Select(int item) {
	if (item >= 0 && item < m_count) {
		if (m_selections.find(item) == m_selections.end()) {
			if (!m_multipleSelection) {
				m_selections.clear();
			}
			m_selections.insert(item);
			Refresh();
			return true;
		}
	}
	return false;
}

bool NanoVGListBox::Deselect(int item) {
	if (m_selections.erase(item) > 0) {
		Refresh();
		return true;
	}
	return false;
}

void NanoVGListBox::ClearSelection() {
	if (!m_selections.empty()) {
		m_selections.clear();
		m_anchorIndex = -1;
		Refresh();
	}
}

int NanoVGListBox::GetSelectedCount() const {
	return static_cast<int>(m_selections.size());
}

int NanoVGListBox::GetSelections(wxArrayInt& selections) const {
	selections.Clear();
	for (int sel : m_selections) {
		selections.Add(sel);
	}
	return static_cast<int>(m_selections.size());
}

void NanoVGListBox::EnsureVisible(int item) {
	if (item < 0 || item >= m_count) {
		return;
	}

	int rowHeight = OnMeasureItem(item); // Assume uniform for simplicity in scrolling logic
	int itemY = item * rowHeight;
	int scrollPos = GetScrollPosition();
	int clientHeight = GetClientSize().y;

	if (itemY < scrollPos) {
		SetScrollPosition(itemY);
	} else if (itemY + rowHeight > scrollPos + clientHeight) {
		SetScrollPosition(itemY + rowHeight - clientHeight);
	}
}

void NanoVGListBox::UpdateScroll() {
	int rowHeight = OnMeasureItem(0);
	int contentHeight = m_count * rowHeight;
	UpdateScrollbar(contentHeight);
}

int NanoVGListBox::HitTest(int y) const {
	int rowHeight = OnMeasureItem(0);
	if (rowHeight <= 0) return wxNOT_FOUND;

	int scrollPos = GetScrollPosition();
	int index = (y + scrollPos) / rowHeight;

	if (index >= 0 && index < m_count) {
		return index;
	}
	return wxNOT_FOUND;
}

void NanoVGListBox::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	int rowHeight = OnMeasureItem(0);
	if (rowHeight <= 0) return;

	int scrollPos = GetScrollPosition();
	int startRow = scrollPos / rowHeight;
	int endRow = (scrollPos + height + rowHeight - 1) / rowHeight + 1;

	if (startRow < 0) startRow = 0;
	if (endRow > m_count) endRow = m_count;

	// Background
	nvgBeginPath(vg);
	nvgRect(vg, 0, scrollPos, width, height); // Fill visible area
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255)); // White background by default
	nvgFill(vg);

	for (int i = startRow; i < endRow; ++i) {
		wxRect rect(0, i * rowHeight, width, rowHeight);

		// Highlight selection
		if (IsSelected(i)) {
			nvgBeginPath(vg);
			nvgRect(vg, rect.x, rect.y, rect.width, rect.height);
			if (HasFocus()) {
				nvgFillColor(vg, nvgRGBA(0, 120, 215, 255)); // Standard selection blue
			} else {
				nvgFillColor(vg, nvgRGBA(200, 200, 200, 255)); // Inactive selection gray
			}
			nvgFill(vg);
		} else if (i == m_hoverIndex) {
			nvgBeginPath(vg);
			nvgRect(vg, rect.x, rect.y, rect.width, rect.height);
			nvgFillColor(vg, nvgRGBA(240, 240, 240, 255)); // Hover gray
			nvgFill(vg);
		}

		OnDrawBackground(vg, rect, i);
		OnDrawItem(vg, rect, i);
	}
}

void NanoVGListBox::OnLeftDown(wxMouseEvent& event) {
	SetFocus();
	int index = HitTest(event.GetY());

	if (index != wxNOT_FOUND) {
		bool ctrl = event.ControlDown();
		bool shift = event.ShiftDown();

		long style = GetWindowStyle();
		bool isExtended = (style & wxLB_EXTENDED) != 0;
		bool isMultiple = (style & wxLB_MULTIPLE) != 0;

		if (isExtended) {
			if (shift && m_anchorIndex != -1) {
				m_selections.clear();
				int start = std::min(m_anchorIndex, index);
				int end = std::max(m_anchorIndex, index);
				for (int i = start; i <= end; ++i) {
					m_selections.insert(i);
				}
			} else if (ctrl) {
				if (IsSelected(index)) {
					m_selections.erase(index);
				} else {
					m_selections.insert(index);
					m_anchorIndex = index;
				}
			} else {
				m_selections.clear();
				m_selections.insert(index);
				m_anchorIndex = index;
			}
		} else if (isMultiple) {
			if (IsSelected(index)) {
				m_selections.erase(index);
			} else {
				m_selections.insert(index);
				m_anchorIndex = index;
			}
		} else {
			m_selections.clear();
			m_selections.insert(index);
			m_anchorIndex = index;
		}

		Refresh();
		SendSelectionEvent(wxEVT_LISTBOX);
	}
}

void NanoVGListBox::OnLeftDClick(wxMouseEvent& event) {
	int index = HitTest(event.GetY());
	if (index != wxNOT_FOUND) {
		SendSelectionEvent(wxEVT_LISTBOX_DCLICK);
	}
}

void NanoVGListBox::OnMotion(wxMouseEvent& event) {
	int index = HitTest(event.GetY());
	if (index != m_hoverIndex) {
		m_hoverIndex = index;
		Refresh();
	}
}

void NanoVGListBox::OnLeaveWindow(wxMouseEvent& event) {
	if (m_hoverIndex != -1) {
		m_hoverIndex = -1;
		Refresh();
	}
}

void NanoVGListBox::OnKeyDown(wxKeyEvent& event) {
	int key = event.GetKeyCode();
	int selection = GetSelection();

	if (key == WXK_DOWN) {
		if (selection < m_count - 1) {
			SetSelection(selection + 1);
			SendSelectionEvent(wxEVT_LISTBOX);
		} else if (selection == wxNOT_FOUND && m_count > 0) {
			SetSelection(0);
			SendSelectionEvent(wxEVT_LISTBOX);
		}
	} else if (key == WXK_UP) {
		if (selection > 0) {
			SetSelection(selection - 1);
			SendSelectionEvent(wxEVT_LISTBOX);
		} else if (selection == wxNOT_FOUND && m_count > 0) {
			SetSelection(m_count - 1);
			SendSelectionEvent(wxEVT_LISTBOX);
		}
	} else if (key == WXK_PAGEDOWN) {
		int visibleRows = GetClientSize().y / std::max(1, OnMeasureItem(0));
		int newSel = std::min(m_count - 1, selection + visibleRows);
		SetSelection(newSel);
		SendSelectionEvent(wxEVT_LISTBOX);
	} else if (key == WXK_PAGEUP) {
		int visibleRows = GetClientSize().y / std::max(1, OnMeasureItem(0));
		int newSel = std::max(0, selection - visibleRows);
		SetSelection(newSel);
		SendSelectionEvent(wxEVT_LISTBOX);
	} else {
		event.Skip();
	}
}

void NanoVGListBox::OnSize(wxSizeEvent& event) {
	UpdateScroll();
	Refresh();
	event.Skip();
}

void NanoVGListBox::SendSelectionEvent(wxEventType type) {
	wxCommandEvent event(type, GetId());
	event.SetEventObject(this);
	if (!m_selections.empty()) {
		event.SetInt(*m_selections.begin());
	} else {
		event.SetInt(wxNOT_FOUND);
	}
	ProcessEvent(event);
}

int NanoVGListBox::OnMeasureItem(size_t n) const {
	return 20; // Default height
}

void NanoVGListBox::OnDrawBackground(NVGcontext* vg, const wxRect& rect, size_t n) const {
	// Optional override
}
