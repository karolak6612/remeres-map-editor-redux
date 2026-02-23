#include "ui/replace_tool/rule_list_control.h"
#include "ui/theme.h"
#include "util/image_manager.h"
#include <nanovg.h>
#include <memory>
#include <algorithm>
#include <ranges>

RuleListControl::RuleListControl(wxWindow* parent, Listener* listener) : NanoVGCanvas(parent, wxID_ANY, wxVSCROLL | wxNO_BORDER | wxWANTS_CHARS),
																		 m_listener(listener) {
	// NanoVGCanvas handles background style
	m_itemHeight = FromDIP(56);

	Bind(wxEVT_SIZE, &RuleListControl::OnSize, this);
	Bind(wxEVT_LEFT_DOWN, &RuleListControl::OnMouse, this);
	Bind(wxEVT_MOTION, &RuleListControl::OnMouse, this);
	Bind(wxEVT_LEAVE_WINDOW, &RuleListControl::OnMouse, this);
	Bind(wxEVT_MOUSEWHEEL, &RuleListControl::OnMouseWheel, this);
	Bind(wxEVT_CONTEXT_MENU, &RuleListControl::OnContextMenu, this);

	// Bind scroll events - NanoVGCanvas might handle some, but we keep our logic for now to ensure behavior
	Bind(wxEVT_SCROLLWIN_TOP, &RuleListControl::OnScroll, this);
	Bind(wxEVT_SCROLLWIN_BOTTOM, &RuleListControl::OnScroll, this);
	Bind(wxEVT_SCROLLWIN_LINEUP, &RuleListControl::OnScroll, this);
	Bind(wxEVT_SCROLLWIN_LINEDOWN, &RuleListControl::OnScroll, this);
	Bind(wxEVT_SCROLLWIN_PAGEUP, &RuleListControl::OnScroll, this);
	Bind(wxEVT_SCROLLWIN_PAGEDOWN, &RuleListControl::OnScroll, this);
	Bind(wxEVT_SCROLLWIN_THUMBTRACK, &RuleListControl::OnScroll, this);
	Bind(wxEVT_SCROLLWIN_THUMBRELEASE, &RuleListControl::OnScroll, this);
}

void RuleListControl::SetRuleSets(const std::vector<std::string>& ruleSetNames) {
	m_ruleSetNames = ruleSetNames;
	m_selectedIndex = -1;
	RefreshVirtualSize();
	Refresh();
}

void RuleListControl::RefreshVirtualSize() {
	wxSize clientSize = GetClientSize();
	int totalHeight = static_cast<int>(m_ruleSetNames.size()) * m_itemHeight;
	SetScrollbar(wxVERTICAL, GetScrollPos(wxVERTICAL), clientSize.y, totalHeight);
}

wxSize RuleListControl::DoGetBestClientSize() const {
	return wxSize(FromDIP(150), FromDIP(200));
}

void RuleListControl::OnSize(wxSizeEvent& event) {
	RefreshVirtualSize();
	Refresh();
	event.Skip();
}

void RuleListControl::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	int scrollPos = GetScrollPos(wxVERTICAL);
	// Assumption: NanoVGCanvas applies nvgTranslate(0, -scrollPos) automatically.
	// So we draw items at their absolute Y position.

	wxSize clientSize = GetClientSize();

	int startIdx = scrollPos / m_itemHeight;
	int count = static_cast<int>(m_ruleSetNames.size());
	int endIdx = std::min(count - 1, (scrollPos + clientSize.y) / m_itemHeight + 1);

	static const float padding = 4.0f;
	static const float radius = 4.0f;
	static const float FONT_SIZE = 16.0f; // 10pt equivalent, adjusted for better visual weight

	// Card Colors
	wxColour cardBase = Theme::Get(Theme::Role::CardBase);
	wxColour cardBaseHover = Theme::Get(Theme::Role::CardBaseHover);
	wxColour cardBaseSelected = Theme::Get(Theme::Role::Accent);
	wxColour textNormal = Theme::Get(Theme::Role::Text);
	wxColour textSelected = Theme::Get(Theme::Role::TextOnAccent);
	wxColour borderNormal = Theme::Get(Theme::Role::CardBorder);
	wxColour borderSelected = Theme::Get(Theme::Role::TextOnAccent);

	nvgFontSize(vg, FONT_SIZE);
	if (nvgFindFont(vg, "sans") != -1) {
		nvgFontFace(vg, "sans");
	}
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

	for (int i : std::views::iota(startIdx, endIdx + 1)) {
		if (i >= count) break;

		float y = i * m_itemHeight + padding;
		float x = padding;
		float w = width - 2 * padding;
		float h = m_itemHeight - 2 * padding;

		// Determine State
		bool isSelected = (i == m_selectedIndex);
		bool isHovered = (i == m_hoveredIndex);

		wxColour fillColor = cardBase;
		if (isSelected) {
			fillColor = cardBaseSelected;
		} else if (isHovered) {
			fillColor = cardBaseHover;
		}

		// Draw Shadow (Simple offset)
		if (!isSelected) {
			nvgBeginPath(vg);
			nvgRoundedRect(vg, x + 2, y + 2, w, h, radius);
			nvgFillColor(vg, nvgRGBA(0, 0, 0, 50));
			nvgFill(vg);
		}

		// Draw Card Body
		nvgBeginPath(vg);
		nvgRoundedRect(vg, x, y, w, h, radius);
		nvgFillColor(vg, nvgRGBA(fillColor.Red(), fillColor.Green(), fillColor.Blue(), fillColor.Alpha()));
		nvgFill(vg);

		// Border
		wxColour borderColor = isSelected ? borderSelected : borderNormal;
		nvgStrokeColor(vg, nvgRGBA(borderColor.Red(), borderColor.Green(), borderColor.Blue(), borderColor.Alpha()));
		nvgStrokeWidth(vg, 1.0f);
		nvgStroke(vg);

		// Text
		wxColour textColor = isSelected ? textSelected : textNormal;
		nvgFillColor(vg, nvgRGBA(textColor.Red(), textColor.Green(), textColor.Blue(), textColor.Alpha()));

		nvgText(vg, x + 10, y + h / 2.0f, m_ruleSetNames[i].c_str(), nullptr);
	}
}

wxRect RuleListControl::GetItemRect(int index) const {
	if (index < 0 || index >= static_cast<int>(m_ruleSetNames.size())) {
		return wxRect();
	}
	wxSize clientSize = GetClientSize();
	int scrollPos = GetScrollPos(wxVERTICAL);
	return wxRect(0, index * m_itemHeight - scrollPos, clientSize.x, m_itemHeight);
}

void RuleListControl::OnMouse(wxMouseEvent& event) {
	int oldHover = m_hoveredIndex;

	if (event.GetEventType() == wxEVT_LEAVE_WINDOW) {
		m_hoveredIndex = -1;
	} else {
		int scrollPos = GetScrollPos(wxVERTICAL);
		int y = event.GetY() + scrollPos;
		int idx = y / m_itemHeight;

		if (idx >= 0 && idx < static_cast<int>(m_ruleSetNames.size())) {
			m_hoveredIndex = idx;
		} else {
			m_hoveredIndex = -1;
		}

		if (event.LeftDown() && m_hoveredIndex != -1) {
			m_selectedIndex = m_hoveredIndex;
			if (m_listener) {
				m_listener->OnRuleSelected(RuleManager::Get().LoadRuleSet(m_ruleSetNames[m_selectedIndex]));
			}
		}
	}

	if (oldHover != m_hoveredIndex || event.LeftDown()) {
		Refresh();
	}
}

void RuleListControl::OnMouseWheel(wxMouseEvent& event) {
	int rotation = event.GetWheelRotation();
	int delta = (rotation / 120) * 3; // 3 items per notch
	int newPos = GetScrollPos(wxVERTICAL) - delta * m_itemHeight;

	int totalHeight = static_cast<int>(m_ruleSetNames.size()) * m_itemHeight;
	int maxScroll = std::max(0, totalHeight - GetClientSize().y);
	newPos = std::clamp(newPos, 0, maxScroll);

	SetScrollPos(wxVERTICAL, newPos);
	Refresh();
}

void RuleListControl::OnScroll(wxScrollWinEvent& event) {
	int pos = GetScrollPos(wxVERTICAL);
	int h = GetClientSize().y;
	int total = static_cast<int>(m_ruleSetNames.size()) * m_itemHeight;
	int maxScroll = std::max(0, total - h);

	wxEventType type = event.GetEventType();
	if (type == wxEVT_SCROLLWIN_TOP) {
		pos = 0;
	} else if (type == wxEVT_SCROLLWIN_BOTTOM) {
		pos = maxScroll;
	} else if (type == wxEVT_SCROLLWIN_LINEUP) {
		pos -= m_itemHeight;
	} else if (type == wxEVT_SCROLLWIN_LINEDOWN) {
		pos += m_itemHeight;
	} else if (type == wxEVT_SCROLLWIN_PAGEUP) {
		pos -= h;
	} else if (type == wxEVT_SCROLLWIN_PAGEDOWN) {
		pos += h;
	} else if (type == wxEVT_SCROLLWIN_THUMBTRACK || type == wxEVT_SCROLLWIN_THUMBRELEASE) {
		pos = event.GetPosition();
	}

	pos = std::clamp(pos, 0, maxScroll);
	SetScrollPos(wxVERTICAL, pos);
	Refresh();
}

void RuleListControl::OnContextMenu(wxContextMenuEvent& event) {
	int menuIdx = m_hoveredIndex;
	if (menuIdx == -1) {
		return;
	}

	wxMenu menu;
	menu.Append(wxID_EDIT, "Edit Name")->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_PEN_TO_SQUARE, wxSize(16, 16)));
	menu.Append(wxID_DELETE, "Delete")->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_TRASH_CAN, wxSize(16, 16)));

	menu.Bind(wxEVT_MENU, [this, menuIdx](wxCommandEvent& e) {
		if (menuIdx < 0 || menuIdx >= static_cast<int>(m_ruleSetNames.size())) {
			return;
		}

		std::string name = m_ruleSetNames[menuIdx];

		if (e.GetId() == wxID_EDIT) {
			wxString newName = wxGetTextFromUser("Enter new name for rule set:", "Edit Name", name);
			if (!newName.IsEmpty() && newName.ToStdString() != name) {
				if (m_listener) {
					m_listener->OnRuleRenamed(name, newName.ToStdString());
				}
			}
		} else if (e.GetId() == wxID_DELETE) {
			wxMessageDialog dlg(this, "Are you sure you want to delete rule set '" + name + "'?", "Delete Rule Set", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING);
			if (dlg.ShowModal() == wxID_YES) {
				if (m_listener) {
					m_listener->OnRuleDeleted(name);
				}
			}
		}
	});

	PopupMenu(&menu);
}
