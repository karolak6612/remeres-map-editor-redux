//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "ui/dialogs/missing_items_dialog.h"

#include <format>

MissingItemsDialog::MissingItemsDialog(wxWindow* parent, const MissingItemReport& report, bool hasOtb) :
	wxDialog(parent, wxID_ANY, "Missing Item Definitions Report", wxDefaultPosition, wxSize(700, 600), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	report(report),
	hasOtb(hasOtb),
	listMissingInDat(nullptr),
	listMissingInOtb(nullptr),
	listXmlNoOtb(nullptr),
	listOtbNoXml(nullptr),
	countMissingInDat(nullptr),
	countMissingInOtb(nullptr),
	countXmlNoOtb(nullptr),
	countOtbNoXml(nullptr) {
	BuildUI();
	CenterOnParent();
}

void MissingItemsDialog::BuildUI() {
	auto* mainSizer = newd wxBoxSizer(wxVERTICAL);

	// Header text
	wxString headerTextContent;
	if (hasOtb) {
		headerTextContent = "The following items have mismatched definitions between items.otb, tibia.dat, and items.xml.\n"
		                   "Items without definitions will be marked as invalid on the map.";
	} else {
		headerTextContent = "The following items have mismatched definitions between tibia.dat and items.xml.\n"
		                   "Items without definitions will be marked as invalid on the map.";
	}
	auto* headerText = newd wxStaticText(this, wxID_ANY, headerTextContent,
		wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
	mainSizer->Add(headerText, 0, wxALL | wxEXPAND, 10);

	// Create notebook for sections
	auto* notebook = newd wxNotebook(this, wxID_ANY);

	// Section 1: Missing in DAT
	auto* page1 = newd wxPanel(notebook, wxID_ANY);
	auto* sizer1 = newd wxBoxSizer(wxVERTICAL);
	countMissingInDat = newd wxStaticText(page1, wxID_ANY, "");
	sizer1->Add(countMissingInDat, 0, wxALL, 5);

	listMissingInDat = newd wxDataViewListCtrl(page1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_HORIZ_RULES);
	listMissingInDat->AppendTextColumn("Server ID", wxDATAVIEW_CELL_INERT, 100);
	listMissingInDat->AppendTextColumn("Client ID", wxDATAVIEW_CELL_INERT, 100);
	listMissingInDat->AppendTextColumn("Name", wxDATAVIEW_CELL_INERT, 200);
	listMissingInDat->AppendTextColumn("Description", wxDATAVIEW_CELL_INERT, 250);
	sizer1->Add(listMissingInDat, 1, wxALL | wxEXPAND, 5);

	for (const auto& entry : report.missing_in_dat) {
		listMissingInDat->AppendItem({
			wxString::Format("%u", entry.server_id),
			wxString::Format("%u", entry.client_id),
			wxString::FromUTF8(entry.name.empty() ? "unknown" : entry.name),
			wxString::FromUTF8(entry.description)
		});
	}
	countMissingInDat->SetLabel(wxString::Format("Items in %s missing from tibia.dat: %zu",
		hasOtb ? "items.otb" : "items.xml", report.missing_in_dat.size()));
	page1->SetSizer(sizer1);
	notebook->AddPage(page1, wxString::Format("Missing in DAT (%zu)", report.missing_in_dat.size()));

	// Section 2: Missing in OTB
	auto* page2 = newd wxPanel(notebook, wxID_ANY);
	auto* sizer2 = newd wxBoxSizer(wxVERTICAL);
	countMissingInOtb = newd wxStaticText(page2, wxID_ANY, "");
	sizer2->Add(countMissingInOtb, 0, wxALL, 5);

	listMissingInOtb = newd wxDataViewListCtrl(page2, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_HORIZ_RULES);
	listMissingInOtb->AppendTextColumn("Client ID", wxDATAVIEW_CELL_INERT, 150);
	listMissingInOtb->AppendTextColumn("Name", wxDATAVIEW_CELL_INERT, 200);
	sizer2->Add(listMissingInOtb, 1, wxALL | wxEXPAND, 5);

	for (const auto& entry : report.missing_in_otb) {
		listMissingInOtb->AppendItem({
			wxString::Format("%u", entry.client_id),
			wxString::FromUTF8(entry.name.empty() ? "(no name)" : entry.name)
		});
	}
	countMissingInOtb->SetLabel(wxString::Format("Items in tibia.dat not referenced by %s: %zu",
		hasOtb ? "items.otb" : "items.xml", report.missing_in_otb.size()));
	page2->SetSizer(sizer2);
	notebook->AddPage(page2, wxString::Format("DAT not in %s (%zu)", hasOtb ? "OTB" : "XML", report.missing_in_otb.size()));

	// Section 3: XML no OTB
	auto* page3 = newd wxPanel(notebook, wxID_ANY);
	auto* sizer3 = newd wxBoxSizer(wxVERTICAL);
	countXmlNoOtb = newd wxStaticText(page3, wxID_ANY, "");
	sizer3->Add(countXmlNoOtb, 0, wxALL, 5);

	listXmlNoOtb = newd wxDataViewListCtrl(page3, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_HORIZ_RULES);
	listXmlNoOtb->AppendTextColumn("Server ID", wxDATAVIEW_CELL_INERT, 100);
	listXmlNoOtb->AppendTextColumn("Client ID", wxDATAVIEW_CELL_INERT, 100);
	listXmlNoOtb->AppendTextColumn("Name", wxDATAVIEW_CELL_INERT, 200);
	listXmlNoOtb->AppendTextColumn("Description", wxDATAVIEW_CELL_INERT, 250);
	sizer3->Add(listXmlNoOtb, 1, wxALL | wxEXPAND, 5);

	for (const auto& entry : report.xml_no_otb) {
		listXmlNoOtb->AppendItem({
			wxString::Format("%u", entry.server_id),
			wxString::Format("%u", entry.client_id),
			wxString::FromUTF8(entry.name.empty() ? "unknown" : entry.name),
			wxString::FromUTF8(entry.description)
		});
	}
	countXmlNoOtb->SetLabel(wxString::Format("Items in items.xml missing from items.otb: %zu", report.xml_no_otb.size()));
	page3->SetSizer(sizer3);
	if (hasOtb) {
		notebook->AddPage(page3, wxString::Format("XML no OTB (%zu)", report.xml_no_otb.size()));
	}

	// Section 4: OTB no XML (only shown if hasOtb is true)
	if (hasOtb) {
		auto* page4 = newd wxPanel(notebook, wxID_ANY);
		auto* sizer4 = newd wxBoxSizer(wxVERTICAL);
		countOtbNoXml = newd wxStaticText(page4, wxID_ANY, "");
		sizer4->Add(countOtbNoXml, 0, wxALL, 5);

		listOtbNoXml = newd wxDataViewListCtrl(page4, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_HORIZ_RULES);
		listOtbNoXml->AppendTextColumn("Server ID", wxDATAVIEW_CELL_INERT, 100);
		listOtbNoXml->AppendTextColumn("Client ID", wxDATAVIEW_CELL_INERT, 100);
		listOtbNoXml->AppendTextColumn("Name", wxDATAVIEW_CELL_INERT, 200);
		listOtbNoXml->AppendTextColumn("Description", wxDATAVIEW_CELL_INERT, 250);
		sizer4->Add(listOtbNoXml, 1, wxALL | wxEXPAND, 5);

		for (const auto& entry : report.otb_no_xml) {
			listOtbNoXml->AppendItem({
				wxString::Format("%u", entry.server_id),
				wxString::Format("%u", entry.client_id),
				wxString::FromUTF8(entry.name.empty() ? "unknown" : entry.name),
				wxString::FromUTF8(entry.description)
			});
		}
		countOtbNoXml->SetLabel(wxString::Format("Items in items.otb missing from items.xml: %zu", report.otb_no_xml.size()));
		page4->SetSizer(sizer4);
		notebook->AddPage(page4, wxString::Format("OTB no XML (%zu)", report.otb_no_xml.size()));
	}

	mainSizer->Add(notebook, 1, wxALL | wxEXPAND, 5);

	// Buttons
	auto* buttonSizer = newd wxBoxSizer(wxHORIZONTAL);

	auto* copyButton = newd wxButton(this, wxID_ANY, "Copy to Clipboard");
	copyButton->Bind(wxEVT_BUTTON, &MissingItemsDialog::OnCopyToClipboard, this);
	buttonSizer->Add(copyButton, 0, wxALL, 5);

	auto* saveButton = newd wxButton(this, wxID_ANY, "Save Report...");
	saveButton->Bind(wxEVT_BUTTON, &MissingItemsDialog::OnSaveReport, this);
	buttonSizer->Add(saveButton, 0, wxALL, 5);

	buttonSizer->AddStretchSpacer();

	auto* ignoreButton = newd wxButton(this, wxID_ANY, "Ignore");
	ignoreButton->SetDefault();
	ignoreButton->Bind(wxEVT_BUTTON, &MissingItemsDialog::OnIgnore, this);
	buttonSizer->Add(ignoreButton, 0, wxALL, 5);

	mainSizer->Add(buttonSizer, 0, wxEXPAND | wxTOP, 5);

	SetSizer(mainSizer);
	mainSizer->Fit(this);
	SetMinSize(wxSize(600, 400));
}

wxString MissingItemsDialog::GenerateReportText(bool hasOtb) const {
	wxString text;
	text += "=== Missing Item Definitions Report ===\n\n";

	if (hasOtb) {
		text += wxString::Format("--- Items in items.otb missing from tibia.dat (%zu) ---\n", report.missing_in_dat.size());
	} else {
		text += wxString::Format("--- Items in items.xml missing from tibia.dat (%zu) ---\n", report.missing_in_dat.size());
	}
	for (const auto& entry : report.missing_in_dat) {
		text += wxString::Format("Server ID: %u, Client ID: %u, Name: %s, Description: %s\n",
			entry.server_id, entry.client_id, entry.name, entry.description);
	}
	text += "\n";

	if (hasOtb) {
		text += wxString::Format("--- Items in tibia.dat missing from items.otb (%zu) ---\n", report.missing_in_otb.size());
	} else {
		text += wxString::Format("--- Items in tibia.dat not referenced by items.xml (%zu) ---\n", report.missing_in_otb.size());
	}
	for (const auto& entry : report.missing_in_otb) {
		text += wxString::Format("Client ID: %u\n", entry.client_id);
	}
	text += "\n";

	if (hasOtb) {
		text += wxString::Format("--- Items in items.xml missing from items.otb (%zu) ---\n", report.xml_no_otb.size());
		for (const auto& entry : report.xml_no_otb) {
			text += wxString::Format("Server ID: %u, Client ID: %u, Name: %s, Description: %s\n",
				entry.server_id, entry.client_id, entry.name, entry.description);
		}
		text += "\n";
	}

	return text;
}

void MissingItemsDialog::OnCopyToClipboard(wxCommandEvent& WXUNUSED(evt)) {
	if (wxTheClipboard->Open()) {
		wxTheClipboard->SetData(newd wxTextDataObject(GenerateReportText(hasOtb)));
		wxTheClipboard->Close();
		wxMessageBox("Report copied to clipboard.", "Clipboard", wxOK | wxICON_INFORMATION, this);
	}
}

void MissingItemsDialog::OnSaveReport(wxCommandEvent& WXUNUSED(evt)) {
	wxFileDialog saveDlg(this, "Save Report As", "", "missing_items_report.txt",
		"Text files (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (saveDlg.ShowModal() == wxID_OK) {
		wxString path = saveDlg.GetPath();
		wxFileOutputStream stream(path);
		if (stream.IsOk()) {
			wxTextOutputStream txt(stream);
			txt << GenerateReportText(hasOtb);
			wxMessageBox("Report saved successfully.", "Save Report", wxOK | wxICON_INFORMATION, this);
		} else {
			wxMessageBox("Failed to create report file.", "Save Report Error", wxOK | wxICON_ERROR, this);
		}
	}
}

void MissingItemsDialog::OnIgnore(wxCommandEvent& WXUNUSED(evt)) {
	EndModal(wxID_OK);
}

void MissingItemsDialog::Show(wxWindow* parent, const MissingItemReport& report, bool hasOtb) {
	if (report.missing_in_dat.empty() && report.missing_in_otb.empty() && report.xml_no_otb.empty() && report.otb_no_xml.empty()) {
		return;
	}

	MissingItemsDialog dialog(parent, report, hasOtb);
	dialog.ShowModal();
}
