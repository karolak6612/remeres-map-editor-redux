#include "app/main.h"
#include "ui/controls/nanovg_listbox.h"
#include <nanovg.h>
#include <spdlog/spdlog.h>

NanoVGListBox::NanoVGListBox(wxWindow* parent, wxWindowID id, long style) :
	NanoVGCanvas(parent, id, style | wxVSCROLL | wxWANTS_CHARS),
	m_itemCount(0),
	m_selection(-1),
	m_hover(-1),
	m_multipleSelection(style & wxLB_MULTIPLE),
	m_itemHeight(32),
	m_animTimer(this) {

	Bind(wxEVT_LEFT_DOWN, &NanoVGListBox::OnMouseDown, this);
	Bind(wxEVT_LEFT_DCLICK, &NanoVGListBox::OnLeftDClick, this);
	Bind(wxEVT_MOTION, &NanoVGListBox::OnMotion, this);
	Bind(wxEVT_LEAVE_WINDOW, &NanoVGListBox::OnLeave, this);
	Bind(wxEVT_SIZE, &NanoVGListBox::OnSize, this);
	Bind(wxEVT_TIMER, &NanoVGListBox::OnTimer, this);
}

NanoVGListBox::~NanoVGListBox() {
	m_animTimer.Stop();
}

void NanoVGListBox::SetItemCount(int count) {
	if (m_itemCount != count) {
		m_itemCount = count;
		if (m_selection >= m_itemCount) {
			m_selection = -1;
		}
		// Prune selections
		if (m_multipleSelection) {
			auto it = m_selections.begin();
			while (it != m_selections.end()) {
				if (*it >= m_itemCount) {
					it = m_selections.erase(it);
				} else {
					++it;
				}
			}
		}
		UpdateLayout();
		Refresh();
	}
}

void NanoVGListBox::SetSelection(int index) {
	if (index < -1 || index >= m_itemCount) {
		index = -1;
	}

	if (m_multipleSelection) {
		m_selections.clear();
		if (index != -1) {
			m_selections.insert(index);
		}
		m_selection = index;
		Refresh();

		wxCommandEvent event(wxEVT_LISTBOX, GetId());
		event.SetEventObject(this);
		event.SetInt(m_selection);
		ProcessEvent(event);
	} else {
		if (m_selection != index) {
			m_selection = index;
			Refresh();

			wxCommandEvent event(wxEVT_LISTBOX, GetId());
			event.SetEventObject(this);
			event.SetInt(m_selection);
			ProcessEvent(event);
		}
	}
}

bool NanoVGListBox::IsSelected(int index) const {
	if (m_multipleSelection) {
		return m_selections.count(index) > 0;
	}
	return m_selection == index;
}

void NanoVGListBox::Select(int index, bool select) {
	if (index < 0 || index >= m_itemCount) return;

	if (m_multipleSelection) {
		if (select) m_selections.insert(index);
		else m_selections.erase(index);
		m_selection = index; // Last modified becomes 'primary'?
		Refresh();

		wxCommandEvent event(wxEVT_LISTBOX, GetId());
		event.SetEventObject(this);
		event.SetInt(index);
		ProcessEvent(event);
	} else {
		if (select) SetSelection(index);
		else if (m_selection == index) SetSelection(-1);
	}
}

void NanoVGListBox::DeselectAll() {
	if (m_multipleSelection) {
		m_selections.clear();
	}
	m_selection = -1;
	Refresh();
}

int NanoVGListBox::GetSelectedCount() const {
	if (m_multipleSelection) return (int)m_selections.size();
	return (m_selection != -1) ? 1 : 0;
}

void NanoVGListBox::SetItemHeight(int height) {
	if (m_itemHeight != height) {
		m_itemHeight = height;
		UpdateLayout();
		Refresh();
	}
}

void NanoVGListBox::UpdateLayout() {
	// Simple fixed height calculation
	int totalHeight = m_itemCount * m_itemHeight;
	UpdateScrollbar(totalHeight);
}

wxRect NanoVGListBox::GetItemRect(int index) const {
	int w = GetClientSize().x;
	return wxRect(0, index * m_itemHeight, w, m_itemHeight);
}

int NanoVGListBox::HitTest(int y) const {
	int scrollPos = GetScrollPosition();
	int realY = y + scrollPos;
	int index = realY / m_itemHeight;

	if (index >= 0 && index < m_itemCount) {
		return index;
	}
	return -1;
}

void NanoVGListBox::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	int scrollPos = GetScrollPosition();

	// Start drawing
	int startIdx = scrollPos / m_itemHeight;
	int endIdx = (scrollPos + height + m_itemHeight - 1) / m_itemHeight + 1;

	if (startIdx < 0) startIdx = 0;
	if (endIdx > m_itemCount) endIdx = m_itemCount;

	for (int i = startIdx; i < endIdx; ++i) {
		wxRect rect = GetItemRect(i);

		// Background for selection/hover
		if (IsSelected(i)) {
			nvgBeginPath(vg);
			nvgRect(vg, rect.x, rect.y, rect.width, rect.height);
			nvgFillColor(vg, nvgRGBA(80, 120, 180, 255)); // Blueish selection
			nvgFill(vg);
		} else if (i == m_hover) {
			nvgBeginPath(vg);
			nvgRect(vg, rect.x, rect.y, rect.width, rect.height);
			nvgFillColor(vg, nvgRGBA(60, 60, 65, 255)); // Hover highlight
			nvgFill(vg);
		}

		// Draw content
		nvgSave(vg);
		OnDrawItem(vg, i, rect);
		nvgRestore(vg);
	}
}

void NanoVGListBox::OnMouseDown(wxMouseEvent& event) {
	int index = HitTest(event.GetY());
	if (index != -1) {
		if (m_multipleSelection) {
			bool selected = IsSelected(index);
			Select(index, !selected);
		} else {
			if (index != m_selection) {
				SetSelection(index);
			}
		}
	}
	event.Skip();
}

void NanoVGListBox::OnLeftDClick(wxMouseEvent& event) {
	int index = HitTest(event.GetY());
	if (index != -1) {
		wxCommandEvent evt(wxEVT_LISTBOX_DCLICK, GetId());
		evt.SetEventObject(this);
		evt.SetInt(index);
		ProcessEvent(evt);
	}
	event.Skip();
}

void NanoVGListBox::OnMotion(wxMouseEvent& event) {
	int index = HitTest(event.GetY());
	if (index != m_hover) {
		m_hover = index;
		Refresh();
	}
	event.Skip();
}

void NanoVGListBox::OnLeave(wxMouseEvent& event) {
	if (m_hover != -1) {
		m_hover = -1;
		Refresh();
	}
	event.Skip();
}

void NanoVGListBox::OnSize(wxSizeEvent& event) {
	UpdateLayout();
	Refresh();
	event.Skip();
}

void NanoVGListBox::OnTimer(wxTimerEvent& event) {
	// Animation if needed
}
