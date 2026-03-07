#ifndef RME_RENDERING_CORE_DRAW_CONTEXT_H_
#define RME_RENDERING_CORE_DRAW_CONTEXT_H_

class SpriteBatch;
class PrimitiveRenderer;
struct ViewState;
struct DrawingOptions;
struct LightBuffer;

struct CanvasState;
class BrushCursorDrawer;

struct ContextState {
  const ViewState &view;
  const DrawingOptions &options;
  const CanvasState &canvas_state;
  bool is_preload_pass = false;
};

struct RenderBackend {
  SpriteBatch &sprite_batch;
  PrimitiveRenderer &primitive_renderer;
};

struct FrameOutput {
  LightBuffer &light_buffer;
  BrushCursorDrawer *brush_cursor_drawer = nullptr;
};

struct DrawContext {
  const ContextState state;
  const RenderBackend backend;
  const FrameOutput output;
};

#endif
