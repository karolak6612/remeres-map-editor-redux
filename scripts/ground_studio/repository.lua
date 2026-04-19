local Common = dofile("common.lua")
local GroundDraft = dofile("ground_draft.lua")
local XmlCodec = dofile("xml_codec.lua")
local Logger = dofile("logger.lua")

local M = {}

local function file_exists(path)
    if not path or path == "" then
        return false
    end

    local store = app.storage(path)
    if store and store.exists then
        return store:exists()
    end

    return false
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

local function build_repository_from_app_grounds(app_grounds, session_grounds)
    local repository = {
        ordered = {},
        by_name = {},
        warnings = {},
        target_path = "",
        api_missing = nil,
    }

    for _, ground_data in pairs(app_grounds or {}) do
        local name = Common.trim(ground_data.name or "")
        if name ~= "" then
            local items = {}
            for _, item_data in ipairs(ground_data.items or {}) do
                local item_id = tonumber(item_data.id or item_data.itemId or 0) or 0
                local chance = tonumber(item_data.chance or 1) or 1
                if item_id > 0 then
                    table.insert(items, {
                        itemId = item_id,
                        chance = chance,
                    })
                end
            end

            local main_item = items[1]
            local variants = {}
            for index = 2, #items do
                table.insert(variants, items[index])
            end

            repository.by_name[name] = GroundDraft.new({
                groundId = tonumber(ground_data.id or 0) or 0,
                name = name,
                mainItem = main_item,
                mainChance = main_item and main_item.chance or 1,
                variants = variants,
                lookId = tonumber(ground_data.lookid or 0) or 0,
                serverLookId = tonumber(ground_data.server_lookid or 0) or 0,
                zOrder = tonumber(ground_data.z_order or ground_data["z-order"] or 0) or 0,
                randomize = ground_data.randomize,
                soloOptional = ground_data.solo_optional,
                isNew = false,
                sourceName = name,
            })
        end
    end

    for name, overlay in pairs(session_grounds or {}) do
        repository.by_name[name] = overlay:clone()
        repository.by_name[name].isNew = false
    end

    for _, ground in pairs(repository.by_name) do
        table.insert(repository.ordered, ground)
    end

    table.sort(repository.ordered, function(a, b)
        return string.lower(a.name or "") < string.lower(b.name or "")
    end)

    return repository
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
    if saved_path and saved_path ~= "" and file_exists(saved_path) then
        return saved_path
    end

    local data_dir = app.getDataDirectory()
    local preferred = tostring(data_dir or "") .. "/1310/grounds.xml"
    if file_exists(preferred) then
        return preferred
    end

    return preferred
end

function M.read(target_path, session_grounds)
    local repository

    if app and app.grounds then
        repository = build_repository_from_app_grounds(app.grounds, session_grounds)
        repository.target_path = target_path or ""
        Logger.log("repository", string.format("read done from app.grounds count=%d warnings=%d", #repository.ordered, #repository.warnings))
        return repository
    end

    repository = {
        ordered = {},
        by_name = {},
        warnings = {},
        target_path = target_path or "",
        api_missing = nil,
    }

    Logger.log("repository", "read start")
    local content, err = read_file(target_path)
    if not content then
        repository.api_missing = err
        table.insert(repository.warnings, err)
        Logger.log("repository", "read failed: " .. tostring(err))
        return repository
    end

    local parsed = XmlCodec.parse_grounds(content)
    repository.ordered = parsed.ordered
    repository.by_name = parsed.by_name
    repository.warnings = parsed.warnings or {}

    for name, overlay in pairs(session_grounds or {}) do
        repository.by_name[name] = overlay:clone()
        repository.by_name[name].isNew = false
    end

    repository.ordered = {}
    for _, ground in pairs(repository.by_name) do
        table.insert(repository.ordered, ground)
    end

    table.sort(repository.ordered, function(a, b)
        return string.lower(a.name or "") < string.lower(b.name or "")
    end)

    for _, warning in ipairs(repository.warnings) do
        Logger.log("repository", "warning: " .. tostring(warning))
    end

    Logger.log("repository", string.format("read done count=%d warnings=%d", #repository.ordered, #repository.warnings))
    return repository
end

function M.next_name(repository, session_grounds, preferred_base)
    local base = Common.trim(preferred_base or "new ground")
    if base == "" then
        base = "new ground"
    end

    local candidate = base
    local index = 1
    while (repository.by_name and repository.by_name[candidate]) or (session_grounds and session_grounds[candidate]) do
        index = index + 1
        candidate = string.format("%s %d", base, index)
    end
    return candidate
end

function M.new_draft(repository, session_grounds)
    return GroundDraft.new({
        name = M.next_name(repository, session_grounds, "new ground"),
        randomize = true,
        isNew = true,
    })
end

function M.load_draft(repository, name)
    local ground = repository.by_name[name]
    if not ground then
        return nil
    end
    return ground:clone()
end

function M.duplicate_draft(repository, session_grounds, draft)
    local copy = draft:clone()
    copy.name = M.next_name(repository, session_grounds, Common.trim((draft.name ~= "" and draft.name or "ground") .. " copy"))
    copy.sourceName = copy.name
    copy.isNew = true
    return copy
end

function M.library_items(repository, filter_text)
    local items = {}
    local visible_names = {}
    local needle = string.lower(filter_text or "")

    for _, ground in ipairs(repository.ordered or {}) do
        local status = Common.ground_status(ground)
        local item_count = #ground:getAllItems()
        local main_item_id = Common.primary_item_id(ground)
        local main_item_name = Common.safe_item_name(main_item_id)
        local blob = string.lower((ground.name or "") .. " " .. status .. " " .. main_item_name)
        if needle == "" or string.find(blob, needle, 1, true) then
            table.insert(items, {
                text = string.format("%s  |  %s  |  %s", ground.name or "Unnamed", status, main_item_name ~= "" and main_item_name or ("Item " .. tostring(main_item_id or 0))),
                tooltip = string.format("%s\nStatus: %s\nMain: %s (%d)\nItems: %d", ground.name or "Unnamed", status, main_item_name ~= "" and main_item_name or "Unknown", tonumber(main_item_id or 0) or 0, item_count),
                image = Common.safe_image(main_item_id),
            })
            table.insert(visible_names, ground.name)
        end
    end

    return items, visible_names
end

function M.save_intent(repository, draft, mode)
    local name = Common.trim(draft and draft.name or "")
    local source_name = Common.trim(draft and draft.sourceName or "")
    local exists = name ~= "" and repository.by_name and repository.by_name[name] ~= nil
    local source_exists = source_name ~= "" and repository.by_name and repository.by_name[source_name] ~= nil

    if mode == "new" then
        if name == "" then
            return "Podaj nazwe grounda, aby zapisac nowy wpis."
        end
        if exists then
            return string.format("Save as New zablokowany: ground '%s' juz istnieje.", name)
        end
        return string.format("Utworzy nowy ground '%s'.", name)
    end

    if not source_exists then
        return "Overwrite wymaga istniejacego grounda w bibliotece."
    end
    return string.format("Nadpisze ground '%s'.", source_name)
end

function M.validate_save(repository, draft, mode, target_path)
    if not target_path or target_path == "" then
        return false, "Target grounds.xml could not be resolved."
    end
    if not Common.path_starts_with(target_path, app.getDataDirectory()) then
        return false, "Target grounds.xml must stay inside the RME data directory."
    end
    if not tostring(target_path):lower():match("%.xml$") then
        return false, "Target file must be an XML file."
    end
    if not file_exists(target_path) then
        return false, "Target grounds.xml does not exist."
    end
    if not draft then
        return false, "Brak draftu do zapisu."
    end

    local validation = draft:getValidation()
    if not validation.isValid then
        return false, validation.errors[1] or "Ground draft jest niepoprawny."
    end

    local current_name = Common.trim(draft.name or "")
    local source_name = Common.trim(draft.sourceName or current_name)
    local exists = repository.by_name and repository.by_name[current_name] ~= nil
    local source_exists = repository.by_name and repository.by_name[source_name] ~= nil

    if mode == "new" and exists then
        return false, string.format("Ground '%s' juz istnieje.", current_name)
    end
    if mode == "overwrite" and not source_exists then
        return false, string.format("Ground '%s' nie istnieje do nadpisania.", source_name)
    end

    return true
end

function M.backup_xml(target_path)
    local original, err = read_file(target_path)
    if not original then
        return false, nil, nil, "Could not read grounds.xml before backup: " .. tostring(err)
    end

    local backup_path = string.format("%s.groundstudio.%s.bak", target_path, os.date("%Y%m%d_%H%M%S"))
    local ok, backup_err = write_file(backup_path, original)
    if not ok then
        return false, nil, nil, "Could not write backup file: " .. tostring(backup_err)
    end

    return true, backup_path, original
end

function M.save_draft(repository, session_grounds, draft, mode, target_path)
    local ok, validation_message = M.validate_save(repository, draft, mode, target_path)
    if not ok then
        return false, validation_message
    end

    local backup_ok, backup_path, original_content, backup_message = M.backup_xml(target_path)
    if not backup_ok then
        return false, backup_message
    end

    local saved_draft = draft:clone()
    saved_draft.name = Common.trim(saved_draft.name)
    saved_draft.isNew = false

    local writer = app.storage(target_path)
    if not writer or not writer.upsertXml then
        return false, "The current RME build does not expose XML upsert support."
    end

    local match_name = mode == "overwrite" and Common.trim(saved_draft.sourceName or saved_draft.name) or saved_draft.name
    local xml = XmlCodec.serialize_ground(saved_draft)
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

    if match_name ~= saved_draft.name then
        session_grounds[match_name] = nil
    end

    saved_draft.sourceName = saved_draft.name
    session_grounds[saved_draft.name] = saved_draft:clone()

    return true, {
        draft = saved_draft,
        backupPath = backup_path,
        xmlMode = write_mode or mode,
        message = string.format("Saved ground '%s' successfully.\nBackup: %s", saved_draft.name, backup_path),
    }
end

return M
