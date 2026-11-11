#ifndef __GEMSHEET_H__
#define __GEMSHEET_H__

// port(variant): more gem colours (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.2). Not part of the original
// game, which has art for 7. The gem sheet (images/gemsheet6) gives each colour 104 px: a 52 px row of 15 spin frames,
// then a 52 px row of glow frames. A board of more colours (board.gemColors 8..12) needs more rows: a mod may ship a
// taller sheet, and the rows it lacks are generated from a row it has, recoloured, on the 1x pixels and on the HD
// pixels of a mod's @2x art alike. The mods' "imageOps" ({"op": "extendRows"}) describe the look of the rows they
// generate; the rows they leave out get the default look (WinDMApp::EnsureGemColors). Generated rows carry a small
// corner glyph so the extra colours differ by more than colour.

#include "GameRules.h"

#include <string>
#include <vector>

namespace Platform
{
struct ModImageOp;
}

namespace Sexy
{

class MemoryImage;

namespace GemSheet
{

enum
{
	ROW_HEIGHT = 104,       // per colour: spin frames, then glow frames
	SUB_ROW_HEIGHT = 52,
	FRAME_WIDTH = 52,
	ORIGINAL_ROWS = 7,
	MAX_ROWS = GameRules::MAX_GEM_COLORS
};

// The look of one generated row
struct RowLook
{
	std::string	mModId;               // the mod whose imageOps gave it ("" for the default look)
	int			mFrom = 1;            // the row it is made from (default: the white gem)
	bool		mTinted = false;      // recoloured to mTintHue/Saturation/Lightness, the base row's shading kept
	double		mTintHue = 0;         // degrees
	double		mTintSaturation = 0;  // 0..1
	double		mTintLightness = 0.5; // 0..1: 0.5 keeps the base's lightness
	double		mHue = 0;             // then a hue rotation in degrees,
	double		mSaturation = 1;      // a saturation factor
	double		mLightness = 0;       // and a lightness shift, -1..1
	bool		mGlyph = true;
};

// The looks the enabled mods' imageOps give the rows after the sheet's own, in order; what they get wrong goes
// to theWarnings (may be NULL)
std::vector<RowLook> ModLooks(const std::vector<Platform::ModImageOp>& theOps, std::vector<RulesMessage>* theWarnings);

// The look of generated row theIndex (0 is the first after the original 7) when no mod gives one
RowLook DefaultLook(int theIndex);

// Colour rows theSheet has
int Rows(const MemoryImage* theSheet);

// Grows theSheet to theRows colour rows (at most MAX_ROWS), its HD pixels too: row r gets theLooks[r - its rows] or the
// default look
void Extend(MemoryImage* theSheet, int theRows, const std::vector<RowLook>& theLooks);

// Test hook (the script command gemsheet): theSheet's pixels to theFileName (.png), its valid HD pixels to
// <name>@<scale>x.png
bool WritePngs(MemoryImage* theSheet, const std::string& theFileName);

}

}

#endif
