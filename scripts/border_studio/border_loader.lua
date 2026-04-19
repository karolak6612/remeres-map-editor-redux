local Common = dofile("common.lua")
local BorderDraft = dofile("border_draft.lua")
local Logger = dofile("logger.lua")

local M = {}

local TILE_INDEX_TO_EDGE = {
    [2] = "n",
    [3] = "e",
    [4] = "s",
    [5] = "w",
    [6] = "cnw",
    [7] = "cne",
    [8] = "csw",
    [9] = "cse",
    [10] = "dnw",
    [11] = "dne",
    [12] = "dse",
    [13] = "dsw",
}

local function fallback_name(border_id)
    return "Border #" .. tostring(border_id)
end

local function build_status(draft)
    return Common.assigned_slot_count(draft) == #Common.slot_order and "complete" or "partial"
end

function M.apply_edge_map(draft, edge_map, border_id)
    local warnings = {}

    for edge_name, raw_item in pairs(edge_map or {}) do
        if Common.is_valid_slot_name(edge_name) then
            local ok, message = draft:assignSlot(edge_name, raw_item)
            if not ok then
                table.insert(warnings, string.format("Border %d: %s", border_id or 0, message))
            end
        else
            table.insert(warnings, string.format("Border %d: unsupported edge '%s' was skipped.", border_id or 0, tostring(edge_name)))
        end
    end

    return warnings
end

function M.draft_from_border_table(border)
    local border_id = tonumber((border and (border.id or border.borderId)) or 0) or 0
    Logger.log("loader", string.format("draft_from_border_table start border_id=%d", border_id))
    local draft = BorderDraft.new({
        borderId = border_id,
        name = (border and border.name) or fallback_name(border_id),
        group = tonumber((border and border.group) or 0) or 0,
        isNew = false,
    })

    local edge_map = {}
    local tiles = (border and border.tiles) or {}
    for tile_index, edge_name in pairs(TILE_INDEX_TO_EDGE) do
        local item_id = tonumber(tiles[tile_index] or 0) or 0
        if item_id > 0 then
            edge_map[edge_name] = item_id
        end
    end

    local warnings = M.apply_edge_map(draft, edge_map, border_id)
    draft.status = build_status(draft)
    Logger.log("loader", string.format("draft_from_border_table done border_id=%d assigned=%d status=%s", border_id, Common.assigned_slot_count(draft), draft.status))
    return draft, warnings
end

function M.read_library(border_table)
    Logger.log("loader", "read_library start")
    local repository = {
        by_id = {},
        ordered = {},
        warnings = {},
    }

    for id, border in pairs(border_table or {}) do
        local entry, warnings = M.draft_from_border_table(border)
        if entry and entry.borderId > 0 then
            repository.by_id[entry.borderId] = entry
            table.insert(repository.ordered, entry)
        end

        for _, warning in ipairs(warnings or {}) do
            table.insert(repository.warnings, warning)
            Common.log_warning(warning)
        end
    end

    table.sort(repository.ordered, function(left, right)
        return left.borderId < right.borderId
    end)

    Logger.log("loader", string.format("read_library done count=%d warnings=%d", #repository.ordered, #repository.warnings))

    return repository
end

return M
