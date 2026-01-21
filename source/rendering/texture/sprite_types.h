//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_SPRITE_TYPES_H_
#define RME_SPRITE_TYPES_H_

#include "main.h"
#include "../graphics.h"
#include "animator.h"
#include <memory>
#include <vector>
#include <list>

class GameSprite : public Sprite {
public:
	class TemplateImage;
	GameSprite();
	~GameSprite();

	int getIndex(int width, int height, int layer, int pattern_x, int pattern_y, int pattern_z, int frame) const;
	GLuint getHardwareID(int _x, int _y, int _layer, int _subtype, int _pattern_x, int _pattern_y, int _pattern_z, int _frame);
	GLuint getHardwareID(int _x, int _y, int _dir, int _addon, int _pattern_z, const Outfit& _outfit, int _frame); // CreatureDatabase
	virtual void DrawTo(wxDC* dc, SpriteSize sz, int start_x, int start_y, int width = -1, int height = -1);

	virtual void unloadDC();

	wxMemoryDC* getDC(SpriteSize size);
	TemplateImage* getTemplateImage(int sprite_index, const Outfit& outfit);

	void clean(int time);

	int getDrawHeight() const;
	std::pair<int, int> getDrawOffset() const;
	uint8_t getMiniMapColor() const;

	bool hasLight() const noexcept {
		return has_light;
	}
	const SpriteLight& getLight() const noexcept {
		return light;
	}

	class Image {
	public:
		Image();
		virtual ~Image();

		bool isGLLoaded;
		int lastaccess;

		void visit();
		virtual void clean(int time);

		virtual GLuint getHardwareID() = 0;
		virtual uint8_t* getRGBData() = 0;
		virtual uint8_t* getRGBAData() = 0;

	protected:
		virtual void createGLTexture(GLuint whatid);
		virtual void unloadGLTexture(GLuint whatid);
	};

	class NormalImage : public Image {
	public:
		NormalImage();
		virtual ~NormalImage();

		// We use the sprite id as GL texture id
		uint32_t id;

		// This contains the pixel data
		uint16_t size;
		uint8_t* dump;

		virtual void clean(int time);

		virtual GLuint getHardwareID();
		virtual uint8_t* getRGBData();
		virtual uint8_t* getRGBAData();

	protected:
		virtual void createGLTexture(GLuint ignored = 0);
		virtual void unloadGLTexture(GLuint ignored = 0);
	};

	class TemplateImage : public Image {
	public:
		TemplateImage(GameSprite* parent, int v, const Outfit& outfit);
		virtual ~TemplateImage();

		virtual GLuint getHardwareID();
		virtual uint8_t* getRGBData();
		virtual uint8_t* getRGBAData();

		GLuint gl_tid;
		GameSprite* parent;
		int sprite_index;
		uint8_t lookHead;
		uint8_t lookBody;
		uint8_t lookLegs;
		uint8_t lookFeet;

	protected:
		void colorizePixel(uint8_t color, uint8_t& r, uint8_t& b, uint8_t& g);

		virtual void createGLTexture(GLuint ignored = 0);
		virtual void unloadGLTexture(GLuint ignored = 0);
	};

	uint32_t id;
	wxMemoryDC* dc[SPRITE_SIZE_COUNT];

	// GameSprite info
	uint8_t height;
	uint8_t width;
	uint8_t layers;
	uint8_t pattern_x;
	uint8_t pattern_y;
	uint8_t pattern_z;
	uint8_t frames;
	uint32_t numsprites;

	Animator* animator;

	uint16_t draw_height;
	uint16_t drawoffset_x;
	uint16_t drawoffset_y;

	uint16_t minimap_color;

	bool has_light = false;
	SpriteLight light;

	std::vector<NormalImage*> spriteList;
	std::list<TemplateImage*> instanced_templates; // Templates that use this sprite
};

#endif // RME_SPRITE_TYPES_H_
