#include "rendering/io/editor_sprite_loader.h"
#include "app/definitions.h"
#include "game/sprites.h"
#include "rendering/core/editor_sprite.h"
#include "rendering/core/sprite_database.h"
#include "rendering/io/sprite_loader.h"
#include "util/image_manager.h"
#include <memory>
#include <string_view>

namespace {

    auto makeEditorSprite(std::string_view pathSmall, std::string_view pathLarge)
    {
        return std::make_unique<EditorSprite>(
            std::make_unique<wxBitmap>(IMAGE_MANAGER.GetBitmap(pathSmall)), std::make_unique<wxBitmap>(IMAGE_MANAGER.GetBitmap(pathLarge))
        );
    }

    auto makeSingleEditorSprite(std::string_view path)
    {
        return std::make_unique<EditorSprite>(std::make_unique<wxBitmap>(IMAGE_MANAGER.GetBitmap(path)), nullptr);
    }

} // namespace

bool EditorSpriteLoader::Load([[maybe_unused]] SpriteLoader* loader, SpriteDatabase& db)
{
    // Unused graphics MIGHT be loaded here, but it's a neglectable loss
    db.insertEditorSprite(
        EDITOR_SPRITE_SELECTION_MARKER,
        std::make_unique<EditorSprite>(
            std::make_unique<wxBitmap>(selection_marker_xpm16x16), std::make_unique<wxBitmap>(selection_marker_xpm32x32)
        )
    );
    db.insertEditorSprite(EDITOR_SPRITE_BRUSH_CD_1x1, makeEditorSprite(IMAGE_CIRCULAR_1_SMALL, IMAGE_CIRCULAR_1));
    db.insertEditorSprite(EDITOR_SPRITE_BRUSH_CD_3x3, makeEditorSprite(IMAGE_CIRCULAR_2_SMALL, IMAGE_CIRCULAR_2));
    db.insertEditorSprite(EDITOR_SPRITE_BRUSH_CD_5x5, makeEditorSprite(IMAGE_CIRCULAR_3_SMALL, IMAGE_CIRCULAR_3));
    db.insertEditorSprite(EDITOR_SPRITE_BRUSH_CD_7x7, makeEditorSprite(IMAGE_CIRCULAR_4_SMALL, IMAGE_CIRCULAR_4));
    db.insertEditorSprite(EDITOR_SPRITE_BRUSH_CD_9x9, makeEditorSprite(IMAGE_CIRCULAR_5_SMALL, IMAGE_CIRCULAR_5));
    db.insertEditorSprite(EDITOR_SPRITE_BRUSH_CD_15x15, makeEditorSprite(IMAGE_CIRCULAR_6_SMALL, IMAGE_CIRCULAR_6));
    db.insertEditorSprite(EDITOR_SPRITE_BRUSH_CD_19x19, makeEditorSprite(IMAGE_CIRCULAR_7_SMALL, IMAGE_CIRCULAR_7));
    db.insertEditorSprite(EDITOR_SPRITE_BRUSH_SD_1x1, makeEditorSprite(IMAGE_RECTANGULAR_1_SMALL, IMAGE_RECTANGULAR_1));
    db.insertEditorSprite(EDITOR_SPRITE_BRUSH_SD_3x3, makeEditorSprite(IMAGE_RECTANGULAR_2_SMALL, IMAGE_RECTANGULAR_2));
    db.insertEditorSprite(EDITOR_SPRITE_BRUSH_SD_5x5, makeEditorSprite(IMAGE_RECTANGULAR_3_SMALL, IMAGE_RECTANGULAR_3));
    db.insertEditorSprite(EDITOR_SPRITE_BRUSH_SD_7x7, makeEditorSprite(IMAGE_RECTANGULAR_4_SMALL, IMAGE_RECTANGULAR_4));
    db.insertEditorSprite(EDITOR_SPRITE_BRUSH_SD_9x9, makeEditorSprite(IMAGE_RECTANGULAR_5_SMALL, IMAGE_RECTANGULAR_5));
    db.insertEditorSprite(EDITOR_SPRITE_BRUSH_SD_15x15, makeEditorSprite(IMAGE_RECTANGULAR_6_SMALL, IMAGE_RECTANGULAR_6));
    db.insertEditorSprite(EDITOR_SPRITE_BRUSH_SD_19x19, makeEditorSprite(IMAGE_RECTANGULAR_7_SMALL, IMAGE_RECTANGULAR_7));

    db.insertEditorSprite(EDITOR_SPRITE_OPTIONAL_BORDER_TOOL, makeEditorSprite(IMAGE_OPTIONAL_BORDER_SMALL, IMAGE_OPTIONAL_BORDER));
    db.insertEditorSprite(EDITOR_SPRITE_ERASER, makeEditorSprite(IMAGE_ERASER_SMALL, IMAGE_ERASER));
    db.insertEditorSprite(EDITOR_SPRITE_PZ_TOOL, makeEditorSprite(IMAGE_PROTECTION_ZONE_SMALL, IMAGE_PROTECTION_ZONE));
    db.insertEditorSprite(EDITOR_SPRITE_PVPZ_TOOL, makeEditorSprite(IMAGE_PVP_ZONE_SMALL, IMAGE_PVP_ZONE));
    db.insertEditorSprite(EDITOR_SPRITE_NOLOG_TOOL, makeEditorSprite(IMAGE_NO_LOGOUT_ZONE_SMALL, IMAGE_NO_LOGOUT_ZONE));
    db.insertEditorSprite(EDITOR_SPRITE_NOPVP_TOOL, makeEditorSprite(IMAGE_NO_PVP_ZONE_SMALL, IMAGE_NO_PVP_ZONE));

    db.insertEditorSprite(EDITOR_SPRITE_DOOR_NORMAL, makeEditorSprite(IMAGE_DOOR_NORMAL_SMALL, IMAGE_DOOR_NORMAL));
    db.insertEditorSprite(EDITOR_SPRITE_DOOR_LOCKED, makeEditorSprite(IMAGE_DOOR_LOCKED_SMALL, IMAGE_DOOR_LOCKED));
    db.insertEditorSprite(EDITOR_SPRITE_DOOR_MAGIC, makeEditorSprite(IMAGE_DOOR_MAGIC_SMALL, IMAGE_DOOR_MAGIC));
    db.insertEditorSprite(EDITOR_SPRITE_DOOR_QUEST, makeEditorSprite(IMAGE_DOOR_QUEST_SMALL, IMAGE_DOOR_QUEST));
    db.insertEditorSprite(EDITOR_SPRITE_DOOR_NORMAL_ALT, makeEditorSprite(IMAGE_DOOR_NORMAL_ALT_SMALL, IMAGE_DOOR_NORMAL_ALT));
    db.insertEditorSprite(EDITOR_SPRITE_DOOR_ARCHWAY, makeEditorSprite(IMAGE_DOOR_ARCHWAY_SMALL, IMAGE_DOOR_ARCHWAY));
    db.insertEditorSprite(EDITOR_SPRITE_WINDOW_NORMAL, makeEditorSprite(IMAGE_WINDOW_NORMAL_SMALL, IMAGE_WINDOW_NORMAL));
    db.insertEditorSprite(EDITOR_SPRITE_WINDOW_HATCH, makeEditorSprite(IMAGE_WINDOW_HATCH_SMALL, IMAGE_WINDOW_HATCH));

    db.insertEditorSprite(EDITOR_SPRITE_SELECTION_GEM, makeSingleEditorSprite(IMAGE_GEM_EDIT));
    db.insertEditorSprite(EDITOR_SPRITE_DRAWING_GEM, makeSingleEditorSprite(IMAGE_GEM_MOVE));

    return true;
}
