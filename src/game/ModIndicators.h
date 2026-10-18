#ifndef __MODINDICATORS_H__
#define __MODINDICATORS_H__

// port: small signs that the game is running with mods (GameConfig::mAnyModEnabled, any mod at all,
// even one that only changes art):
// - the banner font's G (GO!, ...) gets a crossbar (AddModsCrossbarToG, when the fonts load);
// - a hint gets a fourth sparkle (Board::Draw).
// Neither uses Rand(), so a modded game plays out exactly as an unmodded one with the same seed.

namespace Sexy
{

class ImageFont;

void AddModsCrossbarToG(ImageFont* theFont);

}

#endif
