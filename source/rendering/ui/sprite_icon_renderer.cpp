//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "rendering/ui/sprite_icon_renderer.h"
#include "rendering/core/game_sprite.h"
#include "rendering/core/texture_gc.h"
#include "rendering/utilities/sprite_icon_generator.h"
#include "ui/gui.h"

void SpriteIconRenderer::unloadDC()
{
    dc[SPRITE_SIZE_16x16].reset();
    dc[SPRITE_SIZE_32x32].reset();
    dc[SPRITE_SIZE_64x64].reset();
    bm[SPRITE_SIZE_16x16].reset();
    bm[SPRITE_SIZE_32x32].reset();
    bm[SPRITE_SIZE_64x64].reset();
    colored_dc.clear();
}

void SpriteIconRenderer::unloadOutfitDC(const Outfit& outfit)
{
    RenderKey key;
    key.colorHash = outfit.getColorHash();
    key.mountColorHash = outfit.getMountColorHash();
    key.lookMount = outfit.lookMount;
    key.lookAddon = outfit.lookAddon;
    key.lookMountHead = outfit.lookMountHead;
    key.lookMountBody = outfit.lookMountBody;
    key.lookMountLegs = outfit.lookMountLegs;
    key.lookMountFeet = outfit.lookMountFeet;

    key.size = SPRITE_SIZE_16x16;
    colored_dc.erase(key);

    key.size = SPRITE_SIZE_32x32;
    colored_dc.erase(key);
}

wxMemoryDC* SpriteIconRenderer::getDC(uint32_t clientID, const SpriteMetadata& metadata, SpriteSize size)
{
    if (!dc[size]) {
        wxBitmap bmp = SpriteIconGenerator::Generate(clientID, size);
        if (bmp.IsOk()) {
            bm[size] = std::make_unique<wxBitmap>(bmp);
            dc[size] = std::make_unique<wxMemoryDC>(*bm[size]);
        }
        if (metadata.id != 0) {
            g_gui.gc.addSpriteToCleanup(metadata.id);
        }
    }
    return dc[size].get();
}

wxMemoryDC* SpriteIconRenderer::getDC(uint32_t clientID, const SpriteMetadata& metadata, SpriteSize size, const Outfit& outfit)
{
    RenderKey key;
    key.size = size;
    key.colorHash = outfit.getColorHash();
    key.mountColorHash = outfit.getMountColorHash();
    key.lookMount = outfit.lookMount;
    key.lookAddon = outfit.lookAddon;
    key.lookMountHead = outfit.lookMountHead;
    key.lookMountBody = outfit.lookMountBody;
    key.lookMountLegs = outfit.lookMountLegs;
    key.lookMountFeet = outfit.lookMountFeet;

    auto it = colored_dc.find(key);
    if (it == colored_dc.end()) {
        wxBitmap bmp = SpriteIconGenerator::Generate(clientID, size, outfit);
        if (bmp.IsOk()) {
            auto cache = std::make_unique<CachedDC>();
            cache->bm = std::make_unique<wxBitmap>(bmp);
            cache->dc = std::make_unique<wxMemoryDC>(*cache->bm);

            auto res = colored_dc.insert(std::make_pair(key, std::move(cache)));
            if (metadata.id != 0) {
                g_gui.gc.addSpriteToCleanup(metadata.id);
            }
            return res.first->second->dc.get();
        }
        return nullptr;
    }
    return it->second->dc.get();
}

void SpriteIconRenderer::DrawTo(
    uint32_t clientID, const SpriteMetadata& metadata, wxDC* target_dc, SpriteSize sz, int start_x, int start_y, int width, int height
)
{
    const int sprite_dim = (sz == SPRITE_SIZE_64x64) ? 64 : (sz == SPRITE_SIZE_32x32 ? 32 : 16);
    int src_width = sprite_dim;
    int src_height = sprite_dim;

    if (width == -1) {
        width = src_width;
    }
    if (height == -1) {
        height = src_height;
    }
    wxDC* sdc = getDC(clientID, metadata, sz);
    if (sdc) {
        target_dc->StretchBlit(start_x, start_y, width, height, sdc, 0, 0, src_width, src_height, wxCOPY, true);
    } else {
        const wxBrush& b = target_dc->GetBrush();
        target_dc->SetBrush(*wxRED_BRUSH);
        target_dc->DrawRectangle(start_x, start_y, width, height);
        target_dc->SetBrush(b);
    }
}

void SpriteIconRenderer::DrawTo(
    uint32_t clientID, const SpriteMetadata& metadata, wxDC* target_dc, SpriteSize sz, const Outfit& outfit, int start_x, int start_y,
    int width, int height
)
{
    const int sprite_dim = (sz == SPRITE_SIZE_64x64) ? 64 : (sz == SPRITE_SIZE_32x32 ? 32 : 16);
    int src_width = sprite_dim;
    int src_height = sprite_dim;

    if (width == -1) {
        width = src_width;
    }
    if (height == -1) {
        height = src_height;
    }
    wxDC* sdc = getDC(clientID, metadata, sz, outfit);
    if (sdc) {
        target_dc->StretchBlit(start_x, start_y, width, height, sdc, 0, 0, src_width, src_height, wxCOPY, true);
    } else {
        const wxBrush& b = target_dc->GetBrush();
        target_dc->SetBrush(*wxRED_BRUSH);
        target_dc->DrawRectangle(start_x, start_y, width, height);
        target_dc->SetBrush(b);
    }
}
