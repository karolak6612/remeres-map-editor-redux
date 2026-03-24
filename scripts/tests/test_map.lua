-- @Title: Test Map API
-- @Description: Verification tests for this API category.
local framework = require("framework")

framework.test("map properties", function()
    if not app.hasMap() then
        print("Skipping map tests (no map open)")
        return
    end
    
    local map = app.map
    framework.assert(type(map.name) == "string", "map.name should be string")
    framework.assert(type(map.width) == "number", "map.width should be number")
    framework.assert(type(map.height) == "number", "map.height should be number")
    framework.assert(type(map.tileCount) == "number", "map.tileCount should be number")
end)

framework.test("map methods", function()
    if not app.hasMap() then return end
    
    local map = app.map
    framework.assert(type(map.getTile) == "function", "getTile should be function")
    framework.assert(type(map.getOrCreateTile) == "function", "getOrCreateTile should be function")
    framework.assert(type(map.tiles) ~= "nil", "map.tiles should exist")
end)

framework.test("getTile basic", function()
    if not app.hasMap() then return end
    
    local map = app.map
    local tile = map:getTile(100, 100, 7)
    -- tile can be nil if not exists, so check type
    framework.assert(type(tile) == "table" or type(tile) == "userdata" or type(tile) == "nil", "getTile should return tile or nil")
end)

framework.summary()
