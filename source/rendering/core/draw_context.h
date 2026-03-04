#ifndef RME_RENDERING_CORE_DRAW_CONTEXT_H_
#define RME_RENDERING_CORE_DRAW_CONTEXT_H_

class SpriteBatch;
class PrimitiveRenderer;
struct ViewState;
struct DrawingOptions;
struct LightBuffer;

struct CanvasState;
class BrushCursorDrawer;

struct DrawContext {
  SpriteBatch &sprite_batch;
  PrimitiveRenderer &primitive_renderer;
  const ViewState &view;
  const DrawingOptions &options;
  LightBuffer &light_buffer;
  const CanvasState &canvas_state;
  BrushCursorDrawer *brush_cursor_drawer = nullptr;
};

#endif
