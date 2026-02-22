#ifndef RME_RENDERING_CORE_SPRITE_SINK_H_
#define RME_RENDERING_CORE_SPRITE_SINK_H_

#include <glm/glm.hpp>

struct AtlasRegion;
class AtlasManager;

class ISpriteSink {
public:
	virtual ~ISpriteSink() = default;

	virtual void draw(float x, float y, float w, float h, const AtlasRegion& region) = 0;
	virtual void draw(float x, float y, float w, float h, const AtlasRegion& region, float r, float g, float b, float a) = 0;
	// Use AtlasRegion for white pixel instead of AtlasManager to allow thread-safe cached access
	virtual void drawRect(float x, float y, float w, float h, const glm::vec4& color, const AtlasRegion& white_pixel) = 0;
	// Removed AtlasManager from setGlobalTint as it's implementation detail of SpriteBatch
	virtual void setGlobalTint(float r, float g, float b, float a) = 0;
	virtual void reportMissingSprite(uint32_t id) {}
};

#endif
