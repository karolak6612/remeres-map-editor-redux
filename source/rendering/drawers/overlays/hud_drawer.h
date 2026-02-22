#ifndef RME_HUD_DRAWER_H_
#define RME_HUD_DRAWER_H_

#include "app/main.h"
#include <nanovg.h>
#include <unordered_map>

class MapCanvas;
class Sprite;

class HUDDrawer {
public:
	HUDDrawer();
	~HUDDrawer();

	void Draw(NVGcontext* vg, MapCanvas* canvas);

private:
	int getSpriteImage(NVGcontext* vg, Sprite* sprite);
	void ClearCache(NVGcontext* vg);

	std::unordered_map<uint64_t, int> spriteCache;
	NVGcontext* lastContext = nullptr;
};

#endif
