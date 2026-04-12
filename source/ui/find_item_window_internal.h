#ifndef RME_FIND_ITEM_WINDOW_INTERNAL_H_
#define RME_FIND_ITEM_WINDOW_INTERNAL_H_

#include "ui/find_item_window.h"

#include <wx/srchctrl.h>

#include <array>
#include <optional>
#include <string_view>
#include <utility>

class wxSearchCtrl;

template <typename Enum>
constexpr size_t findItemDialogIndex(Enum value) {
	return static_cast<size_t>(std::to_underlying(value));
}

class ForwardingSearchCtrl final : public wxSearchCtrl {
public:
	ForwardingSearchCtrl(wxWindow* parent, wxWindowID id);

private:
	void OnKeyDown(wxKeyEvent& event);
};

constexpr auto kTypeCheckboxes = std::to_array<std::pair<AdvancedFinderTypeFilter, std::string_view>>({
	{ AdvancedFinderTypeFilter::Depot, "Depot" },
	{ AdvancedFinderTypeFilter::Mailbox, "Mailbox" },
	{ AdvancedFinderTypeFilter::TrashHolder, "Trash Holder" },
	{ AdvancedFinderTypeFilter::Container, "Container" },
	{ AdvancedFinderTypeFilter::Door, "Door" },
	{ AdvancedFinderTypeFilter::MagicField, "Magic Field" },
	{ AdvancedFinderTypeFilter::Teleport, "Teleport" },
	{ AdvancedFinderTypeFilter::Bed, "Bed" },
	{ AdvancedFinderTypeFilter::Key, "Key" },
	{ AdvancedFinderTypeFilter::Podium, "Podium" },
	{ AdvancedFinderTypeFilter::Weapon, "Weapon" },
	{ AdvancedFinderTypeFilter::Ammo, "Ammo" },
	{ AdvancedFinderTypeFilter::Armor, "Armor" },
	{ AdvancedFinderTypeFilter::Rune, "Rune" },
	{ AdvancedFinderTypeFilter::Creature, "Creature" },
});

constexpr auto kPropertyCheckboxes = std::to_array<std::pair<AdvancedFinderPropertyFilter, std::string_view>>({
	{ AdvancedFinderPropertyFilter::Unpassable, "Unpassable" },
	{ AdvancedFinderPropertyFilter::Unmovable, "Unmovable" },
	{ AdvancedFinderPropertyFilter::BlockMissiles, "Block Missiles" },
	{ AdvancedFinderPropertyFilter::BlockPathfinder, "Block Pathfinder" },
	{ AdvancedFinderPropertyFilter::HasElevation, "Has Elevation" },
	{ AdvancedFinderPropertyFilter::FloorChange, "Floor Change" },
	{ AdvancedFinderPropertyFilter::FullTile, "Full Tile" },
});

constexpr auto kInteractionCheckboxes = std::to_array<std::pair<AdvancedFinderInteractionFilter, std::string_view>>({
	{ AdvancedFinderInteractionFilter::Readable, "Readable" },
	{ AdvancedFinderInteractionFilter::Writeable, "Writeable" },
	{ AdvancedFinderInteractionFilter::Pickupable, "Pickupable" },
	{ AdvancedFinderInteractionFilter::ForceUse, "Force Use" },
	{ AdvancedFinderInteractionFilter::DistRead, "Dist Read" },
	{ AdvancedFinderInteractionFilter::Rotatable, "Rotatable" },
	{ AdvancedFinderInteractionFilter::Hangable, "Hangable" },
});

constexpr auto kVisualCheckboxes = std::to_array<std::pair<AdvancedFinderVisualFilter, std::string_view>>({
	{ AdvancedFinderVisualFilter::HasLight, "Has Light" },
	{ AdvancedFinderVisualFilter::Animation, "Animation" },
	{ AdvancedFinderVisualFilter::AlwaysTop, "Always Top" },
	{ AdvancedFinderVisualFilter::IgnoreLook, "Ignore Look" },
	{ AdvancedFinderVisualFilter::HasCharges, "Has Charges" },
	{ AdvancedFinderVisualFilter::ClientCharges, "Client Charges" },
	{ AdvancedFinderVisualFilter::Decays, "Decays" },
	{ AdvancedFinderVisualFilter::HasSpeed, "Has Speed" },
});

struct SessionFinderState {
	AdvancedFinderPersistedState persisted;
	AdvancedFinderResultViewMode result_view_mode = AdvancedFinderResultViewMode::Grid;
};

extern std::optional<SessionFinderState> g_session_finder_state;

#endif
