#ifndef RME_OTB_ITEM_FORMAT_H_
#define RME_OTB_ITEM_FORMAT_H_

#include <cstdint>

enum class OtbFileFormatVersion {
	V1 = 1,
	V2 = 2,
	V3 = 3
};

enum rootattrib_t : std::uint8_t {
	ROOT_ATTR_VERSION = 0x01
};

enum itemattrib_t : std::uint8_t {
	ITEM_ATTR_FIRST = 0x10,
	ITEM_ATTR_SERVERID = ITEM_ATTR_FIRST,
	ITEM_ATTR_CLIENTID,
	ITEM_ATTR_NAME,
	ITEM_ATTR_DESCR,
	ITEM_ATTR_SPEED,
	ITEM_ATTR_SLOT,
	ITEM_ATTR_MAXITEMS,
	ITEM_ATTR_WEIGHT,
	ITEM_ATTR_WEAPON,
	ITEM_ATTR_AMU,
	ITEM_ATTR_ARMOR,
	ITEM_ATTR_MAGLEVEL,
	ITEM_ATTR_MAGFIELDTYPE,
	ITEM_ATTR_WRITEABLE,
	ITEM_ATTR_ROTATETO,
	ITEM_ATTR_DECAY,
	ITEM_ATTR_SPRITEHASH,
	ITEM_ATTR_MINIMAPCOLOR,
	ITEM_ATTR_07,
	ITEM_ATTR_08,
	ITEM_ATTR_LIGHT,
	ITEM_ATTR_DECAY2,
	ITEM_ATTR_WEAPON2,
	ITEM_ATTR_AMU2,
	ITEM_ATTR_ARMOR2,
	ITEM_ATTR_WRITEABLE2,
	ITEM_ATTR_LIGHT2,
	ITEM_ATTR_TOPORDER,
	ITEM_ATTR_WRITEABLE3,
	ITEM_ATTR_WAREID,
	ITEM_ATTR_CLASSIFICATION,
	ITEM_ATTR_LAST
};

enum itemflags_t : std::uint32_t {
	FLAG_UNPASSABLE = std::uint32_t { 1 } << 0,
	FLAG_BLOCK_MISSILES = std::uint32_t { 1 } << 1,
	FLAG_BLOCK_PATHFINDER = std::uint32_t { 1 } << 2,
	FLAG_HAS_ELEVATION = std::uint32_t { 1 } << 3,
	FLAG_USEABLE = std::uint32_t { 1 } << 4,
	FLAG_PICKUPABLE = std::uint32_t { 1 } << 5,
	FLAG_MOVEABLE = std::uint32_t { 1 } << 6,
	FLAG_STACKABLE = std::uint32_t { 1 } << 7,
	FLAG_FLOORCHANGEDOWN = std::uint32_t { 1 } << 8,
	FLAG_FLOORCHANGENORTH = std::uint32_t { 1 } << 9,
	FLAG_FLOORCHANGEEAST = std::uint32_t { 1 } << 10,
	FLAG_FLOORCHANGESOUTH = std::uint32_t { 1 } << 11,
	FLAG_FLOORCHANGEWEST = std::uint32_t { 1 } << 12,
	FLAG_ALWAYSONTOP = std::uint32_t { 1 } << 13,
	FLAG_READABLE = std::uint32_t { 1 } << 14,
	FLAG_ROTABLE = std::uint32_t { 1 } << 15,
	FLAG_HANGABLE = std::uint32_t { 1 } << 16,
	FLAG_HOOK_EAST = std::uint32_t { 1 } << 17,
	FLAG_HOOK_SOUTH = std::uint32_t { 1 } << 18,
	FLAG_CANNOTDECAY = std::uint32_t { 1 } << 19,
	FLAG_ALLOWDISTREAD = std::uint32_t { 1 } << 20,
	FLAG_UNUSED = std::uint32_t { 1 } << 21,
	FLAG_CLIENTCHARGES = std::uint32_t { 1 } << 22,
	FLAG_IGNORE_LOOK = std::uint32_t { 1 } << 23
};

#endif
