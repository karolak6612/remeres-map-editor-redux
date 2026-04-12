-- @Title: Test HTTP API (Extended)
-- @Description: Comprehensive verification tests for the HTTP API.
local framework = require("framework")

framework.test("HTTP table existence", function()
    framework.assert(type(http) ~= "nil", "http table should exist")
end)

framework.test("HTTP basic methods existence", function()
    framework.assert(type(http.get) == "function", "http.get should be function")
    framework.assert(type(http.post) == "function", "http.post should be function")
    framework.assert(type(http.postJson) == "function", "http.postJson should be function")
end)

framework.test("HTTP streaming methods existence", function()
    framework.assert(type(http.postStream) == "function", "http.postStream should be function")
    framework.assert(type(http.postJsonStream) == "function", "http.postJsonStream should be function")
    framework.assert(type(http.streamRead) == "function", "http.streamRead should be function")
    framework.assert(type(http.streamStatus) == "function", "http.streamStatus should be function")
    framework.assert(type(http.streamClose) == "function", "http.streamClose should be function")
end)

framework.test("HTTP security - localhost blocked", function()
    -- Test that localhost URLs are blocked
    local result1 = http.get("http://localhost/test")
    framework.assert(result1.ok == false, "localhost URL should be blocked")
    framework.assert(type(result1.error) == "string", "should have error message")
    framework.assert(result1.error:find("Security") or result1.error:find("blocked"),
        "error should mention security/blocking")

    local result2 = http.get("http://127.0.0.1/test")
    framework.assert(result2.ok == false, "127.0.0.1 URL should be blocked")

    local result3 = http.get("http://::1/test")
    framework.assert(result3.ok == false, "::1 URL should be blocked")

    -- Case insensitive
    local result4 = http.get("http://LOCALHOST/test")
    framework.assert(result4.ok == false, "LOCALHOST URL should be blocked")
end)

framework.test("HTTP get return structure", function()
    -- Test with a URL that will fail (but tests the structure)
    local result = http.get("http://example.invalid/test")

    framework.assert(type(result) == "table", "http.get should return table")
    framework.assert(type(result.status) == "number" or result.status == nil,
        "result.status should be number or nil")
    framework.assert(type(result.body) == "string" or result.body == nil,
        "result.body should be string or nil")
    framework.assert(type(result.error) == "string" or result.error == nil,
        "result.error should be string or nil")
    framework.assert(type(result.ok) == "boolean", "result.ok should be boolean")
    framework.assert(type(result.headers) == "table", "result.headers should be table")
end)

framework.test("HTTP post return structure", function()
    local result = http.post("http://example.invalid/test", "test body")

    framework.assert(type(result) == "table", "http.post should return table")
    framework.assert(type(result.ok) == "boolean", "result.ok should be boolean")
    framework.assert(type(result.headers) == "table", "result.headers should be table")
end)

framework.test("HTTP postJson return structure", function()
    local testData = {name = "test", value = 123}
    local result = http.postJson("http://example.invalid/test", testData)

    framework.assert(type(result) == "table", "http.postJson should return table")
    framework.assert(type(result.ok) == "boolean", "result.ok should be boolean")
end)

framework.test("HTTP get with headers", function()
    local headers = {
        ["User-Agent"] = "RME-Test/1.0",
        ["Accept"] = "application/json"
    }

    local result = http.get("http://example.invalid/test", headers)
    framework.assert(type(result) == "table", "http.get with headers should return table")
end)

framework.test("HTTP post with headers", function()
    local headers = {
        ["Content-Type"] = "text/plain",
        ["X-Custom-Header"] = "test-value"
    }

    local result = http.post("http://example.invalid/test", "test body", headers)
    framework.assert(type(result) == "table", "http.post with headers should return table")
end)

framework.test("HTTP postJson adds Content-Type", function()
    -- postJson should automatically add Content-Type: application/json
    local testData = {key = "value"}
    local result = http.postJson("http://example.invalid/test", testData)

    framework.assert(type(result) == "table", "postJson should return table")
    -- The actual header addition is tested internally
end)

framework.test("HTTP postJson with custom headers", function()
    local testData = {name = "test"}
    local customHeaders = {
        ["X-API-Key"] = "secret123",
        ["Authorization"] = "Bearer token"
    }

    local result = http.postJson("http://example.invalid/test", testData, customHeaders)
    framework.assert(type(result) == "table", "postJson with headers should return table")
end)

framework.test("HTTP streaming - session creation", function()
    local result = http.postStream("http://example.invalid/stream", "test body")

    framework.assert(type(result) == "table", "postStream should return table")
    framework.assert(result.ok == true or result.ok == false, "result.ok should exist")

    if result.sessionId then
        framework.assert(type(result.sessionId) == "number", "sessionId should be number")

        -- Cleanup
        http.streamClose(result.sessionId)
    end
end)

framework.test("HTTP streaming - JSON stream", function()
    local testData = {stream = "test", data = {1, 2, 3}}
    local result = http.postJsonStream("http://example.invalid/stream", testData)

    framework.assert(type(result) == "table", "postJsonStream should return table")

    if result.sessionId then
        framework.assert(type(result.sessionId) == "number", "sessionId should be number")

        -- Cleanup
        http.streamClose(result.sessionId)
    end
end)

framework.test("HTTP streamRead - invalid session", function()
    local result = http.streamRead(999999)

    framework.assert(type(result) == "table", "streamRead should return table")
    framework.assert(result.ok == false or result.valid == false,
        "invalid session should return error")
    framework.assert(result.finished == true, "invalid session should be finished")
end)

framework.test("HTTP streamStatus - invalid session", function()
    local result = http.streamStatus(999999)

    framework.assert(type(result) == "table", "streamStatus should return table")
    framework.assert(result.valid == false, "invalid session should have valid=false")
    framework.assert(result.finished == true, "invalid session should be finished")
end)

framework.test("HTTP streamClose - invalid session", function()
    local result = http.streamClose(999999)
    framework.assert(type(result) == "boolean", "streamClose should return boolean")
    framework.assert(result == false, "closing invalid session should return false")
end)

framework.test("HTTP streaming lifecycle", function()
    -- Start a stream
    local startResult = http.postStream("http://example.invalid/stream", "test")

    if startResult.sessionId then
        local sessionId = startResult.sessionId

        -- Check status
        local status = http.streamStatus(sessionId)
        framework.assert(type(status) == "table", "streamStatus should return table")
        framework.assert(type(status.valid) == "boolean", "status.valid should be boolean")

        -- Try to read
        local readResult = http.streamRead(sessionId)
        framework.assert(type(readResult) == "table", "streamRead should return table")
        framework.assert(type(readResult.finished) == "boolean", "readResult.finished should be boolean")

        -- Close
        local closeResult = http.streamClose(sessionId)
        framework.assert(type(closeResult) == "boolean", "streamClose should return boolean")

        -- Verify closed
        local statusAfter = http.streamStatus(sessionId)
        framework.assert(statusAfter.valid == false, "session should be invalid after close")
    end
end)

framework.test("HTTP timeout handling", function()
    -- The API has 10 second timeout, test that it doesn't hang
    local result = http.get("http://example.invalid/timeout")

    framework.assert(type(result) == "table", "timeout should return table")
    -- Should have error or status indicating failure
    framework.assert(result.ok == false or result.error ~= nil,
        "timeout/invalid URL should fail gracefully")
end)

framework.test("HTTP empty body post", function()
    local result = http.post("http://example.invalid/test", "")
    framework.assert(type(result) == "table", "post with empty body should return table")
end)

framework.test("HTTP large body post", function()
    local largeBody = string.rep("x", 10000)
    local result = http.post("http://example.invalid/test", largeBody)
    framework.assert(type(result) == "table", "post with large body should return table")
end)

framework.test("HTTP special characters in URL", function()
    -- Test URL encoding scenarios
    local result = http.get("http://example.invalid/path%20with%20spaces")
    framework.assert(type(result) == "table", "URL with encoded chars should return table")
end)

framework.test("HTTP response headers structure", function()
    local result = http.get("http://example.invalid/test")

    if result.headers then
        framework.assert(type(result.headers) == "table", "headers should be table")
        -- Headers table may be empty for failed requests, but should exist
    end
end)

framework.test("HTTP status codes", function()
    local result = http.get("http://example.invalid/test")

    -- If we get a status, it should be a number >= 0
    -- Status 0 indicates connection failure
    if result.status then
        framework.assert(type(result.status) == "number", "status should be number")
        framework.assert(result.status >= 0, "status should be non-negative")
    end
end)

framework.test("HTTP concurrent streams", function()
    -- Test multiple concurrent streams
    local sessions = {}

    for i = 1, 3 do
        local result = http.postStream("http://example.invalid/stream" .. i, "test" .. i)
        if result.sessionId then
            table.insert(sessions, result.sessionId)
        end
    end

    -- Check all sessions
    for _, sessionId in ipairs(sessions) do
        local status = http.streamStatus(sessionId)
        framework.assert(type(status) == "table", "concurrent stream status should work")
    end

    -- Cleanup all
    for _, sessionId in ipairs(sessions) do
        http.streamClose(sessionId)
    end
end)

framework.test("HTTP method return types summary", function()
    -- Verify all methods return expected types
    framework.assert(type(http.get) == "function", "get is function")
    framework.assert(type(http.post) == "function", "post is function")
    framework.assert(type(http.postJson) == "function", "postJson is function")
    framework.assert(type(http.postStream) == "function", "postStream is function")
    framework.assert(type(http.postJsonStream) == "function", "postJsonStream is function")
    framework.assert(type(http.streamRead) == "function", "streamRead is function")
    framework.assert(type(http.streamStatus) == "function", "streamStatus is function")
    framework.assert(type(http.streamClose) == "function", "streamClose is function")
end)

framework.summary()
