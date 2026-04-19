local M = {}
M._image_cache = M._image_cache or {}

M.palette = {
    header = "#1F2937",
    panel = "#243447",
    panel_alt = "#2C3E50",
    accent = "#D97706",
    accent_soft = "#F59E0B",
    success = "#2F855A",
    text = "#F8FAFC",
    muted = "#CBD5E1",
    subtle = "#94A3B8",
    border = "#475569",
}

M.slot_order = {
    "n", "e", "s", "w",
    "cnw", "cne", "csw", "cse",
    "dnw", "dne", "dsw", "dse",
}

M.slot_set = {}
for _, slot_name in ipairs(M.slot_order) do
    M.slot_set[slot_name] = true
end

M.slot_meta = {
    n = { short = "N", full = "North", edge = "n" },
    e = { short = "E", full = "East", edge = "e" },
    s = { short = "S", full = "South", edge = "s" },
    w = { short = "W", full = "West", edge = "w" },
    cnw = { short = "CNW", full = "Corner North-West", edge = "cnw" },
    cne = { short = "CNE", full = "Corner North-East", edge = "cne" },
    csw = { short = "CSW", full = "Corner South-West", edge = "csw" },
    cse = { short = "CSE", full = "Corner South-East", edge = "cse" },
    dnw = { short = "DNW", full = "Diagonal North-West", edge = "dnw" },
    dne = { short = "DNE", full = "Diagonal North-East", edge = "dne" },
    dsw = { short = "DSW", full = "Diagonal South-West", edge = "dsw" },
    dse = { short = "DSE", full = "Diagonal South-East", edge = "dse" },
    c = { short = "C", full = "Center Reference", edge = nil },
}

M.slot_layout = {
    "dnw", nil,   nil,   nil,   "dne",
    nil,   "cnw", "n",   "cne", nil,
    nil,   "w",   "c",   "e",   nil,
    nil,   "csw", "s",   "cse", nil,
    "dsw", nil,   nil,   nil,   "dse",
}
M.slot_layout_count = 25

-- The XML/app.borders slot names are stored from the border brush perspective.
-- In the editor we want to show the patch from the map/player perspective,
-- so the visual slot on screen needs to read from the opposite stored edge.
M.preview_slot_source = {
    n = "s",
    e = "w",
    s = "n",
    w = "e",
    cnw = "cse",
    cne = "csw",
    csw = "cne",
    cse = "cnw",
    dnw = "dse",
    dne = "dsw",
    dsw = "dne",
    dse = "dnw",
}

function M.clone(value)
    if type(value) ~= "table" then
        return value
    end

    local copy = {}
    for key, inner in pairs(value) do
        copy[key] = M.clone(inner)
    end
    return copy
end

function M.new_slots()
    local slots = {}
    for _, key in ipairs(M.slot_order) do
        slots[key] = nil
    end
    return slots
end

function M.is_valid_slot_name(slot_name)
    return M.slot_set[slot_name] == true
end

function M.slot_name_from_layout(index)
    local key = M.slot_layout[index]
    if key == "c" then
        return nil
    end
    return key
end

function M.visual_slot_name_from_layout(index)
    return M.slot_name_from_layout(index)
end

function M.storage_slot_name(slot_name)
    if not slot_name then
        return nil
    end
    return M.preview_slot_source[slot_name] or slot_name
end

function M.storage_slot_name_from_layout(index)
    local visual_key = M.visual_slot_name_from_layout(index)
    return M.storage_slot_name(visual_key)
end

function M.slot_item_id(border, slot_name)
    if not border or not slot_name then
        return 0
    end

    if border.getSlotAssignment then
        local assignment = border:getSlotAssignment(slot_name)
        return assignment and (assignment.itemId or 0) or 0
    end

    if not border.slots then
        return 0
    end

    local value = border.slots[slot_name]
    if type(value) == "table" then
        return tonumber(value.itemId or value.item_id or value.id or 0) or 0
    end

    return tonumber(value or 0) or 0
end

function M.primary_item_id(border)
    if not border or not border.slots then
        return 0
    end

    for _, key in ipairs(M.slot_order) do
        local item_id = M.slot_item_id(border, key)
        if item_id > 0 then
            return item_id
        end
    end

    return 0
end

function M.assigned_slot_count(border)
    local count = 0
    if not border or not border.slots then
        return count
    end

    for _, key in ipairs(M.slot_order) do
        if M.slot_item_id(border, key) > 0 then
            count = count + 1
        end
    end
    return count
end

function M.format_border_title(border)
    if not border then
        return "No border"
    end

    local border_id = border.borderId or border.id or 0
    local group = border.group or 0
    local is_new = border.isNew
    if is_new == nil then
        is_new = border.is_new
    end

    local suffix = is_new and " (new)" or ""
    return string.format("Border %d  |  Group %d%s", border_id, group, suffix)
end

function M.normalize_path(path)
    path = tostring(path or ""):gsub("\\", "/")
    return string.lower(path)
end

function M.path_starts_with(path, prefix)
    local lhs = M.normalize_path(path)
    local rhs = M.normalize_path(prefix)
    return lhs:sub(1, #rhs) == rhs
end

function M.safe_item_name(item_id)
    if not item_id or item_id <= 0 or not Items or not Items.exists(item_id) then
        return ""
    end
    return Items.getName(item_id) or ""
end

function M.safe_item_info(item_id)
    if not item_id or item_id <= 0 or not Items or not Items.exists(item_id) then
        return nil
    end
    return Items.getInfo(item_id)
end

function M.safe_image(item_id)
    if not item_id or item_id <= 0 or not Items or not Items.exists(item_id) then
        return nil
    end
    item_id = tonumber(item_id or 0) or 0
    if item_id <= 0 then
        return nil
    end
    if not M._image_cache[item_id] then
        M._image_cache[item_id] = Image.fromItemSprite(item_id)
    end
    return M._image_cache[item_id]
end

function M.active_raw_brush_id()
    local brush = app and app.brush or nil
    if not brush or brush.type ~= "raw" then
        return 0
    end

    local brush_name = tostring(brush.name or "")
    local parsed = tonumber(brush_name:match("^(%d+)"))
    if parsed and parsed > 0 then
        return parsed
    end

    local fallback = tonumber(brush.id or 0) or 0
    if fallback > 0 and Items and Items.exists and Items.exists(fallback) then
        return fallback
    end

    return 0
end

function M.log_warning(message)
    print("[Border Studio] Warning: " .. tostring(message))
end

function M.border_status(border)
    if border and border.status then
        return border.status
    end
    return M.assigned_slot_count(border) == #M.slot_order and "complete" or "partial"
end

function M.assignment_snapshot(border)
    if not border then
        return "no draft"
    end

    local parts = {}
    for _, key in ipairs(M.slot_order) do
        local item_id = M.slot_item_id(border, key)
        if item_id > 0 then
            table.insert(parts, string.format("%s=%d", key, item_id))
        else
            table.insert(parts, string.format("%s=-", key))
        end
    end
    return table.concat(parts, ", ")
end

function M.assignment_preview(border, limit)
    if not border then
        return "no draft"
    end

    limit = tonumber(limit or 6) or 6
    local parts = {}
    for _, key in ipairs(M.slot_order) do
        local item_id = M.slot_item_id(border, key)
        if item_id > 0 then
            table.insert(parts, string.format("%s=%d", key, item_id))
        end
        if #parts >= limit then
            break
        end
    end

    if #parts == 0 then
        return "no assigned slots"
    end

    if M.assigned_slot_count(border) > #parts then
        table.insert(parts, "...")
    end
    return table.concat(parts, ", ")
end

function M.slot_matrix_lines(border, selected_raw_id)
    local lines = {}
    local cells = {}

    for index = 1, M.slot_layout_count do
        local key = M.slot_layout[index]
        local text = "     "
        if key == "c" then
            local item_id = tonumber(selected_raw_id or 0) or 0
            text = item_id > 0 and string.format("C:%-3d", item_id) or "C:---"
        elseif key then
            local item_id = M.slot_item_id(border, key)
            local short = (M.slot_meta[key] and M.slot_meta[key].short) or tostring(key)
            if #short > 3 then
                short = short:sub(1, 3)
            end
            text = item_id > 0 and string.format("%s:%-3d", short, item_id) or string.format("%s:---", short)
        end
        table.insert(cells, text)

        if index % 5 == 0 then
            table.insert(lines, table.concat(cells, "  "))
            cells = {}
        end
    end

    return lines
end

function M.slot_matrix_text(border, selected_raw_id)
    return table.concat(M.slot_matrix_lines(border, selected_raw_id), "\n")
end

function M.slot_layout_texts(border, selected_raw_id)
    local texts = {}

    for index = 1, M.slot_layout_count do
        local key = M.slot_layout[index]
        if key == nil then
            table.insert(texts, "")
        else
            local short = (M.slot_meta[key] and M.slot_meta[key].short) or tostring(key)
            table.insert(texts, short)
        end
    end

    return texts
end

function M.current_step(state)
    if not state.draft or state.draft:isEmpty() then
        return 1
    end
    local raw_id = tonumber(state.selected_raw_id or 0) or 0
    local active_raw = M.active_raw_brush_id()
    if active_raw > 0 then
        raw_id = active_raw
    end
    if raw_id <= 0 then
        return 2
    end
    if state.dirty then
        return 3
    end
    if state.draft:hasRequiredDataForSave() then
        return 5
    end
    return 4
end

return M
