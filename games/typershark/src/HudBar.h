#ifndef __TYPERSHARK_HUDBAR_H__
#define __TYPERSHARK_HUDBAR_H__

// HudBar.cpp: the strip across the top of the board: a half-transparent black band 25 pixels high with the level (or
// the depth in an abyss game) and the points on the left and the lives on the right ("Game Over!" when they are
// below 0). When the player has won an extra life (Player::mExtraLifeFlashing, set by AddScore) "EXTRA LIFE!" flashes in green
// next to the level text: the alpha rises by 2 a tick to 255 and falls back to 0, four times, and the flag is cleared.
// Original: class Sexy::HudBar (no base, no virtual functions), sizeof 0x1c; constructor 0x441730 (the player and the
// app), Update 0x441760, Draw 0x449910. The board makes it in its constructor (Board +0xe04) and calls Update on every
// tick and Draw at the end of its own Draw. The field comments give the offsets in the original's object.

namespace Sexy
{

class Font;
class Graphics;
class Player;
class WinWSApp;

class HudBar
{
public:
	int						mFlashStep;				// +0x00 how many times "EXTRA LIFE!" has flashed (constructor 0)
	bool					mFadingUp;				// +0x04 the alpha is rising (constructor true)
	int						mAlpha;					// +0x08 the alpha of "EXTRA LIFE!" (constructor 0)
	Player*					mPlayer;				// +0x0c constructor argument 1
	Font*					mFont;					// +0x10 the app's Rockwell 13 font (WinWSApp +0x8bc)
	WinWSApp*				mApp;					// +0x14 constructor argument 2
	int						mAlwaysZero;				// +0x18 constructor 0; never read

public:
	HudBar(Player* thePlayer, WinWSApp* theApp);	// 0x441730
	void					Update();				// 0x441760
	void					Draw(Graphics* g);		// 0x449910
};

}

#endif
