#ifndef __HEAVYWEAPON_POWERUPDROPS_H__
#define __HEAVYWEAPON_POWERUPDROPS_H__

// PowerUpDrops.cpp: the power-ups dropped on the field, falling to the ground and rolling back with it until the tank
// drives over one. Original: 0x18 bytes, constructor 0x42c170, the board's at +0x2fc.

#include "ObjectList.h"

namespace Sexy
{

class Board;
class Graphics;
class HeavyWeaponApp;

// One power-up on the field (0x28 bytes)
// A power-up (HeavyWeaponApp::POWERUP_): what its drop says when caught, how high it goes, and how far ahead of the game
// it may get: a supply copter brings it only while its level is at most the game's level times mLevelFactor (0: no
// limit)
struct PowerUpInfo
{
	unsigned int	mName;			// +0x00 the text's address (GT); the original holds the text, char[0x20]
	int				mMax;			// +0x20
	double			mLevelFactor;	// +0x28
};

extern const PowerUpInfo gPowerUpInfo[];

struct PowerUpDrop
{
	int				mType;			// +0x00 0..5 a power-up (HeavyWeaponApp::POWERUP_), 6..11 a special weapon
									// (WEAPON_ + 6); the sprite's column
	double			mX;				// +0x08 from the screen's centre
	double			mY;				// +0x10
	double			mVelX;			// +0x18
	double			mVelY;			// +0x20
};

class PowerUpDrops
{
public:
	enum
	{
		FIRST_WEAPON_TYPE = 6,		// mType of the first special weapon
		MAX_WEAPON_LEVEL = 3
	};

	Board*					mBoard;		// +0x00
	HeavyWeaponApp*			mApp;		// +0x04
	ObjectList<PowerUpDrop>	mDrops;		// +0x08

public:
	PowerUpDrops(Board* theBoard);		// 0x42c170

	// theVelX is not used: the drop drifts back at the speed the ground would carry it from x
	void	Add(double x, double y, double theVelX, double theVelY, int theType); // 0x42c1b0
	void	Draw(Graphics* g);			// 0x42c230
	void	Clear();					// 0x42c2a0
	void	Update();					// 0x42c2b0
};

}

#endif
