local Common = dofile("common.lua")
local WallDraft = dofile("wall_draft.lua")

local M = {}

local function parse_attributes(text)
    local attrs = {}
    for key, value in tostring(text or ""):gmatch('([%w_%-]+)%s*=%s*"([^"]-)"') do
        attrs[key] = Common.unescape_xml(value)
    end
    return attrs
end

local function extract_brush_blocks(xml)
    local blocks = {}
    local cursor = 1

    while true do
        local start_index = xml:find("<brush%s", cursor)
        if not start_index then
            break
        end

        local tag_end = xml:find(">", start_index, true)
        if not tag_end then
            break
        end

        local open_tag = xml:sub(start_index, tag_end)
        local self_closing = open_tag:match("/%s*>$") ~= nil
        if self_closing then
            table.insert(blocks, {
                attributes = parse_attributes(open_tag),
                inner = "",
            })
            cursor = tag_end + 1
        else
            local close_start, close_end = xml:find("</brush>", tag_end + 1, true)
            if not close_start then
                break
            end
            table.insert(blocks, {
                attributes = parse_attributes(open_tag),
                inner = xml:sub(tag_end + 1, close_start - 1),
            })
            cursor = close_end + 1
        end
    end

    return blocks
end

local function extract_wall_blocks(inner)
    local blocks = {}
    local cursor = 1
    while true do
        local start_index = tostring(inner or ""):find("<wall%s", cursor)
        if not start_index then
            break
        end
        local tag_end = tostring(inner or ""):find(">", start_index, true)
        if not tag_end then
            break
        end
        local open_tag = tostring(inner or ""):sub(start_index, tag_end)
        local close_start, close_end = tostring(inner or ""):find("</wall>", tag_end + 1, true)
        if not close_start then
            break
        end
        table.insert(blocks, {
            attributes = parse_attributes(open_tag),
            inner = tostring(inner or ""):sub(tag_end + 1, close_start - 1),
        })
        cursor = close_end + 1
    end
    return blocks
end

function M.parse_walls(xml)
    local repository = {
        ordered = {},
        by_name = {},
        warnings = {},
    }

    for _, block in ipairs(extract_brush_blocks(tostring(xml or ""))) do
        local attrs = block.attributes or {}
        if string.lower(tostring(attrs.type or "")) == "wall" then
            local name = Common.trim(attrs.name or "")
            if name == "" then
                table.insert(repository.warnings, "Skipped wall brush without name.")
            else
                local alignments = {}
                local has_supported_wall = false

                for _, alignment in ipairs(Common.alignment_order) do
                    alignments[alignment.token] = Common.make_bucket(alignment.token)
                end

                local found_unsupported = false
                for tag in tostring(block.inner or ""):gmatch("<%s*([%w_%-]+)") do
                    local lower = string.lower(tag)
                    if lower ~= "wall" and lower ~= "friend" and lower ~= "!--" then
                        found_unsupported = true
                    end
                end

                for _, wall_block in ipairs(extract_wall_blocks(block.inner)) do
                    local token = Common.trim((wall_block.attributes or {}).type or "")
                    if token ~= "" then
                        local normalized = token == "corner" and "corner" or token
                        local bucket = alignments[normalized]
                        if bucket then
                            has_supported_wall = true
                            for item_attrs in tostring(wall_block.inner or ""):gmatch("<item%s+(.-)%s*/>") do
                                local parsed = parse_attributes(item_attrs)
                                local item_id = tonumber(parsed.id or 0) or 0
                                if item_id > 0 then
                                    table.insert(bucket.wallItems, {
                                        itemId = item_id,
                                        chance = tonumber(parsed.chance or 1) or 1,
                                        name = Common.safe_item_name(item_id),
                                        lookId = (Common.safe_item_info(item_id) or {}).clientId or 0,
                                    })
                                end
                            end

                            for door_attrs in tostring(wall_block.inner or ""):gmatch("<door%s+(.-)%s*/>") do
                                local parsed = parse_attributes(door_attrs)
                                local item_id = tonumber(parsed.id or 0) or 0
                                if item_id > 0 then
                                    table.insert(bucket.doorItems, {
                                        itemId = item_id,
                                        doorType = Common.trim(parsed.type or "normal"),
                                        isOpen = Common.normalize_bool(parsed.open, parsed.type == "window" or parsed.type == "hatch_window"),
                                        isLocked = Common.normalize_bool(parsed.locked, false),
                                        hate = Common.normalize_bool(parsed.hate, false),
                                        name = Common.safe_item_name(item_id),
                                        lookId = (Common.safe_item_info(item_id) or {}).clientId or 0,
                                    })
                                end
                            end
                        else
                            table.insert(repository.warnings, string.format("Wall '%s' uses unsupported alignment '%s'.", name, token))
                        end
                    end
                end

                if found_unsupported and not has_supported_wall then
                    table.insert(repository.warnings, string.format("Skipped wall brush '%s' because it does not use <wall> buckets.", name))
                elseif has_supported_wall then
                    local friends = {}
                    local redirect_name = ""
                    for friend_attrs in tostring(block.inner or ""):gmatch("<friend%s+(.-)%s*/>") do
                        local parsed = parse_attributes(friend_attrs)
                        local friend_name = Common.trim(parsed.name or "")
                        if friend_name ~= "" then
                            table.insert(friends, friend_name)
                            if Common.normalize_bool(parsed.redirect, false) and redirect_name == "" then
                                redirect_name = friend_name
                            end
                        elseif parsed.id then
                            table.insert(repository.warnings, string.format("Wall '%s' uses <friend id=...>, but loader expects name=...", name))
                        end
                    end

                    local draft = WallDraft.new({
                        name = name,
                        lookId = attrs.lookid,
                        serverLookId = attrs.server_lookid,
                        isNew = false,
                        sourceName = name,
                        friends = friends,
                        redirectName = redirect_name,
                        alignments = alignments,
                    })
                    repository.by_name[name] = draft
                    table.insert(repository.ordered, draft)
                end
            end
        end
    end

    table.sort(repository.ordered, function(a, b)
        return string.lower(a.name or "") < string.lower(b.name or "")
    end)

    return repository
end

function M.serialize_wall(draft)
    local attrs = {
        string.format('name="%s"', Common.escape_xml(draft.name or "")),
        'type="wall"',
    }
    if tonumber(draft.lookId or 0) > 0 then
        table.insert(attrs, string.format('lookid="%d"', tonumber(draft.lookId or 0) or 0))
    end
    if tonumber(draft.serverLookId or 0) > 0 then
        table.insert(attrs, string.format('server_lookid="%d"', tonumber(draft.serverLookId or 0) or 0))
    end

    local lines = {
        string.format("<brush %s>", table.concat(attrs, " ")),
    }

    for _, alignment in ipairs(Common.alignment_order) do
        local bucket = draft.alignments[alignment.token]
        if bucket and (#(bucket.wallItems or {}) > 0 or #(bucket.doorItems or {}) > 0) then
            table.insert(lines, string.format('\t<wall type="%s">', Common.escape_xml(alignment.token)))
            for _, item in ipairs(bucket.wallItems or {}) do
                table.insert(lines, string.format('\t\t<item id="%d" chance="%d"/>', tonumber(item.itemId or 0) or 0, tonumber(item.chance or 0) or 0))
            end
            for _, door in ipairs(bucket.doorItems or {}) do
                local door_attrs = {
                    string.format('id="%d"', tonumber(door.itemId or 0) or 0),
                    string.format('type="%s"', Common.escape_xml(door.doorType or "normal")),
                    string.format('open="%s"', door.isOpen and "true" or "false"),
                }
                if door.isLocked then
                    table.insert(door_attrs, 'locked="true"')
                end
                if door.hate then
                    table.insert(door_attrs, 'hate="true"')
                end
                table.insert(lines, string.format('\t\t<door %s/>', table.concat(door_attrs, " ")))
            end
            table.insert(lines, "\t</wall>")
        end
    end

    local emitted_redirect = false
    local seen = {}
    for _, friend_name in ipairs(draft.friends or {}) do
        local trimmed = Common.trim(friend_name)
        if trimmed ~= "" and not seen[string.lower(trimmed)] then
            seen[string.lower(trimmed)] = true
            local attrs_line = string.format('name="%s"', Common.escape_xml(trimmed))
            if draft.redirectName ~= "" and trimmed == draft.redirectName then
                attrs_line = attrs_line .. ' redirect="true"'
                emitted_redirect = true
            end
            table.insert(lines, string.format('\t<friend %s/>', attrs_line))
        end
    end

    if draft.redirectName ~= "" and not emitted_redirect then
        table.insert(lines, string.format('\t<friend name="%s" redirect="true"/>', Common.escape_xml(draft.redirectName)))
    end

    table.insert(lines, "</brush>")
    return table.concat(lines, "\n")
end

return M
