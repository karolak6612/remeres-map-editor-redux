//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "ui/replace_items_window.h"
#include "ui/find_item_window.h"
#include "editor/action_queue.h"
#include "rendering/core/graphics.h"
#include "ui/gui.h"
#include "util/image_manager.h"
#include "game/items.h"

#include <glad/glad.h>
#include <nanovg.h>
#include <format>

// ============================================================================
// ReplaceItemsButton

ReplaceItemsButton::ReplaceItemsButton(wxWindow* parent) :
	DCButton(parent, wxID_ANY, wxDefaultPosition, DC_BTN_TOGGLE, RENDER_SIZE_32x32, 0),
	m_id(0) {
	////
}

ItemGroup_t ReplaceItemsButton::GetGroup() const {
	if (m_id != 0) {
		const ItemType& it = g_items.getItemType(m_id);
		if (it.id != 0) {
			return it.group;
		}
	}
	return ITEM_GROUP_NONE;
}

void ReplaceItemsButton::SetItemId(uint16_t id) {
	if (m_id == id) {
		return;
	}

	m_id = id;

	if (m_id != 0) {
		const ItemType& it = g_items.getItemType(m_id);
		if (it.id != 0) {
			SetSprite(it.clientID);
			return;
		}
	}

	SetSprite(0);
}

// ============================================================================
// ReplaceItemsCanvas

ReplaceItemsCanvas::ReplaceItemsCanvas(wxWindow* parent) :
	NanoVGCanvas(parent, wxID_ANY, wxVSCROLL | wxWANTS_CHARS),
	m_selectedIndex(-1),
	m_hoverIndex(-1),
	m_itemHeight(50) {

	Bind(wxEVT_SIZE, &ReplaceItemsCanvas::OnSize, this);
	Bind(wxEVT_LEFT_DOWN, &ReplaceItemsCanvas::OnMouseDown, this);
	Bind(wxEVT_MOTION, &ReplaceItemsCanvas::OnMotion, this);
	Bind(wxEVT_LEAVE_WINDOW, &ReplaceItemsCanvas::OnLeave, this);
}

ReplaceItemsCanvas::~ReplaceItemsCanvas() {
}

bool ReplaceItemsCanvas::AddItem(const ReplacingItem& item) {
	if (item.replaceId == 0 || item.withId == 0 || item.replaceId == item.withId) {
		return false;
	}

	m_items.push_back(item);
	UpdateLayout();
	Refresh();

	return true;
}

void ReplaceItemsCanvas::MarkAsComplete(const ReplacingItem& item, uint32_t total) {
	auto it = std::find(m_items.begin(), m_items.end(), item);
	if (it != m_items.end()) {
		it->total = total;
		it->complete = true;
		Refresh();
	}
}

void ReplaceItemsCanvas::RemoveSelected() {
	if (m_items.empty() || m_selectedIndex == -1) {
		return;
	}

	if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_items.size())) {
		m_items.erase(m_items.begin() + m_selectedIndex);
		m_selectedIndex = -1;
		UpdateLayout();
		Refresh();

		// Notify parent that selection changed (cleared)
		wxCommandEvent event(wxEVT_LISTBOX, GetId());
		event.SetEventObject(this);
		GetEventHandler()->ProcessEvent(event);
	}
}

bool ReplaceItemsCanvas::CanAdd(uint16_t replaceId, uint16_t withId) const {
	if (replaceId == 0 || withId == 0 || replaceId == withId) {
		return false;
	}

	for (const ReplacingItem& item : m_items) {
		if (replaceId == item.replaceId) {
			return false;
		}
	}
	return true;
}

void ReplaceItemsCanvas::UpdateLayout() {
	int rows = static_cast<int>(m_items.size());
	UpdateScrollbar(rows * m_itemHeight);
}

void ReplaceItemsCanvas::OnSize(wxSizeEvent& event) {
	UpdateLayout();
	Refresh();
	event.Skip();
}

wxSize ReplaceItemsCanvas::DoGetBestClientSize() const {
	return FromDIP(wxSize(480, 320));
}

int ReplaceItemsCanvas::HitTest(int x, int y) {
	int scrollPos = GetScrollPosition();
	int realY = y + scrollPos;
	int index = realY / m_itemHeight;

	if (index >= 0 && index < static_cast<int>(m_items.size())) {
		return index;
	}
	return -1;
}

void ReplaceItemsCanvas::OnMouseDown(wxMouseEvent& event) {
	int index = HitTest(event.GetX(), event.GetY());
	if (index != m_selectedIndex) {
		m_selectedIndex = index;
		Refresh();

		wxCommandEvent cmdEvent(wxEVT_LISTBOX, GetId());
		cmdEvent.SetEventObject(this);
		GetEventHandler()->ProcessEvent(cmdEvent);
	}
	event.Skip();
}

void ReplaceItemsCanvas::OnMotion(wxMouseEvent& event) {
	int index = HitTest(event.GetX(), event.GetY());
	if (index != m_hoverIndex) {
		m_hoverIndex = index;
		if (m_hoverIndex != -1) {
			SetCursor(wxCursor(wxCURSOR_HAND));
		} else {
			SetCursor(wxNullCursor);
		}
		Refresh();
	}
	event.Skip();
}

void ReplaceItemsCanvas::OnLeave(wxMouseEvent& event) {
	if (m_hoverIndex != -1) {
		m_hoverIndex = -1;
		Refresh();
	}
	event.Skip();
}

void ReplaceItemsCanvas::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	int scrollPos = GetScrollPosition();

	// Background
	wxColour bgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX);
	nvgBeginPath(vg);
	nvgRect(vg, 0, scrollPos, width, height);
	nvgFillColor(vg, nvgRGBA(bgCol.Red(), bgCol.Green(), bgCol.Blue(), 255));
	nvgFill(vg);

	int startRow = scrollPos / m_itemHeight;
	int endRow = (scrollPos + height + m_itemHeight - 1) / m_itemHeight;
	int count = static_cast<int>(m_items.size());

	int startIdx = std::max(0, startRow);
	int endIdx = std::min(count, endRow + 1);

	wxColour selCol = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
	NVGcolor selColor = nvgRGBA(selCol.Red(), selCol.Green(), selCol.Blue(), 255);

	wxColour textCol = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT);
	NVGcolor textColor = nvgRGBA(textCol.Red(), textCol.Green(), textCol.Blue(), 255);

	wxColour selTextCol = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);
	NVGcolor selTextColor = nvgRGBA(selTextCol.Red(), selTextCol.Green(), selTextCol.Blue(), 255);

	wxColour borderCol = wxSystemSettings::GetColour(wxSYS_COLOUR_3DLIGHT);
	NVGcolor borderColor = nvgRGBA(borderCol.Red(), borderCol.Green(), borderCol.Blue(), 255);

	// Pre-calc lighter/darker shades for alternating rows if needed, or stick to solid
	// Let's use simple solid background for standard list look, with selection

	for (int i = startIdx; i < endIdx; ++i) {
		int y = i * m_itemHeight;
		const ReplacingItem& item = m_items[i];

		bool isSelected = (i == m_selectedIndex);
		bool isHovered = (i == m_hoverIndex);

		// Background
		nvgBeginPath(vg);
		nvgRect(vg, 0, y, width, m_itemHeight);
		if (isSelected) {
			nvgFillColor(vg, selColor);
		} else if (isHovered) {
			// Slight hover effect
			nvgFillColor(vg, nvgRGBA(selCol.Red(), selCol.Green(), selCol.Blue(), 50));
		} else {
			// Alternating
			/*if (i % 2 == 1) {
				nvgFillColor(vg, nvgRGBA(bgCol.Red() * 0.95, bgCol.Green() * 0.95, bgCol.Blue() * 0.95, 255));
			} else {
				nvgFillColor(vg, nvgRGBA(bgCol.Red(), bgCol.Green(), bgCol.Blue(), 255));
			}*/
			// Actually just standard background is fine for cleaner look
			nvgFillColor(vg, nvgRGBA(bgCol.Red(), bgCol.Green(), bgCol.Blue(), 255));
		}
		nvgFill(vg);

		// Separator
		nvgBeginPath(vg);
		nvgMoveTo(vg, 0, y + m_itemHeight - 0.5f);
		nvgLineTo(vg, width, y + m_itemHeight - 0.5f);
		nvgStrokeColor(vg, borderColor);
		nvgStrokeWidth(vg, 1.0f);
		nvgStroke(vg);

		// Sprites
		int tex1 = GetOrCreateItemImage(item.replaceId);
		int tex2 = GetOrCreateItemImage(item.withId);

		float imgSize = 32.0f;
		float startX = 10.0f;
		float startY = y + (m_itemHeight - imgSize) / 2.0f;

		// Sprite 1
		if (tex1 > 0) {
			NVGpaint imgPaint = nvgImagePattern(vg, startX, startY, imgSize, imgSize, 0, tex1, 1.0f);
			nvgBeginPath(vg);
			nvgRect(vg, startX, startY, imgSize, imgSize);
			nvgFillPaint(vg, imgPaint);
			nvgFill(vg);
		}

		// Arrow
		nvgFontSize(vg, 20.0f);
		nvgFontFace(vg, "sans-bold");
		if (isSelected) {
			nvgFillColor(vg, selTextColor);
		} else {
			nvgFillColor(vg, nvgRGBA(150, 150, 150, 255));
		}
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgText(vg, startX + imgSize + 20, y + m_itemHeight / 2.0f, "->", nullptr);

		// Sprite 2
		float x2 = startX + imgSize + 40;
		if (tex2 > 0) {
			NVGpaint imgPaint = nvgImagePattern(vg, x2, startY, imgSize, imgSize, 0, tex2, 1.0f);
			nvgBeginPath(vg);
			nvgRect(vg, x2, startY, imgSize, imgSize);
			nvgFillPaint(vg, imgPaint);
			nvgFill(vg);
		}

		// Text
		float tx = x2 + imgSize + 15;
		nvgFontSize(vg, 14.0f);
		nvgFontFace(vg, "sans");
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		if (isSelected) {
			nvgFillColor(vg, selTextColor);
		} else {
			nvgFillColor(vg, textColor);
		}

		std::string label = std::format("Replace: {} With: {}", item.replaceId, item.withId);
		nvgText(vg, tx, y + m_itemHeight / 2.0f, label.c_str(), nullptr);

		// Completion status
		if (item.complete) {
			std::string status = std::format("Total: {}", item.total);
			nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
			if (isSelected) {
				nvgFillColor(vg, nvgRGBA(150, 255, 150, 255));
			} else {
				nvgFillColor(vg, nvgRGBA(50, 180, 50, 255));
			}
			nvgText(vg, width - 10, y + m_itemHeight / 2.0f, status.c_str(), nullptr);
		}
	}
}

// ============================================================================
// ReplaceItemsDialog

ReplaceItemsDialog::ReplaceItemsDialog(wxWindow* parent, bool selectionOnly) :
	wxDialog(parent, wxID_ANY, (selectionOnly ? "Replace Items on Selection" : "Replace Items"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE),
	selectionOnly(selectionOnly) {
	SetSizeHints(wxDefaultSize, wxDefaultSize);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	list = new ReplaceItemsCanvas(this);
	list->SetMinSize(FromDIP(wxSize(480, 320)));

	sizer->Add(list, 1, wxALL | wxEXPAND, 5);

	wxBoxSizer* items_sizer = new wxBoxSizer(wxHORIZONTAL);
	items_sizer->SetMinSize(wxSize(-1, FromDIP(40)));

	replace_button = new ReplaceItemsButton(this);
	items_sizer->Add(replace_button, 0, wxALL, 5);

	wxBitmap bitmap = IMAGE_MANAGER.GetBitmap(ICON_LOCATION_ARROW, FromDIP(wxSize(16, 16)));
	arrow_bitmap = new wxStaticBitmap(this, wxID_ANY, bitmap);
	items_sizer->Add(arrow_bitmap, 0, wxTOP, 15);

	with_button = new ReplaceItemsButton(this);
	items_sizer->Add(with_button, 0, wxALL, 5);

	items_sizer->Add(0, 0, 1, wxEXPAND, 5);

	progress = new wxGauge(this, wxID_ANY, 100);
	progress->SetValue(0);
	items_sizer->Add(progress, 0, wxALL, 5);

	sizer->Add(items_sizer, 1, wxALL | wxEXPAND, 5);

	wxBoxSizer* buttons_sizer = new wxBoxSizer(wxHORIZONTAL);

	add_button = new wxButton(this, wxID_ANY, "Add");
	add_button->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_PLUS, wxSize(16, 16)));
	add_button->SetToolTip("Add replacement rule to list");
	add_button->Enable(false);
	buttons_sizer->Add(add_button, 0, wxALL, 5);

	remove_button = new wxButton(this, wxID_ANY, "Remove");
	remove_button->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_MINUS, wxSize(16, 16)));
	remove_button->SetToolTip("Remove selected rule");
	remove_button->Enable(false);
	buttons_sizer->Add(remove_button, 0, wxALL, 5);

	buttons_sizer->Add(0, 0, 1, wxEXPAND, 5);

	execute_button = new wxButton(this, wxID_ANY, "Execute");
	execute_button->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_PLAY, wxSize(16, 16)));
	execute_button->SetToolTip("Execute all replacement rules");
	execute_button->Enable(false);
	buttons_sizer->Add(execute_button, 0, wxALL, 5);

	close_button = new wxButton(this, wxID_ANY, "Close");
	close_button->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_XMARK, wxSize(16, 16)));
	close_button->SetToolTip("Close this window");
	buttons_sizer->Add(close_button, 0, wxALL, 5);

	sizer->Add(buttons_sizer, 1, wxALL | wxLEFT | wxRIGHT | wxSHAPED, 5);

	SetSizer(sizer);
	Layout();
	Centre(wxBOTH);

	// Connect Events
	// Note: ReplaceItemsCanvas emits wxEVT_LISTBOX with its ID
	list->Bind(wxEVT_LISTBOX, &ReplaceItemsDialog::OnListSelected, this);

	replace_button->Bind(wxEVT_LEFT_DOWN, &ReplaceItemsDialog::OnReplaceItemClicked, this);
	with_button->Bind(wxEVT_LEFT_DOWN, &ReplaceItemsDialog::OnWithItemClicked, this);
	add_button->Bind(wxEVT_BUTTON, &ReplaceItemsDialog::OnAddButtonClicked, this);
	remove_button->Bind(wxEVT_BUTTON, &ReplaceItemsDialog::OnRemoveButtonClicked, this);
	execute_button->Bind(wxEVT_BUTTON, &ReplaceItemsDialog::OnExecuteButtonClicked, this);
	close_button->Bind(wxEVT_BUTTON, &ReplaceItemsDialog::OnCancelButtonClicked, this);

	wxIcon icon;
	icon.CopyFromBitmap(IMAGE_MANAGER.GetBitmap(ICON_SYNC, wxSize(32, 32)));
	SetIcon(icon);
}

ReplaceItemsDialog::~ReplaceItemsDialog() {
}

void ReplaceItemsDialog::UpdateWidgets() {
	const uint16_t replaceId = replace_button->GetItemId();
	const uint16_t withId = with_button->GetItemId();
	add_button->Enable(list->CanAdd(replaceId, withId));
	if (add_button->IsEnabled()) {
		add_button->SetToolTip("Add replacement rule to list");
	} else {
		add_button->SetToolTip("Select replacement and target items to add.");
	}

	remove_button->Enable(list->GetCount() != 0 && list->GetSelection() != -1);
	if (remove_button->IsEnabled()) {
		remove_button->SetToolTip("Remove selected rule");
	} else {
		remove_button->SetToolTip("Select a rule to remove.");
	}

	execute_button->Enable(list->GetCount() != 0);
	if (execute_button->IsEnabled()) {
		execute_button->SetToolTip("Execute all replacement rules");
	} else {
		execute_button->SetToolTip("Add rules to list first.");
	}
}

void ReplaceItemsDialog::OnListSelected(wxCommandEvent& WXUNUSED(event)) {
	remove_button->Enable(list->GetCount() != 0 && list->GetSelection() != -1);
}

void ReplaceItemsDialog::OnReplaceItemClicked(wxMouseEvent& WXUNUSED(event)) {
	FindItemDialog dialog(this, "Replace Item");
	if (dialog.ShowModal() == wxID_OK) {
		uint16_t id = dialog.getResultID();
		if (id != with_button->GetItemId()) {
			replace_button->SetItemId(id);
			UpdateWidgets();
		}
	}
	dialog.Destroy();
}

void ReplaceItemsDialog::OnWithItemClicked(wxMouseEvent& WXUNUSED(event)) {
	if (replace_button->GetItemId() == 0) {
		return;
	}

	FindItemDialog dialog(this, "With Item");
	if (dialog.ShowModal() == wxID_OK) {
		uint16_t id = dialog.getResultID();
		if (id != replace_button->GetItemId()) {
			with_button->SetItemId(id);
			UpdateWidgets();
		}
	}
	dialog.Destroy();
}

void ReplaceItemsDialog::OnAddButtonClicked(wxCommandEvent& WXUNUSED(event)) {
	const uint16_t replaceId = replace_button->GetItemId();
	const uint16_t withId = with_button->GetItemId();
	if (list->CanAdd(replaceId, withId)) {
		ReplacingItem item;
		item.replaceId = replaceId;
		item.withId = withId;
		if (list->AddItem(item)) {
			replace_button->SetItemId(0);
			with_button->SetItemId(0);
			UpdateWidgets();
		}
	}
}

void ReplaceItemsDialog::OnRemoveButtonClicked(wxCommandEvent& WXUNUSED(event)) {
	list->RemoveSelected();
	UpdateWidgets();
}

void ReplaceItemsDialog::OnExecuteButtonClicked(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	const auto& items = list->GetItems();
	if (items.empty()) {
		return;
	}

	replace_button->Enable(false);
	with_button->Enable(false);
	add_button->Enable(false);
	remove_button->Enable(false);
	execute_button->Enable(false);
	close_button->Enable(false);
	progress->SetValue(0);

	MapTab* tab = dynamic_cast<MapTab*>(GetParent());
	if (!tab) {
		return;
	}

	Editor* editor = tab->GetEditor();

	int done = 0;
	for (const ReplacingItem& info : items) {
		ItemFinder finder(info.replaceId, (uint32_t)g_settings.getInteger(Config::REPLACE_SIZE));

		// search on map
		foreach_ItemOnMap(editor->map, finder, selectionOnly);

		uint32_t total = 0;
		std::vector<std::pair<Tile*, Item*>>& result = finder.result;

		if (!result.empty()) {
			std::unique_ptr<Action> action = editor->actionQueue->createAction(ACTION_REPLACE_ITEMS);
			for (std::vector<std::pair<Tile*, Item*>>::const_iterator rit = result.begin(); rit != result.end(); ++rit) {
				std::unique_ptr<Tile> new_tile = rit->first->deepCopy(editor->map);
				int index = rit->first->getIndexOf(rit->second);
				ASSERT(index != wxNOT_FOUND);
				Item* item = new_tile->getItemAt(index);
				ASSERT(item && item->getID() == rit->second->getID());
				transformItem(item, info.withId, new_tile.get());
				action->addChange(std::make_unique<Change>(new_tile.release()));
				total++;
			}
			editor->actionQueue->addAction(std::move(action));
		}

		done++;
		const int value = static_cast<int>((done / items.size()) * 100);
		progress->SetValue(std::clamp<int>(value, 0, 100));
		list->MarkAsComplete(info, total);
	}

	tab->Refresh();
	close_button->Enable(true);
	UpdateWidgets();
}

void ReplaceItemsDialog::OnCancelButtonClicked(wxCommandEvent& WXUNUSED(event)) {
	Close();
}
