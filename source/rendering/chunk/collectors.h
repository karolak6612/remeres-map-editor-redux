#ifndef RME_RENDERING_CHUNK_COLLECTORS_H_
#define RME_RENDERING_CHUNK_COLLECTORS_H_

#include "rendering/core/sprite_sink.h"
#include "rendering/core/light_sink.h"
#include "rendering/core/sprite_instance.h"
#include "rendering/core/light_buffer.h"
#include <vector>
#include <glm/glm.hpp>
#include <unordered_set>

class SpriteCollector : public ISpriteSink {
public:
	SpriteCollector() = default;

	void clear() {
		sprites.clear();
		missing_sprites.clear();
		current_tint = glm::vec4(1.0f);
	}

	void draw(float x, float y, float w, float h, const AtlasRegion& region) override;
	void draw(float x, float y, float w, float h, const AtlasRegion& region, float r, float g, float b, float a) override;
	void drawRect(float x, float y, float w, float h, const glm::vec4& color, const AtlasRegion& white_pixel) override;
	void setGlobalTint(float r, float g, float b, float a) override;
	void reportMissingSprite(uint32_t id) override;

	const std::vector<SpriteInstance>& getSprites() const {
		return sprites;
	}

	std::vector<SpriteInstance> takeSprites() {
		missing_sprites.clear();
		current_tint = glm::vec4(1.0f);
		return std::move(sprites);
	}

	const std::unordered_set<uint32_t>& getMissingSprites() const {
		return missing_sprites;
	}

private:
	std::vector<SpriteInstance> sprites;
	std::unordered_set<uint32_t> missing_sprites;
	glm::vec4 current_tint = glm::vec4(1.0f);
};

class LightCollector : public ILightSink {
public:
	LightCollector() = default;

	void clear() {
		lights.clear();
	}

	void addLight(int map_x, int map_y, int map_z, const SpriteLight& light) override;

	const std::vector<LightBuffer::Light>& getLights() const {
		return lights;
	}

	std::vector<LightBuffer::Light> takeLights() {
		return std::move(lights);
	}

private:
	std::vector<LightBuffer::Light> lights;
};

#endif
