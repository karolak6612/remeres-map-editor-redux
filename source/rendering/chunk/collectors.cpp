#include "rendering/chunk/collectors.h"
#include "rendering/core/atlas_manager.h"
#include "rendering/core/texture_atlas.h"

void SpriteCollector::draw(float x, float y, float w, float h, const AtlasRegion& region) {
	draw(x, y, w, h, region, 1.0f, 1.0f, 1.0f, 1.0f);
}

void SpriteCollector::draw(float x, float y, float w, float h, const AtlasRegion& region, float r, float g, float b, float a) {
	SpriteInstance& inst = sprites.emplace_back();
	inst.x = x;
	inst.y = y;
	inst.w = w;
	inst.h = h;
	inst.u_min = region.u_min;
	inst.v_min = region.v_min;
	inst.u_max = region.u_max;
	inst.v_max = region.v_max;

	// Apply global tint
	inst.r = r * current_tint.r;
	inst.g = g * current_tint.g;
	inst.b = b * current_tint.b;
	inst.a = a * current_tint.a;

	inst.atlas_layer = static_cast<float>(region.atlas_index);
}

void SpriteCollector::drawRect(float x, float y, float w, float h, const glm::vec4& color, const AtlasManager& atlas_manager) {
	const AtlasRegion* region = atlas_manager.getWhitePixel();
	if (region) {
		draw(x, y, w, h, *region, color.r, color.g, color.b, color.a);
	}
}

void SpriteCollector::setGlobalTint(float r, float g, float b, float a, const AtlasManager& atlas_manager) {
	current_tint = glm::vec4(r, g, b, a);
}

void SpriteCollector::reportMissingSprite(uint32_t id) {
	missing_sprites.insert(id);
}

void LightCollector::AddLight(int map_x, int map_y, int map_z, const SpriteLight& light) {
	if (map_z <= GROUND_LAYER) {
		map_x -= (GROUND_LAYER - map_z);
		map_y -= (GROUND_LAYER - map_z);
	}

	// Validate bounds (ClientMapWidth is just a safe max, but we use uint16)
	if (map_x > 0 && map_x <= MAP_MAX_WIDTH && map_y > 0 && map_y <= MAP_MAX_HEIGHT) {
		LightBuffer::Light l;
		l.map_x = static_cast<uint16_t>(map_x);
		l.map_y = static_cast<uint16_t>(map_y);
		l.color = light.color;
		l.intensity = light.intensity;
		lights.push_back(l);
	}
}
