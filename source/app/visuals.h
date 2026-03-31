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
	LightIndicator,
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

enum class VisualResourceKind {
	None,
	FlatColor,
	NativeSpriteId,
	NativeItemVisual,
	AtlasSprite,
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

struct ResolvedVisualResource {
	VisualResourceKind kind = VisualResourceKind::None;
	wxColour color = wxColour(255, 255, 255, 255);
	uint32_t sprite_id = 0;
	uint16_t item_id = 0;
	uint32_t atlas_sprite_id = 0;
	bool valid = false;
};

struct AtlasSpriteResource {
	uint32_t sprite_id = 0;
	std::vector<std::uint8_t> rgba_pixels;
};

class AtlasManager;

class VisualResourceRegistry {
public:
	void Clear();
	void SetRuleResource(std::string key, ResolvedVisualResource resource);
	const ResolvedVisualResource* GetRuleResource(const std::string& key) const;
	void SetFallbackOverlayResource(OverlayVisualKind kind, ResolvedVisualResource resource);
	const ResolvedVisualResource* GetFallbackOverlayResource(OverlayVisualKind kind) const;
	uint32_t AddAtlasSprite(std::vector<std::uint8_t> rgba_pixels);
	void EnsureAtlasResourcesUploaded(AtlasManager& atlas) const;

private:
	std::map<std::string, ResolvedVisualResource> rule_resources_;
	std::map<OverlayVisualKind, ResolvedVisualResource> fallback_overlay_resources_;
	std::vector<AtlasSpriteResource> atlas_sprites_;
	uint32_t next_custom_sprite_id_ = 2'800'000;
	mutable uint32_t uploaded_texture_id_ = 0;
	mutable size_t uploaded_sprite_count_ = 0;
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
	bool PrepareRuntimeResources();

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
	const ResolvedVisualResource* ResolveItemResource(uint16_t item_id) const;
	const ResolvedVisualResource* ResolveMarkerResource(MarkerVisualKind kind) const;
	const ResolvedVisualResource* ResolveOverlayResource(OverlayVisualKind kind) const;
	const ResolvedVisualResource* ResolveTileResource(TileVisualKind kind) const;
	const ResolvedVisualResource* GetFallbackOverlayResource(OverlayVisualKind kind) const;
	void EnsureAtlasResourcesUploaded(AtlasManager& atlas) const;

	static std::string GetApplicationName();
	static std::string GetSiteUrl();
	static std::string GetAssetsName();

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
	static wxColour EffectiveImageTint(const wxColour& tint);
	static bool SupportsSpriteModes(const VisualRule& rule);
	static bool SupportsImageModes(const VisualRule& rule);

private:
	bool LoadDefaults();
	bool LoadUserOverrides();
	bool LoadRulesFromFile(const wxString& path, bool user_file, std::map<std::string, VisualRule>& destination, std::vector<VisualRule>& client_rules);
	bool SaveRulesToFile(const wxString& path, const std::map<std::string, VisualRule>& rules) const;
	void EnsureServerItemRulesMaterialized() const;
	void InvalidateResolvedRules();
	void InvalidateRuntimeResources();
	static uint64_t CurrentItemDefinitionSignature();
	static void ExpandClientRules(const std::vector<VisualRule>& source, std::map<std::string, VisualRule>& destination);
	void EnsureRuntimeResourcesPrepared() const;
	bool PrepareUserRulesForSerialization() const;
	bool EnsureManagedAssetPath(VisualRule& rule) const;
	static wxString ResolveAssetPath(const std::string& asset_path);
	static wxString GetManagedAssetsDirectory();

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
	mutable std::map<std::string, VisualRule> default_rules;
	mutable std::map<std::string, VisualRule> user_rules;
	mutable std::map<std::string, VisualRule> base_default_rules;
	mutable std::map<std::string, VisualRule> base_user_rules;
	mutable std::vector<VisualRule> legacy_default_client_rules;
	mutable std::vector<VisualRule> legacy_user_client_rules;
	mutable uint64_t resolved_signature = 0;
	mutable bool resolved_rules_dirty = true;
	mutable VisualResourceRegistry resource_registry;
	mutable bool runtime_resources_dirty = true;
};

extern Visuals g_visuals;

#endif
