#include "app/main.h"
#include "ui/controls/nanovg_listbox.h"
#include "ui/theme.h"
#include <nanovg.h>
#include <wx/settings.h>
#include <algorithm>

NanoVGListBox::NanoVGListBox(wxWindow* parent, wxWindowID id, long style) :
	NanoVGCanvas(parent, id, style | wxVSCROLL | wxWANTS_CHARS),
	m_itemCount(0),
	m_itemHeight(24),
	m_multipleSelection((style & wxLB_MULTIPLE) || (style & wxLB_EXTENDED)) {

	m_itemHeight = FromDIP(24);

	Bind(wxEVT_LEFT_DOWN, &NanoVGListBox::OnLeftDown, this);
	Bind(wxEVT_LEFT_UP, &NanoVGListBox::OnLeftUp, this);
	Bind(wxEVT_MOTION, &NanoVGListBox::OnMotion, this);
	Bind(wxEVT_KEY_DOWN, &NanoVGListBox::OnKeyDown, this);
	Bind(wxEVT_SIZE, &NanoVGListBox::OnSize, this);
}

NanoVGListBox::~NanoVGListBox() {
}

void NanoVGListBox::SetItemCount(size_t count) {
	m_itemCount = count;
	UpdateScrollbar(static_cast<int>(m_itemCount * m_itemHeight));
	Refresh();
}

void NanoVGListBox::SetItemHeight(int height) {
	m_itemHeight = height;
	UpdateScrollbar(static_cast<int>(m_itemCount * m_itemHeight));
	Refresh();
}

void NanoVGListBox::SetSelection(int n) {
	if (n < 0 || n >= static_cast<int>(m_itemCount)) {
		return;
	}

	if (!m_multipleSelection) {
		for (int sel : m_selections) {
			OnSelectionChanged(sel, false);
		}
		m_selections.clear();
	}

	if (m_selections.find(n) == m_selections.end()) {
		m_selections.insert(n);
		OnSelectionChanged(n, true);
	}

	EnsureVisible(n);
	Refresh();
}

bool NanoVGListBox::IsSelected(int n) const {
	return m_selections.find(n) != m_selections.end();
}

void NanoVGListBox::Select(int n) {
	if (n >= 0 && n < static_cast<int>(m_itemCount)) {
		if (m_selections.find(n) == m_selections.end()) {
			m_selections.insert(n);
			OnSelectionChanged(n, true);
			Refresh();
		}
	}
}

void NanoVGListBox::Deselect(int n) {
	if (m_selections.erase(n) > 0) {
		OnSelectionChanged(n, false);
		Refresh();
	}
}

void NanoVGListBox::DeselectAll() {
	std::set<int> oldSelections = m_selections;
	m_selections.clear();
	for (int n : oldSelections) {
		OnSelectionChanged(n, false);
	}
	Refresh();
}

void NanoVGListBox::SelectAll() {
	if (m_multipleSelection) {
		for (size_t i = 0; i < m_itemCount; ++i) {
			int n = static_cast<int>(i);
			if (m_selections.find(n) == m_selections.end()) {
				m_selections.insert(n);
				OnSelectionChanged(n, true);
			}
		}
		Refresh();
	}
}

int NanoVGListBox::GetSelection() const {
	if (m_selections.empty()) {
		return -1;
	}
	return *m_selections.begin();
}

int NanoVGListBox::GetSelectedCount() const {
	return static_cast<int>(m_selections.size());
}

void NanoVGListBox::EnsureVisible(int n) {
	if (n < 0 || n >= static_cast<int>(m_itemCount)) {
		return;
	}

	int itemTop = n * m_itemHeight;
	int itemBottom = itemTop + m_itemHeight;
	int scrollTop = GetScrollPosition();
	int scrollBottom = scrollTop + GetClientSize().y;

	if (itemTop < scrollTop) {
		SetScrollPosition(itemTop);
	} else if (itemBottom > scrollBottom) {
		SetScrollPosition(itemBottom - GetClientSize().y);
	}
}

int NanoVGListBox::OnMeasureItem(size_t n) const {
	return m_itemHeight;
}

void NanoVGListBox::OnSelectionChanged(int n, bool selected) {
	// Default implementation does nothing
}

void NanoVGListBox::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	int scrollY = GetScrollPosition();

	// Background
	// Coordinate system is shifted by -scrollPos (scrolled content).
	// Viewport visible area is [0, scrollPos, width, height] in translated coordinates?
	// No, translation is y -= scrollPos.
	// So drawing at y=0 is content top.
	// The viewport top in content space is scrollPos.
	// So to fill the viewport background, we draw rect at y=scrollPos.

	nvgBeginPath(vg);
	nvgRect(vg, 0, scrollY, width, height);
	wxColour bg = Theme::Get(Theme::Role::Background);
	nvgFillColor(vg, nvgRGBA(bg.Red(), bg.Green(), bg.Blue(), 255));
	nvgFill(vg);

	// Calculate visible range
	int firstItem = scrollY / m_itemHeight;
	int lastItem = (scrollY + height) / m_itemHeight + 1;

	firstItem = std::max(0, firstItem);
	lastItem = std::min(static_cast<int>(m_itemCount), lastItem);

	for (int i = firstItem; i < lastItem; ++i) {
		wxRect rect(0, i * m_itemHeight, width, m_itemHeight);

		// Draw selection background
		if (IsSelected(i)) {
			nvgBeginPath(vg);
			nvgRect(vg, rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
			wxColour selColor = Theme::Get(Theme::Role::Selected);
			nvgFillColor(vg, nvgRGBA(selColor.Red(), selColor.Green(), selColor.Blue(), selColor.Alpha()));
			nvgFill(vg);
		}

		// Draw item content
		OnDrawItem(vg, rect, i);
	}
}

void NanoVGListBox::OnLeftDown(wxMouseEvent& event) {
	int index = HitTest(event.GetPosition());
	if (index != -1) {
		if (m_multipleSelection && event.ControlDown()) {
			if (IsSelected(index)) {
				Deselect(index);
			} else {
				Select(index);
			}
		} else if (m_multipleSelection && event.ShiftDown()) {
			int last = GetSelection();
			if (last != -1) {
				int start = std::min(last, index);
				int end = std::max(last, index);
				if (!event.ControlDown()) {
					DeselectAll();
				}
				for (int i = start; i <= end; ++i) {
					Select(i);
				}
			} else {
				SetSelection(index);
			}
		} else {
			SetSelection(index);
		}

		wxCommandEvent evt(wxEVT_LISTBOX, GetId());
		evt.SetEventObject(this);
		evt.SetInt(index);
		GetEventHandler()->ProcessEvent(evt);
	}
	SetFocus();
}

void NanoVGListBox::OnLeftUp(wxMouseEvent& event) {
	event.Skip();
}

void NanoVGListBox::OnMotion(wxMouseEvent& event) {
	event.Skip();
}

void NanoVGListBox::OnKeyDown(wxKeyEvent& event) {
	if (m_itemCount == 0) {
		event.Skip();
		return;
	}

	int selection = GetSelection();
	int newSelection = selection;
	if (newSelection == -1) newSelection = 0;

	switch (event.GetKeyCode()) {
		case WXK_UP:
			newSelection--;
			break;
		case WXK_DOWN:
			newSelection++;
			break;
		case WXK_HOME:
			newSelection = 0;
			break;
		case WXK_END:
			newSelection = static_cast<int>(m_itemCount) - 1;
			break;
		case WXK_PAGEUP:
			newSelection -= (GetClientSize().y / m_itemHeight);
			break;
		case WXK_PAGEDOWN:
			newSelection += (GetClientSize().y / m_itemHeight);
			break;
		default:
			event.Skip();
			return;
	}

	newSelection = std::clamp(newSelection, 0, static_cast<int>(m_itemCount) - 1);

	if (newSelection != selection) {
		SetSelection(newSelection);
		wxCommandEvent evt(wxEVT_LISTBOX, GetId());
		evt.SetEventObject(this);
		evt.SetInt(newSelection);
		GetEventHandler()->ProcessEvent(evt);
	}
}

void NanoVGListBox::OnSize(wxSizeEvent& event) {
	UpdateScrollbar(static_cast<int>(m_itemCount * m_itemHeight));
	Refresh();
	event.Skip();
}

int NanoVGListBox::HitTest(const wxPoint& pt) const {
	int y = pt.y + GetScrollPosition();
	int index = y / m_itemHeight;
	if (index >= 0 && index < static_cast<int>(m_itemCount)) {
		return index;
	}
	return -1;
}
