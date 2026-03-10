#ifndef RME_RENDERING_CORE_DRAW_CONTEXT_H_
#define RME_RENDERING_CORE_DRAW_CONTEXT_H_

class SpriteBatch;
class PrimitiveRenderer;
struct ViewState;
struct DrawingOptions;
struct LightBuffer;

// Bundles all per-frame drawing dependencies into a single struct,
// reducing parameter bloat across drawer call sites.
// Passed as const DrawContext& — internal non-const references
// (sprite_batch, primitive_renderer, light_buffer) remain mutable
// since drawers need to write to them.
struct DrawContext {
	SpriteBatch& sprite_batch;
	PrimitiveRenderer& primitive_renderer;
	const ViewState& view;
	const DrawingOptions& options;
	LightBuffer& light_buffer;
};

#endif
