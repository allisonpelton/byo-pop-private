#ifndef __ART_VECTORFONT_H__
#define __ART_VECTORFONT_H__

// port(variant): the code-drawn art's letters (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.18): a stroke font written
// for the port. Each glyph is a skeleton of lines and elliptic arcs on a grid with the capitals 10 units high (x-height
// 7, descenders to -3), drawn as round-capped strokes of a chosen weight, so one outline gives every size and boldness.
// It covers ASCII 32..126; other characters fall back to their ASCII letter where Latin-1 has one, else to '?'.

#include "Raster.h"

#include <string>

namespace Art
{

struct TextStyle
{
	double mCap = 10;        // capital height, logical px
	double mWeight = 1.4;    // stroke width, glyph units (a tenth of the capital height)
	double mTracking = 1.0;  // space added between letters, units
	double mWiden = 1.0;     // horizontal scale of the skeletons
	double mSlant = 0.0;     // shear: x moves right by mSlant per unit of height
	bool mCaps = false;      // lower case drawn as capitals

	double Unit() const { return mCap / 10.0; }
};

double CharAdvance(int theChar, const TextStyle& theStyle);           // logical px
double TextWidth(const std::string& theText, const TextStyle& theStyle);
// Strokes the text into the mask, its baseline at y and its pen starting at x; returns the pen's end
double DrawText(Mask& theMask, const std::string& theText, double x, double y, const TextStyle& theStyle);
void DrawChar(Mask& theMask, int theChar, double x, double y, const TextStyle& theStyle);

}

#endif
