local Common = dofile("common.lua")
local GroundDraft = dofile("ground_draft.lua")

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

local function normalize_extra_children(inner)
    local extras = tostring(inner or "")
    extras = extras:gsub("<item%s+.-%s*/>", "")
    extras = extras:gsub("\r\n", "\n")
    extras = extras:gsub("^\n+", "")
    extras = extras:gsub("\n+$", "")
    return extras
end

local function collect_unsupported_child_warnings(inner, name, warnings)
    local seen = {}
    for tag in tostring(inner or ""):gmatch("<%s*([%w_%-]+)") do
        local lower = string.lower(tag)
        if lower ~= "item" and lower ~= "!--" and not seen[lower] then
            seen[lower] = true
            table.insert(warnings, string.format("Ground '%s' contains unsupported child <%s>. It was preserved but not edited by Ground Studio.", name, tag))
        end
    end
end

function M.parse_grounds(xml)
    local repository = {
        ordered = {},
        by_name = {},
        warnings = {},
    }

    for _, block in ipairs(extract_brush_blocks(tostring(xml or ""))) do
        local attrs = block.attributes or {}
        if string.lower(tostring(attrs.type or "")) == "ground" then
            local name = Common.trim(attrs.name or "")
            if name == "" then
                table.insert(repository.warnings, "Skipped ground brush without name.")
            else
                local items = {}
                for item_attrs in tostring(block.inner or ""):gmatch("<item%s+(.-)%s*/>") do
                    local parsed = parse_attributes(item_attrs)
                    local item_id = tonumber(parsed.id or 0) or 0
                    local chance = tonumber(parsed.chance or 1) or 1
                    if item_id > 0 then
                        table.insert(items, { itemId = item_id, chance = chance })
                    else
                        table.insert(repository.warnings, "Skipped ground item with invalid id in " .. name)
                    end
                end

                local main_item = items[1]
                local variants = {}
                for index = 2, #items do
                    table.insert(variants, items[index])
                end

                local draft = GroundDraft.new({
                    name = name,
                    mainItem = main_item,
                    mainChance = main_item and main_item.chance or 1,
                    variants = variants,
                    lookId = attrs.lookid,
                    serverLookId = attrs.server_lookid,
                    zOrder = attrs["z-order"],
                    randomize = attrs.randomize,
                    soloOptional = attrs.solo_optional,
                    extraChildrenXml = normalize_extra_children(block.inner),
                    isNew = false,
                    sourceName = name,
                })

                collect_unsupported_child_warnings(block.inner, name, repository.warnings)

                repository.by_name[name] = draft
                table.insert(repository.ordered, draft)
            end
        end
    end

    table.sort(repository.ordered, function(a, b)
        return string.lower(a.name or "") < string.lower(b.name or "")
    end)

    return repository
end

function M.serialize_ground(draft)
    local data = draft:toXmlGroundData()
    local attrs = {
        string.format('name="%s"', Common.escape_xml(data.name)),
        'type="ground"',
    }

    if tonumber(data.lookId or 0) > 0 then
        table.insert(attrs, string.format('lookid="%d"', tonumber(data.lookId or 0) or 0))
    end
    if tonumber(data.serverLookId or 0) > 0 then
        table.insert(attrs, string.format('server_lookid="%d"', tonumber(data.serverLookId or 0) or 0))
    end
    if tonumber(data.zOrder or 0) ~= 0 then
        table.insert(attrs, string.format('z-order="%d"', tonumber(data.zOrder or 0) or 0))
    end
    table.insert(attrs, string.format('randomize="%s"', data.randomize and "true" or "false"))
    if data.soloOptional then
        table.insert(attrs, 'solo_optional="true"')
    end

    local lines = {
        string.format("<brush %s>", table.concat(attrs, " ")),
    }

    for _, item in ipairs(data.items or {}) do
        table.insert(lines, string.format('\t<item id="%d" chance="%d"/>', tonumber(item.itemId or 0) or 0, tonumber(item.chance or 0) or 0))
    end

    local extras = Common.trim(data.extraChildrenXml or "")
    if extras ~= "" then
        for line in extras:gmatch("[^\r\n]+") do
            local trimmed = Common.trim(line)
            if trimmed ~= "" then
                table.insert(lines, "\t" .. trimmed)
            end
        end
    end

    table.insert(lines, "</brush>")
    return table.concat(lines, "\n")
end

return M
