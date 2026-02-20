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
	for (auto btn : container_items) {
		btn->Destroy();
	}
	container_items.clear();
	contents_sizer->Clear(true);

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
	if (!last_clicked_button) {
		return;
	}

	Container* container = dynamic_cast<Container*>(current_item);
	if (!container || !current_map) {
		return;
	}

	FindItemDialog dialog(this, "Select Item");
	if (dialog.ShowModal() == wxID_OK) {
		uint16_t item_id = dialog.getResultID();
		if (item_id != 0) {
			auto& contents = container->getVector();
			uint32_t index = last_clicked_button->getIndex();

			std::unique_ptr<Item> new_item(Item::Create(item_id));
			if (new_item) {
				if (index < contents.size()) {
					contents.insert(contents.begin() + index, std::move(new_item));
				} else {
					contents.push_back(std::move(new_item));
				}
				current_map->doChange();
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

	d->ShowModal();
	d->Destroy();
	current_map->doChange();
	RebuildGrid();
}

void ContainerPropertyPanel::OnRemoveItem(wxCommandEvent& WXUNUSED(event)) {
	if (!last_clicked_button || !last_clicked_button->getItem()) {
		return;
	}

	int32_t ret = DialogUtil::PopupDialog(this, "Remove Item", "Are you sure you want to remove this item from the container?", wxYES | wxNO);

	if (ret != wxID_YES) {
		return;
	}

	Container* container = dynamic_cast<Container*>(current_item);
	if (!container || !current_map) {
		return;
	}

	auto& contents = container->getVector();
	Item* to_remove = last_clicked_button->getItem();

	auto it = std::find_if(contents.begin(), contents.end(), [to_remove](const std::unique_ptr<Item>& item) {
		return item.get() == to_remove;
	});
	if (it != contents.end()) {
		contents.erase(it);
		current_map->doChange();
	}

	RebuildGrid();
}
