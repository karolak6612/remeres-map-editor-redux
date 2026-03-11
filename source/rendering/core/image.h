#ifndef RME_RENDERING_CORE_IMAGE_H_
#define RME_RENDERING_CORE_IMAGE_H_

#include <atomic>
#include <memory>
#include <cstdint>
#include "rendering/core/atlas_manager.h" // For AtlasRegion

class GraphicManager;

class Image {
public:
	Image();
	virtual ~Image() = default;

	bool isGLLoaded = false;
	mutable std::atomic<int64_t> lastaccess;
	uint32_t generation_id = 0;

	void visit() const;
	virtual void clean(time_t time, int longevity);

	virtual std::unique_ptr<uint8_t[]> getRGBData() = 0;
	virtual std::unique_ptr<uint8_t[]> getRGBAData() = 0;

	virtual bool isNormalImage() const {
		return false;
	}

	void setGraphicManager(GraphicManager* graphics) {
		graphics_ = graphics;
	}

protected:
	[[nodiscard]] GraphicManager& graphics() const;
	// Helper to handle atlas interactions
	const AtlasRegion* EnsureAtlasSprite(uint32_t sprite_id, std::unique_ptr<uint8_t[]> preloaded_data = nullptr);

private:
	GraphicManager* graphics_ = nullptr;
};

#endif
