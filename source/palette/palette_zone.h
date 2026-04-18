#ifndef RME_PALETTE_ZONE_H_
#define RME_PALETTE_ZONE_H_

#include "palette/palette_common.h"

#include <optional>
#include <string>

class Map;
class wxButton;
class wxDataViewEvent;
class wxDataViewListCtrl;

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

	void OnSelectZone(wxDataViewEvent& event);
	void OnActivateZone(wxDataViewEvent& event);
	void OnAddZone(wxCommandEvent& event);
	void OnEditZone(wxCommandEvent& event);
	void OnRemoveZone(wxCommandEvent& event);

private:
	[[nodiscard]] std::string GetSelectedZoneName() const;
	[[nodiscard]] std::optional<uint16_t> GetSelectedZoneId() const;
	[[nodiscard]] std::string ResolvePreferredZoneName() const;
	void JumpToZone(uint16_t zone_id);
	[[nodiscard]] std::optional<std::string> PromptForZoneName(const wxString& title, const std::string& initial_value) const;
	void RefreshZoneList();
	void SyncSelection(const std::string& zone_name);
	void UpdateButtonState();
	void SetMutatingUi(bool value);
	[[nodiscard]] bool IsMutatingUi() const;
	[[nodiscard]] bool HasMap() const;

	Map* map;
	wxDataViewListCtrl* zone_list;
	wxButton* add_button;
	wxButton* edit_button;
	wxButton* remove_button;
	bool mutating_ui;
};

#endif
