#include "ui/windows/item_config_window.h"
#include "ui/gui.h"
#include "game/items.h"
#include "game/item.h" // For ItemType
#include "ui/find_item_window.h"

#include <wx/msgdlg.h>

BEGIN_EVENT_TABLE(ItemConfigWindow, wxDialog)
    EVT_BUTTON(wxID_ADD, ItemConfigWindow::OnAdd)
    EVT_BUTTON(wxID_EDIT, ItemConfigWindow::OnEdit)
    EVT_BUTTON(wxID_REMOVE, ItemConfigWindow::OnRemove)
    EVT_BUTTON(wxID_SAVE, ItemConfigWindow::OnSave)
    EVT_BUTTON(wxID_CLOSE, ItemConfigWindow::OnClose)
END_EVENT_TABLE()

ItemConfigWindow::ItemConfigWindow(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Technical Items Configuration", wxDefaultPosition, wxSize(600, 400), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    itemList = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    itemList->InsertColumn(0, "ID", wxLIST_FORMAT_LEFT, 60);
    itemList->InsertColumn(1, "Name", wxLIST_FORMAT_LEFT, 150);
    itemList->InsertColumn(2, "Technical Type", wxLIST_FORMAT_LEFT, 150);
    itemList->InsertColumn(3, "Disguise", wxLIST_FORMAT_LEFT, 150);
    mainSizer->Add(itemList, 1, wxEXPAND | wxALL, 5);

    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnAdd = new wxButton(this, wxID_ADD, "Add");
    btnEdit = new wxButton(this, wxID_EDIT, "Edit");
    btnRemove = new wxButton(this, wxID_REMOVE, "Remove");
    btnSave = new wxButton(this, wxID_SAVE, "Save to File");
    btnClose = new wxButton(this, wxID_CLOSE, "Close");

    btnSizer->Add(btnAdd, 0, wxALL, 5);
    btnSizer->Add(btnEdit, 0, wxALL, 5);
    btnSizer->Add(btnRemove, 0, wxALL, 5);
    btnSizer->AddStretchSpacer();
    btnSizer->Add(btnSave, 0, wxALL, 5);
    btnSizer->Add(btnClose, 0, wxALL, 5);

    mainSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 5);

    SetSizer(mainSizer);
    RefreshList();
}

ItemConfigWindow::~ItemConfigWindow() {
}

void ItemConfigWindow::RefreshList() {
    itemList->DeleteAllItems();

    const auto& map = g_gui.itemMetadataManager->getAllMetadata();
    for (const auto& pair : map) {
        long index = itemList->InsertItem(itemList->GetItemCount(), wxString::Format("%d", pair.first));

        ItemType& it = g_items.getItemType(pair.first);
        itemList->SetItem(index, 1, it.name);

        wxString typeStr;
        switch (pair.second.techType) {
            case TechItemType::NONE: typeStr = "None"; break;
            case TechItemType::INVISIBLE_STAIRS: typeStr = "Invisible Stairs"; break;
            case TechItemType::INVISIBLE_WALKABLE: typeStr = "Invisible Walkable"; break;
            case TechItemType::INVISIBLE_WALL: typeStr = "Invisible Wall"; break;
            case TechItemType::PRIMAL_LIGHT: typeStr = "Primal Light"; break;
            case TechItemType::CLIENT_ID_ZERO: typeStr = "Invalid ID (Zero)"; break;
        }
        itemList->SetItem(index, 2, typeStr);

        wxString disguiseStr = "None";
        if (pair.second.disguiseID != 0) {
            ItemType& dit = g_items.getItemType(pair.second.disguiseID);
            disguiseStr = wxString::Format("%d (%s)", pair.second.disguiseID, dit.name);
        }
        itemList->SetItem(index, 3, disguiseStr);

        // Store ID as item data for easy retrieval
        itemList->SetItemData(index, pair.first);
    }
}

void ItemConfigWindow::OnAdd(wxCommandEvent& event) {
    ItemConfigDialog dlg(this);
    if (dlg.ShowModal() == wxID_OK) {
        uint16_t id = dlg.GetItemID();
        if (id != 0) {
            g_gui.itemMetadataManager->setMetadata(id, dlg.GetMetadata());
            RefreshList();
        }
    }
}

void ItemConfigWindow::OnEdit(wxCommandEvent& event) {
    long itemIndex = itemList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (itemIndex == -1) return;

    uint16_t id = (uint16_t)itemList->GetItemData(itemIndex);
    ItemMetadata meta = g_gui.itemMetadataManager->getMetadata(id);

    ItemConfigDialog dlg(this, id, meta);
    if (dlg.ShowModal() == wxID_OK) {
        // The ID might have changed if the user selected a different item,
        // but typically Edit keeps the same item or replaces entry.
        // Our dialog allows changing item, so we remove old and add new.

        if (dlg.GetItemID() != id) {
             g_gui.itemMetadataManager->removeMetadata(id);
        }
        g_gui.itemMetadataManager->setMetadata(dlg.GetItemID(), dlg.GetMetadata());
        RefreshList();
    }
}

void ItemConfigWindow::OnRemove(wxCommandEvent& event) {
    long itemIndex = itemList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (itemIndex == -1) return;

    uint16_t id = (uint16_t)itemList->GetItemData(itemIndex);

    if (wxMessageBox("Are you sure you want to remove this configuration?", "Confirm Remove", wxYES_NO | wxICON_QUESTION, this) == wxYES) {
        g_gui.itemMetadataManager->removeMetadata(id);
        RefreshList();
    }
}

void ItemConfigWindow::OnSave(wxCommandEvent& event) {
    std::string path = g_gui.m_dataDirectory.ToStdString() + "technical-items.xml";
    if (g_gui.itemMetadataManager->save(path)) {
        wxMessageBox("Configuration saved successfully.", "Success", wxOK | wxICON_INFORMATION, this);
    } else {
        wxMessageBox("Failed to save configuration.", "Error", wxOK | wxICON_ERROR, this);
    }
}

void ItemConfigWindow::OnClose(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

// ----------------------------------------------------------------------------
// ItemConfigDialog
// ----------------------------------------------------------------------------

enum {
    ID_BTN_SELECT_ITEM = 1001,
    ID_BTN_SELECT_DISGUISE,
    ID_BTN_CLEAR_DISGUISE
};

BEGIN_EVENT_TABLE(ItemConfigDialog, wxDialog)
    EVT_BUTTON(ID_BTN_SELECT_ITEM, ItemConfigDialog::OnSelectItem)
    EVT_BUTTON(ID_BTN_SELECT_DISGUISE, ItemConfigDialog::OnSelectDisguise)
    EVT_BUTTON(ID_BTN_CLEAR_DISGUISE, ItemConfigDialog::OnClearDisguise)
END_EVENT_TABLE()

ItemConfigDialog::ItemConfigDialog(wxWindow* parent, uint16_t itemId, const ItemMetadata& meta)
    : wxDialog(parent, wxID_ANY, "Edit Item Configuration", wxDefaultPosition, wxSize(400, 300)),
      selectedItemID(itemId), disguiseID(meta.disguiseID)
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Item Selection
    wxStaticBoxSizer* itemBox = new wxStaticBoxSizer(wxVERTICAL, this, "Item");
    lblItem = new wxStaticText(this, wxID_ANY, "No Item Selected");
    if (selectedItemID != 0) {
        ItemType& it = g_items.getItemType(selectedItemID);
        lblItem->SetLabel(wxString::Format("%d: %s", selectedItemID, it.name));
    }
    btnSelectItem = new wxButton(this, ID_BTN_SELECT_ITEM, "Select Item...");
    itemBox->Add(lblItem, 0, wxALL, 5);
    itemBox->Add(btnSelectItem, 0, wxALL, 5);
    mainSizer->Add(itemBox, 0, wxEXPAND | wxALL, 5);

    // Tech Type
    wxStaticBoxSizer* typeBox = new wxStaticBoxSizer(wxVERTICAL, this, "Technical Type");
    wxArrayString choices;
    choices.Add("None");
    choices.Add("Invisible Stairs");
    choices.Add("Invisible Walkable");
    choices.Add("Invisible Wall");
    choices.Add("Primal Light");
    choices.Add("Invalid ID (Zero)");

    choiceTechType = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, choices);
    choiceTechType->SetSelection((int)meta.techType);
    typeBox->Add(choiceTechType, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(typeBox, 0, wxEXPAND | wxALL, 5);

    // Disguise
    wxStaticBoxSizer* disguiseBox = new wxStaticBoxSizer(wxVERTICAL, this, "Disguise (Visual Override)");
    lblDisguise = new wxStaticText(this, wxID_ANY, "None");
    if (disguiseID != 0) {
        ItemType& it = g_items.getItemType(disguiseID);
        lblDisguise->SetLabel(wxString::Format("%d: %s", disguiseID, it.name));
    }
    btnSelectDisguise = new wxButton(this, ID_BTN_SELECT_DISGUISE, "Select Disguise...");
    btnClearDisguise = new wxButton(this, ID_BTN_CLEAR_DISGUISE, "Clear");

    wxBoxSizer* disguiseBtnSizer = new wxBoxSizer(wxHORIZONTAL);
    disguiseBtnSizer->Add(btnSelectDisguise, 1, wxRIGHT, 5);
    disguiseBtnSizer->Add(btnClearDisguise, 0);

    disguiseBox->Add(lblDisguise, 0, wxALL, 5);
    disguiseBox->Add(disguiseBtnSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(disguiseBox, 0, wxEXPAND | wxALL, 5);

    // Dialog Buttons
    mainSizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 10);

    SetSizer(mainSizer);
}

ItemConfigDialog::~ItemConfigDialog() {
}

uint16_t ItemConfigDialog::GetItemID() const {
    return selectedItemID;
}

ItemMetadata ItemConfigDialog::GetMetadata() const {
    ItemMetadata meta;
    meta.techType = (TechItemType)choiceTechType->GetSelection();
    meta.disguiseID = disguiseID;
    return meta;
}

void ItemConfigDialog::OnSelectItem(wxCommandEvent& event) {
    FindItemDialog fid(this, "Select Item");
    if (fid.ShowModal() == wxID_OK) {
        selectedItemID = fid.getResultID();
        ItemType& it = g_items.getItemType(selectedItemID);
        lblItem->SetLabel(wxString::Format("%d: %s", selectedItemID, it.name));
    }
}

void ItemConfigDialog::OnSelectDisguise(wxCommandEvent& event) {
    FindItemDialog fid(this, "Select Disguise Item");
    if (fid.ShowModal() == wxID_OK) {
        disguiseID = fid.getResultID();
        ItemType& it = g_items.getItemType(disguiseID);
        lblDisguise->SetLabel(wxString::Format("%d: %s", disguiseID, it.name));
    }
}

void ItemConfigDialog::OnClearDisguise(wxCommandEvent& event) {
    disguiseID = 0;
    lblDisguise->SetLabel("None");
}
