#include "util/nvg_utils.h"

#include "game/items.h"
#include "rendering/core/game_sprite.h"
#include "rendering/core/normal_image.h"
#include "ui/gui.h"

#include <algorithm>

namespace NvgUtils {

    std::unique_ptr<uint8_t[]> CreateCompositeRGBA(uint32_t clientID, int& outW, int& outH)
    {
        if (clientID == 0 || clientID >= g_gui.sprites.getMetadataSpace().size()) {
            return nullptr;
        }
        const SpriteMetadata& metadata = g_gui.sprites.getMetadata(clientID);
        const SpriteAtlasCache& atlas_cache = g_gui.sprites.getAtlasCache(clientID);

        outW = metadata.width * 32;
        outH = metadata.height * 32;

        if (outW <= 0 || outH <= 0) {
            return nullptr;
        }

        size_t bufferSize = static_cast<size_t>(outW) * outH * 4;
        auto composite = std::make_unique<uint8_t[]>(bufferSize);
        std::fill(composite.get(), composite.get() + bufferSize, 0);

        int pattern_x = (metadata.pattern_x >= 3) ? 2 : 0;
        int pattern_y = 0;
        int pattern_z = 0;
        int frame = 0;

        for (int l = 0; l < metadata.layers; ++l) {
            for (int w = 0; w < metadata.width; ++w) {
                for (int h = 0; h < metadata.height; ++h) {
                    int spriteIdx = metadata.getIndex(w, h, l, pattern_x, pattern_y, pattern_z, frame);
                    if (spriteIdx < 0 || (size_t)spriteIdx >= atlas_cache.spriteList.size()) {
                        continue;
                    }

                    auto* normal_img = dynamic_cast<NormalImage*>(atlas_cache.spriteList[spriteIdx]);
                    if (!normal_img) {
                        continue;
                    }

                    auto spriteData = normal_img->getRGBAData();
                    if (!spriteData) {
                        continue;
                    }

                    // Right-to-left, bottom-to-top arrangement (standard RME rendering order)
                    int part_x = (metadata.width - w - 1) * 32;
                    int part_y = (metadata.height - h - 1) * 32;

                    for (int sy = 0; sy < 32; ++sy) {
                        for (int sx = 0; sx < 32; ++sx) {
                            int dy = part_y + sy;
                            int dx = part_x + sx;

                            if (dx < 0 || dx >= outW || dy < 0 || dy >= outH) {
                                continue;
                            }

                            int src_idx = (sy * 32 + sx) * 4;
                            int dst_idx = (dy * outW + dx) * 4;

                            uint8_t sa = spriteData.get()[src_idx + 3];
                            if (sa == 0) {
                                continue;
                            }

                            if (sa == 255) {
                                composite.get()[dst_idx + 0] = spriteData.get()[src_idx + 0];
                                composite.get()[dst_idx + 1] = spriteData.get()[src_idx + 1];
                                composite.get()[dst_idx + 2] = spriteData.get()[src_idx + 2];
                                composite.get()[dst_idx + 3] = 255;
                            } else {
                                float srcA = sa / 255.0f;
                                float dstA = composite.get()[dst_idx + 3] / 255.0f;
                                float outA = srcA + dstA * (1.0f - srcA);

                                if (outA > 0.0f) {
                                    float srcR = spriteData.get()[src_idx + 0] / 255.0f;
                                    float srcG = spriteData.get()[src_idx + 1] / 255.0f;
                                    float srcB = spriteData.get()[src_idx + 2] / 255.0f;

                                    float dstR = composite.get()[dst_idx + 0] / 255.0f;
                                    float dstG = composite.get()[dst_idx + 1] / 255.0f;
                                    float dstB = composite.get()[dst_idx + 2] / 255.0f;

                                    float outR = (srcR * srcA + dstR * dstA * (1.0f - srcA)) / outA;
                                    float outG = (srcG * srcA + dstG * dstA * (1.0f - srcA)) / outA;
                                    float outB = (srcB * srcA + dstB * dstA * (1.0f - srcA)) / outA;

                                    composite.get()[dst_idx + 0] = static_cast<uint8_t>(std::clamp(outR * 255.0f, 0.0f, 255.0f));
                                    composite.get()[dst_idx + 1] = static_cast<uint8_t>(std::clamp(outG * 255.0f, 0.0f, 255.0f));
                                    composite.get()[dst_idx + 2] = static_cast<uint8_t>(std::clamp(outB * 255.0f, 0.0f, 255.0f));
                                }
                                composite.get()[dst_idx + 3] = static_cast<uint8_t>(std::clamp(outA * 255.0f, 0.0f, 255.0f));
                            }
                        }
                    }
                }
            }
        }
        return composite;
    }

    int CreateItemTexture(NVGcontext* vg, uint16_t id)
    {
        const ItemType& it = g_items.getItemType(id);
        if (it.clientID == 0) {
            return 0;
        }

        int w, h;
        auto composite = CreateCompositeRGBA(it.clientID, w, h);
        if (!composite) {
            return 0;
        }

        if (!vg) {
            spdlog::error("NanoVG context is null in CreateItemTexture");
            return 0;
        }

        return nvgCreateImageRGBA(vg, w, h, 0, composite.get());
    }
} // namespace NvgUtils
