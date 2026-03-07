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
class SpriteDatabase;
class AtlasManager;
class TextureGC;
class SpriteLoader;

struct RenderBackend {
  SpriteBatch &sprite_batch;
  PrimitiveRenderer &primitive_renderer;
  SpriteDatabase &sprite_database;
  AtlasManager &atlas_manager;
  TextureGC &texture_gc;
  SpriteLoader &sprite_loader;
  bool use_memcached;
};

struct FrameOutput {
  LightBuffer &light_buffer;
  BrushCursorDrawer *brush_cursor_drawer = nullptr;
};

struct OverlayState {
  struct BrushSettings {
    class Brush* current_brush = nullptr;
    int size = 0;
    int shape = 0;
    bool is_drawing_mode = false;
  } brush;
};

struct DrawContext {
  const ContextState state;
  const OverlayState overlays;
  const RenderBackend backend;
  const FrameOutput output;
};

#endif
