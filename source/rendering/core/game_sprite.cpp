//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "rendering/core/game_sprite.h"
#include "rendering/core/graphics.h"
#include "ui/gui.h"
#include "app/settings.h"
#include "rendering/core/normal_image.h"
#include "rendering/core/template_image.h"
#include <spdlog/spdlog.h>
#include <atomic>
#include <algorithm>
#include <ranges>
#include <span>

CreatureSprite::CreatureSprite(GameSprite* parent, const Outfit& outfit) :
	parent(parent),
	outfit(outfit) {
}

CreatureSprite::~CreatureSprite() {
}

void CreatureSprite::DrawTo(wxDC* dc, SpriteSize sz, int start_x, int start_y, int width, int height) {
	if (parent) {
		parent->DrawTo(dc, sz, outfit, start_x, start_y, width, height);
	}
}

void CreatureSprite::unloadDC() {
	if (parent) {
		SpriteIconRenderer::RenderKey key;
		key.colorHash = outfit.getColorHash();
		key.mountColorHash = outfit.getMountColorHash();
		key.lookMount = outfit.lookMount;
		key.lookAddon = outfit.lookAddon;
		key.lookMountHead = outfit.lookMountHead;
		key.lookMountBody = outfit.lookMountBody;
		key.lookMountLegs = outfit.lookMountLegs;
		key.lookMountFeet = outfit.lookMountFeet;

		key.size = SPRITE_SIZE_16x16;
		parent->iconRenderer().eraseColoredDC(key);

		key.size = SPRITE_SIZE_32x32;
		parent->iconRenderer().eraseColoredDC(key);
	}
}

GameSprite::GameSprite() :
	animator(nullptr) {
}

GameSprite::~GameSprite() {
	unloadDC();
}

void GameSprite::clean(time_t time, int longevity) {
	for (auto& iter : instanced_templates) {
		iter->clean(time, longevity);
	}
}

void GameSprite::unloadDC() {
	icon_renderer_.unloadDC();
}

int GameSprite::getDrawHeight() const {
	return meta.draw_height;
}

bool GameSprite::isSimpleAndLoaded() const {
	return meta.is_simple && spriteList[0]->isGLLoaded;
}

uint32_t GameSprite::getDebugImageId(size_t index) const {
	if (index < spriteList.size() && spriteList[index]->isNormalImage()) {
		return static_cast<const NormalImage*>(spriteList[index])->id;
	}
	return 0;
}

uint32_t GameSprite::getSpriteId(int frameIndex, int pattern_x, int pattern_y) const {
	auto idx = getIndex(meta.width, meta.height, 0, pattern_x, pattern_y, 0, frameIndex);
	if (idx >= 0 && static_cast<size_t>(idx) < spriteList.size() && spriteList[idx]->isNormalImage()) {
		return static_cast<const NormalImage*>(spriteList[idx])->id;
	}
	return 0;
}

std::pair<int, int> GameSprite::getDrawOffset() const {
	return std::make_pair(meta.drawoffset_x, meta.drawoffset_y);
}

uint8_t GameSprite::getMiniMapColor() const {
	return static_cast<uint8_t>(meta.minimap_color);
}

size_t GameSprite::getIndex(int width, int height, int layer, int pattern_x, int pattern_y, int pattern_z, int frame) const {
	if (meta.is_simple) {
		return 0;
	}
	if (meta.frames == 0) {
		return 0;
	}
	size_t idx = (meta.frames > 1) ? frame % meta.frames : 0;
	idx = idx * static_cast<size_t>(meta.pattern_z) + static_cast<size_t>(pattern_z);
	idx = idx * static_cast<size_t>(meta.pattern_y) + static_cast<size_t>(pattern_y);
	idx = idx * static_cast<size_t>(meta.pattern_x) + static_cast<size_t>(pattern_x);
	idx = idx * static_cast<size_t>(meta.layers) + static_cast<size_t>(layer);
	idx = idx * static_cast<size_t>(meta.height) + static_cast<size_t>(height);
	idx = idx * static_cast<size_t>(meta.width) + static_cast<size_t>(width);
	return idx;
}

const AtlasRegion* GameSprite::getAtlasRegion(int _x, int _y, int _layer, int _count, int _pattern_x, int _pattern_y, int _pattern_z, int _frame) {
	if (meta.numsprites == 0) {
		return nullptr;
	}

	if (_count == -1 && meta.numsprites == 1 && meta.frames == 1 && meta.layers == 1 && meta.width == 1 && meta.height == 1) {
		if (_x == 0 && _y == 0 && _layer == 0 && _frame == 0 && _pattern_x == 0 && _pattern_y == 0 && _pattern_z == 0) {
			if (atlas_cache.cached_default_region && spriteList[0]->isGLLoaded && atlas_cache.cached_generation_id == spriteList[0]->generation_id && atlas_cache.cached_sprite_id == spriteList[0]->id) {
				return atlas_cache.cached_default_region;
			}

			const AtlasRegion* valid_region = spriteList[0]->getAtlasRegion();
			if (valid_region && spriteList[0]->isGLLoaded) {
				atlas_cache.cached_default_region = valid_region;
				atlas_cache.cached_generation_id = spriteList[0]->generation_id;
				atlas_cache.cached_sprite_id = spriteList[0]->id;
			} else {
				atlas_cache.invalidate();
			}

			return valid_region;
		}
	}

	uint32_t v;
	if (_count >= 0 && meta.height <= 1 && meta.width <= 1) {
		v = _count;
	} else {
		v = ((((((_frame)*meta.pattern_y + _pattern_y) * meta.pattern_x + _pattern_x) * meta.layers + _layer) * meta.height + _y) * meta.width + _x);
	}
	if (v >= meta.numsprites) {
		if (meta.numsprites == 1) {
			v = 0;
		} else {
			v %= meta.numsprites;
		}
	}

	if (spriteList[v]) {
		return spriteList[v]->getAtlasRegion();
	}
	return nullptr;
}

TemplateImage* GameSprite::getTemplateImage(int sprite_index, const Outfit& outfit) {
	auto it = std::ranges::find_if(instanced_templates, [sprite_index, &outfit](const auto& img) {
		if (img->sprite_index != sprite_index) {
			return false;
		}
		uint32_t lookHash = img->lookHead << 24 | img->lookBody << 16 | img->lookLegs << 8 | img->lookFeet;
		return outfit.getColorHash() == lookHash;
	});

	if (it != instanced_templates.end()) {
		if (it != instanced_templates.begin()) {
			std::iter_swap(it, instanced_templates.begin());
			return instanced_templates.front().get();
		}
		return it->get();
	}

	auto img = std::make_unique<TemplateImage>(this, sprite_index, outfit);
	TemplateImage* ptr = img.get();
	instanced_templates.push_back(std::move(img));
	return ptr;
}

const AtlasRegion* GameSprite::getAtlasRegion(int _x, int _y, int _dir, int _addon, int _pattern_z, const Outfit& _outfit, int _frame) {
	if (meta.numsprites == 0) {
		return nullptr;
	}

	uint32_t v = getIndex(_x, _y, 0, _dir, _addon, _pattern_z, _frame);
	if (v >= meta.numsprites) {
		if (meta.numsprites == 1) {
			v = 0;
		} else {
			v %= meta.numsprites;
		}
	}
	if (meta.layers > 1) {
		TemplateImage* img = getTemplateImage(v, _outfit);
		return img->getAtlasRegion();
	}
	if (spriteList[v]) {
		return spriteList[v]->getAtlasRegion();
	}
	return nullptr;
}

void GameSprite::DrawTo(wxDC* dc, SpriteSize sz, int start_x, int start_y, int width, int height) {
	icon_renderer_.DrawTo(dc, sz, this, start_x, start_y, width, height);
}

void GameSprite::DrawTo(wxDC* dc, SpriteSize sz, const Outfit& outfit, int start_x, int start_y, int width, int height) {
	icon_renderer_.DrawTo(dc, sz, this, outfit, start_x, start_y, width, height);
}
