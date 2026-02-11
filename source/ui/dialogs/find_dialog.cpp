#include "ui/dialogs/find_dialog.h"

#include "brushes/brush.h"
#include "game/items.h"
#include "ui/gui.h"
#include "brushes/raw/raw_brush.h"
#include "util/nvg_utils.h"
#include "ui/theme.h"

#include <nanovg.h>

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
	Bind(wxEVT_LISTBOX_DCLICK, &FindDialog::OnClickList, this, JUMP_DIALOG_LIST);
	Bind(wxEVT_BUTTON, &FindDialog::OnClickOK, this, wxID_OK);
	Bind(wxEVT_BUTTON, &FindDialog::OnClickCancel, this, wxID_CANCEL);

	// We can't call it here since it calls an abstract function, call in child constructors instead.
	// RefreshContents();
}

FindDialog::~FindDialog() = default;

void FindDialog::OnKeyDown(wxKeyEvent& event) {
	// Let the list box handle its own navigation if focused?
	// But this dialog catches events from text ctrl too via pending event?

	int w, h;
	item_list->GetSize(&w, &h);
	size_t amount = 1;
	int rowHeight = item_list->GetRowHeight();

	switch (event.GetKeyCode()) {
		case WXK_PAGEUP:
			amount = h / rowHeight + 1;
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
				// TODO: EnsureVisible logic is inside SetSelection/NanoVGListBox already?
				// Yes, SetSelection in NanoVGListBox handles selection but not scrolling EnsureVisible?
				// No, NanoVGListBox::OnKeyDown handles it. But here we manually set selection.
				// We might need to expose EnsureVisible or make SetSelection do it.
			}
			break;
		}

		case WXK_PAGEDOWN:
			amount = h / rowHeight + 1;
			[[fallthrough]];
		case WXK_DOWN: {
			if (item_list->GetItemCount() > 0) {
				int n = item_list->GetSelection();
				int itemcount = item_list->GetItemCount();
				if (n == -1) {
					n = 0;
				} else if (static_cast<size_t>(n) < itemcount - amount && itemcount - amount < static_cast<size_t>(itemcount)) {
					n += amount;
				} else {
					n = itemcount - 1;
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
	NanoVGListBox(parent, id, wxLB_SINGLE),
	cleared(false),
	no_matches(false) {
	SetRowHeight(FROM_DIP(this, 32));
	Clear();
}

FindDialogListBox::~FindDialogListBox() {
	////
}

void FindDialogListBox::Clear() {
	cleared = true;
	no_matches = false;
	brushlist.clear();
	SetItemCount(1);
}

void FindDialogListBox::SetNoMatches() {
	cleared = false;
	no_matches = true;
	brushlist.clear();
	SetItemCount(1);
}

void FindDialogListBox::AddBrush(Brush* brush) {
	if (cleared || no_matches) {
		SetItemCount(0);
	}

	cleared = false;
	no_matches = false;

	brushlist.push_back(brush);
	// We only update the count, NanoVGListBox handles scrolling update
	// But to avoid O(N) refresh spam, we should probably defer this?
	// For now, simple enough.
	SetItemCount(static_cast<int>(brushlist.size()));
}

Brush* FindDialogListBox::GetSelectedBrush() {
	int n = GetSelection();
	if (n == -1 || no_matches || cleared) {
		return nullptr;
	}
	return brushlist[n];
}

void FindDialogListBox::OnDrawItem(NVGcontext* vg, const wxRect& rect, int n) {
	if (no_matches) {
		nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));
		nvgFontSize(vg, 16.0f);
		nvgFontFace(vg, "sans");
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgText(vg, rect.x + 40, rect.y + rect.height / 2.0f, "No matches for your search.", nullptr);
	} else if (cleared) {
		nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));
		nvgFontSize(vg, 16.0f);
		nvgFontFace(vg, "sans");
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgText(vg, rect.x + 40, rect.y + rect.height / 2.0f, "Please enter your search string.", nullptr);
	} else {
		if (n >= static_cast<int>(brushlist.size())) return;

		bool isSelected = IsSelected(n);

		// Background handled by OnNanoVGPaint loop if needed, but NanoVGListBox::OnNanoVGPaint doesn't draw item background
		// We should draw background
		if (isSelected) {
			nvgBeginPath(vg);
			nvgRect(vg, rect.x, rect.y, rect.width, rect.height);
			wxColour c = Theme::Get(Theme::Role::Accent);
			nvgFillColor(vg, nvgRGBA(c.Red(), c.Green(), c.Blue(), 255));
			nvgFill(vg);
		} else if (m_hoverIndex == n) {
			nvgBeginPath(vg);
			nvgRect(vg, rect.x, rect.y, rect.width, rect.height);
			wxColour c = Theme::Get(Theme::Role::Surface);
			nvgFillColor(vg, nvgRGBA(c.Red() + 10, c.Green() + 10, c.Blue() + 10, 255)); // Slightly lighter
			nvgFill(vg);
		}

		Brush* brush = brushlist[n];
		Sprite* spr = brush->getSprite();
		if (!spr) {
			spr = g_gui.gfx.getSprite(brush->getLookID());
		}

		if (spr) {
			int tex = GetOrCreateSpriteTexture(vg, spr);
			if (tex > 0) {
				int iconSize = rect.height - 4;
				NVGpaint imgPaint = nvgImagePattern(vg, rect.x + 2, rect.y + 2, iconSize, iconSize, 0, tex, 1.0f);
				nvgBeginPath(vg);
				nvgRect(vg, rect.x + 2, rect.y + 2, iconSize, iconSize);
				nvgFillPaint(vg, imgPaint);
				nvgFill(vg);
			}
		}

		wxColour textColor = isSelected ? *wxWHITE : Theme::Get(Theme::Role::Text);
		nvgFillColor(vg, nvgRGBA(textColor.Red(), textColor.Green(), textColor.Blue(), 255));
		nvgFontSize(vg, 15.0f);
		nvgFontFace(vg, "sans");
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

		std::string name = wxstr(brush->getName()).ToStdString();
		nvgText(vg, rect.x + rect.height + 10, rect.y + rect.height / 2.0f, name.c_str(), nullptr);
	}
}
