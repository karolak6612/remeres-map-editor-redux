#include <wx/listctrl.h>
#include <wx/imaglist.h>
#include <wx/statline.h>
#include <wx/filename.h>
#include <wx/dcbuffer.h>

#include "app/main.h"
#include "app/definitions.h"
#include "ui/welcome_dialog.h"
#include "app/settings.h"
#include "app/preferences.h"
#include "util/image_manager.h"
#include "ui/theme.h"

wxDEFINE_EVENT(WELCOME_DIALOG_ACTION, wxCommandEvent);

// --- Convex Button Helper Class ---
class ConvexButton : public wxControl {
public:
	ConvexButton(wxWindow* parent, wxWindowID id, const wxString& label, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0) :
		wxControl(parent, id, pos, size, style | wxBORDER_NONE) {
		SetLabel(label);
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		Bind(wxEVT_PAINT, &ConvexButton::OnPaint, this);
		Bind(wxEVT_ENTER_WINDOW, &ConvexButton::OnMouse, this);
		Bind(wxEVT_LEAVE_WINDOW, &ConvexButton::OnMouse, this);
		Bind(wxEVT_LEFT_DOWN, &ConvexButton::OnMouse, this);
		Bind(wxEVT_LEFT_UP, &ConvexButton::OnMouse, this);
		Bind(wxEVT_MOUSE_CAPTURE_LOST, &ConvexButton::OnMouseCaptureLost, this);
	}

	void SetBitmap(const wxBitmap& bitmap) {
		m_icon = bitmap;
		InvalidateBestSize();
		Refresh();
	}

	wxSize DoGetBestClientSize() const override {
		wxClientDC dc(const_cast<ConvexButton*>(this));
		dc.SetFont(GetFont());
		wxSize text = dc.GetTextExtent(GetLabel());
		int width = text.x + FromDIP(30);
		if (m_icon.IsOk()) {
			width += m_icon.GetWidth() + FromDIP(8);
		}
		int height = std::max(text.y, m_icon.IsOk() ? m_icon.GetHeight() : 0) + FromDIP(16);
		return wxSize(std::max(width, FromDIP(100)), std::max(height, FromDIP(30)));
	}

private:
	wxBitmap m_icon;
	bool m_hover = false;
	bool m_pressed = false;

	void OnPaint(wxPaintEvent& evt) {
		wxAutoBufferedPaintDC dc(this);
		wxSize sz = GetClientSize();
		wxColour bg = Theme::Get(Theme::Role::Surface);

		// Determine gradient colors for convex look
		wxColour top = bg.ChangeLightness(120);
		wxColour bottom = bg.ChangeLightness(90);
		if (m_pressed) {
			std::swap(top, bottom); // Invert gradient for pressed state
		} else if (m_hover) {
			top = top.ChangeLightness(110);
			bottom = bottom.ChangeLightness(110);
		}

		// Draw Gradient Background
		dc.GradientFillLinear(wxRect(sz), top, bottom, wxSOUTH);

		// Draw Border
		dc.SetPen(wxPen(Theme::Get(Theme::Role::Border)));
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.DrawRectangle(0, 0, sz.x, sz.y);

		// Draw Inner Highlight/Shadow for 3D effect
		if (!m_pressed) {
			dc.SetPen(wxPen(wxColour(255, 255, 255))); // Highlight top/left
			dc.DrawLine(1, 1, sz.x - 1, 1);
			dc.DrawLine(1, 1, 1, sz.y - 1);

			dc.SetPen(wxPen(wxColour(0, 0, 0))); // Shadow bottom/right
			dc.DrawLine(1, sz.y - 2, sz.x - 1, sz.y - 2);
			dc.DrawLine(sz.x - 2, 1, sz.x - 2, sz.y - 2);
		}

		// Draw Icon and Text
		dc.SetFont(GetFont());
		dc.SetTextForeground(Theme::Get(Theme::Role::Text));
		wxSize textSize = dc.GetTextExtent(GetLabel());

		int x = (sz.x - textSize.x) / 2;
		if (m_icon.IsOk()) {
			x -= (m_icon.GetWidth() + FromDIP(8)) / 2;
		}

		int y = (sz.y - textSize.y) / 2;
		if (m_pressed) {
			x += 1;
			y += 1;
		} // Shift content when pressed

		if (m_icon.IsOk()) {
			int iy = (sz.y - m_icon.GetHeight()) / 2;
			if (m_pressed) {
				iy += 1;
			}
			dc.DrawBitmap(m_icon, x, iy, true);
			x += m_icon.GetWidth() + FromDIP(8);
		}

		dc.DrawText(GetLabel(), x, y);
	}

	void OnMouse(wxMouseEvent& evt) {
		if (evt.Entering()) {
			m_hover = true;
		} else if (evt.Leaving()) {
			m_hover = false;
			if (!HasCapture()) {
				m_pressed = false;
			}
		} else if (evt.LeftDown()) {
			m_pressed = true;
			CaptureMouse();
		} else if (evt.LeftUp()) {
			if (HasCapture()) {
				ReleaseMouse();
			}
			if (m_pressed && GetClientRect().Contains(evt.GetPosition())) {
				wxCommandEvent event(wxEVT_BUTTON, GetId());
				event.SetEventObject(this);
				ProcessWindowEvent(event);
			}
			m_pressed = false;
		}
		Refresh();
	}

	void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt) {
		if (HasCapture()) {
			ReleaseMouse();
		}
		m_pressed = false;
		Refresh();
	}
};

// --- Dark Card Panel Helper Class ---
class DarkCardPanel : public wxPanel {
public:
	DarkCardPanel(wxWindow* parent, const wxString& title) : wxPanel(parent, wxID_ANY) {
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		Bind(wxEVT_PAINT, &DarkCardPanel::OnPaint, this);

		m_title = title;
		m_mainSizer = new wxBoxSizer(wxVERTICAL);
		m_mainSizer->AddSpacer(FromDIP(30)); // Header space

		SetSizer(m_mainSizer);
	}

	wxBoxSizer* GetSizer() {
		return m_mainSizer;
	}

	// Override to ensure refresh on color change
	bool SetBackgroundColour(const wxColour& colour) override {
		bool ret = wxPanel::SetBackgroundColour(colour);
		Refresh();
		return ret;
	}

private:
	wxString m_title;
	wxBoxSizer* m_mainSizer;

	void OnPaint(wxPaintEvent& evt) {
		wxAutoBufferedPaintDC dc(this);
		wxSize sz = GetClientSize();
		wxColour bg = Theme::Get(Theme::Role::Surface);
		wxColour border = Theme::Get(Theme::Role::Border);
		wxColour headerBg = Theme::Get(Theme::Role::Header);
		wxColour textCol = Theme::Get(Theme::Role::Text);

		// Fill Background
		dc.SetBrush(wxBrush(bg));
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.DrawRectangle(sz);

		// Draw Border
		dc.SetPen(wxPen(border));
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.DrawRectangle(0, 0, sz.x, sz.y);

		// Draw Header
		int headerH = FromDIP(28);
		dc.SetBrush(wxBrush(headerBg));
		dc.DrawRectangle(1, 1, sz.x - 2, headerH);
		dc.DrawLine(0, headerH + 1, sz.x, headerH + 1);

		// Draw Title
		dc.SetFont(Theme::GetFont(9, true));
		dc.SetTextForeground(textCol);
		dc.DrawText(m_title, FromDIP(8), FromDIP(6));
	}
};

// --- Bordered Dark Panel (for Header/Footer) ---
class BorderedDarkPanel : public wxPanel {
public:
	BorderedDarkPanel(wxWindow* parent) : wxPanel(parent, wxID_ANY) {
		// Set actual background colour so children inherit it
		wxPanel::SetBackgroundColour(Theme::Get(Theme::Role::Background));
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		Bind(wxEVT_PAINT, &BorderedDarkPanel::OnPaint, this);
	}

private:
	void OnPaint(wxPaintEvent& evt) {
		wxAutoBufferedPaintDC dc(this);
		wxSize sz = GetClientSize();
		wxColour bg = Theme::Get(Theme::Role::Background); // Default header/footer bg
		wxColour border = Theme::Get(Theme::Role::Border);

		dc.SetBrush(wxBrush(bg));
		dc.SetPen(wxPen(border));
		dc.DrawRectangle(0, 0, sz.x, sz.y);
	}
};

WelcomeDialog::WelcomeDialog(const wxString& titleText, const wxString& versionText, const wxSize& size, const wxBitmap& rmeLogo, const std::vector<wxString>& recentFiles) :
	wxDialog(nullptr, wxID_ANY, titleText, wxDefaultPosition, size, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {

	SetBackgroundColour(Theme::Get(Theme::Role::Surface));
	SetForegroundColour(Theme::Get(Theme::Role::Text));

	// Image List for Icons
	m_imageList = std::make_unique<wxImageList>(FromDIP(16), FromDIP(16));
	m_imageList->Add(IMAGE_MANAGER.GetBitmap(ICON_OPEN, FromDIP(wxSize(16, 16))));
	m_imageList->Add(IMAGE_MANAGER.GetBitmap(ICON_NEW, FromDIP(wxSize(16, 16))));
	m_imageList->Add(IMAGE_MANAGER.GetBitmap(ICON_SEARCH, FromDIP(wxSize(16, 16))));
	m_imageList->Add(IMAGE_MANAGER.GetBitmap(ICON_HARD_DRIVE, FromDIP(wxSize(16, 16))));
	m_imageList->Add(IMAGE_MANAGER.GetBitmap(ICON_FILE, FromDIP(wxSize(16, 16))));
	m_imageList->Add(IMAGE_MANAGER.GetBitmap(ICON_CHECK, FromDIP(wxSize(16, 16))));
	m_imageList->Add(IMAGE_MANAGER.GetBitmap(ICON_FOLDER, FromDIP(wxSize(16, 16))));

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	mainSizer->Add(CreateHeaderPanel(this, titleText, rmeLogo), 0, wxEXPAND | wxALL, 2);
	mainSizer->Add(CreateContentPanel(this, recentFiles), 1, wxEXPAND | wxALL, 2);
	mainSizer->Add(CreateFooterPanel(this, versionText), 0, wxEXPAND | wxALL, 2);

	SetSizer(mainSizer);
	Layout();
	Centre();
}

WelcomeDialog::~WelcomeDialog() {
	if (m_clientList) {
		m_clientList->SetImageList(nullptr, wxIMAGE_LIST_SMALL);
	}
}

void WelcomeDialog::AddInfoField(wxSizer* sizer, wxWindow* parent, const wxString& label, const wxString& value, std::string_view artId, const wxColour& valCol) {
	wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);

	wxStaticBitmap* icon = new wxStaticBitmap(parent, wxID_ANY, IMAGE_MANAGER.GetBitmap(artId, wxSize(14, 14)));
	row->Add(icon, 0, wxCENTER | wxRIGHT, 4);

	wxStaticText* lbl = new wxStaticText(parent, wxID_ANY, label);
	wxFont f = lbl->GetFont();
	f.SetPointSize(8);
	lbl->SetFont(f);
	lbl->SetForegroundColour(Theme::Get(Theme::Role::TextSubtle));
	row->Add(lbl, 0, wxCENTER, 0);

	sizer->Add(row, 0, wxTOP | wxLEFT | wxRIGHT, 2);

	wxStaticText* val = new wxStaticText(parent, wxID_ANY, value);
	wxFont vf = val->GetFont();
	vf.SetWeight(wxFONTWEIGHT_BOLD);
	val->SetFont(vf);

	if (!valCol.IsOk()) {
		val->SetForegroundColour(Theme::Get(Theme::Role::Text));
	} else {
		val->SetForegroundColour(valCol);
	}

	sizer->Add(val, 0, wxBOTTOM | wxLEFT | wxRIGHT, 4);
}

wxPanel* WelcomeDialog::CreateHeaderPanel(wxWindow* parent, const wxString& titleText, const wxBitmap& rmeLogo) {
	wxPanel* headerPanel = new BorderedDarkPanel(parent);
	wxBoxSizer* headerSizer = new wxBoxSizer(wxHORIZONTAL);

	// Icon Button - Align Left: 10px total
	ConvexButton* iconBtn = new ConvexButton(headerPanel, wxID_ANY, "", wxDefaultPosition, wxSize(48, 48));
	iconBtn->SetBitmap(rmeLogo.IsOk() ? rmeLogo : IMAGE_MANAGER.GetBitmap(ICON_QUESTION_CIRCLE, wxSize(32, 32))); // Fallback if logo invalid
	headerSizer->Add(iconBtn, 0, wxALL | wxCENTER, 8);

	wxBoxSizer* titleSizer = new wxBoxSizer(wxVERTICAL);
	wxStaticText* title = new wxStaticText(headerPanel, wxID_ANY, titleText);
	wxFont titleFont = title->GetFont();
	titleFont.SetPointSize(14);
	titleFont.SetWeight(wxFONTWEIGHT_BOLD);
	title->SetFont(titleFont);
	title->SetForegroundColour(Theme::Get(Theme::Role::Text));
	title->SetBackgroundColour(Theme::Get(Theme::Role::Background));
	titleSizer->Add(title, 0, wxALL, 2);

	wxStaticText* subtitle = new wxStaticText(headerPanel, wxID_ANY, "Welcome! Start a new project or continue where you left off.");
	subtitle->SetForegroundColour(Theme::Get(Theme::Role::TextSubtle));
	subtitle->SetBackgroundColour(Theme::Get(Theme::Role::Background));
	titleSizer->Add(subtitle, 0, wxALL, 2);

	headerSizer->Add(titleSizer, 1, wxALL | wxEXPAND, 10);

	ConvexButton* prefBtn = new ConvexButton(headerPanel, wxID_PREFERENCES, "Preferences");
	prefBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_GEAR, FromDIP(wxSize(24, 24))));
	prefBtn->Bind(wxEVT_BUTTON, &WelcomeDialog::OnButtonClicked, this);

	// Align Right: 10px total
	headerSizer->Add(prefBtn, 0, wxCENTER | wxALL, 8);

	headerPanel->SetSizer(headerSizer);
	return headerPanel;
}

wxPanel* WelcomeDialog::CreateFooterPanel(wxWindow* parent, const wxString& versionText) {
	wxPanel* footerPanel = new BorderedDarkPanel(parent);
	wxBoxSizer* footerSizer = new wxBoxSizer(wxHORIZONTAL);

	ConvexButton* exitBtn = new ConvexButton(footerPanel, wxID_EXIT, "Exit");
	exitBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_POWER_OFF, FromDIP(wxSize(24, 24))));
	exitBtn->Bind(wxEVT_BUTTON, &WelcomeDialog::OnButtonClicked, this);

	// Align Left: 10px total
	footerSizer->Add(exitBtn, 0, wxALL, 8);

	ConvexButton* newBtn = new ConvexButton(footerPanel, wxID_NEW, "New Map");
	newBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_NEW, FromDIP(wxSize(24, 24))));
	newBtn->Bind(wxEVT_BUTTON, &WelcomeDialog::OnButtonClicked, this);
	footerSizer->Add(newBtn, 0, wxALL, 8);

	footerSizer->AddStretchSpacer();
	wxStaticText* version = new wxStaticText(footerPanel, wxID_ANY, versionText);
	version->SetForegroundColour(Theme::Get(Theme::Role::TextSubtle));
	version->SetBackgroundColour(Theme::Get(Theme::Role::Background));
	footerSizer->Add(version, 0, wxCENTER | wxALL, 5);
	footerSizer->AddStretchSpacer();

	ConvexButton* loadBtn = new ConvexButton(footerPanel, wxID_ANY, "Load Map");
	loadBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_OPEN, FromDIP(wxSize(24, 24))));
	loadBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		wxString path = m_recentList->GetSelectedFile();
		if (!path.IsEmpty()) {
			wxCommandEvent* newEvent = new wxCommandEvent(WELCOME_DIALOG_ACTION);
			newEvent->SetId(wxID_OPEN);
			newEvent->SetString(path);
			QueueEvent(newEvent);
		}
	});

	// Align Right: 10px total
	footerSizer->Add(loadBtn, 0, wxALL, 8);

	footerPanel->SetSizer(footerSizer);
	return footerPanel;
}

wxPanel* WelcomeDialog::CreateContentPanel(wxWindow* parent, const std::vector<wxString>& recentFiles) {
	wxPanel* contentPanel = new wxPanel(parent, wxID_ANY);
	contentPanel->SetBackgroundColour(Theme::Get(Theme::Role::Surface));
	wxBoxSizer* contentSizer = new wxBoxSizer(wxHORIZONTAL);

	// Column 1: Actions (Buttons directly on panel)
	wxBoxSizer* col1 = new wxBoxSizer(wxVERTICAL);

	ConvexButton* newMapBtn = new ConvexButton(contentPanel, wxID_NEW, "New map", wxDefaultPosition, wxSize(130, 50));
	newMapBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_NEW, wxSize(24, 24)));
	newMapBtn->Bind(wxEVT_BUTTON, &WelcomeDialog::OnButtonClicked, this);
	// Align "New Map" with Icon (8px from left edge of content panel)
	col1->Add(newMapBtn, 0, wxEXPAND | wxTOP | wxBOTTOM | wxRIGHT, 4);

	ConvexButton* browseBtn = new ConvexButton(contentPanel, wxID_OPEN, "Browse Map", wxDefaultPosition, wxSize(130, 50));
	browseBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_OPEN, wxSize(24, 24)));
	browseBtn->Bind(wxEVT_BUTTON, &WelcomeDialog::OnButtonClicked, this);
	col1->Add(browseBtn, 0, wxEXPAND | wxTOP | wxBOTTOM | wxRIGHT, 4);

	// Add Column 1 to Sizer FIRST (Explicit LEFTmost placement)
	contentSizer->Add(col1, 0, wxTOP | wxBOTTOM | wxRIGHT | wxLEFT, 8); // Add 8px left margin to whole column

	// Column 2: Recent Maps
	DarkCardPanel* col2 = new DarkCardPanel(contentPanel, "Recent Maps");
	m_recentList = newd RecentFileListBox(col2, wxID_ANY);
	m_recentList->SetRecentFiles(recentFiles);

	// Bind to listbox double click event (requires RecentFileListBox to emit it, which NanoVGListBox default doesn't,
	// but I implemented double click logic in WaypointListBox.
	// NanoVGListBox does NOT implement it by default.
	// I should probably add double click logic to RecentFileListBox constructor as well!)
	// Or better: update NanoVGListBox to support it? No, I can't touch it easily.
	// I will add Bind logic in RecentFileListBox constructor or just use bind here if I expose it.
	// I forgot to add Bind logic in RecentFileListBox constructor.
	// I can add it here via binding to LEFT_DCLICK on the control itself.

	m_recentList->Bind(wxEVT_LEFT_DCLICK, [this](wxMouseEvent& event) {
		// Forward as if item activated
		wxCommandEvent evt(wxEVT_LISTBOX_DCLICK, m_recentList->GetId());
		OnRecentFileActivated(evt);
	});

	m_recentList->Bind(wxEVT_KEY_DOWN, [this](wxKeyEvent& event) {
		if (event.GetKeyCode() == WXK_RETURN || event.GetKeyCode() == WXK_NUMPAD_ENTER) {
			wxCommandEvent evt(wxEVT_LISTBOX_DCLICK, m_recentList->GetId());
			OnRecentFileActivated(evt);
		} else {
			event.Skip();
		}
	});

	m_recentList->Bind(wxEVT_LISTBOX, &WelcomeDialog::OnRecentFileSelected, this);

	col2->GetSizer()->Add(m_recentList, 1, wxEXPAND | wxALL, 1);
	contentSizer->Add(col2, 1, wxEXPAND | wxTOP | wxBOTTOM | wxRIGHT, 5); // Add Column 2

	// Column 3: Selected Map Info
	DarkCardPanel* col3 = new DarkCardPanel(contentPanel, "Selected Map Info");
	AddInfoField(col3->GetSizer(), col3, "Map Name", "Placeholder", ICON_FILE);
	AddInfoField(col3->GetSizer(), col3, "Client Version", "Placeholder", ICON_CHECK);
	AddInfoField(col3->GetSizer(), col3, "Dimensions", "Placeholder", ICON_LIST);
	col3->GetSizer()->Add(new wxStaticLine(col3), 0, wxEXPAND | wxALL, 4);
	AddInfoField(col3->GetSizer(), col3, "House File", "Placeholder", ICON_FILE);
	AddInfoField(col3->GetSizer(), col3, "Spawn File", "Placeholder", ICON_FILE);
	AddInfoField(col3->GetSizer(), col3, "Description", "Placeholder", ICON_FILE_LINES);
	contentSizer->Add(col3, 1, wxEXPAND | wxTOP | wxBOTTOM | wxRIGHT, 5); // Add Column 3 (Center)

	// Column 4: Client Info
	DarkCardPanel* col4 = new DarkCardPanel(contentPanel, "Client Information");
	AddInfoField(col4->GetSizer(), col4, "Client Name", "Placeholder", ICON_HARD_DRIVE);
	AddInfoField(col4->GetSizer(), col4, "Client Version", "Placeholder", ICON_CHECK);
	AddInfoField(col4->GetSizer(), col4, "Data Directory", "Placeholder", ICON_FOLDER);
	col4->GetSizer()->Add(new wxStaticLine(col4), 0, wxEXPAND | wxALL, 4);
	AddInfoField(col4->GetSizer(), col4, "Status", "Placeholder", ICON_CHECK, wxColour(0, 200, 0));
	contentSizer->Add(col4, 1, wxEXPAND | wxTOP | wxBOTTOM | wxRIGHT, 5); // Add Column 4

	// Column 5: Available Clients
	DarkCardPanel* col5 = new DarkCardPanel(contentPanel, "Available Clients");
	m_clientList = new wxListCtrl(col5, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_NO_HEADER | wxBORDER_NONE);
	// SetImageList — ownership retained by m_imageList (do not replace with AssignImageList)
	m_clientList->SetImageList(m_imageList.get(), wxIMAGE_LIST_SMALL);
	m_clientList->InsertColumn(0, "Icon", wxLIST_FORMAT_LEFT, FromDIP(24));
	m_clientList->InsertColumn(1, "Name", wxLIST_FORMAT_LEFT, FromDIP(150));

	m_clientList->SetBackgroundColour(Theme::Get(Theme::Role::Background));
	m_clientList->SetTextColour(Theme::Get(Theme::Role::Text));

	long ph1 = m_clientList->InsertItem(0, "", 3);
	m_clientList->SetItem(ph1, 1, "Placeholder Client 1");
	long ph2 = m_clientList->InsertItem(1, "", 3);
	m_clientList->SetItem(ph2, 1, "Placeholder Client 2");

	col5->GetSizer()->Add(m_clientList, 1, wxEXPAND | wxALL, 1);
	contentSizer->Add(col5, 1, wxEXPAND | wxTOP | wxBOTTOM | wxRIGHT, 5); // Add Column 5 (Right)

	contentPanel->SetSizer(contentSizer);
	return contentPanel;
}

void WelcomeDialog::OnButtonClicked(wxCommandEvent& event) {
	int id = event.GetId();
	if (id == wxID_PREFERENCES) {
		PreferencesWindow preferences_window(this, true);
		preferences_window.ShowModal();
	} else if (id == wxID_NEW) {
		wxCommandEvent* newEvent = new wxCommandEvent(WELCOME_DIALOG_ACTION);
		newEvent->SetId(wxID_NEW);
		QueueEvent(newEvent);
	} else if (id == wxID_OPEN) {
		wxString wildcard = MAP_LOAD_FILE_WILDCARD;
		wxString filePath;
		{
			wxFileDialog file_dialog(this, "Open map file", "", "", wildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
			if (file_dialog.ShowModal() == wxID_OK) {
				filePath = file_dialog.GetPath();
			}
		}

		if (!filePath.IsEmpty()) {
			wxCommandEvent* newEvent = new wxCommandEvent(WELCOME_DIALOG_ACTION);
			newEvent->SetId(wxID_OPEN);
			newEvent->SetString(filePath);
			QueueEvent(newEvent);
		}
	} else if (id == wxID_EXIT) {
		Close(true); // Close the dialog
	}
}

void WelcomeDialog::OnRecentFileActivated(wxCommandEvent& event) {
	wxString realPath = m_recentList->GetSelectedFile();
	if (!realPath.IsEmpty()) {
		wxCommandEvent* newEvent = new wxCommandEvent(WELCOME_DIALOG_ACTION);
		newEvent->SetId(wxID_OPEN);
		newEvent->SetString(realPath);
		QueueEvent(newEvent);
	}
}

void WelcomeDialog::OnRecentFileSelected(wxCommandEvent& event) {
	// Placeholder for displaying details
}
