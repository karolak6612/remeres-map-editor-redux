local Common = dofile("common.lua")

local GroundDraft = {}
GroundDraft.__index = GroundDraft

local function build_item_reference(raw_item)
    if type(raw_item) == "table" then
        local item_id = tonumber(raw_item.itemId or raw_item.item_id or raw_item.id or 0) or 0
        if item_id <= 0 then
            return nil
        end
        local info = Common.safe_item_info(item_id)
        return {
            itemId = item_id,
            rawId = tonumber(raw_item.rawId or raw_item.raw_id or item_id) or item_id,
            lookId = tonumber(raw_item.lookId or raw_item.look_id or raw_item.clientId or (info and info.clientId) or 0) or 0,
            name = raw_item.name or (info and info.name) or ("Item " .. tostring(item_id)),
        }
    end

    local item_id = tonumber(raw_item or 0) or 0
    if item_id <= 0 then
        return nil
    end
    local info = Common.safe_item_info(item_id)
    return {
        itemId = item_id,
        rawId = item_id,
        lookId = info and info.clientId or 0,
        name = info and info.name or ("Item " .. tostring(item_id)),
    }
end

local function normalize_chance(value, default)
    local chance = tonumber(value or default or 1) or tonumber(default or 1) or 1
    chance = math.floor(chance)
    if chance <= 0 then
        chance = tonumber(default or 1) or 1
        chance = math.floor(chance)
        if chance <= 0 then
            chance = 1
        end
    end
    return chance
end

local function sync_main_aliases(self, reference)
    self.mainGroundItem = reference
    self.mainItem = reference
end

local function clone_variant(variant)
    local copy = Common.clone(variant)
    copy.chance = normalize_chance(copy.chance, 1)
    return copy
end

function GroundDraft.new(init)
    local self = setmetatable({}, GroundDraft)
    self.groundId = tonumber((init and (init.groundId or init.ground_id or init.id)) or 0) or 0
    self.name = Common.trim(init and init.name or "")
    sync_main_aliases(self, build_item_reference(init and (init.mainGroundItem or init.main_ground_item or init.mainItem)))
    self.mainChance = normalize_chance(init and (init.mainChance or init.main_chance), 1)
    self.variants = {}
    self.lookId = tonumber((init and (init.lookId or init.look_id)) or 0) or 0
    self.serverLookId = tonumber((init and (init.serverLookId or init.server_lookid)) or 0) or 0
    self.zOrder = tonumber((init and (init.zOrder or init["z-order"] or init.z_order)) or 0) or 0
    self.randomize = Common.normalize_bool(init and init.randomize, true)
    self.soloOptional = Common.normalize_bool(init and (init.soloOptional or init.solo_optional), false)
    self.extraChildrenXml = tostring((init and (init.extraChildrenXml or init.extra_children_xml)) or "")
    self.isNew = Common.normalize_bool(init and init.isNew, true)
    self.sourceName = Common.trim(init and (init.sourceName or init.source_name or init.name) or "")

    for _, variant in ipairs((init and init.variants) or {}) do
        self:addVariant(variant, variant.chance)
    end

    if self.mainGroundItem then
        if self.lookId <= 0 then
            self.lookId = self.mainGroundItem.lookId or 0
        end
        if self.serverLookId <= 0 then
            self.serverLookId = self.mainGroundItem.itemId or 0
        end
    end

    return self
end

function GroundDraft:clear()
    self.name = ""
    sync_main_aliases(self, nil)
    self.mainChance = 1
    self.variants = {}
    self.lookId = 0
    self.serverLookId = 0
    self.zOrder = 0
    self.randomize = true
    self.soloOptional = false
    self.extraChildrenXml = ""
end

function GroundDraft:setName(name)
    self.name = Common.trim(name)
end

function GroundDraft:setMainGround(raw_item)
    local reference = build_item_reference(raw_item)
    if not reference then
        return false
    end
    sync_main_aliases(self, reference)
    if self.lookId <= 0 then
        self.lookId = reference.lookId or 0
    end
    if self.serverLookId <= 0 then
        self.serverLookId = reference.itemId or 0
    end
    return true
end

function GroundDraft:setMainItem(raw_item)
    return self:setMainGround(raw_item)
end

function GroundDraft:clearMainItem()
    sync_main_aliases(self, nil)
end

function GroundDraft:setMainChance(chance)
    self.mainChance = normalize_chance(chance, self.mainChance)
end

function GroundDraft:findVariantByItem(item_id)
    item_id = tonumber(item_id or 0) or 0
    if item_id <= 0 then
        return nil
    end
    for index, variant in ipairs(self.variants) do
        if tonumber(variant.itemId or 0) == item_id then
            return index, variant
        end
    end
    return nil
end

function GroundDraft:addVariant(raw_item, chance)
    local reference = build_item_reference(raw_item)
    if not reference then
        return false, "Invalid ground variant item."
    end
    if self.mainGroundItem and self.mainGroundItem.itemId == reference.itemId then
        return false, "This item is already used as the main ground tile."
    end
    if self:findVariantByItem(reference.itemId) then
        return false, "This variant already exists."
    end

    reference.chance = normalize_chance(chance, 1)
    table.insert(self.variants, reference)
    return true
end

function GroundDraft:updateVariant(index, raw_item, chance)
    index = tonumber(index or 0) or 0
    if index <= 0 or index > #self.variants then
        return false, "Variant index is out of range."
    end

    local reference = build_item_reference(raw_item)
    if not reference then
        return false, "Invalid ground variant item."
    end
    if self.mainGroundItem and self.mainGroundItem.itemId == reference.itemId then
        return false, "This item is already used as the main ground tile."
    end

    local existing_index = self:findVariantByItem(reference.itemId)
    if existing_index and existing_index ~= index then
        return false, "This variant already exists."
    end

    reference.chance = normalize_chance(chance, self.variants[index].chance or 1)
    self.variants[index] = reference
    return true
end

function GroundDraft:removeVariant(index)
    index = tonumber(index or 0) or 0
    if index <= 0 or index > #self.variants then
        return false
    end
    table.remove(self.variants, index)
    return true
end

function GroundDraft:setVariantChance(index, chance)
    index = tonumber(index or 0) or 0
    if index <= 0 or index > #self.variants then
        return false
    end
    self.variants[index].chance = normalize_chance(chance, self.variants[index].chance)
    return true
end

function GroundDraft:normalizeChancesIfNeeded()
    self.mainChance = normalize_chance(self.mainChance, 1)
    for index, variant in ipairs(self.variants) do
        self.variants[index].chance = normalize_chance(variant.chance, 1)
    end
end

function GroundDraft:setLookId(value)
    self.lookId = tonumber(value or 0) or 0
end

function GroundDraft:setServerLookId(value)
    self.serverLookId = tonumber(value or 0) or 0
end

function GroundDraft:setZOrder(value)
    self.zOrder = tonumber(value or 0) or 0
end

function GroundDraft:setRandomize(value)
    self.randomize = Common.normalize_bool(value, self.randomize)
end

function GroundDraft:setSoloOptional(value)
    self.soloOptional = Common.normalize_bool(value, self.soloOptional)
end

function GroundDraft:hasMainItem()
    return self.mainGroundItem ~= nil and tonumber(self.mainGroundItem.itemId or 0) > 0
end

function GroundDraft:isEmpty()
    return not self:hasMainItem() and #self.variants == 0
end

function GroundDraft:getValidation()
    local errors = {}
    local warnings = {}

    if self.name == "" then
        table.insert(errors, "Nazwa grounda nie moze byc pusta.")
    end
    if not self:hasMainItem() then
        table.insert(errors, "Main ground tile nie moze byc pusty.")
    end

    if self.mainGroundItem and (tonumber(self.mainGroundItem.itemId or 0) or 0) <= 0 then
        table.insert(errors, "Main ground tile ma niepoprawny item id.")
    end

    local seen = {}
    for index, variant in ipairs(self.variants) do
        local item_id = tonumber(variant.itemId or 0) or 0
        local chance = tonumber(variant.chance or 0) or 0
        if item_id <= 0 then
            table.insert(errors, string.format("Wariant %d ma niepoprawny item id.", index))
        end
        if chance <= 0 then
            table.insert(errors, string.format("Wariant %d ma niepoprawny chance.", index))
        end
        if item_id > 0 then
            if seen[item_id] then
                table.insert(errors, string.format("Wariant %d duplikuje item %d.", index, item_id))
            end
            seen[item_id] = true
        end
    end

    if self.mainGroundItem and seen[self.mainGroundItem.itemId] then
        table.insert(errors, "Wariant nie moze duplikowac glownego ground tile.")
    end

    return {
        hasData = not self:isEmpty(),
        isValid = #errors == 0,
        errors = errors,
        warnings = warnings,
    }
end

function GroundDraft:hasRequiredDataForSave()
    return self:getValidation().isValid
end

function GroundDraft:getAllItems()
    self:normalizeChancesIfNeeded()
    local items = {}
    if self.mainGroundItem then
        local main = Common.clone(self.mainGroundItem)
        main.chance = normalize_chance(self.mainChance, 1)
        table.insert(items, main)
    end
    for _, variant in ipairs(self.variants) do
        table.insert(items, clone_variant(variant))
    end
    return items
end

function GroundDraft:clone()
    return GroundDraft.new({
        groundId = self.groundId,
        name = self.name,
        mainGroundItem = self.mainGroundItem and Common.clone(self.mainGroundItem) or nil,
        mainChance = self.mainChance,
        variants = Common.clone(self.variants),
        lookId = self.lookId,
        serverLookId = self.serverLookId,
        zOrder = self.zOrder,
        randomize = self.randomize,
        soloOptional = self.soloOptional,
        extraChildrenXml = self.extraChildrenXml,
        isNew = self.isNew,
        sourceName = self.sourceName,
    })
end

function GroundDraft:toSaveData()
    return {
        groundId = self.groundId,
        name = self.name,
        mainGroundItem = self.mainGroundItem and Common.clone(self.mainGroundItem) or nil,
        mainChance = self.mainChance,
        variants = Common.clone(self.variants),
        lookId = self.lookId,
        serverLookId = self.serverLookId,
        zOrder = self.zOrder,
        randomize = self.randomize,
        soloOptional = self.soloOptional,
        extraChildrenXml = self.extraChildrenXml,
        sourceName = self.sourceName,
    }
end

function GroundDraft:toXmlGroundData()
    self:normalizeChancesIfNeeded()
    return {
        groundId = self.groundId,
        name = self.name,
        mainGroundItem = self.mainGroundItem and Common.clone(self.mainGroundItem) or nil,
        variants = Common.clone(self.variants),
        lookId = self.lookId,
        serverLookId = self.serverLookId,
        zOrder = self.zOrder,
        randomize = self.randomize,
        soloOptional = self.soloOptional,
        extraChildrenXml = self.extraChildrenXml,
        sourceName = self.sourceName,
        items = self:getAllItems(),
    }
end

return GroundDraft
