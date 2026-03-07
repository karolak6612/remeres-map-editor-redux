#include "rendering/drawers/entities/sprite_drawer.h"
#include "game/items.h"
#include "game/sprites.h"
#include "rendering/core/atlas_lifecycle.h"
#include "rendering/core/sprite_database.h"
#include "rendering/core/texture_gc.h"
#include "rendering/io/sprite_loader.h"

#include "rendering/core/atlas_manager.h"
#include "rendering/core/sprite_batch.h"

#include <spdlog/spdlog.h>

SpriteDrawer::SpriteDrawer() { }

SpriteDrawer::~SpriteDrawer() { }

void SpriteDrawer::glBlitAtlasQuad(const DrawContext& ctx, int sx, int sy, const AtlasRegion* region, DrawColor color)
{
    if (ctx.state.is_preload_pass) return;

    if (region) {
        float normalizedR = color.r / 255.0f;
        float normalizedG = color.g / 255.0f;
        float normalizedB = color.b / 255.0f;
        float normalizedA = color.a / 255.0f;

        ctx.backend.sprite_batch.draw(
            static_cast<float>(sx), static_cast<float>(sy), static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE), *region,
            normalizedR, normalizedG, normalizedB, normalizedA
        );
    }
}

void SpriteDrawer::glBlitSquare(const DrawContext& ctx, int sx, int sy, DrawColor color, int size)
{
    if (ctx.state.is_preload_pass) return;

    if (size == 0) {
        size = TILE_SIZE;
    }

    float normalizedR = color.r / 255.0f;
    float normalizedG = color.g / 255.0f;
    float normalizedB = color.b / 255.0f;
    float normalizedA = color.a / 255.0f;

    ctx.backend.sprite_batch.drawRect(
        static_cast<float>(sx), static_cast<float>(sy), static_cast<float>(size), static_cast<float>(size),
        glm::vec4(normalizedR, normalizedG, normalizedB, normalizedA), ctx.backend.atlas_manager
    );
}

void SpriteDrawer::glDrawBox(const DrawContext& ctx, int sx, int sy, int width, int height, DrawColor color)
{
    if (ctx.state.is_preload_pass) return;

    float normalizedR = color.r / 255.0f;
    float normalizedG = color.g / 255.0f;
    float normalizedB = color.b / 255.0f;
    float normalizedA = color.a / 255.0f;

    ctx.backend.sprite_batch.drawRectLines(
        static_cast<float>(sx), static_cast<float>(sy), static_cast<float>(width), static_cast<float>(height),
        glm::vec4(normalizedR, normalizedG, normalizedB, normalizedA), ctx.backend.atlas_manager
    );
}

void SpriteDrawer::glSetColor(wxColor color)
{
    // Not needed with BatchRenderer automatic color handling in DrawQuad,
    // but if used for stateful drawing elsewhere, we might need a state setter.
    // For now, ignoring as glBlitTexture/Square takes explicit color.
}

void SpriteDrawer::BlitSprite(const DrawContext& ctx, int screenx, int screeny, uint32_t clientID, DrawColor color)
{
    if (clientID == 0 || clientID >= ctx.backend.sprite_database.getMetadataSpace().size() || clientID >= ctx.backend.sprite_database.getAtlasCacheSpace().size()) {
        return;
    }
    const SpriteMetadata& meta = ctx.backend.sprite_database.getMetadataSpace()[clientID];
    SpriteAtlasCache& atlas = ctx.backend.sprite_database.getAtlasCacheSpace()[clientID];

    screenx -= meta.getDrawOffset().first;
    screeny -= meta.getDrawOffset().second;

    int tme = 0; // GetTime() % itype->FPA;

    for (int cx = 0; cx != meta.width; ++cx) {
        for (int cy = 0; cy != meta.height; ++cy) {
            for (int cf = 0; cf != meta.layers; ++cf) {
                const AtlasRegion* region = atlas.getAtlasRegion(clientID, meta, cx, cy, cf, -1, 0, 0, 0, tme, ctx.state.is_preload_pass);
                if (region) {
                    glBlitAtlasQuad(ctx, screenx - cx * TILE_SIZE, screeny - cy * TILE_SIZE, region, color);
                }
            }
        }
    }
}
