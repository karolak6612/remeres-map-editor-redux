#ifndef RME_PREVIEW_DRAWER_H_
#define RME_PREVIEW_DRAWER_H_

#include "rendering/core/render_view.h"

struct DrawContext;
struct ViewSnapshot;
struct FloorViewParams;
class Editor;
class ItemDrawer;
class SpriteDrawer;
class CreatureDrawer;
class Brush;

class PreviewDrawer {
public:
    PreviewDrawer();
    ~PreviewDrawer();

    void draw(
        const DrawContext& ctx, const ViewSnapshot& snapshot, const FloorViewParams& floor_params, int map_z, Editor& editor,
        ItemDrawer* item_drawer, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer, Brush* current_brush
    );
};

#endif
