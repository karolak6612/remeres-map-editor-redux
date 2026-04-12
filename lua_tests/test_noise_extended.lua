-- @Title: Test Noise API (Extended)
-- @Description: Comprehensive verification tests for the Noise API.
local framework = require("framework")

framework.test("Noise table existence", function()
    framework.assert(type(noise) ~= "nil", "noise table should exist")
end)

framework.test("Noise 2D functions existence", function()
    framework.assert(type(noise.perlin) == "function", "noise.perlin should be function")
    framework.assert(type(noise.simplex) == "function", "noise.simplex should be function")
    framework.assert(type(noise.simplexSmooth) == "function", "noise.simplexSmooth should be function")
    framework.assert(type(noise.cellular) == "function", "noise.cellular should be function")
    framework.assert(type(noise.value) == "function", "noise.value should be function")
    framework.assert(type(noise.valueCubic) == "function", "noise.valueCubic should be function")
end)

framework.test("Noise 3D functions existence", function()
    framework.assert(type(noise.perlin3d) == "function", "noise.perlin3d should be function")
    framework.assert(type(noise.simplex3d) == "function", "noise.simplex3d should be function")
    framework.assert(type(noise.cellular3d) == "function", "noise.cellular3d should be function")
end)

framework.test("Noise fractal functions existence", function()
    framework.assert(type(noise.fbm) == "function", "noise.fbm should be function")
    framework.assert(type(noise.fbm3d) == "function", "noise.fbm3d should be function")
    framework.assert(type(noise.ridged) == "function", "noise.ridged should be function")
end)

framework.test("Noise domain warp existence", function()
    framework.assert(type(noise.warp) == "function", "noise.warp should be function")
end)

framework.test("Noise utility functions existence", function()
    framework.assert(type(noise.normalize) == "function", "noise.normalize should be function")
    framework.assert(type(noise.threshold) == "function", "noise.threshold should be function")
    framework.assert(type(noise.map) == "function", "noise.map should be function")
    framework.assert(type(noise.clamp) == "function", "noise.clamp should be function")
    framework.assert(type(noise.lerp) == "function", "noise.lerp should be function")
    framework.assert(type(noise.smoothstep) == "function", "noise.smoothstep should be function")
    framework.assert(type(noise.clearCache) == "function", "noise.clearCache should be function")
end)

framework.test("Noise batch generation existence", function()
    framework.assert(type(noise.generateGrid) == "function", "noise.generateGrid should be function")
end)

framework.test("Noise perlin basic", function()
    local val = noise.perlin(0, 0, 1234)
    framework.assert(type(val) == "number", "perlin should return number")
    framework.assert(val >= -1 and val <= 1, "perlin should return value in [-1, 1]")

    local val2 = noise.perlin(100, 200, 1234)
    framework.assert(type(val2) == "number", "perlin with different coords should work")

    -- Same seed and coords should give same result
    local val3 = noise.perlin(0, 0, 1234)
    framework.assert(val3 == val, "same seed/coords should give same result")
end)

framework.test("Noise perlin with frequency", function()
    local val1 = noise.perlin(10, 20, 1234, 0.01)
    local val2 = noise.perlin(10, 20, 1234, 0.1)

    framework.assert(type(val1) == "number", "perlin with frequency should work")
    framework.assert(type(val2) == "number", "perlin with different frequency should work")
    -- Different frequencies should generally give different results
    framework.assert(val1 ~= val2, "different frequencies should give different results")
end)

framework.test("Noise perlin3d", function()
    local val = noise.perlin3d(0, 0, 0, 1234)
    framework.assert(type(val) == "number", "perlin3d should return number")
    framework.assert(val >= -1 and val <= 1, "perlin3d should return value in [-1, 1]")

    local val2 = noise.perlin3d(10, 20, 30, 1234)
    framework.assert(type(val2) == "number", "perlin3d with different coords should work")
end)

framework.test("Noise simplex basic", function()
    local val = noise.simplex(0, 0, 1234)
    framework.assert(type(val) == "number", "simplex should return number")
    framework.assert(val >= -1 and val <= 1, "simplex should return value in [-1, 1]")
end)

framework.test("Noise simplex3d", function()
    local val = noise.simplex3d(0, 0, 0, 1234)
    framework.assert(type(val) == "number", "simplex3d should return number")
end)

framework.test("Noise simplexSmooth", function()
    local val = noise.simplexSmooth(0, 0, 1234)
    framework.assert(type(val) == "number", "simplexSmooth should return number")
    framework.assert(val >= -1 and val <= 1, "simplexSmooth should return value in [-1, 1]")
end)

framework.test("Noise cellular basic", function()
    local val = noise.cellular(0, 0, 1234)
    framework.assert(type(val) == "number", "cellular should return number")
end)

framework.test("Noise cellular with distance function", function()
    local val1 = noise.cellular(10, 20, 1234, 0.01, "euclidean")
    local val2 = noise.cellular(10, 20, 1234, 0.01, "manhattan")
    local val3 = noise.cellular(10, 20, 1234, 0.01, "hybrid")

    framework.assert(type(val1) == "number", "cellular with euclidean should work")
    framework.assert(type(val2) == "number", "cellular with manhattan should work")
    framework.assert(type(val3) == "number", "cellular with hybrid should work")
end)

framework.test("Noise cellular with return type", function()
    local val1 = noise.cellular(10, 20, 1234, 0.01, "euclidean", "distance")
    local val2 = noise.cellular(10, 20, 1234, 0.01, "euclidean", "cellValue")
    local val3 = noise.cellular(10, 20, 1234, 0.01, "euclidean", "distance2")

    framework.assert(type(val1) == "number", "cellular with distance return type should work")
    framework.assert(type(val2) == "number", "cellular with cellValue return type should work")
    framework.assert(type(val3) == "number", "cellular with distance2 return type should work")
end)

framework.test("Noise cellular3d", function()
    local val = noise.cellular3d(10, 20, 30, 1234)
    framework.assert(type(val) == "number", "cellular3d should return number")
end)

framework.test("Noise value", function()
    local val = noise.value(0, 0, 1234)
    framework.assert(type(val) == "number", "value should return number")
    framework.assert(val >= -1 and val <= 1, "value should return value in [-1, 1]")
end)

framework.test("Noise valueCubic", function()
    local val = noise.valueCubic(0, 0, 1234)
    framework.assert(type(val) == "number", "valueCubic should return number")
end)

framework.test("Noise fbm basic", function()
    local val = noise.fbm(0, 0, 1234)
    framework.assert(type(val) == "number", "fbm should return number")
end)

framework.test("Noise fbm with options", function()
    local val = noise.fbm(10, 20, 1234, {
        frequency = 0.05,
        octaves = 4,
        lacunarity = 2.0,
        gain = 0.5,
        noiseType = "perlin"
    })
    framework.assert(type(val) == "number", "fbm with options should work")
end)

framework.test("Noise fbm3d", function()
    local val = noise.fbm3d(10, 20, 30, 1234)
    framework.assert(type(val) == "number", "fbm3d should return number")
end)

framework.test("Noise ridged", function()
    local val = noise.ridged(0, 0, 1234)
    framework.assert(type(val) == "number", "ridged should return number")
end)

framework.test("Noise ridged with options", function()
    local val = noise.ridged(10, 20, 1234, {
        frequency = 0.02,
        octaves = 5,
        lacunarity = 2.0,
        gain = 0.5
    })
    framework.assert(type(val) == "number", "ridged with options should work")
end)

framework.test("Noise warp", function()
    local result = noise.warp(10, 20, 1234)
    framework.assert(type(result) == "table", "warp should return table")
    framework.assert(type(result.x) == "number", "warp.x should be number")
    framework.assert(type(result.y) == "number", "warp.y should be number")
end)

framework.test("Noise warp with options", function()
    local result = noise.warp(10, 20, 1234, {
        amplitude = 50,
        frequency = 0.02,
        type = "simplex"
    })
    framework.assert(type(result) == "table", "warp with options should return table")
    framework.assert(type(result.x) == "number", "warp.x with options should be number")
end)

framework.test("Noise normalize", function()
    local val1 = noise.normalize(0, 0, 10)
    framework.assert(math.abs(val1 - 5) < 0.0001, "normalize(0) to [0,10] should be 5")

    local val2 = noise.normalize(-1, 0, 100)
    framework.assert(math.abs(val2 - 0) < 0.0001, "normalize(-1) to [0,100] should be 0")

    local val3 = noise.normalize(1, 0, 100)
    framework.assert(math.abs(val3 - 100) < 0.0001, "normalize(1) to [0,100] should be 100")
end)

framework.test("Noise threshold", function()
    framework.assert(noise.threshold(0.6, 0.5) == true, "0.6 >= 0.5 should be true")
    framework.assert(noise.threshold(0.4, 0.5) == false, "0.4 >= 0.5 should be false")
    framework.assert(noise.threshold(0.5, 0.5) == true, "0.5 >= 0.5 should be true")
end)

framework.test("Noise map", function()
    local val = noise.map(0.5, 0, 1, 0, 100)
    framework.assert(math.abs(val - 50) < 0.0001, "map(0.5, 0, 1, 0, 100) should be 50")

    local val2 = noise.map(0, -1, 1, 0, 200)
    framework.assert(math.abs(val2 - 100) < 0.0001, "map(0, -1, 1, 0, 200) should be 100")
end)

framework.test("Noise clamp", function()
    framework.assert(noise.clamp(5, 0, 10) == 5, "clamp(5, 0, 10) should be 5")
    framework.assert(noise.clamp(-5, 0, 10) == 0, "clamp(-5, 0, 10) should be 0")
    framework.assert(noise.clamp(15, 0, 10) == 10, "clamp(15, 0, 10) should be 10")
end)

framework.test("Noise lerp", function()
    framework.assert(noise.lerp(0, 10, 0) == 0, "lerp(0, 10, 0) should be 0")
    framework.assert(noise.lerp(0, 10, 1) == 10, "lerp(0, 10, 1) should be 10")
    framework.assert(noise.lerp(0, 10, 0.5) == 5, "lerp(0, 10, 0.5) should be 5")
    framework.assert(noise.lerp(-10, 10, 0.5) == 0, "lerp(-10, 10, 0.5) should be 0")
end)

framework.test("Noise smoothstep", function()
    local val1 = noise.smoothstep(0, 1, 0)
    framework.assert(math.abs(val1 - 0) < 0.0001, "smoothstep(0, 1, 0) should be 0")

    local val2 = noise.smoothstep(0, 1, 1)
    framework.assert(math.abs(val2 - 1) < 0.0001, "smoothstep(0, 1, 1) should be 1")

    local val3 = noise.smoothstep(0, 1, 0.5)
    framework.assert(math.abs(val3 - 0.5) < 0.01, "smoothstep(0, 1, 0.5) should be ~0.5")
end)

framework.test("Noise clearCache", function()
    -- Should not error
    local success, err = pcall(function() noise.clearCache() end)
    framework.assert(success, "clearCache should not error")
end)

framework.test("Noise generateGrid", function()
    local grid = noise.generateGrid(0, 0, 10, 10, {seed = 1234})
    framework.assert(type(grid) == "table", "generateGrid should return table")
    framework.assert(type(grid[1]) == "table", "grid rows should be tables")
    framework.assert(type(grid[1][1]) == "number", "grid values should be numbers")
end)

framework.test("Noise generateGrid with different noise types", function()
    local grid1 = noise.generateGrid(0, 0, 5, 5, {noiseType = "perlin"})
    framework.assert(type(grid1) == "table", "perlin grid should work")

    local grid2 = noise.generateGrid(0, 0, 5, 5, {noiseType = "cellular"})
    framework.assert(type(grid2) == "table", "cellular grid should work")

    local grid3 = noise.generateGrid(0, 0, 5, 5, {noiseType = "value"})
    framework.assert(type(grid3) == "table", "value grid should work")
end)

framework.test("Noise generateGrid with fractal", function()
    local grid = noise.generateGrid(0, 0, 5, 5, {
        fractal = "fbm",
        octaves = 4,
        lacunarity = 2.0,
        gain = 0.5
    })
    framework.assert(type(grid) == "table", "fbm grid should work")
end)

framework.test("Noise consistency", function()
    -- Same inputs should give same outputs
    local val1 = noise.perlin(50, 60, 999)
    local val2 = noise.perlin(50, 60, 999)
    framework.assert(val1 == val2, "same inputs should give same output")

    -- Different seeds should give different outputs (usually)
    local val3 = noise.perlin(50, 60, 111)
    framework.assert(val3 ~= val1, "different seeds should give different output")
end)

framework.test("Noise value ranges", function()
    -- Test that various noise functions return values in expected ranges
    local testCoords = {{0, 0}, {100, 100}, {-100, -100}, {500, 500}}

    for _, coords in ipairs(testCoords) do
        local x, y = coords[1], coords[2]

        local perlin = noise.perlin(x, y, 1234)
        framework.assert(perlin >= -1 and perlin <= 1, "perlin should be in [-1, 1]")

        local simplex = noise.simplex(x, y, 1234)
        framework.assert(simplex >= -1 and simplex <= 1, "simplex should be in [-1, 1]")

        local value = noise.value(x, y, 1234)
        framework.assert(value >= -1 and value <= 1, "value should be in [-1, 1]")
    end
end)

framework.test("Noise default parameters", function()
    -- Test that functions work without optional parameters
    local val1 = noise.perlin(10, 20)
    framework.assert(type(val1) == "number", "perlin without seed should work")

    local val2 = noise.simplex(10, 20)
    framework.assert(type(val2) == "number", "simplex without seed should work")

    local val3 = noise.fbm(10, 20)
    framework.assert(type(val3) == "number", "fbm without options should work")
end)

framework.summary()
