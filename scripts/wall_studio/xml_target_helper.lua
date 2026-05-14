local M = {}

local function normalize(path)
    return tostring(path or ""):gsub("\\", "/")
end

local function lower_path(path)
    return string.lower(normalize(path))
end

function M.is_xml_file(path)
    return lower_path(path):match("%.xml$") ~= nil
end

function M.matches_expected_filename(path, expected_filename)
    expected_filename = tostring(expected_filename or "")
    if expected_filename == "" then
        return true
    end
    local normalized = lower_path(path)
    local suffix = "/" .. string.lower(expected_filename)
    return normalized:sub(-#suffix) == suffix or normalized == string.lower(expected_filename)
end

function M.is_inside_data_dir(path)
    local data_dir = lower_path(app.getDataDirectory() or "")
    local normalized = lower_path(path)
    if data_dir == "" then
        return false
    end
    if normalized == data_dir then
        return true
    end
    return normalized:sub(1, #data_dir + 1) == (data_dir .. "/")
end

function M.default_xml_wildcard(expected_filename)
    expected_filename = tostring(expected_filename or "")
    if expected_filename == "" then
        return "XML files (*.xml)|*.xml|All files (*.*)|*.*"
    end
    return string.format("%s (%s)|%s|XML files (*.xml)|*.xml|All files (*.*)|*.*", expected_filename, expected_filename, expected_filename)
end

function M.pick_xml_file(expected_filename, current_path, title)
    if not app or not app.chooseFile then
        return nil, "This RME build does not expose file picker support."
    end

    local chosen = app.chooseFile({
        title = title or "Select XML file",
        path = normalize(current_path),
        wildcard = M.default_xml_wildcard(expected_filename),
    })

    if not chosen or chosen == "" then
        return nil
    end

    chosen = normalize(chosen)
    if not M.is_xml_file(chosen) then
        return nil, "Wybrany plik musi byc plikiem XML."
    end
    if not M.matches_expected_filename(chosen, expected_filename) then
        return nil, string.format("Wybierz plik '%s'.", tostring(expected_filename or ""))
    end
    if not M.is_inside_data_dir(chosen) then
        return nil, "Wybrany plik musi znajdowac sie w aktualnym katalogu data tego RME."
    end

    return chosen
end

return M
