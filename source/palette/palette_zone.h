#ifndef RME_PALETTE_ZONE_H_
#define RME_PALETTE_ZONE_H_

#include "palette/palette_common.h"

#include <optional>
#include <string>

class Map;
class wxBitmapButton;
class wxListBox;
class wxTextCtrl;

class ZonePalettePanel final : public PalettePanel {
public:
	ZonePalettePanel(wxWindow* parent, wxWindowID id = wxID_ANY);
	~ZonePalettePanel() override = default;

	PaletteType GetType() const override;
	wxString GetName() const override;
	Brush* GetSelectedBrush() const override;
	int GetSelectedBrushSize() const override;
	bool SelectBrush(const Brush* whatbrush) override;
	void OnSwitchIn() override;
	void OnUpdate() override;

	void SetMap(Map* map);

	void OnSelectZone(wxCommandEvent& event);
	void OnAddZone(wxCommandEvent& event);
	void OnRenameZone(wxCommandEvent& event);
	void OnRemoveZone(wxCommandEvent& event);
	void OnZoneTextChanged(wxCommandEvent& event);

private:
	[[nodiscard]] std::string GetSelectedZoneName() const;
	[[nodiscard]] std::optional<uint16_t> GetSelectedZoneId() const;
	[[nodiscard]] std::string ResolvePreferredZoneName() const;
	void RefreshZoneList();
	void SyncSelection(const std::string& zone_name);
	void SyncZoneText(const std::string& zone_name);
	void UpdateButtonState();
	void SetMutatingUi(bool value);
	[[nodiscard]] bool IsMutatingUi() const;
	[[nodiscard]] bool HasMap() const;

	Map* map;
	wxListBox* zone_list;
	wxTextCtrl* zone_text;
	wxBitmapButton* add_button;
	wxBitmapButton* rename_button;
	wxBitmapButton* remove_button;
	bool mutating_ui;
};

#endif
