local Common = dofile("common.lua")

local M = {}

local function empty_cell()
    return {
        kind = "empty",
        label = "",
    }
end

local function fill_cell()
    return {
        kind = "fill",
        label = "AREA",
        tooltip = "Filled area reference",
    }
end

local function slot_cell(draft, slot_name)
    local meta = Common.slot_meta[slot_name]
    local item_id = draft and draft:getSlotItemId(slot_name) or 0
    local missing = item_id <= 0

    return {
        kind = missing and "missing" or "slot",
        slotName = slot_name,
        label = meta and meta.short or tostring(slot_name),
        tooltip = meta and meta.full or tostring(slot_name),
        itemId = item_id,
    }
end

local function build_scenario_cells(draft, rows)
    local cells = {}
    for _, row in ipairs(rows) do
        local next_row = {}
        for _, token in ipairs(row) do
            if token == "." then
                table.insert(next_row, empty_cell())
            elseif token == "#" then
                table.insert(next_row, fill_cell())
            else
                table.insert(next_row, slot_cell(draft, token))
            end
        end
        table.insert(cells, next_row)
    end
    return cells
end

function M.slot_name_from_layout(index)
    return Common.storage_slot_name_from_layout(index)
end

function M.visual_slot_name_from_layout(index)
    return Common.visual_slot_name_from_layout(index)
end

function M.slot_grid_items(draft, selected_raw_id, options)
    options = options or {}
    local show_images = options.showImages == true
    local items = {}

    for index = 1, Common.slot_layout_count do
        local visual_key = Common.slot_layout[index]
        if not visual_key then
            table.insert(items, { text = "", tooltip = "" })
        else
            local meta = Common.slot_meta[visual_key]
            local item_id = 0

            if visual_key ~= "c" and draft then
                item_id = draft:getSlotItemId(Common.storage_slot_name(visual_key))
            end

            local tooltip = meta.full
            if visual_key == "c" then
                tooltip = "Center Reference"
            elseif item_id > 0 then
                tooltip = string.format("%s\nItem ID %d", meta.full, item_id)
            else
                tooltip = meta.full .. "\nEmpty slot"
            end

            table.insert(items, {
                text = meta.short,
                tooltip = tooltip,
                image = show_images and Common.safe_image(item_id) or nil,
            })
        end
    end

    return items
end

function M.preview_state(draft)
    local validation = draft and draft:getValidation() or {
        hasAssignments = false,
        emptySlots = Common.clone(Common.slot_order),
    }

    return {
        hasAssignments = validation.hasAssignments == true,
        emptySlots = validation.emptySlots or {},
        assignedCount = draft and Common.assigned_slot_count(draft) or 0,
    }
end

function M.usage_preview_state(draft)
    local state = M.preview_state(draft)
    local is_complete = #state.emptySlots == 0 and state.hasAssignments

    return {
        isComplete = is_complete,
        statusLabel = is_complete and "border kompletny" or "border niepelny",
        missingSlots = state.emptySlots,
        missingLabel = #state.emptySlots > 0 and table.concat(state.emptySlots, ", ") or "none",
    }
end

function M.usage_scenarios(draft)
    local missing_slots = draft and draft:getEmptySlots() or Common.clone(Common.slot_order)
    local partial_tokens = {
        missing_slots[1] or "dne",
        missing_slots[2] or "dsw",
        missing_slots[3] or "cnw",
        missing_slots[4] or "cse",
    }

    local scenarios = {
        {
            id = "single_edge",
            title = "1. Pojedyncza krawedz",
            subtitle = "Single edge sample",
            rows = {
                { ".", "n", "." },
                { ".", "#", "." },
                { ".", ".", "." },
            },
        },
        {
            id = "outer_corner",
            title = "2. Naroznik zewnetrzny",
            subtitle = "Outer corner sample",
            rows = {
                { "cnw", "n", "." },
                { "w", "#", "." },
                { ".", ".", "." },
            },
        },
        {
            id = "inner_corner",
            title = "3. Naroznik wewnetrzny",
            subtitle = "Inner corner sample",
            rows = {
                { "#", "#", "e" },
                { "#", "cse", "s" },
                { ".", ".", "." },
            },
        },
        {
            id = "diagonal",
            title = "4. Diagonal",
            subtitle = "Diagonal sample",
            rows = {
                { ".", "#", "dne" },
                { ".", ".", "#" },
                { ".", ".", "." },
            },
        },
        {
            id = "rectangle",
            title = "5. Prostokat",
            subtitle = "Closed area sample",
            rows = {
                { "cnw", "n", "cne" },
                { "w", "#", "e" },
                { "csw", "s", "cse" },
            },
        },
        {
            id = "partial",
            title = "6. Partial / brak",
            subtitle = #missing_slots > 0 and "Missing slots highlighted" or "No missing slots right now",
            rows = {
                { partial_tokens[1], partial_tokens[2] },
                { partial_tokens[3], partial_tokens[4] },
            },
        },
    }

    for _, scenario in ipairs(scenarios) do
        scenario.cells = build_scenario_cells(draft, scenario.rows)
    end

    return scenarios
end

return M
