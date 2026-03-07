#include "rendering/core/sprite_database.h"
#include "rendering/core/sprite_preloader.h"
#include "ui/gui.h"

void SpriteDatabase::clear() {
  g_gui.atlas.getSpritePreloader().clear();
  metadata_space.clear();
  atlas_space.clear();
  icon_space.clear();
  normal_images_.clear();
  template_images_.clear();
  // editor_sprite_space is intentionally not cleared here, as editor sprites
  // persist across map versions.
  item_count = 0;
  creature_count = 0;
}

Sprite *SpriteDatabase::getEditorSprite(int id) {
  if (id < 0) {
    if (auto it = editor_sprite_space.find(id);
        it != editor_sprite_space.end()) {
      return it->second.get();
    }
  }
  return nullptr;
}

void SpriteDatabase::insertEditorSprite(int id,
                                        std::unique_ptr<Sprite> sprite) {
  if (id < 0) {
    editor_sprite_space[id] = std::move(sprite);
  }
}

const SpriteMetadata &SpriteDatabase::getMetadata(uint32_t id) const {
  if (id >= metadata_space.size()) {
    static const SpriteMetadata empty_metadata;
    return empty_metadata;
  }
  return metadata_space[id];
}

const SpriteAtlasCache &SpriteDatabase::getAtlasCache(uint32_t id) const {
  if (id >= atlas_space.size()) {
    static const SpriteAtlasCache fallback;
    return fallback;
  }
  return atlas_space[id];
}

const SpriteIconRenderer &SpriteDatabase::getIconRenderer(uint32_t id) const {
  if (id >= icon_space.size()) {
    static const SpriteIconRenderer fallback;
    return fallback;
  }
  return icon_space[id];
}

void SpriteDatabase::resizeSpriteSpaces(size_t new_size) {
  if (new_size > metadata_space.size()) {
    metadata_space.resize(new_size);
    atlas_space.resize(new_size);
    icon_space.resize(new_size);
  }
}

void SpriteDatabase::DrawItemSprite(int clientID, wxDC *dc, SpriteSize sz,
                                    int start_x, int start_y, int width,
                                    int height) {
  if (clientID < 0) {
    if (Sprite *editor_sprite = getEditorSprite(clientID)) {
      editor_sprite->DrawTo(dc, sz, start_x, start_y, width, height);
    }
  } else if (static_cast<size_t>(clientID) < metadata_space.size()) {
    icon_space[clientID].DrawTo(clientID, metadata_space[clientID], dc, sz,
                                start_x, start_y, width, height);
  }
}

void SpriteDatabase::DrawCreatureSprite(int lookType, const Outfit &outfit,
                                        wxDC *dc, SpriteSize sz, int start_x,
                                        int start_y, int width, int height) {
  size_t clientID = static_cast<size_t>(lookType) + item_count;
  if (clientID < metadata_space.size()) {
    icon_space[clientID].DrawTo(clientID, metadata_space[clientID], dc, sz,
                                outfit, start_x, start_y, width, height);
  }
}

// --- CreatureSprite implementation ---

CreatureSprite::CreatureSprite(uint32_t clientID, const Outfit &outfit)
    : clientID(clientID), outfit(outfit) {}

CreatureSprite::~CreatureSprite() = default;

void CreatureSprite::DrawTo(wxDC *dc, SpriteSize sz, int start_x, int start_y,
                            int width, int height) {
  g_gui.sprites.DrawCreatureSprite(clientID, outfit, dc, sz, start_x, start_y,
                                   width, height);
}

void CreatureSprite::unloadDC() {
  // No cached DC to unload — rendering is delegated to SpriteDatabase
}

Image* SpriteDatabase::resolveImage(ImageHandle handle) {
    if (handle.type == ImageType::Normal) {
        if (handle.index < normal_images_.size()) {
            NormalImage& img = normal_images_[handle.index];
            if (img.generation_id == handle.generation) {
                return &img;
            }
        }
    } else if (handle.type == ImageType::Template) {
        if (handle.index < template_images_.size()) {
            TemplateImage& img = template_images_[handle.index];
            if (img.generation_id == handle.generation) {
                return &img;
            }
        }
    }
    return nullptr;
}
