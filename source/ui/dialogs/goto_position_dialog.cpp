//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "ui/dialogs/goto_position_dialog.h"
#include "editor/editor.h"
#include "util/common.h"
#include "map/map.h"
#include "ui/positionctrl.h"
#include "ui/gui.h"
#include "util/image_manager.h"

// ============================================================================
// Go To Position Dialog
// Jump to a position on the map by entering XYZ coordinates

GotoPositionDialog::GotoPositionDialog(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Go To Position", wxDefaultPosition, wxDefaultSize),
	editor(editor) {
	Map& map = editor.map;

	// create topsizer
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	posctrl = newd PositionCtrl(this, "Destination", map.getWidth() / 2, map.getHeight() / 2, GROUND_LAYER, map.getWidth(), map.getHeight());
	sizer->Add(posctrl, 0, wxTOP | wxLEFT | wxRIGHT, 20);

	// Paste Button
	wxButton* pasteBtn = newd wxButton(this, wxID_ANY, "Paste from Clipboard");
	pasteBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_PASTE, wxSize(16, 16)));
	pasteBtn->SetToolTip("Paste coordinates from clipboard");
	pasteBtn->Bind(wxEVT_BUTTON, [this, &map](wxCommandEvent&) {
		Position pos;
		if (posFromClipboard(pos, map.getWidth(), map.getHeight())) {
			posctrl->SetPosition(pos);
		}
	});
	sizer->Add(pasteBtn, wxSizerFlags(0).Center().Border(wxTOP, 10));

	// OK/Cancel buttons
	wxSizer* tmpsizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* okBtn = newd wxButton(this, wxID_OK, "OK");
	okBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_CHECK, wxSize(16, 16)));
	okBtn->SetToolTip("Go to position");
	tmpsizer->Add(okBtn, wxSizerFlags(1).Center());
	wxButton* cancelBtn = newd wxButton(this, wxID_CANCEL, "Cancel");
	cancelBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_XMARK, wxSize(16, 16)));
	cancelBtn->SetToolTip("Close this window");
	tmpsizer->Add(cancelBtn, wxSizerFlags(1).Center());
	sizer->Add(tmpsizer, 0, wxALL | wxCENTER, 20); // Border to top too

	SetSizerAndFit(sizer);
	Centre(wxBOTH);

	okBtn->Bind(wxEVT_BUTTON, &GotoPositionDialog::OnClickOK, this);
	cancelBtn->Bind(wxEVT_BUTTON, &GotoPositionDialog::OnClickCancel, this);

	wxIcon icon;
	icon.CopyFromBitmap(IMAGE_MANAGER.GetBitmap(ICON_LOCATION, wxSize(32, 32)));
	SetIcon(icon);
}

void GotoPositionDialog::OnClickCancel(wxCommandEvent&) {
	EndModal(0);
}

void GotoPositionDialog::OnClickOK(wxCommandEvent&) {
	g_gui.SetScreenCenterPosition(posctrl->GetPosition());
	EndModal(1);
}
