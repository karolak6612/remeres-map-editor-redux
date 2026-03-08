#include "app/preferences/preferences_layout.h"

#include <algorithm>

#include <wx/dcbuffer.h>

namespace {
wxStaticText* CreateTitleLabel(wxWindow* parent, const wxString& text) {
	auto* label = new wxStaticText(parent, wxID_ANY, text);
	label->SetFont(Theme::GetFont(9, true));
	label->SetForegroundColour(Theme::Get(Theme::Role::Text));
	label->SetBackgroundColour(parent->GetBackgroundColour());
	return label;
}

wxStaticText* CreateDescriptionLabel(wxWindow* parent, const wxString& text) {
	auto* label = new wxStaticText(parent, wxID_ANY, text);
	label->SetFont(Theme::GetFont(8, false));
	label->SetForegroundColour(Theme::Get(Theme::Role::TextSubtle));
	label->SetBackgroundColour(parent->GetBackgroundColour());
	if (auto* section = dynamic_cast<PreferencesSectionPanel*>(parent)) {
		section->RegisterWrappedLabel(label);
	}
	return label;
}
}

PreferencesSectionPanel::PreferencesSectionPanel(wxWindow* parent, const wxString& title, const wxString& description) :
	wxPanel(parent, wxID_ANY) {
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetBackgroundColour(Theme::Get(Theme::Role::RaisedSurface));
	SetForegroundColour(Theme::Get(Theme::Role::Text));

	auto* root_sizer = new wxBoxSizer(wxVERTICAL);
	const int padding = wxWindow::FromDIP(14, this);

	root_sizer->AddSpacer(wxWindow::FromDIP(12, this));

	auto* title_label = new wxStaticText(this, wxID_ANY, title);
	title_label->SetFont(Theme::GetFont(10, true));
	title_label->SetForegroundColour(Theme::Get(Theme::Role::Text));
	title_label->SetBackgroundColour(GetBackgroundColour());
	root_sizer->Add(title_label, 0, wxLEFT | wxRIGHT, padding);

	if (!description.empty()) {
		root_sizer->AddSpacer(wxWindow::FromDIP(4, this));
		m_description_label = CreateDescriptionLabel(this, description);
		root_sizer->Add(m_description_label, 0, wxLEFT | wxRIGHT, padding);
	}

	auto* separator = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, wxWindow::FromDIP(1, this)));
	separator->SetBackgroundColour(Theme::Get(Theme::Role::Border));
	root_sizer->Add(separator, 0, wxEXPAND | wxALL, padding);

	m_body_sizer = new wxBoxSizer(wxVERTICAL);
	root_sizer->Add(m_body_sizer, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, padding);

	SetSizer(root_sizer);
	Bind(wxEVT_PAINT, &PreferencesSectionPanel::OnPaint, this);
	Bind(wxEVT_SIZE, &PreferencesSectionPanel::OnSize, this);
}

void PreferencesSectionPanel::RegisterWrappedLabel(wxStaticText* label) {
	if (!label) {
		return;
	}

	m_wrapped_labels.push_back(label);
	UpdateWrapping();
}

void PreferencesSectionPanel::OnPaint(wxPaintEvent& WXUNUSED(event)) {
	wxAutoBufferedPaintDC dc(this);
	dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
	dc.Clear();

	const wxRect rect = GetClientRect().Deflate(1);
	dc.SetPen(wxPen(Theme::Get(Theme::Role::Border)));
	dc.SetBrush(wxBrush(Theme::Get(Theme::Role::RaisedSurface)));
	dc.DrawRoundedRectangle(rect, wxWindow::FromDIP(10, this));
}

void PreferencesSectionPanel::OnSize(wxSizeEvent& event) {
	UpdateWrapping();
	event.Skip();
}

void PreferencesSectionPanel::UpdateWrapping() {
	const int wrap_width = std::max(GetClientSize().GetWidth() - wxWindow::FromDIP(120, this), wxWindow::FromDIP(220, this));
	for (auto* label : m_wrapped_labels) {
		if (label) {
			label->Wrap(wrap_width);
		}
	}
	Layout();
}

wxStaticText* PreferencesLayout::CreateBodyText(wxWindow* parent, const wxString& text, bool bold) {
	auto* label = new wxStaticText(parent, wxID_ANY, text);
	label->SetFont(Theme::GetFont(9, bold));
	label->SetForegroundColour(Theme::Get(Theme::Role::Text));
	label->SetBackgroundColour(parent->GetBackgroundColour());
	if (auto* section = dynamic_cast<PreferencesSectionPanel*>(parent)) {
		section->RegisterWrappedLabel(label);
	}
	return label;
}

wxStaticText* PreferencesLayout::AddNotice(PreferencesSectionPanel* section, const wxString& text, Theme::Role role) {
	auto* notice = new wxStaticText(section, wxID_ANY, text);
	notice->SetFont(Theme::GetFont(8, false));
	notice->SetForegroundColour(Theme::Get(role));
	notice->SetBackgroundColour(section->GetBackgroundColour());
	section->RegisterWrappedLabel(notice);
	section->GetBodySizer()->Add(notice, 0, wxEXPAND | wxBOTTOM, wxWindow::FromDIP(10, section));
	return notice;
}

wxCheckBox* PreferencesLayout::AddCheckBoxRow(
	PreferencesSectionPanel* section,
	const wxString& title,
	const wxString& description,
	bool value
) {
	auto* row_sizer = new wxBoxSizer(wxHORIZONTAL);
	auto* text_sizer = new wxBoxSizer(wxVERTICAL);

	text_sizer->Add(CreateTitleLabel(section, title), 0, wxBOTTOM, wxWindow::FromDIP(2, section));
	text_sizer->Add(CreateDescriptionLabel(section, description), 0, wxEXPAND);
	row_sizer->Add(text_sizer, 1, wxEXPAND | wxRIGHT, wxWindow::FromDIP(12, section));

	auto* checkbox = new wxCheckBox(section, wxID_ANY, wxEmptyString);
	checkbox->SetValue(value);
	row_sizer->Add(checkbox, 0, wxALIGN_CENTER_VERTICAL | wxTOP, wxWindow::FromDIP(2, section));

	section->GetBodySizer()->Add(row_sizer, 0, wxEXPAND | wxBOTTOM, wxWindow::FromDIP(12, section));
	return checkbox;
}

void PreferencesLayout::AddControlRow(
	PreferencesSectionPanel* section,
	const wxString& title,
	const wxString& description,
	wxWindow* control,
	bool expand_control
) {
	auto* row_sizer = new wxBoxSizer(wxHORIZONTAL);
	auto* text_sizer = new wxBoxSizer(wxVERTICAL);

	text_sizer->Add(CreateTitleLabel(section, title), 0, wxBOTTOM, wxWindow::FromDIP(2, section));
	text_sizer->Add(CreateDescriptionLabel(section, description), 0, wxEXPAND);
	row_sizer->Add(text_sizer, 1, wxEXPAND | wxRIGHT, wxWindow::FromDIP(12, section));
	row_sizer->Add(control, expand_control ? 1 : 0, (expand_control ? wxEXPAND : wxALIGN_CENTER_VERTICAL) | wxTOP, wxWindow::FromDIP(2, section));

	section->GetBodySizer()->Add(row_sizer, 0, wxEXPAND | wxBOTTOM, wxWindow::FromDIP(12, section));
}

wxStaticText* PreferencesLayout::AddValuePreviewRow(
	PreferencesSectionPanel* section,
	const wxString& title,
	const wxString& description,
	const wxString& value
) {
	auto* value_label = CreateBodyText(section, value, true);
	value_label->SetForegroundColour(Theme::Get(Theme::Role::Accent));
	AddControlRow(section, title, description, value_label, false);
	return value_label;
}
