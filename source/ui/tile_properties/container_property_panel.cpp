//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "ui/tile_properties/container_property_panel.h"
#include "ui/properties/container_properties_window.h" // For ContainerItemButton
#include "game/complexitem.h"
#include "map/tile.h"
#include "map/map.h"
#include "ui/dialog_util.h"
#include "ui/find_item_window.h"
#include "ui/gui_ids.h"
#include "util/image_manager.h"
#include "ui/gui.h"
#include "editor/editor.h"
#include "editor/action.h"
#include "editor/action_queue.h"
#include "ui/properties/properties_window.h"
#include "ui/properties/old_properties_window.h"
#include "app/settings.h"

ContainerPropertyPanel::ContainerPropertyPanel(wxWindow* parent) :
	ItemPropertyPanel(parent), last_clicked_button(nullptr) {

	contents_sizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Contents");

	// Add contents_sizer to the main sizer (inherited from ItemPropertyPanel)
	GetSizer()->Add(contents_sizer, wxSizerFlags(1).Expand().Border(wxALL, 5));

	// Context menu events
	Bind(wxEVT_MENU, &ContainerPropertyPanel::OnAddItem, this, CONTAINER_POPUP_MENU_ADD);
	Bind(wxEVT_MENU, &ContainerPropertyPanel::OnEditItem, this, CONTAINER_POPUP_MENU_EDIT);
	Bind(wxEVT_MENU, &ContainerPropertyPanel::OnRemoveItem, this, CONTAINER_POPUP_MENU_REMOVE);
}

ContainerPropertyPanel::~ContainerPropertyPanel() {
}

void ContainerPropertyPanel::SetItem(Item* item, Tile* tile, Map* map) {
	// Call base class to show Action ID / Unique ID / etc.
	ItemPropertyPanel::SetItem(item, tile, map);

	RebuildGrid();
}

void ContainerPropertyPanel::RebuildGrid() {
	// Clear existing grid
	contents_sizer->Clear(true);
	container_items.clear();

	Container* container = current_item ? dynamic_cast<Container*>(current_item) : nullptr;

	if (container) {
		bool use_large_sprites = g_settings.getBoolean(Config::USE_LARGE_CONTAINER_ICONS);
		int32_t maxColumns = use_large_sprites ? 6 : 12;
		wxSizer* horizontal_sizer = nullptr;

		for (uint32_t index = 0; index < container->getVolume(); ++index) {
			if (!horizontal_sizer) {
				horizontal_sizer = newd wxBoxSizer(wxHORIZONTAL);
			}

			Item* sub_item = container->getItem(index);
			ContainerItemButton* btn = newd ContainerItemButton(this, use_large_sprites, index, current_map, sub_item);

			btn->Bind(wxEVT_BUTTON, &ContainerPropertyPanel::OnContainerItemClick, this);
			btn->Bind(wxEVT_RIGHT_UP, &ContainerPropertyPanel::OnContainerItemRightClick, this);

			container_items.push_back(btn);
			horizontal_sizer->Add(btn, wxSizerFlags(0).Border(wxALL, 1));

			if (((index + 1) % maxColumns) == 0) {
				contents_sizer->Add(horizontal_sizer);
				horizontal_sizer = nullptr;
			}
		}

		if (horizontal_sizer != nullptr) {
			contents_sizer->Add(horizontal_sizer);
		}
	}

	Layout();
}

void ContainerPropertyPanel::OnContainerItemClick(wxCommandEvent& event) {
	ContainerItemButton* button = dynamic_cast<ContainerItemButton*>(event.GetEventObject());
	if (!button) {
		return;
	}

	last_clicked_button = button;

	if (button->getItem()) {
		OnEditItem(event);
	} else {
		OnAddItem(event);
	}
}

void ContainerPropertyPanel::OnContainerItemRightClick(wxMouseEvent& event) {
	ContainerItemButton* button = dynamic_cast<ContainerItemButton*>(event.GetEventObject());
	if (!button) {
		return;
	}

	last_clicked_button = button;

	wxMenu menu;
	if (button->getItem()) {
		menu.Append(CONTAINER_POPUP_MENU_EDIT, "&Edit Item")->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_PEN_TO_SQUARE, wxSize(16, 16)));
		menu.Append(CONTAINER_POPUP_MENU_ADD, "&Add Item")->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_PLUS, wxSize(16, 16)));
		menu.Append(CONTAINER_POPUP_MENU_REMOVE, "&Remove Item")->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_MINUS, wxSize(16, 16)));
	} else {
		menu.Append(CONTAINER_POPUP_MENU_ADD, "&Add Item")->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_PLUS, wxSize(16, 16)));
	}

	Container* container = dynamic_cast<Container*>(current_item);
	if (container && container->getVolume() <= (int)container->getVector().size()) {
		if (wxMenuItem* addItem = menu.FindItem(CONTAINER_POPUP_MENU_ADD)) {
			addItem->Enable(false);
		}
	}

	PopupMenu(&menu);
}

void ContainerPropertyPanel::OnAddItem(wxCommandEvent& WXUNUSED(event)) {
	if (!last_clicked_button || !current_tile || !current_map) {
		return;
	}

	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) {
		return;
	}

	FindItemDialog dialog(this, "Select Item");
	if (dialog.ShowModal() == wxID_OK) {
		uint16_t item_id = dialog.getResultID();
		if (item_id != 0) {
			std::unique_ptr<Tile> new_tile = current_tile->deepCopy(*current_map);
			int index = current_tile->getIndexOf(current_item);
			if (index != -1) {
				Item* new_item_base = new_tile->getItemAt(index);
				if (new_item_base->asContainer()) {
					Container* container = static_cast<Container*>(new_item_base);
					auto& contents = container->getVector();
					uint32_t sub_index = last_clicked_button->getIndex();

					std::unique_ptr<Item> new_sub_item(Item::Create(item_id));
					if (new_sub_item) {
						if (sub_index < contents.size()) {
							contents.insert(contents.begin() + sub_index, std::move(new_sub_item));
						} else {
							contents.push_back(std::move(new_sub_item));
						}

						std::unique_ptr<Action> action = editor->actionQueue->createAction(ACTION_CHANGE_PROPERTIES);
						action->addChange(std::make_unique<Change>(std::move(new_tile)));
						editor->addAction(std::move(action));
					}
				}
			}
			RebuildGrid();
		}
	}
}

void ContainerPropertyPanel::OnEditItem(wxCommandEvent& WXUNUSED(event)) {
	if (!last_clicked_button || !last_clicked_button->getItem() || !current_map) {
		return;
	}

	Item* sub_item = last_clicked_button->getItem();
	wxPoint newDialogAt = GetPosition() + FROM_DIP(this, wxPoint(20, 20));

	wxDialog* d;
	if (current_map->getVersion().otbm >= MAP_OTBM_4) {
		d = newd PropertiesWindow(this, current_map, current_tile, sub_item, newDialogAt);
	} else {
		d = newd OldPropertiesWindow(this, current_map, current_tile, sub_item, newDialogAt);
	}

	if (d->ShowModal() == wxID_OK) {
		current_map->doChange();
		RebuildGrid();
	}
	d->Destroy();
}

void ContainerPropertyPanel::OnRemoveItem(wxCommandEvent& WXUNUSED(event)) {
	if (!last_clicked_button || !last_clicked_button->getItem() || !current_tile || !current_map) {
		return;
	}

	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) {
		return;
	}

	int32_t ret = DialogUtil::PopupDialog(this, "Remove Item", "Are you sure you want to remove this item from the container?", wxYES | wxNO);

	if (ret != wxID_YES) {
		return;
	}

	std::unique_ptr<Tile> new_tile = current_tile->deepCopy(*current_map);
	int index = current_tile->getIndexOf(current_item);
	if (index != -1) {
		Item* new_item_base = new_tile->getItemAt(index);
		if (new_item_base->asContainer()) {
			Container* container = static_cast<Container*>(new_item_base);
			auto& contents = container->getVector();
			Item* to_remove = last_clicked_button->getItem();

			// We need to find the item in the new container that corresponds to 'to_remove'
			// Since we know the index from the button, let's use it.
			uint32_t sub_index = last_clicked_button->getIndex();
			if (sub_index < contents.size()) {
				contents.erase(contents.begin() + sub_index);

				std::unique_ptr<Action> action = editor->actionQueue->createAction(ACTION_CHANGE_PROPERTIES);
				action->addChange(std::make_unique<Change>(std::move(new_tile)));
				editor->addAction(std::move(action));
			}
		}
	}

	RebuildGrid();
}
