//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_UI_SPRITE_ICON_RENDERER_H_
#define RME_RENDERING_UI_SPRITE_ICON_RENDERER_H_

#include "game/outfit.h"
#include "rendering/core/sprite_metadata.h"
#include "rendering/ui/sprite_size.h"
#include <wx/bitmap.h>
#include <wx/dc.h>
#include <wx/dcmemory.h>
#include <memory>
#include <unordered_map>

class SpriteIconRenderer {
public:
    SpriteIconRenderer() = default;
    ~SpriteIconRenderer()
    {
        unloadDC();
    }

    void clean(time_t time, int longevity)
    {
        unloadDC();
    }

    // Non-copyable
    SpriteIconRenderer(const SpriteIconRenderer&) = delete;
    SpriteIconRenderer& operator=(const SpriteIconRenderer&) = delete;
    SpriteIconRenderer(SpriteIconRenderer&&) = default;
    SpriteIconRenderer& operator=(SpriteIconRenderer&&) = default;

    void DrawTo(
        uint32_t clientID, const SpriteMetadata& metadata, wxDC* dc, SpriteSize sz, int start_x, int start_y, int width = -1,
        int height = -1
    );
    void DrawTo(
        uint32_t clientID, const SpriteMetadata& metadata, wxDC* dc, SpriteSize sz, const Outfit& outfit, int start_x, int start_y,
        int width = -1, int height = -1
    );

    void unloadDC();
    void unloadOutfitDC(const Outfit& outfit);

protected:
    wxMemoryDC* getDC(uint32_t clientID, const SpriteMetadata& metadata, SpriteSize size);
    wxMemoryDC* getDC(uint32_t clientID, const SpriteMetadata& metadata, SpriteSize size, const Outfit& outfit);

    std::unique_ptr<wxMemoryDC> dc[SPRITE_SIZE_COUNT];
    std::unique_ptr<wxBitmap> bm[SPRITE_SIZE_COUNT];

    struct CachedDC {
        std::unique_ptr<wxMemoryDC> dc;
        std::unique_ptr<wxBitmap> bm;
    };

    struct RenderKey {
        SpriteSize size;
        uint32_t colorHash;
        uint32_t mountColorHash;
        int lookMount, lookAddon, lookMountHead, lookMountBody, lookMountLegs, lookMountFeet;

        bool operator==(const RenderKey& rk) const
        {
            return size == rk.size && colorHash == rk.colorHash && mountColorHash == rk.mountColorHash && lookMount == rk.lookMount
                && lookAddon == rk.lookAddon && lookMountHead == rk.lookMountHead && lookMountBody == rk.lookMountBody
                && lookMountLegs == rk.lookMountLegs && lookMountFeet == rk.lookMountFeet;
        }
    };

    struct RenderKeyHash {
        size_t operator()(const RenderKey& k) const noexcept
        {
            size_t h = std::hash<uint64_t> {}((uint64_t(k.colorHash) << 32) | k.mountColorHash);
            h ^= std::hash<uint64_t> {}((uint64_t(k.lookMount) << 32) | k.lookAddon) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<uint64_t> {}((uint64_t(k.lookMountHead) << 32) | k.lookMountBody) + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
    };
    std::unordered_map<RenderKey, std::unique_ptr<CachedDC>, RenderKeyHash> colored_dc;
};

#endif // RME_RENDERING_UI_SPRITE_ICON_RENDERER_H_
