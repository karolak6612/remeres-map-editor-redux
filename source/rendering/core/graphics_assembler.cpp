#include "rendering/core/graphics_assembler.h"

#include "item_definitions/formats/dat/dat_catalog.h"
#include "rendering/core/animator.h"
#include "rendering/core/game_sprite.h"
#include "rendering/core/graphics.h"
#include "rendering/core/normal_image.h"
#include "rendering/core/sprite_archive.h"
#include "rendering/core/sprite_preloader.h"

#include <algorithm>
#include <format>
#include <memory>
#include <wx/string.h>

namespace {
	[[nodiscard]] size_t imageSpaceSize(const DatCatalog& catalog) {
		return std::max<size_t>(1, static_cast<size_t>(catalog.max_sprite_id) + 1);
	}

	bool validateCatalog(const DatCatalog& catalog, wxString& error) {
		if (catalog.max_sprite_id >= MAX_SPRITES) {
			error = wxString::FromUTF8(std::format(
				"DAT catalog references sprite id {} which exceeds MAX_SPRITES={}.",
				catalog.max_sprite_id,
				MAX_SPRITES));
			return false;
		}

		for (uint32_t client_id = 100; client_id <= catalog.lastEntryId(); ++client_id) {
			const auto* entry = catalog.entry(client_id);
			if (!entry) {
				error = wxString::FromUTF8(std::format("Missing DAT catalog entry for client id {}.", client_id));
				return false;
			}
		}

		return true;
	}
}

void GraphicsAssembler::installAnimation(GameSprite& sprite, const DatCatalogEntry& entry) {
	if (!entry.animation.has_value()) {
		sprite.animator.reset();
		return;
	}

	const auto& animation = *entry.animation;
	sprite.animator = std::make_unique<Animator>(entry.frames, animation.start_frame, animation.loop_count, animation.asynchronous);
	for (size_t frame_index = 0; frame_index < animation.frame_durations.size(); ++frame_index) {
		FrameDuration* duration = sprite.animator->getFrameDuration(static_cast<int>(frame_index));
		duration->setValues(static_cast<int>(animation.frame_durations[frame_index].minimum), static_cast<int>(animation.frame_durations[frame_index].maximum));
	}
	sprite.animator->reset();
}

bool GraphicsAssembler::installSpriteEntry(GraphicManager& manager, const DatCatalogEntry& entry, std::vector<std::string>& warnings) {
	auto sprite = std::make_unique<GameSprite>();
	auto* sprite_ptr = sprite.get();

	sprite_ptr->meta.id = entry.client_id;
	sprite_ptr->meta.height = entry.height;
	sprite_ptr->meta.width = entry.width;
	sprite_ptr->meta.layers = entry.layers;
	sprite_ptr->meta.pattern_x = entry.pattern_x;
	sprite_ptr->meta.pattern_y = entry.pattern_y;
	sprite_ptr->meta.pattern_z = entry.pattern_z;
	sprite_ptr->meta.frames = entry.frames;
	sprite_ptr->meta.numsprites = entry.numsprites;
	sprite_ptr->meta.draw_height = entry.draw_height;
	sprite_ptr->meta.drawoffset_x = entry.drawoffset_x;
	sprite_ptr->meta.drawoffset_y = entry.drawoffset_y;
	sprite_ptr->meta.minimap_color = entry.minimap_color;
	sprite_ptr->meta.has_light = entry.has_light;
	sprite_ptr->meta.light = entry.light;
	installAnimation(*sprite_ptr, entry);

	auto& sprite_list = sprite_ptr->getSpriteList();
	sprite_list.clear();
	sprite_list.reserve(entry.sprite_ids.size());
	for (uint32_t sprite_id : entry.sprite_ids) {
		NormalImage* image = manager.getOrCreateNormalImage(sprite_id);
		if (!image) {
			warnings.push_back(std::format("GraphicsAssembler: sprite {} references out-of-range image {}.", entry.client_id, sprite_id));
			return false;
		}
		sprite_list.push_back(image);
	}
	sprite_ptr->updateSimpleStatus();

	manager.insertSprite(entry.client_id, std::move(sprite));
	return true;
}

void GraphicsAssembler::resetRuntimeState(GraphicManager& manager) {
	manager.resetLoadedGraphicsState();
}

bool GraphicsAssembler::install(GraphicManager& manager, const DatCatalog& catalog, std::shared_ptr<SpriteArchive> sprite_archive, wxString& error, std::vector<std::string>& warnings) {
	if (!sprite_archive) {
		error = "Sprite archive is missing.";
		return false;
	}

	if (!validateCatalog(catalog, error)) {
		return false;
	}

	const auto sprite_space_size = static_cast<size_t>(catalog.lastEntryId()) + 1;
	const auto image_space_size = imageSpaceSize(catalog);

	resetRuntimeState(manager);
	manager.resizeStorage(sprite_space_size, image_space_size);

	for (uint32_t client_id = 100; client_id <= catalog.lastEntryId(); ++client_id) {
		const auto* entry = catalog.entry(client_id);
		if (!entry) {
			error = wxString::FromUTF8(std::format("Missing DAT catalog entry for client id {}.", client_id));
			return false;
		}
		if (!installSpriteEntry(manager, *entry, warnings)) {
			error = wxString::FromUTF8(std::format("Failed to install graphics for client id {}.", client_id));
			return false;
		}
	}

	manager.finalizeLoadedCatalog(
		catalog.format,
		catalog.item_count,
		catalog.creature_count,
		catalog.is_extended,
		catalog.has_transparency,
		catalog.has_frame_durations,
		catalog.has_frame_groups,
		std::move(sprite_archive)
	);

	return true;
}
