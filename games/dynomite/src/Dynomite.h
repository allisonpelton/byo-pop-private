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
class HighScores;
class Launcher;
class MenuTalker;
class PowerUp;
class Steggy;
class ButtonWidget;
class Tank;
class Thunderbolt;

class Dynomite : public Widget
{
public:
	FossilizedApp*		mApp;				// +0x58
	Launcher*			mLauncher;			// +0x5c (0x401050)
	FirePrompt*			mFirePrompt;		// +0x60 (0x431fc0)
	int					mSavedZero;			// +0x64 0 throughout (the original leaves it unset): saved and loaded, nothing reads it
	int					mPlayWidth;			// +0x68 the play field's width between the walls (306)
	int					mPlayHalfWidth;		// +0x6c half of it (153)
	int					mPlayMiddleX;		// +0x70 the play field's middle x (250), the launcher's
	int					mLineY;				// +0x74 the line the eggs must stay above (352)
	int					mDifficulty;		// +0x78 1-3 (the side bar's skill picture)
	int					mGameType;			// +0x7c (the side bar's mode picture; 2 shows its sixth button)
	bool				mFireBlocked;		// +0x80 no shot can be fired (a round setting up, a cleared board, the tutorial's pauses)
	Steggy*				mSteggy;			// +0x84 (0x437aa0)
	Tank*				mTank;				// +0x88 (0x438400)
	EggGrid*			mGrid;				// +0x8c (0x415770)
	int					mClipRect[4];		// +0x90 where SetActive keeps the cursor (left, top, right, bottom)
	int					mWindowLeft;		// +0xa0 the window's left on the screen (nothing reads it)
	int					mWindowTop;			// +0xa4 and its top (nothing reads it)
	int					mClipPointX;		// +0xa8 the clip's middle x (nothing reads it)
	int					mClipPointY;		// +0xac the board's top + 400 (nothing reads it)
	bool				mPlaying;			// +0xb0 a round is in play: the clock runs and Steggy tosses
	bool				mInitialized;		// +0xb1 what the destructor frees
	int					mShotCount;			// +0xb4 the shots fired (the tutorial waits for the fourth)
	PtrList<FlyingEgg>	mFlyingEggs;		// +0xb8
	PtrList<Effect>		mEffects;			// +0xd0
	PtrList<Effect>		mPopups;			// +0xe8 the points rising (0x4346a0)
	bool				mCeilingAtGoal;		// +0x100 the ceiling has eased down to mCeilingGoal
	int					mCeilingGoal;		// +0x104 where the ceiling eases to, 5% of the way an update
	int					mNextEggKind;		// +0x108 the kind the next egg gets (0 none; a leaving baby sets it to 20)
	PtrList<PowerUp>	mPowerUps;			// +0x10c
	int					mPowerUpAngle;		// +0x124 where on its circle the next power-up starts
	int					mPowerUpSpin;		// +0x128 the power-ups' turn, 0 to 359 degrees
	int					mBonusBits;			// +0x12c 2, 4 and 8 double, triple and quadruple the points
	bool				mLosing;			// +0x130 the eggs stayed past the line too long: Inchy sinks and the grid is pushed down
	bool				mGameOver;			// +0x131 the eggs ran off the bottom: the game's end has begun
	bool				mGameOverShown;		// +0x132 the end panel and its button are up (Draw calls slot 54)
	int					mGameOverDelay;		// +0x134 the updates from mGameOver to the end panel (350)
	int					mShakeX;			// +0x138 the board's shake in x, halved each update (only the constructor sets it, to 0)
	int					mShakeY;			// +0x13c the board's shake in y, halved each update (60 at the game's end, -50 from a blast)
	int					mGameOverAlpha;			// +0x140 the alpha of a mode's "game over" (Panic)
	MenuTalker*			mTalker;			// +0x144 the large Inchy (Stomped's end)
	bool				mCountsChains;		// +0x148 Chain counts the chains (true; false in Time Trial)
	bool				mPastLineLoses;		// +0x149 eggs past the line can lose the game (true; false in Time Trial)
	int					mChain;				// +0x14c (-1)
	int					mPauseCount;		// +0x150
	Thunderbolt*		mThunderbolt;		// +0x154 (slot 51)
	int					mLauncherOffsetX;	// +0x158 the launcher's x from the middle (Stomped's puzzles move it): the egg piles follow it
	int					mAvoidColor;		// +0x15c the colour fired last, which the next egg's pick avoids (-1: none)
	int					mLastFiredColor;	// +0x160 the colour fired last (-1 before the first shot)
	BinFile				mBinFile;			// +0x164
	bool				mFiredThisRound;	// +0x16c a shot has been fired this round (the fire prompt shows until then)
	int					mScoreRanks[20];	// +0x170 the scores past which mRank goes up
	int					mRank;				// +0x1c0
	bool				mRankChanged;		// +0x1c4
	int					mPanelTop;			// +0x1c8 the end panel's top, easing from -480 to 0
	int					mPlayTicks;			// +0x1cc the hundredths of a second played
	int					mLongestChain;		// +0x1d0
	int					mBabies;			// +0x1d4 the babies hatched
	HighScores*			mHighScores;		// +0x1d8 the mode's (made by its constructor)
	char				mHighScoreTitle[0x28];	// +0x1dc the mode's high-score title, from the exe
	ButtonWidget*		mReplayButton;		// +0x204 the game's end's button (id 5)
	char				mReplayText[0x30];	// +0x208 PLAY_THIS_PUZZLE_AGAIN
	int					mSong;				// +0x238 the music the destructor fades out
	int					mSongOffset;		// +0x23c where mSong starts (FadeIn's offset; Panic 0x47, Time Trial 0x2b)
	unsigned int		mStartTicks;		// +0x240 GetTickCount at construction
	bool				mInputLocked;		// +0x244 the tutorial holds the aim, the shot and the fire prompt
	// port(livesplit): inert state the autosplit block reads (LiveSplitObserver.h); the game never reads it
	unsigned int		mSerial;			// this game's number, from NextSerial
	bool				mResumed;			// LoadSavedGame filled this board from a saved game
	// port(livesplit): the next game's number (1 the first), for a board and for the tutorial
	static unsigned int	NextSerial();

	Dynomite();								// 0x404030
	virtual ~Dynomite();					// 0x4041e0

	virtual void		Draw(Graphics* g) override;		// 0x4044a0
	virtual void		Update() override;				// 0x406aa0
	virtual void		KeyDown(KeyCode theKey) override;	// 0x404930: space, escape, F1 and P pause
	virtual void		MouseMove(int x, int y) override;	// 0x404a30 (slots 25 and 29)
	virtual void		MouseDrag(int x, int y) override;	// 0x404a30
	// 0x404980: fires, and the first shot of a run starts the music
	virtual void		MouseDown(int x, int y, int theClickCount) override;

	void				SetPaused(bool isPaused);		// 0x404fb0
	void				CreateSaveFile(const char* theName);	// 0x4050e0
	// 0x4050f0: the saved game's header: the mode's name, the game's, and the app's picture of the board
	void				WriteSaveHeader();
	void				OpenSaveFile(const char* theName);		// 0x405540
	void				SkipSaveHeader();						// 0x405560
	void				CloseSaveFile();						// 0x405580
	void				SetActive(bool isActive);		// 0x4047e0
	void				AddPowerUp(int theKind);		// 0x4060c0
	virtual void		GetPowerUpCenter(int* x, int* y);	// 0x404f30 (slot 43; empty)
	// 0x404a70 (slot 45): an egg of a colour in the bottom ten rows, not the one fired last (else the board's
	// mLastFiredColor, else any colour up to the highest placed)
	virtual Egg*		NextEgg();
	// 0x407ac0 (slot 46): the launcher, the grid, the fire prompt, Steggy and Tank; the small Inchy rises
	virtual void		Init(FossilizedApp* theApp, int theDifficulty);
	// 0x404c30 (slot 44): thePoints times theMultiplier (when positive) plus the chain's bonus, by skill, ten
	// times over, times the bonus bits; a popup at x, y
	virtual void		AddPoints(int x, int y, int thePoints, int theMultiplier, bool isChain);
	// 0x404f40 (slot 50): a match (isMatch) adds to the chain, which Inchy calls out; a miss or a chain of eight
	// ends it
	virtual void		Chain(int x, int y, bool isMatch);
	// 0x404660 (slot 47): the walls, Steggy, Tank, the egg piles, the power-ups, the thunderbolt and the menu's edge
	virtual void		DrawScenery();
	virtual void		DrawEffects();					// 0x406d60 (slot 48): the effects, the popups and "paused"
	virtual void		DrawMode();						// 0x4b1930 (slot 49; empty): what a mode draws between them
	// 0x406f70 (slot 54): the game's end, with the time, the rank and the high scores
	virtual void		DrawGameOver(Graphics* g);
	// 0x405070 (slot 53): the game's end's panel comes down from above, and its button with it
	virtual void		UpdateGameOver();
	// 0x406160 (slot 55): the power-ups' spin, the flying eggs, the ceiling, the minute played, the effects
	virtual void		UpdateBoard();
	virtual void		MakeDynamite();					// 0x405010 (slot 51): a thunderbolt strikes the loaded egg
	virtual void		Blasted();						// 0x4b1930 (slot 52; empty): after a bomb's blast
	virtual void		EggAlone(int theColor);			// 0x4050d0 (slot 56; empty): a landed egg touches none of its colour
	virtual void		DrawPiece(int theColor, int x, int y);	// 0x406040 (slot 57; empty): a falling colour from 50 up
	virtual void		PieceFell(int theColor);		// 0x4050d0 (slot 58; empty): one off the bottom
	// 0x405140 (slot 59): the board into mBinFile, after the saved game's header (a mode's adds its own after)
	virtual void		Save();
	virtual void		Load();							// 0x405590 (slot 60): and back
	virtual void		EggLanded();					// 0x4b1930 (slot 61; empty)
};

// 0x5304a0, zeroed by the board's constructor
extern int gBoardGlobal5304a0;
// 0x5304a4, the score ranks' names (the string list gScoreNames), read by Init
extern std::vector<std::string> gScoreNames;

}

#endif
