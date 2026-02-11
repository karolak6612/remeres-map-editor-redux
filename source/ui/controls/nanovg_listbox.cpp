#include "ui/controls/nanovg_listbox.h"
#include <algorithm>
#include <glad/glad.h>
#include <nanovg.h>
#include "app/main.h"
#include "ui/theme.h"

NanoVGListBox::NanoVGListBox(wxWindow* parent, wxWindowID id, long style) :
	NanoVGCanvas(parent, id, wxVSCROLL | wxWANTS_CHARS),
	m_multipleSelection(style & wxLB_MULTIPLE) {
	Bind(wxEVT_LEFT_DOWN, &NanoVGListBox::OnMouseDown, this);
	Bind(wxEVT_MOTION, &NanoVGListBox::OnMotion, this);
	Bind(wxEVT_KEY_DOWN, &NanoVGListBox::OnKeyDown, this);

	// Set dark theme default
	SetBackgroundColour(Theme::Get(Theme::Role::Background));
}

NanoVGListBox::~NanoVGListBox() {
	// NanoVGCanvas handles context cleanup
}

void NanoVGListBox::SetItemCount(int count) {
	m_count = std::max(0, count);
	UpdateScrollbar(m_count * m_rowHeight);
	Refresh();
}

void NanoVGListBox::SetRowHeight(int height) {
	if (m_rowHeight != height) {
		m_rowHeight = std::max(1, height);
		UpdateScrollbar(m_count * m_rowHeight);
		Refresh();
	}
}

void NanoVGListBox::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	if (m_count <= 0) return;

	int scrollPos = GetScrollPosition();
	int startIdx = scrollPos / m_rowHeight;
	int endIdx = (scrollPos + height + m_rowHeight - 1) / m_rowHeight;

	startIdx = std::max(0, startIdx);
	endIdx = std::min(m_count, endIdx + 1); // Ensure we cover partial items

	for (int i = startIdx; i < endIdx; ++i) {
		wxRect rect(0, i * m_rowHeight, width, m_rowHeight);
		OnDrawItem(vg, rect, i);
	}
}

wxSize NanoVGListBox::DoGetBestClientSize() const {
	return FromDIP(wxSize(200, 300));
}

int NanoVGListBox::HitTest(int y) const {
	int scrollPos = GetScrollPosition();
	int row = (y + scrollPos) / m_rowHeight;
	if (row >= 0 && row < m_count) {
		return row;
	}
	return -1;
}

void NanoVGListBox::OnMouseDown(wxMouseEvent& event) {
	SetFocus();
	int index = HitTest(event.GetY());

	if (index == -1) {
		if (!event.ControlDown() && !event.ShiftDown()) {
			DeselectAll();
		}
		return;
	}

	if (m_multipleSelection) {
		if (event.ControlDown()) {
			Toggle(index);
			m_anchorIndex = index;
		} else if (event.ShiftDown() && m_anchorIndex != -1) {
			DeselectAll();
			SelectRange(std::min(m_anchorIndex, index), std::max(m_anchorIndex, index), true);
		} else {
			DeselectAll();
			Select(index, true);
			m_anchorIndex = index;
		}
	} else {
		SetSelection(index);
	}
}

void NanoVGListBox::OnMotion(wxMouseEvent& event) {
	int index = HitTest(event.GetY());
	if (index != m_hoverIndex) {
		m_hoverIndex = index;
		Refresh();
	}
	event.Skip();
}

void NanoVGListBox::OnKeyDown(wxKeyEvent& event) {
	int navIndex = GetSelection();
	if (navIndex == -1) navIndex = 0;

	bool update = false;
	int newIndex = navIndex;
	int viewH = GetClientSize().y;
	int itemsPerPage = std::max(1, viewH / m_rowHeight);

	switch (event.GetKeyCode()) {
		case WXK_UP:
			newIndex = std::max(0, navIndex - 1);
			update = true;
			break;
		case WXK_DOWN:
			newIndex = std::min(m_count - 1, navIndex + 1);
			update = true;
			break;
		case WXK_PAGEUP: {
			newIndex = std::max(0, navIndex - itemsPerPage);
			update = true;
			break;
		}
		case WXK_PAGEDOWN: {
			newIndex = std::min(m_count - 1, navIndex + itemsPerPage);
			update = true;
			break;
		}
		case WXK_HOME:
			newIndex = 0;
			update = true;
			break;
		case WXK_END:
			newIndex = m_count - 1;
			update = true;
			break;
		default:
			event.Skip();
			return;
	}

	if (update) {
		newIndex = std::clamp(newIndex, 0, m_count - 1);

		if (m_multipleSelection && event.ShiftDown()) {
			if (m_anchorIndex == -1) m_anchorIndex = navIndex;
			DeselectAll();
			SelectRange(std::min(m_anchorIndex, newIndex), std::max(m_anchorIndex, newIndex), true);
		} else {
			SetSelection(newIndex);
			m_anchorIndex = newIndex;
		}
		EnsureVisible(newIndex);
	}
}

void NanoVGListBox::EnsureVisible(int index) {
	int itemY = index * m_rowHeight;
	int itemH = m_rowHeight;
	int scrollPos = GetScrollPosition();
	int viewH = GetClientSize().y;

	if (itemY < scrollPos) {
		SetScrollPosition(itemY);
	} else if (itemY + itemH > scrollPos + viewH) {
		SetScrollPosition(itemY + itemH - viewH);
	}
}

void NanoVGListBox::SetSelection(int index) {
	DeselectAll();
	Select(index, true);
	m_anchorIndex = index;
}

void NanoVGListBox::Select(int index, bool select) {
	if (index < 0 || index >= m_count) return;

	if (select) {
		if (m_selections.find(index) == m_selections.end()) {
			m_selections.insert(index);
			Refresh();
			OnSelectionChanged();

			wxCommandEvent evt(wxEVT_LISTBOX, GetId());
			evt.SetEventObject(this);
			evt.SetInt(index);
			GetEventHandler()->ProcessEvent(evt);
		}
	} else {
		if (m_selections.erase(index) > 0) {
			Refresh();
			OnSelectionChanged();
		}
	}
}

void NanoVGListBox::SelectRange(int start, int end, bool select) {
	if (start > end) std::swap(start, end);
	start = std::max(0, start);
	end = std::min(m_count - 1, end);

	bool changed = false;
	for (int i = start; i <= end; ++i) {
		if (select) {
			if (m_selections.find(i) == m_selections.end()) {
				m_selections.insert(i);
				changed = true;
			}
		} else {
			if (m_selections.erase(i) > 0) {
				changed = true;
			}
		}
	}

	if (changed) {
		Refresh();
		OnSelectionChanged();
		if (m_anchorIndex != -1) {
			wxCommandEvent evt(wxEVT_LISTBOX, GetId());
			evt.SetEventObject(this);
			evt.SetInt(m_anchorIndex);
			GetEventHandler()->ProcessEvent(evt);
		}
	}
}

void NanoVGListBox::Toggle(int index) {
	if (IsSelected(index)) {
		Select(index, false);
	} else {
		Select(index, true);
	}
}

void NanoVGListBox::DeselectAll() {
	if (!m_selections.empty()) {
		m_selections.clear();
		Refresh();
		OnSelectionChanged();
	}
}

bool NanoVGListBox::IsSelected(int index) const {
	return m_selections.find(index) != m_selections.end();
}

int NanoVGListBox::GetSelection() const {
	if (m_selections.empty()) return -1;
	int minIdx = m_count + 1;
	for (int idx : m_selections) {
		if (idx < minIdx) minIdx = idx;
	}
	return (minIdx <= m_count) ? minIdx : -1;
}

int NanoVGListBox::GetSelectedCount() const {
	return static_cast<int>(m_selections.size());
}

std::vector<int> NanoVGListBox::GetSelections() const {
	std::vector<int> sels(m_selections.begin(), m_selections.end());
	std::sort(sels.begin(), sels.end());
	return sels;
}
