#include "ui/welcome/startup_card.h"

#include "ui/theme.h"

StartupCardPanel::StartupCardPanel(wxWindow* parent, const wxString& title) :
	wxPanel(parent, wxID_ANY),
	m_title(title) {
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetBackgroundColour(Theme::Get(Theme::Role::RaisedSurface));
	SetForegroundColour(Theme::Get(Theme::Role::Text));

	auto* root_sizer = new wxBoxSizer(wxVERTICAL);
	root_sizer->AddSpacer(FromDIP(12));

	m_title_label = new wxStaticText(this, wxID_ANY, m_title);
	m_title_label->SetFont(Theme::GetFont(9, true));
	m_title_label->SetForegroundColour(Theme::Get(Theme::Role::Text));
	m_title_label->SetBackgroundColour(GetBackgroundColour());
	root_sizer->Add(m_title_label, 0, wxLEFT | wxRIGHT, FromDIP(14));

	auto* separator = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, FromDIP(1)));
	separator->SetBackgroundColour(Theme::Get(Theme::Role::Border));
	root_sizer->Add(separator, 0, wxEXPAND | wxALL, FromDIP(14));

	m_body_sizer = new wxBoxSizer(wxVERTICAL);
	root_sizer->Add(m_body_sizer, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(14));

	SetSizer(root_sizer);
	Bind(wxEVT_PAINT, &StartupCardPanel::OnPaint, this);
}

void StartupCardPanel::OnPaint(wxPaintEvent& WXUNUSED(event)) {
	wxAutoBufferedPaintDC dc(this);
	dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
	dc.Clear();

	const wxRect rect = GetClientRect().Deflate(1);
	dc.SetPen(wxPen(Theme::Get(Theme::Role::Border)));
	dc.SetBrush(wxBrush(Theme::Get(Theme::Role::RaisedSurface)));
	dc.DrawRoundedRectangle(rect, FromDIP(10));
}
