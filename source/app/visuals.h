#ifndef RME_VISUALS_H_
#define RME_VISUALS_H_

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <wx/colour.h>
#include <wx/string.h>

enum class VisualMatchType {
	ItemId,
	ClientId,
	Marker,
	Overlay,
	Tile,
};

enum class MarkerVisualKind {
	Waypoint,
	HouseExitCurrent,
	HouseExitOther,
	TownTemple,
	Spawn,
	SpawnSelected,
};

enum class OverlayVisualKind {
	DoorLocked,
	DoorUnlocked,
	HookSouth,
	HookEast,
};

enum class TileVisualKind {
	Blocking,
	Pz,
	Pvp,
	NoLogout,
	NoPvp,
	HouseOverlay,
};

enum class VisualAppearanceType {
	Rgba,
	SpriteId,
	OtherItemVisual,
	Png,
	Svg,
};

struct VisualAppearance {
	VisualAppearanceType type = VisualAppearanceType::Rgba;
	wxColour color = wxColour(255, 255, 255, 255);
	uint32_t sprite_id = 0;
	uint16_t item_id = 0;
	std::string asset_path;
};

struct VisualRule {
	std::string key;
	std::string label;
	std::string group;
	VisualMatchType match_type = VisualMatchType::ItemId;
	int32_t match_id = 0;
	std::string match_value;
	VisualAppearance appearance;
	bool enabled = true;
	bool valid = true;
	std::string validation_error;
};

struct VisualCatalogEntry {
	std::optional<VisualRule> default_rule;
	std::optional<VisualRule> user_rule;

	const VisualRule* effective() const {
		return user_rule ? &*user_rule : default_rule ? &*default_rule : nullptr;
	}

	bool hasOverride() const {
		return user_rule.has_value();
	}
};

struct VisualEditContext {
	VisualRule seed_rule;
};

class Visuals {
public:
	Visuals() = default;

	bool Load();
	bool SaveUserOverrides() const;
	bool ExportUserOverrides(const wxString& path) const;
	bool ImportUserOverrides(const wxString& path);

	std::vector<VisualCatalogEntry> BuildCatalog() const;

	const VisualRule* GetDefaultRule(const std::string& key) const;
	const VisualRule* GetUserRule(const std::string& key) const;
	const VisualRule* GetEffectiveRule(const std::string& key) const;

	void SetUserRule(VisualRule rule);
	void RemoveUserRule(const std::string& key);
	void ClearUserOverrides();

	const VisualRule* ResolveItem(uint16_t item_id) const;
	const VisualRule* ResolveMarker(MarkerVisualKind kind) const;
	const VisualRule* ResolveOverlay(OverlayVisualKind kind) const;
	const VisualRule* ResolveTile(TileVisualKind kind) const;

	std::string GetAssetsName() const;
	void SetAssetsNameOverride(std::string assets_name);
	bool HasAssetsNameOverride() const {
		return user_assets_name.has_value();
	}

	wxString GetDefaultConfigPath() const {
		return default_config_path;
	}

	wxString GetUserConfigPath() const {
		return user_config_path;
	}

	static std::string MakeKeyForItemId(uint16_t item_id);
	static std::string MakeKeyForMarker(MarkerVisualKind kind);
	static std::string MakeKeyForOverlay(OverlayVisualKind kind);
	static std::string MakeKeyForTile(TileVisualKind kind);

	static VisualRule MakeItemRule(uint16_t item_id);
	static VisualRule MakeMarkerRule(MarkerVisualKind kind);
	static VisualRule MakeOverlayRule(OverlayVisualKind kind);
	static VisualRule MakeTileRule(TileVisualKind kind);

	static wxColour CombineColor(const wxColour& base_color, const wxColour& tint);
	static bool SupportsSpriteModes(const VisualRule& rule);
	static bool SupportsImageModes(const VisualRule& rule);

private:
	bool LoadDefaults();
	bool LoadUserOverrides();
	bool LoadRulesFromFile(const wxString& path, bool user_file, std::map<std::string, VisualRule>& destination, std::vector<VisualRule>& client_rules, std::optional<std::string>& assets_name_override);
	bool SaveRulesToFile(const wxString& path, const std::map<std::string, VisualRule>& rules, const std::optional<std::string>& assets_name_override) const;
	void EnsureServerItemRulesMaterialized() const;
	void InvalidateResolvedRules();
	static uint64_t CurrentItemDefinitionSignature();
	static void ExpandClientRules(const std::vector<VisualRule>& source, std::map<std::string, VisualRule>& destination);

	static std::string BuildKey(const VisualRule& rule);
	static std::string DeriveLabel(const VisualRule& rule);
	static std::string DeriveGroup(const VisualRule& rule);
	static void ValidateRule(VisualRule& rule);

	static std::string ToString(VisualMatchType type);
	static std::string ToString(MarkerVisualKind kind);
	static std::string ToString(OverlayVisualKind kind);
	static std::string ToString(TileVisualKind kind);
	static std::string ToString(VisualAppearanceType type);

	static std::optional<VisualMatchType> ParseMatchType(const std::string& value);
	static std::optional<MarkerVisualKind> ParseMarkerKind(const std::string& value);
	static std::optional<OverlayVisualKind> ParseOverlayKind(const std::string& value);
	static std::optional<TileVisualKind> ParseTileKind(const std::string& value);
	static std::optional<VisualAppearanceType> ParseAppearanceType(const std::string& value);

	wxString default_config_path;
	wxString user_config_path;
	std::string default_assets_name;
	std::optional<std::string> user_assets_name;
	mutable std::map<std::string, VisualRule> default_rules;
	mutable std::map<std::string, VisualRule> user_rules;
	mutable std::map<std::string, VisualRule> base_default_rules;
	mutable std::map<std::string, VisualRule> base_user_rules;
	mutable std::vector<VisualRule> legacy_default_client_rules;
	mutable std::vector<VisualRule> legacy_user_client_rules;
	mutable uint64_t resolved_signature = 0;
	mutable bool resolved_rules_dirty = true;
};

extern Visuals g_visuals;

#endif
