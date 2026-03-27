-- Check if app table exists (sanity check)
if not app then
	print("Error: RME Lua API not found.")
	return
end

local palette = {
	header = "#334155",
	card = "#3B4252",
	card_alt = "#475569",
	accent = "#3478D4",
	accent_hover = "#2A62AF",
	text = "#F8FAFC",
	muted = "#CBD5E1",
	subtle = "#94A3B8",
	info = "#2D3748",
	success = "#2F855A",
	warning = "#B7791F",
}

-- The current Lua HTTP wrapper resolves the host and performs a plain HTTP
-- request to the resolved address. This showcase uses a plain-HTTP endpoint
-- that succeeds within that constraint.
local QUOTE_ENDPOINT = "http://68.71.131.28/api/fortune"

local function normalize_http_text(value)
	if type(value) ~= "string" then
		return nil
	end

	return value
		:gsub("\r\n", "\n")
		:gsub("\r", "\n")
		:gsub("\t", "    ")
		:gsub("\n%s+", "\n")
end

local function format_http_failure(response)
	if type(response) ~= "table" then
		return "Request failed. No response data was returned."
	end

	local details = {}
	if response.status and response.status ~= 0 then
		table.insert(details, "status " .. tostring(response.status))
	end
	if response.error and response.error ~= "" then
		table.insert(details, response.error)
	end

	if #details == 0 then
		return "Request failed. The endpoint did not return a usable response."
	end

	return "Request failed (" .. table.concat(details, ", ") .. ")."
end

local function extract_quote_text(response)
	if not json then
		return false, "JSON support is unavailable in this build."
	end

	local ok, payload = pcall(json.decode, response.body or "")
	if not ok or type(payload) ~= "table" then
		return false, "The response arrived, but it could not be parsed as JSON."
	end

	local quote = normalize_http_text(payload.fortune or payload.quote or payload.message)
	if not quote or quote == "" then
		return false, "The endpoint returned JSON, but there was no quote text to display."
	end

	return true, quote
end

local function colorToString(c)
	if not c then
		return "nil"
	end
	return string.format("R:%d G:%d B:%d", c.red, c.green, c.blue)
end

local function getMapInfoText()
	if not app.map then
		return "No map loaded."
	end

	return table.concat({
		string.format("Current Map: %s", app.map.name or "Untitled"),
		string.format("Dimensions: %dx%d", app.map.width, app.map.height),
		string.format("Total Tiles: %d", app.map.tileCount or 0),
	}, "\n")
end

local function getSelectionInfoText()
	local sel = app.selection
	if not sel or sel.isEmpty then
		return "Selection: empty"
	end

	return string.format("Selection: %d tiles ready for inspection or mutation.", sel.size)
end

local createAndShowDialog

createAndShowDialog = function(is_dockable)
	local dlg = Dialog {
		title = "RME Lua UI Showcase",
		width = 760,
		height = 520,
		resizable = true,
		dockable = is_dockable,
		onclose = function()
			print("Lua UI Showcase closed.")
		end
	}

	dlg:panel({ bgcolor = palette.header, padding = 10, margin = 4, height = 34, expand = false })
		dlg:label({
			text = "RME LUA UI SHOWCASE",
			fgcolor = palette.text,
			font_size = 14,
			font_weight = "bold",
			align = "center"
		})
		dlg:label({
			text = "Curated layout, control, preview, environment, and system demos",
			fgcolor = palette.muted,
			font_size = 9,
			align = "center"
		})
	dlg:endpanel()

	dlg:tab { text = "Layout" }
	dlg:box({ orient = "vertical", label = "Layout Patterns", padding = 10, margin = 6, fgcolor = palette.muted, expand = false })
		dlg:label({
			text = "Compact cards, bounded color use, and one deliberate accent example.",
			fgcolor = palette.muted
		})
		dlg:newrow()

		dlg:wrap({ padding = 0 })
			dlg:box({ label = "Primary Action", width = 260, expand = false, padding = 8, margin = 4, fgcolor = palette.muted })
				dlg:label({
					text = "Use one strong action treatment instead of tinting the whole page.",
					fgcolor = palette.muted
				})
				dlg:newrow()
				dlg:button({
					text = "Styled Showcase Button",
					bgcolor = palette.accent,
					fgcolor = palette.text,
					width = 190,
					align = "center",
					rounded = true,
					hover = { bgcolor = palette.accent_hover }
				})
			dlg:endbox()

			dlg:box({ label = "Spacing Rhythm", width = 260, expand = false, padding = 8, margin = 4, fgcolor = palette.muted })
				dlg:label({
					text = "Content defines height. Empty space should be intentional, not accidental.",
					fgcolor = palette.muted
				})
				dlg:newrow()
				dlg:box({ orient = "horizontal", padding = 4, expand = false, align = "center" })
					dlg:panel({ bgcolor = palette.card, width = 64, height = 28, margin = 3, expand = false })
						dlg:label({ text = "4px", fgcolor = palette.text, align = "center", valign = "center" })
					dlg:endpanel()
					dlg:panel({ bgcolor = palette.card_alt, width = 64, height = 28, margin = 3, expand = false })
						dlg:label({ text = "8px", fgcolor = palette.text, align = "center", valign = "center" })
					dlg:endpanel()
					dlg:panel({ bgcolor = palette.info, width = 64, height = 28, margin = 3, expand = false })
						dlg:label({ text = "12px", fgcolor = palette.text, align = "center", valign = "center" })
					dlg:endpanel()
				dlg:endbox()
			dlg:endbox()
		dlg:endwrap()

		dlg:newrow()
		dlg:box({ label = "Palette Samples", orient = "horizontal", padding = 8, margin = 4, expand = false, align = "center", fgcolor = palette.muted })
			dlg:panel({ bgcolor = palette.accent, width = 78, height = 44, margin = 3, expand = false })
				dlg:label({ text = "Accent", fgcolor = palette.text, align = "center", valign = "center" })
			dlg:endpanel()
			dlg:panel({ bgcolor = palette.success, width = 78, height = 44, margin = 3, expand = false })
				dlg:label({ text = "Success", fgcolor = palette.text, align = "center", valign = "center" })
			dlg:endpanel()
			dlg:panel({ bgcolor = palette.info, width = 78, height = 44, margin = 3, expand = false })
				dlg:label({ text = "Info", fgcolor = palette.text, align = "center", valign = "center" })
			dlg:endpanel()
			dlg:panel({ bgcolor = palette.warning, width = 78, height = 44, margin = 3, expand = false })
				dlg:label({ text = "Warn", fgcolor = palette.text, align = "center", valign = "center" })
			dlg:endpanel()
		dlg:endbox()

		dlg:newrow()
		dlg:panel({ bgcolor = palette.card, padding = 10, margin = 4, expand = false })
			dlg:label({ text = "Showcase rule", fgcolor = palette.text, font_weight = "bold" })
			dlg:newrow()
			dlg:label({
				text = "Prefer the dialog's native dark host surface. Use custom colors only for bounded content like swatches, previews, and primary actions.",
				fgcolor = palette.muted
			})
		dlg:endpanel()
	dlg:endbox()

	dlg:tab { text = "Controls" }
	dlg:box({ orient = "vertical", label = "Example Settings", padding = 10, margin = 6, fgcolor = palette.muted, expand = false })
		dlg:label({
			text = "Aligned inputs and compact grouping make native controls easier to scan.",
			fgcolor = palette.muted
		})
		dlg:newrow()
		dlg:input { id = "t_input", label = "Text Entry:", text = "Edit me!", expand = true }
		dlg:number { id = "t_number", label = "Number Spinner:", value = 100, min = 0, max = 500, align = "left" }
		dlg:slider { id = "t_slider", label = "Slider:", value = 50, min = 0, max = 100, expand = true }
		dlg:box({ orient = "horizontal", padding = 0, expand = true })
			dlg:color { id = "t_color", label = "Color Picker:", color = { red = 100, green = 200, blue = 255 } }
			dlg:file { id = "t_file", label = "File Selection:", filename = "test.txt", save = false, expand = true }
		dlg:endbox()
	dlg:endbox()

	dlg:wrap({ padding = 0 })
		dlg:box({ label = "Feature Flags", width = 260, expand = false, padding = 8, margin = 6, fgcolor = palette.muted })
			dlg:label({ text = "Grouped toggles should read like settings, not decoration.", fgcolor = palette.muted })
			dlg:newrow()
			dlg:check { id = "t_check_1", text = "Enable Feature A", selected = true }
			dlg:newrow()
			dlg:check { id = "t_check_2", text = "Enable Feature B", selected = false }
		dlg:endbox()

		dlg:box({ label = "Modes", width = 260, expand = false, padding = 8, margin = 6, fgcolor = palette.muted })
			dlg:label({ text = "Keep mutually exclusive choices in their own compact group.", fgcolor = palette.muted })
			dlg:newrow()
			dlg:radio { id = "t_radio_1", text = "High Performance", selected = true }
			dlg:newrow()
			dlg:radio { id = "t_radio_2", text = "Power Saving", selected = false }
		dlg:endbox()
	dlg:endwrap()

	dlg:box({ orient = "horizontal", padding = 6, margin = 6, expand = false, align = "left" })
		dlg:combobox {
			id = "t_combo",
			label = "Category:",
			options = { "Red", "Green", "Blue", "Alpha" },
			option = "Green"
		}
		dlg:button {
			text = "Debug Current State",
			bgcolor = palette.accent,
			fgcolor = palette.text,
			margin = 0,
			onclick = function(d)
				local data = d.data
				local info = string.format(
					"Input: %s\nNumber: %d\nSlider: %d\nColor: %s\nCheck A: %s\nRadio 1: %s",
					data.t_input,
					data.t_number,
					data.t_slider,
					colorToString(data.t_color),
					tostring(data.t_check_1),
					tostring(data.t_radio_1)
				)
				app.alert(info)
			end
		}
	dlg:endbox()

	dlg:tab { text = "Visuals" }
	dlg:wrap({ padding = 0 })
		dlg:box({ orient = "vertical", label = "Sprite Preview", width = 250, expand = false, padding = 10, margin = 6, fgcolor = palette.muted })
			dlg:box({ orient = "horizontal", padding = 0, expand = false })
				dlg:panel({ padding = 4, width = 98, expand = false })
					dlg:label({ text = "Item 2160", align = "center", fgcolor = palette.muted })
					dlg:newrow()
					dlg:image { id = "img_item", itemid = 2160, width = 64, height = 64, align = "center", smooth = false }
				dlg:endpanel()

				dlg:panel({ padding = 4, width = 98, expand = false })
					dlg:label({ text = "Raw Sprite 100", align = "center", fgcolor = palette.muted })
					dlg:newrow()
					dlg:image { id = "img_sprite", spriteid = 100, width = 64, height = 64, align = "center" }
				dlg:endpanel()
			dlg:endbox()
		dlg:endbox()

		dlg:box({ orient = "vertical", label = "Map Preview", expand = true, padding = 10, margin = 6, fgcolor = palette.muted })
			dlg:mapCanvas { id = "preview_canvas", expand = true, height = 180 }
			dlg:newrow()
			dlg:label({
				text = "Live preview centered on the current map view.",
				align = "center",
				font_size = 8,
				fgcolor = palette.muted
			})
		dlg:endbox()
	dlg:endwrap()

	dlg:wrap({ padding = 0 })
		dlg:box({ orient = "vertical", label = "Custom List", width = 330, expand = false, padding = 10, margin = 6, fgcolor = palette.muted })
			dlg:label({
				text = "Owner-drawn list sample with icons and compact item spacing.",
				fgcolor = palette.muted
			})
			dlg:newrow()
			dlg:list {
				id = "demo_list",
				height = 160,
				expand = true,
				items = {
					{ text = "Legendary Sword", icon = 2160, tooltip = "Deals massive damage" },
					{ text = "Health Potion", icon = 2152, tooltip = "Restores 100 HP" },
					{ text = "Mystery Key", icon = 2148, tooltip = "What does it open?" }
				}
			}
		dlg:endbox()

		dlg:box({ orient = "vertical", label = "Item Grid", expand = true, padding = 10, margin = 6, fgcolor = palette.muted })
			dlg:label({
				text = "Dense preview grid with both item and raw sprite sources.",
				fgcolor = palette.muted
			})
			dlg:newrow()
			dlg:grid {
				id = "demo_grid",
				height = 160,
				cell_size = 48,
				item_size = 32,
				expand = true,
				items = {
					{ tooltip = "Gold", image = Image.fromItemSprite(2148) },
					{ tooltip = "Platinum", image = Image.fromItemSprite(2152) },
					{ tooltip = "Crystal", image = Image.fromItemSprite(2160) },
					{ tooltip = "Raw 100", image = Image.fromSprite(100) },
					{ tooltip = "Raw 101", image = Image.fromSprite(101) }
				}
			}
		dlg:endbox()
	dlg:endwrap()

	dlg:tab { text = "Environment" }
	dlg:box({ orient = "vertical", label = "Current Map", padding = 10, margin = 6, fgcolor = palette.muted, expand = false })
		dlg:panel({ bgcolor = palette.card, padding = 10, margin = 4, expand = false })
			dlg:label({ id = "lbl_map_info", text = getMapInfoText(), fgcolor = palette.text, font_size = 10 })
			dlg:newrow()
			dlg:label({ text = getSelectionInfoText(), fgcolor = palette.muted })
		dlg:endpanel()
	dlg:endbox()

	dlg:wrap({ padding = 0 })
		dlg:box({ orient = "vertical", label = "Read-Only", width = 260, expand = false, padding = 8, margin = 6, fgcolor = palette.muted })
			dlg:label({
				text = "Inspect the current selection without changing the map.",
				fgcolor = palette.muted
			})
			dlg:newrow()
			dlg:button {
				text = "Inspect Selection",
				width = 170,
				onclick = function()
					local sel = app.selection
					if not sel or sel.isEmpty then
						app.alert("Selection is empty.")
					else
						app.alert(string.format("Selected Tiles: %d", sel.size))
					end
				end
			}
		dlg:endbox()

		dlg:box({ orient = "vertical", label = "Mutation Demo", width = 310, expand = false, padding = 8, margin = 6, fgcolor = palette.muted })
			dlg:label({
				text = "Writes item 2785 to each selected tile in the current selection.",
				fgcolor = palette.muted
			})
			dlg:newrow()
			dlg:button {
				text = "Add Blueberry Bushes",
				bgcolor = palette.accent,
				fgcolor = palette.text,
				width = 190,
				onclick = function()
					if not app.map or app.selection.isEmpty then
						app.alert("Select map area first!")
						return
					end

					app.transaction("Add Bushes", function()
						for _, tile in ipairs(app.selection.tiles) do
							tile:addItem(2785, 1)
						end
					end)
				end
			}
		dlg:endbox()
	dlg:endwrap()

	dlg:tab { text = "System" }
	dlg:box({ orient = "vertical", label = "Application", padding = 10, margin = 6, fgcolor = palette.muted, expand = false })
		dlg:label({ text = "Version", fgcolor = palette.muted })
		dlg:newrow()
		dlg:panel({ bgcolor = palette.card, padding = 8, margin = 4, expand = false })
			dlg:label({ text = app.version, fgcolor = palette.text, font_weight = "bold" })
		dlg:endpanel()
	dlg:endbox()

	dlg:box({ orient = "vertical", label = "Window Mode", padding = 10, margin = 6, fgcolor = palette.muted, expand = false })
		local dock_btn_text = is_dockable and "Switch to Floating Window" or "Switch to Dockable Side-Panel"
		local dock_status = is_dockable and "This instance is currently docked into the editor layout." or "This instance is currently floating as a standalone dialog."
		dlg:label({ text = dock_status, fgcolor = palette.muted })
		dlg:newrow()
		dlg:button {
			text = dock_btn_text,
			bgcolor = palette.card_alt,
			fgcolor = palette.text,
			width = 240,
			onclick = function(d)
				d:close()
				createAndShowDialog(not is_dockable)
			end
		}
	dlg:endbox()

	dlg:box({ orient = "vertical", label = "Network Demo", padding = 10, margin = 6, fgcolor = palette.muted, expand = false })
		dlg:label({
			text = "Fetches a quote from a plain HTTP demo endpoint and writes it into a bounded result surface.",
			fgcolor = palette.muted
		})
		dlg:newrow()
		dlg:panel({ id = "http_box", bgcolor = palette.card, height = 96, padding = 10, margin = 4, expand = false })
			dlg:label({
				text = "Result will appear here...",
				id = "lbl_http_res",
				fgcolor = palette.text
			})
		dlg:endpanel()
		dlg:newrow()
		dlg:button {
			text = "Fetch Random Quote",
			bgcolor = palette.accent,
			fgcolor = palette.text,
			width = 170,
			onclick = function(d)
				if not http then
					d:modify {
						lbl_http_res = { text = "HTTP API is unavailable in this build." },
						http_box = { bgcolor = palette.info }
					}
					return
				end

				d:modify {
					lbl_http_res = { text = "Fetching random quote..." },
					http_box = { bgcolor = palette.card_alt }
				}

				local res = http.get(QUOTE_ENDPOINT)
				if not res or not res.ok then
					d:modify {
						lbl_http_res = { text = format_http_failure(res) },
						http_box = { bgcolor = palette.info }
					}
					return
				end

				local ok, content = extract_quote_text(res)
				if not ok then
					d:modify {
						lbl_http_res = { text = content },
						http_box = { bgcolor = palette.info }
					}
					return
				end

				d:modify {
					lbl_http_res = { text = content },
					http_box = { bgcolor = palette.card }
				}
			end
		}
	dlg:endbox()

	dlg:endtabs()

	dlg:separator()
	dlg:box({ orient = "horizontal", padding = 0, margin = 6, expand = false })
		dlg:button {
			text = "Close Showcase",
			bgcolor = palette.card_alt,
			fgcolor = palette.text,
			align = "right",
			onclick = function(d)
				d:close()
			end
		}
	dlg:endbox()

	dlg:show { wait = false }
end

createAndShowDialog(false)

print("Lua UI Showcase executed.")
