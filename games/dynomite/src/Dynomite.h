#ifndef __DYNOMITE_H__
#define __DYNOMITE_H__

// The board (Dynomite, vtable 0x4f7470: the 43 Widget slots and 19 of its own; constructor 0x404030, destructor
// 0x4041e0). The four modes derive from it (DynomiteStomped, DynomitePanic, DynomiteFossil, DynomiteTimeTrial).
// Work in progress: the fields are named as the translated code comes to use them.

#include "BinFile.h"
#include "PtrList.h"
#include "Widget.h"
#include <string>
#include <vector>

namespace Sexy
{

class Effect;
class Egg;
class EggGrid;
class FirePrompt;
class FlyingEgg;
class FossilizedApp;
class Launcher;
class MenuTalker;
class PowerUp;
class Steggy;
class Tank;
class Thunderbolt;

class Dynomite : public Widget
{
public:
	FossilizedApp*		mApp;				// +0x58
	Launcher*			mLauncher;			// +0x5c (0x401050)
	FirePrompt*			mFirePrompt;		// +0x60 (0x431fc0)
	int					mField68;			// +0x68 306
	int					mField6c;			// +0x6c 153
	int					mField70;			// +0x70 250, the launcher's middle
	int					mField74;			// +0x74 (352)
	int					mDifficulty;		// +0x78 1-3 (the side bar's skill picture)
	int					mGameType;			// +0x7c (the side bar's mode picture; 2 shows its sixth button)
	bool				mField80;			// +0x80
	Steggy*				mSteggy;			// +0x84 (0x437aa0)
	Tank*				mTank;				// +0x88 (0x438400)
	EggGrid*			mGrid;				// +0x8c (0x415770)
	int					mClipRect[4];		// +0x90 where SetActive keeps the cursor (left, top, right, bottom)
	int					mFieldA0;			// +0xa0 the window's left on the screen
	int					mFieldA4;			// +0xa4 and its top
	int					mFieldA8;			// +0xa8 the clip's middle
	int					mFieldAc;			// +0xac the window's top + 400
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
	int					mField128;			// +0x128
	int					mBonusBits;			// +0x12c 2, 4 and 8 double, triple and quadruple the points
	bool				mField130;			// +0x130
	bool				mField131;			// +0x131
	bool				mField132;			// +0x132 (Draw calls slot 54 when it is set)
	int					mField134;			// +0x134 (350)
	int					mField138;			// +0x138
	int					mField13c;			// +0x13c
	MenuTalker*			mTalker;			// +0x144 the large Inchy (Stomped's end)
	bool				mField148;			// +0x148 (true)
	bool				mField149;			// +0x149 (true)
	int					mChain;				// +0x14c (-1)
	int					mPauseCount;		// +0x150
	Thunderbolt*		mThunderbolt;		// +0x154 (slot 51)
	int					mField158;			// +0x158 (the launcher's x from the middle)
	int					mField15c;			// +0x15c (-1; the colour fired last)
	int					mField160;			// +0x160 (-1)
	BinFile				mBinFile;			// +0x164
	bool				mField16c;			// +0x16c
	int					mScoreRanks[20];	// +0x170 the scores past which mRank goes up
	int					mRank;				// +0x1c0
	bool				mRankChanged;		// +0x1c4
	int					mField1c8;			// +0x1c8 (-480)
	int					mField1cc;			// +0x1cc
	int					mLongestChain;		// +0x1d0
	int					mBabies;			// +0x1d4 the babies hatched
	char				mReplayText[0x30];	// +0x208 PLAY_THIS_PUZZLE_AGAIN
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
	// 0x404a70 (slot 45): an egg of a colour in the bottom ten rows, not the one fired last (else the board's
	// +0x160, else any colour up to the highest placed)
	virtual Egg*		NextEgg();
	// 0x407ac0 (slot 46): the launcher, the grid, the fire prompt, Steggy and Tank; the small Inchy rises
	virtual void		Init(FossilizedApp* theApp, int theDifficulty);
	// 0x404c30 (slot 44): thePoints times theMultiplier (when positive) plus the chain's bonus, by skill, ten
	// times over, times the bonus bits; a popup at x, y
	virtual void		AddPoints(int x, int y, int thePoints, int theMultiplier, bool isChain);
	// 0x404f40 (slot 50): a match (isMatch) adds to the chain, which Inchy calls out; a miss or a chain of eight
	// ends it
	virtual void		Chain(int x, int y, bool isMatch);
	virtual void		MakeDynamite();					// 0x405010 (slot 51): a thunderbolt strikes the loaded egg
	virtual void		Blasted();						// 0x4b1930 (slot 52; empty): after a bomb's blast
	virtual void		EggAlone(int theColor);			// 0x4050d0 (slot 56; empty): a landed egg touches none of its colour
	virtual void		DrawPiece(int theColor, int x, int y);	// 0x406040 (slot 57; empty): a falling colour from 50 up
	virtual void		PieceFell(int theColor);		// 0x4050d0 (slot 58; empty): one off the bottom
	virtual void		EggLanded();					// 0x4b1930 (slot 61; empty)
	void				Hint();							// 0x40ad90 (the side bar's sixth button)
};

// 0x5304a0, zeroed by the board's constructor
extern int gBoardGlobal5304a0;
// 0x5304a4, the score ranks' names (the string list gScoreNames), read by Init
extern std::vector<std::string> gScoreNames;

}

#endif
