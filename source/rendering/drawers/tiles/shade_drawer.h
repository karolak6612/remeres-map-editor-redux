#ifndef RME_SHADE_DRAWER_H_
#define RME_SHADE_DRAWER_H_

#include "rendering/core/render_view.h"
#include "rendering/core/drawing_options.h"

class ISpriteSink;

class ShadeDrawer {
public:
	ShadeDrawer();
	~ShadeDrawer();

	void draw(ISpriteSink& sprite_sink, const RenderView& view, const DrawingOptions& options);
};

#endif
