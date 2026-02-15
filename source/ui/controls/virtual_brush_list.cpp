#include "ui/controls/virtual_brush_list.h"
#include "brushes/brush.h"
#include "ui/gui.h"
#include "util/image_manager.h"
#include <nanovg.h>
#include <algorithm>

VirtualBrushList::VirtualBrushList(wxWindow* parent, wxWindowID id) :
	NanoVGCanvas(parent, id, wxVSCROLL | wxWANTS_CHARS),
	cleared(false),
	no_matches(false),
	selected_index(-1),
	hover_index(-1),
	item_height(32) {

	Bind(wxEVT_LEFT_DOWN, &VirtualBrushList::OnMouseDown, this);
	Bind(wxEVT_LEFT_DCLICK, &VirtualBrushList::OnMouseDoubleClick, this);
	Bind(wxEVT_SIZE, &VirtualBrushList::OnSize, this);
	Bind(wxEVT_MOTION, &VirtualBrushList::OnMotion, this);
	Bind(wxEVT_KEY_DOWN, &VirtualBrushList::OnKeyDown, this);

	item_height = FromDIP(32);
	Clear();
}

VirtualBrushList::~VirtualBrushList() {
}

void VirtualBrushList::Clear() {
	cleared = true;
	no_matches = false;
	brushlist.clear();
	selected_index = -1;
	hover_index = -1;
	UpdateLayout();
	Refresh();
}

void VirtualBrushList::SetNoMatches() {
	cleared = false;
	no_matches = true;
	brushlist.clear();
	selected_index = -1;
	hover_index = -1;
	UpdateLayout();
	Refresh();
}

void VirtualBrushList::AddBrush(Brush* brush) {
	if (cleared || no_matches) {
		brushlist.clear();
		cleared = false;
		no_matches = false;
	}
	brushlist.push_back(brush);
	// Defer UpdateLayout() if possible, but for now we rely on explicit RecalculateLayout() for batch adds
	// or UpdateLayout() implicitly.
	// Since FindDialogListBox::AddBrush was called in loop, we should avoid expensive ops here.
}

void VirtualBrushList::RecalculateLayout() {
	UpdateLayout();
	Refresh();
}

Brush* VirtualBrushList::GetSelectedBrush() const {
	if (selected_index >= 0 && selected_index < (int)brushlist.size()) {
		return brushlist[selected_index];
	}
	return nullptr;
}

void VirtualBrushList::SetSelection(int index) {
	if (index >= -1 && index < (int)brushlist.size()) {
		if (selected_index != index) {
			selected_index = index;
			if (selected_index != -1) {
				wxRect rect = GetItemRect(selected_index);
				int scrollPos = GetScrollPosition();
				int clientHeight = GetClientSize().y;

				if (rect.y < scrollPos) {
					SetScrollPosition(rect.y - PADDING);
				} else if (rect.y + rect.height > scrollPos + clientHeight) {
					SetScrollPosition(rect.y + rect.height - clientHeight + PADDING);
				}
			}
			Refresh();
		}
	}
}

int VirtualBrushList::GetSelection() const {
	return selected_index;
}

int VirtualBrushList::GetItemCount() const {
	return (int)brushlist.size();
}

void VirtualBrushList::UpdateLayout() {
	int count = brushlist.size();
	if (count == 0 && (cleared || no_matches)) {
		count = 1; // Message
	}
	int contentHeight = count * (item_height + PADDING) + PADDING;
	UpdateScrollbar(contentHeight);
}

wxSize VirtualBrushList::DoGetBestClientSize() const {
	return FromDIP(wxSize(300, 400));
}

void VirtualBrushList::OnSize(wxSizeEvent& event) {
	UpdateLayout();
	Refresh();
	event.Skip();
}

void VirtualBrushList::OnMouseDown(wxMouseEvent& event) {
	SetFocus();
	int index = HitTest(event.GetX(), event.GetY());
	if (index != -1) {
		SetSelection(index);

		// Emit selection event
		wxCommandEvent evt(wxEVT_LISTBOX, GetId());
		evt.SetInt(index);
		evt.SetEventObject(this);
		GetEventHandler()->ProcessEvent(evt);
	}
	event.Skip();
}

void VirtualBrushList::OnMouseDoubleClick(wxMouseEvent& event) {
	int index = HitTest(event.GetX(), event.GetY());
	if (index != -1) {
		SetSelection(index);

		wxCommandEvent evt(wxEVT_LISTBOX_DCLICK, GetId());
		evt.SetInt(index);
		evt.SetEventObject(this);
		GetEventHandler()->ProcessEvent(evt);
	}
	event.Skip();
}

void VirtualBrushList::OnMotion(wxMouseEvent& event) {
	int index = HitTest(event.GetX(), event.GetY());
	if (index != hover_index) {
		hover_index = index;
		Refresh();
	}
	event.Skip();
}

void VirtualBrushList::OnKeyDown(wxKeyEvent& event) {
	int kc = event.GetKeyCode();
	if (kc == WXK_UP) {
		if (selected_index > 0) SetSelection(selected_index - 1);
	} else if (kc == WXK_DOWN) {
		if (selected_index < (int)brushlist.size() - 1) SetSelection(selected_index + 1);
	} else if (kc == WXK_PAGEUP) {
		int visible = GetClientSize().y / (item_height + PADDING);
		SetSelection(std::max(0, selected_index - visible));
	} else if (kc == WXK_PAGEDOWN) {
		int visible = GetClientSize().y / (item_height + PADDING);
		SetSelection(std::min((int)brushlist.size() - 1, selected_index + visible));
	} else if (kc == WXK_HOME) {
		SetSelection(0);
	} else if (kc == WXK_END) {
		SetSelection((int)brushlist.size() - 1);
	} else if (kc == WXK_RETURN) {
		if (selected_index != -1) {
			wxCommandEvent evt(wxEVT_LISTBOX_DCLICK, GetId());
			evt.SetInt(selected_index);
			evt.SetEventObject(this);
			GetEventHandler()->ProcessEvent(evt);
		}
	} else {
		event.Skip();
	}
}

int VirtualBrushList::HitTest(int x, int y) const {
	int scrollPos = GetScrollPosition();
	int realY = y + scrollPos;

	if (realY < PADDING) return -1;

	int index = (realY - PADDING) / (item_height + PADDING);

	if (index >= 0 && index < (int)brushlist.size()) {
		return index;
	}
	return -1;
}

wxRect VirtualBrushList::GetItemRect(int index) const {
	int width = GetClientSize().x - 2 * PADDING;
	return wxRect(PADDING, PADDING + index * (item_height + PADDING), width, item_height);
}

void VirtualBrushList::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	int scrollPos = GetScrollPosition();

	// Draw background for visible area (already done by NanoVGCanvas? No, usually handled by user)
	// NanoVGCanvas implementation clears background if desired?
	// We'll draw explicitly.

	nvgBeginPath(vg);
	nvgRect(vg, 0, scrollPos, width, height);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255)); // White background like ListBox
	nvgFill(vg);

	if (cleared || no_matches) {
		nvgFontSize(vg, 14.0f);
		nvgFontFace(vg, "sans");
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
		nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
		const char* msg = no_matches ? "No matches for your search." : "Please enter your search string.";
		nvgText(vg, 40, scrollPos + 6, msg, nullptr);
		return;
	}

	int startRow = scrollPos / (item_height + PADDING);
	int endRow = (scrollPos + height + (item_height + PADDING) - 1) / (item_height + PADDING) + 1;

	int startIdx = std::max(0, startRow);
	int endIdx = std::min((int)brushlist.size(), endRow);

	for (int i = startIdx; i < endIdx; ++i) {
		wxRect rect = GetItemRect(i);

		// Selection / Hover
		if (i == selected_index) {
			nvgBeginPath(vg);
			nvgRect(vg, 0, rect.y - PADDING/2, width, rect.height + PADDING);
			nvgFillColor(vg, nvgRGBA(51, 153, 255, 255)); // Standard highlight blue
			nvgFill(vg);
		} else if (i == hover_index) {
			nvgBeginPath(vg);
			nvgRect(vg, 0, rect.y - PADDING/2, width, rect.height + PADDING);
			nvgFillColor(vg, nvgRGBA(240, 240, 240, 255)); // Light gray hover
			nvgFill(vg);
		}

		// Draw Icon
		Brush* brush = brushlist[i];
		Sprite* spr = nullptr;
		if (brush) {
			spr = g_gui.gfx.getSprite(brush->getLookID());
		}

		if (spr) {
			int tex = GetOrCreateSpriteTexture(vg, spr);
			if (tex > 0) {
				int iconSize = rect.height;
				NVGpaint imgPaint = nvgImagePattern(vg, rect.x, rect.y, iconSize, iconSize, 0, tex, 1.0f);
				nvgBeginPath(vg);
				nvgRect(vg, rect.x, rect.y, iconSize, iconSize);
				nvgFillPaint(vg, imgPaint);
				nvgFill(vg);
			}
		}

		// Draw Text
		nvgFontSize(vg, 14.0f);
		nvgFontFace(vg, "sans");
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		if (i == selected_index) {
			nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		} else {
			nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
		}

		auto it = m_utf8NameCache.find(brush);
		if (it == m_utf8NameCache.end()) {
			m_utf8NameCache[brush] = std::string(wxstr(brush->getName()).ToUTF8());
			it = m_utf8NameCache.find(brush);
		}
		nvgText(vg, rect.x + rect.height + 8, rect.y + rect.height / 2.0f, it->second.c_str(), nullptr);
	}
}
