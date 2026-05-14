local Contract = {}

local REQUIRED_METHODS = {
    "getTitle",
    "isDirty",
    "confirmLeave",
    "onEnter",
    "onLeave",
    "render_into",
}

function Contract.assert_page(page, label)
    for _, method_name in ipairs(REQUIRED_METHODS) do
        if type(page[method_name]) ~= "function" then
            error(string.format("Brush Studio page '%s' is missing method '%s'.", tostring(label or "?"), method_name))
        end
    end
    return page
end

return Contract
