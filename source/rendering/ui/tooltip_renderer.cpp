//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "rendering/ui/tooltip_renderer.h"
#include "rendering/ui/nvg_image_cache.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/view_state.h"
#include "ui/theme.h"
#include <wx/wx.h>
#include <format>
#include <nanovg.h>

void TooltipRenderer::getHeaderColor(TooltipCategory cat, uint8_t& r, uint8_t& g, uint8_t& b) const
{
    wxColour color;
    switch (cat) {
        case TooltipCategory::WAYPOINT:
            color = Theme::Get(Theme::Role::TooltipBorderWaypoint);
            break;
        case TooltipCategory::DOOR:
            color = Theme::Get(Theme::Role::TooltipBorderDoor);
            break;
        case TooltipCategory::TELEPORT:
            color = Theme::Get(Theme::Role::TooltipBorderTeleport);
            break;
        case TooltipCategory::TEXT:
            color = Theme::Get(Theme::Role::TooltipBorderText);
            break;
        case TooltipCategory::ITEM:
        default:
            color = Theme::Get(Theme::Role::TooltipBorderItem);
            break;
    }
    r = color.Red();
    g = color.Green();
    b = color.Blue();
}

void TooltipRenderer::prepareFields(const TooltipData& tooltip)
{
    // Build content lines with word wrapping support
    scratch_fields_count = 0;
    storage.clear();
    if (storage.capacity() < 4096) {
        storage.reserve(4096);
    }

    auto addField = [&](std::string_view label, std::string_view value, Theme::Role colorRole) {
        if (scratch_fields_count >= scratch_fields.size()) {
            scratch_fields.emplace_back();
        }
        FieldLine& field = scratch_fields[scratch_fields_count++];
        field.label = std::string(label);
        field.value = std::string(value);
        wxColour c = Theme::Get(colorRole);
        field.r = c.Red();
        field.g = c.Green();
        field.b = c.Blue();
        field.wrappedLines.clear();
    };

    if (tooltip.category == TooltipCategory::WAYPOINT) {
        addField("Waypoint", tooltip.waypointName, Theme::Role::TooltipWaypoint);
    } else {
        if (tooltip.actionId > 0) {
            size_t start = storage.size();
            std::format_to(std::back_inserter(storage), "{}", tooltip.actionId);
            addField("Action ID", std::string_view(storage.data() + start, storage.size() - start), Theme::Role::TooltipActionId);
        }
        if (tooltip.uniqueId > 0) {
            size_t start = storage.size();
            std::format_to(std::back_inserter(storage), "{}", tooltip.uniqueId);
            addField("Unique ID", std::string_view(storage.data() + start, storage.size() - start), Theme::Role::TooltipUniqueId);
        }
        if (tooltip.doorId > 0) {
            size_t start = storage.size();
            std::format_to(std::back_inserter(storage), "{}", tooltip.doorId);
            addField("Door ID", std::string_view(storage.data() + start, storage.size() - start), Theme::Role::TooltipDoorId);
        }
        if (tooltip.destination.x > 0) {
            size_t start = storage.size();
            std::format_to(std::back_inserter(storage), "{}, {}, {}", tooltip.destination.x, tooltip.destination.y, tooltip.destination.z);
            addField("Destination", std::string_view(storage.data() + start, storage.size() - start), Theme::Role::TooltipTeleport);
        }
        if (!tooltip.description.empty()) {
            addField("Description", tooltip.description, Theme::Role::TooltipBodyText);
        }
        if (!tooltip.text.empty()) {
            size_t start = storage.size();
            std::format_to(std::back_inserter(storage), "\"{}\"", tooltip.text);
            addField("Text", std::string_view(storage.data() + start, storage.size() - start), Theme::Role::TooltipTextValue);
        }
    }
}

TooltipRenderer::LayoutMetrics TooltipRenderer::calculateLayout(
    NVGcontext* vg, const TooltipData& tooltip, float maxWidth, float minWidth, float padding, float fontSize
)
{
    LayoutMetrics lm = {};

    // Set up font for measurements
    nvgFontSize(vg, fontSize);
    nvgFontFace(vg, "sans");

    // Measure label widths
    float maxLabelWidth = 0.0f;
    for (size_t i = 0; i < scratch_fields_count; ++i) {
        const auto& field = scratch_fields[i];
        float labelBounds[4];
        nvgTextBounds(vg, 0, 0, field.label.data(), field.label.data() + field.label.size(), labelBounds);
        float lw = labelBounds[2] - labelBounds[0];
        if (lw > maxLabelWidth) {
            maxLabelWidth = lw;
        }
    }

    lm.valueStartX = maxLabelWidth + 12.0f; // Gap between label and value
    float maxValueWidth = maxWidth - lm.valueStartX - padding * 2;

    // Word wrap values that are too long
    int totalLines = 0;
    float actualMaxWidth = minWidth;

    for (size_t i = 0; i < scratch_fields_count; ++i) {
        auto& field = scratch_fields[i];
        const char* start = field.value.data();
        const char* end = start + field.value.length();

        // Check if value fits on one line
        float valueBounds[4];
        nvgTextBounds(vg, 0, 0, start, end, valueBounds);
        float valueWidth = valueBounds[2] - valueBounds[0];

        if (valueWidth <= maxValueWidth) {
            // Single line
            field.wrappedLines.push_back(field.value);
            totalLines++;
            float lineWidth = lm.valueStartX + valueWidth + padding * 2;
            if (lineWidth > actualMaxWidth) {
                actualMaxWidth = lineWidth;
            }
        } else {
            // Need to wrap - use NanoVG text breaking with loop for >16 rows
            const char* currentStart = start;
            bool any_rows = false;
            while (currentStart && currentStart < end) {
                NVGtextRow rows[16];
                int nRows = nvgTextBreakLines(vg, currentStart, end, maxValueWidth, rows, 16);
                if (nRows == 0) {
                    break;
                }
                any_rows = true;
                for (int j = 0; j < nRows; ++j) {
                    field.wrappedLines.emplace_back(rows[j].start, rows[j].end - rows[j].start);
                    totalLines++;
                    float lineWidth = lm.valueStartX + rows[j].width + padding * 2;
                    if (lineWidth > actualMaxWidth) {
                        actualMaxWidth = lineWidth;
                    }
                }
                currentStart = rows[nRows - 1].next;
            }

            if (!any_rows) {
                // Fallback if breaking failed
                field.wrappedLines.push_back(field.value);
                totalLines++;
            }
        }
    }

    // Calculate container grid dimensions
    lm.containerCols = 0;
    lm.containerRows = 0;
    lm.gridSlotSize = 34.0f; // 32px + padding
    lm.containerHeight = 0.0f;

    lm.numContainerItems = static_cast<int>(tooltip.containerItems.size());
    int capacity = static_cast<int>(tooltip.containerCapacity);
    lm.emptyContainerSlots = std::max(0, capacity - lm.numContainerItems);
    lm.totalContainerSlots = lm.numContainerItems;

    if (lm.emptyContainerSlots > 0) {
        lm.totalContainerSlots++; // Add one slot for the summary
    }

    // Apply a hard cap for visual safety
    if (lm.totalContainerSlots > 33) {
        lm.totalContainerSlots = 33;
    }

    if (capacity > 0 || lm.numContainerItems > 0) {
        // Heuristic: try to keep it somewhat square but matching width
        lm.containerCols = std::min(4, lm.totalContainerSlots);
        if (lm.totalContainerSlots > 4) {
            lm.containerCols = 5;
        }
        if (lm.totalContainerSlots > 10) {
            lm.containerCols = 6;
        }
        if (lm.totalContainerSlots > 15) {
            lm.containerCols = 8;
        }

        if (lm.containerCols == 0 && lm.totalContainerSlots > 0) {
            lm.containerCols = 1;
        }

        if (lm.containerCols > 0) {
            lm.containerRows = (lm.totalContainerSlots + lm.containerCols - 1) / lm.containerCols;
            lm.containerHeight = lm.containerRows * lm.gridSlotSize + 4.0f; // + top margin
        }
    }

    // Calculate final box dimensions
    lm.width = std::min(maxWidth + padding * 2, std::max(minWidth, actualMaxWidth));

    bool hasContainer = lm.totalContainerSlots > 0;
    if (hasContainer) {
        float gridWidth = lm.containerCols * lm.gridSlotSize;
        lm.width = std::max(lm.width, gridWidth + padding * 2);
    }

    float lineHeight = fontSize * 1.4f;
    lm.height = totalLines * lineHeight + padding * 2;
    if (hasContainer) {
        lm.height += lm.containerHeight + 4.0f;
    }

    return lm;
}

void TooltipRenderer::drawBackground(
    NVGcontext* vg, float x, float y, float width, float height, float cornerRadius, const TooltipData& tooltip
)
{
    // Get border color based on category
    uint8_t borderR, borderG, borderB;
    getHeaderColor(tooltip.category, borderR, borderG, borderB);

    // Shadow (multi-layer soft shadow)
    for (int i = 3; i >= 0; i--) {
        float alpha = 35.0f + (3 - i) * 20.0f;
        float spread = i * 2.0f;
        float offsetY = 3.0f + i * 1.0f;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x - spread, y + offsetY - spread, width + spread * 2, height + spread * 2, cornerRadius + spread);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, static_cast<unsigned char>(alpha)));
        nvgFill(vg);
    }

    // Main background - use theme
    wxColour bgCol = Theme::Get(Theme::Role::TooltipBg);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, width, height, cornerRadius);
    nvgFillColor(vg, nvgRGBA(bgCol.Red(), bgCol.Green(), bgCol.Blue(), 250));
    nvgFill(vg);

    // Full colored border around entire frame
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, width, height, cornerRadius);
    nvgStrokeColor(vg, nvgRGBA(borderR, borderG, borderB, 255));
    nvgStrokeWidth(vg, 1.0f);
    nvgStroke(vg);
}

void TooltipRenderer::drawFields(NVGcontext* vg, float x, float y, float valueStartX, float lineHeight, float padding, float fontSize)
{
    float contentX = x + padding;
    float cursorY = y + padding;

    nvgFontSize(vg, fontSize);
    nvgFontFace(vg, "sans");
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

    for (size_t i = 0; i < scratch_fields_count; ++i) {
        const auto& field = scratch_fields[i];
        bool firstLine = true;
        for (const auto& line : field.wrappedLines) {
            if (firstLine) {
                // Draw label - use theme tooltip label color
                wxColour labelCol = Theme::Get(Theme::Role::TooltipLabel);
                nvgFillColor(vg, nvgRGBA(labelCol.Red(), labelCol.Green(), labelCol.Blue(), 200));
                nvgText(vg, contentX, cursorY, field.label.data(), field.label.data() + field.label.size());
                firstLine = false;
            }

            // Draw value line in theme-defined semantic color
            nvgFillColor(vg, nvgRGBA(field.r, field.g, field.b, 255));
            nvgText(vg, contentX + valueStartX, cursorY, line.data(), line.data() + line.size());

            cursorY += lineHeight;
        }
    }
}

void TooltipRenderer::drawContainerGrid(
    NVGcontext* vg, float x, float y, const TooltipData& tooltip, const LayoutMetrics& layout, NVGImageCache* cache
)
{
    if (layout.totalContainerSlots <= 0) {
        return;
    }

    // Calculate cursorY after text fields
    float fontSize = 11.0f;
    float lineHeight = fontSize * 1.4f;
    float textBlockHeight = 0.0f;
    for (size_t i = 0; i < scratch_fields_count; ++i) {
        const auto& field = scratch_fields[i];
        textBlockHeight += field.wrappedLines.size() * lineHeight;
    }

    float startY = y + 10.0f + textBlockHeight + 8.0f; // Padding + Text + Spacer
    float startX = x + 10.0f; // Padding

    for (int idx = 0; idx < layout.totalContainerSlots; ++idx) {
        int col = idx % layout.containerCols;
        int row = idx / layout.containerCols;

        float itemX = startX + col * layout.gridSlotSize;
        float itemY = startY + row * layout.gridSlotSize;

        // Draw slot background (always)
        nvgBeginPath(vg);
        nvgRect(vg, itemX, itemY, 32, 32);
        wxColour baseCol = Theme::Get(Theme::Role::CardBase);
        wxColour borderCol = Theme::Get(Theme::Role::Border);

        nvgFillColor(vg, nvgRGBA(baseCol.Red(), baseCol.Green(), baseCol.Blue(), 100)); // Dark slot placeholder
        nvgStrokeColor(vg, nvgRGBA(borderCol.Red(), borderCol.Green(), borderCol.Blue(), 100)); // Light border
        nvgStrokeWidth(vg, 1.0f);
        nvgFill(vg);
        nvgStroke(vg);

        // Check if this is the summary info slot (last slot if we have empty spaces)
        bool isSummarySlot = (layout.emptyContainerSlots > 0 && idx == layout.totalContainerSlots - 1);

        if (isSummarySlot) {
            // Draw empty slots count: "+N"
            std::string summary = "+" + std::to_string(layout.emptyContainerSlots);

            nvgFontSize(vg, 12.0f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

            // Text shadow
            nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
            nvgText(vg, itemX + 17, itemY + 17, summary.c_str(), nullptr);

            // Text
            wxColour countCol = Theme::Get(Theme::Role::TooltipCountText);
            nvgFillColor(vg, nvgRGBA(countCol.Red(), countCol.Green(), countCol.Blue(), 255));
            nvgText(vg, itemX + 16, itemY + 16, summary.c_str(), nullptr);

        } else if (idx < layout.numContainerItems) {
            // Draw Actual Item
            const auto& item = tooltip.containerItems[idx];

            // Draw item sprite
            int img = cache->getSpriteImage(vg, item.id);
            if (img > 0) {
                nvgBeginPath(vg);
                nvgRect(vg, itemX, itemY, 32, 32);
                nvgFillPaint(vg, nvgImagePattern(vg, itemX, itemY, 32, 32, 0, img, 1.0f));
                nvgFill(vg);
            }

            // Draw Count
            if (item.count > 1) {
                std::string countStr = std::to_string(item.count);
                nvgFontSize(vg, 10.0f);
                nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);

                // Text shadow
                nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
                nvgText(vg, itemX + 33, itemY + 33, countStr.c_str(), nullptr);

                // Text
                wxColour countCol = Theme::Get(Theme::Role::TooltipCountText);
                nvgFillColor(vg, nvgRGBA(countCol.Red(), countCol.Green(), countCol.Blue(), 255));
                nvgText(vg, itemX + 32, itemY + 32, countStr.c_str(), nullptr);
            }
        }
    }
}

void TooltipRenderer::draw(NVGcontext* vg, const DrawContext& ctx, NVGImageCache* cache, std::span<const TooltipData> tooltips)
{
    if (!vg) {
        return;
    }
    const auto& view = ctx.state.view;

    for (const auto& tooltip : tooltips) {
        int unscaled_x, unscaled_y;
        view.getScreenPosition(tooltip.pos.x, tooltip.pos.y, tooltip.pos.z, unscaled_x, unscaled_y);

        float zoom = view.zoom < 0.01f ? 1.0f : view.zoom;

        float screen_x = unscaled_x / zoom;
        float screen_y = unscaled_y / zoom;
        float tile_size_screen = 32.0f / zoom;

        // Center on tile
        screen_x += tile_size_screen / 2.0f;
        screen_y += tile_size_screen / 2.0f;

        // Constants
        float fontSize = 11.0f;
        float padding = 10.0f;
        float minWidth = 120.0f;
        float maxWidth = 220.0f;

        // 1. Prepare Content
        prepareFields(tooltip);

        // Skip if nothing to show
        if (scratch_fields_count == 0 && tooltip.containerItems.empty()) {
            continue;
        }

        // 2. Calculate Layout
        LayoutMetrics layout = calculateLayout(vg, tooltip, maxWidth, minWidth, padding, fontSize);

        // Position tooltip above tile
        float tooltipX = screen_x - (layout.width / 2.0f);
        float tooltipY = screen_y - layout.height - 12.0f;

        // 3. Draw Background & Shadow
        drawBackground(vg, tooltipX, tooltipY, layout.width, layout.height, 4.0f, tooltip);

        // 4. Draw Text Fields
        drawFields(vg, tooltipX, tooltipY, layout.valueStartX, fontSize * 1.4f, padding, fontSize);

        // 5. Draw Container Grid
        drawContainerGrid(vg, tooltipX, tooltipY, tooltip, layout, cache);
    }
}
