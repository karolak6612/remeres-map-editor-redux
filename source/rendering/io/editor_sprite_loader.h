//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_EDITOR_SPRITE_LOADER_H_
#define RME_RENDERING_EDITOR_SPRITE_LOADER_H_

class SpriteLoader;
class SpriteDatabase;

class EditorSpriteLoader {
public:
	static bool Load(SpriteLoader* loader, SpriteDatabase& db);
};

#endif
