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

	item_list = newd FindDialogGridCanvas(this, JUMP_DIALOG_LIST);
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
// Listbox in find item / brush stuff

FindDialogGridCanvas::FindDialogGridCanvas(wxWindow* parent, wxWindowID id) :
	NanoVGCanvas(parent, id, wxVSCROLL | wxWANTS_CHARS),
	cleared(false),
	no_matches(false),
	m_selection(-1),
	m_hoverIndex(-1),
	m_columns(1),
	m_itemSize(32),
	m_padding(8) {

	Bind(wxEVT_SIZE, &FindDialogGridCanvas::OnSize, this);
	Bind(wxEVT_LEFT_DOWN, &FindDialogGridCanvas::OnMouseDown, this);
	Bind(wxEVT_LEFT_DCLICK, &FindDialogGridCanvas::OnMouseDown, this);
	Bind(wxEVT_MOTION, &FindDialogGridCanvas::OnMotion, this);
	Bind(wxEVT_LEAVE_WINDOW, &FindDialogGridCanvas::OnLeave, this);
	Bind(wxEVT_KEY_DOWN, &FindDialogGridCanvas::OnKeyDown, this);

	Clear();
}

FindDialogGridCanvas::~FindDialogGridCanvas() {}

void FindDialogGridCanvas::Clear() {
	cleared = true;
	no_matches = false;
	brushlist.clear();
	m_selection = -1;
	m_hoverIndex = -1;
	UpdateLayout();
}

void FindDialogGridCanvas::SetNoMatches() {
	cleared = false;
	no_matches = true;
	brushlist.clear();
	m_selection = -1;
	m_hoverIndex = -1;
	UpdateLayout();
}

void FindDialogGridCanvas::AddBrush(Brush* brush) {
	cleared = false;
	no_matches = false;
	brushlist.push_back(brush);
}

void FindDialogGridCanvas::CommitUpdates() {
	UpdateLayout();
	Refresh();
}

Brush* FindDialogGridCanvas::GetSelectedBrush() {
	if (m_selection == -1 || no_matches || cleared) {
		return nullptr;
	}
	if (m_selection >= 0 && m_selection < static_cast<int>(brushlist.size())) {
		return brushlist[m_selection];
	}
	return nullptr;
}

void FindDialogGridCanvas::SetSelection(int index) {
	if (index != m_selection) {
		m_selection = index;
		SendSelectionEvent();

		if (index >= 0 && index < static_cast<int>(brushlist.size())) {
			wxRect rect = GetItemRect(index);
			int scrollPos = GetScrollPosition();
			int clientH = GetClientSize().y;

			if (rect.y < scrollPos) {
				SetScrollPosition(rect.y - m_padding);
			} else if (rect.y + rect.height > scrollPos + clientH) {
				SetScrollPosition(rect.y + rect.height - clientH + m_padding);
			}
		}
		Refresh();
	}
}

void FindDialogGridCanvas::UpdateLayout() {
	int width = GetClientSize().x;
	if (width <= 0) width = 200;

	m_columns = std::max(1, (width - m_padding) / (m_itemSize + m_padding));
	int count = GetItemCount();
	if (count == 0) count = 1; // for no match / cleared message

	int rows = (count + m_columns - 1) / m_columns;
	int contentHeight = rows * (m_itemSize + m_padding) + m_padding;

	UpdateScrollbar(contentHeight);
}

wxSize FindDialogGridCanvas::DoGetBestClientSize() const {
	return FromDIP(wxSize(470, 400));
}

int FindDialogGridCanvas::HitTest(int x, int y) const {
	if (cleared || no_matches) return -1;

	int scrollPos = GetScrollPosition();
	int realY = y + scrollPos;
	int col = (x - m_padding) / (m_itemSize + m_padding);
	int row = (realY - m_padding) / (m_itemSize + m_padding);

	if (col < 0 || col >= m_columns || row < 0) {
		return -1;
	}

	int index = row * m_columns + col;
	if (index < static_cast<int>(GetItemCount())) {
		wxRect r = GetItemRect(index);
		r.y -= scrollPos; // Convert back to local
		if (r.Contains(x, y)) {
			return index;
		}
	}
	return -1;
}

wxRect FindDialogGridCanvas::GetItemRect(int index) const {
	int row = index / m_columns;
	int col = index % m_columns;
	return wxRect(
		m_padding + col * (m_itemSize + m_padding),
		m_padding + row * (m_itemSize + m_padding),
		m_itemSize,
		m_itemSize
	);
}

void FindDialogGridCanvas::SendSelectionEvent() {
	wxCommandEvent event(wxEVT_LISTBOX, GetId());
	event.SetEventObject(this);
	event.SetInt(m_selection);
	GetEventHandler()->ProcessEvent(event);
}

void FindDialogGridCanvas::OnSize(wxSizeEvent& event) {
	UpdateLayout();
	Refresh();
	event.Skip();
}

void FindDialogGridCanvas::OnMouseDown(wxMouseEvent& event) {
	int index = HitTest(event.GetX(), event.GetY());
	if (index != -1) {
		SetSelection(index);
		if (event.LeftDClick()) {
			wxCommandEvent dclickEvent(wxEVT_LISTBOX_DCLICK, GetId());
			dclickEvent.SetEventObject(this);
			dclickEvent.SetInt(index);
			GetEventHandler()->ProcessEvent(dclickEvent);
		}
	}
	event.Skip();
}

void FindDialogGridCanvas::OnMotion(wxMouseEvent& event) {
	int index = HitTest(event.GetX(), event.GetY());
	if (index != m_hoverIndex) {
		m_hoverIndex = index;
		if (m_hoverIndex != -1) {
			SetCursor(wxCursor(wxCURSOR_HAND));
			SetToolTip(wxstr(brushlist[m_hoverIndex]->getName()));
		} else {
			SetCursor(wxNullCursor);
			SetToolTip("");
		}
		Refresh();
	}
	event.Skip();
}

void FindDialogGridCanvas::OnLeave(wxMouseEvent& event) {
	if (m_hoverIndex != -1) {
		m_hoverIndex = -1;
		Refresh();
	}
	event.Skip();
}

void FindDialogGridCanvas::OnKeyDown(wxKeyEvent& event) {
	// Forward some keys to parent if needed or handle up/down
	event.Skip();
}

void FindDialogGridCanvas::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	wxColour textColour = Theme::Get(Theme::Role::Text);
	if (no_matches) {
		nvgFontSize(vg, 12.0f);
		nvgFontFace(vg, "sans");
		nvgFillColor(vg, nvgRGBA(textColour.Red(), textColour.Green(), textColour.Blue(), 255));
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgText(vg, m_padding, m_padding + 16, "No matches for your search.", nullptr);
	} else if (cleared) {
		nvgFontSize(vg, 12.0f);
		nvgFontFace(vg, "sans");
		nvgFillColor(vg, nvgRGBA(textColour.Red(), textColour.Green(), textColour.Blue(), 255));
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgText(vg, m_padding, m_padding + 16, "Please enter your search string.", nullptr);
	} else {
		int scrollPos = GetScrollPosition();
		int rowHeight = m_itemSize + m_padding;
		int startRow = scrollPos / rowHeight;
		int endRow = (scrollPos + height + rowHeight - 1) / rowHeight;
		int startIdx = startRow * m_columns;
		int endIdx = std::min(static_cast<int>(brushlist.size()), (endRow + 1) * m_columns);

		for (int i = startIdx; i < endIdx; ++i) {
			if (i < 0) continue;

			wxRect r = GetItemRect(i);
			float x = r.x;
			float y = r.y;
			float w = r.width;
			float h = r.height;

			bool isSelected = (i == m_selection);
			bool isHovered = (i == m_hoverIndex);

			nvgBeginPath(vg);
			nvgRoundedRect(vg, x, y, w, h, 4.0f);
			if (isSelected) {
				wxColour selCol = Theme::Get(Theme::Role::Accent);
				nvgFillColor(vg, nvgRGBA(selCol.Red(), selCol.Green(), selCol.Blue(), 200));
			} else if (isHovered) {
				wxColour hoverCol = Theme::Get(Theme::Role::CardBaseHover);
				nvgFillColor(vg, nvgRGBA(hoverCol.Red(), hoverCol.Green(), hoverCol.Blue(), 255));
			} else {
				wxColour baseCol = Theme::Get(Theme::Role::CardBase);
				nvgFillColor(vg, nvgRGBA(baseCol.Red(), baseCol.Green(), baseCol.Blue(), 255));
			}
			nvgFill(vg);

			if (isSelected) {
				nvgBeginPath(vg);
				nvgRoundedRect(vg, x + 0.5f, y + 0.5f, w - 1, h - 1, 4.0f);
				wxColour accentCol = Theme::Get(Theme::Role::Accent);
				nvgStrokeColor(vg, nvgRGBA(accentCol.Red(), accentCol.Green(), accentCol.Blue(), 255));
				nvgStrokeWidth(vg, 2.0f);
				nvgStroke(vg);
			}

			Sprite* spr = g_gui.gfx.getSprite(brushlist[i]->getLookID());
			if (spr) {
				int tex = GetOrCreateSpriteTexture(vg, spr);
				if (tex > 0) {
					int tw, th;
					nvgImageSize(vg, tex, &tw, &th);
					float scale = std::min(w / tw, h / th);
					if (scale > 1.0f && std::max(tw, th) >= 32) scale = 1.0f;
					float dw = tw * scale;
					float dh = th * scale;
					float dx = x + (w - dw) / 2.0f;
					float dy = y + (h - dh) / 2.0f;

					NVGpaint imgPaint = nvgImagePattern(vg, dx, dy, dw, dh, 0.0f, tex, 1.0f);
					nvgBeginPath(vg);
					nvgRect(vg, dx, dy, dw, dh);
					nvgFillPaint(vg, imgPaint);
					nvgFill(vg);
				}
			}
		}
	}
}
