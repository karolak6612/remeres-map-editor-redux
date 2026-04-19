local BorderRepository = dofile("border_repository.lua")
local GroundDraft = dofile("ground_draft.lua")

local M = {}

function M.read_border_repository(session_borders, border_table)
    return BorderRepository.read(session_borders, border_table or app.borders or {})
end

function M.border_sources(repository)
    return BorderRepository.border_sources(repository)
end

function M.find_border_source(repository, border_id)
    return BorderRepository.find_border_source(repository, border_id)
end

function M.new_ground_draft(init)
    return GroundDraft.new(init)
end

return M
