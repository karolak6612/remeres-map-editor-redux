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

#include <wx/collpane.h>

#include "app/settings.h"

#include "app/client_version.h"
#include "editor/editor.h"
#include "rendering/postprocess/post_process_manager.h"

#include "ui/gui.h"

#include "ui/dialog_util.h"
#include "app/managers/version_manager.h"
#include "app/preferences.h"
#include "util/image_manager.h"

PreferencesWindow::PreferencesWindow(wxWindow* parent, bool clientVersionSelected) :
	wxDialog(parent, wxID_ANY, "Preferences", wxDefaultPosition, wxSize(600, 500), wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER) {
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	book = newd wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBK_TOP);
	// book->SetPadding(4);

	wxImageList* imageList = new wxImageList(16, 16);
	imageList->Add(IMAGE_MANAGER.GetBitmap(ICON_GEAR, wxSize(16, 16)));
	imageList->Add(IMAGE_MANAGER.GetBitmap(ICON_PEN_TO_SQUARE, wxSize(16, 16)));
	imageList->Add(IMAGE_MANAGER.GetBitmap(ICON_IMAGE, wxSize(16, 16)));
	imageList->Add(IMAGE_MANAGER.GetBitmap(ICON_WINDOW_MAXIMIZE, wxSize(16, 16)));
	imageList->Add(IMAGE_MANAGER.GetBitmap(ICON_GAMEPAD, wxSize(16, 16)));
	book->AssignImageList(imageList);

	general_page = new GeneralPage(book);
	editor_page = new EditorPage(book);
	graphics_page = new GraphicsPage(book);
	interface_page = new InterfacePage(book);
	client_version_page = new ClientVersionPage(book);

	book->AddPage(general_page, "General", true, 0);
	book->AddPage(editor_page, "Editor", false, 1);
	book->AddPage(graphics_page, "Graphics", false, 2);
	book->AddPage(interface_page, "Interface", false, 3);
	book->AddPage(client_version_page, "Client Version", clientVersionSelected, 4);

	sizer->Add(book, 1, wxEXPAND | wxALL, 10);

	wxSizer* subsizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* okBtn = newd wxButton(this, wxID_OK, "OK");
	okBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_CHECK, wxSize(16, 16)));
	subsizer->Add(okBtn, wxSizerFlags(1).Center());

	wxButton* cancelBtn = newd wxButton(this, wxID_CANCEL, "Cancel");
	cancelBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_XMARK, wxSize(16, 16)));
	subsizer->Add(cancelBtn, wxSizerFlags(1).Border(wxALL, 5).Left().Center());

	wxButton* applyBtn = newd wxButton(this, wxID_APPLY, "Apply");
	applyBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_SYNC, wxSize(16, 16)));
	subsizer->Add(applyBtn, wxSizerFlags(1).Center());

	sizer->Add(subsizer, 0, wxCENTER | wxLEFT | wxBOTTOM | wxRIGHT, 10);

	SetSizerAndFit(sizer);

	int w = g_settings.getInteger(Config::PREFERENCES_WINDOW_WIDTH);
	int h = g_settings.getInteger(Config::PREFERENCES_WINDOW_HEIGHT);
	if (w > 0 && h > 0) {
		SetSize(w, h);
	} else {
		SetSize(600, 500);
	}

	Centre(wxBOTH);

	Bind(wxEVT_BUTTON, &PreferencesWindow::OnClickOK, this, wxID_OK);
	Bind(wxEVT_BUTTON, &PreferencesWindow::OnClickCancel, this, wxID_CANCEL);
	Bind(wxEVT_BUTTON, &PreferencesWindow::OnClickApply, this, wxID_APPLY);
	Bind(wxEVT_COLLAPSIBLEPANE_CHANGED, &PreferencesWindow::OnCollapsiblePane, this);

	wxIcon icon;
	icon.CopyFromBitmap(IMAGE_MANAGER.GetBitmap(ICON_GEAR, wxSize(32, 32)));
	SetIcon(icon);
}

PreferencesWindow::~PreferencesWindow() {
	int w, h;
	GetSize(&w, &h);
	g_settings.setInteger(Config::PREFERENCES_WINDOW_WIDTH, w);
	g_settings.setInteger(Config::PREFERENCES_WINDOW_HEIGHT, h);
}

// Event handlers!

void PreferencesWindow::OnClickOK(wxCommandEvent& event) {
	if (!client_version_page->ValidateData()) {
		return;
	}
	Apply();
	EndModal(wxID_OK);
}

void PreferencesWindow::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	EndModal(wxID_CANCEL);
}

void PreferencesWindow::OnClickApply(wxCommandEvent& WXUNUSED(event)) {
	if (!client_version_page->ValidateData()) {
		return;
	}
	Apply();
}

void PreferencesWindow::OnCollapsiblePane(wxCollapsiblePaneEvent& event) {
	auto* win = (wxWindow*)event.GetEventObject();
	win->GetParent()->Fit();
}

void PreferencesWindow::Apply() {
	general_page->Apply();
	editor_page->Apply();
	graphics_page->Apply();
	interface_page->Apply();
	client_version_page->Apply();

	g_settings.save();
}
