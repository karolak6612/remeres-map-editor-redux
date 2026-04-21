local Session = {}

function Session.new()
    return {
        lastSelectedRaw = 0,
        recentItems = {},
        lastSavedBorder = 0,
        lastSavedGround = "",
        lastSavedComposedBrush = "",
        lastSavedPaletteEntry = "",
        lastSavedWall = "",
        selectedBorderForComposer = 0,
        selectedGroundForComposer = "",
        selectedWallCreator = "",
        pageDirty = {
            borders = false,
            grounds = false,
            composer = false,
            palette = false,
            walls = false,
        },
    }
end

return Session
