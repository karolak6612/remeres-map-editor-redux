#include "ui/dialogs/find_dialog.h"

#include "brushes/brush.h"
#include "game/items.h"
#include "ui/gui.h"
#include "ui/theme.h"
#include "brushes/raw/raw_brush.h"
#include "util/image_manager.h"
#include <glad/glad.h>
#include <nanovg.h>
#include <algorithm>

static bool caseInsensitiveContains(std::string_view haystack, std::string_view needle_lower) {
	auto it = std::search(
		haystack.begin(), haystack.end(),
		needle_lower.begin(), needle_lower.end(),
		[](char ch1, char ch2) {
			return std::tolower(static_cast<unsigned char>(ch1)) == std::tolower(static_cast<unsigned char>(ch2));
		}
	);
	return it != haystack.end();
}

// ============================================================================
// Numkey forwarding text control

void KeyForwardingTextCtrl::OnKeyDown(wxKeyEvent& event) {
	if (event.GetKeyCode() == WXK_UP || event.GetKeyCode() == WXK_DOWN || event.GetKeyCode() == WXK_PAGEDOWN || event.GetKeyCode() == WXK_PAGEUP) {
		GetParent()->GetEventHandler()->AddPendingEvent(event);
	} else {
		event.Skip();
	}
}

// ============================================================================
// Find Item Dialog (Jump to item)

FindDialog::FindDialog(wxWindow* parent, wxString title) :
	wxDialog(g_gui.root, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX),
	idle_input_timer(this),
	result_brush(nullptr),
	result_id(0) {
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	search_field = newd KeyForwardingTextCtrl(this, JUMP_DIALOG_TEXT, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	search_field->SetHint("Type to search...");
	search_field->SetToolTip("Type at least 2 characters to search for brushes or items.");
	search_field->SetFocus();
	sizer->Add(search_field, 0, wxEXPAND);

	item_list = newd FindDialogListBox(this, JUMP_DIALOG_LIST);
	item_list->SetMinSize(FROM_DIP(item_list, wxSize(470, 400)));
	item_list->SetToolTip("Double click to select.");
	sizer->Add(item_list, wxSizerFlags(1).Expand().Border());

	wxSizer* stdsizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* okBtn = newd wxButton(this, wxID_OK, "OK");
	okBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_CHECK, wxSize(16, 16)));
	okBtn->SetToolTip("Jump to selected item/brush");
	stdsizer->Add(okBtn, wxSizerFlags(1).Center());
	wxButton* cancelBtn = newd wxButton(this, wxID_CANCEL, "Cancel");
	cancelBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_XMARK, wxSize(16, 16)));
	cancelBtn->SetToolTip("Close this window");
	stdsizer->Add(cancelBtn, wxSizerFlags(1).Center());
	sizer->Add(stdsizer, wxSizerFlags(0).Center().Border());

	SetSizerAndFit(sizer);
	Centre(wxBOTH);

	Bind(wxEVT_TIMER, &FindDialog::OnTextIdle, this, wxID_ANY);
	Bind(wxEVT_TEXT, &FindDialog::OnTextChange, this, JUMP_DIALOG_TEXT);
	Bind(wxEVT_KEY_DOWN, &FindDialog::OnKeyDown, this);
	Bind(wxEVT_TEXT_ENTER, &FindDialog::OnClickOK, this, JUMP_DIALOG_TEXT);
	Bind(wxEVT_LISTBOX_DCLICK, &FindDialog::OnClickList, this, JUMP_DIALOG_LIST);
	Bind(wxEVT_BUTTON, &FindDialog::OnClickOK, this, wxID_OK);
	Bind(wxEVT_BUTTON, &FindDialog::OnClickCancel, this, wxID_CANCEL);

	// We can't call it here since it calls an abstract function, call in child constructors instead.
	// RefreshContents();

	wxIcon icon;
	icon.CopyFromBitmap(IMAGE_MANAGER.GetBitmap(ICON_SEARCH, wxSize(32, 32)));
	SetIcon(icon);
}

FindDialog::~FindDialog() = default;

void FindDialog::OnKeyDown(wxKeyEvent& event) {
	// Delegate to item_list
	// We want to give the listbox a chance to handle navigation keys (Arrows, PageUp/Down, etc.)
	// But if it doesn't handle it (e.g. it's a character key for the search box), we MUST skip it so the text ctrl gets it.

	wxKeyEvent keyEvent(event);
	keyEvent.SetEventObject(item_list);

	if (!item_list->GetEventHandler()->ProcessEvent(keyEvent)) {
		// If the listbox didn't process it, we skip the original event
		// to allow bubbling or default handling (likely by the focused control, which is the search box).
		event.Skip();
	}
}

void FindDialog::OnTextIdle(wxTimerEvent& WXUNUSED(event)) {
	RefreshContents();
}

void FindDialog::OnTextChange(wxCommandEvent& WXUNUSED(event)) {
	idle_input_timer.Start(800, true);
}

void FindDialog::OnClickList(wxCommandEvent& event) {
	OnClickListInternal(event);
}

void FindDialog::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	// This is to get virtual callback
	OnClickOKInternal();
}

void FindDialog::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	EndModal(0);
}

void FindDialog::RefreshContents() {
	// This is to get virtual callback
	RefreshContentsInternal();
}

// ============================================================================
// Find Brush Dialog (Jump to brush)

FindBrushDialog::FindBrushDialog(wxWindow* parent, wxString title) :
	FindDialog(parent, title) {
	RefreshContents();
}

FindBrushDialog::~FindBrushDialog() = default;

void FindBrushDialog::OnClickListInternal(wxCommandEvent& event) {
	Brush* brush = item_list->GetSelectedBrush();
	if (brush) {
		result_brush = brush;
		EndModal(1);
	}
}

void FindBrushDialog::OnClickOKInternal() {
	// This is kind of stupid as it would fail unless the "Please enter a search string" wasn't there
	if (item_list->GetItemCount() > 0) {
		if (item_list->GetSelection() == wxNOT_FOUND) {
			item_list->SetSelection(0);
		}
		Brush* brush = item_list->GetSelectedBrush();
		if (!brush) {
			// It's either "Please enter a search string" or "No matches"
			// Perhaps we can refresh now?
			std::string search_string = as_lower_str(nstr(search_field->GetValue()));
			bool do_search = (search_string.size() >= 2);

			if (do_search) {
				const BrushMap& map = g_brushes.getMap();
				for (const auto& [name, brush_ptr] : map) {
					const Brush* brush = brush_ptr.get();
					if (!caseInsensitiveContains(brush->getName(), search_string)) {
						continue;
					}

					// Don't match RAWs now.
					if (brush->is<RAWBrush>()) {
						continue;
					}

					// Found one!
					result_brush = brush;
					break;
				}

				// Did we not find a matching brush?
				if (!result_brush) {
					// Then let's search the RAWs
					for (int id = 0; id <= g_items.getMaxID(); ++id) {
						ItemType& it = g_items[id];
						if (it.id == 0) {
							continue;
						}

						RAWBrush* raw_brush = it.raw_brush;
						if (!raw_brush) {
							continue;
						}

						if (!caseInsensitiveContains(raw_brush->getName(), search_string)) {
							continue;
						}

						// Found one!
						result_brush = raw_brush;
						break;
					}
				}
				// Done!
			}
		} else {
			result_brush = brush;
		}
	}
	EndModal(1);
}

void FindBrushDialog::RefreshContentsInternal() {
	item_list->Clear();

	std::string search_string = as_lower_str(nstr(search_field->GetValue()));
	bool do_search = (search_string.size() >= 2);

	if (do_search) {

		bool found_search_results = false;

		const BrushMap& brushes_map = g_brushes.getMap();

		// We store the raws so they display last of all results
		std::deque<const RAWBrush*> raws;

		for (const auto& [name, brush_ptr] : brushes_map) {
			const Brush* brush = brush_ptr.get();

			if (!caseInsensitiveContains(brush->getName(), search_string)) {
				continue;
			}

			if (brush->is<RAWBrush>()) {
				continue;
			}

			found_search_results = true;
			item_list->AddBrush(const_cast<Brush*>(brush));
		}

		for (int id = 0; id <= g_items.getMaxID(); ++id) {
			ItemType& it = g_items[id];
			if (it.id == 0) {
				continue;
			}

			RAWBrush* raw_brush = it.raw_brush;
			if (!raw_brush) {
				continue;
			}

			if (!caseInsensitiveContains(raw_brush->getName(), search_string)) {
				continue;
			}

			found_search_results = true;
			item_list->AddBrush(raw_brush);
		}

		while (!raws.empty()) {
			item_list->AddBrush(const_cast<RAWBrush*>(raws.front()));
			raws.pop_front();
		}

		if (found_search_results) {
			item_list->SetSelection(0);
		} else {
			item_list->SetNoMatches();
		}
	}
	item_list->CommitUpdates();
}

// ============================================================================
// Grid Listbox in find item / brush stuff

FindDialogListBox::FindDialogListBox(wxWindow* parent, wxWindowID id) :
	NanoVGCanvas(parent, id, wxVSCROLL | wxWANTS_CHARS),
	cleared(false),
	no_matches(false),
	m_columns(1),
	m_selection(-1),
	m_hoverIndex(-1) {

	m_item_width = FromDIP(64);
	m_item_height = FromDIP(80); // Icon + Text

	Bind(wxEVT_SIZE, &FindDialogListBox::OnSize, this);
	Bind(wxEVT_LEFT_DOWN, &FindDialogListBox::OnMouseDown, this);
	Bind(wxEVT_KEY_DOWN, &FindDialogListBox::OnKeyDown, this);
	Bind(wxEVT_MOTION, &FindDialogListBox::OnMotion, this);
	Bind(wxEVT_LEAVE_WINDOW, &FindDialogListBox::OnLeave, this);

	Clear();
}

FindDialogListBox::~FindDialogListBox() {
}

void FindDialogListBox::Clear() {
	cleared = true;
	no_matches = false;
	brushlist.clear();
	m_selection = -1;
	m_hoverIndex = -1;
	UpdateGrid();
}

void FindDialogListBox::SetNoMatches() {
	cleared = false;
	no_matches = true;
	brushlist.clear();
	m_selection = -1;
	m_hoverIndex = -1;
	UpdateGrid();
}

void FindDialogListBox::AddBrush(Brush* brush) {
	cleared = false;
	no_matches = false;
	brushlist.push_back(brush);
}

void FindDialogListBox::CommitUpdates() {
	// Reset selection if out of bounds or invalid
	if (brushlist.empty()) {
		m_selection = -1;
	} else if (m_selection >= static_cast<int>(brushlist.size())) {
		m_selection = 0;
	}
	UpdateGrid();
}

Brush* FindDialogListBox::GetSelectedBrush() {
	if (m_selection == -1 || no_matches || cleared || m_selection >= static_cast<int>(brushlist.size())) {
		return nullptr;
	}
	return brushlist[m_selection];
}

void FindDialogListBox::SetSelection(int n) {
	if (n >= 0 && n < static_cast<int>(brushlist.size())) {
		m_selection = n;
		EnsureVisible(n);
		Refresh();
	}
}

void FindDialogListBox::OnSize(wxSizeEvent& event) {
	UpdateGrid();
	event.Skip();
}

void FindDialogListBox::UpdateGrid() {
	int clientW = GetClientSize().x;
	m_columns = std::max(1, clientW / m_item_width);

	int totalRows = 0;
	if (cleared || no_matches) {
		totalRows = 1; // Just a message
	} else {
		totalRows = (static_cast<int>(brushlist.size()) + m_columns - 1) / m_columns;
	}

	int totalHeight = totalRows * m_item_height;
	UpdateScrollbar(totalHeight);
	Refresh();
}

void FindDialogListBox::EnsureVisible(int index) {
	if (index < 0 || index >= static_cast<int>(brushlist.size())) return;

	int row = index / m_columns;
	int itemTop = row * m_item_height;
	int itemBottom = itemTop + m_item_height;

	int scrollPos = GetScrollPosition();
	int clientHeight = GetClientSize().y;

	if (itemTop < scrollPos) {
		SetScrollPosition(itemTop);
	} else if (itemBottom > scrollPos + clientHeight) {
		SetScrollPosition(itemBottom - clientHeight);
	}
}

void FindDialogListBox::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	wxColour bg = Theme::Get(Theme::Role::Surface);
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, width, height + GetScrollPosition()); // Fill whole virtual area effectively? No, OnNanoVGPaint coord system is shifted.
	// Actually, just fill visible area. 0,0 is top-left of visible area in OnNanoVGPaint?
	// NanoVGCanvas logic: nvgTranslate(vg, 0, -scrollPos). So 0 is top of CONTENT.
	// We should just fill rect(0, scrollPos, width, height) to clear bg?
	// But NanoVGCanvas doesn't clear BG automatically? It usually relies on wxGLCanvas.
	// Let's assume background is handled or we draw it.

	wxColour textCol = Theme::Get(Theme::Role::Text);

	if (cleared) {
		nvgFontSize(vg, 16.0f);
		nvgFontFace(vg, "sans-bold");
		nvgFillColor(vg, nvgRGBA(textCol.Red(), textCol.Green(), textCol.Blue(), 255));
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgText(vg, width / 2, GetScrollPosition() + height / 2, "Please enter search term...", nullptr);
		return;
	}

	if (no_matches) {
		nvgFontSize(vg, 16.0f);
		nvgFontFace(vg, "sans-bold");
		nvgFillColor(vg, nvgRGBA(textCol.Red(), textCol.Green(), textCol.Blue(), 255));
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgText(vg, width / 2, GetScrollPosition() + height / 2, "No matches found.", nullptr);
		return;
	}

	// Calculate visible range
	int scrollPos = GetScrollPosition();
	int startRow = scrollPos / m_item_height;
	int endRow = (scrollPos + height + m_item_height - 1) / m_item_height;

	int startIndex = startRow * m_columns;
	int endIndex = std::min(static_cast<int>(brushlist.size()), endRow * m_columns);

	for (int i = startIndex; i < endIndex; ++i) {
		int col = i % m_columns;
		int row = i / m_columns;

		float x = col * m_item_width;
		float y = row * m_item_height;

		bool selected = (i == m_selection);
		bool hover = (i == m_hoverIndex);

		// Draw Item Background
		if (selected || hover) {
			nvgBeginPath(vg);
			nvgRoundedRect(vg, x + 2, y + 2, m_item_width - 4, m_item_height - 4, 4.0f);
			if (selected) {
				wxColour c = Theme::Get(Theme::Role::Accent); // Selected color
				nvgFillColor(vg, nvgRGBA(c.Red(), c.Green(), c.Blue(), 128));
				nvgStrokeColor(vg, nvgRGBA(c.Red(), c.Green(), c.Blue(), 255));
				nvgStrokeWidth(vg, 2.0f);
			} else {
				wxColour c = Theme::Get(Theme::Role::CardBaseHover);
				nvgFillColor(vg, nvgRGBA(c.Red(), c.Green(), c.Blue(), c.Alpha()));
			}
			nvgFill(vg);
			if (selected) nvgStroke(vg);
		}

		// Draw Icon
		Sprite* spr = g_gui.gfx.getSprite(brushlist[i]->getLookID());
		if (spr) {
			int tex = GetOrCreateSpriteTexture(vg, spr);
			if (tex > 0) {
				float iconSize = 32.0f;
				float ix = x + (m_item_width - iconSize) / 2;
				float iy = y + 8;

				NVGpaint imgPaint = nvgImagePattern(vg, ix, iy, iconSize, iconSize, 0, tex, 1.0f);
				nvgBeginPath(vg);
				nvgRect(vg, ix, iy, iconSize, iconSize);
				nvgFillPaint(vg, imgPaint);
				nvgFill(vg);
			}
		}

		// Draw Text
		wxString name = wxstr(brushlist[i]->getName());
		// Truncate if too long?
		nvgFontSize(vg, 12.0f);
		nvgFontFace(vg, "sans");
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
		nvgFillColor(vg, nvgRGBA(textCol.Red(), textCol.Green(), textCol.Blue(), 255));

		// Simple text wrapping or truncation could go here, for now just print
		nvgTextBox(vg, x + 4, y + 44, m_item_width - 8, name.ToUTF8().data(), nullptr);
	}
}

wxSize FindDialogListBox::DoGetBestClientSize() const {
	return FromDIP(wxSize(600, 400));
}

void FindDialogListBox::OnMouseDown(wxMouseEvent& event) {
	int x = event.GetX();
	int y = event.GetY() + GetScrollPosition();

	int col = x / m_item_width;
	int row = y / m_item_height;

	if (col >= 0 && col < m_columns) {
		int index = row * m_columns + col;
		if (index >= 0 && index < static_cast<int>(brushlist.size())) {
			m_selection = index;
			Refresh();

			// Double click?
			if (event.LeftDClick()) {
				wxCommandEvent evt(wxEVT_LISTBOX_DCLICK, GetId());
				evt.SetEventObject(this);
				ProcessWindowEvent(evt);
			}
		}
	}
	SetFocus();
}

void FindDialogListBox::OnMotion(wxMouseEvent& event) {
	int x = event.GetX();
	int y = event.GetY() + GetScrollPosition();

	int col = x / m_item_width;
	int row = y / m_item_height;
	int index = -1;

	if (col >= 0 && col < m_columns) {
		index = row * m_columns + col;
		if (index < 0 || index >= static_cast<int>(brushlist.size())) {
			index = -1;
		}
	}

	if (index != m_hoverIndex) {
		m_hoverIndex = index;
		Refresh();
	}
	event.Skip();
}

void FindDialogListBox::OnLeave(wxMouseEvent& event) {
	if (m_hoverIndex != -1) {
		m_hoverIndex = -1;
		Refresh();
	}
}

void FindDialogListBox::OnKeyDown(wxKeyEvent& event) {
	if (brushlist.empty()) {
		event.Skip();
		return;
	}

	int next = m_selection;
	bool handled = false;

	switch (event.GetKeyCode()) {
		case WXK_LEFT:
			next--;
			handled = true;
			break;
		case WXK_RIGHT:
			next++;
			handled = true;
			break;
		case WXK_UP:
			next -= m_columns;
			handled = true;
			break;
		case WXK_DOWN:
			next += m_columns;
			handled = true;
			break;
		case WXK_PAGEUP:
			next -= m_columns * 4; // Approx page
			handled = true;
			break;
		case WXK_PAGEDOWN:
			next += m_columns * 4;
			handled = true;
			break;
		case WXK_HOME:
			next = 0;
			handled = true;
			break;
		case WXK_END:
			next = brushlist.size() - 1;
			handled = true;
			break;
		case WXK_RETURN:
		case WXK_NUMPAD_ENTER: {
				wxCommandEvent evt(wxEVT_LISTBOX_DCLICK, GetId());
				evt.SetEventObject(this);
				ProcessWindowEvent(evt);
				return;
			}
		default:
			event.Skip();
			return;
	}

	if (handled) {
		if (next < 0) next = 0;
		if (next >= static_cast<int>(brushlist.size())) next = brushlist.size() - 1;

		if (next != m_selection) {
			m_selection = next;
			EnsureVisible(m_selection);
			Refresh();
		}
	}
}
