#include "util/nanovg_listbox.h"
#include <wx/wx.h>
#include <nanovg.h>
#include <algorithm>

NanoVGListBox::NanoVGListBox(wxWindow* parent, wxWindowID id, long style) :
	NanoVGCanvas(parent, id, wxVSCROLL | wxWANTS_CHARS),
	m_count(0),
	m_style(style),
	m_totalHeight(0),
	m_lastSelection(-1) {
	Bind(wxEVT_LEFT_DOWN, &NanoVGListBox::OnLeftDown, this);
	Bind(wxEVT_LEFT_DCLICK, &NanoVGListBox::OnLeftDClick, this);
	Bind(wxEVT_KEY_DOWN, &NanoVGListBox::OnKeyDown, this);
}

NanoVGListBox::~NanoVGListBox() {
	//
}

void NanoVGListBox::SetItemCount(int count) {
	m_count = count;
	RecalculateLayout();
	m_selections.clear();
	m_lastSelection = -1;
	Refresh();
}

void NanoVGListBox::RecalculateLayout() {
	m_itemY.clear();
	m_itemY.reserve(m_count + 1);
	m_totalHeight = 0;
	for (int i = 0; i < m_count; ++i) {
		m_itemY.push_back(m_totalHeight);
		m_totalHeight += OnMeasureItem(i);
	}
	m_itemY.push_back(m_totalHeight);
	UpdateScrollbar(m_totalHeight);
}

void NanoVGListBox::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	int startY = GetScrollPosition();
	int endY = startY + height;

	// Find first visible item
	auto it = std::lower_bound(m_itemY.begin(), m_itemY.end(), startY);
	int first = std::distance(m_itemY.begin(), it);
	if (first > 0 && m_itemY[first] > startY) {
		first--;
	}

	for (int i = first; i < m_count; ++i) {
		int y = m_itemY[i];
		if (y > endY) {
			break;
		}

		int h = m_itemY[i + 1] - y;
		wxRect rect(0, y, width, h);

		// Draw selection background
		if (IsSelected(i)) {
			nvgBeginPath(vg);
			nvgRect(vg, 0, y, width, h);
			nvgFillColor(vg, nvgRGBA(50, 100, 200, 255)); // Blueish selection
			nvgFill(vg);
		}

		OnDrawItem(vg, rect, i);
	}
}

int NanoVGListBox::OnMeasureItem(int index) const {
	return 20;
}

int NanoVGListBox::HitTest(const wxPoint& point) const {
	int y = point.y + GetScrollPosition();
	auto it = std::upper_bound(m_itemY.begin(), m_itemY.end(), y);
	int index = std::distance(m_itemY.begin(), it) - 1;
	if (index >= 0 && index < m_count) {
		return index;
	}
	return wxNOT_FOUND;
}

void NanoVGListBox::EnsureVisible(int n) {
	if (n < 0 || n >= m_count) {
		return;
	}

	int y = m_itemY[n];
	int h = m_itemY[n + 1] - y;
	int viewY = GetScrollPosition();
	int viewH = GetClientSize().y;

	if (y < viewY) {
		SetScrollPosition(y);
	} else if (y + h > viewY + viewH) {
		SetScrollPosition(y + h - viewH);
	}
}

void NanoVGListBox::SetSelection(int n) {
	if (n < 0 || n >= m_count) {
		return;
	}
	m_selections.clear();
	m_selections.insert(n);
	m_lastSelection = n;
	EnsureVisible(n);
	Refresh();

	wxCommandEvent event(wxEVT_LISTBOX, GetId());
	event.SetEventObject(this);
	event.SetInt(n);
	GetEventHandler()->ProcessEvent(event);
}

int NanoVGListBox::GetSelection() const {
	if (m_selections.empty()) {
		return wxNOT_FOUND;
	}
	return *m_selections.begin(); // Not deterministic if multiple, but consistent with single
}

bool NanoVGListBox::IsSelected(int n) const {
	return m_selections.count(n) > 0;
}

void NanoVGListBox::Select(int n) {
	if (n < 0 || n >= m_count) {
		return;
	}
	if ((m_style & wxLB_MULTIPLE) || (m_style & wxLB_EXTENDED)) {
		m_selections.insert(n);
	} else {
		m_selections.clear();
		m_selections.insert(n);
	}
	m_lastSelection = n;
	Refresh();
}

void NanoVGListBox::Deselect(int n) {
	m_selections.erase(n);
	Refresh();
}

void NanoVGListBox::DeselectAll() {
	m_selections.clear();
	Refresh();
}

void NanoVGListBox::OnLeftDown(wxMouseEvent& event) {
	SetFocus();
	int index = HitTest(event.GetPosition());
	if (index == wxNOT_FOUND) {
		return;
	}

	if ((m_style & wxLB_MULTIPLE) || (m_style & wxLB_EXTENDED)) {
		if (event.ControlDown()) {
			if (IsSelected(index)) {
				Deselect(index);
			} else {
				Select(index);
			}
		} else if (event.ShiftDown() && m_lastSelection != -1) {
			int start = std::min(m_lastSelection, index);
			int end = std::max(m_lastSelection, index);
			if (!event.ControlDown()) {
				m_selections.clear();
			}
			for (int i = start; i <= end; ++i) {
				m_selections.insert(i);
			}
		} else {
			// Click without modifiers in multiple selection listbox usually toggles (simple multiple)
			// or selects only one (extended).
			// wxLB_MULTIPLE: Toggles the item at the position of the click.
			// wxLB_EXTENDED: Selects the item... Ctrl toggles...
			if (m_style & wxLB_MULTIPLE) {
				if (IsSelected(index)) {
					Deselect(index);
				} else {
					Select(index);
				}
			} else {
				// Extended: Click selects only one
				m_selections.clear();
				Select(index);
			}
		}
	} else {
		// Single selection
		SetSelection(index);
	}

	m_lastSelection = index;
	Refresh();

	wxCommandEvent evt(wxEVT_LISTBOX, GetId());
	evt.SetEventObject(this);
	evt.SetInt(index);
	GetEventHandler()->ProcessEvent(evt);

	// Double click handled by parent?
	// We need to forward double click?
	// NanoVGCanvas receives double click?
	// Check Bind in NanoVGCanvas or NanoVGListBox?
	// We bind LEFT_DOWN.
}

void NanoVGListBox::OnLeftDClick(wxMouseEvent& event) {
	int index = HitTest(event.GetPosition());
	if (index != wxNOT_FOUND) {
		wxCommandEvent evt(wxEVT_LISTBOX_DCLICK, GetId());
		evt.SetEventObject(this);
		evt.SetInt(index);
		GetEventHandler()->ProcessEvent(evt);
	}
}

void NanoVGListBox::OnKeyDown(wxKeyEvent& event) {
	int sel = GetSelection();
	if (sel == wxNOT_FOUND) {
		sel = 0;
	}

	switch (event.GetKeyCode()) {
		case WXK_UP:
			if (sel > 0) {
				SetSelection(sel - 1);
			}
			break;
		case WXK_DOWN:
			if (sel < m_count - 1) {
				SetSelection(sel + 1);
			}
			break;
		case WXK_HOME:
			if (m_count > 0) {
				SetSelection(0);
			}
			break;
		case WXK_END:
			if (m_count > 0) {
				SetSelection(m_count - 1);
			}
			break;
		case WXK_PAGEUP:
			if (m_count > 0) {
				int visibleCount = GetClientSize().y / 20; // Approx
				SetSelection(std::max(0, sel - visibleCount));
			}
			break;
		case WXK_PAGEDOWN:
			if (m_count > 0) {
				int visibleCount = GetClientSize().y / 20; // Approx
				SetSelection(std::min(m_count - 1, sel + visibleCount));
			}
			break;
		default:
			event.Skip();
	}
}
