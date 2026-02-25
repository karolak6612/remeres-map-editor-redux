#ifndef RME_RENDERING_CORE_RENDER_LIST_H_
#define RME_RENDERING_CORE_RENDER_LIST_H_

#include <vector>
#include "rendering/core/sprite_instance.h"
#include "rendering/core/atlas_manager.h" // For AtlasRegion
#include "rendering/core/sprite_batch.h" // For SpriteBatch

class RenderList {
public:
	RenderList() {
		sprites_.reserve(10000);
	}

	void addSprite(float x, float y, float w, float h, const AtlasRegion& region, float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f) {
		SpriteInstance& inst = sprites_.emplace_back();
		inst.x = x;
		inst.y = y;
		inst.w = w;
		inst.h = h;
		inst.u_min = region.u_min;
		inst.v_min = region.v_min;
		inst.u_max = region.u_max;
		inst.v_max = region.v_max;
		inst.r = r;
		inst.g = g;
		inst.b = b;
		inst.a = a;
		inst.atlas_layer = static_cast<float>(region.atlas_index);
	}

	void addRect(float x, float y, float w, float h, float r, float g, float b, float a, const AtlasRegion* white_pixel) {
		if (white_pixel) {
			addSprite(x, y, w, h, *white_pixel, r, g, b, a);
		}
	}

	void addRectLines(float x, float y, float w, float h, float r, float g, float b, float a, const AtlasRegion* white_pixel) {
		// Top
		addRect(x, y, w, 1.0f, r, g, b, a, white_pixel);
		// Bottom
		addRect(x, y + h - 1.0f, w, 1.0f, r, g, b, a, white_pixel);
		// Left
		addRect(x, y + 1.0f, 1.0f, h - 2.0f, r, g, b, a, white_pixel);
		// Right
		addRect(x + w - 1.0f, y + 1.0f, 1.0f, h - 2.0f, r, g, b, a, white_pixel);
	}

	const std::vector<SpriteInstance>& getSprites() const {
		return sprites_;
	}

	void clear() {
		sprites_.clear();
	}

	void submit(SpriteBatch& sprite_batch, float offset_x = 0.0f, float offset_y = 0.0f) const {
		if (sprites_.empty()) {
			return;
		}
		sprite_batch.ensureCapacity(sprites_.size());
		for (const auto& instance : sprites_) {
			sprite_batch.push(instance, offset_x, offset_y);
		}
	}

private:
	std::vector<SpriteInstance> sprites_;
};

#endif
