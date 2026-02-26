#ifndef RME_UI_DIALOGS_FIND_DIALOG_H_
#define RME_UI_DIALOGS_FIND_DIALOG_H_

#include "app/main.h"
#include "util/nanovg_canvas.h"
#include <wx/wx.h>
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

class FindDialogListBox : public NanoVGCanvas {
public:
	FindDialogListBox(wxWindow* parent, wxWindowID id);
	~FindDialogListBox();

	void Clear();
	void SetNoMatches();
	void AddBrush(Brush*);
	void CommitUpdates();
	Brush* GetSelectedBrush();

	size_t GetItemCount() const {
		return brushlist.size();
	}

	void SetSelection(int n);
	int GetSelection() const {
		return m_selection;
	}

	void GetSize(int* w, int* h) const {
		*w = m_columns;
		*h = (brushlist.size() + m_columns - 1) / std::max(1, m_columns);
	}

protected:
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;
	wxSize DoGetBestClientSize() const override;

	void OnSize(wxSizeEvent& event);
	void OnMouseDown(wxMouseEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnMotion(wxMouseEvent& event);
	void OnLeave(wxMouseEvent& event);

	void UpdateGrid();
	void EnsureVisible(int index);

	bool cleared;
	bool no_matches;
	std::vector<Brush*> brushlist;

	int m_columns = 1;
	int m_item_width;
	int m_item_height;
	int m_selection = -1;
	int m_hoverIndex = -1;
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
