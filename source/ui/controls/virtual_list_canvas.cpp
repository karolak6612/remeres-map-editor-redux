#include "ui/controls/virtual_list_canvas.h"
#include <wx/wx.h>
#include <algorithm>
#include <nanovg.h>
#include <spdlog/spdlog.h>

VirtualListCanvas::VirtualListCanvas(wxWindow* parent, wxWindowID id, SelectionMode mode) :
	NanoVGCanvas(parent, id, wxVSCROLL | wxWANTS_CHARS),
	m_selectionMode(mode) {
	Bind(wxEVT_LEFT_DOWN, &VirtualListCanvas::OnLeftDown, this);
	Bind(wxEVT_LEFT_DCLICK, &VirtualListCanvas::OnLeftDClick, this);
	Bind(wxEVT_KEY_DOWN, &VirtualListCanvas::OnKeyDown, this);
	Bind(wxEVT_SIZE, &VirtualListCanvas::OnSize, this);

	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetScrollStep(GetItemHeight());
}

VirtualListCanvas::~VirtualListCanvas() {
}

void VirtualListCanvas::RefreshList() {
	UpdateScrollbar();
	Refresh();
}

void VirtualListCanvas::UpdateScrollbar() {
	m_itemHeight = GetItemHeight();
	if (m_itemHeight <= 0) {
		m_itemHeight = 32;
	}
	NanoVGCanvas::UpdateScrollbar(static_cast<int>(GetItemCount()) * m_itemHeight);
}

void VirtualListCanvas::EnsureVisible(int index) {
	if (index < 0 || static_cast<size_t>(index) >= GetItemCount()) {
		return;
	}

	int itemTop = index * m_itemHeight;
	int itemBottom = itemTop + m_itemHeight;

	int scrollTop = GetScrollPosition();
	int scrollBottom = scrollTop + GetClientSize().y;

	if (itemTop < scrollTop) {
		SetScrollPosition(itemTop);
	} else if (itemBottom > scrollBottom) {
		SetScrollPosition(itemBottom - GetClientSize().y);
	}
}

void VirtualListCanvas::SetSelection(int index) {
	DeselectAll();
	if (index != wxNOT_FOUND) {
		Select(index, true);
		m_anchorIndex = index;
		EnsureVisible(index);
	}
	Refresh();
	// Fire selection changed event
	wxCommandEvent event(wxEVT_LISTBOX, GetId());
	event.SetEventObject(this);
	event.SetInt(index);
	GetEventHandler()->ProcessEvent(event);
}

int VirtualListCanvas::GetSelection() const {
	if (m_selections.empty()) {
		return wxNOT_FOUND;
	}
	// Return the "first" selection (arbitrary if unordered_set, usually smallest index is expected)
	// For compatibility, we can scan or just return *begin()
	// To be robust, find min index
	int minIdx = std::numeric_limits<int>::max();
	for (int idx : m_selections) {
		if (idx < minIdx) minIdx = idx;
	}
	return minIdx;
}

bool VirtualListCanvas::IsSelected(size_t index) const {
	return m_selections.count(static_cast<int>(index)) > 0;
}

void VirtualListCanvas::Select(size_t index, bool select) {
	if (index >= GetItemCount()) {
		return;
	}

	if (select) {
		if (m_selectionMode == SINGLE) {
			m_selections.clear();
		}
		m_selections.insert(static_cast<int>(index));
	} else {
		m_selections.erase(static_cast<int>(index));
	}
}

void VirtualListCanvas::DeselectAll() {
	m_selections.clear();
}

void VirtualListCanvas::SelectAll() {
	if (m_selectionMode == MULTIPLE) {
		for (size_t i = 0; i < GetItemCount(); ++i) {
			m_selections.insert(static_cast<int>(i));
		}
	}
}

size_t VirtualListCanvas::GetSelectedCount() const {
	return m_selections.size();
}

std::vector<int> VirtualListCanvas::GetSelections() const {
	std::vector<int> sels(m_selections.begin(), m_selections.end());
	std::sort(sels.begin(), sels.end());
	return sels;
}

void VirtualListCanvas::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	size_t count = GetItemCount();
	if (count == 0) {
		return;
	}

	int itemH = GetItemHeight();
	int scrollTop = GetScrollPosition();
	int startIndex = scrollTop / itemH;
	int endIndex = (scrollTop + height + itemH - 1) / itemH;

	startIndex = std::max(0, startIndex);
	endIndex = std::min(static_cast<int>(count), endIndex);

	// Retrieve system colors
	wxColour highlight = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
	wxColour highlightText = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);
	wxColour listText = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT);

	NVGcolor selBg = nvgRGBA(highlight.Red(), highlight.Green(), highlight.Blue(), 255);
	// We might pass text color to OnDrawItem or rely on subclass to fetch it
	// For simplicity, subclasses should handle text color based on IsSelected(index)

	for (int i = startIndex; i < endIndex; ++i) {
		int y = i * itemH; // Absolute Y
		wxRect rect(0, y, width, itemH);

		if (IsSelected(i)) {
			nvgBeginPath(vg);
			nvgRect(vg, 0, (float)y, (float)width, (float)itemH);
			nvgFillColor(vg, selBg);
			nvgFill(vg);
		}

		OnDrawItem(vg, i, rect);
	}
}

wxSize VirtualListCanvas::DoGetBestClientSize() const {
	return FromDIP(wxSize(200, 300));
}

int VirtualListCanvas::HitTest(int y) const {
	int itemH = GetItemHeight();
	if (itemH <= 0) return wxNOT_FOUND;

	int scrollTop = GetScrollPosition();
	int absoluteY = scrollTop + y;
	int index = absoluteY / itemH;

	if (index >= 0 && static_cast<size_t>(index) < GetItemCount()) {
		return index;
	}
	return wxNOT_FOUND;
}

void VirtualListCanvas::OnLeftDown(wxMouseEvent& event) {
	SetFocus(); // Essential for key events

	int index = HitTest(event.GetY());
	if (index == wxNOT_FOUND) {
		return;
	}

	if (m_selectionMode == MULTIPLE && event.ControlDown()) {
		// Toggle
		if (IsSelected(index)) {
			Select(index, false);
		} else {
			Select(index, true);
			m_anchorIndex = index;
		}
	} else if (m_selectionMode == MULTIPLE && event.ShiftDown() && m_anchorIndex != wxNOT_FOUND) {
		// Range select
		DeselectAll();
		int start = std::min(m_anchorIndex, index);
		int end = std::max(m_anchorIndex, index);
		for (int i = start; i <= end; ++i) {
			Select(i, true);
		}
	} else {
		// Single click select (clears others)
		DeselectAll();
		Select(index, true);
		m_anchorIndex = index;
	}

	Refresh();

	// Fire event
	wxCommandEvent evt(wxEVT_LISTBOX, GetId());
	evt.SetEventObject(this);
	evt.SetInt(index);
	GetEventHandler()->ProcessEvent(evt);
}

void VirtualListCanvas::OnLeftDClick(wxMouseEvent& event) {
	int index = HitTest(event.GetY());
	if (index != wxNOT_FOUND) {
		wxCommandEvent evt(wxEVT_LISTBOX_DCLICK, GetId());
		evt.SetEventObject(this);
		evt.SetInt(index);
		GetEventHandler()->ProcessEvent(evt);
	}
}

void VirtualListCanvas::OnKeyDown(wxKeyEvent& event) {
	int key = event.GetKeyCode();
	int sel = GetSelection();
	size_t count = GetItemCount();

	if (count == 0) return;

	int newSel = sel;

	if (key == WXK_DOWN) {
		if (sel == wxNOT_FOUND) newSel = 0;
		else if (sel < (int)count - 1) newSel++;
	} else if (key == WXK_UP) {
		if (sel == wxNOT_FOUND) newSel = (int)count - 1;
		else if (sel > 0) newSel--;
	} else if (key == WXK_HOME) {
		newSel = 0;
	} else if (key == WXK_END) {
		newSel = (int)count - 1;
	} else if (key == WXK_PAGEUP) {
		int visibleItems = GetClientSize().y / GetItemHeight();
		newSel = std::max(0, sel - visibleItems);
	} else if (key == WXK_PAGEDOWN) {
		int visibleItems = GetClientSize().y / GetItemHeight();
		newSel = std::min((int)count - 1, sel + visibleItems);
	} else {
		event.Skip();
		return;
	}

	if (newSel != sel) {
		if (m_selectionMode == MULTIPLE && event.ShiftDown()) {
			// Range extension
			// Simplified: Just extend from anchor
			if (m_anchorIndex == wxNOT_FOUND) m_anchorIndex = sel != wxNOT_FOUND ? sel : 0;

			DeselectAll();
			int start = std::min(m_anchorIndex, newSel);
			int end = std::max(m_anchorIndex, newSel);
			for (int i = start; i <= end; ++i) {
				Select(i, true);
			}
		} else {
			// Move selection
			DeselectAll();
			Select(newSel, true);
			m_anchorIndex = newSel;
		}

		EnsureVisible(newSel);
		Refresh();

		wxCommandEvent evt(wxEVT_LISTBOX, GetId());
		evt.SetEventObject(this);
		evt.SetInt(newSel);
		GetEventHandler()->ProcessEvent(evt);
	}
}

void VirtualListCanvas::OnSize(wxSizeEvent& event) {
	UpdateScrollbar();
	Refresh();
	event.Skip();
}
