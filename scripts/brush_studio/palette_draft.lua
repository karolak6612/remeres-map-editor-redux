local Paths = dofile("module_paths.lua")
local GroundCommon = Paths.load("ground_studio/common.lua")

local PaletteDraft = {}
PaletteDraft.__index = PaletteDraft

local function clone_reference(reference)
    if type(reference) ~= "table" then
        return nil
    end
    return GroundCommon.clone(reference)
end

function PaletteDraft.new(init)
    local self = setmetatable({}, PaletteDraft)
    self.selectedBrush = clone_reference(init and init.selectedBrush)
    self.targetTileset = GroundCommon.trim(init and init.targetTileset or "")
    self.targetCategory = GroundCommon.trim(init and init.targetCategory or "")
    self.displayName = GroundCommon.trim(init and init.displayName or "")
    self.order = tonumber(init and init.order or 0) or 0
    self.dirty = GroundCommon.normalize_bool(init and init.dirty, false)
    return self
end

function PaletteDraft:clear()
    self.selectedBrush = nil
    self.targetTileset = ""
    self.targetCategory = ""
    self.displayName = ""
    self.order = 0
    self.dirty = false
end

function PaletteDraft:isEmpty()
    return not self.selectedBrush and self.targetTileset == "" and self.targetCategory == "" and self.displayName == ""
end

function PaletteDraft:setBrush(reference)
    self.selectedBrush = clone_reference(reference)
    if self.displayName == "" and self.selectedBrush then
        self.displayName = GroundCommon.trim(self.selectedBrush.title or self.selectedBrush.name or "")
    end
    self.dirty = true
    return self.selectedBrush ~= nil
end

function PaletteDraft:setTileset(value)
    self.targetTileset = GroundCommon.trim(value)
    self.dirty = true
end

function PaletteDraft:setCategory(value)
    self.targetCategory = GroundCommon.trim(value)
    self.dirty = true
end

function PaletteDraft:setDisplayName(value)
    self.displayName = GroundCommon.trim(value)
    self.dirty = true
end

function PaletteDraft:setOrder(value)
    local parsed = tonumber(value or 0) or 0
    parsed = math.floor(parsed)
    if parsed < 0 then
        parsed = 0
    end
    self.order = parsed
    self.dirty = true
end

function PaletteDraft:effectiveDisplayName()
    if self.displayName ~= "" then
        return self.displayName
    end
    if self.selectedBrush then
        return GroundCommon.trim(self.selectedBrush.title or self.selectedBrush.name or "")
    end
    return ""
end

function PaletteDraft:hasRequiredDataForSave()
    if not self.selectedBrush or GroundCommon.trim(self.selectedBrush.name or "") == "" then
        return false
    end
    if self.targetTileset == "" or self.targetCategory == "" then
        return false
    end
    return true
end

function PaletteDraft:getValidation()
    local errors = {}
    local warnings = {}

    if not self.selectedBrush or GroundCommon.trim(self.selectedBrush.name or "") == "" then
        table.insert(errors, "Najpierw wybierz gotowy brush.")
    end
    if self.targetTileset == "" or self.targetCategory == "" then
        table.insert(errors, "Wybierz miejsce w palecie.")
    end
    if self.displayName == "" and self.selectedBrush and GroundCommon.trim(self.selectedBrush.name or "") ~= "" then
        table.insert(warnings, "Display name jest pusty, wiec paleta uzyje nazwy brusha.")
    end

    return {
        isValid = #errors == 0,
        errors = errors,
        warnings = warnings,
    }
end

function PaletteDraft:clone()
    return PaletteDraft.new({
        selectedBrush = self.selectedBrush,
        targetTileset = self.targetTileset,
        targetCategory = self.targetCategory,
        displayName = self.displayName,
        order = self.order,
        dirty = self.dirty,
    })
end

return PaletteDraft
