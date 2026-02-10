#include "ui/controls/nanovg_listbox.h"
#include "app/main.h"
#include <wx/wx.h>
#include <algorithm>

NanoVGListBox::NanoVGListBox(wxWindow* parent, wxWindowID id, long style) :
	NanoVGCanvas(parent, id, style | wxWANTS_CHARS), // Ensure we get key events
	m_rowHeight(FROM_DIP(parent, 32)),
	m_fixedHeight(true),
	m_multipleSelection((style & wxLB_MULTIPLE) || (style & wxLB_EXTENDED))
{
	Bind(wxEVT_LEFT_DOWN, &NanoVGListBox::OnMouse, this);
	Bind(wxEVT_LEFT_DCLICK, &NanoVGListBox::OnMouse, this);
	Bind(wxEVT_KEY_DOWN, &NanoVGListBox::OnKey, this);
}

NanoVGListBox::~NanoVGListBox() {}

void NanoVGListBox::SetItemCount(size_t count) {
	m_itemCount = count;
	UpdateScrollbar(static_cast<int>(m_itemCount * m_rowHeight));
	Refresh();
}

void NanoVGListBox::SetSelection(int index) {
	if (!m_multipleSelection) {
		m_selectedIndices.clear();
	}
	if (index >= 0 && index < static_cast<int>(m_itemCount)) {
		m_selectedIndices.insert(index);
		m_focusedIndex = index;
		m_anchorIndex = index;
	}
	Refresh();
	OnSelectionChanged();
}

void NanoVGListBox::SetFocusItem(int index) {
	if (index >= 0 && index < static_cast<int>(m_itemCount)) {
		m_focusedIndex = index;
		Refresh();
	}
}

int NanoVGListBox::GetSelection() const {
	if (m_selectedIndices.empty()) {
		return wxNOT_FOUND;
	}
	// For single selection, returning the first one is correct.
	// For multiple, it returns *one* of them.
	return *m_selectedIndices.begin();
}

bool NanoVGListBox::IsSelected(int index) const {
	return m_selectedIndices.find(index) != m_selectedIndices.end();
}

void NanoVGListBox::Select(int index, bool select) {
	bool changed = false;
	if (select) {
		if (m_selectedIndices.find(index) == m_selectedIndices.end()) {
			m_selectedIndices.insert(index);
			changed = true;
		}
	} else {
		if (m_selectedIndices.find(index) != m_selectedIndices.end()) {
			m_selectedIndices.erase(index);
			changed = true;
		}
	}

	if (changed) {
		Refresh();
		OnSelectionChanged();
	}
}

void NanoVGListBox::DeselectAll() {
	if (!m_selectedIndices.empty()) {
		m_selectedIndices.clear();
		Refresh();
		OnSelectionChanged();
	}
}

size_t NanoVGListBox::GetSelectedCount() const {
	return m_selectedIndices.size();
}

void NanoVGListBox::ScrollTo(int index) {
	if (index < 0 || index >= static_cast<int>(m_itemCount)) return;

	int y = index * m_rowHeight;
	int h = GetClientSize().y;
	int scrollPos = GetScrollPosition();

	if (y < scrollPos) {
		SetScrollPosition(y);
	} else if (y + m_rowHeight > scrollPos + h) {
		SetScrollPosition(y + m_rowHeight - h);
	}
}

void NanoVGListBox::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	if (m_itemCount == 0) return;

	int scrollPos = GetScrollPosition();
	int startIdx = scrollPos / m_rowHeight;
	int endIdx = (scrollPos + height) / m_rowHeight + 1;

	startIdx = std::max(0, startIdx);
	endIdx = std::min(endIdx, static_cast<int>(m_itemCount));

	for (int i = startIdx; i < endIdx; ++i) {
		wxRect rect(0, i * m_rowHeight, width, m_rowHeight);
		bool selected = IsSelected(i);

		// Draw selection background
		if (selected) {
			nvgBeginPath(vg);
			nvgRect(vg, rect.x, rect.y, rect.width, rect.height);
			nvgFillColor(vg, nvgRGBA(0, 120, 215, 255)); // Standard blue selection
			nvgFill(vg);
		}

		// Draw focus rect
		if (i == m_focusedIndex) {
			nvgBeginPath(vg);
			nvgRect(vg, rect.x + 0.5f, rect.y + 0.5f, rect.width - 1.0f, rect.height - 1.0f);
			nvgStrokeColor(vg, nvgRGBA(200, 200, 200, 128));
			nvgStroke(vg);
		}

		OnDrawItem(vg, i, rect, selected);
	}
}

void NanoVGListBox::OnMouse(wxMouseEvent& evt) {
	SetFocus();
	int y = evt.GetPosition().y + GetScrollPosition();
	int idx = y / m_rowHeight;

	if (idx >= 0 && idx < static_cast<int>(m_itemCount)) {
		m_focusedIndex = idx; // Always update focus on click

		if (m_multipleSelection) {
			if (evt.ControlDown()) {
				// Toggle
				Select(idx, !IsSelected(idx));
				m_anchorIndex = idx;
			} else if (evt.ShiftDown()) {
				// Range selection
				if (m_anchorIndex == wxNOT_FOUND) m_anchorIndex = idx;

				DeselectAll();
				int start = std::min(m_anchorIndex, idx);
				int end = std::max(m_anchorIndex, idx);
				for (int i = start; i <= end; ++i) {
					Select(i, true);
				}
			} else {
				// Normal click: Select one, clear others
				DeselectAll();
				Select(idx, true);
				m_anchorIndex = idx;
			}
		} else {
			// Single selection
			DeselectAll(); // Clear previous
			Select(idx, true);
			m_anchorIndex = idx;
		}

		Refresh();

		// Fire selection event
		wxCommandEvent event(wxEVT_LISTBOX, GetId());
		event.SetEventObject(this);
		event.SetInt(idx);
		ProcessEvent(event);

		if (evt.LeftDClick()) {
			wxCommandEvent dclickEvent(wxEVT_LISTBOX_DCLICK, GetId());
			dclickEvent.SetEventObject(this);
			dclickEvent.SetInt(idx);
			ProcessEvent(dclickEvent);
		}
	}
}

void NanoVGListBox::OnKey(wxKeyEvent& evt) {
	int sel = m_focusedIndex;
	if (sel == wxNOT_FOUND && GetItemCount() > 0) {
		sel = 0;
	}

	int newSel = sel;

	switch (evt.GetKeyCode()) {
		case WXK_UP:
			newSel = std::max(0, sel - 1);
			break;
		case WXK_DOWN:
			newSel = std::min(static_cast<int>(m_itemCount) - 1, sel + 1);
			break;
		case WXK_PAGEUP: {
			int visibleItems = GetClientSize().y / m_rowHeight;
			newSel = std::max(0, sel - visibleItems);
			break;
		}
		case WXK_PAGEDOWN: {
			int visibleItems = GetClientSize().y / m_rowHeight;
			newSel = std::min(static_cast<int>(m_itemCount) - 1, sel + visibleItems);
			break;
		}
		case WXK_HOME:
			newSel = 0;
			break;
		case WXK_END:
			newSel = static_cast<int>(m_itemCount) - 1;
			break;
		case WXK_SPACE:
			if (sel != wxNOT_FOUND) {
				if (m_multipleSelection) {
					// Toggle selection of focused item
					Select(sel, !IsSelected(sel));
					m_anchorIndex = sel;
				} else {
					// Single selection: Space selects focused item
					SetSelection(sel);
				}

				// Fire event
				wxCommandEvent event(wxEVT_LISTBOX, GetId());
				event.SetEventObject(this);
				event.SetInt(sel);
				ProcessEvent(event);
			}
			return; // Handled
		default:
			evt.Skip();
			return;
	}

	if (newSel != sel && newSel >= 0) {
		SetFocusItem(newSel);
		ScrollTo(newSel);

		if (m_multipleSelection) {
			if (evt.ShiftDown()) {
				if (m_anchorIndex == wxNOT_FOUND) m_anchorIndex = sel; // Start from old focus

				DeselectAll();
				int start = std::min(m_anchorIndex, newSel);
				int end = std::max(m_anchorIndex, newSel);
				for (int i = start; i <= end; ++i) {
					Select(i, true);
				}
			} else if (!evt.ControlDown()) {
				// Standard navigation: select focused
				DeselectAll();
				Select(newSel, true);
				m_anchorIndex = newSel;
			}
			// If ControlDown, we move focus but don't change selection
		} else {
			// Single selection
			DeselectAll();
			Select(newSel, true);
			m_anchorIndex = newSel;
		}

		wxCommandEvent event(wxEVT_LISTBOX, GetId());
		event.SetEventObject(this);
		event.SetInt(newSel);
		ProcessEvent(event);
	}
}

void NanoVGListBox::SetRowHeight(int height) {
	m_rowHeight = height;
	UpdateScrollbar(static_cast<int>(m_itemCount * m_rowHeight));
	Refresh();
}
