#include "ui/dialogs/find_dialog.h"

#include "brushes/brush.h"
#include "game/items.h"
#include "ui/gui.h"
#include "brushes/raw/raw_brush.h"
#include "rendering/utilities/sprite_icon_generator.h"
#include <nanovg.h>
#include <algorithm>

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
	item_list->SetMinSize(wxSize(470, 400));
	item_list->SetToolTip("Double click to select.");
	sizer->Add(item_list, wxSizerFlags(1).Expand().Border());

	wxSizer* stdsizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* okBtn = newd wxButton(this, wxID_OK, "OK");
	okBtn->SetToolTip("Jump to selected item/brush");
	stdsizer->Add(okBtn, wxSizerFlags(1).Center());
	wxButton* cancelBtn = newd wxButton(this, wxID_CANCEL, "Cancel");
	cancelBtn->SetToolTip("Close this window");
	stdsizer->Add(cancelBtn, wxSizerFlags(1).Center());
	sizer->Add(stdsizer, wxSizerFlags(0).Center().Border());

	SetSizerAndFit(sizer);
	Centre(wxBOTH);

	Bind(wxEVT_TIMER, &FindDialog::OnTextIdle, this, wxID_ANY);
	Bind(wxEVT_TEXT, &FindDialog::OnTextChange, this, JUMP_DIALOG_TEXT);
	Bind(wxEVT_KEY_DOWN, &FindDialog::OnKeyDown, this);
	Bind(wxEVT_TEXT_ENTER, &FindDialog::OnClickOK, this, JUMP_DIALOG_TEXT);

    item_list->Bind(wxEVT_LEFT_DCLICK, [this](wxMouseEvent& evt) {
        wxCommandEvent cmd(wxEVT_LISTBOX_DCLICK, JUMP_DIALOG_LIST);
        cmd.SetEventObject(item_list);
        OnClickList(cmd);
    });

	Bind(wxEVT_BUTTON, &FindDialog::OnClickOK, this, wxID_OK);
	Bind(wxEVT_BUTTON, &FindDialog::OnClickCancel, this, wxID_CANCEL);
}

FindDialog::~FindDialog() = default;

void FindDialog::OnKeyDown(wxKeyEvent& event) {
	int w, h;
	item_list->GetSize(&w, &h);
	size_t amount = 1;

	switch (event.GetKeyCode()) {
		case WXK_PAGEUP:
			amount = h / 40 + 1; // 40 is item_height
			[[fallthrough]];
		case WXK_UP: {
			if (item_list->GetItemCount() > 0) {
				int n = item_list->GetSelection();
				if (n == -1) {
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
			amount = h / 40 + 1;
			[[fallthrough]];
		case WXK_DOWN: {
			if (item_list->GetItemCount() > 0) {
				int n = item_list->GetSelection();
				size_t itemcount = item_list->GetItemCount();
				if (n == -1) {
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
		if (item_list->GetSelection() == -1) {
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
					if (as_lower_str(brush->getName()).find(search_string) == std::string::npos) {
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

						if (as_lower_str(raw_brush->getName()).find(search_string) == std::string::npos) {
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

			if (as_lower_str(brush->getName()).find(search_string) == std::string::npos) {
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

			if (as_lower_str(raw_brush->getName()).find(search_string) == std::string::npos) {
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
	item_list->Refresh();
}

// ============================================================================
// Listbox in find item / brush stuff

FindDialogListBox::FindDialogListBox(wxWindow* parent, wxWindowID id) :
	NanoVGCanvas(parent, id, wxVSCROLL | wxWANTS_CHARS),
	cleared(true),
	no_matches(false) {
	Bind(wxEVT_LEFT_DOWN, &FindDialogListBox::OnMouse, this);
    Bind(wxEVT_SIZE, &FindDialogListBox::OnSize, this);
	UpdateVirtualSize();
}

FindDialogListBox::~FindDialogListBox() {
}

void FindDialogListBox::Clear() {
	cleared = true;
	no_matches = false;
	brushlist.clear();
	selected_index = -1;
	UpdateVirtualSize();
}

void FindDialogListBox::SetNoMatches() {
	cleared = false;
	no_matches = true;
	brushlist.clear();
	selected_index = -1;
	UpdateVirtualSize();
}

void FindDialogListBox::AddBrush(Brush* brush) {
	cleared = false;
	no_matches = false;
	brushlist.push_back(brush);
	UpdateVirtualSize();
}

void FindDialogListBox::UpdateVirtualSize() {
	int count = brushlist.size();
	if (cleared || no_matches) count = 1;
	int contentHeight = count * item_height;
	UpdateScrollbar(contentHeight);
	Refresh();
}

void FindDialogListBox::OnSize(wxSizeEvent& evt) {
    UpdateVirtualSize();
    evt.Skip();
}

Brush* FindDialogListBox::GetSelectedBrush() {
	if (selected_index == -1 || no_matches || cleared) {
		return nullptr;
	}
	if (selected_index >= 0 && selected_index < (int)brushlist.size()) {
		return brushlist[selected_index];
	}
	return nullptr;
}

void FindDialogListBox::SetSelection(int index) {
    int max = brushlist.size();
    if (cleared || no_matches) max = 1;

    if (index >= 0 && index < max) {
        selected_index = index;

        // Ensure visible
        int scrollPos = GetScrollPosition();
        int y = index * item_height;
        int h = GetClientSize().GetHeight();

        if (y < scrollPos) {
            SetScrollPosition(y);
        } else if (y + item_height > scrollPos + h) {
            SetScrollPosition(y + item_height - h);
        }

        Refresh();
    }
}

int FindDialogListBox::GetOrCreateBrushImage(NVGcontext* vg, Brush* brush) {
	if (!brush) return 0;

	int lookID = brush->getLookID();
	if (lookID == 0) return 0;

	uint32_t cacheKey = static_cast<uint32_t>(lookID);

	int existing = GetCachedImage(cacheKey);
	if (existing > 0) return existing;

	GameSprite* spr = g_gui.gfx.getSprite(lookID);
	if (!spr) return 0;

	wxBitmap bmp = SpriteIconGenerator::Generate(spr, SPRITE_SIZE_32x32);
	if (!bmp.IsOk()) return 0;

	wxImage img = bmp.ConvertToImage();
	if (!img.IsOk()) return 0;

	int w = img.GetWidth();
	int h = img.GetHeight();
	std::vector<uint8_t> rgba(w * h * 4);
	const uint8_t* data = img.GetData();
	const uint8_t* alpha = img.GetAlpha();

	for (int i = 0; i < w * h; ++i) {
		rgba[i * 4 + 0] = data[i * 3 + 0];
		rgba[i * 4 + 1] = data[i * 3 + 1];
		rgba[i * 4 + 2] = data[i * 3 + 2];
		if (alpha) {
			rgba[i * 4 + 3] = alpha[i];
		} else {
			rgba[i * 4 + 3] = 255;
		}
	}

	return GetOrCreateImage(cacheKey, rgba.data(), w, h);
}

void FindDialogListBox::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
    // Clear background (white for listbox)
    nvgBeginPath(vg);
	nvgRect(vg, 0, GetScrollPosition(), width, height);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgFill(vg);

	if (no_matches) {
		nvgFontSize(vg, 14.0f);
        nvgFontFace(vg, "sans");
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
        nvgText(vg, 40, 20 + GetScrollPosition(), "No matches for your search.", nullptr);
        return;
	}

    if (cleared) {
		nvgFontSize(vg, 14.0f);
        nvgFontFace(vg, "sans");
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
        nvgText(vg, 40, 20 + GetScrollPosition(), "Please enter your search string.", nullptr);
        return;
	}

    // Draw List
    int scrollPos = GetScrollPosition();
    int start_idx = scrollPos / item_height;
    int end_idx = (scrollPos + height + item_height - 1) / item_height;

    if (start_idx < 0) start_idx = 0;
    if (end_idx > (int)brushlist.size()) end_idx = brushlist.size();

    for (int i = start_idx; i < end_idx; ++i) {
        float y = i * item_height;
        float w = width;
        float h = item_height;

        // Background
        if (i == selected_index) {
             nvgBeginPath(vg);
             nvgRect(vg, 0, y, w, h);
             nvgFillColor(vg, nvgRGBA(50, 100, 200, 255)); // Selection Blue
             nvgFill(vg);
        }

        Brush* brush = brushlist[i];

        // Icon
        if (brush) {
            int imgId = GetOrCreateBrushImage(vg, brush);
            if (imgId > 0) {
                float ix = 4;
                float iy = y + (h - 32) / 2;
                NVGpaint imgPaint = nvgImagePattern(vg, ix, iy, 32, 32, 0, imgId, 1.0f);
                nvgBeginPath(vg);
                nvgRect(vg, ix, iy, 32, 32);
                nvgFillPaint(vg, imgPaint);
                nvgFill(vg);
            }
        }

        // Text
        nvgFontSize(vg, 14.0f);
        nvgFontFace(vg, "sans");
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

        if (i == selected_index) {
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
        } else {
            nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
        }

        nvgText(vg, 40, y + h / 2, brush->getName().c_str(), nullptr);
    }
}

void FindDialogListBox::OnMouse(wxMouseEvent& evt) {
    int scrollPos = GetScrollPosition();
    int y = evt.GetY() + scrollPos;

    int index = y / item_height;
    int count = brushlist.size();
    if (cleared || no_matches) count = 1;

    if (index >= 0 && index < count) {
        if (cleared || no_matches) return; // Can't select message

        SetSelection(index);
    }
}
