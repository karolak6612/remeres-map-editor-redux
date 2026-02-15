#include "ui/dialogs/find_dialog.h"

#include "brushes/brush.h"
#include "game/items.h"
#include "ui/gui.h"
#include "brushes/raw/raw_brush.h"
#include "util/image_manager.h"
#include <algorithm>

#include <glad/glad.h>
#include <nanovg.h>

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
	int w, h;
	item_list->GetSize(&w, &h);
	size_t amount = 1;

	switch (event.GetKeyCode()) {
		case WXK_PAGEUP:
			amount = h / 32 + 1;
			[[fallthrough]];
		case WXK_UP: {
			if (item_list->GetItemCount() > 0) {
				ssize_t n = item_list->GetSelection();
				if (n == wxNOT_FOUND) {
					n = 0;
				} else if (static_cast<size_t>(n) >= amount) {
					n -= amount;
				} else {
					n = 0;
				}
				item_list->SetSelection(n);
			}
			break;
		}

		case WXK_PAGEDOWN:
			amount = h / 32 + 1;
			[[fallthrough]];
		case WXK_DOWN: {
			if (item_list->GetItemCount() > 0) {
				ssize_t n = item_list->GetSelection();
				size_t itemcount = item_list->GetItemCount();
				if (n == wxNOT_FOUND) {
					n = 0;
				} else if (static_cast<uint32_t>(n) < itemcount - amount && itemcount - amount < itemcount) {
					n += amount;
				} else {
					n = item_list->GetItemCount() - 1;
				}

				item_list->SetSelection(n);
			}
			break;
		}
		default:
			event.Skip();
			break;
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
					if (brush->isRaw()) {
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

			if (brush->isRaw()) {
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
			item_list->UpdateLayout();
			item_list->SetSelection(0);
		} else {
			item_list->SetNoMatches();
		}
	}
}

// ============================================================================
// Listbox in find item / brush stuff

FindDialogListBox::FindDialogListBox(wxWindow* parent, wxWindowID id) :
	NanoVGCanvas(parent, id, wxVSCROLL | wxWANTS_CHARS),
	cleared(true),
	no_matches(false),
	selected_index(-1),
	hover_index(-1),
	item_height(0) {

	// Set white background
	m_bgRed = 1.0f;
	m_bgGreen = 1.0f;
	m_bgBlue = 1.0f;

	item_height = FromDIP(32);

	Bind(wxEVT_LEFT_DOWN, &FindDialogListBox::OnLeftDown, this);
	Bind(wxEVT_MOTION, &FindDialogListBox::OnMotion, this);
	Bind(wxEVT_LEFT_DCLICK, &FindDialogListBox::OnLeftDClick, this);
	Bind(wxEVT_SIZE, &FindDialogListBox::OnSize, this);

	Clear();
}

FindDialogListBox::~FindDialogListBox() {
	////
}

void FindDialogListBox::Clear() {
	cleared = true;
	no_matches = false;
	brushlist.clear();
	selected_index = -1;
	hover_index = -1;
	UpdateLayout();
	Refresh();
}

void FindDialogListBox::SetNoMatches() {
	cleared = false;
	no_matches = true;
	brushlist.clear();
	selected_index = -1;
	hover_index = -1;
	UpdateLayout();
	Refresh();
}

void FindDialogListBox::AddBrush(Brush* brush) {
	if (cleared || no_matches) {
		brushlist.clear();
		selected_index = -1;
	}

	cleared = false;
	no_matches = false;

	brushlist.push_back(brush);
}

Brush* FindDialogListBox::GetSelectedBrush() {
	if (selected_index >= 0 && selected_index < (int)brushlist.size()) {
		return brushlist[selected_index];
	}
	return nullptr;
}

void FindDialogListBox::SetSelection(int index) {
	if (index >= -1 && index < (int)brushlist.size()) {
		selected_index = index;

		// Scroll into view
		if (index != -1) {
			int y = index * item_height;
			int scrollPos = GetScrollPosition();
			int h = GetClientSize().y;
			if (y < scrollPos) {
				SetScrollPosition(y);
			} else if (y + item_height > scrollPos + h) {
				SetScrollPosition(y + item_height - h);
			}
		}
		Refresh();
	}
}

int FindDialogListBox::GetSelection() const {
	return selected_index;
}

size_t FindDialogListBox::GetItemCount() const {
	return brushlist.size();
}

wxSize FindDialogListBox::DoGetBestClientSize() const {
	return FromDIP(wxSize(470, 400));
}

void FindDialogListBox::UpdateLayout() {
	int contentHeight = 0;
	if (cleared || no_matches) {
		contentHeight = item_height; // Just one line of text
	} else {
		contentHeight = brushlist.size() * item_height;
	}
	UpdateScrollbar(contentHeight);
}

void FindDialogListBox::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	if (cleared) {
		nvgFontSize(vg, 16.0f); // 16px is approx standard UI font
		nvgFontFace(vg, "sans");
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
		nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
		nvgText(vg, FromDIP(40), FromDIP(6), "Please enter your search string.", nullptr);
		return;
	}
	if (no_matches) {
		nvgFontSize(vg, 16.0f);
		nvgFontFace(vg, "sans");
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
		nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
		nvgText(vg, FromDIP(40), FromDIP(6), "No matches for your search.", nullptr);
		return;
	}

	int scrollPos = GetScrollPosition();
	int startIdx = scrollPos / item_height;
	int endIdx = std::min((int)brushlist.size(), (scrollPos + height + item_height - 1) / item_height + 1);

	nvgFontSize(vg, 14.0f); // Slightly smaller for list items
	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

	for (int i = startIdx; i < endIdx; ++i) {
		int y = i * item_height;

		// Selection/Hover
		if (i == selected_index) {
			// Draw selection rect
			nvgBeginPath(vg);
			nvgRect(vg, 0, y, width, item_height);
			nvgFillColor(vg, nvgRGBA(51, 153, 255, 255)); // Standard selection blue
			nvgFill(vg);
		} else if (i == hover_index) {
			// Draw hover rect
			nvgBeginPath(vg);
			nvgRect(vg, 0, y, width, item_height);
			nvgFillColor(vg, nvgRGBA(230, 230, 230, 255)); // Light gray
			nvgFill(vg);
		}

		// Draw sprite
		Brush* brush = brushlist[i];
		Sprite* spr = g_gui.gfx.getSprite(brush->getLookID());
		if (spr) {
			int tex = GetOrCreateSpriteTexture(vg, spr);
			if (tex > 0) {
				int icon_padding = FromDIP(2);
				int icon_size = item_height - icon_padding * 2;
				NVGpaint imgPaint = nvgImagePattern(vg, icon_padding, y + icon_padding, icon_size, icon_size, 0, tex, 1.0f);
				nvgBeginPath(vg);
				nvgRect(vg, icon_padding, y + icon_padding, icon_size, icon_size);
				nvgFillPaint(vg, imgPaint);
				nvgFill(vg);
			}
		}

		// Draw text
		if (i == selected_index) {
			nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		} else {
			nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
		}

		nvgText(vg, item_height + FromDIP(10), y + item_height / 2.0f, wxstr(brush->getName()).ToUTF8(), nullptr);
	}
}

void FindDialogListBox::OnLeftDown(wxMouseEvent& event) {
	int y = event.GetY() + GetScrollPosition();
	int idx = HitTest(y);
	if (idx != -1) {
		SetSelection(idx);

		wxCommandEvent evt(wxEVT_LISTBOX, GetId());
		evt.SetEventObject(this);
		evt.SetInt(idx);
		ProcessEvent(evt);
	}
	event.Skip();
}

void FindDialogListBox::OnMotion(wxMouseEvent& event) {
	int y = event.GetY() + GetScrollPosition();
	int idx = HitTest(y);

	if (idx != hover_index) {
		hover_index = idx;
		Refresh();
	}

	event.Skip();
}

void FindDialogListBox::OnLeftDClick(wxMouseEvent& event) {
	int y = event.GetY() + GetScrollPosition();
	int idx = HitTest(y);
	if (idx != -1) {
		SetSelection(idx);
		// Propagate event
		wxCommandEvent evt(wxEVT_LISTBOX_DCLICK, GetId());
		evt.SetEventObject(this);
		ProcessEvent(evt);
	}
}

void FindDialogListBox::OnSize(wxSizeEvent& event) {
	UpdateLayout();
	Refresh();
	event.Skip();
}

int FindDialogListBox::HitTest(int y) const {
	if (cleared || no_matches) return -1;
	int idx = y / item_height;
	if (idx >= 0 && idx < (int)brushlist.size()) return idx;
	return -1;
}
