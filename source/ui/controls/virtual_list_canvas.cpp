#include "ui/controls/virtual_list_canvas.h"
#include "ui/theme.h"
#include "rendering/core/graphics.h"
#include "util/image_manager.h"
#include "rendering/core/text_renderer.h"
#include <wx/settings.h>
#include <algorithm>

VirtualListCanvas::VirtualListCanvas(wxWindow* parent, wxWindowID id, long style) :
	NanoVGCanvas(parent, id, style | wxVSCROLL | wxWANTS_CHARS),
	m_itemHeight(32),
	m_selectionMode(SelectionMode::Single),
	m_anchorIndex(-1),
	m_hoverIndex(-1),
	m_focusedIndex(-1) {

	m_itemHeight = FromDIP(32);

	// Initialize theme colors
	wxColour selColor = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
	m_selectionColor = nvgRGBA(selColor.Red(), selColor.Green(), selColor.Blue(), 128); // 50% opacity

	wxColour hoverColor = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT).ChangeLightness(150);
	m_hoverColor = nvgRGBA(hoverColor.Red(), hoverColor.Green(), hoverColor.Blue(), 64); // 25% opacity

	wxColour textColor = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT);
	m_textColor = nvgRGBA(textColor.Red(), textColor.Green(), textColor.Blue(), 255);

	Bind(wxEVT_LEFT_DOWN, &VirtualListCanvas::OnMouseDown, this);
	Bind(wxEVT_LEFT_UP, &VirtualListCanvas::OnMouseUp, this);
	Bind(wxEVT_MOTION, &VirtualListCanvas::OnMotion, this);
	Bind(wxEVT_LEAVE_WINDOW, &VirtualListCanvas::OnLeave, this);
	Bind(wxEVT_KEY_DOWN, &VirtualListCanvas::OnKeyDown, this);
	Bind(wxEVT_CHAR, &VirtualListCanvas::OnChar, this);
	Bind(wxEVT_SIZE, &VirtualListCanvas::OnSize, this);

	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

VirtualListCanvas::~VirtualListCanvas() {
}

void VirtualListCanvas::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	int count = (int)GetItemCount();
	if (count == 0) {
		return;
	}

	int scrollY = GetScrollPosition();
	int startIdx = std::max(0, scrollY / m_itemHeight);
	int endIdx = std::min(count, (scrollY + height + m_itemHeight - 1) / m_itemHeight);

	nvgScissor(vg, 0, 0, width, height);

	for (int i = startIdx; i < endIdx; ++i) {
		int y = i * m_itemHeight - scrollY;
		wxRect rect(0, y, width, m_itemHeight);

		// Selection
		if (IsSelected(i)) {
			nvgBeginPath(vg);
			nvgRect(vg, rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
			nvgFillColor(vg, m_selectionColor);
			nvgFill(vg);
		} else if (i == m_hoverIndex) {
			nvgBeginPath(vg);
			nvgRect(vg, rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
			nvgFillColor(vg, m_hoverColor);
			nvgFill(vg);
		}

		// Draw Item Content
		OnDrawItem(vg, i, rect);
	}

	nvgResetScissor(vg);
}

void VirtualListCanvas::SetSelectionMode(SelectionMode mode) {
	m_selectionMode = mode;
	if (mode == SelectionMode::Single && m_selectedIndices.size() > 1) {
		int first = *m_selectedIndices.begin();
		m_selectedIndices.clear();
		m_selectedIndices.insert(first);
		Refresh();
	}
}

void VirtualListCanvas::SetSelection(int index) {
	if (index < 0 || index >= (int)GetItemCount()) {
		DeselectAll();
		return;
	}

	if (m_selectionMode == SelectionMode::Single) {
		m_selectedIndices.clear();
		m_selectedIndices.insert(index);
	} else {
		if (m_selectedIndices.find(index) == m_selectedIndices.end()) {
			m_selectedIndices.insert(index);
		}
	}
	m_focusedIndex = index;
	m_anchorIndex = index;
	EnsureVisible(index);
	Refresh();
	SendSelectionEvent();
}

void VirtualListCanvas::Select(int index, bool select) {
	if (index < 0 || index >= (int)GetItemCount()) return;

	if (select) {
		if (m_selectionMode == SelectionMode::Single) {
			m_selectedIndices.clear();
		}
		m_selectedIndices.insert(index);
	} else {
		m_selectedIndices.erase(index);
	}
	Refresh();
}

void VirtualListCanvas::Deselect(int index) {
	Select(index, false);
}

void VirtualListCanvas::SelectAll() {
	if (m_selectionMode == SelectionMode::Multiple) {
		for (int i = 0; i < (int)GetItemCount(); ++i) {
			m_selectedIndices.insert(i);
		}
		Refresh();
		SendSelectionEvent();
	}
}

void VirtualListCanvas::DeselectAll() {
	m_selectedIndices.clear();
	Refresh();
	SendSelectionEvent();
}

bool VirtualListCanvas::IsSelected(int index) const {
	return m_selectedIndices.find(index) != m_selectedIndices.end();
}

int VirtualListCanvas::GetSelection() const {
	if (m_selectedIndices.empty()) return wxNOT_FOUND;
	return *m_selectedIndices.begin();
}

size_t VirtualListCanvas::GetSelectedCount() const {
	return m_selectedIndices.size();
}

std::vector<int> VirtualListCanvas::GetSelections() const {
	std::vector<int> sels(m_selectedIndices.begin(), m_selectedIndices.end());
	return sels;
}

void VirtualListCanvas::SetItemHeight(int height) {
	m_itemHeight = FromDIP(height); // Ensure scaled
	RefreshList();
}

int VirtualListCanvas::GetItemHeight(int index) const {
	return m_itemHeight; // Simplified for now
}

void VirtualListCanvas::RefreshList() {
	int count = (int)GetItemCount();
	int totalHeight = count * m_itemHeight;
	UpdateScrollbar(totalHeight);
	Refresh();
}

void VirtualListCanvas::EnsureVisible(int index) {
	if (index < 0 || index >= (int)GetItemCount()) return;

	int itemTop = index * m_itemHeight;
	int itemBottom = itemTop + m_itemHeight;
	int viewTop = GetScrollPosition();
	int viewBottom = viewTop + GetClientSize().GetHeight();

	if (itemTop < viewTop) {
		SetScrollPosition(itemTop);
	} else if (itemBottom > viewBottom) {
		SetScrollPosition(itemBottom - GetClientSize().GetHeight());
	}
}

void VirtualListCanvas::SendSelectionEvent() {
	wxCommandEvent event(wxEVT_LISTBOX, GetId());
	event.SetEventObject(this);
	event.SetInt(GetSelection());
	GetEventHandler()->ProcessEvent(event);
}

void VirtualListCanvas::OnMouseDown(wxMouseEvent& event) {
	SetFocus();
	int index = HitTest(event.GetY());
	if (index == wxNOT_FOUND) {
		DeselectAll();
		return;
	}

	if (m_selectionMode == SelectionMode::Multiple) {
		if (event.ShiftDown() && m_anchorIndex != -1) {
			RangeSelect(m_anchorIndex, index);
		} else if (event.ControlDown()) {
			if (IsSelected(index)) {
				Deselect(index);
			} else {
				Select(index, true);
				m_anchorIndex = index;
			}
		} else {
			SetSelection(index);
		}
	} else {
		SetSelection(index);
	}
	m_focusedIndex = index;
}

void VirtualListCanvas::OnMouseUp(wxMouseEvent& event) {
	// Optional: Handle drag end
}

void VirtualListCanvas::OnMotion(wxMouseEvent& event) {
	int index = HitTest(event.GetY());
	if (index != m_hoverIndex) {
		m_hoverIndex = index;
		Refresh();
	}
}

void VirtualListCanvas::OnLeave(wxMouseEvent& event) {
	if (m_hoverIndex != -1) {
		m_hoverIndex = -1;
		Refresh();
	}
}

void VirtualListCanvas::OnKeyDown(wxKeyEvent& event) {
	int count = (int)GetItemCount();
	if (count == 0) {
		event.Skip();
		return;
	}

	int nextIndex = m_focusedIndex;
	int pageSize = GetClientSize().GetHeight() / m_itemHeight;

	switch (event.GetKeyCode()) {
		case WXK_UP:
			nextIndex = std::max(0, m_focusedIndex - 1);
			break;
		case WXK_DOWN:
			nextIndex = std::min(count - 1, m_focusedIndex + 1);
			break;
		case WXK_PAGEUP:
			nextIndex = std::max(0, m_focusedIndex - pageSize);
			break;
		case WXK_PAGEDOWN:
			nextIndex = std::min(count - 1, m_focusedIndex + pageSize);
			break;
		case WXK_HOME:
			nextIndex = 0;
			break;
		case WXK_END:
			nextIndex = count - 1;
			break;
		default:
			event.Skip();
			return;
	}

	if (nextIndex != m_focusedIndex) {
		if (m_selectionMode == SelectionMode::Multiple && event.ShiftDown()) {
			if (m_anchorIndex == -1) m_anchorIndex = m_focusedIndex;
			RangeSelect(m_anchorIndex, nextIndex);
			m_focusedIndex = nextIndex;
			EnsureVisible(nextIndex);
			Refresh();
			SendSelectionEvent();
		} else {
			SetSelection(nextIndex);
		}
	}
}

void VirtualListCanvas::OnChar(wxKeyEvent& event) {
	// Optional: Type-to-search or other shortcuts
	event.Skip();
}

void VirtualListCanvas::OnSize(wxSizeEvent& event) {
	RefreshList(); // Update scrollbar
	event.Skip();
}

wxSize VirtualListCanvas::DoGetBestClientSize() const {
	return FromDIP(wxSize(200, 300));
}

int VirtualListCanvas::HitTest(int y) const {
	int scrollY = GetScrollPosition();
	int index = (scrollY + y) / m_itemHeight;
	if (index >= 0 && index < (int)GetItemCount()) {
		return index;
	}
	return wxNOT_FOUND;
}

void VirtualListCanvas::RangeSelect(int anchor, int current) {
	m_selectedIndices.clear();
	int start = std::min(anchor, current);
	int end = std::max(anchor, current);
	for (int i = start; i <= end; ++i) {
		m_selectedIndices.insert(i);
	}
	Refresh();
	SendSelectionEvent();
}
