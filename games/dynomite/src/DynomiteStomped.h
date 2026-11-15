#ifndef __DYNOMITESTOMPED_H__
#define __DYNOMITESTOMPED_H__

// Stomped ("Stomped Puzzle" on the side bar; vtable 0x4f9e68, 0x62c bytes; constructor 0x40ea50, destructor 0x40ec00;
// mode 1 of StartGame): the puzzles of a pack (DEFAULT.EGG or one in the STOMPED folder), one a round. A puzzle sets
// the board's width and the launcher's place, how many shots bring the ceiling down a row, the chance of baby eggs,
// the first eggs to be fired (colours and kinds, colour 20 a random one) and the grid. The walls move to the width,
// the eggs come in, and the round is cleared when every egg is gone; then "round cleared" and its bonus, and the next
// puzzle. Past the skill's last puzzle the pack starts over at its first, two shots a row fewer each time.

#include "Dynomite.h"

namespace Sexy
{

class DynomiteStomped : public Dynomite
{
public:
	BinFile				mPack;				// +0x248 STOMPED.TMP, open only while a puzzle is read
	int					mPackPos;			// +0x250 the round's grid; once it is placed, the next puzzle
	int					mFirstPos;			// +0x254 the skill's first puzzle, where the pack starts over
	float				mKick;				// +0x378 (0; the ceiling's push up, less a twentieth a frame)
	float				mCeilingTarget;		// +0x37c where the ceiling is coming down to (33 below it each drop)
	bool				mCeilingMoving;		// +0x380
	int					mWidth;				// +0x384 the puzzle's width; the board's +0x68 moves to it
	int					mLauncherX;			// +0x388 the puzzle's launcher place; the board's +0x158 moves to it
	int					mGrindFrames;		// +0x38c the grinding goes on 50 frames after the walls stop
	bool				mSettingUp;			// +0x390 the walls move to the puzzle's width and its name shows
	bool				mRoundLoaded;		// +0x391 the eggs are in
	bool				mFirstRound;		// +0x392 the walls start at the width (true until the first round is in)
	int					mShotsPerDrop;		// +0x394 shots that bring the ceiling down a row (3 at least)
	int					mShotsLeft;			// +0x398 to the next drop
	char				mColors[0x100];		// +0x39c the first eggs to fire (20 a random colour)
	char				mKinds[0x100];		// +0x49c and their kinds (0 the usual)
	int					mEggIndex;			// +0x59c the next of them
	int					mEggCount;			// +0x5a0 the last of them
	int					mWarnTimer;			// +0x5a4 the warning's beat while a drop is three shots off or less
	int					mMeter;				// +0x5a8 the mama meter's empty part, a pixel a frame toward
	int					mMeterTarget;		// +0x5ac 132 less 132 times the shots left of the shots per drop
	int					mStartEggs;			// +0x5b0 the round's eggs at its first update
	int					mShots;				// +0x5b4 the eggs landed this round
	int					mRound;				// +0x5b8 the round's number (the game's end calls it rounds cleared)
	int					mRoundsCleared;		// +0x5bc
	int					mRoundBonus;		// +0x5c0 (100 when the bonus is paid; an egg landing alone takes 25)
	char				mRoundText[0x15];	// +0x5c4 ROUND_D
	char				mPuzzleName[0x33];	// +0x5d9 from the pack
	int					mTextAlpha;			// +0x60c the round and the puzzle's name over the board
	int					mTextHold;			// +0x610 frames they stay before the walls stop (10)
	int					mTextTimer;			// +0x614 frames they stay after (500), unless the first shot is fired
	int					mPackPasses;		// +0x61c times the pack started over
	bool				mCleared;			// +0x620 the round is cleared: the bonus count
	int					mClearedAlpha;		// +0x624 "cleared"
	int					mClearedFrames;		// +0x628 frames of the count: the texts at 50, 90 and 130, the bonus at
											// 200, the next puzzle at 400

	DynomiteStomped();						// 0x40ea50
	virtual ~DynomiteStomped();				// 0x40ec00

	// 0x40fa80: the screen shakes while the walls grind and on the last shot before a drop; at the game's end
	// "game over" fades in
	virtual void		Draw(Graphics* g) override;
	// 0x411740: the walls move to the puzzle's width; the round's play; the bonus count once it is cleared
	virtual void		Update() override;
	virtual void		GetPowerUpCenter(int* x, int* y) override;	// 0x40f420: over the mama portrait
	// 0x40f0c0: the board's, but Easy halves the points; during the bonus count the popup stays in place
	virtual void		AddPoints(int x, int y, int thePoints, int theMultiplier, bool isChain) override;
	// 0x40ec70: the puzzle's next egg, else the board's; a baby that left the board (mNextEggKind 20) makes a
	// power-up egg
	virtual Egg*		NextEgg() override;
	// 0x411370: the pack, the skill's first puzzle (theDifficulty 1-3) and the first round
	virtual void		Init(FossilizedApp* theApp, int theDifficulty) override;
	// 0x40ffc0: the walls, the mama meter, Steggy and Tank, the warning's glow, "cleared" and the bonus's texts,
	// the round and the puzzle's name
	virtual void		DrawScenery() override;
	// 0x410880: the board's, with the best combo, the babies hatched, the rounds and the final score
	virtual void		DrawGameOver(Graphics* g) override;
	virtual void		EggAlone(int theColor) override;	// 0x40f3f0: while the grid has its colour, -25 bonus
	virtual void		EggLanded() override;				// 0x40f030: a shot nearer the next drop
	virtual void		Save() override;				// 0x40f440 (slot 59): the board's, the mode's, then the pack
	virtual void		Load() override;				// 0x40f6b0 (slot 60)

	void				LoadGrid();			// 0x40eef0: the round's eggs, once the walls are in place
	static void			CopyPack(const char* theFile);		// 0x40f8f0: the pack to STOMPED.TMP
	void				SkipPuzzles(int theCount);			// 0x40f9a0
	void				NextPuzzle();			// 0x40fc30: reads the next puzzle and starts its round
	void				NextRound();						// 0x4107f0: after the bonus count
	void				UpdateCleared();					// 0x411630: the bonus count
};

}

#endif
