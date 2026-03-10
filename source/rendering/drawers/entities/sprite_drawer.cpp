#include "rendering/drawers/entities/sprite_drawer.h"
#include "game/sprites.h"
#include "rendering/core/game_sprite.h"
#include "rendering/core/sprite_resolver.h"
#include "item_definitions/core/item_definition_store.h"

#include "rendering/core/atlas_manager.h"
#include "rendering/core/sprite_batch.h"
#include <spdlog/spdlog.h>

SpriteDrawer::SpriteDrawer() { }

SpriteDrawer::~SpriteDrawer() { }

void SpriteDrawer::glBlitAtlasQuad(SpriteBatch& sprite_batch, int sx, int sy, const AtlasRegion* region, DrawColor color)
{
    if (region) {
        float normalizedR = color.r / 255.0f;
        float normalizedG = color.g / 255.0f;
        float normalizedB = color.b / 255.0f;
        float normalizedA = color.a / 255.0f;

        sprite_batch.draw(
            static_cast<float>(sx), static_cast<float>(sy), static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE), *region,
            normalizedR, normalizedG, normalizedB, normalizedA
        );
    }
}

void SpriteDrawer::glBlitSquare(SpriteBatch& sprite_batch, int sx, int sy, DrawColor color, int size)
{
    if (size == 0) {
        size = TILE_SIZE;
    }

    float normalizedR = color.r / 255.0f;
    float normalizedG = color.g / 255.0f;
    float normalizedB = color.b / 255.0f;
    float normalizedA = color.a / 255.0f;

    if (atlas_) {
        sprite_batch.drawRect(
            static_cast<float>(sx), static_cast<float>(sy), static_cast<float>(size), static_cast<float>(size),
            glm::vec4(normalizedR, normalizedG, normalizedB, normalizedA), *atlas_
        );
    }
}

void SpriteDrawer::glDrawBox(SpriteBatch& sprite_batch, int sx, int sy, int width, int height, DrawColor color)
{
    float normalizedR = color.r / 255.0f;
    float normalizedG = color.g / 255.0f;
    float normalizedB = color.b / 255.0f;
    float normalizedA = color.a / 255.0f;

    if (atlas_) {
        sprite_batch.drawRectLines(
            static_cast<float>(sx), static_cast<float>(sy), static_cast<float>(width), static_cast<float>(height),
            glm::vec4(normalizedR, normalizedG, normalizedB, normalizedA), *atlas_
        );
    }
}

void SpriteDrawer::BlitSprite(SpriteBatch& sprite_batch, int screenx, int screeny, ServerItemId server_item_id, DrawColor color)
{
    const auto definition = g_item_definitions.get(server_item_id);
    GameSprite* spr = (definition && sprite_resolver) ? sprite_resolver->getSprite(definition.clientId()) : nullptr;
    if (spr == nullptr) {
        return;
    }
    // Call the pointer overload
    BlitSprite(sprite_batch, screenx, screeny, spr, color);
}

void SpriteDrawer::BlitSprite(SpriteBatch& sprite_batch, int screenx, int screeny, GameSprite* spr, DrawColor color)
{
    if (spr == nullptr) {
        return;
    }
    screenx -= spr->getDrawOffset().first;
    screeny -= spr->getDrawOffset().second;

    int tme = 0; // GetTime() % itype->FPA;

    for (int cx = 0; cx != spr->width; ++cx) {
        for (int cy = 0; cy != spr->height; ++cy) {
            for (int cf = 0; cf != spr->layers; ++cf) {
                const AtlasRegion* region = spr->getAtlasRegion(cx, cy, cf, -1, 0, 0, 0, tme);
                if (region) {
                    glBlitAtlasQuad(sprite_batch, screenx - cx * TILE_SIZE, screeny - cy * TILE_SIZE, region, color);
                }
                // No fallback - if region is null, sprite failed to load
            }
        }
    }
}
