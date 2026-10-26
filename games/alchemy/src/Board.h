#ifndef __ALCHEMY_BOARD_H__
#define __ALCHEMY_BOARD_H__

// The board (class 0x4e32d0, ctor 0x407ebe, size 0xe78): a game of Alchemy. A grid of 9 x 8 cells of 52 pixels, a piece
// that follows the mouse, the forge gauge (mProgress) and everything that happens when a piece is put down.
// The ButtonListener part is at +0x58 (its vftable is 0x4e32b0). Addresses in comments are the original's.

#include "Widget.h"
#include "ButtonListener.h"
#include "MTRand.h"
#include "Buffer.h"
#include <list>
#include <string>

namespace Sexy
{

class AlchemyApp;
class Piece;
class MemoryImage;
class WizardBox;
class ButtonWidget;
class DialogButton;

// A spark of a bomb (the std::list at +0xddc, node fields at +8, +0x10 ..)
struct BombSpark
{
	int			mLife;      // +8 updates left
	double		mX;         // +0x10
	double		mY;         // +0x18
	double		mVX;        // +0x20
	double		mVY;        // +0x28 gains 0.1 per update
	BombSpark() : mLife(0), mX(0), mY(0), mVX(0), mVY(0) {}
};

class Board : public Widget, public ButtonListener
{
public:
	enum
	{
		COLS = 9,
		ROWS = 8,
		CELL = 0x34,                // 52 pixels
		GRID_X = 10,                // the grid's top left corner in the board
		GRID_Y = 0x27,
		FIRST_COL = 4,              // the first piece goes here
		FIRST_ROW = 3
	};

	enum State
	{
		STATE_PLAYING = 0,
		STATE_CLEARING = 1,         // a full row or column flashes and goes
		STATE_ENDING = 2,           // the game is over: the pieces shrink away
		STATE_GAME_OVER = 3,        // GAMEOVER fades in, then the score table follows
		STATE_WON = 4,              // every cell was lit: pieces and tiles vanish in groups of three
		STATE_BOARD_CLEARED = 5,    // the statistics panel waits for a click
		STATE_NEXT_BOARD = 6        // the next board fades in
	};

	AlchemyApp*		mApp;                   // +0x5c
	int				mState;                 // +0x60
	MTRand			mRand;                  // +0x64
	Buffer			mRecord;                // +0xa28 the game's moves, for the internet scores of the original (kept, never sent)
	std::string		mScript;                // +0xa64 "Move: n " commands: the test hook's input to the board
	int				mScriptPos;             // +0xa80 (saved, never used)
	Piece*			mCurPiece;              // +0xae8 the piece to place, or NULL
	bool			mAnimateLights;         // +0xaec false while the tutorial builds a position
	bool			mTimeTrial;             // +0xaed
	bool			mPristine;              // +0xaee true until the player has done anything
	int				mMoves;                 // +0xaf0 pieces placed
	int				mDiscards;              // +0xaf4 pieces forged
	int				mMultiMoves;            // +0xaf8 pieces placed next to more than one other
	double			mNextBoardFade;         // +0xb00 0..1 while the next board fades in
	Piece*			mGrid[ROWS][COLS];      // +0xb08
	bool			mLit[ROWS][COLS];       // +0xc28 the cell was used (its tile is lit)
	bool			mFlash[ROWS][COLS];     // +0xc70 the cell flashes (state 4)
	bool			mValid[ROWS][COLS];     // +0xcb8 the piece to place fits here
	int				mNumValid;              // +0xd00
	int				mHoverX;                // +0xd04 the cell the piece is over, or -1
	int				mHoverY;                // +0xd08
	double			mProgress;              // +0xd10 the forge's fill: the game ends at 3
	int				mScore;                 // +0xd18
	double			mProgRate;              // +0xd20 time trial: how fast the forge fills
	double			mProgDecay;             // +0xd28
	int				mAnimDir;               // +0xd30
	int				mAnimPct;               // +0xd34 0..100
	int				mClearRow;              // +0xd38
	int				mClearCol;              // +0xd3c
	int				mSparkle;               // +0xd40 updates left of the sparkles at the top of the board
	bool			mLightOn;               // +0xd44 a newly lit tile is dissolving in
	bool			mLightFading;           // +0xd45
	int				mLightFade;             // +0xd48
	int				mLightVariant;          // +0xd4c
	int				mLightCol;              // +0xd50
	int				mLightRow;              // +0xd54
	int				mLightPos;              // +0xd58 where in the order of pixels it is
	MemoryImage*	mLightBase;             // +0xd5c the cell before
	MemoryImage*	mLightLit;              // +0xd60 the cell after
	MemoryImage*	mLightOut;              // +0xd64 the cell as shown
	double			mLightMix;              // +0xd68 the placed piece's colour pulses toward white
	double			mLightMixStep;          // +0xd70
	bool			mBombOn;                // +0xd78 the explosion
	int				mBombX;                 // +0xd7c
	int				mBombY;                 // +0xd80
	int				mBombFrame;             // +0xd84
	double			mOverFade;              // +0xd88 GAMEOVER's opacity
	int				mOverTimer;             // +0xd90 updates until the score table
	int				mSkillBase;             // +0xd94 skill * 5 - 5
	int				mNumShapes;             // +0xd98 symbols in play (5..22)
	int				mNumColors;             // +0xd9c colours in play (5..9, as the random range)
	int				mBombChance;            // +0xda0 per mille
	int				mSkullChance;           // +0xda4 per mille
	MemoryImage*	mBgImage;               // +0xda8 the board without the pieces to place
	MemoryImage*	mFxImage;               // +0xdac the discarded piece's picture (50 x 50)
	Piece*			mFxPiece;               // +0xdb0 the discarded piece on its way to the forge
	bool			mTutorial;              // +0xdd0
	int				mTutStep;               // +0xdd4 -1 before the first
	int				mTutCount;              // +0xdd8 pieces placed in this step
	std::list<BombSpark> mSparks;           // +0xddc
	DWORD			mStartTick;             // +0xde8
	int				mElapsed;               // +0xdec seconds before the last start
	int				mTotalSeconds;          // +0xdf0 the game's length, when it ended
	int				mPause;                 // +0xdf4 pauses
	int				mTrialPause;            // +0xdf8 time trial: the pause that hides the board
	int				mLastWarning;           // +0xdfc update of the last warning sound
	int				mFullCount;             // +0xe00 updates with the forge full (time trial)
	int				mRun;                   // +0xe04 pieces since the last forge
	int				mLongestRun;            // +0xe08
	int				mBoardsCleared;         // +0xe0c
	std::string		mRankName;              // +0xe10
	int				mNextRank;              // +0xe2c the score of the next rank, 0: none
	bool			mHintOn;                // +0xe30 the places for the piece are shown (blinks)
	int				mHintTimer;             // +0xe34
	int				mNewPieceDelay;         // +0xe38
	int				mMouseX;                // +0xe3c
	int				mMouseY;                // +0xe40
	bool			mTutWaiting;            // +0xe44 a tutorial step's text waits for the continue button
	double			mRate;                  // +0xe48 points scale with the skill level (x2 in time trial)
	int				mLastShape;             // +0xe60 the last piece made
	int				mLastColor;             // +0xe64
	ButtonWidget*	mButton;                // +0xe68 the button of the panel or tutorial step (ids 0, 1, 2)
	WizardBox*		mHelpBox;               // +0xe6c the wizard with the messages
	bool			mHelpShown;             // +0xe70

public:
	Board(AlchemyApp* theApp);              // 0x407ebe
	virtual ~Board();                       // 0x4026fa

	// cells
	static int		ColToX(int theCol) { return theCol * CELL + GRID_X; }           // 0x4014e6
	static int		RowToY(int theRow) { return theRow * CELL + GRID_Y; }           // 0x401510
	static int		XToCol(int theX);                                                // 0x4014f3
	static int		YToRow(int theY);                                                // 0x40151d
	int				CountPieces();                                                   // 0x401304
	bool			CanPlace(Piece* thePiece, int theCol, int theRow, int* theReason); // 0x40153a
	void			ComputeValid();                                                  // 0x401682
	int				CountValid();                                                    // 0x4017ba
	void			ClearGrid();                                                     // 0x4017dd
	void			SetDifficulty();                                                 // 0x401220
	Piece*			NewPiece();                                                      // 0x401324
	void			NewCurrentPiece();                                               // 0x401c9b
	void			PlacePiece(Piece* thePiece, int theCol, int theRow);             // 0x401d15
	void			PlaceCurrentPiece();                                             // 0x406347
	void			UseForge();                                                      // 0x405e31
	void			UseForgeClicked();                                               // 0x4061d6
	bool			CheckAllLit();                                                   // 0x4062e5
	void			ShowHint();                                                      // 0x403c24

	// the game's course
	void			ComputeRank();                                                   // 0x403ee5
	void			GameOver();                                                      // 0x403f5c
	void			GameWon();                                                       // 0x406209
	void			BoardCleared();                                                  // 0x40400d
	void			StartNextBoard();                                                // 0x402292
	void			ShowHelpBox();                                                   // 0x40182f
	bool			HasProgress();                                                   // 0x4010e2
	void			Pause(bool isTrialMode);                                         // 0x401130
	void			Resume(bool isTrialMode);                                        // 0x40118b

	// the tutorial
	void			AdvanceTutorial();                                               // 0x402fcf

	// the moves, for the record
	void			RecordMove(int theIndex);                                        // 0x4011d5
	void			RecordForge(bool isLastOne);                                     // 0x4011fb
	void			RecordPlacement(int theCol, int theRow);                         // 0x402f42

	// drawing
	void			RedrawCell(int theCol, int theRow);                              // 0x401a82
	void			RedrawAllCells();                                                // 0x40204a
	void			DrawPieceImage(Graphics* g, Piece* thePiece, int theX, int theY); // 0x4019d1
	void			DrawStatLine(Graphics* g, const Rect& theRect, int theDX, int theDY, const SexyString& theLabel, const SexyString& theValue); // 0x40187c
	void			ShowMessage(const char* thePrefix, int theFirst = 1);            // 0x4114ff with the three strings thePrefix + "1".."3" (or from theFirst on)

	// the saved game
	bool			SaveGame();                                                      // 0x402a44
	bool			LoadGame();                                                      // 0x40589a
	std::string		SerializeGame();
	static std::string	GetSaveGameFileName();                                       // "userdata\savegame.dat"

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;       // 0x401014
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;   // 0x40105c
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x40578f
	virtual void	Draw(Graphics* g) override;                                      // 0x404923
	virtual void	Update() override;                                               // 0x406f2a
	virtual void	KeyChar(SexyChar theChar) override;                              // 0x4017a0 any key pauses or resumes
	virtual void	MouseMove(int x, int y) override;                                // 0x401707
	virtual void	MouseDown(int x, int y, int theClickCount) override;             // 0x4068f1
	virtual void	ButtonPress(int theId) override;                                 // 0x40186d
	virtual void	ButtonDepress(int theId) override;                               // 0x4053aa

	using Widget::Resize;
	using Widget::MouseDown;
	using ButtonListener::ButtonPress;
	using ButtonListener::ButtonDepress;
};

}

#endif
