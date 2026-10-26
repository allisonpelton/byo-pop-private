#ifndef __ROCKETMANIA_BOARD_H__
#define __ROCKETMANIA_BOARD_H__

// The game of Rocket Mania Deluxe: the Board (RocketManiaGame 0x41ae80-0x429ed0, 141 functions) and the objects it
// keeps. The original's field offsets are given as comments (board+0x..., piece+0x...); the port's base classes have
// other sizes, so code never depends on them. Fields keep their offset in the name (m5c) until the code that uses them
// has been read; the ones that are understood have a name and a comment. The saved game (savea/b/c.dat) writes the
// fields in the order of 0x41c830, which is the order of the lists below.
//
// The playing field is 6 columns wide (board+0x4e4) and 9 rows high (board+0x4e8) of 42 by 42 cells: the cell
// (row, column) is at x = 250 + 42 column, y = 78 + 42 row (0x4220e0, 0x422100) and the grid is stored row by row,
// grid[row * 6 + column] at board+0x114. Around it are five lines of edge pieces: the left (column -1, board+0x208,
// 9 pieces: where the fuses start), the right (column 6, board+0x22c, 9 rocket launchers), the top (row -1,
// board+0x1ec, 6 tiles that wait to fall in arcade mode) and the spare tile (board+0x204). An earlier note called the
// 6 "rows" and the 9 "columns"; the draw and the connection tables settled it (see docs/GAME_NOTES.md).
//
// docs/GAME_NOTES.md has the map of what is where.

#include "Widget.h"
#include "ButtonListener.h"
#include "Common.h"
#include "Effects.h"
#include <list>
#include <string>
#include <vector>

namespace Sexy
{

class RocketApp;
class HoverTip;
class WidgetManager;
class MTRand;
class Graphics;
class Buffer;
class Font;
class Image;

// The connections of a pipe piece: bit 1 up, 2 right, 4 down, 8 left (table at 0x4c78b8, indexed by kind * 4 +
// rotation: straight 5 a 5 a, corner 3 6 c 9, T e d b 7, cross f f f f, end 1 2 4 8).
extern const unsigned int gPipeMask[20];
// The four neighbours (0x4c7908): row step, column step, this piece's bit, the neighbour's matching bit.
struct PipeStep { int mRow, mCol, mOut, mIn; };
extern const PipeStep gPipeSteps[4];

// The 0xb8-byte object of the original (constructor 0x41ad90): a tile of the grid, one of the pieces that wait or
// move along the edges, a flame or a rocket. Saved fields are written by 0x41c5a0 and read by 0x41b430.
// mType: 0 falling about, 1 a tile in the grid, 2 a tile that drops, 3 a flash, 4 a burning tile, 5 an edge piece (a
// rocket on the right, a fuse on the left), 6 a special (a coin, a bomb, ...: mKind tells which), 7 at the start.
struct Piece
{
	int			mId;            // +0x00 the unique number (lists are searched by it, saves keep it)
	bool		m04;            // +0x04 (set once the references of a loaded piece are resolved, 0x41b690)
	double		mX;             // +0x08
	double		mY;             // +0x10
	double		m18;            // +0x18 starts as a random number from -5 to 5 (the horizontal speed of a tile that flies)
	double		m20;            // +0x20 (the vertical speed of the same)
	bool		mMoving;        // +0x28 the piece slides from (m40, m44) to (m48, m4c)
	double		m30;            // +0x30 the progress of that slide
	double		m38;            // +0x38 its step
	int			m40, m44;       // +0x40 where it started (x, y)
	int			m48, m4c;       // +0x48 where it goes (x, y)
	int			mRow;           // +0x50 (-1 on the top line)
	int			mColumn;        // +0x54 (-1 on the left line, 6 on the right line)
	int			mFromRow, mFromColumn;  // +0x58 the original reads these as the burn's start
	bool		m60, m61;       // +0x60 (m60: the tile has been used in a launch)
	int			mType;          // +0x64 7 at the start; see above
	int			mKind;          // +0x68 the picture's row: 0 to 4 (the generator 0x42a590 draws it), -1 none; for a special its kind
	int			mRotation;      // +0x6c the picture's column: 0 to 3, -1 none
	int			mBorn;          // +0x70 the update count at which it was made (saved as an age)
	int			mPath;          // +0x74 the number of the connection path a tile belongs to (0 none)
	int			mPathFrom;      // +0x78 -1 at the start
	int			m7c;            // +0x7c
	double		m80;            // +0x80 the tilt of a tile that was hit (decays to 0)
	int			m88;            // +0x88 the board's update count at the creation (0x41ad90's argument)
	Piece*		mSpecial;       // +0x8c a special attached to this tile (in a save its id)
	Piece*		mHost;          // +0x90 the tile a special is attached to
	bool		m94;            // +0x94 the piece is in the board's list of moving pieces
	int			m98;            // +0x98
	Effect*		mFlash;     // +0x9c an effect attached to it
	int			mIndex;         // +0xa0 the launcher's number, or other counters
	int			mIndex2;        // +0xa4
	int			ma8;            // +0xa8 (3 at the start: the hits a bomb takes)
	int			mac;            // +0xac (the shake of a tile that was moved, counts down by 2)
	int			mb0, mb4;       // +0xb0 where a moved tile came from
	int			mLoadedSpecial, mLoadedHost;    // (port) the numbers of mSpecial and mHost that a save holds, until 0x41b690 resolves them
	Piece(int theBorn);
};

// 0x42a510, 0x74 bytes: draws the next piece of the board by the tables at 0x4e6800/0x4e6828 and keeps the counts of the
// launches (the bonus messages of 0x42aa40).
class Board;
class PieceGenerator
{
public:
	int			mLaunched[10];      // +0x00 the number of rockets launched with each launcher in this round
	int			mCoins[4];          // +0x28
	int			mTotal;             // +0x38 the launches of the round (0x42aa40 shows the bonus for them)
	int			m3c;                // +0x3c
	int			mMeter[9];          // +0x40 per launcher, 0 to 9 (0x42add0 raises one, 0x42adf0 asks whether all are full)
	RocketApp*	mApp;               // +0x64
	Board*		mBoard;             // +0x68
	MTRand*		mRand;              // +0x6c
	int			m70;                // +0x70 a score the launches add up (set by 0x42aa40)
	// the tables 0x42a460 builds: the thresholds by which a kind of tile is drawn (0x4f6cfc, five of them), the table
	// number it was built for (0x4f6cf8)
	int			mThreshold[5];
	int			mTableIndex;

	PieceGenerator(RocketApp* theApp, Board* theBoard, MTRand* theRand);
	void		ResetCounts();                          // 0x42aa00 (and 0x42aa20)
	void		BuildThresholds(int theDifficulty, int theLevel, int theMode);   // 0x42a460
	int			DrawKind();                             // 0x42a590
	int			SpecialInterval();                      // 0x42a570
	static unsigned char SpecialWeight(int theKind);    // 0x42a5c0
	int			DrawSpecial();                          // 0x42a600 -1 none, else 2, 0 or 0xc
	int			DrawBombKind(int theLevel);             // 0x42a6b0
	bool		SpecialNow();                           // 0x42a720
	int			RocketsForLevel(int theLevel);          // 0x42a750
	void		ResetForLevel();                        // 0x42a770
	int			FallDelay(int theLevel);                // 0x42a890
	int			Rows();                                 // 0x42a940
	int			Rows20();                               // 0x42a980
	void		CountSpecial(Piece* thePiece);          // 0x42a990
	void		CountPiece(Piece* thePiece);            // 0x42a9c0
	void		ShowBonus();                            // 0x42aa40 (the bonus messages of the launch count)
	void		Seed(unsigned int* theKind, int* theB, Board* theBoard, int theColumn);   // 0x42ad50
	unsigned int Fuse(int theRow);                      // 0x42ad70
	int			FullestMeter();                         // 0x42ada0
	void		RaiseMeter(int theRow);                 // 0x42add0
	bool		AllMetersFull();                        // 0x42adf0
	int			Upgrades(int theCoins, int theMode, int theIndex);   // 0x42ae10: how many launchers the coins upgrade (mode 0), or the launcher of the n-th upgrade (mode 1)
};

// 0x429ed0: the floating text (points, messages) made by the board; 0x98 bytes. It floats up one pixel every third
// update while 'mStatic' is false, flashes between two colours and is removed when its life runs out.
class Points : public Widget
{
public:
	RocketApp*	mApp;
	std::string	mText;
	Font*		mFont;              // +0x78
	Image*		mBackImage;         // +0x58 (set by 0x42a230)
	Color		mBackColor;         // +0x5c
	int			mLife;              // +0x64
	int			mPhase;             // +0x7c
	bool		mStatic;            // +0x80
	int			mTextWidth;         // +0x84
	int			mLeft, mTop;        // +0x88 the position the text is centred on
	int			mMode;              // +0x90 0 centred
	int			mBaseline;          // +0x94
	Points(RocketApp* theApp, Font* theFont, const std::string& theText, int theX, int theY, int theLife, int theMode);
	void		SetBack(Image* theImage, int r, int g, int b);   // 0x42a230
	void		Position();                                     // 0x42a150
	virtual void Draw(Graphics* g) override;                     // 0x42a2d0
};

class Board : public Widget, public ButtonListener
{
public:
	RocketApp*		mApp;               // +0x4dc
	MTRand*			mRand;              // +0x110 (0x9c4 bytes in the original: the 2003 Mersenne twister)
	PieceGenerator*	mGen;               // +0x4ec
	EffectManager*	mEffects;           // +0x4f0
	HoverTip*		mTip;               // +0x530 the tip that shows when the pointer rests on a part of the board
	int				mCols;              // +0x4e4 6
	int				mRows;              // +0x4e8 9

	// ------------------------------------------------------------------ saved state, in the file's order
	// (+0x4c is the base class's mUpdateCnt, which Widget::Update counts)
	int				mState;             // +0x5c 0 playing, 1 game over, 2 finished (saved), 3, 4/5/6/7 end-of-level screens
	int				mScore;             // +0x60
	double			m68;                // +0x68 the length of the night (classic mode)
	int				mLevel;             // +0x70 the level number counted from 0 (0x42a460 reads it)
	int				m74, m78, m7c, m80; // +0x74 m74/m78 the launches the level needs; m7c/m80 the pieces on the left and right
	bool			m84;                // +0x84
	int				m88;                // +0x88
	int				m8c[14];            // +0x8c
	Piece*			mGrid[9][6];        // +0x114 row by row, six columns each
	Piece*			mTop[6];            // +0x1ec the tiles that wait above the first row
	Piece*			mSpare;             // +0x204 the next tile
	Piece*			mLeft[9];           // +0x208 the fuses (column -1)
	Piece*			mRight[9];          // +0x22c the rockets (column 6)
	int				m250[9];            // +0x250
	int				m280[9];            // +0x280
	int				m2a4[9];            // +0x2a4
	bool			m2c8[9];            // +0x2c8
	bool			m2d1[9];            // +0x2d1
	int				m2dc, m2e0, m2e4;   // +0x2dc m2dc the fall timer, m2e0 its smooth value, m2e4 the score shown
	bool			m2e8;               // +0x2e8 a launch is burning
	int				m2ec, m2f0, m2f4, m2f8;     // +0x2ec m2ec the burn countdown, m2f0 the path burning, m2f4 flames, m2f8 tiles dropped
	double			m300, m308, m310, m318, m320;   // +0x300 m300 the fraction of the level done, m308 its smooth value; m310 the night left, m318 smooth, m320 its step
	int				m328[6];            // +0x328 the countdowns of the six hints
	bool			m340[6];            // +0x340
	int				m348, m34c, m350;   // +0x348 m350: paused
	double			m358;               // +0x358 the fade of the pause
	int				m360, m364, m368, m36c, m370, m374;     // +0x360 m360 the seconds played, m364/m368 the tick count, m36c the end-of-level countdown
	int				m108, m10c;         // +0x108
	int				m380, m378, m37c;   // +0x380
	int				m384[30];           // +0x384
	bool			m3fc[30];           // +0x3fc
	bool			m41a[30];           // +0x41a
	bool			m438, m439;         // +0x438
	int				m43c, m440, m444, m448, m44c, m450, m454, m458;     // +0x43c
	bool			m45c;               // +0x45c
	int				m460, m464;         // +0x460
	int				m468[10];           // +0x468
	int				m490, m494;         // +0x490
	bool			m498, m499;         // +0x498
	int				m49c, m4a0, m4a4;   // +0x49c
	bool			m4a8, m4a9;         // +0x4a8
	int				m4ac, m4b0, m4b4, m4b8, m4bc;   // +0x4ac
	bool			m4c0;               // +0x4c0
	int				m4c4;               // +0x4c4
	bool			m4c8, m4c9;         // +0x4c8
	double			m4d0;               // +0x4d0
	bool			m4d8, m4d9, m4da;   // +0x4d8
	int				mFc, m100, m104;    // +0xfc
	int				mC4, mC8, mCc, mD0, mD4, mD8, mDc, mE0, mE4, mE8, mEc, mF0, mF4, mF8;   // +0xc4

	// ------------------------------------------------------------------ not saved
	int				m4f4, m4f8;         // +0x4f4 the pointer's place
	Effect*			m4fc;               // +0x4fc the sweep effect of state 7
	int				m51c, m50c, m510;   // +0x51c; +0x50c the last click
	Widget*			m4e0;               // +0x4e0 a widget the board owns (deleted in RemovedFromManager)
	Widget*			m51cWidget;         // +0x51c
	int				m520;               // +0x520
	Piece*			m514;               // +0x514 the piece the pointer is on
	bool			m518, m519;         // +0x518
	std::list<Piece*>	mObjects;       // +0x278 (count at +0x27c) the pieces that move about
	std::list<Piece*>	m504;           // +0x504 pieces read from a save, until their references are set
	std::list<Points*>	mTexts;         // +0x528 the floating texts

public:
	Board(RocketApp* theApp, bool theContinuing);
	virtual ~Board();

	// Widget
	virtual void	Update() override;
	virtual void	Draw(Graphics* g) override;
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;   // 0x424260
	virtual void	OrderInManagerChanged() override;                               // 0x424340
	virtual void	ButtonDepress(int theId) override;
	virtual void	KeyChar(SexyChar theChar) override;                     // 0x427a90
	virtual void	MouseLeave() override;                                  // 0x4285b0
	virtual void	MouseMove(int x, int y) override;                       // 0x4285d0
	virtual void	MouseDown(int x, int y, int theClickCount) override;    // 0x4283e0
	virtual void	MouseUp(int x, int y, int theClickCount) override;      // 0x428520
	virtual void	MouseDrag(int x, int y) override;                       // 0x4281a0

	static int		CellX(int theColumn) { return theColumn * 42 + 250; }     // 0x4220e0
	static int		CellY(int theRow) { return theRow * 42 + 78; }            // 0x422100

	// ---- drawing (BoardDraw.cpp)
	void			DrawTile(Graphics* g, Piece* thePiece, int theHint);                // 0x41e770
	void			DrawTop(Graphics* g, Piece* thePiece);                              // 0x41e3e0
	void			DrawSpare(Graphics* g, Piece* thePiece);                            // 0x41e530
	void			DrawEdge(Graphics* g, Piece* thePiece, int theRow, int theColumn);  // 0x41ec20
	void			DrawMover(Graphics* g, Piece* thePiece);                            // 0x41d420
	void			DrawRocket(Graphics* g, int theIndex, int theX, int theY);          // 0x41d620
	void			DrawSpecial(Graphics* g, Piece* thePiece);                          // 0x41d670
	void			DrawStretched(Graphics* g, Image* theImage, double theScale, Piece* thePiece, bool theOdd);
	int				SpecialDy(Piece* thePiece);
	int				ChestImage();                                                       // 0x41f240
	void			DrawCoins(Graphics* g);                                             // 0x421b80
	void			DrawWiggle(Graphics* g, const SexyString& theText, int thePhase, int theRed, int theGreen, int theBlue, int theAmplitude);  // 0x4219a0
	void			DrawOutlined(Graphics* g, const SexyString& theText, int theLeft, int theRight, int theSideY, int theAboveY, int theBelowY);

	// ---- saved games (BoardSave.cpp)
	Piece*			FindLoadedPiece(int theId);                             // 0x41b660: a piece of m504 by its number
	void			ResolveLoadedPieces();                                  // 0x41b690: the references of the pieces read from a save
	void			WritePiece(Buffer* theBuffer, Piece* thePiece);         // 0x41c5a0
	void			ReadPiece(const Buffer* theBuffer, Piece* thePiece);    // 0x41b430
	Piece*			LoadPiece(const Buffer* theBuffer);                     // 0x41b6f0
	template<class Visitor> void	Fields(Visitor& theVisitor);            // the plain fields, in the file's order
	bool			Save();                                                 // 0x41c830
	bool			Load();                                                 // 0x41b880
	void			AfterLoad();                                            // 0x41b7e0

	// ---- setup (BoardSetup.cpp)
	void			ResetState(bool theContinuing);                         // 0x422120
	Piece*			AddRight(int theRow);                                   // 0x422390
	void			ClearBoard(bool theKeepPieces);                         // 0x422460
	void			SetupBoard(bool theContinuing, bool theLoaded);         // 0x422620
	Piece*			NewSpare(int theColumn);                                // 0x422c50 (0x423f30 makes the same for the top line)
	Piece*			NewTop(int theColumn);                                  // 0x423f30
	Piece*			NewPiece();                                             // new Piece(mUpdateCnt)

	// ---- the logic (BoardLogic.cpp)
	void			DropPiece(Piece* thePiece);                             // 0x422a90
	void			SettleAll(bool theFlag);                                // 0x422bc0
	void			LevelEffect(int theAction, int theKind, int theX, int theY);   // 0x422d30
	void			Burst(int theKind, int theX, int theY);                 // 0x422df0
	void			BurstSome(int theKind, int theX, int theY);             // 0x4230b0
	void			ComboSound(double theX, double theY);                   // 0x423120 (the doubles are hidden by the decompiler)
	static int		BonusFor(int theIndex);                                 // 0x4231a0
	void			SmokePuff(Piece* thePiece);                             // 0x423200
	void			DestroySpecial(Piece* thePiece, bool theFlag);          // 0x423270
	int				RandomTick();                                           // 0x423b00
	Piece*			MakeSpecial(int theKind, Piece* theHost);               // 0x423b40
	Piece*			MaybeSpecial(Piece* theHost);                           // 0x423ca0
	Piece*			FindFreeTile();                                         // 0x423cd0
	void			AttachBomb(int theIndex);                               // 0x423d10
	void			AttachSpecial10();                                      // 0x423e20
	void			EndLevelA();                                            // 0x425940 (the level is done: the end-of-level countdown starts)
	void			EndLevelB();                                            // 0x425980
	void			DropTop();                                              // 0x424010
	int				DropTopTimed();                                         // 0x424180
	Points*			AddText(const std::string& theText, int theX, int theY);   // 0x424370
	void			AddScoreText(int theScore, int theX, int theY);         // 0x4244f0
	int				CountBurning();                                         // 0x4246c0
	void			Ping();                                                 // 0x424700
	void			UpdatePieces();                                         // 0x424730
	void			Flood(int theColumn, int theRow, int thePath, int* theCount, unsigned int* theEnds, int theFrom);  // 0x4251b0
	void			StartBurn(int thePath);                                 // 0x425410
	void			Launch();                                               // 0x425490
	int				AddScore(int thePoints);                                // 0x4259c0 (the points are a stack argument the decompiler hides)
	void			MakeFlame0(); void MakeFlame1();                        // 0x425a00, 0x425a30
	void			MakeMarker(int theRow, int theKind);                    // 0x425a60
	void			MakeRocket(int theRow);                                 // 0x425a80
	void			FinishBurn();                                           // 0x425b50
	void			Ignite(int theRow, int theColumn, int theToRow, int theToColumn, bool theBlocked);  // 0x425ca0
	int				ResolveLaunch(int thePath);                             // 0x425e30
	bool			BothEnds();                                             // 0x426030
	void			Trace(int theColumn, unsigned int theMask, int theEnd, int theOtherEnd, int* thePath, bool theStart);  // 0x426070
	bool			TraceUp(int theColumn, unsigned int theMask, int theEnd, int theOtherEnd, int* thePath, bool theStart);  // 0x4261e0
	void			Connections();                                          // 0x426370
	void			SmoothFraction();                                       // 0x426580
	void			NightTimer();                                           // 0x426620
	void			Timers();                                               // 0x426860
	bool			AllTiles();                                             // 0x426eb0
	void			Bounce(Piece* thePiece);                                // 0x427830
	void			HitBombs();                                             // 0x427910
	Piece*			PieceAtPoint(int theX, int theY);                       // 0x427ab0
	int				LowestEmpty(int theColumn);                             // 0x427b70
	void			MovePiece(Piece* thePiece, int theRow, int theColumn);  // 0x427ba0
	void			Shock(int theRow, int theColumn, int theRadius2);       // 0x427c50
	void			RemovePiece(Piece* thePiece, bool theFlag);             // 0x427d00 (deletes it)
	void			AddMover(Piece* thePiece);                              // 0x427f90
	void			EraseMover(Piece* thePiece);                            // 0x427ff0
	void			AddFlash(Piece* thePiece, int theKind);                 // 0x428100
	void			F428780();
	bool			F428ae0();
	void			RotatePiece(Piece* thePiece, bool theCounter);          // 0x428340
	bool			EdgeOpen(int theRow, int theColumn);                    // 0x428e80
	void			F4282a0(int thePath);
	void			F428ef0();
	void			F428d10();
	void			ShowBuyRockets();                                      // 0x428620
	void			F4286b0();                                             // 0x4286b0: after the level-up screen, the question of the next level
	SexyString		RankName(int* theRank, int* thePointsToNext);          // 0x428ca0: the rank of the score, how far the next one is
	void			NextLevel();                                           // 0x428bc0: the level after the buy-rockets screen
	void			ShowLevelUp(int theKind);                              // the level-up dialog (0x40c2c0) as the board opens it
	void			F428de0(int theValue);
	void			F421de0(int theKind);
	void			F421f30(bool theFlag);
	void			F41f280(int theIndex);
	void			F429060(int theRow);
	void			F429050();
	void			F429080();
	void			F428090();
};

}

#endif
