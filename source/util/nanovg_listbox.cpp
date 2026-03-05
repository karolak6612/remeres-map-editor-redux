#include "util/nanovg_listbox.h"
#include "ui/theme.h"
#include <wx/vlbox.h> // For styles like wxLB_MULTIPLE
#include <glad/glad.h>
#include <nanovg.h>
#include <nanovg_gl.h>
#include <algorithm>
#include <iterator>

NanoVGListBox::NanoVGListBox(wxWindow* parent, wxWindowID id, long style) :
	NanoVGCanvas(parent, id, wxVSCROLL | wxWANTS_CHARS),
	m_count(0),
	m_style(style),
	m_hoverIndex(-1),
	m_focusIndex(-1) {

	Bind(wxEVT_SIZE, &NanoVGListBox::OnSize, this);
	Bind(wxEVT_LEFT_DOWN, &NanoVGListBox::OnMouseDown, this);
	Bind(wxEVT_KEY_DOWN, &NanoVGListBox::OnKeyDown, this);
	Bind(wxEVT_MOTION, &NanoVGListBox::OnMotion, this);
	Bind(wxEVT_LEAVE_WINDOW, &NanoVGListBox::OnLeave, this);
}

NanoVGListBox::~NanoVGListBox() {
}

void NanoVGListBox::SetGridMode(bool grid_mode, int item_width, int item_height) {
	m_gridMode = grid_mode;
	m_gridItemWidth = item_width;
	m_gridItemHeight = item_height;
	UpdateScrollbar();
	Refresh();
}

void NanoVGListBox::SetItemCount(size_t count) {
	if (m_count != count) {
		m_count = count;
		if (m_style & wxLB_MULTIPLE) {
			m_multiSelection.resize(m_count, false);
		} else {
			if (m_selection >= static_cast<int>(m_count)) {
				m_selection = -1;
			}
		}
		if (m_focusIndex >= static_cast<int>(m_count)) {
			m_focusIndex = -1;
		}
		UpdateScrollbar();
		Refresh();
	}
}

void NanoVGListBox::SetSelection(int index) {
	if (index < -1 || index >= static_cast<int>(m_count)) {
		return;
	}

	if (m_style & wxLB_MULTIPLE) {
		if (index == -1) {
			std::fill(m_multiSelection.begin(), m_multiSelection.end(), false);
		} else {
			// In wxLB_MULTIPLE, SetSelection selects and deselects others?
			// Usually SetSelection in single selection clears selection.
			// In multiple selection, it might be ambiguous. Let's assume single selection behavior for compatibility.
			std::fill(m_multiSelection.begin(), m_multiSelection.end(), false);
			m_multiSelection[index] = true;
		}
	} else {
		m_selection = index;
	}
	m_focusIndex = index;
	Refresh();
}

int NanoVGListBox::GetSelection() const {
	if (m_style & wxLB_MULTIPLE) {
		const auto it = std::find(m_multiSelection.begin(), m_multiSelection.end(), true);
		if (it != m_multiSelection.end()) {
			return (int)std::distance(m_multiSelection.begin(), it);
		}
		return -1;
	}
	return m_selection;
}

bool NanoVGListBox::IsSelected(int index) const {
	if (index < 0 || index >= static_cast<int>(m_count)) {
		return false;
	}

	if (m_style & wxLB_MULTIPLE) {
		return m_multiSelection[index];
	}
	return m_selection == index;
}

void NanoVGListBox::Select(int index, bool select) {
	if (index < 0 || index >= static_cast<int>(m_count)) {
		return;
	}

	if (m_style & wxLB_MULTIPLE) {
		if (m_multiSelection[index] != select) {
			m_multiSelection[index] = select;
			Refresh();
		}
	} else {
		if (select) {
			m_selection = index;
			m_focusIndex = index; // Sync focus index in single selection mode
		} else if (m_selection == index) {
			m_selection = -1;
			m_focusIndex = -1;
		}
		Refresh();
	}
}

int NanoVGListBox::GetSelectedCount() const {
	if (m_style & wxLB_MULTIPLE) {
		return std::count(m_multiSelection.begin(), m_multiSelection.end(), true);
	}
	return (m_selection != -1) ? 1 : 0;
}

void NanoVGListBox::ClearSelection() {
	if (m_style & wxLB_MULTIPLE) {
		std::fill(m_multiSelection.begin(), m_multiSelection.end(), false);
	} else {
		m_selection = -1;
	}
	m_focusIndex = -1;
	Refresh();
}

void NanoVGListBox::RefreshItem(size_t index) {
	Refresh(); // Ideally, invalidate only rect, but NanoVG usually redraws full frame
}

void NanoVGListBox::RefreshAll() {
	Refresh();
}

void NanoVGListBox::ScrollToLine(int line) {
	EnsureVisible(line);
}

void NanoVGListBox::EnsureVisible(int line) {
	if (line < 0 || line >= static_cast<int>(m_count)) {
		return;
	}

	int itemTop, itemBottom;
	int clientWidth = GetClientSize().x;

	if (m_gridMode && clientWidth > 0 && m_gridItemWidth > 0) {
		int cols = std::max(1, clientWidth / m_gridItemWidth);
		int row = line / cols;
		itemTop = row * m_gridItemHeight;
		itemBottom = itemTop + m_gridItemHeight;
	} else {
		int itemHeight = std::max(1, OnMeasureItem(0));
		itemTop = line * itemHeight;
		itemBottom = itemTop + itemHeight;
	}

	int scrollPos = GetScrollPosition();
	int clientHeight = GetClientSize().y;

	if (itemTop < scrollPos) {
		SetScrollPosition(itemTop);
		Refresh();
	} else if (itemBottom > scrollPos + clientHeight) {
		SetScrollPosition(itemBottom - clientHeight);
		Refresh();
	}
}

void NanoVGListBox::UpdateScrollbar() {
	if (m_count == 0) {
		SetScrollbar(wxVERTICAL, 0, 0, 0);
		return;
	}

	int totalHeight = 0;
	int clientWidth = GetClientSize().x;

	if (m_gridMode && clientWidth > 0 && m_gridItemWidth > 0) {
		int cols = std::max(1, clientWidth / m_gridItemWidth);
		int rows = (static_cast<int>(m_count) + cols - 1) / cols;
		totalHeight = rows * m_gridItemHeight;
	} else {
		int itemHeight = std::max(1, OnMeasureItem(0));
		totalHeight = static_cast<int>(m_count) * itemHeight;
	}

	NanoVGCanvas::UpdateScrollbar(totalHeight);
}

void NanoVGListBox::OnSize(wxSizeEvent& event) {
	UpdateScrollbar();
	Refresh();
	event.Skip();
}

wxSize NanoVGListBox::DoGetBestClientSize() const {
	return FromDIP(wxSize(200, 300));
}

void NanoVGListBox::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	if (m_count == 0) {
		return;
	}

	int scrollPos = GetScrollPosition();

	if (m_gridMode) {
		int cols = std::max(1, width / m_gridItemWidth);
		int startRow = scrollPos / m_gridItemHeight;
		int endRow = (scrollPos + height + m_gridItemHeight - 1) / m_gridItemHeight;

		startRow = std::max(0, startRow);

		int startIndex = startRow * cols;
		int endIndex = std::min(static_cast<int>(m_count), endRow * cols);

		for (int i = startIndex; i < endIndex; ++i) {
			int row = i / cols;
			int col = i % cols;
			wxRect rect(col * m_gridItemWidth, row * m_gridItemHeight, m_gridItemWidth, m_gridItemHeight);

			bool selected = IsSelected(i);
			bool hover = (i == m_hoverIndex);
			bool focused = (i == m_focusIndex);

			if (selected || hover || focused) {
				nvgBeginPath(vg);
				nvgRoundedRect(vg, rect.x + 2, rect.y + 2, rect.width - 4, rect.height - 4, 4.0f);
				wxColour c;
				if (selected) {
					c = Theme::Get(Theme::Role::Accent);
				} else if (focused) {
					c = Theme::Get(Theme::Role::Selected);
				} else {
					c = Theme::Get(Theme::Role::CardBaseHover);
				}
				nvgFillColor(vg, nvgRGBA(c.Red(), c.Green(), c.Blue(), c.Alpha()));
				nvgFill(vg);
			}

			OnDrawItem(vg, rect, i);
		}
	} else {
		int itemHeight = std::max(1, OnMeasureItem(0));

		int startRow = scrollPos / itemHeight;
		int endRow = (scrollPos + height + itemHeight - 1) / itemHeight;

		startRow = std::max(0, startRow);
		endRow = std::min(static_cast<int>(m_count), endRow);

		for (int i = startRow; i < endRow; ++i) {
			wxRect rect(0, i * itemHeight, width, itemHeight);

			bool selected = IsSelected(i);
			bool hover = (i == m_hoverIndex);
			bool focused = (i == m_focusIndex);

			if (selected || hover || focused) {
				nvgBeginPath(vg);
				nvgRect(vg, rect.x, rect.y, rect.width, rect.height);
				wxColour c;
				if (selected) {
					c = Theme::Get(Theme::Role::Accent);
				} else if (focused) {
					c = Theme::Get(Theme::Role::Selected);
				} else {
					c = Theme::Get(Theme::Role::CardBaseHover);
				}
				nvgFillColor(vg, nvgRGBA(c.Red(), c.Green(), c.Blue(), c.Alpha()));
				nvgFill(vg);
			}

			OnDrawItem(vg, rect, i);
		}
	}
}

int NanoVGListBox::HitTest(int x, int y) const {
	if (m_count == 0) {
		return -1;
	}

	if (m_gridMode) {
		int clientWidth = GetClientSize().x;
		int cols = std::max(1, clientWidth / m_gridItemWidth);

		int col = x / m_gridItemWidth;
		if (col >= cols) return -1; // Clicked in empty space on the right

		int row = y / m_gridItemHeight;
		int index = row * cols + col;

		if (index >= 0 && index < static_cast<int>(m_count)) {
			return index;
		}
	} else {
		int itemHeight = std::max(1, OnMeasureItem(0));
		int index = y / itemHeight;
		if (index >= 0 && index < static_cast<int>(m_count)) {
			return index;
		}
	}
	return -1;
}

wxRect NanoVGListBox::GetItemRect(int index) const {
	if (m_gridMode) {
		int clientWidth = GetClientSize().x;
		int cols = std::max(1, clientWidth / m_gridItemWidth);
		int row = index / cols;
		int col = index % cols;
		return wxRect(col * m_gridItemWidth, row * m_gridItemHeight, m_gridItemWidth, m_gridItemHeight);
	} else {
		int itemHeight = std::max(1, OnMeasureItem(0));
		return wxRect(0, index * itemHeight, GetClientSize().x, itemHeight);
	}
}

void NanoVGListBox::OnMouseDown(wxMouseEvent& event) {
	int scrollPos = GetScrollPosition();
	int index = HitTest(event.GetX(), event.GetY() + scrollPos);

	if (index != -1) {
		if (m_style & wxLB_MULTIPLE) {
			if (m_multiSelection.size() != m_count) {
				m_multiSelection.resize(m_count, false);
			}
			if (event.ControlDown()) {
				Select(index, !IsSelected(index));
			} else if (event.ShiftDown()) {
				// Implement shift-selection later if needed
				// For now, just select the item
				Select(index, true);
			} else {
				ClearSelection();
				Select(index, true);
			}
			m_focusIndex = index; // Set focus to the clicked item
		} else {
			Select(index, true);
		}
		SendSelectionEvent();
		Refresh();
	}
	SetFocus(); // Ensure we get key events
}

void NanoVGListBox::OnMotion(wxMouseEvent& event) {
	int scrollPos = GetScrollPosition();
	int index = HitTest(event.GetX(), event.GetY() + scrollPos);

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
}

void NanoVGListBox::OnKeyDown(wxKeyEvent& event) {
	if (m_count == 0) {
		return;
	}

	int current = (m_style & wxLB_MULTIPLE) ? m_focusIndex : m_selection;
	if (current == -1 && m_count > 0) {
		current = 0; // If no item is selected/focused, start from the first item
	}
	int next = current;

	switch (event.GetKeyCode()) {
		case WXK_UP:
			if (m_gridMode) {
				int cols = std::max(1, GetClientSize().x / m_gridItemWidth);
				next = std::max(0, current - cols);
			} else {
				next = std::max(0, current - 1);
			}
			break;
		case WXK_DOWN:
			if (m_gridMode) {
				int cols = std::max(1, GetClientSize().x / m_gridItemWidth);
				next = std::min(static_cast<int>(m_count) - 1, current + cols);
			} else {
				next = std::min(static_cast<int>(m_count) - 1, current + 1);
			}
			break;
		case WXK_LEFT:
			if (m_gridMode) {
				next = std::max(0, current - 1);
			}
			break;
		case WXK_RIGHT:
			if (m_gridMode) {
				next = std::min(static_cast<int>(m_count) - 1, current + 1);
			}
			break;
		case WXK_HOME:
			next = 0;
			break;
		case WXK_END:
			next = static_cast<int>(m_count) - 1;
			break;
		case WXK_PAGEUP: {
			if (m_gridMode) {
				int cols = std::max(1, GetClientSize().x / m_gridItemWidth);
				int rows = GetClientSize().y / m_gridItemHeight;
				next = std::max(0, current - (cols * rows));
			} else {
				int itemHeight = OnMeasureItem(0);
				int pageSize = GetClientSize().y / itemHeight;
				next = std::max(0, current - pageSize);
			}
			break;
		}
		case WXK_PAGEDOWN: {
			if (m_gridMode) {
				int cols = std::max(1, GetClientSize().x / m_gridItemWidth);
				int rows = GetClientSize().y / m_gridItemHeight;
				next = std::min(static_cast<int>(m_count) - 1, current + (cols * rows));
			} else {
				int itemHeight = OnMeasureItem(0);
				int pageSize = GetClientSize().y / itemHeight;
				next = std::min(static_cast<int>(m_count) - 1, current + pageSize);
			}
			break;
		}
		default:
			event.Skip();
			return;
	}

	if (next != current || (m_style & wxLB_SINGLE && m_selection == -1)) {
		if (m_style & wxLB_MULTIPLE) {
			// Move selection or extend? For simple replacement, behave like single selection movement but keep multiple selection?
			// Standard behavior: Arrow keys move focus, Space selects.
			// But for now, let's make it simple: Arrows move single selection if single style.
			// For multiple, we really need a 'focus' index separate from selection.
			// Let's implement simple 'radio' behavior for single selection.
			// For multiple selection, we just scroll/move focus but don't change selection unless specific logic.
			// Implementing full listbox behavior is hard.
			// Let's stick to single selection logic which covers most use cases here.
			// If it's multiple selection, we might just scroll to it?
		} else {
			m_selection = next;
			SendSelectionEvent();
		}
		EnsureVisible(next);
		Refresh();
	}
}

void NanoVGListBox::SendSelectionEvent() {
	wxCommandEvent event(wxEVT_LISTBOX, GetId());
	event.SetEventObject(this);
	event.SetInt(GetSelection());
	ProcessWindowEvent(event);
}
