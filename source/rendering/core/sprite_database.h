#ifndef RME_RENDERING_CORE_SPRITE_DATABASE_H_
#define RME_RENDERING_CORE_SPRITE_DATABASE_H_

#include "util/common.h"
#include <memory>
#include <unordered_map>
#include <vector>

#include "app/client_version.h"

class Sprite;
class GameSprite;
class Image;

class SpriteDatabase {
public:
  SpriteDatabase();
  ~SpriteDatabase();

  void clear();
  void cleanSoftwareSprites();

  bool loadEditorSprites();
  bool loadSpriteMetadata(const FileName &datafile, wxString &error,
                          std::vector<std::string> &warnings);
  bool loadSpriteData(const FileName &datafile, wxString &error,
                      std::vector<std::string> &warnings);

  Sprite *getSprite(int id);
  GameSprite *getCreatureSprite(int id);
  void insertSprite(int id, std::unique_ptr<Sprite> sprite);
  void insertSprite(int id, Sprite *sprite);
  bool loadSpriteDump(std::unique_ptr<uint8_t[]> &target, uint16_t &size,
                      int sprite_id);

  uint16_t getItemSpriteMaxID() const { return item_count; }
  uint16_t getCreatureSpriteMaxID() const { return creature_count; }

  bool isUnloaded() const { return unloaded; }
  void setUnloaded(bool st) { unloaded = st; }

  bool hasTransparency() const { return has_transparency; }
  void setHasTransparency(bool val) { has_transparency = val; }

  ClientVersion *client_version = nullptr;
  DatFormat dat_format = DAT_FORMAT_UNKNOWN;
  std::string spritefile;

  // Allow loaders to set these directly (or add setters)
  uint16_t item_count = 0;
  uint16_t creature_count = 0;
  bool is_extended = false;
  bool has_transparency = false;
  bool has_frame_durations = false;
  bool has_frame_groups = false;

  using SpriteVector = std::vector<std::unique_ptr<Sprite>>;
  SpriteVector sprite_space;

  using ImageVector = std::vector<std::unique_ptr<Image>>;
  ImageVector image_space;

  // Editor sprites use negative IDs, so they need a separate map
  std::unordered_map<int, std::unique_ptr<Sprite>> editor_sprite_space;

private:
  bool unloaded = true;
};

#endif
