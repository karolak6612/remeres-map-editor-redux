local Common = dofile("common.lua")

local M = {}

local function export_data(border_or_export)
    if border_or_export and border_or_export.toBorderItemEntries then
        return {
            borderId = border_or_export.borderId,
            group = border_or_export.group or 0,
            borderItems = border_or_export:toBorderItemEntries(),
        }
    end
    return border_or_export
end

function M.serialize_border(border_or_export)
    local border = export_data(border_or_export) or {}
    local attrs = string.format('id="%d"', border.borderId or border.id or 0)
    if (border.group or 0) > 0 then
        attrs = attrs .. string.format(' group="%d"', border.group)
    end

    local lines = {
        string.format('\t<border %s>', attrs),
    }

    for _, entry in ipairs(border.borderItems or {}) do
        table.insert(lines, string.format('\t\t<borderitem edge="%s" item="%d"/>', entry.edge, entry.itemId))
    end

    table.insert(lines, "\t</border>")
    return table.concat(lines, "\n")
end

function M.serialize(border_map)
    local ids = {}
    for id in pairs(border_map) do
        table.insert(ids, id)
    end
    table.sort(ids)

    local lines = {
        '<?xml version="1.0" encoding="utf-8"?>',
        "<materials>",
    }

    for _, id in ipairs(ids) do
        table.insert(lines, M.serialize_border(border_map[id]))
    end

    table.insert(lines, "</materials>")
    table.insert(lines, "")
    return table.concat(lines, "\n")
end

return M
