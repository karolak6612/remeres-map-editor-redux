#include "ui/controls/nanovg_listbox.h"
#include <wx/wx.h>
#include <nanovg.h>
#include "app/main.h"

NanoVGListBox::NanoVGListBox(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style) :
	NanoVGCanvas(parent, id, wxVSCROLL | wxWANTS_CHARS),
	m_itemCount(0),
	m_hoverIndex(-1),
	m_style(style),
	m_lastFocusIndex(-1),
	m_totalHeight(0) {

	if (pos != wxDefaultPosition) SetPosition(pos);
	if (size != wxDefaultSize) SetSize(size);

	Bind(wxEVT_LEFT_DOWN, &NanoVGListBox::OnMouseDown, this);
	Bind(wxEVT_MOTION, &NanoVGListBox::OnMotion, this);
	Bind(wxEVT_LEAVE_WINDOW, &NanoVGListBox::OnLeave, this);
	Bind(wxEVT_KEY_DOWN, &NanoVGListBox::OnKeyDown, this);
	Bind(wxEVT_SIZE, &NanoVGListBox::OnSize, this);
	Bind(wxEVT_CHAR, &NanoVGListBox::OnChar, this);

	RecalculateHeights();
}

NanoVGListBox::~NanoVGListBox() {
	////
}

void NanoVGListBox::SetItemCount(size_t count) {
	if (count == m_itemCount) return;

	m_itemCount = count;
	m_selections.clear();
	m_lastFocusIndex = -1;
	m_hoverIndex = -1;

	RecalculateHeights();
	UpdateScrollbar(m_totalHeight);
	Refresh();
}

void NanoVGListBox::RecalculateHeights() {
	m_itemY.clear();
	m_itemY.reserve(m_itemCount + 1);

	int y = 0;
	m_itemY.push_back(y);
	for (size_t i = 0; i < m_itemCount; ++i) {
		int h = OnMeasureItem(i);
		y += h;
		m_itemY.push_back(y);
	}
	m_totalHeight = y;
	UpdateScrollbar(m_totalHeight);
}

int NanoVGListBox::GetSelection() const {
	if (m_selections.empty()) return wxNOT_FOUND;
	return *m_selections.begin();
}

bool NanoVGListBox::IsSelected(size_t n) const {
	return m_selections.find(n) != m_selections.end();
}

size_t NanoVGListBox::GetSelectedCount() const {
	return m_selections.size();
}

void NanoVGListBox::DeselectAll() {
	bool changed = !m_selections.empty();
	m_selections.clear();
	if (changed) Refresh();
}

bool NanoVGListBox::Select(size_t n, bool select) {
	if (n >= m_itemCount) return false;

	bool changed = false;
	if (select) {
		if (m_selections.find(n) == m_selections.end()) {
			m_selections.insert(n);
			changed = true;
		}
	} else {
		if (m_selections.erase(n) > 0) {
			changed = true;
		}
	}

	if (changed) {
		RefreshItem(n);
	}
	return changed;
}

bool NanoVGListBox::SetSelection(int n) {
	if (n == wxNOT_FOUND) {
		DeselectAll();
		return true;
	}
	if (n < 0 || (size_t)n >= m_itemCount) return false;

	bool changed = false;
	if (m_selections.size() != 1 || *m_selections.begin() != (size_t)n) {
		changed = true;
		m_selections.clear();
		m_selections.insert((size_t)n);
		m_lastFocusIndex = n;
		EnsureVisible((size_t)n);
		Refresh();
	}
	return changed;
}

void NanoVGListBox::RefreshItem(size_t n) {
	// For now, just refresh whole canvas as NanoVG redraws everything anyway
	Refresh();
}

void NanoVGListBox::RefreshAll() {
	Refresh();
}

void NanoVGListBox::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	int scrollPos = GetScrollPosition();

	// Find visible range
	// Since m_itemY is sorted, we can binary search or just linear search if few items
	// But binary search is O(log N) which is better.

	auto startIt = std::lower_bound(m_itemY.begin(), m_itemY.end(), scrollPos);
	size_t startIdx = std::distance(m_itemY.begin(), startIt);
	if (startIdx > 0 && m_itemY[startIdx] > scrollPos) startIdx--; // Adjust for partial visibility

	size_t endIdx = startIdx;
	int endY = scrollPos + height;
	while (endIdx < m_itemCount && m_itemY[endIdx] < endY) {
		endIdx++;
	}
	if (endIdx < m_itemCount) endIdx++; // Ensure last item is included

	// Background
	nvgBeginPath(vg);
	nvgRect(vg, 0, scrollPos, width, height);
	nvgFillColor(vg, nvgRGBAf(m_bgRed, m_bgGreen, m_bgBlue, 1.0f));
	nvgFill(vg);

	for (size_t i = startIdx; i < endIdx && i < m_itemCount; ++i) {
		int y = m_itemY[i];
		int h = m_itemY[i+1] - y;
		wxRect rect(0, y, width, h);

		OnDrawBackground(vg, rect, i);
		OnDrawItem(vg, rect, i);
	}
}

void NanoVGListBox::OnDrawBackground(NVGcontext* vg, const wxRect& rect, size_t n) const {
	bool selected = IsSelected(n);
	bool hovered = ((int)n == m_hoverIndex);

	if (selected || hovered) {
		nvgBeginPath(vg);
		nvgRect(vg, rect.x, rect.y, rect.width, rect.height);
		if (selected) {
			if (HasFocus()) {
				nvgFillColor(vg, nvgRGBA(0, 120, 215, 255)); // Standard selection blue
			} else {
				nvgFillColor(vg, nvgRGBA(80, 80, 80, 255)); // Unfocused selection grey
			}
		} else {
			nvgFillColor(vg, nvgRGBA(60, 60, 60, 255)); // Hover grey
		}
		nvgFill(vg);
	}
}

void NanoVGListBox::OnSize(wxSizeEvent& event) {
	RecalculateHeights();
	event.Skip();
}

int NanoVGListBox::HitTest(int x, int y) const {
	int scrollPos = GetScrollPosition();
	int realY = y + scrollPos;

	auto it = std::lower_bound(m_itemY.begin(), m_itemY.end(), realY);
	if (it == m_itemY.end()) {
		// Could be last item
		if (realY < m_totalHeight) return m_itemCount - 1;
		return wxNOT_FOUND;
	}

	size_t idx = std::distance(m_itemY.begin(), it);
	if (idx > 0 && m_itemY[idx] > realY) idx--;

	if (idx < m_itemCount) return (int)idx;
	return wxNOT_FOUND;
}

wxRect NanoVGListBox::GetItemRect(size_t n) const {
	if (n >= m_itemCount) return wxRect();
	return wxRect(0, m_itemY[n], GetClientSize().x, m_itemY[n+1] - m_itemY[n]);
}

void NanoVGListBox::EnsureVisible(size_t n) {
	if (n >= m_itemCount) return;

	int y = m_itemY[n];
	int h = m_itemY[n+1] - y;
	int scrollPos = GetScrollPosition();
	int clientHeight = GetClientSize().y;

	if (y < scrollPos) {
		SetScrollPosition(y);
	} else if (y + h > scrollPos + clientHeight) {
		SetScrollPosition(y + h - clientHeight);
	}
}

void NanoVGListBox::OnMouseDown(wxMouseEvent& event) {
	SetFocus();
	int index = HitTest(event.GetX(), event.GetY());
	if (index != wxNOT_FOUND) {
		int flags = 0;
		if (event.ControlDown()) flags |= 1;
		if (event.ShiftDown()) flags |= 2;
		HandleClick(index, flags);

		wxCommandEvent cmd(wxEVT_LISTBOX, GetId());
		cmd.SetEventObject(this);
		cmd.SetInt(index);
		GetEventHandler()->ProcessEvent(cmd);

		// Also handle double click via timer? No, wxMouseEvent has LeftDClick
	}
}

void NanoVGListBox::HandleClick(int index, int flags) {
	if (m_style & wxLB_MULTIPLE) {
		if (flags & 1) { // Ctrl
			Select(index, !IsSelected(index));
			m_lastFocusIndex = index;
		} else if (flags & 2) { // Shift
			if (m_lastFocusIndex != -1) {
				int start = std::min(m_lastFocusIndex, index);
				int end = std::max(m_lastFocusIndex, index);
				m_selections.clear();
				for (int i = start; i <= end; ++i) {
					m_selections.insert(i);
				}
			} else {
				m_selections.clear();
				m_selections.insert(index);
				m_lastFocusIndex = index;
			}
		} else {
			m_selections.clear();
			m_selections.insert(index);
			m_lastFocusIndex = index;
		}
	} else {
		// Single selection
		SetSelection(index);
	}
	Refresh();
}

void NanoVGListBox::OnMotion(wxMouseEvent& event) {
	int index = HitTest(event.GetX(), event.GetY());
	if (index != m_hoverIndex) {
		m_hoverIndex = index;
		Refresh();
	}
	event.Skip();
}

void NanoVGListBox::OnLeave(wxMouseEvent& event) {
	if (m_hoverIndex != -1) {
		m_hoverIndex = -1;
		Refresh();
	}
	event.Skip();
}

void NanoVGListBox::OnKeyDown(wxKeyEvent& event) {
	int selection = GetSelection();
	if (selection == wxNOT_FOUND && m_itemCount > 0) selection = 0;

	int newSelection = selection;
	int pageSize = 10; // Approx
	if (m_itemCount > 0) {
		pageSize = GetClientSize().y / (m_totalHeight / m_itemCount + 1);
	}

	switch (event.GetKeyCode()) {
		case WXK_UP:
			if (newSelection > 0) newSelection--;
			break;
		case WXK_DOWN:
			if (newSelection < (int)m_itemCount - 1) newSelection++;
			break;
		case WXK_HOME:
			newSelection = 0;
			break;
		case WXK_END:
			newSelection = m_itemCount - 1;
			break;
		case WXK_PAGEUP:
			newSelection = std::max(0, newSelection - pageSize);
			break;
		case WXK_PAGEDOWN:
			newSelection = std::min((int)m_itemCount - 1, newSelection + pageSize);
			break;
		default:
			event.Skip();
			return;
	}

	if (newSelection != selection || GetSelection() == wxNOT_FOUND) {
		SetSelection(newSelection);

		wxCommandEvent cmd(wxEVT_LISTBOX, GetId());
		cmd.SetEventObject(this);
		cmd.SetInt(newSelection);
		GetEventHandler()->ProcessEvent(cmd);
	}
}

void NanoVGListBox::OnChar(wxKeyEvent& event) {
	// Simple search/jump could be implemented here
	event.Skip();
}
