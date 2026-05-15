#ifndef RME_PALETTE_SPAWN_PALETTE_H_
#define RME_PALETTE_SPAWN_PALETTE_H_

#include "palette/palette_common.h"

class SpawnPalettePanel final : public PalettePanel {
public:
	explicit SpawnPalettePanel(wxWindow* parent, wxWindowID id = wxID_ANY);

	wxString GetName() const override;
	Brush* GetSelectedBrush() const override;
	int GetSelectedBrushSize() const override;
	bool SelectBrush(const Brush* whatbrush) override;
	void OnSwitchIn() override;
};

#endif
