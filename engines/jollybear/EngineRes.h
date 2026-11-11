#ifndef __JB_ENGINERES_H__
#define __JB_ENGINERES_H__

// port: names for the resource ids the engine itself uses (the original passes the numbers).

namespace jb
{

// sounds of Resource.dll (WAVE resources)
enum
{
	SOUND_CLICK = 0x69,
};

// pictures of the exe
enum
{
	EXE_IMAGE_WINDOWED_BUTTON = 0x35,
	EXE_IMAGE_JOLLYBEAR_LOGO = 0x37,
	EXE_IMAGE_CURSOR_HAND = 0x46,
	EXE_IMAGE_CURSOR_ARROW = 0x47,
	EXE_IMAGE_CURSOR_TEXT = 0x48,
	EXE_IMAGE_CURSOR_HAND_SECOND = 0x49,
	EXE_IMAGE_CURSOR_ARROW_SECOND = 0x4a,
	EXE_IMAGE_SPINTOP_LOGO = 0x4d,
	EXE_IMAGE_COPYRIGHT = 0x4e,
	EXE_IMAGE_CREATED_BY = 0x4f,
	EXE_IMAGE_DEVELOPED_BY = 0x50,
	EXE_IMAGE_TRADEMARK = 0x51,
};

// the exe's one font: its picture and its character map
enum
{
	EXE_FONT_SPLASH_MAP = 0x39,
	EXE_FONT_SPLASH = 0x3a,
};

// pictures loaded from files, under ids of the game's own
enum
{
	FILE_IMAGE_SPLASH_LOGO = 0xff01,
};

} // namespace jb

#endif
