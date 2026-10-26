#ifndef __ART_GEMART_H__
#define __ART_GEMART_H__

// port(variant): the code-drawn gems (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.18): the gem sheet's spin and glow rows,
// ray-cast from faceted solids as games/bejeweled/tools/gemsheet.py does. Three styles:
//   cel     the port's own cuts (one per colour, twelve), flat tones from a fixed light, a glint and a dark outline;
//   glossy  the same cuts as glass: facets lit and reflecting a sky, a hard highlight, no outline;
//   su      the Steven Universe gems of tools/make_su_gems_mod.py (stage 15), their cel look ported.
// Each colour c fills row 2c (frame 0 the rest pose, frames 1..14 turned 12 degrees each about the vertical axis) and
// row 2c + 1 (the rest pose under a light circling it, 24 degrees a frame), cells of 52 px at the scale.

#include "Raster.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Art
{

enum GemStyle
{
	GEMS_CEL,
	GEMS_GLOSSY,
	GEMS_SU
};

GemStyle GemStyleFromName(const std::string& theName);   // "cel" (also anything unknown), "glossy", "su"
int GemModelCount(GemStyle theStyle);                     // colours with a model of their own: 12, 12, 11

// The sheet of theColors colours (rows 2 * theColors, at most 12) at theScale: straight ARGB, (780 x 104 * theColors) *
// theScale. The su style's twelfth colour is the cel style's.
void DrawGemSheet(GemStyle theStyle, int theColors, int theScale, std::vector<uint32_t>& theOut);

// One gem in its rest pose, as large as fits theW x theH (logical px) centred at (cx, cy), drawn over the canvas
void DrawGem(Canvas& theCanvas, GemStyle theStyle, int theColor, double cx, double cy, double theW, double theH);

}

#endif
