#ifndef RME_RENDERING_CORE_RENDER_CONTEXT_H_
#define RME_RENDERING_CORE_RENDER_CONTEXT_H_

class GraphicManager;

// Explicit dependency injection for graphics services.
// Constructed once in MapCanvas, passed to MapDrawer.
struct RenderContext {
	GraphicManager& gfx;
};

#endif
