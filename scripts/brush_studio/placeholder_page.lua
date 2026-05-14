local PlaceholderPage = {}

function PlaceholderPage.create(title, message)
    local page = {}

    function page.getTitle()
        return title
    end

    function page.isDirty()
        return false
    end

    function page.confirmLeave()
        return true
    end

    function page.onEnter()
    end

    function page.onLeave()
    end

    function page.render_into(dlg)
        dlg:panel({
            padding = 16,
            margin = 4,
            expand = true,
            bgcolor = "#2c2d31",
        })
            dlg:label({
                text = title,
                font_size = 14,
                font_weight = "bold",
                fgcolor = "#f1f1f1",
            })
            dlg:newrow()
            dlg:label({
                text = message,
                fgcolor = "#b8bcc7",
            })
        dlg:endpanel()
    end

    return page
end

return PlaceholderPage
