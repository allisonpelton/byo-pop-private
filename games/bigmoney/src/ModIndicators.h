#ifndef __BIGMONEY_MODINDICATORS_H__
#define __BIGMONEY_MODINDICATORS_H__

// port: a small sign that the game is running with mods (any enabled mod that changes art, sound, text or
// play, even one that only changes art; not one that only turns on replays or LiveSplit, ModRules.h):
// the score font's "$" gets a double stroke - a thin dark gap splits the stubs above and below the S.
// It is drawn into the font's top layer when the fonts load (WinBMApp::Init), so the score display
// shows it all through play. It uses no random numbers, so a modded game plays out exactly as an
// unmodded one.

namespace Sexy
{

class ImageFont;

void AddModsDoubleStrokeToDollar(ImageFont* theFont);

}

#endif
