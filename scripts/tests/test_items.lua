-- @Title: Test Items Namespace API
-- @Description: Comprehensive verification tests for the Items namespace API.
local framework = require("framework")

framework.test("Items table existence", function()
    framework.assert(type(Items) ~= "nil", "Items table should exist")
end)

framework.test("Items functions existence", function()
    framework.assert(type(Items.getInfo) == "function", "Items.getInfo should be function")
    framework.assert(type(Items.exists) == "function", "Items.exists should be function")
    framework.assert(type(Items.getMaxId) == "function", "Items.getMaxId should be function")
    framework.assert(type(Items.findByName) == "function", "Items.findByName should be function")
    framework.assert(type(Items.findIdByName) == "function", "Items.findIdByName should be function")
    framework.assert(type(Items.getName) == "function", "Items.getName should be function")
end)

framework.test("Items.exists", function()
    -- Test with known item (crystal coin is usually 2160)
    local exists1 = Items.exists(2160)
    framework.assert(type(exists1) == "boolean", "exists should return boolean")

    -- Test with invalid ID
    local exists2 = Items.exists(999999)
    framework.assert(type(exists2) == "boolean", "exists with invalid ID should return boolean")

    -- Test with zero
    local exists3 = Items.exists(0)
    framework.assert(type(exists3) == "boolean", "exists with 0 should return boolean")

    -- Test with negative
    local exists4 = Items.exists(-1)
    framework.assert(type(exists4) == "boolean", "exists with negative should return boolean")
end)

framework.test("Items.getMaxId", function()
    local maxId = Items.getMaxId()
    framework.assert(type(maxId) == "number", "getMaxId should return number")
    framework.assert(maxId > 0, "maxId should be positive")
    framework.assert(maxId < 1000000, "maxId should be reasonable")
end)

framework.test("Items.getName", function()
    -- Test with known item
    local name1 = Items.getName(2160)
    framework.assert(type(name1) == "string", "getName should return string")

    -- Test with invalid ID
    local name2 = Items.getName(999999)
    framework.assert(type(name2) == "string", "getName with invalid ID should return string")
    framework.assert(name2 == "", "getName with invalid ID should return empty string")

    -- Test with zero
    local name3 = Items.getName(0)
    framework.assert(type(name3) == "string", "getName with 0 should return string")
end)

framework.test("Items.getInfo", function()
    -- Test with known item (crystal coin)
    local info = Items.getInfo(2160)
    framework.assert(type(info) == "table", "getInfo should return table")

    -- Check expected fields
    framework.assert(type(info.id) == "number", "info.id should be number")
    framework.assert(type(info.clientId) == "number", "info.clientId should be number")
    framework.assert(type(info.name) == "string", "info.name should be string")
    framework.assert(type(info.description) == "string", "info.description should be string")
    framework.assert(type(info.isStackable) == "boolean", "info.isStackable should be boolean")
    framework.assert(type(info.isMoveable) == "boolean", "info.isMoveable should be boolean")
    framework.assert(type(info.isPickupable) == "boolean", "info.isPickupable should be boolean")
    framework.assert(type(info.isGroundTile) == "boolean", "info.isGroundTile should be boolean")
    framework.assert(type(info.isBorder) == "boolean", "info.isBorder should be boolean")
    framework.assert(type(info.isWall) == "boolean", "info.isWall should be boolean")
    framework.assert(type(info.isDoor) == "boolean", "info.isDoor should be boolean")
    framework.assert(type(info.isTable) == "boolean", "info.isTable should be boolean")
    framework.assert(type(info.isCarpet) == "boolean", "info.isCarpet should be boolean")
    framework.assert(type(info.hasElevation) == "boolean", "info.hasElevation should be boolean")

    -- Verify crystal coin is stackable
    framework.assert(info.isStackable == true, "crystal coin should be stackable")
end)

framework.test("Items.getInfo with invalid ID", function()
    local info = Items.getInfo(999999)
    -- getInfo may return nil or empty table for invalid IDs
    framework.assert(info == nil or type(info) == "table", "getInfo with invalid ID should return nil or table")
end)

framework.test("Items.getInfo with various item types", function()
    -- Test ground tile
    local groundInfo = Items.getInfo(100) -- grass
    if groundInfo then
        framework.assert(type(groundInfo.isGroundTile) == "boolean",
            "ground item should have isGroundTile")
    end

    -- Test wall
    local wallInfo = Items.getInfo(1000) -- wall
    if wallInfo then
        framework.assert(type(wallInfo.isWall) == "boolean",
            "wall item should have isWall")
    end

    -- Test door
    local doorInfo = Items.getInfo(1000) -- door
    if doorInfo then
        framework.assert(type(doorInfo.isDoor) == "boolean",
            "door item should have isDoor")
    end
end)

framework.test("Items.findByName basic", function()
    local results = Items.findByName("coin")
    framework.assert(type(results) == "table", "findByName should return table")

    -- Check result structure
    for i = 1, #results do
        local item = results[i]
        framework.assert(type(item) == "table", "each result should be table")
        framework.assert(type(item.id) == "number", "result.id should be number")
        framework.assert(type(item.name) == "string", "result.name should be string")
    end
end)

framework.test("Items.findByName with maxResults", function()
    local results1 = Items.findByName("sword", 5)
    framework.assert(type(results1) == "table", "findByName with maxResults should work")
    framework.assert(#results1 <= 5, "should respect maxResults limit")

    local results2 = Items.findByName("sword", 20)
    framework.assert(#results2 >= #results1, "higher maxResults should return more or equal")
end)

framework.test("Items.findByName case insensitive", function()
    local results1 = Items.findByName("coin")
    local results2 = Items.findByName("Coin")
    local results3 = Items.findByName("COIN")

    framework.assert(#results1 > 0, "should find coins with lowercase")
    framework.assert(#results1 == #results2, "search should be case insensitive")
    framework.assert(#results1 == #results3, "search should be case insensitive")
end)

framework.test("Items.findByName partial match", function()
    local results = Items.findByName("crystal")
    framework.assert(type(results) == "table", "partial match should work")

    -- Should find items containing "crystal" in name
    for i = 1, #results do
        local name = results[i].name:lower()
        framework.assert(name:find("crystal"), "result should contain 'crystal'")
    end
end)

framework.test("Items.findByName no results", function()
    local results = Items.findByName("xyznonexistent123")
    framework.assert(type(results) == "table", "no results should return empty table")
    framework.assert(#results == 0, "should return empty table for no matches")
end)

framework.test("Items.findIdByName", function()
    local id = Items.findIdByName("Crystal Coin")
    framework.assert(type(id) == "number" or id == nil, "findIdByName should return number or nil")

    if id then
        framework.assert(id > 0, "found ID should be positive")
        framework.assert(Items.exists(id), "found ID should exist")
    end
end)

framework.test("Items.findIdByName case insensitive", function()
    local id1 = Items.findIdByName("crystal coin")
    local id2 = Items.findIdByName("Crystal Coin")
    local id3 = Items.findIdByName("CRYSTAL COIN")

    framework.assert(id1 == id2, "search should be case insensitive")
    framework.assert(id2 == id3, "search should be case insensitive")
end)

framework.test("Items.findIdByName exact match preference", function()
    -- Should prefer exact matches
    local id = Items.findIdByName("Crystal Coin")
    if id then
        local info = Items.getInfo(id)
        if info then
            framework.assert(info.name:lower() == "crystal coin",
                "exact match should return correct item")
        end
    end
end)

framework.test("Items.findIdByName no results", function()
    local id = Items.findIdByName("xyznonexistent999")
    framework.assert(id == nil, "should return nil for no match")
end)

framework.test("Items consistency checks", function()
    -- getMaxId should be greater than any valid ID
    local maxId = Items.getMaxId()

    -- Test some IDs up to maxId
    local validCount = 0
    local invalidCount = 0

    for id = 1, math.min(maxId, 1000) do
        if Items.exists(id) then
            validCount = validCount + 1
        else
            invalidCount = invalidCount + 1
        end
    end

    framework.assert(validCount > 0, "should have some valid items")
end)

framework.test("Items.getInfo field consistency", function()
    -- All items should have consistent field types
    local maxId = Items.getMaxId()
    local checked = 0

    for id = 1, math.min(maxId, 500) do
        local info = Items.getInfo(id)
        if info then
            framework.assert(type(info.id) == "number", "id should be number for item " .. id)
            framework.assert(type(info.name) == "string", "name should be string for item " .. id)
            checked = checked + 1
        end
    end

    framework.assert(checked > 0, "should check at least one item")
end)

framework.test("Items.getName consistency with getInfo", function()
    local name1 = Items.getName(2160)
    local info = Items.getInfo(2160)

    if info then
        framework.assert(name1 == info.name,
            "getName should match info.name")
    end
end)

framework.test("Items edge cases", function()
    -- Empty string search
    local emptyResults = Items.findByName("")
    framework.assert(type(emptyResults) == "table", "empty string search should return table")

    -- Very long search string
    local longSearch = string.rep("a", 100)
    local longResults = Items.findByName(longSearch)
    framework.assert(type(longResults) == "table", "long search should return table")
    framework.assert(#longResults == 0, "long search should return no results")

    -- Special characters
    local specialResults = Items.findByName("@#$%")
    framework.assert(type(specialResults) == "table", "special chars should return table")
end)

framework.test("Items performance - multiple calls", function()
    -- Test that multiple calls don't cause issues
    for i = 1, 100 do
        local name = Items.getName(i)
        framework.assert(type(name) == "string", "getName loop should work")
    end

    for i = 1, 100 do
        local exists = Items.exists(i)
        framework.assert(type(exists) == "boolean", "exists loop should work")
    end
end)

framework.test("Items return value ranges", function()
    local maxId = Items.getMaxId()

    -- Test boundary IDs
    local id1 = Items.getInfo(1)
    framework.assert(id1 == nil or type(id1) == "table", "ID 1 should return nil or table")

    local idMax = Items.getInfo(maxId)
    framework.assert(idMax == nil or type(idMax) == "table", "maxId should return nil or table")

    local idOver = Items.getInfo(maxId + 1)
    framework.assert(idOver == nil, "maxId + 1 should return nil")
end)

framework.summary()
