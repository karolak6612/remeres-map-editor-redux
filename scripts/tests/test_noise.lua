-- @Title: Test Noise API
-- @Description: Verification tests for this API category.
local framework = require("framework")

framework.test("basic noise functions", function()
    local x, y, seed = 10, 20, 1234
    framework.assert(type(noise.perlin(x, y, seed)) == "number", "noise.perlin should return number")
    framework.assert(type(noise.simplex(x, y, seed)) == "number", "noise.simplex should return number")
    framework.assert(type(noise.simplexSmooth(x, y, seed)) == "number", "noise.simplexSmooth should return number")
    framework.assert(type(noise.cellular(x, y, seed)) == "number", "noise.cellular should return number")
    framework.assert(type(noise.value(x, y, seed)) == "number", "noise.value should return number")
    framework.assert(type(noise.valueCubic(x, y, seed)) == "number", "noise.valueCubic should return number")
end)

framework.test("3D noise functions", function()
    local x, y, z, seed = 10, 20, 30, 1234
    framework.assert(type(noise.perlin3d(x, y, z, seed)) == "number", "noise.perlin3d should return number")
    framework.assert(type(noise.simplex3d(x, y, z, seed)) == "number", "noise.simplex3d should return number")
end)

framework.test("fractal noise functions", function()
    local x, y, seed = 10, 20, 1234
    framework.assert(type(noise.fbm(x, y, seed)) == "number", "noise.fbm should return number")
    framework.assert(type(noise.ridged(x, y, seed)) == "number", "noise.ridged should return number")
end)

framework.test("noise utilities", function()
    -- Normalizes [-1, 1] to [min, max]. 
    local norm = noise.normalize(0.5, 0, 10)
    framework.assert(math.abs(norm - 7.5) < 0.0001, "noise.normalize failed")
    
    framework.assert(noise.threshold(0.6, 0.5) == true, "noise.threshold failed")
    framework.assert(noise.clamp(15, 0, 10) == 10, "noise.clamp failed")
    framework.assert(noise.lerp(0, 10, 0.5) == 5, "noise.lerp failed")
end)

framework.test("generateGrid", function()
    local grid = noise.generateGrid(0, 0, 10, 10, {seed = 1234})
    framework.assert(type(grid) == "table", "generateGrid should return table")
    -- Check if it's 1-based or 0-based indexing if possible, usually Lua is 1-based
    framework.assert(grid[1] ~= nil, "grid should have elements")
end)

framework.summary()
