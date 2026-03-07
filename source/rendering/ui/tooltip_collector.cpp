//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "rendering/ui/tooltip_collector.h"
#include <utility>

void TooltipCollector::clear()
{
    active_count = 0;
}

TooltipData& TooltipCollector::requestTooltipData()
{
    if (active_count >= tooltips.size()) {
        tooltips.emplace_back();
    }
    TooltipData& data = tooltips[active_count];
    data.clear();
    return data;
}

void TooltipCollector::commitTooltip()
{
    active_count++;
}

void TooltipCollector::addItemTooltip(const TooltipData& data)
{
    if (!data.hasVisibleFields()) {
        return;
    }
    TooltipData& dest = requestTooltipData();
    dest = data;
    commitTooltip();
}

void TooltipCollector::addItemTooltip(TooltipData&& data)
{
    if (!data.hasVisibleFields()) {
        return;
    }
    TooltipData& dest = requestTooltipData();
    dest = std::move(data);
    commitTooltip();
}

void TooltipCollector::addWaypointTooltip(Position pos, std::string_view name)
{
    if (name.empty()) {
        return;
    }
    TooltipData& data = requestTooltipData();
    data.pos = pos;
    data.category = TooltipCategory::WAYPOINT;
    data.waypointName = name;
    commitTooltip();
}
