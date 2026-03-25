//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_CORE_GAME_SPRITE_H_
#define RME_RENDERING_CORE_GAME_SPRITE_H_

#include "game/outfit.h"
#include "util/common.h"
#include "rendering/core/atlas_region_cache.h"
#include "rendering/core/sprite_animation_state.h"
#include "rendering/core/sprite_icon_data.h"
#include "rendering/core/sprite_light.h"
#include "rendering/core/texture_garbage_collector.h"
#include "rendering/core/atlas_manager.h"
#include "rendering/core/sprite_metadata.h"
#include <atomic>
#include <cstdint>
#include <span>

#include <wx/dc.h>

enum SpriteSize : int {
	SPRITE_SIZE_16x16,
	// SPRITE_SIZE_24x24,
	SPRITE_SIZE_32x32,
	SPRITE_SIZE_64x64,
	SPRITE_SIZE_COUNT
};

class GraphicManager;
class SpritePreloader;

class Sprite {
public:
	Sprite() { }
	virtual ~Sprite() = default;

	virtual void DrawTo(wxDC* dc, SpriteSize sz, int start_x, int start_y, int width = -1, int height = -1) = 0;
	virtual void unloadDC() = 0;
	virtual wxSize GetSize() const = 0;

private:
	Sprite(const Sprite&);
	Sprite& operator=(const Sprite&);
};

class GameSprite;
class CreatureSprite : public Sprite {
public:
	CreatureSprite(GameSprite* parent, const Outfit& outfit);
	~CreatureSprite() override;

	void DrawTo(wxDC* dc, SpriteSize sz, int start_x, int start_y, int width = -1, int height = -1) override;
	virtual void unloadDC() override;
	wxSize GetSize() const override {
		return wxSize(32, 32);
	}

	GameSprite* parent;
	Outfit outfit;
};

class Image;
class NormalImage;
class TemplateImage;

class GameSprite : public Sprite {
public:
	GameSprite();
	~GameSprite() override;
	void setGraphicManager(GraphicManager* graphics);
	[[nodiscard]] GraphicManager* graphics() const { return graphics_; }

	size_t getIndex(int width, int height, int layer, int pattern_x, int pattern_y, int pattern_z, int frame) const;

	// Phase 2: Get atlas region for texture array rendering
	const AtlasRegion* getAtlasRegion(int _x, int _y, int _layer, int _subtype, int _pattern_x, int _pattern_y, int _pattern_z, int _frame);
	const AtlasRegion* getAtlasRegion(int _x, int _y, int _dir, int _addon, int _pattern_z, const Outfit& _outfit, int _frame);

	void DrawTo(wxDC* dc, SpriteSize sz, int start_x, int start_y, int width = -1, int height = -1) override;
	virtual void DrawTo(wxDC* dc, SpriteSize sz, const Outfit& outfit, int start_x, int start_y, int width = -1, int height = -1);

	void unloadDC() override;

	wxSize GetSize() const override {
		return wxSize(meta.width * 32, meta.height * 32);
	}

	void clean(time_t time, int longevity = -1);

	int getDrawHeight() const;
	std::pair<int, int> getDrawOffset() const;
	uint8_t getMiniMapColor() const;

	bool hasLight() const noexcept {
		return meta.has_light;
	}
	const SpriteLight& getLight() const noexcept {
		return meta.light;
	}

	// Sprite metadata (public for GraphicsAssembler direct writes)
	SpriteMetadata meta;
	AtlasRegionCache atlas_cache;
	SpriteAnimationState animation;
	SpriteIconData icon_data;

	[[nodiscard]] uint32_t getId() const noexcept { return meta.id; }
	[[nodiscard]] uint8_t getHeight() const noexcept { return meta.height; }
	[[nodiscard]] uint8_t getWidth() const noexcept { return meta.width; }
	[[nodiscard]] uint8_t getLayers() const noexcept { return meta.layers; }
	[[nodiscard]] uint8_t getPatternX() const noexcept { return meta.pattern_x; }
	[[nodiscard]] uint8_t getPatternY() const noexcept { return meta.pattern_y; }
	[[nodiscard]] uint8_t getPatternZ() const noexcept { return meta.pattern_z; }
	[[nodiscard]] uint8_t getFrames() const noexcept { return meta.frames; }
	[[nodiscard]] uint32_t getNumSprites() const noexcept { return meta.numsprites; }
	[[nodiscard]] bool isSimple() const noexcept { return meta.is_simple; }
	uint32_t getDebugImageId(size_t index = 0) const;
	TemplateImage* getTemplateImage(int sprite_index, const Outfit& outfit);

	bool is_resident = false;

	// Exposed for fast-path rendering (BlitItem)
	const AtlasRegion* getCachedDefaultRegion() const {
		return atlas_cache.cached_default_region;
	}

	uint32_t getSpriteId(int frameIndex, int pattern_x, int pattern_y) const;

	bool isSimpleAndLoaded() const;

	void updateSimpleStatus() {
		meta.is_simple = (meta.numsprites == 1 && meta.frames == 1 && meta.layers == 1 && meta.width == 1 && meta.height == 1 && !icon_data.sprite_list.empty());
	}

private:
	GraphicManager* graphics_ = nullptr;
};

#endif
