#include "app/main.h"
#include "ui/controls/nanovg_listbox.h"
#include <algorithm>
#include <glad/glad.h>
#include <nanovg.h>
#include <wx/settings.h>
#include <wx/gdicmn.h>
#include <wx/event.h>

NanoVGListBox::NanoVGListBox(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style) :
	NanoVGCanvas(parent, id, style),
	m_itemCount(0),
	m_focusedItem(-1),
	m_totalHeight(0),
	m_multipleSelection((style & wxLB_MULTIPLE) || (style & wxLB_EXTENDED)) {

	Bind(wxEVT_LEFT_DOWN, &NanoVGListBox::OnLeftDown, this);
	Bind(wxEVT_LEFT_DCLICK, &NanoVGListBox::OnLeftDClick, this);
	Bind(wxEVT_KEY_DOWN, &NanoVGListBox::OnKeyDown, this);

	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX));
}

NanoVGListBox::~NanoVGListBox() {
}

void NanoVGListBox::SetItemCount(size_t count) {
	if (m_itemCount == count) {
		Refresh(); // Just refresh content if count is same
		return;
	}
	m_itemCount = count;
	m_selectedItems.clear();
	if (m_focusedItem >= (int)m_itemCount) {
		m_focusedItem = (int)m_itemCount - 1;
	}
	RecalculateContentHeight();
	Refresh();
}

void NanoVGListBox::SetSelection(int selection) {
	if (selection < 0 || selection >= (int)m_itemCount) {
		m_selectedItems.clear();
		m_focusedItem = -1;
	} else {
		m_selectedItems.clear();
		m_selectedItems.insert(selection);
		m_focusedItem = selection;
		EnsureVisible(selection);
	}
	Refresh();
}

int NanoVGListBox::GetSelection() const {
	if (m_selectedItems.empty()) {
		return wxNOT_FOUND;
	}
	// Return the first found (arbitrary order for hash set, but usually we want specific order?)
	// For single selection, it's fine.
	return *m_selectedItems.begin();
}

bool NanoVGListBox::IsSelected(int n) const {
	return m_selectedItems.find(n) != m_selectedItems.end();
}

void NanoVGListBox::Select(int n) {
	if (n >= 0 && n < (int)m_itemCount) {
		m_selectedItems.insert(n);
		Refresh();
	}
}

void NanoVGListBox::Deselect(int n) {
	m_selectedItems.erase(n);
	Refresh();
}

void NanoVGListBox::DeselectAll() {
	m_selectedItems.clear();
	Refresh();
}

void NanoVGListBox::ScrollToLine(int line) {
	if (line < 0 || line >= (int)m_itemCount) {
		return;
	}

	// Calculate Y position of line
	int y = 0;
	if (m_itemYCache.size() > (size_t)line) {
		y = m_itemYCache[line];
	} else {
		// Fallback or recalculate
		RecalculateContentHeight();
		if (m_itemYCache.size() > (size_t)line) {
			y = m_itemYCache[line];
		}
	}

	SetScrollPosition(y);
}

void NanoVGListBox::EnsureVisible(int line) {
	if (line < 0 || line >= (int)m_itemCount) {
		return;
	}

	int y = 0;
	int h = 0;

	if (m_itemYCache.size() > (size_t)line) {
		y = m_itemYCache[line];
		h = OnMeasureItem(line);
	} else {
		RecalculateContentHeight();
		if (m_itemYCache.size() > (size_t)line) {
			y = m_itemYCache[line];
			h = OnMeasureItem(line);
		}
	}

	int clientH = GetClientSize().y;
	int currentScroll = m_scrollPos;

	if (y < currentScroll) {
		SetScrollPosition(y);
	} else if (y + h > currentScroll + clientH) {
		SetScrollPosition(y + h - clientH);
	}
}

void NanoVGListBox::RecalculateContentHeight() {
	m_itemYCache.clear();
	m_itemYCache.reserve(m_itemCount);

	int y = 0;
	for (size_t i = 0; i < m_itemCount; ++i) {
		m_itemYCache.push_back(y);
		y += OnMeasureItem(i);
	}
	m_totalHeight = y;
	UpdateScrollbar(m_totalHeight);
}

int NanoVGListBox::HitTest(int y) const {
	// Binary search or simple scan?
	// m_itemYCache is sorted.
	// We want to find index i such that m_itemYCache[i] <= y < m_itemYCache[i+1]

	auto it = std::upper_bound(m_itemYCache.begin(), m_itemYCache.end(), y);
	if (it == m_itemYCache.begin()) {
		return -1;
	}
	int index = (int)std::distance(m_itemYCache.begin(), it) - 1;
	if (index >= 0 && index < (int)m_itemCount) {
		int itemY = m_itemYCache[index];
		int itemH = OnMeasureItem(index);
		if (y < itemY + itemH) {
			return index;
		}
	}
	return -1;
}

void NanoVGListBox::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	if (m_itemYCache.size() != m_itemCount) {
		RecalculateContentHeight();
	}

	// Calculate visible range
	int startY = m_scrollPos;
	int endY = m_scrollPos + height;

	// Find start index
	auto it = std::upper_bound(m_itemYCache.begin(), m_itemYCache.end(), startY);
	int startIndex = (it == m_itemYCache.begin()) ? 0 : (int)std::distance(m_itemYCache.begin(), it) - 1;

	for (size_t i = startIndex; i < m_itemCount; ++i) {
		int itemY = m_itemYCache[i];
		if (itemY >= endY) {
			break;
		}

		int itemH = OnMeasureItem(i);
		if (itemY + itemH > startY) {
			// Visible
			wxRect rect(0, itemY, width, itemH);

			// Selection background
			if (IsSelected(i)) {
				nvgBeginPath(vg);
				nvgRect(vg, 0, itemY, width, itemH);
				if (HasFocus()) {
					nvgFillColor(vg, nvgRGBA(51, 153, 255, 255)); // Standard Blue
				} else {
					nvgFillColor(vg, nvgRGBA(200, 200, 200, 255)); // Grey
				}
				nvgFill(vg);
			}

			OnDrawItem(vg, rect, i);
		}
	}
}

void NanoVGListBox::OnLeftDown(wxMouseEvent& event) {
	SetFocus();

	int y = event.GetY() + m_scrollPos;
	int index = HitTest(y);

	if (index != -1) {
		if (m_multipleSelection && event.ControlDown()) {
			if (IsSelected(index)) {
				Deselect(index);
			} else {
				Select(index);
			}
			m_focusedItem = index;
		} else if (m_multipleSelection && event.ShiftDown() && m_focusedItem != -1) {
			// Select range
			int start = std::min(m_focusedItem, index);
			int end = std::max(m_focusedItem, index);

			if (!event.ControlDown()) {
				DeselectAll();
			}

			for (int i = start; i <= end; ++i) {
				Select(i);
			}
		} else {
			// Single select or no modifier
			SetSelection(index);
		}

		// Fire event
		wxCommandEvent evt(wxEVT_LISTBOX, GetId());
		evt.SetEventObject(this);
		evt.SetInt(index);
		ProcessEvent(evt);
	}
}

void NanoVGListBox::OnLeftDClick(wxMouseEvent& event) {
	int y = event.GetY() + m_scrollPos;
	int index = HitTest(y);

	if (index != -1) {
		wxCommandEvent evt(wxEVT_LISTBOX_DCLICK, GetId());
		evt.SetEventObject(this);
		evt.SetInt(index);
		ProcessEvent(evt);
	}
}

void NanoVGListBox::OnKeyDown(wxKeyEvent& event) {
	int key = event.GetKeyCode();

	if (m_itemCount == 0) return;

	int nextItem = m_focusedItem;

	switch (key) {
		case WXK_UP:
			nextItem--;
			break;
		case WXK_DOWN:
			nextItem++;
			break;
		case WXK_HOME:
			nextItem = 0;
			break;
		case WXK_END:
			nextItem = (int)m_itemCount - 1;
			break;
		case WXK_PAGEUP: {
			int currentY = (m_itemYCache.size() > (size_t)m_focusedItem) ? m_itemYCache[m_focusedItem] : 0;
			int targetY = currentY - GetClientSize().y;
			int idx = HitTest(targetY);
			if (idx != -1) {
				nextItem = idx;
			} else {
				// If targetY < 0, HitTest returns -1.
				nextItem = 0;
			}
			break;
		}
		case WXK_PAGEDOWN: {
			int currentY = (m_itemYCache.size() > (size_t)m_focusedItem) ? m_itemYCache[m_focusedItem] : 0;
			int targetY = currentY + GetClientSize().y;
			int idx = HitTest(targetY);
			if (idx != -1) {
				nextItem = idx;
			} else {
				// If targetY > totalHeight, HitTest returns -1.
				nextItem = (int)m_itemCount - 1;
			}
			break;
		}
		default:
			event.Skip();
			return;
	}

	if (nextItem < 0) nextItem = 0;
	if (nextItem >= (int)m_itemCount) nextItem = (int)m_itemCount - 1;

	if (nextItem != m_focusedItem) {
		if (m_multipleSelection && event.ShiftDown()) {
			// Expand selection
			// Simplified: Just select target range
			// Ideally we track anchor.
			Select(nextItem);
			m_focusedItem = nextItem;
			EnsureVisible(nextItem);
		} else {
			SetSelection(nextItem);

			wxCommandEvent evt(wxEVT_LISTBOX, GetId());
			evt.SetEventObject(this);
			evt.SetInt(nextItem);
			ProcessEvent(evt);
		}
	}
}
