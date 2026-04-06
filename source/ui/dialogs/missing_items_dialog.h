//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_MISSING_ITEMS_DIALOG_H_
#define RME_MISSING_ITEMS_DIALOG_H_

#include "app/main.h"

#include <wx/wx.h>
#include <wx/dataview.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/clipbrd.h>
#include <wx/filedlg.h>
#include <wx/textfile.h>
#include <wx/stream.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>

#include "item_definitions/core/missing_item_report.h"

class MissingItemsDialog : public wxDialog {
public:
	MissingItemsDialog(wxWindow* parent, const MissingItemReport& report, bool hasOtb = true);

	static void Show(wxWindow* parent, const MissingItemReport& report, bool hasOtb = true);

private:
	void BuildUI();
	void OnCopyToClipboard(wxCommandEvent& evt);
	void OnSaveReport(wxCommandEvent& evt);
	void OnIgnore(wxCommandEvent& evt);

	wxString GenerateReportText(bool hasOtb) const;

	const MissingItemReport& report;
	const bool hasOtb;

	wxDataViewListCtrl* listMissingInDat;
	wxDataViewListCtrl* listMissingInOtb;
	wxDataViewListCtrl* listXmlNoOtb;
	wxDataViewListCtrl* listOtbNoXml;

	wxStaticText* countMissingInDat;
	wxStaticText* countMissingInOtb;
	wxStaticText* countXmlNoOtb;
	wxStaticText* countOtbNoXml;
};

#endif
