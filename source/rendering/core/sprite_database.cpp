#include "rendering/core/sprite_database.h"
#include "app/main.h"
#include "game/sprites.h"
#include "rendering/core/game_sprite.h"
#include "rendering/core/image.h"
#include "rendering/io/editor_sprite_loader.h"
#include "rendering/io/game_sprite_loader.h"
#include "ui/gui.h"
#include <nanovg.h>
#include <nanovg_gl.h>

void NVGDeleter::operator()(NVGcontext *nvg) const {
  if (nvg) {
    nvgDeleteGL3(nvg);
  }
}

SpriteDatabase::SpriteDatabase() = default;
SpriteDatabase::~SpriteDatabase() = default;

void SpriteDatabase::clear() {
  sprite_space.clear();
  image_space.clear();
  editor_sprite_space.clear();

  item_count = 0;
  creature_count = 0;
  unloaded = true;
  is_extended = false;
  has_transparency = false;
  has_frame_durations = false;
  has_frame_groups = false;
  dat_format = DAT_FORMAT_UNKNOWN;
  spritefile = "";
}

void SpriteDatabase::cleanSoftwareSprites() {
  g_gui.texture_gc.collector.CleanSoftwareSprites(sprite_space);
}

bool SpriteDatabase::loadEditorSprites() {
  return EditorSpriteLoader::Load(this);
}

bool SpriteDatabase::loadSpriteMetadata(const FileName &datafile,
                                        wxString &error,
                                        std::vector<std::string> &warnings) {
  return GameSpriteLoader::LoadSpriteMetadata(this, datafile, error, warnings);
}

bool SpriteDatabase::loadSpriteData(const FileName &datafile, wxString &error,
                                    std::vector<std::string> &warnings) {
  return GameSpriteLoader::LoadSpriteData(this, datafile, error, warnings);
}

Sprite *SpriteDatabase::getSprite(int id) {
  if (id < 0) {
    if (auto it = editor_sprite_space.find(id);
        it != editor_sprite_space.end()) {
      return it->second.get();
    }
    return nullptr;
  }
  if (static_cast<size_t>(id) >= sprite_space.size()) {
    return nullptr;
  }
  return sprite_space[id].get();
}

void SpriteDatabase::insertSprite(int id, std::unique_ptr<Sprite> sprite) {
  if (id < 0) {
    editor_sprite_space[id] = std::move(sprite);
  } else {
    if (static_cast<size_t>(id) >= sprite_space.size()) {
      sprite_space.resize(id + 1);
    }
    sprite_space[id] = std::move(sprite);
  }
}

#include "rendering/io/game_sprite_loader.h"

void SpriteDatabase::insertSprite(int id, Sprite *sprite) {
  insertSprite(id, std::unique_ptr<Sprite>(sprite));
}

bool SpriteDatabase::loadSpriteDump(std::unique_ptr<uint8_t[]> &target,
                                    uint16_t &size, int sprite_id) {
  return GameSpriteLoader::LoadSpriteDump(this, target, size, sprite_id);
}

GameSprite *SpriteDatabase::getCreatureSprite(int id) {
  if (id < 0) {
    return nullptr;
  }

  size_t target_id = static_cast<size_t>(id) + item_count;
  if (target_id >= sprite_space.size()) {
    return nullptr;
  }
  return static_cast<GameSprite *>(sprite_space[target_id].get());
}
