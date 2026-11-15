#ifndef __HEAVYWEAPON_BOSSWARNING_H__
#define __HEAVYWEAPON_BOSSWARNING_H__

// BossWarning.cpp: the warning before a boss: five red flashes with an alarm, arrows and a danger sign pointing where
// the boss comes from, and its description typed out, a beep a letter. At the end it starts the boss music.
// Original: 0xfc bytes, constructor 0x40ed50, Update 0x40e800, Draw 0x40e920, text 0x40eb30, destructor 0x40e7d0;
// the board keeps it at +0x30c. Its int at +0x10 is set to 0 and never read, and is left out.

#include "Point.h"

#include <string>

namespace Sexy
{

class Board;
class Graphics;
class HeavyWeaponApp;

// Each boss's warning and its element in data\bosses.xml (0x52aef8, 0x20 bytes each, by HeavyWeaponApp::GetBossIndex).
// The bosses' own loader (0x43f910) finds their elements by these names too.
struct BossInfo
{
	int				mWarningX;		// +0x00 where the danger sign and arrows point, from the screen's centre
	int				mWarningY;		// +0x04
	double			mWarningAngle;	// +0x08 the arrows' turn (radians, against the clock)
	const char*		mName;			// +0x10 (a char[16] in the original)
};

extern const BossInfo BOSS_INFOS[];

class BossWarning
{
public:
	enum
	{
		NUM_FLASHES = 5,
		NUM_ARROWS = 6
	};

	Board*			mBoard;							// +0x00
	HeavyWeaponApp*	mApp;							// +0x04
	int				mFlashes;						// +0x08 flashes over so far
	int				mFlash;							// +0x0c the red flash's brightness, 254 down 2 an update
	std::string		mText;							// +0x1c the boss's description
	int				mTextShown;						// +0x38 characters of it typed so far
	Point			mArrows[NUM_ARROWS][4];			// +0x3c each arrow's corners, turned toward the boss

public:
	BossWarning(Board* theBoard);	// 0x40ed50

	void			Update();			// 0x40e800; at the end it deletes itself and clears the board's pointer
	void			Draw(Graphics* g);	// 0x40e920

protected:
	void			LoadText();			// 0x40eb30
};

}

#endif
