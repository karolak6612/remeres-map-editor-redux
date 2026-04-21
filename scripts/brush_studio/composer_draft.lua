local Paths = dofile("module_paths.lua")
local GroundCommon = Paths.load("ground_studio/common.lua")

local ComposerDraft = {}
ComposerDraft.__index = ComposerDraft

local function clone_reference(reference)
    if type(reference) ~= "table" then
        return nil
    end
    local copy = GroundCommon.clone(reference)
    if type(reference.draft) == "table" and type(reference.draft.clone) == "function" then
        copy.draft = reference.draft:clone()
    end
    return copy
end

local function normalize_align(value)
    local text = string.lower(tostring(value or "outer"))
    if text ~= "inner" then
        return "outer"
    end
    return text
end

function ComposerDraft.new(init)
    local self = setmetatable({}, ComposerDraft)
    self.name = GroundCommon.trim(init and init.name or "")
    self.selectedGround = clone_reference(init and init.selectedGround)
    self.selectedBorder = clone_reference(init and init.selectedBorder)
    self.toBrush = GroundCommon.trim(init and init.toBrush or "")
    self.align = normalize_align(init and init.align)
    self.friends = {}
    self.dirty = GroundCommon.normalize_bool(init and init.dirty, false)
    self.isNew = GroundCommon.normalize_bool(init and init.isNew, true)
    self.sourceName = GroundCommon.trim(init and (init.sourceName or init.name) or "")
    self.extraChildrenXml = tostring(init and init.extraChildrenXml or "")

    for _, friend_name in ipairs((init and init.friends) or {}) do
        local trimmed = GroundCommon.trim(friend_name)
        if trimmed ~= "" then
            table.insert(self.friends, trimmed)
        end
    end

    return self
end

function ComposerDraft:clear()
    self.name = ""
    self.selectedGround = nil
    self.selectedBorder = nil
    self.toBrush = ""
    self.align = "outer"
    self.friends = {}
    self.dirty = false
    self.extraChildrenXml = ""
end

function ComposerDraft:isEmpty()
    return not self.selectedGround and not self.selectedBorder and self.name == "" and #self.friends == 0 and self.toBrush == ""
end

function ComposerDraft:setGround(reference)
    self.selectedGround = clone_reference(reference)
    self.dirty = true
    return self.selectedGround ~= nil
end

function ComposerDraft:setBorder(reference)
    self.selectedBorder = clone_reference(reference)
    self.dirty = true
    return self.selectedBorder ~= nil
end

function ComposerDraft:setName(value)
    self.name = GroundCommon.trim(value)
    self.dirty = true
end

function ComposerDraft:setToBrush(value)
    self.toBrush = GroundCommon.trim(value)
    self.dirty = true
end

function ComposerDraft:setAlign(value)
    self.align = normalize_align(value)
    self.dirty = true
end

function ComposerDraft:addFriend(friend_name)
    local trimmed = GroundCommon.trim(friend_name)
    if trimmed == "" then
        return false, "Podaj nazwe friend brush."
    end
    for _, existing in ipairs(self.friends) do
        if string.lower(existing) == string.lower(trimmed) then
            return false, "Ten friend jest juz dodany."
        end
    end
    table.insert(self.friends, trimmed)
    self.dirty = true
    return true
end

function ComposerDraft:removeFriend(index)
    index = tonumber(index or 0) or 0
    if index <= 0 or index > #self.friends then
        return false
    end
    table.remove(self.friends, index)
    self.dirty = true
    return true
end

function ComposerDraft:hasRequiredDataForSave()
    if self.name == "" then
        return false
    end
    if not self.selectedGround or not self.selectedGround.draft then
        return false
    end
    if not self.selectedGround.draft.hasMainItem or not self.selectedGround.draft:hasMainItem() then
        return false
    end
    if not self.selectedBorder or (tonumber(self.selectedBorder.borderId or 0) or 0) <= 0 then
        return false
    end
    return true
end

function ComposerDraft:getValidation()
    local errors = {}
    local warnings = {}

    if self.name == "" then
        table.insert(errors, "Podaj nazwe, aby zapisac brush.")
    end
    if not self.selectedGround then
        table.insert(errors, "Najpierw wybierz ground.")
    elseif not self.selectedGround.draft or not self.selectedGround.draft.hasMainItem or not self.selectedGround.draft:hasMainItem() then
        table.insert(errors, "Wybrany ground nie ma poprawnego main tile.")
    end
    if not self.selectedBorder or (tonumber(self.selectedBorder.borderId or 0) or 0) <= 0 then
        table.insert(errors, "Najpierw wybierz border.")
    end
    if self.toBrush ~= "" and self.selectedGround and string.lower(self.toBrush) == string.lower(self.selectedGround.name or "") then
        table.insert(warnings, "toBrush wskazuje na ten sam ground co zrodlo.")
    end

    return {
        isValid = #errors == 0,
        errors = errors,
        warnings = warnings,
    }
end

function ComposerDraft:clone()
    return ComposerDraft.new({
        name = self.name,
        selectedGround = self.selectedGround,
        selectedBorder = self.selectedBorder,
        toBrush = self.toBrush,
        align = self.align,
        friends = self.friends,
        dirty = self.dirty,
        isNew = self.isNew,
        sourceName = self.sourceName,
        extraChildrenXml = self.extraChildrenXml,
    })
end

return ComposerDraft
