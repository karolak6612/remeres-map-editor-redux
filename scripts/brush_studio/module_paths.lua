local M = {}

local function normalize_slashes(path)
    return tostring(path or ""):gsub("\\", "/")
end

local brush_studio_dir = normalize_slashes(rawget(_G, "SCRIPT_DIR") or "")
if brush_studio_dir == "" then
    error("Brush Studio loader requires SCRIPT_DIR from the script runtime.")
end

local scripts_dir = brush_studio_dir:gsub("/brush_studio$", "")

function M.resolve(relative_path)
    relative_path = normalize_slashes(relative_path):gsub("^/*", "")
    return scripts_dir .. "/" .. relative_path
end

function M.load(relative_path)
    relative_path = normalize_slashes(relative_path):gsub("^/*", "")

    local old_script_dir = rawget(_G, "SCRIPT_DIR")
    _G.SCRIPT_DIR = scripts_dir

    local ok, result = pcall(dofile, relative_path)

    _G.SCRIPT_DIR = old_script_dir
    if not ok then
        error(tostring(result))
    end

    return result
end

return M
