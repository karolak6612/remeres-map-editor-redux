local Paths = dofile("module_paths.lua")
local GroundCommon = Paths.load("ground_studio/common.lua")
local GroundDraft = Paths.load("ground_studio/ground_draft.lua")
local GroundRepository = Paths.load("ground_studio/repository.lua")
local BorderRepository = Paths.load("border_studio/border_repository.lua")
local BorderCommon = Paths.load("border_studio/common.lua")
local ComposerDraft = dofile("composer_draft.lua")

local M = {}

local function parse_attributes(text)
    local attrs = {}
    for key, value in tostring(text or ""):gmatch('([%w_%-]+)%s*=%s*"([^"]-)"') do
        attrs[key] = GroundCommon.unescape_xml(value)
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
    extras = extras:gsub("<border%s+.-%s*/>", "")
    extras = extras:gsub("<friend%s+.-%s*/>", "")
    extras = extras:gsub("\r\n", "\n")
    extras = extras:gsub("^\n+", "")
    extras = extras:gsub("\n+$", "")
    return extras
end

local function collect_unsupported_child_warnings(inner, name, warnings)
    local seen = {}
    for tag in tostring(inner or ""):gmatch("<%s*([%w_%-]+)") do
        local lower = string.lower(tag)
        if lower ~= "item" and lower ~= "border" and lower ~= "friend" and lower ~= "!--" and not seen[lower] then
            seen[lower] = true
            table.insert(warnings, string.format("Composer brush '%s' contains unsupported child <%s>. It was preserved but not edited by Composer.", name, tag))
        end
    end
end

local function read_file(path)
    local store = app.storage(path)
    if not store or not store.readText then
        return nil, "This RME build does not expose raw file reads for grounds.xml."
    end
    local content, err = store:readText()
    if not content then
        return nil, err or "Could not open file."
    end
    return content
end

local function write_file(path, content)
    local store = app.storage(path)
    if not store or not store.save then
        return false, "This RME build does not expose safe file writes."
    end
    local ok = store:save(content or "")
    if not ok then
        return false, "Could not write file."
    end
    return true
end

function M.resolve_target_path(saved_path)
    return GroundRepository.resolve_target_path(saved_path)
end

local function make_ground_reference(name, draft, embedded)
    if not draft then
        return nil
    end
    return {
        name = GroundCommon.trim(name ~= "" and name or draft.name or "ground"),
        title = draft.name ~= "" and draft.name or "ground",
        sampleItemId = GroundCommon.primary_item_id(draft),
        draft = draft:clone(),
        embedded = embedded == true,
    }
end

local function make_border_reference(repository, border_id)
    border_id = tonumber(border_id or 0) or 0
    if border_id <= 0 then
        return nil
    end

    local source = BorderRepository.find_border_source(repository, border_id)
    local draft = repository.by_id and repository.by_id[border_id] and BorderRepository.copy(repository.by_id[border_id]) or nil
    return {
        borderId = border_id,
        title = source and source.title or ("Border #" .. tostring(border_id)),
        name = source and source.name or ("Border #" .. tostring(border_id)),
        sampleItemId = source and source.sampleItemId or (draft and BorderCommon.primary_item_id(draft) or 0),
        draft = draft,
    }
end

local function same_items(left, right)
    local left_data = left and left:toXmlGroundData() or nil
    local right_data = right and right:toXmlGroundData() or nil
    local left_items = left_data and left_data.items or {}
    local right_items = right_data and right_data.items or {}
    if #left_items ~= #right_items then
        return false
    end
    for index = 1, #left_items do
        if tonumber(left_items[index].itemId or 0) ~= tonumber(right_items[index].itemId or 0) then
            return false
        end
        if tonumber(left_items[index].chance or 0) ~= tonumber(right_items[index].chance or 0) then
            return false
        end
    end
    return true
end

local function find_matching_ground(ground_repository, draft)
    for _, candidate in ipairs((ground_repository and ground_repository.ordered) or {}) do
        if same_items(candidate, draft)
            and tonumber(candidate.lookId or 0) == tonumber(draft.lookId or 0)
            and tonumber(candidate.serverLookId or 0) == tonumber(draft.serverLookId or 0)
            and tonumber(candidate.zOrder or 0) == tonumber(draft.zOrder or 0)
            and candidate.randomize == draft.randomize
            and candidate.soloOptional == draft.soloOptional then
            return make_ground_reference(candidate.name, candidate, false)
        end
    end
    return nil
end

function M.read(target_path)
    local ground_repository = GroundRepository.read(target_path, {})
    local border_repository = BorderRepository.read({}, app.borders or {})
    local repository = {
        target_path = target_path,
        ground_repository = ground_repository,
        border_repository = border_repository,
        composed_ordered = {},
        composed_by_name = {},
        warnings = {},
        api_missing = nil,
    }

    local content, err = read_file(target_path)
    if not content then
        repository.api_missing = err
        table.insert(repository.warnings, err)
        return repository
    end

    for _, block in ipairs(extract_brush_blocks(tostring(content or ""))) do
        local attrs = block.attributes or {}
        if string.lower(tostring(attrs.type or "")) == "ground" then
            local name = GroundCommon.trim(attrs.name or "")
            if name ~= "" then
                local items = {}
                local borders = {}
                local friends = {}

                for item_attrs in tostring(block.inner or ""):gmatch("<item%s+(.-)%s*/>") do
                    local parsed = parse_attributes(item_attrs)
                    local item_id = tonumber(parsed.id or 0) or 0
                    local chance = tonumber(parsed.chance or 1) or 1
                    if item_id > 0 then
                        table.insert(items, { itemId = item_id, chance = chance })
                    end
                end

                for border_attrs in tostring(block.inner or ""):gmatch("<border%s+(.-)%s*/>") do
                    local parsed = parse_attributes(border_attrs)
                    table.insert(borders, {
                        id = tonumber(parsed.id or 0) or 0,
                        align = GroundCommon.trim(parsed.align or ""),
                        toBrush = GroundCommon.trim(parsed.to or ""),
                    })
                end

                for friend_attrs in tostring(block.inner or ""):gmatch("<friend%s+(.-)%s*/>") do
                    local parsed = parse_attributes(friend_attrs)
                    local friend_name = GroundCommon.trim(parsed.name or "")
                    if friend_name ~= "" then
                        table.insert(friends, friend_name)
                    end
                end

                if #borders > 0 or #friends > 0 then
                    local main_item = items[1]
                    local variants = {}
                    for index = 2, #items do
                        table.insert(variants, items[index])
                    end

                    local parsed_ground = GroundDraft.new({
                        name = name,
                        mainItem = main_item,
                        mainChance = main_item and main_item.chance or 1,
                        variants = variants,
                        lookId = attrs.lookid,
                        serverLookId = attrs.server_lookid,
                        zOrder = attrs["z-order"],
                        randomize = attrs.randomize,
                        soloOptional = attrs.solo_optional,
                        isNew = false,
                    })

                    local border_ref = make_border_reference(border_repository, borders[1] and borders[1].id or 0)
                    if not border_ref then
                        table.insert(repository.warnings, string.format("Composer brush '%s' references missing border id %s.", name, tostring(borders[1] and borders[1].id or 0)))
                    end

                    local ground_ref = find_matching_ground(ground_repository, parsed_ground)
                    if not ground_ref then
                        ground_ref = make_ground_reference(name, parsed_ground, true)
                        table.insert(repository.warnings, string.format("Composer brush '%s' did not match an existing ground source. Loaded as embedded ground data.", name))
                    end

                    local composer = ComposerDraft.new({
                        name = name,
                        selectedGround = ground_ref,
                        selectedBorder = border_ref,
                        toBrush = borders[1] and borders[1].toBrush or "",
                        align = borders[1] and borders[1].align or "outer",
                        friends = friends,
                        isNew = false,
                        sourceName = name,
                        extraChildrenXml = normalize_extra_children(block.inner),
                    })
                    collect_unsupported_child_warnings(block.inner, name, repository.warnings)

                    repository.composed_by_name[name] = composer
                    table.insert(repository.composed_ordered, composer)
                end
            end
        end
    end

    table.sort(repository.composed_ordered, function(left, right)
        return string.lower(left.name or "") < string.lower(right.name or "")
    end)

    return repository
end

function M.ground_sources(repository, filter_text)
    local items = {}
    local names = {}
    local needle = string.lower(filter_text or "")
    for _, draft in ipairs((repository.ground_repository and repository.ground_repository.ordered) or {}) do
        if not (repository.composed_by_name and repository.composed_by_name[draft.name or ""]) then
        local main_id = GroundCommon.primary_item_id(draft)
        local blob = string.lower((draft.name or "") .. " " .. GroundCommon.ground_status(draft))
        if needle == "" or string.find(blob, needle, 1, true) then
            table.insert(items, {
                text = string.format("%s  |  %s", draft.name or "Unnamed", GroundCommon.ground_status(draft)),
                tooltip = string.format("%s\nMain item: %d", draft.name or "Unnamed", main_id),
                image = nil,
            })
            table.insert(names, draft.name)
        end
        end
    end
    return items, names
end

function M.border_sources(repository, filter_text)
    local items = {}
    local ids = {}
    local needle = string.lower(filter_text or "")
    for _, source in ipairs(BorderRepository.border_sources(repository.border_repository)) do
        local blob = string.lower(string.format("%s %s %d", source.name or "", source.status or "", source.borderId or 0))
        if needle == "" or string.find(blob, needle, 1, true) then
            table.insert(items, {
                text = string.format("#%d  %s  |  %s", source.borderId or 0, source.name or "Border", source.status or "partial"),
                tooltip = source.tooltip,
                image = nil,
            })
            table.insert(ids, source.borderId)
        end
    end
    return items, ids
end

function M.composed_sources(repository, filter_text)
    local items = {}
    local names = {}
    local needle = string.lower(filter_text or "")
    for _, draft in ipairs(repository.composed_ordered or {}) do
        local border_id = draft.selectedBorder and draft.selectedBorder.borderId or 0
        local ground_name = draft.selectedGround and draft.selectedGround.name or "?"
        local blob = string.lower((draft.name or "") .. " " .. ground_name .. " " .. tostring(border_id))
        if needle == "" or string.find(blob, needle, 1, true) then
            table.insert(items, {
                text = string.format("%s  |  ground: %s  |  border: #%d", draft.name or "Unnamed", ground_name, tonumber(border_id or 0) or 0),
                tooltip = string.format("%s\nGround: %s\nBorder: #%d", draft.name or "Unnamed", ground_name, tonumber(border_id or 0) or 0),
                image = nil,
            })
            table.insert(names, draft.name)
        end
    end
    return items, names
end

function M.make_ground_reference(repository, ground_name)
    local draft = repository.ground_repository and repository.ground_repository.by_name and repository.ground_repository.by_name[ground_name] or nil
    if not draft then
        return nil
    end
    return make_ground_reference(ground_name, draft, false)
end

function M.make_border_reference(repository, border_id)
    return make_border_reference(repository.border_repository, border_id)
end

function M.find_linked_border_for_ground(repository, ground_name)
    local target_name = GroundCommon.trim(ground_name or "")
    if target_name == "" then
        return nil
    end

    local exact = repository.composed_by_name and repository.composed_by_name[target_name] or nil
    if exact and exact.selectedBorder and tonumber(exact.selectedBorder.borderId or 0) > 0 then
        return GroundCommon.clone(exact.selectedBorder), exact.name
    end

    for _, draft in ipairs(repository.composed_ordered or {}) do
        local selected_ground = draft.selectedGround
        if selected_ground and GroundCommon.trim(selected_ground.name or "") == target_name then
            if draft.selectedBorder and tonumber(draft.selectedBorder.borderId or 0) > 0 then
                return GroundCommon.clone(draft.selectedBorder), draft.name
            end
        end
    end

    return nil
end

function M.load_composer(repository, name)
    local draft = repository.composed_by_name and repository.composed_by_name[name] or nil
    return draft and draft:clone() or nil
end

function M.new_draft()
    return ComposerDraft.new({
        align = "outer",
        isNew = true,
    })
end

function M.duplicate_draft(repository, draft)
    local copy = draft:clone()
    local base = GroundCommon.trim((copy.name ~= "" and copy.name or "composed brush") .. " copy")
    local candidate = base
    local index = 1
    while repository.composed_by_name and repository.composed_by_name[candidate] do
        index = index + 1
        candidate = string.format("%s %d", base, index)
    end
    copy.name = candidate
    copy.sourceName = candidate
    copy.isNew = true
    copy.dirty = true
    return copy
end

function M.validate_save(repository, draft, mode, target_path)
    if not target_path or target_path == "" then
        return false, "Target grounds.xml could not be resolved."
    end
    if not GroundCommon.path_starts_with(target_path, app.getDataDirectory()) then
        return false, "Target grounds.xml must stay inside the RME data directory."
    end
    local validation = draft:getValidation()
    if not validation.isValid then
        return false, validation.errors[1] or "Composer draft jest niepoprawny."
    end

    local current_name = GroundCommon.trim(draft.name or "")
    local source_name = GroundCommon.trim(draft.sourceName or current_name)
    local exists = repository.composed_by_name and repository.composed_by_name[current_name] ~= nil
    local source_exists = repository.composed_by_name and repository.composed_by_name[source_name] ~= nil

    if mode == "new" and exists then
        return false, string.format("Brush '%s' juz istnieje.", current_name)
    end
    if mode == "overwrite" and not source_exists then
        return false, string.format("Brush '%s' nie istnieje do nadpisania.", source_name)
    end

    return true
end

function M.backup_xml(target_path)
    local original, err = read_file(target_path)
    if not original then
        return false, nil, nil, "Could not read grounds.xml before backup: " .. tostring(err)
    end

    local backup_path = string.format("%s.composer.%s.bak", target_path, os.date("%Y%m%d_%H%M%S"))
    local ok, backup_err = write_file(backup_path, original)
    if not ok then
        return false, nil, nil, "Could not write backup file: " .. tostring(backup_err)
    end

    return true, backup_path, original
end

local function serialize_composer(draft)
    local ground_data = draft.selectedGround.draft:toXmlGroundData()
    local attrs = {
        string.format('name="%s"', GroundCommon.escape_xml(draft.name)),
        'type="ground"',
    }

    if tonumber(ground_data.lookId or 0) > 0 then
        table.insert(attrs, string.format('lookid="%d"', tonumber(ground_data.lookId or 0) or 0))
    end
    if tonumber(ground_data.serverLookId or 0) > 0 then
        table.insert(attrs, string.format('server_lookid="%d"', tonumber(ground_data.serverLookId or 0) or 0))
    end
    if tonumber(ground_data.zOrder or 0) ~= 0 then
        table.insert(attrs, string.format('z-order="%d"', tonumber(ground_data.zOrder or 0) or 0))
    end
    table.insert(attrs, string.format('randomize="%s"', ground_data.randomize and "true" or "false"))
    if ground_data.soloOptional then
        table.insert(attrs, 'solo_optional="true"')
    end

    local lines = {
        string.format("<brush %s>", table.concat(attrs, " ")),
    }

    for _, item in ipairs(ground_data.items or {}) do
        table.insert(lines, string.format('\t<item id="%d" chance="%d" />', tonumber(item.itemId or 0) or 0, tonumber(item.chance or 0) or 0))
    end

    local border_id = tonumber(draft.selectedBorder.borderId or 0) or 0
    if border_id > 0 then
        local target_to = GroundCommon.trim(draft.toBrush or "")
        local border_attrs = {
            string.format('align="%s"', GroundCommon.escape_xml(draft.align or "outer")),
            string.format('id="%d"', border_id),
        }
        if target_to ~= "" then
            table.insert(border_attrs, string.format('to="%s"', GroundCommon.escape_xml(target_to)))
        end
        table.insert(lines, string.format("\t<border %s />", table.concat(border_attrs, " ")))
    end

    for _, friend_name in ipairs(draft.friends or {}) do
        table.insert(lines, string.format('\t<friend name="%s" />', GroundCommon.escape_xml(friend_name)))
    end

    local extras = GroundCommon.trim(draft.extraChildrenXml or "")
    if extras ~= "" then
        for line in extras:gmatch("[^\r\n]+") do
            local trimmed = GroundCommon.trim(line)
            if trimmed ~= "" then
                table.insert(lines, "\t" .. trimmed)
            end
        end
    end

    table.insert(lines, "</brush>")
    return table.concat(lines, "\n")
end

function M.save_draft(repository, draft, mode, target_path)
    local ok, validation_message = M.validate_save(repository, draft, mode, target_path)
    if not ok then
        return false, validation_message
    end

    local backup_ok, backup_path, original_content, backup_message = M.backup_xml(target_path)
    if not backup_ok then
        return false, backup_message
    end

    local saved_draft = draft:clone()
    saved_draft.name = GroundCommon.trim(saved_draft.name)
    saved_draft.isNew = false

    local writer = app.storage(target_path)
    if not writer or not writer.upsertXml then
        return false, "The current RME build does not expose XML upsert support."
    end

    local match_name = mode == "overwrite" and GroundCommon.trim(saved_draft.sourceName or saved_draft.name) or saved_draft.name
    local xml = serialize_composer(saved_draft)
    local write_ok, write_mode = writer:upsertXml({
        root = "materials",
        parent = "materials",
        tag = "brush",
        match_attr = "name",
        match_value = match_name,
        xml = xml,
    })

    if not write_ok then
        if original_content then
            write_file(target_path, original_content)
        end
        return false, write_mode or "Saving failed while updating grounds.xml."
    end

    saved_draft.sourceName = saved_draft.name
    return true, {
        draft = saved_draft,
        backupPath = backup_path,
        xmlMode = write_mode or mode,
        message = string.format("Saved composed brush '%s' successfully.\nBackup: %s", saved_draft.name, backup_path),
    }
end

return M
