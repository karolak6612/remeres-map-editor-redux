-- @Title: Test HTTP API
-- @Description: Verification tests for this API category.
local framework = require("framework")

framework.test("http basic existence", function()

    framework.assert(type(http) ~= "nil", "http table should exist")
    framework.assert(type(http.get) == "function", "http.get should be function")
    framework.assert(type(http.post) == "function", "http.post should be function")
    framework.assert(type(http.postJson) == "function", "http.postJson should be function")
end)

framework.test("http streaming presence", function()
    framework.assert(type(http.postStream) == "function", "http.postStream should be function")
    framework.assert(type(http.streamRead) == "function", "http.streamRead should be function")
    framework.assert(type(http.streamStatus) == "function", "http.streamStatus should be function")
    framework.assert(type(http.streamClose) == "function", "http.streamClose should be function")
end)

framework.summary()
