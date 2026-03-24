-- @Title: Test Geometry API
-- @Description: Verification tests for this API category.
local framework = require("framework")

framework.test("bresenhamLine", function()
    local points = geo.bresenhamLine(0, 0, 5, 5)
    framework.assert(type(points) == "table", "bresenhamLine should return table")
    framework.assert(#points > 0, "Line should have points")
end)

framework.test("bezierCurve", function()
    local controlPoints = {{x=0,y=0}, {x=50,y=100}, {x=100,y=0}}
    local curve = geo.bezierCurve(controlPoints, 10)
    framework.assert(type(curve) == "table", "bezierCurve should return table")
    -- 10 steps means 11 points (0, 0.1, ..., 1.0)
    framework.assert(#curve == 11, "Curve should have 11 points")
end)

framework.test("floodFill", function()
    local grid = {{1,1,1}, {1,0,1}, {1,1,1}}
    local positions = geo.getFloodFillPositions(grid, 2, 2)
    framework.assert(type(positions) == "table", "getFloodFillPositions should return table")
end)

framework.test("shapes", function()
    local circle = geo.circle(0, 0, 5, {filled=true})
    framework.assert(type(circle) == "table", "circle should return table")
    
    local rect = geo.rectangle(0, 0, 5, 5, {filled=true})
    framework.assert(type(rect) == "table", "rectangle should return table")
end)

framework.test("distances", function()
    framework.assert(math.abs(geo.distance(0, 0, 3, 4) - 5) < 0.0001, "Euclidean distance failed")
    framework.assert(geo.distanceSq(0, 0, 3, 4) == 25, "Squared distance failed")
    framework.assert(geo.distanceManhattan(0, 0, 3, 4) == 7, "Manhattan distance failed")
end)

framework.test("point in shape", function()
    framework.assert(geo.pointInCircle(1, 1, 0, 0, 5) == true, "pointInCircle failed")
    framework.assert(geo.pointInCircle(10, 10, 0, 0, 5) == false, "pointInCircle failed")
    framework.assert(geo.pointInRectangle(1, 1, 0, 0, 5, 5) == true, "pointInRectangle failed")
end)

framework.test("random points", function()
    local scatter = geo.randomScatter(0, 0, 100, 100, 10)
    framework.assert(#scatter <= 10, "randomScatter should return points")
    
    local poisson = geo.poissonDiskSampling(0, 0, 100, 100, 10)
    framework.assert(type(poisson) == "table", "poissonDiskSampling should return table")
end)

framework.summary()
