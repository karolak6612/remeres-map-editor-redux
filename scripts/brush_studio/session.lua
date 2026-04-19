local Session = {}

function Session.new()
    return {
        lastSelectedRaw = 0,
        recentItems = {},
        lastSavedBorder = 0,
        lastSavedGround = "",
        lastSavedComposedBrush = "",
        lastSavedPaletteEntry = "",
        selectedBorderForComposer = 0,
        selectedGroundForComposer = "",
        pageDirty = {
            borders = false,
            grounds = false,
            composer = false,
            palette = false,
        },
    }
end

return Session
