#include "app/main.h"
#include "palette/controls/virtual_brush_grid.h"
#include "ui/gui.h"
#include "rendering/core/graphics.h"
#include "brushes/raw/raw_brush.h"

#include <glad/glad.h>

#include <nanovg.h>
#include <nanovg_gl.h>

#include "util/nvg_utils.h"
#include "ui/theme.h"

#include <spdlog/spdlog.h>

wxDEFINE_EVENT(EVT_VIRTUAL_BRUSH_GRID_SELECTED, wxCommandEvent);

namespace {
	static constexpr float GROW_FACTOR = 2.0f;
	static constexpr float SHADOW_ALPHA_BASE = 20.0f;
	static constexpr float SHADOW_ALPHA_FACTOR = 64.0f;
	static constexpr float SHADOW_BLUR_BASE = 6.0f;
	static constexpr float SHADOW_BLUR_FACTOR = 4.0f;
	static constexpr int TIMER_INTERVAL = 16;
	static constexpr float INTER_THRESHOLD = 0.01f;
	static constexpr float INTER_FACTOR = 0.2f;
	static constexpr int RAW_GRID_LABEL_HEIGHT = 14;
	static constexpr float QUEUED_DIM_ALPHA = 0.38f;
}

VirtualBrushGrid::VirtualBrushGrid(wxWindow* parent, const TilesetCategory* _tileset, RenderSize rsz) :
	NanoVGCanvas(parent, wxID_ANY, wxVSCROLL | wxWANTS_CHARS),
	icon_size(rsz),
	selected_index(-1),
	hover_index(-1),
	columns(1),
	item_size(0),
	padding(4),
	m_animTimer(this) {

	tileset = _tileset;
	palette_type = _tileset ? _tileset->getType() : TILESET_UNKNOWN;

	if (icon_size == RENDER_SIZE_16x16) {
		item_size = 18;
	} else {
		item_size = GRID_ITEM_SIZE_BASE + 2; // + borders
	}

	Bind(wxEVT_LEFT_DOWN, &VirtualBrushGrid::OnMouseDown, this);
	Bind(wxEVT_RIGHT_DOWN, &VirtualBrushGrid::OnMouseRightDown, this);
	Bind(wxEVT_MOTION, &VirtualBrushGrid::OnMotion, this);
	Bind(wxEVT_SIZE, &VirtualBrushGrid::OnSize, this);
	Bind(wxEVT_TIMER, &VirtualBrushGrid::OnTimer, this);

	UpdateLayout();
}

VirtualBrushGrid::VirtualBrushGrid(wxWindow* parent, const EntryList* _entries, TilesetCategoryType _palette_type, RenderSize rsz) :
	NanoVGCanvas(parent, wxID_ANY, wxVSCROLL | wxWANTS_CHARS),
	icon_size(rsz),
	selected_index(-1),
	hover_index(-1),
	columns(1),
	item_size(0),
	padding(4),
	m_animTimer(this) {

	entries = _entries;
	palette_type = _palette_type;

	if (icon_size == RENDER_SIZE_16x16) {
		item_size = 18;
	} else {
		item_size = GRID_ITEM_SIZE_BASE + 2; // + borders
	}

	Bind(wxEVT_LEFT_DOWN, &VirtualBrushGrid::OnMouseDown, this);
	Bind(wxEVT_RIGHT_DOWN, &VirtualBrushGrid::OnMouseRightDown, this);
	Bind(wxEVT_MOTION, &VirtualBrushGrid::OnMotion, this);
	Bind(wxEVT_SIZE, &VirtualBrushGrid::OnSize, this);
	Bind(wxEVT_TIMER, &VirtualBrushGrid::OnTimer, this);

	UpdateLayout();
}

VirtualBrushGrid::~VirtualBrushGrid() = default;

void VirtualBrushGrid::SetDisplayMode(DisplayMode mode) {
	if (display_mode != mode) {
		display_mode = mode;
		UpdateLayout();
		Refresh();
	}
}

void VirtualBrushGrid::SetEntryList(const EntryList* next_entries, TilesetCategoryType next_palette_type) {
	entries = next_entries;
	tileset = nullptr;
	palette_type = next_palette_type;
	ClearSelection();
	hover_index = -1;
	m_utf8NameCache.clear();
	UpdateLayout();
	Refresh();
}

void VirtualBrushGrid::SetIconSize(RenderSize size) {
	if (icon_size == size) {
		return;
	}

	icon_size = size;
	if (icon_size == RENDER_SIZE_16x16) {
		item_size = 18;
	} else {
		item_size = GRID_ITEM_SIZE_BASE + 2; // + borders
	}

	UpdateLayout();
	Refresh();
}

int VirtualBrushGrid::ItemCount() const {
	if (entries) {
		return static_cast<int>(entries->size());
	}
	return tileset ? static_cast<int>(tileset->size()) : 0;
}

Brush* VirtualBrushGrid::ItemBrush(int index) const {
	if (index < 0 || index >= ItemCount()) {
		return nullptr;
	}
	if (entries) {
		return (*entries)[static_cast<size_t>(index)].brush;
	}
	return tileset ? tileset->brushlist[static_cast<size_t>(index)] : nullptr;
}

TilesetCategoryType VirtualBrushGrid::ItemPaletteType(int index) const {
	if (entries && index >= 0 && index < ItemCount()) {
		const auto entry_palette = (*entries)[static_cast<size_t>(index)].palette_type;
		if (entry_palette != TILESET_UNKNOWN) {
			return entry_palette;
		}
	}
	if (palette_type != TILESET_UNKNOWN) {
		return palette_type;
	}
	return tileset ? tileset->getType() : TILESET_UNKNOWN;
}

int VirtualBrushGrid::ItemTilesetIndex(int index) const {
	if (!entries || index < 0 || index >= ItemCount()) {
		return -1;
	}
	return (*entries)[static_cast<size_t>(index)].tileset_index;
}

bool VirtualBrushGrid::ShowRawIdLabel(int index) const {
	return display_mode == DisplayMode::Grid && ItemPaletteType(index) == TILESET_RAW;
}

int VirtualBrushGrid::GridCellHeight() const {
	if (!entries) {
		return item_size + (palette_type == TILESET_RAW ? RAW_GRID_LABEL_HEIGHT : 0);
	}

	for (int index = 0; index < ItemCount(); ++index) {
		if (ShowRawIdLabel(index)) {
			return item_size + RAW_GRID_LABEL_HEIGHT;
		}
	}
	return item_size;
}

void VirtualBrushGrid::UpdateLayout() {
	int width = GetClientSize().x;
	if (width <= 0) {
		width = 200; // Default
	}

	if (display_mode == DisplayMode::List) {
		columns = 1;
		int rows = ItemCount();
		int contentHeight = rows * LIST_ROW_HEIGHT + padding;
		UpdateScrollbar(contentHeight);
	} else {
		columns = std::max(1, (width - padding) / (item_size + padding));
		int rows = (ItemCount() + columns - 1) / columns;
		int contentHeight = rows * (GridCellHeight() + padding) + padding;
		UpdateScrollbar(contentHeight);
	}
}

wxSize VirtualBrushGrid::DoGetBestClientSize() const {
	return FromDIP(wxSize(200, 300));
}

void VirtualBrushGrid::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	// Calculate visible range
	int scrollPos = GetScrollPosition();
	int rowHeight = (display_mode == DisplayMode::List) ? LIST_ROW_HEIGHT : (GridCellHeight() + padding);
	int startRow = scrollPos / rowHeight;
	int endRow = (scrollPos + height + rowHeight - 1) / rowHeight + 1;

	int startIdx = startRow * columns;
	int endIdx = std::min(ItemCount(), endRow * columns);

	// Draw visible items
	for (int i = startIdx; i < endIdx; ++i) {
		DrawBrushItem(vg, i, GetItemRect(i));
	}
}

void VirtualBrushGrid::DrawBrushItem(NVGcontext* vg, int i, const wxRect& rect) {
	const bool showRawIdLabel = ShowRawIdLabel(i) && ItemBrush(i) && ItemBrush(i)->is<RAWBrush>();
	const int cardHeight = (display_mode == DisplayMode::Grid && showRawIdLabel) ? item_size : rect.height;
	const wxRect cardRect(rect.x, rect.y, rect.width, cardHeight);
	const bool isSelected = IsIndexSelected(i);
	const bool isQueued = IsQueuedBrush(ItemBrush(i));

	float x = static_cast<float>(cardRect.x);
	float y = static_cast<float>(cardRect.y);
	float w = static_cast<float>(cardRect.width);
	float h = static_cast<float>(cardRect.height);

	// Animation scaling
	if (i == hover_index) {
		float grow = GROW_FACTOR * hover_anim;
		x -= grow;
		y -= grow;
		w += grow * 2.0f;
		h += grow * 2.0f;
	}

	// Shadow / Glow
	if (isSelected) {
		// Glow for selected
		NVGpaint shadowPaint = nvgBoxGradient(vg, x, y, w, h, 4.0f, 10.0f, nvgRGBA(100, 150, 255, 128), nvgRGBA(0, 0, 0, 0));
		nvgBeginPath(vg);
		nvgRect(vg, x - 10, y - 10, w + 20, h + 20);
		nvgRoundedRect(vg, x, y, w, h, 4.0f);
		nvgPathWinding(vg, NVG_HOLE);
		nvgFillPaint(vg, shadowPaint);
		nvgFill(vg);
	} else if (i == hover_index) {
		// Animated shadow for hover
		float shadowAlpha = SHADOW_ALPHA_FACTOR * hover_anim + SHADOW_ALPHA_BASE;
		float shadowBlur = SHADOW_BLUR_BASE + SHADOW_BLUR_FACTOR * hover_anim;
		NVGpaint shadowPaint = nvgBoxGradient(vg, x, y + 2, w, h, 4.0f, shadowBlur, nvgRGBA(0, 0, 0, static_cast<int>(shadowAlpha)), nvgRGBA(0, 0, 0, 0));
		nvgBeginPath(vg);
		nvgRect(vg, x - 10, y - 10, w + 20, h + 20);
		nvgRoundedRect(vg, x, y, w, h, 4.0f);
		nvgPathWinding(vg, NVG_HOLE);
		nvgFillPaint(vg, shadowPaint);
		nvgFill(vg);
	}

	// Card background
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, w, h, 4.0f);

	if (isSelected) {
		NVGcolor selCol = NvgUtils::ToNvColor(Theme::Get(Theme::Role::Accent));
		selCol.a = 1.0f; // Force opaque for background
		nvgFillColor(vg, selCol);
	} else if (i == hover_index) {
		nvgFillColor(vg, NvgUtils::ToNvColor(Theme::Get(Theme::Role::CardBaseHover)));
	} else {
		// Normal - theme card base
		nvgFillColor(vg, NvgUtils::ToNvColor(Theme::Get(Theme::Role::CardBase)));
	}
	nvgFill(vg);

	// Selection border
	if (isSelected) {
		nvgBeginPath(vg);
		nvgRoundedRect(vg, x + 0.5f, y + 0.5f, w - 1.0f, h - 1.0f, 4.0f);
		nvgStrokeColor(vg, NvgUtils::ToNvColor(Theme::Get(Theme::Role::Accent)));
		nvgStrokeWidth(vg, 2.0f);
		nvgStroke(vg);
	}

	// Draw brush sprite
	Brush* brush = ItemBrush(i);
	if (brush) {
		Sprite* spr = brush->getSprite();
		if (!spr) {
			spr = g_gui.gfx.getSprite(brush->getLookID());
		}

		if (!spr) {
			// Keep drawing name/id even if sprite is missing.
			spr = nullptr;
		}

		const int baseIconSize = (icon_size == RENDER_SIZE_16x16) ? 16 : GRID_ITEM_SIZE_BASE;
		const int iconSize = (display_mode == DisplayMode::List) ? baseIconSize : (item_size - 2 * ICON_OFFSET);
		const int iconX = cardRect.x + ICON_OFFSET;
		const int iconY = (display_mode == DisplayMode::List)
			? cardRect.y + std::max(0, (cardRect.height - iconSize) / 2)
			: cardRect.y + ICON_OFFSET;

		if (spr) {
			int tex = GetOrCreateSpriteTexture(vg, spr);
			if (tex > 0) {
				NVGpaint imgPaint = nvgImagePattern(vg, static_cast<float>(iconX), static_cast<float>(iconY), static_cast<float>(iconSize), static_cast<float>(iconSize), 0.0f, tex, 1.0f);

				nvgBeginPath(vg);
				nvgRoundedRect(vg, static_cast<float>(iconX), static_cast<float>(iconY), static_cast<float>(iconSize), static_cast<float>(iconSize), 3.0f);
				nvgFillPaint(vg, imgPaint);
				nvgFill(vg);
			}
		}

		if (display_mode == DisplayMode::List) {
			nvgFontSize(vg, 14.0f);
			nvgFontFace(vg, "sans");
			nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
			nvgFillColor(vg, NvgUtils::ToNvColor(Theme::Get(Theme::Role::Text)));

			auto it = m_utf8NameCache.find(brush);
			if (it == m_utf8NameCache.end()) {
				m_utf8NameCache[brush] = std::string(wxstr(brush->getName()).ToUTF8());
				it = m_utf8NameCache.find(brush);
			}
			const float textX = static_cast<float>(cardRect.x + iconSize + ICON_OFFSET + 8);
			nvgText(vg, textX, cardRect.y + cardRect.height / 2.0f, it->second.c_str(), nullptr);
		} else {
			if (showRawIdLabel) {
				const uint16_t id = brush->as<RAWBrush>()->getItemID();
				const std::string label = std::to_string(id);
				nvgFontSize(vg, 11.0f);
				nvgFontFace(vg, "sans");
				nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
				const auto role = isSelected ? Theme::Role::Text : Theme::Role::TextSubtle;
				nvgFillColor(vg, NvgUtils::ToNvColor(Theme::Get(role)));
				nvgText(vg, rect.x + rect.width / 2.0f, rect.y + item_size + RAW_GRID_LABEL_HEIGHT / 2.0f, label.c_str(), nullptr);
			}
		}
	}

	if (isQueued) {
		nvgBeginPath(vg);
		nvgRoundedRect(vg, x, y, w, h, 4.0f);
		nvgFillColor(vg, nvgRGBA(12, 12, 14, static_cast<unsigned char>(255.0f * QUEUED_DIM_ALPHA)));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgCircle(vg, x + w - 9.0f, y + 9.0f, 4.0f);
		nvgFillColor(vg, nvgRGBA(255, 196, 64, 255));
		nvgFill(vg);
	}
}

wxRect VirtualBrushGrid::GetItemRect(int index) const {
	if (display_mode == DisplayMode::List) {
		int width = GetClientSize().x - 2 * padding;
		return wxRect(padding, padding + index * LIST_ROW_HEIGHT, width, LIST_ROW_HEIGHT);
	} else {
		int row = index / columns;
		int col = index % columns;

		return wxRect(
			padding + col * (item_size + padding),
			padding + row * (GridCellHeight() + padding),
			item_size,
			GridCellHeight()
		);
	}
}

int VirtualBrushGrid::HitTest(int x, int y) const {
	int scrollPos = GetScrollPosition();
	int realY = y + scrollPos;
	int realX = x;

	if (display_mode == DisplayMode::List) {
		int row = (realY - padding) / LIST_ROW_HEIGHT;

		if (row < 0 || row >= ItemCount()) {
			return -1;
		}

		int index = row;
		// Check horizontal bounds properly
		int width = GetClientSize().x - 2 * padding;
		if (realX >= padding && realX <= GetClientSize().x - padding) {
			return index;
		}
		return -1;
	} else {
		int col = (realX - padding) / (item_size + padding);
		int row = (realY - padding) / (GridCellHeight() + padding);

		if (col < 0 || col >= columns || row < 0) {
			return -1;
		}

		int index = row * columns + col;
		if (index >= 0 && index < ItemCount()) {
			wxRect rect = GetItemRect(index);
			// Adjust rect to scroll position for contains check
			rect.y -= scrollPos;
			if (rect.Contains(x, y)) {
				return index;
			}
		}
		return -1;
	}
}

void VirtualBrushGrid::OnMouseDown(wxMouseEvent& event) {
	int index = HitTest(event.GetX(), event.GetY());
	if (index == -1) {
		if (HasSelection()) {
			ClearSelection();
			g_gui.SetPaletteMultiSelectionCount(0);
			g_gui.SelectBrush();
			Refresh();
		}
		return;
	}

	if (event.ShiftDown() && selection_anchor != -1) {
		SelectRangeTo(index);
	} else if (event.ControlDown()) {
		ToggleIndexSelection(index);
	} else {
		SelectSingleIndex(index);
	}

	NotifySelectionChanged();
	Refresh();
}

void VirtualBrushGrid::OnMouseRightDown(wxMouseEvent& event) {
	const int index = HitTest(event.GetX(), event.GetY());
	if (index != -1 && !IsIndexSelected(index)) {
		SelectSingleIndex(index);
		NotifySelectionChanged();
		Refresh();
	}

	wxContextMenuEvent menu_event(wxEVT_CONTEXT_MENU);
	menu_event.SetEventObject(this);
	menu_event.SetPosition(ClientToScreen(event.GetPosition()));
	ProcessWindowEvent(menu_event);
}

void VirtualBrushGrid::OnMotion(wxMouseEvent& event) {
	int index = HitTest(event.GetX(), event.GetY());

	if (index != hover_index) {
		hover_index = index;
		if (index != -1) {
			hover_anim = 0.0f; // Reset animation for new target
		}
		if (!m_animTimer.IsRunning()) {
			m_animTimer.Start(TIMER_INTERVAL);
		}
		Refresh();
	} else if (hover_index != -1 && !m_animTimer.IsRunning()) {
		m_animTimer.Start(TIMER_INTERVAL);
	}

	// Tooltip
	if (index != -1) {
		Brush* brush = ItemBrush(index);
		if (brush) {
			wxString tip = wxstr(brush->getName());
			if (GetToolTipText() != tip) {
				SetToolTip(tip);
			}
		}
	} else {
		UnsetToolTip();
	}

	event.Skip();
}

void VirtualBrushGrid::OnTimer(wxTimerEvent& event) {
	float target = (hover_index != -1) ? 1.0f : 0.0f;
	if (std::abs(hover_anim - target) > INTER_THRESHOLD) {
		hover_anim += (target - hover_anim) * INTER_FACTOR;
		Refresh();
	} else {
		hover_anim = target;
		if (hover_index == -1) {
			m_animTimer.Stop();
		}
	}
}

void VirtualBrushGrid::OnSize(wxSizeEvent& event) {
	UpdateLayout();
	Refresh();
	event.Skip();
}

void VirtualBrushGrid::SelectFirstBrush() {
	if (ItemCount() > 0) {
		SelectSingleIndex(0);
		g_gui.SetPaletteMultiSelectionCount(1);
		Refresh();
	}
}

Brush* VirtualBrushGrid::GetSelectedBrush() const {
	return ItemBrush(selected_index);
}

bool VirtualBrushGrid::SelectBrush(const Brush* brush) {
	return SelectBrush(brush, SelectionScrollBehavior::EnsureVisible);
}

bool VirtualBrushGrid::SelectBrush(const Brush* brush, SelectionScrollBehavior scroll_behavior) {
	const int count = ItemCount();
	for (int i = 0; i < count; ++i) {
		if (ItemBrush(i) == brush) {
			SelectSingleIndex(i);
			g_gui.SetPaletteMultiSelectionCount(1);

			wxRect rect = GetItemRect(selected_index);
			if (scroll_behavior == SelectionScrollBehavior::AlignToTop) {
				SetScrollPosition(std::max(0, rect.y - padding));
			} else if (scroll_behavior == SelectionScrollBehavior::EnsureVisible) {
				int scrollPos = GetScrollPosition();
				int clientHeight = GetClientSize().y;

				if (rect.y < scrollPos) {
					SetScrollPosition(std::max(0, rect.y - padding));
				} else if (rect.y + rect.height > scrollPos + clientHeight) {
					SetScrollPosition(rect.y + rect.height - clientHeight + padding);
				}
			}

			Refresh();
			return true;
		}
	}
	ClearSelection();
	g_gui.SetPaletteMultiSelectionCount(0);
	Refresh();
	return false;
}

std::vector<VirtualBrushGrid::Entry> VirtualBrushGrid::GetSelectedEntries() const {
	std::vector<int> sorted_indices(selected_indices.begin(), selected_indices.end());
	std::ranges::sort(sorted_indices);

	std::vector<Entry> result;
	result.reserve(sorted_indices.size());
	for (const int index : sorted_indices) {
		if (Brush* brush = ItemBrush(index)) {
			result.push_back(Entry {
				.brush = brush,
				.palette_type = ItemPaletteType(index),
				.tileset_index = ItemTilesetIndex(index),
			});
		}
	}
	return result;
}

std::optional<VirtualBrushGrid::Entry> VirtualBrushGrid::GetSelectedEntry() const {
	if (selected_index < 0 || selected_index >= ItemCount()) {
		return std::nullopt;
	}

	Brush* brush = ItemBrush(selected_index);
	if (!brush) {
		return std::nullopt;
	}

	return Entry {
		.brush = brush,
		.palette_type = ItemPaletteType(selected_index),
		.tileset_index = ItemTilesetIndex(selected_index),
	};
}

std::vector<Brush*> VirtualBrushGrid::GetSelectedBrushes() const {
	std::vector<int> sorted_indices(selected_indices.begin(), selected_indices.end());
	std::ranges::sort(sorted_indices);

	std::vector<Brush*> brushes;
	brushes.reserve(sorted_indices.size());
	for (const int index : sorted_indices) {
		if (Brush* brush = ItemBrush(index)) {
			brushes.push_back(brush);
		}
	}
	return brushes;
}

std::vector<ServerItemId> VirtualBrushGrid::GetSelectedItemIds() const {
	std::vector<ServerItemId> item_ids;
	item_ids.reserve(selected_indices.size());

	for (Brush* brush : GetSelectedBrushes()) {
		if (!brush || !brush->is<RAWBrush>()) {
			continue;
		}
		item_ids.push_back(brush->as<RAWBrush>()->getItemID());
	}
	return item_ids;
}

bool VirtualBrushGrid::HasSelection() const {
	return !selected_indices.empty();
}

size_t VirtualBrushGrid::GetSelectionCount() const {
	return selected_indices.size();
}

bool VirtualBrushGrid::IsIndexSelected(int index) const {
	return selected_indices.contains(index);
}

bool VirtualBrushGrid::IsQueuedBrush(const Brush* brush) const {
	if (!brush || !brush->is<RAWBrush>()) {
		return false;
	}

	return g_gui.GetTilesetMoveQueue().IsQueued(brush->as<RAWBrush>()->getItemID());
}

void VirtualBrushGrid::ClearSelection() {
	selected_indices.clear();
	selected_index = -1;
	selection_anchor = -1;
}

void VirtualBrushGrid::SelectSingleIndex(int index) {
	ClearSelection();
	if (index < 0 || index >= ItemCount()) {
		return;
	}

	selected_indices.insert(index);
	selected_index = index;
	selection_anchor = index;
}

void VirtualBrushGrid::ToggleIndexSelection(int index) {
	if (index < 0 || index >= ItemCount()) {
		return;
	}

	if (selected_indices.contains(index)) {
		selected_indices.erase(index);
		if (selected_index == index) {
			selected_index = selected_indices.empty() ? -1 : *selected_indices.begin();
		}
		if (selection_anchor == index) {
			selection_anchor = selected_index;
		}
		return;
	}

	selected_indices.insert(index);
	selected_index = index;
	selection_anchor = index;
}

void VirtualBrushGrid::SelectRangeTo(int index) {
	if (index < 0 || index >= ItemCount()) {
		return;
	}
	if (selection_anchor == -1) {
		SelectSingleIndex(index);
		return;
	}

	selected_indices.clear();
	const int begin = std::min(selection_anchor, index);
	const int end = std::max(selection_anchor, index);
	for (int current = begin; current <= end; ++current) {
		selected_indices.insert(current);
	}
	selected_index = index;
}

void VirtualBrushGrid::NotifySelectionChanged() {
	wxWindow* w = GetParent();
	while (w) {
		PaletteWindow* pw = dynamic_cast<PaletteWindow*>(w);
		if (pw) {
			g_gui.ActivatePalette(pw);
			break;
		}
		w = w->GetParent();
	}

	g_gui.SetPaletteMultiSelectionCount(GetSelectionCount());

	if (GetSelectionCount() == 1) {
		if (Brush* brush = GetSelectedBrush()) {
			g_gui.SelectBrush(brush, ItemPaletteType(selected_index));
		}
	} else {
		g_gui.SelectBrush();
	}

	wxCommandEvent selected_event(EVT_VIRTUAL_BRUSH_GRID_SELECTED, GetId());
	selected_event.SetEventObject(this);
	selected_event.SetInt(selected_index);
	selected_event.SetExtraLong(selected_index >= 0 ? ItemTilesetIndex(selected_index) : -1);
	ProcessWindowEvent(selected_event);
}
