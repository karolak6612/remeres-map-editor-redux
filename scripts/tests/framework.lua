local framework = {}

local function log(message)
    print(message)
end

framework.tests_passed = 0
framework.tests_failed = 0

function framework.assert(condition, message)
    if not condition then
        error(message or "Assertion failed")
    end
end

function framework.test(name, func)
    log("Running test: " .. name)
    local status, err = pcall(func)
    if status then
        log("PASS: " .. name)
        framework.tests_passed = framework.tests_passed + 1
    else
        log("FAIL: " .. name .. " - " .. tostring(err))
        framework.tests_failed = framework.tests_failed + 1
    end
end

function framework.summary()
    log("----------------------------------------")
    log("Summary: " .. framework.tests_passed .. " passed, " .. framework.tests_failed .. " failed")
    log("----------------------------------------")
end

function framework.clear_log()
    -- Cannot clear log file without IO library
end

return framework
