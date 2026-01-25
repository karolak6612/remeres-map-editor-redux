#ifndef RME_UI_ITEM_CONFIG_WINDOW_H
#define RME_UI_ITEM_CONFIG_WINDOW_H

#include <wx/dialog.h>
#include <wx/listctrl.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include "game/item_metadata.h"

class ItemConfigWindow : public wxDialog {
public:
    ItemConfigWindow(wxWindow* parent);
    ~ItemConfigWindow();

private:
    void RefreshList();
    void OnAdd(wxCommandEvent& event);
    void OnEdit(wxCommandEvent& event);
    void OnRemove(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnClose(wxCommandEvent& event);

    wxListCtrl* itemList;
    wxButton* btnAdd;
    wxButton* btnEdit;
    wxButton* btnRemove;
    wxButton* btnSave;
    wxButton* btnClose;

    DECLARE_EVENT_TABLE()
};

class ItemConfigDialog : public wxDialog {
public:
    ItemConfigDialog(wxWindow* parent, uint16_t itemId = 0, const ItemMetadata& meta = ItemMetadata());
    ~ItemConfigDialog();

    uint16_t GetItemID() const;
    ItemMetadata GetMetadata() const;

private:
    void OnSelectDisguise(wxCommandEvent& event);
    void OnSelectItem(wxCommandEvent& event);
    void OnClearDisguise(wxCommandEvent& event);

    uint16_t selectedItemID;
    uint16_t disguiseID;

    wxStaticText* lblItem;
    wxButton* btnSelectItem;
    wxChoice* choiceTechType;
    wxStaticText* lblDisguise;
    wxButton* btnSelectDisguise;
    wxButton* btnClearDisguise;

    DECLARE_EVENT_TABLE()
};

#endif // RME_UI_ITEM_CONFIG_WINDOW_H
