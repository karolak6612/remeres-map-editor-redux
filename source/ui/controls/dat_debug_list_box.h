//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_DAT_DEBUG_LIST_BOX_H_
#define RME_DAT_DEBUG_LIST_BOX_H_

#include "app/main.h"
#include "util/nanovg_listbox.h"
#include <vector>

class Sprite;

class DatDebugListBox : public NanoVGListBox {
public:
	DatDebugListBox(wxWindow* parent, wxWindowID id);
	~DatDebugListBox();

	void OnDrawItem(NVGcontext* vg, const wxRect& rect, size_t index) override;
	int OnMeasureItem(size_t index) const override;

protected:
	std::vector<Sprite*> sprites;
};

#endif
