#include "item_definitions/formats/dat/dat_metadata_decoder.h"

#include "app/client_version.h"
#include "io/filehandle.h"
#include "item_definitions/core/item_definition_types.h"
#include "rendering/core/graphics.h"
#include "rendering/core/normal_image.h"
#include "util/common.h"

#include <climits>
#include <cstdint>
#include <format>
#include <memory>

namespace {
	constexpr uint64_t itemFlagMask(ItemFlag flag) {
		return uint64_t { 1 } << static_cast<uint8_t>(flag);
	}

	constexpr uint8_t remapFlag(uint8_t flag, DatFormat format) {
		if (format >= DAT_FORMAT_1010) {
			if (flag == 16) {
				return DatFlagNoMoveAnimation;
			}
			if (flag > 16) {
				return flag - 1;
			}
		} else if (format >= DAT_FORMAT_78) {
			if (flag == 8) {
				return DatFlagChargeable;
			}
			if (flag > 8) {
				return flag - 1;
			}
		} else if (format >= DAT_FORMAT_755) {
			if (flag == 23) {
				return DatFlagFloorChange;
			}
		} else if (format >= DAT_FORMAT_74) {
			if (flag > 0 && flag <= 15) {
				return flag + 1;
			}
			switch (flag) {
				case 16: return DatFlagLight;
				case 17: return DatFlagFloorChange;
				case 18: return DatFlagFullGround;
				case 19: return DatFlagElevation;
				case 20: return DatFlagDisplacement;
				case 22: return DatFlagMinimapColor;
				case 23: return DatFlagRotateable;
				case 24: return DatFlagLyingCorpse;
				case 25: return DatFlagHangable;
				case 26: return DatFlagHookSouth;
				case 27: return DatFlagHookEast;
				case 28: return DatFlagAnimateAlways;
				case DatFlagMultiUse: return DatFlagForceUse;
				case DatFlagForceUse: return DatFlagMultiUse;
			}
		}
		return flag;
	}

	void applyDatItemDefinitionFlag(GameSprite* sprite, uint8_t flag) {
		switch (flag) {
			case DatFlagGround:
			case DatFlagOnBottom:
				sprite->item_definition_flags |= itemFlagMask(ItemFlag::AlwaysOnBottom);
				break;
			case DatFlagStackable:
				sprite->item_definition_flags |= itemFlagMask(ItemFlag::Stackable);
				break;
			case DatFlagNotWalkable:
				sprite->item_definition_flags |= itemFlagMask(ItemFlag::Unpassable);
				break;
			case DatFlagNotMoveable:
				sprite->item_definition_flags &= ~itemFlagMask(ItemFlag::Moveable);
				break;
			case DatFlagBlockProjectile:
				sprite->item_definition_flags |= itemFlagMask(ItemFlag::BlockMissiles);
				break;
			case DatFlagNotPathable:
				sprite->item_definition_flags |= itemFlagMask(ItemFlag::BlockPathfinder);
				break;
			case DatFlagPickupable:
				sprite->item_definition_flags |= itemFlagMask(ItemFlag::Pickupable);
				break;
			case DatFlagHangable:
				sprite->item_definition_flags |= itemFlagMask(ItemFlag::IsHangable);
				break;
			case DatFlagHookSouth:
				sprite->item_definition_flags |= itemFlagMask(ItemFlag::HookSouth);
				break;
			case DatFlagHookEast:
				sprite->item_definition_flags |= itemFlagMask(ItemFlag::HookEast);
				break;
			case DatFlagRotateable:
				sprite->item_definition_flags |= itemFlagMask(ItemFlag::Rotatable);
				break;
			case DatFlagFloorChange:
				sprite->item_definition_flags |= itemFlagMask(ItemFlag::FloorChange);
				break;
			case DatFlagChargeable:
				sprite->item_definition_flags |= itemFlagMask(ItemFlag::ClientChargeable);
				break;
			default:
				break;
		}
	}

}

bool DatMetadataDecoder::readFlagPayload(DatFormat format, FileReadHandle& file, GameSprite* sprite, uint8_t flag, uint8_t previous_flag, std::vector<std::string>& warnings) {
		applyDatItemDefinitionFlag(sprite, flag);

		switch (flag) {
			case DatFlagGroundBorder:
			case DatFlagOnBottom:
			case DatFlagOnTop:
			case DatFlagContainer:
			case DatFlagStackable:
			case DatFlagForceUse:
			case DatFlagMultiUse:
			case DatFlagFluidContainer:
			case DatFlagSplash:
			case DatFlagNotWalkable:
			case DatFlagNotMoveable:
			case DatFlagBlockProjectile:
			case DatFlagNotPathable:
			case DatFlagPickupable:
			case DatFlagHangable:
			case DatFlagHookSouth:
			case DatFlagHookEast:
			case DatFlagRotateable:
			case DatFlagDontHide:
			case DatFlagTranslucent:
			case DatFlagLyingCorpse:
			case DatFlagAnimateAlways:
			case DatFlagFullGround:
			case DatFlagLook:
			case DatFlagWrappable:
			case DatFlagUnwrappable:
			case DatFlagTopEffect:
			case DatFlagFloorChange:
			case DatFlagNoMoveAnimation:
			case DatFlagChargeable:
			case DatFlagDefault:
				return true;
			case DatFlagGround:
			case DatFlagWritable:
			case DatFlagWritableOnce:
			case DatFlagCloth:
			case DatFlagLensHelp:
			case DatFlagUsable:
				return file.skip(2);
			case DatFlagLight: {
				uint16_t intensity = 0;
				uint16_t color = 0;
				if (!file.getU16(intensity) || !file.getU16(color)) {
					return false;
				}
				sprite->has_light = true;
				sprite->light = SpriteLight { static_cast<uint8_t>(intensity), static_cast<uint8_t>(color) };
				return true;
			}
			case DatFlagDisplacement: {
				if (format >= DAT_FORMAT_755) {
					uint16_t offset_x = 0;
					uint16_t offset_y = 0;
					if (!file.getU16(offset_x) || !file.getU16(offset_y)) {
						return false;
					}
					sprite->drawoffset_x = offset_x;
					sprite->drawoffset_y = offset_y;
				} else {
					sprite->drawoffset_x = 8;
					sprite->drawoffset_y = 8;
				}
				return true;
			}
			case DatFlagElevation: {
				uint16_t draw_height = 0;
				if (!file.getU16(draw_height)) {
					return false;
				}
				sprite->item_definition_flags |= itemFlagMask(ItemFlag::HasElevation);
				sprite->draw_height = draw_height;
				return true;
			}
			case DatFlagMinimapColor: {
				uint16_t minimap_color = 0;
				if (!file.getU16(minimap_color)) {
					return false;
				}
				sprite->minimap_color = minimap_color;
				return true;
			}
			case DatFlagMarket: {
				if (!file.skip(6)) {
					return false;
				}
				std::string market_name;
				if (!file.getString(market_name)) {
					return false;
				}
				return file.skip(4);
			}
			case DatFlagWings:
				return file.skip(16);
			default:
				warnings.push_back(std::format("Metadata: Unknown flag: {}. Previous flag: {}.", static_cast<int>(flag), static_cast<int>(previous_flag)));
				return true;
		}
}

bool DatMetadataDecoder::readFlags(DatFormat format, FileReadHandle& file, GameSprite* sprite, uint32_t sprite_id, std::vector<std::string>& warnings) {
		uint8_t flag = 0xFF;
		uint8_t previous_flag = 0xFF;

		for (int count = 0; count < static_cast<int>(DatFlagLast); ++count) {
			previous_flag = flag;
			if (!file.getU8(flag)) {
				warnings.push_back(std::format("Metadata: error reading flag for sprite id {}", sprite_id));
				return false;
			}
			if (flag == DatFlagLast) {
				return true;
			}

			flag = remapFlag(flag, format);
			if (!readFlagPayload(format, file, sprite, flag, previous_flag, warnings)) {
				warnings.push_back(std::format("Metadata: error reading flag data for flag {} for sprite id {}", static_cast<int>(flag), sprite_id));
				return false;
			}
		}

		warnings.push_back(std::format("Metadata: corruption warning - flag list exceeded limit (255) without terminator for sprite id {}", sprite_id));
		return true;
}

bool DatMetadataDecoder::readSpriteGroup(GraphicManager* manager, FileReadHandle& file, GameSprite* sprite, uint32_t group_index) {
		if (manager->has_frame_groups && sprite->id > manager->item_count && !file.skip(1)) {
			return false;
		}

		uint8_t width = 0;
		uint8_t height = 0;
		uint8_t layers = 0;
		uint8_t pattern_x = 0;
		uint8_t pattern_y = 0;
		uint8_t pattern_z = 0;
		uint8_t frames = 0;

		if (!file.getByte(width) || !file.getByte(height)) {
			return false;
		}
		if ((width > 1 || height > 1) && !file.skip(1)) {
			return false;
		}
		if (!file.getU8(layers) || !file.getU8(pattern_x) || !file.getU8(pattern_y)) {
			return false;
		}
		if (manager->dat_format <= DAT_FORMAT_74) {
			pattern_z = 1;
		} else if (!file.getU8(pattern_z)) {
			return false;
		}
		if (!file.getU8(frames)) {
			return false;
		}

		if (group_index == 0) {
			sprite->width = width;
			sprite->height = height;
			sprite->layers = layers;
			sprite->pattern_x = pattern_x;
			sprite->pattern_y = pattern_y;
			sprite->pattern_z = pattern_z;
			sprite->frames = frames;
		}

		if (frames > 1) {
			uint8_t async = 0;
			int loop_count = 0;
			int8_t start_frame = 0;
			if (manager->has_frame_durations) {
				if (!file.getByte(async) || !file.get32(loop_count) || !file.getSByte(start_frame)) {
					return false;
				}
			}

			if (group_index == 0) {
				sprite->animator = std::make_unique<Animator>(frames, start_frame, loop_count, async == 1);
			}

			if (manager->has_frame_durations) {
				for (int i = 0; i < static_cast<int>(frames); ++i) {
					uint32_t min = 0;
					uint32_t max = 0;
					if (!file.getU32(min) || !file.getU32(max)) {
						return false;
					}
					if (group_index == 0) {
						FrameDuration* frame_duration = sprite->animator->getFrameDuration(i);
						frame_duration->setValues(static_cast<int>(min), static_cast<int>(max));
					}
				}
				if (group_index == 0) {
					sprite->animator->reset();
				}
			}
		}

		if (width == 0 || height == 0 || layers == 0 || pattern_x == 0 || pattern_y == 0 || pattern_z == 0 || frames == 0) {
			return false;
		}

		const uint64_t numsprites64 = static_cast<uint64_t>(width) * static_cast<uint64_t>(height) * static_cast<uint64_t>(layers) *
			static_cast<uint64_t>(pattern_x) * static_cast<uint64_t>(pattern_y) * static_cast<uint64_t>(pattern_z) * static_cast<uint64_t>(frames);
		if (numsprites64 > UINT32_MAX) {
			return false;
		}
		const uint32_t numsprites = static_cast<uint32_t>(numsprites64);
		if (group_index == 0) {
			sprite->numsprites = numsprites;
			sprite->spriteList.reserve(numsprites);
		}

		for (uint32_t i = 0; i < numsprites; ++i) {
			uint32_t sprite_id = 0;
			if (manager->is_extended) {
				if (!file.getU32(sprite_id)) {
					return false;
				}
			} else {
				uint16_t compact_id = 0;
				if (!file.getU16(compact_id)) {
					return false;
				}
				sprite_id = compact_id;
			}

			if (group_index != 0) {
				continue;
			}
			if (sprite_id == UINT32_MAX || sprite_id >= MAX_SPRITES || sprite_id >= manager->image_space.size()) {
				return false;
			}

			auto& image_ptr = manager->image_space[sprite_id];
			if (!image_ptr) {
				auto image = std::make_unique<NormalImage>();
				image->id = sprite_id;
				image_ptr = std::move(image);
			}
			sprite->spriteList.push_back(static_cast<NormalImage*>(image_ptr.get()));
		}

		if (group_index == 0) {
			sprite->updateSimpleStatus();
		}
		return true;
}

bool DatMetadataDecoder::decodeIntoGraphics(GraphicManager* manager, const wxFileName& datafile, wxString& error, std::vector<std::string>& warnings) {
	FileReadHandle file(nstr(datafile.GetFullPath()));
	if (!file.isOk()) {
		error += wxstr(std::format("Failed to open {} for reading\nThe error reported was: {}", datafile.GetFullPath().utf8_string(), file.getErrorMessage()));
		return false;
	}

	uint16_t effect_count = 0;
	uint16_t distance_count = 0;
	uint32_t dat_signature = 0;
	if (!file.getU32(dat_signature)) {
		error = "Failed to read dat signature";
		return false;
	}
	if (!file.getU16(manager->item_count) || !file.getU16(manager->creature_count) || !file.getU16(effect_count) || !file.getU16(distance_count)) {
		error = "Failed to read dat header counts";
		return false;
	}
	if (manager->item_count == 0 || manager->creature_count == 0) {
		error = "Invalid dat header counts (zero items or creatures)";
		return false;
	}

	constexpr uint32_t min_id = 100;
	const uint32_t max_id_needed = manager->item_count + manager->creature_count + 1;
	manager->sprite_space.resize(max_id_needed);
	if (manager->image_space.size() < MAX_SPRITES) {
		manager->image_space.resize(MAX_SPRITES);
	}

	manager->dat_format = manager->client_version->getDatFormatForSignature(dat_signature);
	if (manager->dat_format == DAT_FORMAT_UNKNOWN) {
		error = wxstr(std::format("Unknown dat signature: {:#x}", dat_signature));
		return false;
	}

	manager->is_extended = manager->client_version->isExtended();
	manager->has_frame_durations = manager->client_version->hasFrameDurations();
	manager->has_frame_groups = manager->client_version->hasFrameGroups();
	manager->has_transparency = manager->client_version->isTransparent();

	for (uint32_t id = min_id; id <= manager->item_count + manager->creature_count; ++id) {
		auto sprite_owner = std::make_unique<GameSprite>();
		GameSprite* sprite = sprite_owner.get();
		manager->sprite_space[id] = std::move(sprite_owner);

		sprite->id = id;
		sprite->item_definition_flags = itemFlagMask(ItemFlag::Moveable) | itemFlagMask(ItemFlag::Replaceable);

		if (!readFlags(manager->dat_format, file, sprite, id, warnings)) {
			error = wxstr(std::format("Failed to read metadata flags for id {}", id));
			return false;
		}

		uint8_t group_count = 1;
		if (manager->has_frame_groups && id > manager->item_count && !file.getU8(group_count)) {
			error = wxstr(std::format("Failed to read group count for id {}", id));
			return false;
		}

		for (uint32_t group_index = 0; group_index < static_cast<uint32_t>(group_count); ++group_index) {
			if (!readSpriteGroup(manager, file, sprite, group_index)) {
				error = wxstr(std::format("Failed to read sprite group {} for id {}", group_index, id));
				return false;
			}
		}
	}

	return true;
}
