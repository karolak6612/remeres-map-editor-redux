//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "ui/toolbar/toolbar_factory.h"

std::unique_ptr<ToolbarRegistry> ToolbarFactory::CreateToolbars(wxWindow* parent) {
	auto registry = std::make_unique<ToolbarRegistry>();

	// Use newd macro as per project convention if applicable, or just new.
	// The original code used `newd`.

	// Toolbars are owned by 'parent' (MainFrame) via wxWindow mechanism.
	// ToolbarRegistry holds non-owning pointers for access.
	registry->SetStandardToolbar(newd StandardToolBar(parent));
	registry->SetBrushToolbar(newd BrushToolBar(parent));
	registry->SetPositionToolbar(newd PositionToolBar(parent));
	registry->SetSizeToolbar(newd SizeToolBar(parent));
	registry->SetLightToolbar(newd LightToolBar(parent));

	return registry;
}
