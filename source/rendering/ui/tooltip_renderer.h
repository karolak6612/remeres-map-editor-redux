//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_TOOLTIP_RENDERER_H_
#define RME_TOOLTIP_RENDERER_H_

#include "rendering/ui/tooltip_collector.h"
#include <string>
#include <vector>
#include <span>

struct NVGcontext;
struct DrawContext;
class NVGImageCache;

class TooltipRenderer {
public:
    TooltipRenderer() = default;

    // Draw all tooltips
    void draw(NVGcontext* vg, const DrawContext& ctx, NVGImageCache* cache, std::span<const TooltipData> tooltips);

private:
    struct FieldLine {
        std::string label;
        std::string value;
        uint8_t r, g, b;
        std::vector<std::string> wrappedLines; // For multi-line values
    };
    std::vector<FieldLine> scratch_fields;
    size_t scratch_fields_count = 0;
    std::string storage; // Scratch buffer for text generation

    // Helper to get header color based on category
    void getHeaderColor(TooltipCategory cat, uint8_t& r, uint8_t& g, uint8_t& b) const;

    // Refactored drawing helpers
    struct LayoutMetrics {
        float width;
        float height;
        float valueStartX;
        float gridSlotSize;
        int containerCols;
        int containerRows;
        float containerHeight;
        int totalContainerSlots;
        int emptyContainerSlots;
        int numContainerItems;
    };

    void prepareFields(const TooltipData& tooltip);
    LayoutMetrics
    calculateLayout(NVGcontext* vg, const TooltipData& tooltip, float maxWidth, float minWidth, float padding, float fontSize);
    void drawBackground(NVGcontext* vg, float x, float y, float width, float height, float cornerRadius, const TooltipData& tooltip);
    void drawFields(NVGcontext* vg, float x, float y, float valueStartX, float lineHeight, float padding, float fontSize);
    void drawContainerGrid(NVGcontext* vg, float x, float y, const TooltipData& tooltip, const LayoutMetrics& layout, NVGImageCache* cache);
};

#endif
