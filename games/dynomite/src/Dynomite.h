#ifndef __DYNOMITE_H__
#define __DYNOMITE_H__

// The board (Dynomite, vtable 0x4f7470: the 43 Widget slots and 19 of its own; constructor 0x404030, destructor
// 0x4041e0). The four modes derive from it (DynomiteStomped, DynomitePanic, DynomiteFossil, DynomiteTimeTrial).
// Work in progress: the fields are named as the translated code comes to use them.

#include "BinFile.h"
#include "PtrList.h"
#include "Widget.h"

namespace Sexy
{

class Effect;
class EggGrid;
class FlyingEgg;
class FossilizedApp;
class PowerUp;

// the board's +0x5c (its +0xc is tested before the dynamite combo gives a stick)
struct BoardPart5c
{
	int					mField00;
	int					mField04;
	int					mField08;
	int					mField0c;
};

class Dynomite : public Widget
{
public:
	FossilizedApp*		mApp;				// +0x58
	BoardPart5c*		mBoard5c;			// +0x5c
	int					mField68;			// +0x68 306
	int					mField6c;			// +0x6c 153
	int					mField70;			// +0x70 250, the launcher's middle
	int					mField74;			// +0x74 (352)
	int					mDifficulty;		// +0x78 1-3 (the side bar's skill picture)
	int					mGameType;			// +0x7c (the side bar's mode picture; 2 shows its sixth button)
	bool				mField80;			// +0x80
	EggGrid*			mGrid;				// +0x8c (0x415770)
	bool				mFieldB0;			// +0xb0
	bool				mInitialized;		// +0xb1 what the destructor frees
	int					mFieldB4;			// +0xb4
	PtrList<FlyingEgg>	mFlyingEggs;		// +0xb8
	PtrList<Effect>		mEffects;			// +0xd0
	PtrList<Effect>		mPopups;			// +0xe8 the points rising (0x4346a0)
	bool				mField100;			// +0x100
	int					mField104;			// +0x104
	int					mField108;			// +0x108 (a leaving baby sets it to 20)
	PtrList<PowerUp>	mPowerUps;			// +0x10c
	int					mPowerUpAngle;		// +0x124 where on its circle the next power-up starts
	int					mBonusBits;			// +0x12c 2, 4 and 8 double, triple and quadruple the points
	bool				mField130;			// +0x130
	bool				mField131;			// +0x131
	bool				mField132;			// +0x132 (Draw calls slot 54 when it is set)
	int					mField134;			// +0x134 (350)
	int					mField138;			// +0x138
	int					mField13c;			// +0x13c
	bool				mField148;			// +0x148 (true)
	bool				mField149;			// +0x149 (true)
	int					mChain;				// +0x14c (-1)
	int					mPauseCount;		// +0x150
	int					mField158;			// +0x158
	int					mField15c;			// +0x15c (-1)
	int					mField160;			// +0x160 (-1)
	BinFile				mBinFile;			// +0x164
	bool				mField16c;			// +0x16c
	int					mScoreRanks[20];	// +0x170 the scores past which mRank goes up
	int					mRank;				// +0x1c0
	bool				mRankChanged;		// +0x1c4
	int					mField1c8;			// +0x1c8 (-480)
	int					mField1cc;			// +0x1cc
	int					mField1d0;			// +0x1d0
	int					mBabies;			// +0x1d4 the babies hatched
	int					mSong;				// +0x238 the music the destructor fades out
	int					mField23c;			// +0x23c
	unsigned int		mStartTicks;		// +0x240 GetTickCount at construction
	bool				mField244;			// +0x244

	Dynomite();								// 0x404030
	virtual ~Dynomite();					// 0x4041e0

	void				SetPaused(bool isPaused);		// 0x404fb0
	void				SetActive(bool isActive);		// 0x4047e0
	void				AddPowerUp(int theKind);		// 0x4060c0
	virtual void		GetPowerUpCenter(int* x, int* y);	// 0x404f30 (slot 43; empty)
	// 0x404c30 (slot 44): thePoints times theMultiplier (when positive) plus the chain's bonus, by skill, ten
	// times over, times the bonus bits; a popup at x, y
	virtual void		AddPoints(int x, int y, int thePoints, int theMultiplier, bool isChain);
	virtual void		MakeDynamite();					// 0x405010 (slot 51)
	virtual void		EggAlone(int theColor);			// 0x4050d0 (slot 56; empty): a landed egg touches none of its colour
	virtual void		EggLanded();					// 0x4b1930 (slot 61; empty)
	void				Hint();							// 0x40ad90 (the side bar's sixth button)
};

// 0x5304a0, zeroed by the board's constructor
extern int gBoardGlobal5304a0;

}

#endif
