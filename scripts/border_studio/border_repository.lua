local Common = dofile("common.lua")
local BorderDraft = dofile("border_draft.lua")
local BorderLoader = dofile("border_loader.lua")
local Logger = dofile("logger.lua")

local M = {}

local function copy_border(border)
    if not border then
        return nil
    end
    if border.clone then
        return border:clone()
    end
    return BorderDraft.new(border)
end

local function rebuild_ordered(repository)
    repository.ordered = {}
    for _, border in pairs(repository.by_id or {}) do
        table.insert(repository.ordered, border)
    end
    table.sort(repository.ordered, function(left, right)
        return left.borderId < right.borderId
    end)
    return repository
end

function M.read(session_borders, border_table)
    Logger.log("repository", "read start")
    local repository = BorderLoader.read_library(border_table or app.borders or {})

    for id, border in pairs(session_borders or {}) do
        repository.by_id[id] = copy_border(border)
        repository.by_id[id].isNew = false
        repository.by_id[id].status = Common.border_status(repository.by_id[id])
    end

    rebuild_ordered(repository)
    Logger.log("repository", string.format("read done count=%d", #repository.ordered))
    return repository
end

function M.copy(border)
    return copy_border(border)
end

function M.next_border_id(repository, session_borders, extra_id)
    local max_id = 0

    for id in pairs((repository and repository.by_id) or {}) do
        if id > max_id then
            max_id = id
        end
    end

    for id in pairs(session_borders or {}) do
        if id > max_id then
            max_id = id
        end
    end

    extra_id = tonumber(extra_id or 0) or 0
    if extra_id > max_id then
        max_id = extra_id
    end

    return max_id + 1
end

function M.border_sources(repository)
    local sources = {}

    for _, border in ipairs((repository and repository.ordered) or {}) do
        local display_name = border.name ~= "" and border.name or ("Border #" .. tostring(border.borderId))
        table.insert(sources, {
            borderId = border.borderId,
            name = display_name,
            group = border.group or 0,
            status = Common.border_status(border),
            title = Common.format_border_title(border),
            sampleItemId = Common.primary_item_id(border),
            assignedSlotCount = Common.assigned_slot_count(border),
            tooltip = string.format(
                "Border %d\n%s\nGroup %d\nStatus: %s\nAssigned slots: %d",
                border.borderId,
                display_name,
                border.group or 0,
                Common.border_status(border),
                Common.assigned_slot_count(border)
            ),
        })
    end

    return sources
end

function M.find_border_source(repository, border_id)
    border_id = tonumber(border_id or 0) or 0
    if border_id <= 0 then
        return nil
    end

    for _, source in ipairs(M.border_sources(repository)) do
        if source.borderId == border_id then
            return source
        end
    end

    return nil
end

return M
