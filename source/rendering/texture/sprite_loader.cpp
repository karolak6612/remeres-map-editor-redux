//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "main.h"
#include "sprite_loader.h"
#include "sprite_types.h"
#include "animator.h"
#include "../graphics.h"
#include "filehandle.h"
#include "otml.h"
#include "settings.h"
#include "gui.h"
#include <wx/dir.h>

namespace rme {
	namespace render {

		SpriteLoader::SpriteLoader(GraphicManager& graphicManager) :
			graphicManager_(graphicManager),
			datFormat_(DAT_FORMAT_UNKNOWN),
			otfiFound_(false),
			isLoaded_(false) {
		}

		SpriteLoader::~SpriteLoader() = default;

		bool SpriteLoader::loadOTFI(const std::string& filename, std::string& error, std::vector<std::string>& warnings) {
			wxDir dir(filename);
			wxString otfi_file;

			otfiFound_ = false;

			if (dir.GetFirst(&otfi_file, "*.otfi", wxDIR_FILES)) {
				wxFileName otfi(filename, otfi_file);
				OTMLDocumentPtr doc = OTMLDocument::parse(otfi.GetFullPath().ToStdString());
				if (doc->size() == 0 || !doc->hasChildAt("DatSpr")) {
					error += "'DatSpr' tag not found";
					return false;
				}

				OTMLNodePtr node = doc->get("DatSpr");
				metadataInfo_.isExtended = node->valueAt<bool>("extended");
				metadataInfo_.hasTransparency = node->valueAt<bool>("transparency");
				metadataInfo_.hasFrameDurations = node->valueAt<bool>("frame-durations");
				metadataInfo_.hasFrameGroups = node->valueAt<bool>("frame-groups");

				std::string metadata = node->valueAt<std::string>("metadata-file", std::string(ASSETS_NAME) + ".dat");
				std::string sprites = node->valueAt<std::string>("sprites-file", std::string(ASSETS_NAME) + ".spr");

				metadataInfo_.metadataFile = wxFileName(filename, wxString(metadata));
				metadataInfo_.spritesFile = wxFileName(filename, wxString(sprites));
				otfiFound_ = true;
			}

			if (!otfiFound_) {
				metadataInfo_.isExtended = false;
				metadataInfo_.hasTransparency = false;
				metadataInfo_.hasFrameDurations = false;
				metadataInfo_.hasFrameGroups = false;
				metadataInfo_.metadataFile = wxFileName(filename, wxString(ASSETS_NAME) + ".dat");
				metadataInfo_.spritesFile = wxFileName(filename, wxString(ASSETS_NAME) + ".spr");
			}

			return true;
		}

		bool SpriteLoader::loadMetadata(const std::string& filename, std::string& error, std::vector<std::string>& warnings) {
			FileReadHandle file(filename);

			if (!file.isOk()) {
				error += "Failed to open " + filename + " for reading\nThe error reported was:" + file.getErrorMessage();
				return false;
			}

			uint16_t effect_count, distance_count;
			uint32_t datSignature;
			file.getU32(datSignature);

			file.getU16(metadataInfo_.itemCount);
			file.getU16(metadataInfo_.creatureCount);
			file.getU16(effect_count);
			file.getU16(distance_count);

			uint32_t minID = 100;
			uint32_t maxID = metadataInfo_.itemCount + metadataInfo_.creatureCount;

			datFormat_ = graphicManager_.client_version->getDatFormatForSignature(datSignature);

			if (!otfiFound_) {
				metadataInfo_.isExtended = datFormat_ >= DAT_FORMAT_96;
				metadataInfo_.hasFrameDurations = datFormat_ >= DAT_FORMAT_1050;
				metadataInfo_.hasFrameGroups = datFormat_ >= DAT_FORMAT_1057;
			}

			auto& spriteSpace = graphicManager_.sprite_space();
			auto& imageSpace = graphicManager_.image_space();

			uint16_t id = minID;
			while (id <= maxID) {
				GameSprite* sType = newd GameSprite();
				spriteSpace[id] = sType;
				sType->id = id;

				if (!loadSpriteMetadataFlags(file, sType, error, warnings)) {
					warnings.push_back("Failed to load flags for sprite " + std::to_string(sType->id));
				}

				uint8_t group_count = 1;
				if (metadataInfo_.hasFrameGroups && id > metadataInfo_.itemCount) {
					file.getU8(group_count);
				}

				for (uint32_t k = 0; k < group_count; ++k) {
					if (metadataInfo_.hasFrameGroups && id > metadataInfo_.itemCount) {
						file.skip(1);
					}

					file.getByte(sType->width);
					file.getByte(sType->height);

					if ((sType->width > 1) || (sType->height > 1)) {
						file.skip(1);
					}

					file.getU8(sType->layers);
					file.getU8(sType->pattern_x);
					file.getU8(sType->pattern_y);
					if (datFormat_ <= DAT_FORMAT_74) {
						sType->pattern_z = 1;
					} else {
						file.getU8(sType->pattern_z);
					}
					file.getU8(sType->frames);

					if (sType->frames > 1) {
						uint8_t async = 0;
						int loop_count = 0;
						int8_t start_frame = 0;
						if (metadataInfo_.hasFrameDurations) {
							file.getByte(async);
							file.get32(loop_count);
							file.getSByte(start_frame);
						}
						sType->animator = newd Animator(sType->frames, start_frame, loop_count, async == 1);
						if (metadataInfo_.hasFrameDurations) {
							for (int i = 0; i < sType->frames; i++) {
								uint32_t min, max;
								file.getU32(min);
								file.getU32(max);
								FrameDuration* frame_duration = sType->animator->getFrameDuration(i);
								frame_duration->setValues(int(min), int(max));
							}
							sType->animator->reset();
						}
					}

					sType->numsprites = (int)sType->width * (int)sType->height * (int)sType->layers * (int)sType->pattern_x * (int)sType->pattern_y * sType->pattern_z * (int)sType->frames;

					for (uint32_t i = 0; i < sType->numsprites; ++i) {
						uint32_t sprite_id;
						if (metadataInfo_.isExtended) {
							file.getU32(sprite_id);
						} else {
							uint16_t u16 = 0;
							file.getU16(u16);
							sprite_id = u16;
						}

						if (imageSpace[sprite_id] == nullptr) {
							GameSprite::NormalImage* img = newd GameSprite::NormalImage();
							img->id = sprite_id;
							imageSpace[sprite_id] = img;
						}
						sType->spriteList.push_back(static_cast<GameSprite::NormalImage*>(imageSpace[sprite_id]));
					}
				}
				++id;
			}

			return true;
		}

		bool SpriteLoader::loadSpriteMetadataFlags(FileReadHandle& file, GameSprite* sType, std::string& error, std::vector<std::string>& warnings) {
			uint8_t flag = DatFlagLast;

			for (int i = 0; i < DatFlagLast; ++i) {
				file.getU8(flag);

				if (flag == DatFlagLast) {
					return true;
				}

				if (datFormat_ >= DAT_FORMAT_1010) {
					if (flag == 16) {
						flag = DatFlagNoMoveAnimation;
					} else if (flag > 16) {
						flag -= 1;
					}
				} else if (datFormat_ >= DAT_FORMAT_86) {
					// No changes
				} else if (datFormat_ >= DAT_FORMAT_78) {
					if (flag == 8) {
						flag = DatFlagChargeable;
					} else if (flag > 8) {
						flag -= 1;
					}
				} else if (datFormat_ >= DAT_FORMAT_755) {
					if (flag == 23) {
						flag = DatFlagFloorChange;
					}
				} else if (datFormat_ >= DAT_FORMAT_74) {
					if (flag > 0 && flag <= 15) {
						flag += 1;
					} else if (flag == 16) {
						flag = DatFlagLight;
					} else if (flag == 17) {
						flag = DatFlagFloorChange;
					} else if (flag == 18) {
						flag = DatFlagFullGround;
					} else if (flag == 19) {
						flag = DatFlagElevation;
					} else if (flag == 20) {
						flag = DatFlagDisplacement;
					} else if (flag == 22) {
						flag = DatFlagMinimapColor;
					} else if (flag == 23) {
						flag = DatFlagRotateable;
					} else if (flag == 24) {
						flag = DatFlagLyingCorpse;
					} else if (flag == 25) {
						flag = DatFlagHangable;
					} else if (flag == 26) {
						flag = DatFlagHookSouth;
					} else if (flag == 27) {
						flag = DatFlagHookEast;
					} else if (flag == 28) {
						flag = DatFlagAnimateAlways;
					}

					if (flag == DatFlagMultiUse) {
						flag = DatFlagForceUse;
					} else if (flag == DatFlagForceUse) {
						flag = DatFlagMultiUse;
					}
				}

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
						break;

					case DatFlagGround:
					case DatFlagWritable:
					case DatFlagWritableOnce:
					case DatFlagCloth:
					case DatFlagLensHelp:
					case DatFlagUsable:
						file.skip(2);
						break;

					case DatFlagLight: {
						uint16_t intensity, color;
						file.getU16(intensity);
						file.getU16(color);
						sType->has_light = true;
						sType->light = SpriteLight { static_cast<uint8_t>(intensity), static_cast<uint8_t>(color) };
						break;
					}

					case DatFlagDisplacement: {
						if (datFormat_ >= DAT_FORMAT_755) {
							uint16_t offset_x, offset_y;
							file.getU16(offset_x);
							file.getU16(offset_y);
							sType->drawoffset_x = offset_x;
							sType->drawoffset_y = offset_y;
						} else {
							sType->drawoffset_x = 8;
							sType->drawoffset_y = 8;
						}
						break;
					}

					case DatFlagElevation: {
						uint16_t draw_height;
						file.getU16(draw_height);
						sType->draw_height = draw_height;
						break;
					}

					case DatFlagMinimapColor: {
						uint16_t minimap_color;
						file.getU16(minimap_color);
						sType->minimap_color = minimap_color;
						break;
					}

					case DatFlagMarket: {
						file.skip(6);
						std::string marketName;
						file.getString(marketName);
						file.skip(4);
						break;
					}

					default: {
						warnings.push_back("Metadata: Unknown flag: " + std::to_string(flag));
						break;
					}
				}
			}
			return true;
		}

		bool SpriteLoader::loadSpriteData(const std::string& filename, std::string& error, std::vector<std::string>& warnings) {
			FileReadHandle fh(filename);
			if (!fh.isOk()) {
				error = "Failed to open file for reading";
				return false;
			}

			uint32_t signature;
			fh.getU32(signature);

			uint32_t max_id;
			if (metadataInfo_.isExtended) {
				fh.getU32(max_id);
			} else {
				uint16_t u16;
				fh.getU16(u16);
				max_id = u16;
			}

			if (g_settings.getInteger(Config::USE_MEMCACHED_SPRITES)) {
				// Loading all dumps to memory
				auto& imageSpace = graphicManager_.image_space();
				for (uint32_t i = 1; i <= max_id; ++i) {
					auto it = imageSpace.find(i);
					if (it != imageSpace.end() && it->second != nullptr) {
						::GameSprite::NormalImage* img = static_cast<::GameSprite::NormalImage*>(it->second);
						if (!loadSpriteDump(img->dump, img->size, img->id)) {
							warnings.push_back("Failed to load sprite dump for " + std::to_string(i));
						}
					}
				}
				spriteFile_ = "";
			} else {
				spriteFile_ = filename;
			}

			isLoaded_ = true;
			return true;
		}

		bool SpriteLoader::loadSpriteDump(uint8_t*& target, uint16_t& size, int sprite_id) {
			std::string file_to_load = spriteFile_.empty() ? metadataInfo_.spritesFile.GetFullPath().ToStdString() : spriteFile_;
			FileReadHandle file(file_to_load);

			if (!file.isOk()) {
				return false;
			}

			file.skip(metadataInfo_.isExtended ? 8 : 6);
			file.skip((sprite_id - 1) * 4);

			uint32_t address;
			file.getU32(address);
			if (address == 0) {
				return false;
			}

			file.seek(address);
			file.skip(3); // skip color (purple)

			file.getU16(size);
			target = newd uint8_t[size];
			file.getRAW(target, size);

			return true;
		}

		SpriteLoadResult SpriteLoader::loadFromDirectory(const std::string& directory) {
			SpriteLoadResult result;
			std::string error;
			std::vector<std::string> warnings;

			if (!loadOTFI(directory, error, warnings)) {
				result.success = false;
				result.errorMessage = error;
				result.warnings = warnings;
				return result;
			}

			if (!loadMetadata(metadataInfo_.metadataFile.GetFullPath().ToStdString(), error, warnings)) {
				result.success = false;
				result.errorMessage = error;
				result.warnings = warnings;
				return result;
			}

			if (!loadSpriteData(metadataInfo_.spritesFile.GetFullPath().ToStdString(), error, warnings)) {
				result.success = false;
				result.errorMessage = error;
				result.warnings = warnings;
				return result;
			}

			result.success = true;
			result.itemCount = metadataInfo_.itemCount;
			result.creatureCount = metadataInfo_.creatureCount;
			result.warnings = warnings;
			return result;
		}

	} // namespace render
} // namespace rme
