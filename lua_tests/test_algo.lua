-- @Title: Test Algorithms API
-- @Description: Verification tests for this API category.
local framework = require("framework")

framework.test("generateCave", function()
    local cave = algo.generateCave(20, 20, {seed = 1234})
    framework.assert(type(cave) == "table", "generateCave should return table")
    framework.assert(cave[1] ~= nil, "cave should have rows")
end)

framework.test("generateDungeon", function()
    local result = algo.generateDungeon(30, 30, {seed = 1234})
    framework.assert(type(result) == "table", "generateDungeon should return table")
    framework.assert(type(result.grid) == "table", "result.grid should be table")
    framework.assert(type(result.rooms) == "table", "result.rooms should be table")
end)

framework.test("generateMaze", function()
    local maze = algo.generateMaze(11, 11, {seed = 1234})
    framework.assert(type(maze) == "table", "generateMaze should return table")
end)

framework.test("voronoi", function()
    local points = algo.generateRandomPoints(100, 100, 5, 1234)
    framework.assert(#points == 5, "generateRandomPoints should return 5 points")
    local voronoi = algo.voronoi(100, 100, points)
    framework.assert(type(voronoi) == "table", "voronoi should return table")
end)

framework.test("cellularAutomata", function()
    local grid = {}
    for y = 1, 10 do
        grid[y] = {}
        for x = 1, 10 do
            grid[y][x] = (x + y) % 2
        end
    end
    local result = algo.cellularAutomata(grid, {width=10, height=10, iterations=1})
    framework.assert(type(result) == "table", "cellularAutomata should return table")
end)

framework.test("erosion", function()
    local grid = {}
    for y = 1, 10 do
        grid[y] = {}
        for x = 1, 10 do
            grid[y][x] = 0.5
        end
    end
    local eroded = algo.erode(grid, {iterations=10})
    framework.assert(type(eroded) == "table", "erode should return table")
    
    local thermal = algo.thermalErode(grid, {iterations=2})
    framework.assert(type(thermal) == "table", "thermalErode should return table")
end)

framework.test("smooth", function()
    local grid = {{0,1}, {1,0}}
    local smoothed = algo.smooth(grid, {iterations=1})
    framework.assert(type(smoothed) == "table", "smooth should return table")
end)

framework.summary()
