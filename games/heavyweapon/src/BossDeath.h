#ifndef __HEAVYWEAPON_BOSSDEATH_H__
#define __HEAVYWEAPON_BOSSDEATH_H__

// BossDeath.cpp: a beaten boss's last moments: big explosions at random points on it, one every 10 to 20 updates.
// The board ends the boss after the twentieth (Board::UpdateBossDeath), which deletes this and so fades the music.
// Original: 0x10 bytes, constructor 0x41bf20, destructor 0x41bf50, Update 0x41bf70; the board keeps it at +0x300 and
// the bosses make it when they die.

namespace Sexy
{

class Board;
class HeavyWeaponApp;

class BossDeath
{
public:
	Board*			mBoard;			// +0x00
	HeavyWeaponApp*	mApp;			// +0x04
	int				mTimer;			// +0x08 up 1 an update; past 20 an explosion, and it starts again from 0 to 9
	int				mExplosions;	// +0x0c explosions so far

public:
	// It saves the players' records first, so the boss counts even if the game is quit now
	BossDeath(Board* theBoard);		// 0x41bf20
	~BossDeath();					// 0x41bf50

	void			Update();		// 0x41bf70
};

}

#endif
