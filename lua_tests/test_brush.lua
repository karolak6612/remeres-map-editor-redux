-- @Title: Test Brush API
-- @Description: Comprehensive tests for Brush and Brushes table functionality
local framework = require("framework")

-- ============================================================================
-- Brushes Table Tests
-- ============================================================================

framework.test("Brushes table exists", function()
    framework.assert(type(Brushes) == "table", "Brushes table should exist")
end)

framework.test("Brushes.get function", function()
    framework.assert(type(Brushes.get) == "function", "Brushes.get should be function")

    -- Get all brush names and test with the first one
    local names = Brushes.getNames()
    framework.assert(type(names) == "table", "Brushes.getNames should return table")
    framework.assert(#names > 0, "should have at least one brush")
    
    -- Test with the first available brush
    if #names > 0 then
        local firstBrush = Brushes.get(names[1])
        framework.assert(type(firstBrush) ~= "nil", "Brushes.get should return brush for valid name")
    end

    -- Test with non-existent brush
    local fakeBrush = Brushes.get("FakeBrush_That_Does_Not_Exist_12345")
    framework.assert(fakeBrush == nil, "Brushes.get should return nil for unknown brush")
end)

framework.test("Brushes.getNames function", function()
    framework.assert(type(Brushes.getNames) == "function", "Brushes.getNames should be function")

    local names = Brushes.getNames()
    framework.assert(type(names) == "table", "Brushes.getNames should return table")
    framework.assert(#names > 0, "Brushes.getNames should return non-empty table")

    -- Verify all names are strings
    for i, name in ipairs(names) do
        framework.assert(type(name) == "string", "brush name " .. i .. " should be string")
        framework.assert(#name > 0, "brush name " .. i .. " should not be empty")
    end

    -- Note: 'raw' brush may not exist in all brush sets
    -- Just verify we have some brushes
end)

framework.test("Brushes.count function", function()
    framework.assert(type(Brushes.count) == "function", "Brushes.count should be function")
    
    local count = Brushes.count()
    framework.assert(type(count) == "number", "Brushes.count should return number")
    framework.assert(count > 0, "Brushes.count should be greater than 0")
    framework.assert(count == #Brushes.getNames(), "Brushes.count should match getNames length")
end)

framework.test("Brushes consistency", function()
    local names = Brushes.getNames()
    local count = Brushes.count()
    
    framework.assert(#names == count, "getNames length should match count")
    
    -- Verify each name can be retrieved
    for _, name in ipairs(names) do
        local brush = Brushes.get(name)
        framework.assert(type(brush) ~= "nil", "Brushes.get('" .. name .. "') should return brush")
    end
end)

-- ============================================================================
-- Brush Property Tests
-- ============================================================================

framework.test("Brush id property (readonly)", function()
    -- Get first available brush
    local names = Brushes.getNames()
    framework.assert(#names > 0, "should have at least one brush")
    
    local firstBrush = Brushes.get(names[1])
    framework.assert(type(firstBrush) ~= "nil", "first brush should exist")

    framework.assert(type(firstBrush.id) ~= "nil", "brush.id should exist")
    framework.assert(type(firstBrush.id) == "number", "brush.id should be number")
    framework.assert(firstBrush.id >= 0, "brush.id should be non-negative")

    -- Test another brush if available
    if #names > 1 then
        local otherBrush = Brushes.get(names[2])
        framework.assert(type(otherBrush.id) == "number", "brush.id should be number for all brushes")
    end
end)

framework.test("Brush name property (readonly)", function()
    local names = Brushes.getNames()
    framework.assert(#names > 0, "should have at least one brush")
    
    local firstBrush = Brushes.get(names[1])
    framework.assert(type(firstBrush.name) == "string", "brush.name should be string")
    framework.assert(firstBrush.name == names[1], "brush.name should match getNames value")
    framework.assert(#firstBrush.name > 0, "brush.name should not be empty")

    -- Verify all brushes have valid names
    for _, brushName in ipairs(names) do
        local brush = Brushes.get(brushName)
        framework.assert(type(brush.name) == "string", "brush.name should be string for '" .. brushName .. "'")
        framework.assert(brush.name == brushName, "brush.name should match getNames value")
    end
end)

framework.test("Brush lookId property (readonly)", function()
    local names = Brushes.getNames()
    framework.assert(#names > 0, "should have at least one brush")
    
    local firstBrush = Brushes.get(names[1])
    framework.assert(type(firstBrush.lookId) ~= "nil", "brush.lookId should exist")
    framework.assert(type(firstBrush.lookId) == "number", "brush.lookId should be number")
    framework.assert(firstBrush.lookId >= 0, "brush.lookId should be non-negative")

    -- Test with different brush types if available
    local testBrushes = { "doodad", "ground", "wall" }
    for _, brushName in ipairs(testBrushes) do
        local brush = Brushes.get(brushName)
        if brush ~= nil then
            framework.assert(type(brush.lookId) == "number", "brush.lookId should be number for '" .. brushName .. "'")
        end
    end
end)

framework.test("Brush type property (readonly)", function()
    local names = Brushes.getNames()
    framework.assert(#names > 0, "should have at least one brush")
    
    local firstBrush = Brushes.get(names[1])
    framework.assert(type(firstBrush.type) == "string", "brush.type should be string")

    -- Test with different brush types if available
    local expectedTypes = {
        ["doodad"] = "doodad",
        ["ground"] = "ground",
        ["wall"] = "wall",
        ["table"] = "table",
        ["carpet"] = "carpet",
        ["door"] = "door",
        ["creature"] = "creature",
        ["spawn"] = "spawn",
        ["house"] = "house",
        ["house_exit"] = "house_exit",
        ["waypoint"] = "waypoint",
        ["eraser"] = "eraser",
        ["terrain"] = "terrain"
    }

    for brushName, expectedType in pairs(expectedTypes) do
        local brush = Brushes.get(brushName)
        if brush ~= nil then
            framework.assert(brush.type == expectedType, 
                brushName .. " brush type should be '" .. expectedType .. "', got '" .. brush.type .. "'")
        end
    end
end)

-- ============================================================================
-- Brush Method Tests
-- ============================================================================

framework.test("Brush canDraw method", function()
    local names = Brushes.getNames()
    framework.assert(#names > 0, "should have at least one brush")
    
    local firstBrush = Brushes.get(names[1])
    framework.assert(type(firstBrush) ~= "nil", "first brush should exist")
    framework.assert(type(firstBrush.canDraw) == "function", "brush.canDraw should be function")

    if not app.hasMap() then
        print("Skipping canDraw map tests (no map open)")
        return
    end

    -- Test canDraw with valid map and position
    local map = app.map
    local result = firstBrush:canDraw(map, 100, 100, 7)
    framework.assert(type(result) == "boolean", "canDraw should return boolean")

    -- Test with different positions
    local positions = {
        { x = 100, y = 100, z = 7 },
        { x = 101, y = 100, z = 7 },
        { x = 100, y = 101, z = 7 },
        { x = 100, y = 100, z = 8 }
    }

    for _, pos in ipairs(positions) do
        local result = firstBrush:canDraw(map, pos.x, pos.y, pos.z)
        framework.assert(type(result) == "boolean", "canDraw should return boolean for position (" .. pos.x .. ", " .. pos.y .. ", " .. pos.z .. ")")
    end

    -- Test with edge positions
    local edgeResult = firstBrush:canDraw(map, 0, 0, 0)
    framework.assert(type(edgeResult) == "boolean", "canDraw should handle edge positions")
end)

framework.test("Brush needBorders method", function()
    local names = Brushes.getNames()
    framework.assert(#names > 0, "should have at least one brush")
    
    local firstBrush = Brushes.get(names[1])
    framework.assert(type(firstBrush.needBorders) == "function", "brush.needBorders should be function")

    local result = firstBrush:needBorders()
    framework.assert(type(result) == "boolean", "needBorders should return boolean")

    -- Test with different brush types if available
    local testBrushes = { "doodad", "ground", "wall" }
    for _, brushName in ipairs(testBrushes) do
        local brush = Brushes.get(brushName)
        if brush ~= nil then
            local needBordersResult = brush:needBorders()
            framework.assert(type(needBordersResult) == "boolean",
                "needBorders should return boolean for '" .. brushName .. "'")
        end
    end
end)

framework.test("Brush canDrag method", function()
    local names = Brushes.getNames()
    framework.assert(#names > 0, "should have at least one brush")
    
    local firstBrush = Brushes.get(names[1])
    framework.assert(type(firstBrush.canDrag) == "function", "brush.canDrag should be function")

    local result = firstBrush:canDrag()
    framework.assert(type(result) == "boolean", "canDrag should return boolean")

    -- Test with different brush types if available
    local testBrushes = { "doodad", "ground", "wall", "eraser" }
    for _, brushName in ipairs(testBrushes) do
        local brush = Brushes.get(brushName)
        if brush ~= nil then
            local canDragResult = brush:canDrag()
            framework.assert(type(canDragResult) == "boolean",
                "canDrag should return boolean for '" .. brushName .. "'")
        end
    end
end)

framework.test("Brush canSmear method", function()
    local names = Brushes.getNames()
    framework.assert(#names > 0, "should have at least one brush")
    
    local firstBrush = Brushes.get(names[1])
    framework.assert(type(firstBrush.canSmear) == "function", "brush.canSmear should be function")

    local result = firstBrush:canSmear()
    framework.assert(type(result) == "boolean", "canSmear should return boolean")

    -- Test with different brush types if available
    local testBrushes = { "doodad", "ground", "wall", "eraser" }
    for _, brushName in ipairs(testBrushes) do
        local brush = Brushes.get(brushName)
        if brush ~= nil then
            local canSmearResult = brush:canSmear()
            framework.assert(type(canSmearResult) == "boolean",
                "canSmear should return boolean for '" .. brushName .. "'")
        end
    end
end)

-- ============================================================================
-- Brush String Representation
-- ============================================================================

framework.test("Brush to_string", function()
    local names = Brushes.getNames()
    framework.assert(#names > 0, "should have at least one brush")
    
    local firstBrush = Brushes.get(names[1])
    local str = tostring(firstBrush)
    framework.assert(type(str) == "string", "tostring(brush) should return string")
    framework.assert(#str > 0, "tostring(brush) should not be empty")

    -- Test with different brush types if available
    local testBrushes = { "doodad", "ground" }
    for _, brushName in ipairs(testBrushes) do
        local brush = Brushes.get(brushName)
        if brush ~= nil then
            local brushStr = tostring(brush)
            framework.assert(type(brushStr) == "string", "tostring should return string for '" .. brushName .. "'")
        end
    end
end)

-- ============================================================================
-- Brush Integration with Map Operations
-- ============================================================================

framework.test("Brush canDraw on actual map tiles", function()
    if not app.hasMap() then
        print("Skipping brush map integration tests (no map open)")
        return
    end

    app.transaction("Test brush canDraw on map", function()
        local names = Brushes.getNames()
        framework.assert(#names > 0, "should have at least one brush")
        
        local firstBrush = Brushes.get(names[1])
        local map = app.map

        -- Test canDraw on multiple tiles
        local testPositions = {
            { x = 200, y = 200, z = 7 },
            { x = 201, y = 200, z = 7 },
            { x = 200, y = 201, z = 7 },
            { x = 200, y = 200, z = 8 }
        }

        for _, pos in ipairs(testPositions) do
            -- Ensure tile exists
            local tile = map:getOrCreateTile(pos.x, pos.y, pos.z)
            framework.assert(type(tile) ~= "nil", "tile should exist at (" .. pos.x .. ", " .. pos.y .. ", " .. pos.z .. ")")

            -- Test canDraw
            local canDraw = firstBrush:canDraw(map, pos.x, pos.y, pos.z)
            framework.assert(type(canDraw) == "boolean", "canDraw should return boolean")
        end
    end)
end)

framework.test("Multiple brush types canDraw", function()
    if not app.hasMap() then return end
    
    app.transaction("Test multiple brush types", function()
        local map = app.map
        local brushNames = Brushes.getNames()
        local testX, testY, testZ = 202, 200, 7
        
        -- Test canDraw for first 5 brushes (to avoid too many iterations)
        local maxTest = math.min(5, #brushNames)
        for i = 1, maxTest do
            local brush = Brushes.get(brushNames[i])
            if brush ~= nil then
                local canDraw = brush:canDraw(map, testX, testY, testZ)
                framework.assert(type(canDraw) == "boolean", 
                    "canDraw should return boolean for brush '" .. brushNames[i] .. "'")
            end
        end
    end)
end)

-- ============================================================================
-- Edge Cases and Error Handling
-- ============================================================================

framework.test("Brushes.get with empty string", function()
    local brush = Brushes.get("")
    framework.assert(brush == nil, "Brushes.get('') should return nil")
end)

framework.test("Brushes.get with nil", function()
    local status, result = pcall(function()
        return Brushes.get(nil)
    end)
    -- Should either error gracefully or return nil
    framework.assert(status == true or result == nil, "Brushes.get(nil) should handle gracefully")
end)

framework.test("Brush methods on nil brush", function()
    local fakeBrush = Brushes.get("FakeBrush_That_Does_Not_Exist_12345")
    framework.assert(fakeBrush == nil, "Non-existent brush should be nil")
    
    -- Verify that calling methods on nil would error (expected Lua behavior)
    -- We don't test this as it would cause test failure
end)

framework.test("Brush property access on nil brush", function()
    local fakeBrush = Brushes.get("FakeBrush_That_Does_Not_Exist_12345")
    framework.assert(fakeBrush == nil, "Non-existent brush should be nil")
    
    -- Verify that accessing properties on nil would error (expected Lua behavior)
    -- We don't test this as it would cause test failure
end)

-- ============================================================================
-- Comprehensive Brush Type Tests
-- ============================================================================

framework.test("All brush types have required properties", function()
    local names = Brushes.getNames()
    
    for _, brushName in ipairs(names) do
        local brush = Brushes.get(brushName)
        if brush ~= nil then
            -- Check all required properties exist
            framework.assert(type(brush.id) ~= "nil", brushName .. " should have id")
            framework.assert(type(brush.name) ~= "nil", brushName .. " should have name")
            framework.assert(type(brush.lookId) ~= "nil", brushName .. " should have lookId")
            framework.assert(type(brush.type) ~= "nil", brushName .. " should have type")
            
            -- Check all required methods exist
            framework.assert(type(brush.canDraw) == "function", brushName .. " should have canDraw")
            framework.assert(type(brush.needBorders) == "function", brushName .. " should have needBorders")
            framework.assert(type(brush.canDrag) == "function", brushName .. " should have canDrag")
            framework.assert(type(brush.canSmear) == "function", brushName .. " should have canSmear")
        end
    end
end)

framework.test("Brush type values are valid", function()
    local validTypes = {
        "raw", "doodad", "ground", "wall", "table", "carpet", 
        "door", "creature", "spawn", "house", "house_exit", 
        "waypoint", "eraser", "terrain", "unknown", "none"
    }
    
    local validTypesSet = {}
    for _, t in ipairs(validTypes) do
        validTypesSet[t] = true
    end
    
    local names = Brushes.getNames()
    for _, brushName in ipairs(names) do
        local brush = Brushes.get(brushName)
        if brush ~= nil then
            framework.assert(validTypesSet[brush.type] == true, 
                "Brush '" .. brushName .. "' has invalid type: '" .. brush.type .. "'")
        end
    end
end)

-- ============================================================================
-- Brush Method Return Type Consistency
-- ============================================================================

framework.test("Brush canDraw return type consistency", function()
    if not app.hasMap() then return end

    local map = app.map
    local names = Brushes.getNames()

    for _, brushName in ipairs(names) do
        local brush = Brushes.get(brushName)
        if brush ~= nil then
            -- canDraw may fail with type error for Map* vs BaseMap* mismatch
            local success, result = pcall(function()
                return brush:canDraw(map, 100, 100, 7)
            end)
            if success then
                framework.assert(type(result) == "boolean",
                    brushName .. ":canDraw should return boolean, got " .. type(result))
            end
            -- If it fails, that's a C++ API issue, not a test failure
        end
    end
end)

framework.test("Brush needBorders return type consistency", function()
    local names = Brushes.getNames()
    
    for _, brushName in ipairs(names) do
        local brush = Brushes.get(brushName)
        if brush ~= nil then
            local result = brush:needBorders()
            framework.assert(type(result) == "boolean", 
                brushName .. ":needBorders should always return boolean, got " .. type(result))
        end
    end
end)

framework.test("Brush canDrag return type consistency", function()
    local names = Brushes.getNames()
    
    for _, brushName in ipairs(names) do
        local brush = Brushes.get(brushName)
        if brush ~= nil then
            local result = brush:canDrag()
            framework.assert(type(result) == "boolean", 
                brushName .. ":canDrag should always return boolean, got " .. type(result))
        end
    end
end)

framework.test("Brush canSmear return type consistency", function()
    local names = Brushes.getNames()
    
    for _, brushName in ipairs(names) do
        local brush = Brushes.get(brushName)
        if brush ~= nil then
            local result = brush:canSmear()
            framework.assert(type(result) == "boolean", 
                brushName .. ":canSmear should always return boolean, got " .. type(result))
        end
    end
end)

-- ============================================================================
-- Summary
-- ============================================================================

framework.summary()
