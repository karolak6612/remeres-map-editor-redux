local Paths = dofile("module_paths.lua")
local GroundCommon = Paths.load("ground_studio/common.lua")
local ComposerRepository = dofile("composer_repository.lua")
local PaletteDraft = dofile("palette_draft.lua")

local M = {}

local PUBLISHABLE_SECTIONS = {
    terrain = true,
    doodad = true,
    items = true,
    items_and_raw = true,
    doodad_and_raw = true,
}

local function parse_attributes(text)
    local attrs = {}
    for key, value in tostring(text or ""):gmatch('([%w_%-]+)%s*=%s*"([^"]-)"') do
        attrs[key] = GroundCommon.unescape_xml(value)
    end
    return attrs
end

local function extract_blocks(xml, tag_name)
    local blocks = {}
    local cursor = 1
    local open_pattern = "<" .. tag_name .. "%s"
    local close_tag = "</" .. tag_name .. ">"

    while true do
        local start_index = xml:find(open_pattern, cursor)
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
                start_index = start_index,
                end_index = tag_end,
                attributes = parse_attributes(open_tag),
                inner = "",
                open_tag = open_tag,
            })
            cursor = tag_end + 1
        else
            local close_start, close_end = xml:find(close_tag, tag_end + 1, true)
            if not close_start then
                break
            end
            table.insert(blocks, {
                start_index = start_index,
                end_index = close_end,
                attributes = parse_attributes(open_tag),
                inner = xml:sub(tag_end + 1, close_start - 1),
                open_tag = open_tag,
            })
            cursor = close_end + 1
        end
    end

    return blocks
end

local function read_file(path)
    local store = app.storage(path)
    if not store or not store.readText then
        return nil, "This RME build does not expose raw file reads for tilesets.xml."
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
    if saved_path and saved_path ~= "" then
        return saved_path
    end
    return tostring(app.getDataDirectory() or "") .. "/1310/tilesets.xml"
end

local function parse_tilesets(xml)
    local repository = {
        ordered_tilesets = {},
        by_tileset = {},
        warnings = {},
    }

    for _, tileset_block in ipairs(extract_blocks(tostring(xml or ""), "tileset")) do
        local tileset_name = GroundCommon.trim((tileset_block.attributes or {}).name or "")
        if tileset_name ~= "" then
            local entry = {
                name = tileset_name,
                sections = {},
                sections_by_name = {},
            }

            for _, section_block in ipairs(extract_blocks(tileset_block.inner or "", "[%w_%-]+")) do
            end

            local cursor = 1
            while true do
                local section_start, section_end, section_name = tostring(tileset_block.inner or ""):find("<([%w_%-]+)>", cursor)
                if not section_start then
                    break
                end
                local close_tag = "</" .. section_name .. ">"
                local close_start, close_end = tostring(tileset_block.inner or ""):find(close_tag, section_end + 1, true)
                if not close_start then
                    break
                end
                local section_inner = tostring(tileset_block.inner or ""):sub(section_end + 1, close_start - 1)

                local section = {
                    name = section_name,
                    title = section_name:gsub("_", " "),
                    brush_entries = {},
                    other_xml = section_inner:gsub("<brush%s+.-%s*/>", ""),
                }

                for brush_attrs in tostring(section_inner):gmatch("<brush%s+(.-)%s*/>") do
                    local parsed = parse_attributes(brush_attrs)
                    local brush_name = GroundCommon.trim(parsed.name or "")
                    if brush_name ~= "" then
                        table.insert(section.brush_entries, {
                            brushName = brush_name,
                        })
                    end
                end

                entry.sections_by_name[section_name] = section
                table.insert(entry.sections, section)
                cursor = close_end + 1
            end

            repository.by_tileset[tileset_name] = entry
            table.insert(repository.ordered_tilesets, entry)
        end
    end

    return repository
end

local function composer_brush_sources(composer_repository)
    local items = {}
    local seen = {}

    for _, draft in ipairs((composer_repository.composed_ordered) or {}) do
        local name = GroundCommon.trim(draft.name or "")
        if name ~= "" and not seen[string.lower(name)] then
            seen[string.lower(name)] = true
            local ground_ref = draft.selectedGround
            table.insert(items, {
                name = name,
                title = name,
                sampleItemId = ground_ref and ground_ref.sampleItemId or 0,
                sourceType = "composer",
            })
        end
    end

    for _, draft in ipairs((composer_repository.ground_repository and composer_repository.ground_repository.ordered) or {}) do
        local name = GroundCommon.trim(draft.name or "")
        if name ~= "" and not seen[string.lower(name)] then
            seen[string.lower(name)] = true
            table.insert(items, {
                name = name,
                title = name,
                sampleItemId = GroundCommon.primary_item_id(draft),
                sourceType = "ground",
            })
        end
    end

    table.sort(items, function(left, right)
        return string.lower(left.title or "") < string.lower(right.title or "")
    end)

    return items
end

function M.read(target_path)
    local repository = {
        target_path = target_path,
        warnings = {},
        api_missing = nil,
        ordered_tilesets = {},
        by_tileset = {},
        brush_sources = {},
        brush_sources_by_name = {},
    }

    local composer_repository = ComposerRepository.read(tostring(app.getDataDirectory() or "") .. "/1310/grounds.xml")
    repository.composer_repository = composer_repository
    for _, warning in ipairs(composer_repository.warnings or {}) do
        table.insert(repository.warnings, warning)
    end

    local content, err = read_file(target_path)
    if not content then
        repository.api_missing = err
        table.insert(repository.warnings, err)
        return repository
    end

    local parsed = parse_tilesets(content)
    repository.ordered_tilesets = parsed.ordered_tilesets
    repository.by_tileset = parsed.by_tileset
    repository.warnings = repository.warnings or {}
    for _, warning in ipairs(parsed.warnings or {}) do
        table.insert(repository.warnings, warning)
    end

    repository.brush_sources = composer_brush_sources(composer_repository)
    for _, source in ipairs(repository.brush_sources) do
        repository.brush_sources_by_name[string.lower(source.name)] = source
    end

    return repository
end

function M.tileset_items(repository)
    local items = {}
    local names = {}
    for _, tileset in ipairs(repository.ordered_tilesets or {}) do
        table.insert(items, {
            text = tileset.name,
            tooltip = tileset.name,
        })
        table.insert(names, tileset.name)
    end
    return items, names
end

function M.category_items(repository, tileset_name)
    local items = {}
    local names = {}
    local tileset = repository.by_tileset and repository.by_tileset[tileset_name] or nil
    for _, section in ipairs(tileset and tileset.sections or {}) do
        if PUBLISHABLE_SECTIONS[section.name] then
            table.insert(items, {
                text = string.format("%s  |  %d wpisow", section.title, #(section.brush_entries or {})),
                tooltip = section.name,
            })
            table.insert(names, section.name)
        end
    end
    return items, names
end

function M.brush_items(repository, filter_text)
    local items = {}
    local names = {}
    local needle = string.lower(filter_text or "")
    for _, source in ipairs(repository.brush_sources or {}) do
        local blob = string.lower((source.title or "") .. " " .. (source.sourceType or ""))
        if needle == "" or string.find(blob, needle, 1, true) then
            table.insert(items, {
                text = string.format("%s  |  %s", source.title or source.name, source.sourceType or "brush"),
                tooltip = string.format("%s\nSource: %s", source.name or "", source.sourceType or "brush"),
                image = GroundCommon.safe_image(source.sampleItemId or 0),
            })
            table.insert(names, source.name)
        end
    end
    return items, names
end

function M.make_brush_reference(repository, brush_name)
    return repository.brush_sources_by_name and repository.brush_sources_by_name[string.lower(brush_name or "")] or nil
end

local function find_entry(repository, brush_name)
    brush_name = GroundCommon.trim(brush_name or "")
    if brush_name == "" then
        return nil
    end
    for _, tileset in ipairs(repository.ordered_tilesets or {}) do
        for _, section in ipairs(tileset.sections or {}) do
            for index, entry in ipairs(section.brush_entries or {}) do
                if string.lower(entry.brushName or "") == string.lower(brush_name) then
                    return {
                        tileset = tileset.name,
                        category = section.name,
                        index = index,
                    }
                end
            end
        end
    end
    return nil
end

function M.preview_entries(repository, draft)
    local tileset = repository.by_tileset and repository.by_tileset[draft.targetTileset] or nil
    local section = tileset and tileset.sections_by_name and tileset.sections_by_name[draft.targetCategory] or nil
    local preview = {}

    if not section then
        return preview
    end

    for _, entry in ipairs(section.brush_entries or {}) do
        table.insert(preview, {
            name = entry.brushName,
            isSelected = draft.selectedBrush and string.lower(entry.brushName or "") == string.lower(draft.selectedBrush.name or ""),
            isInserted = false,
        })
    end

    if draft.selectedBrush and GroundCommon.trim(draft.selectedBrush.name or "") ~= "" then
        local insert_name = draft.selectedBrush.name
        local existing_index = nil
        for index, entry in ipairs(preview) do
            if string.lower(entry.name or "") == string.lower(insert_name) then
                existing_index = index
                break
            end
        end

        if not existing_index then
            local insert_at = tonumber(draft.order or 0) or 0
            if insert_at <= 0 or insert_at > (#preview + 1) then
                insert_at = #preview + 1
            end
            table.insert(preview, insert_at, {
                name = insert_name,
                isSelected = true,
                isInserted = true,
            })
        elseif draft.order > 0 and draft.order ~= existing_index and draft.order <= #preview then
            local item = table.remove(preview, existing_index)
            table.insert(preview, draft.order, item)
            item.isInserted = true
        end
    end

    return preview
end

function M.validate_save(repository, draft, mode, target_path)
    if not target_path or target_path == "" then
        return false, "Target tilesets.xml could not be resolved."
    end
    if not GroundCommon.path_starts_with(target_path, app.getDataDirectory()) then
        return false, "Target tilesets.xml must stay inside the RME data directory."
    end

    local validation = draft:getValidation()
    if not validation.isValid then
        return false, validation.errors[1] or "Palette draft jest niepoprawny."
    end

    local tileset = repository.by_tileset and repository.by_tileset[draft.targetTileset] or nil
    if not tileset then
        return false, "Wybrany tileset nie istnieje."
    end
    local section = tileset.sections_by_name and tileset.sections_by_name[draft.targetCategory] or nil
    if not section then
        return false, "Wybrana sekcja nie istnieje."
    end

    local existing = find_entry(repository, draft.selectedBrush and draft.selectedBrush.name or "")
    if mode == "new" and existing then
        return false, string.format("Brush '%s' jest juz opublikowany w palecie.", draft.selectedBrush.name or "")
    end
    if mode == "overwrite" and not existing then
        return false, string.format("Brush '%s' nie istnieje jeszcze w palecie.", draft.selectedBrush.name or "")
    end

    return true
end

function M.backup_xml(target_path)
    local original, err = read_file(target_path)
    if not original then
        return false, nil, nil, "Could not read tilesets.xml before backup: " .. tostring(err)
    end

    local backup_path = string.format("%s.palette.%s.bak", target_path, os.date("%Y%m%d_%H%M%S"))
    local ok, backup_err = write_file(backup_path, original)
    if not ok then
        return false, nil, nil, "Could not write backup file: " .. tostring(backup_err)
    end

    return true, backup_path, original
end

local function serialize_repository(repository)
    local lines = { "<materials>" }
    for _, tileset in ipairs(repository.ordered_tilesets or {}) do
        table.insert(lines, string.format('\t<tileset name="%s">', GroundCommon.escape_xml(tileset.name)))
        for _, section in ipairs(tileset.sections or {}) do
            table.insert(lines, string.format("\t\t<%s>", section.name))
            local other_xml = tostring(section.other_xml or ""):gsub("\r\n", "\n")
            for line in other_xml:gmatch("[^\r\n]+") do
                local trimmed = GroundCommon.trim(line)
                if trimmed ~= "" then
                    table.insert(lines, "\t\t\t" .. trimmed)
                end
            end
            for _, entry in ipairs(section.brush_entries or {}) do
                table.insert(lines, string.format('\t\t\t<brush name="%s" />', GroundCommon.escape_xml(entry.brushName)))
            end
            table.insert(lines, string.format("\t\t</%s>", section.name))
        end
        table.insert(lines, "\t</tileset>")
    end
    table.insert(lines, "</materials>")
    return table.concat(lines, "\n")
end

local function rebuild_repository_indexes(repository)
    repository.by_tileset = {}
    for _, tileset in ipairs(repository.ordered_tilesets or {}) do
        tileset.sections_by_name = {}
        for _, section in ipairs(tileset.sections or {}) do
            tileset.sections_by_name[section.name] = section
        end
        repository.by_tileset[tileset.name] = tileset
    end
    return repository
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

    local working = rebuild_repository_indexes(GroundCommon.clone(repository))
    local selected_name = draft.selectedBrush.name
    local existing = find_entry(working, selected_name)

    if existing then
        local old_tileset = working.by_tileset[existing.tileset]
        local old_section = old_tileset and old_tileset.sections_by_name and old_tileset.sections_by_name[existing.category] or nil
        if old_section and old_section.brush_entries then
            table.remove(old_section.brush_entries, existing.index)
        end
    end

    local target_tileset = working.by_tileset[draft.targetTileset]
    local target_section = target_tileset and target_tileset.sections_by_name and target_tileset.sections_by_name[draft.targetCategory] or nil
    if not target_section then
        if original_content then
            write_file(target_path, original_content)
        end
        return false, "Wybrana sekcja docelowa nie istnieje."
    end

    local insert_at = tonumber(draft.order or 0) or 0
    if insert_at <= 0 or insert_at > (#target_section.brush_entries + 1) then
        insert_at = #target_section.brush_entries + 1
    end
    table.insert(target_section.brush_entries, insert_at, {
        brushName = selected_name,
    })

    local xml = serialize_repository(working)
    local write_ok, write_err = write_file(target_path, xml)
    if not write_ok then
        if original_content then
            write_file(target_path, original_content)
        end
        return false, write_err or "Saving failed while updating tilesets.xml."
    end

    return true, {
        draft = draft:clone(),
        backupPath = backup_path,
        message = string.format("Published brush '%s' to %s / %s.\nBackup: %s", selected_name, draft.targetTileset, draft.targetCategory, backup_path),
    }
end

return M
