//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_UI_TOOLBAR_TOOLBAR_REGISTRY_H_
#define RME_UI_TOOLBAR_TOOLBAR_REGISTRY_H_

#include <wx/wx.h>
#include <wx/aui/aui.h>
#include <memory>

#include "ui/gui_ids.h"
#include "ui/toolbar/standard_toolbar.h"
#include "ui/toolbar/brush_toolbar.h"
#include "ui/toolbar/position_toolbar.h"
#include "ui/toolbar/size_toolbar.h"
#include "ui/toolbar/light_toolbar.h"

// Responsibility: Holds pointers to all toolbar components and manages their lifecycle.
class ToolbarRegistry {
public:
	ToolbarRegistry();
	~ToolbarRegistry();

	void SetStandardToolbar(StandardToolBar* tb);
	void SetBrushToolbar(BrushToolBar* tb);
	void SetPositionToolbar(PositionToolBar* tb);
	void SetSizeToolbar(SizeToolBar* tb);
	void SetLightToolbar(LightToolBar* tb);

	StandardToolBar* GetStandardToolbar() const {
		return standard_toolbar;
	}
	BrushToolBar* GetBrushToolbar() const {
		return brush_toolbar;
	}
	PositionToolBar* GetPositionToolbar() const {
		return position_toolbar;
	}
	SizeToolBar* GetSizeToolbar() const {
		return size_toolbar;
	}
	LightToolBar* GetLightToolbar() const {
		return light_toolbar;
	}

	wxAuiPaneInfo& GetPane(ToolBarID id, wxAuiManager* manager);

	void UpdateAll();

private:
	// Pointers owned by MainFrame (parent window)
	StandardToolBar* standard_toolbar;
	BrushToolBar* brush_toolbar;
	PositionToolBar* position_toolbar;
	SizeToolBar* size_toolbar;
	LightToolBar* light_toolbar;
};

#endif // RME_UI_TOOLBAR_TOOLBAR_REGISTRY_H_
