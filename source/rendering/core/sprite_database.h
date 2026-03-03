#ifndef RME_RENDERING_CORE_SPRITE_DATABASE_H_
#define RME_RENDERING_CORE_SPRITE_DATABASE_H_

#include "game/outfit.h"
#include "rendering/core/game_sprite.h"
#include "rendering/core/sprite_atlas_cache.h"
#include "rendering/core/sprite_metadata.h"
#include "rendering/ui/sprite_icon_renderer.h"
#include <memory>
#include <unordered_map>
#include <vector>

class SpriteDatabase {
public:
    SpriteDatabase() = default;
    ~SpriteDatabase() = default;

    // Non-copyable/movable to prevent accidental copies of the god object replacement
    SpriteDatabase(const SpriteDatabase&) = delete;
    SpriteDatabase& operator=(const SpriteDatabase&) = delete;
    SpriteDatabase(SpriteDatabase&&) = delete;
    SpriteDatabase& operator=(SpriteDatabase&&) = delete;

    void clear();

    Sprite* getEditorSprite(int id);
    void insertEditorSprite(int id, std::unique_ptr<Sprite> sprite);

    SpriteMetadata& getMetadata(uint32_t id);
    SpriteAtlasCache& getAtlasCache(uint32_t id);
    SpriteIconRenderer& getIconRenderer(uint32_t id);

    void DrawItemSprite(int clientID, wxDC* dc, SpriteSize sz, int start_x, int start_y, int width = -1, int height = -1);
    void DrawCreatureSprite(
        int lookType, const Outfit& outfit, wxDC* dc, SpriteSize sz, int start_x, int start_y, int width = -1, int height = -1
    );

    uint16_t getItemSpriteMaxID() const
    {
        return item_count;
    }
    uint16_t getCreatureSpriteMaxID() const
    {
        return creature_count;
    }

    void setItemSpriteMaxID(uint16_t item_count)
    {
        this->item_count = item_count;
    }
    void setCreatureSpriteMaxID(uint16_t creature_count)
    {
        this->creature_count = creature_count;
    }

    // Access for Loader/Preloader/GC
    std::vector<SpriteMetadata>& getMetadataSpace()
    {
        return metadata_space;
    }
    std::vector<SpriteAtlasCache>& getAtlasCacheSpace()
    {
        return atlas_space;
    }
    std::vector<SpriteIconRenderer>& getIconRendererSpace()
    {
        return icon_space;
    }

    std::unordered_map<int, std::unique_ptr<Sprite>>& getEditorSpriteSpace()
    {
        return editor_sprite_space;
    }
    std::vector<std::unique_ptr<Image>>& getImageSpace()
    {
        return image_space;
    }

    void resizeSpriteSpaces(size_t new_size);

private:
    std::vector<SpriteMetadata> metadata_space;
    std::vector<SpriteAtlasCache> atlas_space;
    std::vector<SpriteIconRenderer> icon_space;
    std::vector<std::unique_ptr<Image>> image_space;
    std::unordered_map<int, std::unique_ptr<Sprite>> editor_sprite_space;

    uint16_t item_count = 0;
    uint16_t creature_count = 0;
};

#endif
