//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

// glut include removed

#include "game/creature.h"
#include "game/items.h"
#include "game/sprites.h"
#include "rendering/core/animator.h"
#include "rendering/core/sprite_atlas_cache.h"
#include "rendering/core/sprite_database.h"
#include "rendering/core/sprite_metadata.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "ui/gui.h"
#include <algorithm>
#include <spdlog/spdlog.h>

CreatureDrawer::CreatureDrawer() { }

CreatureDrawer::~CreatureDrawer() { }

void CreatureDrawer::BlitCreature(
    const DrawContext& ctx, SpriteDrawer* sprite_drawer, int screenx, int screeny, const Creature* c, const CreatureDrawOptions& options
)
{
    if (c == nullptr) {
        return;
    }

    CreatureDrawOptions local_opts = options;
    if (!local_opts.ingame
        && (c->isSelected()
            || (local_opts.transient_selection_bounds.has_value()
                && local_opts.transient_selection_bounds->contains(local_opts.map_pos.x, local_opts.map_pos.y)))) {
        local_opts.color.r /= 2;
        local_opts.color.g /= 2;
        local_opts.color.b /= 2;
    }

    BlitCreature(ctx, sprite_drawer, screenx, screeny, c->getLookType(), c->getDirection(), local_opts);
}

void CreatureDrawer::BlitCreature(
    const DrawContext& ctx, SpriteDrawer* sprite_drawer, int screenx, int screeny, const Outfit& outfit, Direction dir,
    const CreatureDrawOptions& options
)
{
    if (outfit.lookItem != 0) {
        ItemType& it = g_items[outfit.lookItem];
        sprite_drawer->BlitSprite(ctx, screenx, screeny, it.clientID, options.color);
    } else {
        // get outfit sprite
        uint32_t clientID = g_gui.sprites.getItemSpriteMaxID() + outfit.lookType;
        if (clientID >= g_gui.sprites.getMetadataSpace().size() || outfit.lookType == 0) {
            return;
        }

        const SpriteMetadata& meta = g_gui.sprites.getMetadataSpace()[clientID];
        SpriteAtlasCache& atlas = g_gui.sprites.getAtlasCacheSpace()[clientID];

        // Resolve animation frame for walk animation
        // For in-game preview: animationPhase controls walk animation
        // - When > 0: walking (use the provided animation phase)
        // - When == 0: standing idle (ALWAYS use frame 0, NOT the global animator)
        // The global animator is for idle creatures on the map, NOT for the player
        int resolvedFrame = options.animationPhase > 0 ? options.animationPhase : 0;

        // mount and addon drawing thanks to otc code
        // mount colors by Zbizu
        int pattern_z = 0;
        if (outfit.lookMount != 0) {
            uint32_t mountClientID = g_gui.sprites.getItemSpriteMaxID() + outfit.lookMount;
            if (mountClientID < g_gui.sprites.getMetadataSpace().size()) {
                const SpriteMetadata& mountMeta = g_gui.sprites.getMetadataSpace()[mountClientID];
                SpriteAtlasCache& mountAtlas = g_gui.sprites.getAtlasCacheSpace()[mountClientID];

                // generate mount colors
                Outfit mountOutfit;
                mountOutfit.lookType = outfit.lookMount;
                mountOutfit.lookHead = outfit.lookMountHead;
                mountOutfit.lookBody = outfit.lookMountBody;
                mountOutfit.lookLegs = outfit.lookMountLegs;
                mountOutfit.lookFeet = outfit.lookMountFeet;

                for (int cx = 0; cx != mountMeta.width; ++cx) {
                    for (int cy = 0; cy != mountMeta.height; ++cy) {
                        const AtlasRegion* region = mountAtlas.getAtlasRegion(
                            mountClientID, mountMeta, cx, cy, static_cast<int>(dir), 0, 0, mountOutfit, resolvedFrame
                        );
                        if (region) {
                            sprite_drawer->glBlitAtlasQuad(
                                ctx, screenx - cx * TILE_SIZE - mountMeta.getDrawOffset().first,
                                screeny - cy * TILE_SIZE - mountMeta.getDrawOffset().second, region, options.color
                            );
                        }
                    }
                }

                pattern_z = std::clamp(static_cast<int>(meta.pattern_z) - 1, 0, 1);
            }
        }

        // pattern_y => creature addon
        for (int pattern_y = 0; pattern_y < meta.pattern_y; pattern_y++) {

            // continue if we dont have this addon
            if (pattern_y > 0) {
                if ((pattern_y - 1 >= 31) || !(outfit.lookAddon & (1 << (pattern_y - 1)))) {
                    continue;
                }
            }

            for (int cx = 0; cx != meta.width; ++cx) {
                for (int cy = 0; cy != meta.height; ++cy) {
                    const AtlasRegion* region
                        = atlas.getAtlasRegion(clientID, meta, cx, cy, static_cast<int>(dir), pattern_y, pattern_z, outfit, resolvedFrame);
                    if (region) {
                        sprite_drawer->glBlitAtlasQuad(
                            ctx, screenx - cx * TILE_SIZE - meta.getDrawOffset().first,
                            screeny - cy * TILE_SIZE - meta.getDrawOffset().second, region, options.color
                        );
                    }
                }
            }
        }
    }
}
