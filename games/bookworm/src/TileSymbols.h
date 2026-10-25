#ifndef __BOOKWORM_TILESYMBOLS_H__
#define __BOOKWORM_TILESYMBOLS_H__

// port(mod): colour-blind mode (mod "colour-blind", feature "tileSymbols"): each special tile carries a symbol
// in its top-left corner, so the tile types differ by shape as well as colour: a circle on green tiles, a
// square on gold, a rhombus on sapphire, a star on diamond and a triangle on fire tiles. The symbols are white
// with a dark outline, readable on every tile; the setting "large" draws them bigger on a dark plate. They are
// small images with HD pixels, so they stay sharp at any render scale.

namespace Sexy
{
class Graphics;
class SexyAppBase;

// The symbol of a tile of theType at a tile drawn at theX, theY (nothing for a normal tile)
void DrawTileSymbol(Graphics* g, SexyAppBase* theApp, int theX, int theY, int theType, bool isLarge);

}

#endif
