#include "app/definitions.h"
#include "rendering/io/editor_sprite_loader.h"
#include "rendering/core/sprite_database.h"
#include "rendering/io/sprite_loader.h"
#include "rendering/core/editor_sprite.h"
#include "game/sprites.h"
#include "util/image_manager.h"
#include <memory>

// Helper logic moved to wx_utils.h
// Helper to wrap ImageManager returns for EditorSprite
#define getEditorSprite(pathSmall, pathLarge) std::make_unique<EditorSprite>( \
	std::make_unique<wxBitmap>(IMAGE_MANAGER.GetBitmap(pathSmall)),           \
	std::make_unique<wxBitmap>(IMAGE_MANAGER.GetBitmap(pathLarge))            \
)

#define getSingleEditorSprite(path) std::make_unique<EditorSprite>( \
	std::make_unique<wxBitmap>(IMAGE_MANAGER.GetBitmap(path)),      \
	nullptr                                                         \
)

bool EditorSpriteLoader::Load([[maybe_unused]] SpriteLoader* loader, SpriteDatabase& db) {
	// Unused graphics MIGHT be loaded here, but it's a neglectable loss
	db.insertSprite(EDITOR_SPRITE_SELECTION_MARKER, std::make_unique<EditorSprite>(std::make_unique<wxBitmap>(selection_marker_xpm16x16), std::make_unique<wxBitmap>(selection_marker_xpm32x32)));
	db.insertSprite(EDITOR_SPRITE_BRUSH_CD_1x1, getEditorSprite(IMAGE_CIRCULAR_1_SMALL, IMAGE_CIRCULAR_1));
	db.insertSprite(EDITOR_SPRITE_BRUSH_CD_3x3, getEditorSprite(IMAGE_CIRCULAR_2_SMALL, IMAGE_CIRCULAR_2));
	db.insertSprite(EDITOR_SPRITE_BRUSH_CD_5x5, getEditorSprite(IMAGE_CIRCULAR_3_SMALL, IMAGE_CIRCULAR_3));
	db.insertSprite(EDITOR_SPRITE_BRUSH_CD_7x7, getEditorSprite(IMAGE_CIRCULAR_4_SMALL, IMAGE_CIRCULAR_4));
	db.insertSprite(EDITOR_SPRITE_BRUSH_CD_9x9, getEditorSprite(IMAGE_CIRCULAR_5_SMALL, IMAGE_CIRCULAR_5));
	db.insertSprite(EDITOR_SPRITE_BRUSH_CD_15x15, getEditorSprite(IMAGE_CIRCULAR_6_SMALL, IMAGE_CIRCULAR_6));
	db.insertSprite(EDITOR_SPRITE_BRUSH_CD_19x19, getEditorSprite(IMAGE_CIRCULAR_7_SMALL, IMAGE_CIRCULAR_7));
	db.insertSprite(EDITOR_SPRITE_BRUSH_SD_1x1, getEditorSprite(IMAGE_RECTANGULAR_1_SMALL, IMAGE_RECTANGULAR_1));
	db.insertSprite(EDITOR_SPRITE_BRUSH_SD_3x3, getEditorSprite(IMAGE_RECTANGULAR_2_SMALL, IMAGE_RECTANGULAR_2));
	db.insertSprite(EDITOR_SPRITE_BRUSH_SD_5x5, getEditorSprite(IMAGE_RECTANGULAR_3_SMALL, IMAGE_RECTANGULAR_3));
	db.insertSprite(EDITOR_SPRITE_BRUSH_SD_7x7, getEditorSprite(IMAGE_RECTANGULAR_4_SMALL, IMAGE_RECTANGULAR_4));
	db.insertSprite(EDITOR_SPRITE_BRUSH_SD_9x9, getEditorSprite(IMAGE_RECTANGULAR_5_SMALL, IMAGE_RECTANGULAR_5));
	db.insertSprite(EDITOR_SPRITE_BRUSH_SD_15x15, getEditorSprite(IMAGE_RECTANGULAR_6_SMALL, IMAGE_RECTANGULAR_6));
	db.insertSprite(EDITOR_SPRITE_BRUSH_SD_19x19, getEditorSprite(IMAGE_RECTANGULAR_7_SMALL, IMAGE_RECTANGULAR_7));

	db.insertSprite(EDITOR_SPRITE_OPTIONAL_BORDER_TOOL, getEditorSprite(IMAGE_OPTIONAL_BORDER_SMALL, IMAGE_OPTIONAL_BORDER));
	db.insertSprite(EDITOR_SPRITE_ERASER, getEditorSprite(IMAGE_ERASER_SMALL, IMAGE_ERASER));
	db.insertSprite(EDITOR_SPRITE_PZ_TOOL, getEditorSprite(IMAGE_PROTECTION_ZONE_SMALL, IMAGE_PROTECTION_ZONE));
	db.insertSprite(EDITOR_SPRITE_PVPZ_TOOL, getEditorSprite(IMAGE_PVP_ZONE_SMALL, IMAGE_PVP_ZONE));
	db.insertSprite(EDITOR_SPRITE_NOLOG_TOOL, getEditorSprite(IMAGE_NO_LOGOUT_ZONE_SMALL, IMAGE_NO_LOGOUT_ZONE));
	db.insertSprite(EDITOR_SPRITE_NOPVP_TOOL, getEditorSprite(IMAGE_NO_PVP_ZONE_SMALL, IMAGE_NO_PVP_ZONE));

	db.insertSprite(EDITOR_SPRITE_DOOR_NORMAL, getEditorSprite(IMAGE_DOOR_NORMAL_SMALL, IMAGE_DOOR_NORMAL));
	db.insertSprite(EDITOR_SPRITE_DOOR_LOCKED, getEditorSprite(IMAGE_DOOR_LOCKED_SMALL, IMAGE_DOOR_LOCKED));
	db.insertSprite(EDITOR_SPRITE_DOOR_MAGIC, getEditorSprite(IMAGE_DOOR_MAGIC_SMALL, IMAGE_DOOR_MAGIC));
	db.insertSprite(EDITOR_SPRITE_DOOR_QUEST, getEditorSprite(IMAGE_DOOR_QUEST_SMALL, IMAGE_DOOR_QUEST));
	db.insertSprite(EDITOR_SPRITE_DOOR_NORMAL_ALT, getEditorSprite(IMAGE_DOOR_NORMAL_ALT_SMALL, IMAGE_DOOR_NORMAL_ALT));
	db.insertSprite(EDITOR_SPRITE_DOOR_ARCHWAY, getEditorSprite(IMAGE_DOOR_ARCHWAY_SMALL, IMAGE_DOOR_ARCHWAY));
	db.insertSprite(EDITOR_SPRITE_WINDOW_NORMAL, getEditorSprite(IMAGE_WINDOW_NORMAL_SMALL, IMAGE_WINDOW_NORMAL));
	db.insertSprite(EDITOR_SPRITE_WINDOW_HATCH, getEditorSprite(IMAGE_WINDOW_HATCH_SMALL, IMAGE_WINDOW_HATCH));

	db.insertSprite(EDITOR_SPRITE_SELECTION_GEM, getSingleEditorSprite(IMAGE_GEM_EDIT));
	db.insertSprite(EDITOR_SPRITE_DRAWING_GEM, getSingleEditorSprite(IMAGE_GEM_MOVE));

	return true;
}
