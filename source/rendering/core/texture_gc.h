#ifndef RME_RENDERING_CORE_TEXTURE_GC_H_
#define RME_RENDERING_CORE_TEXTURE_GC_H_

#include <vector>
#include <deque>
#include <memory>
#include <time.h>
#include "rendering/core/render_timer.h"

class GameSprite;
class Sprite;
class SpriteDatabase;

class TextureGC {
public:
	TextureGC();
	~TextureGC();

	TextureGC(const TextureGC&) = delete;
	TextureGC& operator=(const TextureGC&) = delete;

	void clear();
	void updateTime();
	
	long getElapsedTime() const { return animation_timer->getElapsedTime(); }
	time_t getCachedTime() const { return cached_time_; }

	void pauseAnimation() { animation_timer->Pause(); }
	void resumeAnimation() { animation_timer->Resume(); }

	void garbageCollection(SpriteDatabase& db);
	void addSpriteToCleanup(GameSprite* spr);
	
	void notifyTextureLoaded();
	void notifyTextureUnloaded();
	int getLoadedTexturesCount() const { return loaded_textures; }

	// Allow preloader/loaders to track resident sets
	std::vector<void*>& getResidentImages() { return resident_images; }
	std::vector<GameSprite*>& getResidentGameSprites() { return resident_game_sprites; }

private:
	std::unique_ptr<RenderTimer> animation_timer;
	time_t cached_time_ = 0;

	std::vector<void*> resident_images;
	std::vector<GameSprite*> resident_game_sprites;

	int loaded_textures = 0;
	time_t lastclean = 0;
	std::deque<GameSprite*> cleanup_list;
};

#endif
