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

function M.normalize_path(path)
    path = tostring(path or ""):gsub("\\", "/")
    return string.lower(path)
end

function M.path_starts_with(path, prefix)
    local lhs = M.normalize_path(path)
    local rhs = M.normalize_path(prefix)
    return lhs:sub(1, #rhs) == rhs
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

function M.ground_status(draft)
    if not draft or draft:isEmpty() then
        return "empty"
    end
    if #draft.variants > 0 then
        return "variants"
    end
    return "basic"
end

function M.format_ground_title(draft)
    if not draft then
        return "No ground"
    end
    local suffix = draft.isNew and " (new)" or ""
    return string.format("%s%s", draft.name ~= "" and draft.name or "Untitled ground", suffix)
end

function M.primary_item_id(draft)
    if not draft then
        return 0
    end
    if draft.mainGroundItem and draft.mainGroundItem.itemId then
        return draft.mainGroundItem.itemId
    end
    if draft.mainItem and draft.mainItem.itemId then
        return draft.mainItem.itemId
    end
    if draft.variants and draft.variants[1] and draft.variants[1].itemId then
        return draft.variants[1].itemId
    end
    return 0
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

function M.variant_list_items(draft)
    local items = {}
    for index, variant in ipairs(draft and draft.variants or {}) do
        local name = variant.name or ("Item " .. tostring(variant.itemId or 0))
        table.insert(items, {
            text = string.format("%d. %s  |  chance %d", index, name, tonumber(variant.chance or 0) or 0),
            tooltip = string.format("Item ID %d\nChance %d", tonumber(variant.itemId or 0) or 0, tonumber(variant.chance or 0) or 0),
            image = M.safe_image(variant.itemId or 0),
        })
    end
    return items
end

function M.variant_preview_items(draft)
    local items = {}
    local variants = draft and draft.variants or {}
    local preview_limit = math.min(#variants, 6)
    for index = 1, preview_limit do
        local variant = variants[index]
        local chance = tonumber(variant.chance or 0) or 0
        table.insert(items, {
            text = string.format("chance %d", chance),
            tooltip = string.format("%s\nItem %d\nChance %d", variant.name or "Variant", tonumber(variant.itemId or 0) or 0, chance),
            image = M.safe_image(variant.itemId or 0),
        })
    end
    if #variants > preview_limit then
        table.insert(items, {
            text = string.format("+%d", #variants - preview_limit),
            tooltip = string.format("Pozostale warianty: %d", #variants - preview_limit),
            image = nil,
        })
    end
    return items
end

function M.surface_preview_items(draft)
    local items = {}
    local main_id = draft and draft.mainGroundItem and draft.mainGroundItem.itemId or 0
    if not draft or main_id <= 0 then
        for _ = 1, 25 do
            table.insert(items, {
                text = "",
                tooltip = "No main ground selected",
                image = nil,
            })
        end
        return items
    end

    local weighted_pool = {}
    for _, entry in ipairs(draft:getAllItems() or {}) do
        local item_id = tonumber(entry.itemId or 0) or 0
        if item_id > 0 then
            table.insert(weighted_pool, {
                itemId = item_id,
                chance = math.max(1, tonumber(entry.chance or 0) or 0),
                name = entry.name or ("Item " .. tostring(item_id)),
            })
        end
    end

    if #weighted_pool == 0 then
        table.insert(weighted_pool, {
            itemId = main_id,
            chance = 1,
            name = M.safe_item_name(main_id),
        })
    end

    local total_weight = 0
    for _, entry in ipairs(weighted_pool) do
        total_weight = total_weight + entry.chance
    end

    for index = 1, 25 do
        local chosen = weighted_pool[1]
        if draft.randomize then
            local pick = (((index - 1) * 137) + (main_id * 17)) % total_weight + 1
            local acc = 0
            for _, entry in ipairs(weighted_pool) do
                acc = acc + entry.chance
                if pick <= acc then
                    chosen = entry
                    break
                end
            end
        end

        if index == 13 then
            chosen = {
                itemId = main_id,
                chance = tonumber(draft.mainChance or 0) or 0,
                name = (draft.mainGroundItem and draft.mainGroundItem.name) or M.safe_item_name(main_id),
            }
        end

        table.insert(items, {
            text = "",
            tooltip = string.format("%s\nItem %d\nChance %d", chosen.name or "Ground", chosen.itemId or 0, chosen.chance or 0),
            image = M.safe_image(chosen.itemId or 0),
        })
    end

    return items
end

function M.ground_set_items(draft)
    local items = {}
    if not draft then
        return items
    end

    local main = draft.mainGroundItem
    if main and tonumber(main.itemId or 0) > 0 then
        table.insert(items, {
            text = string.format("Main  |  %s  |  chance %d", main.name or ("Item " .. tostring(main.itemId or 0)), tonumber(draft.mainChance or 0) or 0),
            tooltip = string.format("Main Ground\nItem %d\nChance %d", tonumber(main.itemId or 0) or 0, tonumber(draft.mainChance or 0) or 0),
            image = M.safe_image(main.itemId or 0),
        })
    end

    for index, variant in ipairs(draft.variants or {}) do
        local chance = tonumber(variant.chance or 0) or 0
        table.insert(items, {
            text = string.format("Var %d  |  %s  |  chance %d", index, variant.name or ("Item " .. tostring(variant.itemId or 0)), chance),
            tooltip = string.format("Variant %d\nItem %d\nChance %d", index, tonumber(variant.itemId or 0) or 0, chance),
            image = M.safe_image(variant.itemId or 0),
        })
    end

    return items
end

function M.current_step(state)
    if not state.draft then
        return 1
    end

    if not state.draft:hasMainItem() then
        return 2
    end

    if #state.draft.variants == 0 then
        return 4
    end

    if not state.draft:hasRequiredDataForSave() then
        return 5
    end

    return 6
end

return M
