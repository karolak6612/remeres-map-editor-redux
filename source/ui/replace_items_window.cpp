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
#include <nanovg.h>
#include <format>
#include "ui/theme.h"

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
// ReplaceItemsListBox

ReplaceItemsListBox::ReplaceItemsListBox(wxWindow* parent) :
	NanoVGListBox(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLB_SINGLE) {
}

bool ReplaceItemsListBox::AddItem(const ReplacingItem& item) {
	if (item.replaceId == 0 || item.withId == 0 || item.replaceId == item.withId) {
		return false;
	}

	m_items.push_back(item);
	SetItemCount(m_items.size()); // Base class handles refresh and scroll update

	return true;
}

void ReplaceItemsListBox::MarkAsComplete(const ReplacingItem& item, uint32_t total) {
	auto it = std::find(m_items.begin(), m_items.end(), item);
	if (it != m_items.end()) {
		it->total = total;
		it->complete = true;
		Refresh();
	}
}

void ReplaceItemsListBox::RemoveSelected() {
	if (m_items.empty()) {
		return;
	}

	const int index = GetSelection();
	if (index == wxNOT_FOUND) {
		return;
	}

	m_items.erase(m_items.begin() + index);
	SetItemCount(m_items.size());
	// Selection correction is handled by NanoVGListBox::SetItemCount (resets selection usually, or we might want to select next?)
	// NanoVGListBox::SetItemCount resets selection to none (-1).
	// Ideally we should select index if valid, or index-1.
	if (!m_items.empty()) {
		int newSel = std::min(index, (int)m_items.size() - 1);
		SetSelection(newSel);
	}
}

bool ReplaceItemsListBox::CanAdd(uint16_t replaceId, uint16_t withId) const {
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

void ReplaceItemsListBox::OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t index) const {
	ASSERT(index < m_items.size());

	const ReplacingItem& item = m_items.at(index);
	const ItemType& type1 = g_items.getItemType(item.replaceId);
	Sprite* sprite1 = g_gui.gfx.getSprite(type1.clientID);
	const ItemType& type2 = g_items.getItemType(item.withId);
	Sprite* sprite2 = g_gui.gfx.getSprite(type2.clientID);

	// Setup text style
	nvgFontSize(vg, (float)FROM_DIP(this, 12));
	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));

	if (sprite1 && sprite2) {
		int x = rect.x;
		int y = rect.y;
		int size = FROM_DIP(this, 32);

		// Item 1
		int tex1 = const_cast<NanoVGCanvas*>(static_cast<const NanoVGCanvas*>(this))->GetOrCreateSpriteTexture(vg, sprite1);
		if (tex1 > 0) {
			NVGpaint imgPaint = nvgImagePattern(vg, x + 4, y + 4, size, size, 0, tex1, 1.0f);
			nvgBeginPath(vg);
			nvgRect(vg, x + 4, y + 4, size, size);
			nvgFillPaint(vg, imgPaint);
			nvgFill(vg);
		}

		// Arrow
		int arrowImg = IMAGE_MANAGER.GetNanoVGImage(vg, ICON_LOCATION_ARROW, Theme::Get(Theme::Role::Icon));
		if (arrowImg > 0) {
			int iconSize = FROM_DIP(this, 16);
			NVGpaint imgPaint = nvgImagePattern(vg, x + 38, y + 10, iconSize, iconSize, 0, arrowImg, 1.0f);
			nvgBeginPath(vg);
			nvgRect(vg, x + 38, y + 10, iconSize, iconSize);
			nvgFillPaint(vg, imgPaint);
			nvgFill(vg);
		}

		// Item 2
		int tex2 = const_cast<NanoVGCanvas*>(static_cast<const NanoVGCanvas*>(this))->GetOrCreateSpriteTexture(vg, sprite2);
		if (tex2 > 0) {
			NVGpaint imgPaint = nvgImagePattern(vg, x + 56, y + 4, size, size, 0, tex2, 1.0f);
			nvgBeginPath(vg);
			nvgRect(vg, x + 56, y + 4, size, size);
			nvgFillPaint(vg, imgPaint);
			nvgFill(vg);
		}

		// Text
		std::string text = std::format("Replace: {} With: {}", item.replaceId, item.withId);
		nvgText(vg, x + 104, y + rect.height / 2.0f, text.c_str(), nullptr);

		if (item.complete) {
			int flagX = rect.width - 100;

			// Flag Icon
			int flagImg = IMAGE_MANAGER.GetNanoVGImage(vg, IMAGE_PROTECTION_ZONE_SMALL);
			if (flagImg > 0) {
				int iconSize = FROM_DIP(this, 16);
				NVGpaint imgPaint = nvgImagePattern(vg, flagX + 70, y + 10, iconSize, iconSize, 0, flagImg, 1.0f);
				nvgBeginPath(vg);
				nvgRect(vg, flagX + 70, y + 10, iconSize, iconSize);
				nvgFillPaint(vg, imgPaint);
				nvgFill(vg);
			}

			std::string totalText = std::format("Total: {}", item.total);
			nvgText(vg, flagX, y + rect.height / 2.0f, totalText.c_str(), nullptr);
		}
	}
}

wxCoord ReplaceItemsListBox::OnMeasureItem(size_t WXUNUSED(index)) const {
	return FromDIP(40);
}

// ============================================================================
// ReplaceItemsDialog

ReplaceItemsDialog::ReplaceItemsDialog(wxWindow* parent, bool selectionOnly) :
	wxDialog(parent, wxID_ANY, (selectionOnly ? "Replace Items on Selection" : "Replace Items"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE),
	selectionOnly(selectionOnly) {
	SetSizeHints(wxDefaultSize, wxDefaultSize);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	// Using wxWrapSizer or keeping FlexGridSizer?
	// The original code used wxFlexGridSizer but effectively as a box.
	// We can simplify this to wxBoxSizer or just add list directly to sizer.

	list = new ReplaceItemsListBox(this);
	list->SetMinSize(FromDIP(wxSize(480, 320)));

	sizer->Add(list, 1, wxALL | wxEXPAND, 5); // Expanded to take space

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

	sizer->Add(items_sizer, 0, wxALL | wxEXPAND, 5);

	wxBoxSizer* buttons_sizer = new wxBoxSizer(wxHORIZONTAL);

	add_button = new wxButton(this, wxID_ANY, "Add");
	add_button->SetToolTip("Add replacement rule to list");
	add_button->Enable(false);
	buttons_sizer->Add(add_button, 0, wxALL, 5);

	remove_button = new wxButton(this, wxID_ANY, "Remove");
	remove_button->SetToolTip("Remove selected rule");
	remove_button->Enable(false);
	buttons_sizer->Add(remove_button, 0, wxALL, 5);

	buttons_sizer->Add(0, 0, 1, wxEXPAND, 5);

	execute_button = new wxButton(this, wxID_ANY, "Execute");
	execute_button->SetToolTip("Execute all replacement rules");
	execute_button->Enable(false);
	buttons_sizer->Add(execute_button, 0, wxALL, 5);

	close_button = new wxButton(this, wxID_ANY, "Close");
	close_button->SetToolTip("Close this window");
	buttons_sizer->Add(close_button, 0, wxALL, 5);

	sizer->Add(buttons_sizer, 0, wxALL | wxLEFT | wxRIGHT | wxSHAPED, 5);

	SetSizer(sizer);
	Layout();
	Centre(wxBOTH);

	// Connect Events
	list->Bind(wxEVT_LISTBOX, &ReplaceItemsDialog::OnListSelected, this);
	replace_button->Bind(wxEVT_LEFT_DOWN, &ReplaceItemsDialog::OnReplaceItemClicked, this);
	with_button->Bind(wxEVT_LEFT_DOWN, &ReplaceItemsDialog::OnWithItemClicked, this);
	add_button->Bind(wxEVT_BUTTON, &ReplaceItemsDialog::OnAddButtonClicked, this);
	remove_button->Bind(wxEVT_BUTTON, &ReplaceItemsDialog::OnRemoveButtonClicked, this);
	execute_button->Bind(wxEVT_BUTTON, &ReplaceItemsDialog::OnExecuteButtonClicked, this);
	close_button->Bind(wxEVT_BUTTON, &ReplaceItemsDialog::OnCancelButtonClicked, this);
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

	remove_button->Enable(list->GetCount() != 0 && list->GetSelection() != wxNOT_FOUND);
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
	remove_button->Enable(list->GetCount() != 0 && list->GetSelection() != wxNOT_FOUND);
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
		const int value = static_cast<int>((static_cast<float>(done) / items.size()) * 100);
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
