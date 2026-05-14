local M = {}
M._image_cache = M._image_cache or {}

M.palette = {
    header = "#1F2937",
    panel = "#243447",
    panel_alt = "#2C3E50",
    accent = "#0F766E",
    accent_soft = "#14B8A6",
    success = "#2F855A",
    danger = "#C53030",
    text = "#F8FAFC",
    muted = "#CBD5E1",
    subtle = "#94A3B8",
    border = "#475569",
}

M.alignment_order = {
    { token = "vertical", label = "Vertical" },
    { token = "horizontal", label = "Horizontal" },
    { token = "corner", label = "Corner" },
    { token = "pole", label = "Pole" },
    { token = "south end", label = "South End" },
    { token = "east end", label = "East End" },
    { token = "north end", label = "North End" },
    { token = "west end", label = "West End" },
    { token = "south T", label = "South T" },
    { token = "east T", label = "East T" },
    { token = "west T", label = "West T" },
    { token = "north T", label = "North T" },
    { token = "northeast diagonal", label = "NE Diagonal" },
    { token = "northwest diagonal", label = "NW Diagonal" },
    { token = "southeast diagonal", label = "SE Diagonal" },
    { token = "southwest diagonal", label = "SW Diagonal" },
    { token = "intersection", label = "Intersection" },
    { token = "untouchable", label = "Untouchable" },
}

M.alignment_lookup = {}
for _, alignment in ipairs(M.alignment_order) do
    M.alignment_lookup[alignment.token] = alignment
end

M.door_type_options = {
    "normal",
    "normal_alt",
    "locked",
    "quest",
    "magic",
    "archway",
    "window",
    "hatch_window",
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

function M.trim(value)
    return tostring(value or ""):match("^%s*(.-)%s*$")
end

function M.normalize_bool(value, default)
    if value == nil then
        return default and true or false
    end
    if type(value) == "boolean" then
        return value
    end

    local text = string.lower(tostring(value))
    return text == "1" or text == "true" or text == "yes" or text == "on"
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
    item_id = tonumber(item_id or 0) or 0
    if item_id <= 0 or not Items or not Items.exists(item_id) then
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

    local parsed = tonumber(tostring(brush.name or ""):match("^(%d+)"))
    if parsed and parsed > 0 then
        return parsed
    end

    local fallback = tonumber(brush.id or 0) or 0
    if fallback > 0 and Items and Items.exists and Items.exists(fallback) then
        return fallback
    end

    return 0
end

function M.escape_xml(value)
    value = tostring(value or "")
    value = value:gsub("&", "&amp;")
    value = value:gsub("<", "&lt;")
    value = value:gsub(">", "&gt;")
    value = value:gsub('"', "&quot;")
    value = value:gsub("'", "&apos;")
    return value
end

function M.unescape_xml(value)
    value = tostring(value or "")
    value = value:gsub("&apos;", "'")
    value = value:gsub("&quot;", '"')
    value = value:gsub("&gt;", ">")
    value = value:gsub("&lt;", "<")
    value = value:gsub("&amp;", "&")
    return value
end

function M.alignment_label(token)
    local alignment = M.alignment_lookup[token]
    return alignment and alignment.label or token
end

function M.make_bucket(token)
    return {
        token = token,
        wallItems = {},
        doorItems = {},
    }
end

function M.representative_item_id(bucket)
    if not bucket then
        return 0
    end
    if bucket.wallItems and bucket.wallItems[1] then
        return tonumber(bucket.wallItems[1].itemId or 0) or 0
    end
    if bucket.doorItems and bucket.doorItems[1] then
        return tonumber(bucket.doorItems[1].itemId or 0) or 0
    end
    return 0
end

function M.wall_status(draft)
    if not draft then
        return "empty"
    end
    local filled = 0
    local door_count = 0
    for _, alignment in ipairs(M.alignment_order) do
        local bucket = draft.alignments and draft.alignments[alignment.token] or nil
        if bucket and (#(bucket.wallItems or {}) > 0 or #(bucket.doorItems or {}) > 0) then
            filled = filled + 1
            door_count = door_count + #(bucket.doorItems or {})
        end
    end
    if filled == 0 then
        return "empty"
    end
    if door_count > 0 then
        return string.format("%d buckets | doors", filled)
    end
    return string.format("%d buckets", filled)
end

function M.format_wall_title(draft)
    if not draft then
        return "No wall"
    end
    local suffix = draft.isNew and " (new)" or ""
    return string.format("%s%s", draft.name ~= "" and draft.name or "Untitled wall", suffix)
end

function M.library_items(repository, filter_text)
    local items = {}
    local visible_names = {}
    local needle = string.lower(filter_text or "")

    for _, draft in ipairs(repository.ordered or {}) do
        local preview_id = draft:getPreviewItemId()
        local label_blob = string.lower((draft.name or "") .. " " .. M.wall_status(draft))
        if needle == "" or string.find(label_blob, needle, 1, true) then
            table.insert(items, {
                text = string.format("%s  |  %s", draft.name or "Unnamed", M.wall_status(draft)),
                tooltip = string.format("%s\nStatus: %s\nPreview item: %d", draft.name or "Unnamed", M.wall_status(draft), preview_id),
                image = M.safe_image(preview_id),
            })
            table.insert(visible_names, draft.name)
        end
    end

    return items, visible_names
end

function M.recent_raw_items(recent_raw_ids)
    local items = {}
    for _, item_id in ipairs(recent_raw_ids or {}) do
        item_id = tonumber(item_id or 0) or 0
        if item_id > 0 then
            local name = M.safe_item_name(item_id)
            table.insert(items, {
                text = string.format("%d  |  %s", item_id, name ~= "" and name or ("Item " .. tostring(item_id))),
                tooltip = string.format("Item ID %d", item_id),
                image = M.safe_image(item_id),
            })
        end
    end
    return items
end

function M.alignment_grid_items(draft)
    local items = {}
    for _, alignment in ipairs(M.alignment_order) do
        local bucket = draft and draft.alignments and draft.alignments[alignment.token] or nil
        local item_count = bucket and #(bucket.wallItems or {}) or 0
        local door_count = bucket and #(bucket.doorItems or {}) or 0
        table.insert(items, {
            text = alignment.label,
            tooltip = string.format("%s\nWall items: %d\nDoors: %d", alignment.label, item_count, door_count),
            image = M.safe_image(M.representative_item_id(bucket)),
        })
    end
    return items
end

function M.bucket_wall_list_items(bucket)
    local items = {}
    for index, item in ipairs(bucket and bucket.wallItems or {}) do
        table.insert(items, {
            text = string.format("%d. Item %d  |  chance %d", index, tonumber(item.itemId or 0) or 0, tonumber(item.chance or 0) or 0),
            tooltip = string.format("Item ID %d\nChance %d", tonumber(item.itemId or 0) or 0, tonumber(item.chance or 0) or 0),
            image = M.safe_image(item.itemId or 0),
        })
    end
    return items
end

function M.bucket_door_list_items(bucket)
    local items = {}
    for index, door in ipairs(bucket and bucket.doorItems or {}) do
        local bits = {
            door.doorType or "normal",
            door.isOpen and "open" or "closed",
        }
        if door.isLocked then
            table.insert(bits, "locked")
        end
        if door.hate then
            table.insert(bits, "hate")
        end
        table.insert(items, {
            text = string.format("%d. Item %d  |  %s", index, tonumber(door.itemId or 0) or 0, table.concat(bits, " / ")),
            tooltip = string.format("Item ID %d\nType %s", tonumber(door.itemId or 0) or 0, door.doorType or "normal"),
            image = M.safe_image(door.itemId or 0),
        })
    end
    return items
end

function M.friend_summary(draft)
    local friends = draft and draft.friends or {}
    if #friends == 0 then
        return "Brak friend links."
    end
    return "Friends: " .. table.concat(friends, ", ")
end

return M
