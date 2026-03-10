#ifndef RME_RENDERING_CORE_DRAW_CONTEXT_H_
#define RME_RENDERING_CORE_DRAW_CONTEXT_H_

class SpriteBatch;
class PrimitiveRenderer;
class AtlasManager;
struct ViewState;
struct RenderSettings;
struct FrameOptions;
struct LightBuffer;
struct FrameAccumulators;

// Bundles all per-frame drawing dependencies into a single struct,
// reducing parameter bloat across drawer call sites.
// Passed as const DrawContext& — internal non-const references
// (sprite_batch, primitive_renderer, light_buffer, accumulators)
// remain mutable since drawers need to write to them.
struct DrawContext {
    SpriteBatch& sprite_batch;
    PrimitiveRenderer& primitive_renderer;
    const ViewState& view;
    const RenderSettings& settings;
    const FrameOptions& frame;
    LightBuffer& light_buffer;
    FrameAccumulators& accumulators;
    const AtlasManager& atlas;
};

#endif
