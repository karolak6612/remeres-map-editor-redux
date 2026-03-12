#ifndef RME_UI_DIALOGS_FIND_DIALOG_H_
#define RME_UI_DIALOGS_FIND_DIALOG_H_

#include "app/main.h"
#include "util/nanovg_listbox.h"
#include <wx/wx.h>
#include <wx/tglbtn.h>
#include <vector>

class Brush;

class KeyForwardingTextCtrl : public wxTextCtrl {
public:
	KeyForwardingTextCtrl(wxWindow* parent, wxWindowID id, const wxString& value = "", const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0, const wxValidator& validator = wxDefaultValidator, const wxString& name = wxTextCtrlNameStr) :
		wxTextCtrl(parent, id, value, pos, size, style, validator, name) {
		Bind(wxEVT_KEY_DOWN, &KeyForwardingTextCtrl::OnKeyDown, this);
	}
	~KeyForwardingTextCtrl() { }

	void OnKeyDown(wxKeyEvent&);
};

class FindDialogListBox : public NanoVGListBox {
public:
	FindDialogListBox(wxWindow* parent, wxWindowID id);
	~FindDialogListBox();

	void Clear();
	void SetNoMatches();
	void AddBrush(Brush*);
	void CommitUpdates();
	Brush* GetSelectedBrush();

	void SetListMode(bool is_list);

	void OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t index) override;
	int OnMeasureItem(size_t index) const override;

	// Override some NanoVGCanvas things for grid layout
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;
	wxSize DoGetBestClientSize() const override;

protected:
	bool cleared;
	bool no_matches;
	bool is_list_mode;
	std::vector<Brush*> brushlist;

	int columns;
	int item_size;
	int padding;

	int HitTest(int x, int y) const;
	wxRect GetItemRect(int index) const;
	void UpdateLayout();

	void OnSize(wxSizeEvent& event);
	void OnMouseDown(wxMouseEvent& event);
	void OnMotion(wxMouseEvent& event);
};

class FindDialog : public wxDialog {
public:
	FindDialog(wxWindow* parent, wxString title);
	virtual ~FindDialog();

	void OnKeyDown(wxKeyEvent&);
	void OnTextChange(wxCommandEvent&);
	void OnTextIdle(wxTimerEvent&);
	void OnClickList(wxCommandEvent&);
	void OnClickOK(wxCommandEvent&);
	void OnClickCancel(wxCommandEvent&);
	void OnToggleListMode(wxCommandEvent&);

	void RefreshContents();
	virtual const Brush* getResult() const {
		return result_brush;
	}
	virtual int getResultID() const {
		return result_id;
	}

protected:
	virtual void RefreshContentsInternal() = 0;
	virtual void OnClickListInternal(wxCommandEvent&) = 0;
	virtual void OnClickOKInternal() = 0;

	FindDialogListBox* item_list;
	KeyForwardingTextCtrl* search_field;
	wxToggleButton* list_mode_btn;
	wxTimer idle_input_timer;
	const Brush* result_brush;
	int result_id;
};

class FindBrushDialog : public FindDialog {
public:
	FindBrushDialog(wxWindow* parent, wxString title = "Jump to Brush");
	virtual ~FindBrushDialog();

	virtual void RefreshContentsInternal();
	virtual void OnClickListInternal(wxCommandEvent&);
	virtual void OnClickOKInternal();
};

#endif
