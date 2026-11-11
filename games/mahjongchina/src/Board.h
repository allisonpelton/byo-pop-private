#ifndef __MAHJONGCHINA_BOARD_H__
#define __MAHJONGCHINA_BOARD_H__

// The board (0x40a380..0x40e100, vtable 0x51cd08, 0xb78 bytes): the play layer's object that holds the layout and
// the deal, draws the tiles, takes the pairs the player picks and flies them into the tray, undoes, shuffles, gives
// hints, and plays the escape: the two gold tiles (KIND_GOLD) end a round, and the tiles still on the board fly off for
// points. The play layer (PlayLayer.h, 0x404fe0) owns it and shows the score, time and moves it reports.

#include "Deal.h"
#include "Layout.h"
#include "Object.h"

#include <list>
#include <string>

namespace jb
{
class Font;
class Layer;
}

namespace mahjongchina
{

// 0x40a380 (vtable 0x51cd00, 0x28 bytes): a saved game, the layout as it is (0x40a3d0)
class SavedGame : public jb::Streamable
{
public:
	int mMode = 0;      // +0x04 the play layer's mode
	int mLevel = 0;     // +0x08 the level, or the layout's place in the list
	int mProgress = 0;  // +0x0c the escape's progress (0x532e3c)
	int mScore = 0;     // +0x10
	int mTime = 0;      // +0x14 the panel's seconds (0x409cf0)
	Layout* mLayout;    // +0x18
	int mMove = 0;      // +0x1c the board's move
	int mMagic = 0;     // +0x20 'MS'
	int mVersion = 0;   // +0x24

	// 0x40a380
	SavedGame(Layout* theLayout) : mLayout(theLayout) {}
	// 0x40a3a0, 0x40a3c0
	~SavedGame() override {}

	// 0x40a3d0
	void Serialize(jb::Stream& theStream) override;
	// 0x40a460
	bool IsValid() const { return mMagic == 0x4d53 && mVersion == 1; }
};

// 0x40a480 (0x20 bytes): a tile flying, into the tray, back onto the board or off it
struct Flight
{
	Tile* mTile;   // +0x00 null for a gold tile drawn in the tray
	int mKind;     // +0x04 the face drawn
	int mFromX;    // +0x08
	int mFromY;    // +0x0c
	int mToX = 0;  // +0x10
	int mToY = 0;  // +0x14
	int mTime;     // +0x18
	int mLeft;     // +0x1c

	// 0x40a480
	Flight(Tile* theTile, int theX, int theY, int theTime)
		: mTile(theTile), mKind(theTile->mKind), mFromX(theX), mFromY(theY), mTime(theTime), mLeft(theTime)
	{
	}
	// 0x40a4b0
	Flight(Tile* theTile, int theKind, int theX, int theY, int theTime)
		: mTile(theTile), mKind(theKind), mFromX(theX), mFromY(theY), mTime(theTime), mLeft(theTime)
	{
	}
	// 0x40a4e0
	void SetTo(int theX, int theY)
	{
		mToX = theX;
		mToY = theY;
	}
};

// 0x40a500, 0x40a580 (0x14 bytes): points or a word rising from where they were won
struct PointsText
{
	int mLeft = 800;    // +0x00
	int mX;             // +0x04
	int mY;             // +0x08
	std::string mText;  // +0x0c

	// 0x40a500: "+%d", or "%d" below 1
	PointsText(int thePoints, int theX, int theY);
	// 0x40a580
	PointsText(const char* theText, int theX, int theY) : mX(theX), mY(theY), mText(theText) {}
};

class PlayLayer;

// 0x40a5b0
class Board : public jb::Object
{
public:
	int mMode;                    // +0x90 the play layer's mode: 0 the escape, 1 a layout of the list
	int mLayoutId = 0;            // +0x94 the LAYOUTS resource
	bool mStarted = false;        // +0x98 the tiles are drawn
	bool mClockIdle = true;       // +0x99 the clock waits for the first click
	bool mOver = false;           // +0x9a the game takes no more pairs
	bool mEscaping = false;       // +0x9b the tiles left fly off for points
	int mQuadPercent = 0;         // +0x9c
	uint32_t mSeed = 0;           // +0xa0 the deal's seed
	bool mDealt = false;          // +0xa4 the kinds were dealt again (a shuffle or a saved game)
	int mPhase = 0;               // +0xa8 0 the tiles fly in, 1 a shuffle's tiles fly out, 2 they fly back
	int mSelImage = 0;            // +0xac the selected faces (0x40a7f0)
	jb::Image* mTiles = nullptr;  // +0xb0 every tile drawn on its back (0x40a7f0)
	Deal mDeal;                   // +0xb4
	Layout mLayout;               // +0xc4
	Tile* mSelected = nullptr;    // +0xec
	int mOffX = 0;                // +0xf0 the layout's offset on the board (0x40b690)
	int mOffY = 0;                // +0xf4
	int mPairs = 0;               // +0xf8 the free pairs (0x414d10)
	int mLeft = 0;                // +0xfc the tiles left
	int mHintIndex = 0;           // +0x100
	int mHintOrder[200];          // +0x104 the kinds of the free pairs, shuffled (0x40d560)
	int mPhaseTime = 0;           // +0x424
	int mPhaseLeft = 0;           // +0x428
	int mSelFrame = 0;            // +0x42c
	uint32_t mSelFrameTime = 0;   // +0x430
	int mSelFrames = 0;           // +0x434
	int mSelAnim = 0;             // +0x438 the image the selected special tile is animated with, or 0
	Flight* mFlights[42];         // +0x43c
	int mFlightCount = 0;         // +0x4e4
	int mFlightKind = 0;          // +0x4e8 what happens when the first flight lands (0x40bd70)
	int mTray[2] = {};            // +0x4ec the kinds of the last pair taken
	int mMove = 0;                // +0x4f4
	bool mAnimating = false;      // +0x4f8
	int mHintCount = 0;           // +0x4fc
	Tile* mHints[400];            // +0x500
	uint32_t mIdleSince = 0;      // +0xb40 the last click (0: the first update sets it 15 s back)
	int mKindsFree = 0;           // +0xb44 the kinds with a free pair
	jb::Font* mFont = nullptr;    // +0xb48
	bool mModeByte;               // +0xb4c the low byte of the mode; nothing reads it
	std::list<PointsText*> mTexts;  // +0xb50
	int mMusicMove = -1;          // +0xb58 the move the gold tiles' music started at
	int mSelSound = -1;           // +0xb5c
	bool mTipShown = false;       // +0xb60 a tip covers the board: it is still drawn
	bool mTipGold = false;        // +0xb61 the gold tiles' tip waits for both to be free
	bool mTipFlowers = false;     // +0xb62 the flowers' tip waits for a free flower
	bool mHighlight = false;      // +0xb63 the tip's tiles bob
	int mHighlightKind = 0;       // +0xb64
	int mHighlightOther = 0;      // +0xb68
	bool mStuckShown = false;     // +0xb6c
	jb::Layer* mTip = nullptr;    // +0xb70
	bool mShade = false;          // +0xb74 the covered tiles are shaded (0x532e49)

	// 0x40a5b0
	Board(int theMode);
	// 0x40a6f0, 0x40a710
	~Board() override;

	void Draw() override;                                          // 0x40b130
	void Init() override;                                          // 0x40aa70
	void Clear() override;                                         // 0x40a7d0
	bool OnCommand(int theCmd) override;                           // 0x40b500
	bool OnLButtonDown(int theFlags, int theX, int theY) override;  // 0x40c030
	bool OnRButtonDown(int theFlags, int theX, int theY) override;  // 0x40c1b0
	void OnChar(int theChar, int theRepeat, int theScan) override;  // 0x40b660
	bool Update() override;                                        // 0x40bd70
	bool OnTimer(int theId, int theIndex) override;                // 0x40dcc0
	void OnEnabledChanged() override;                              // 0x40e0e0

	PlayLayer* GetPlayLayer() { return (PlayLayer*)mLayer; }

	// 0x40a7f0: the tile set (the options' choice, 0-5)
	void SetStyle(int theStyle);
	// 0x40aa40: theSeed -1 is the clock
	void SetLayout(int theLayoutId, int theQuadPercent, uint32_t theSeed);
	// 0x40abd0
	void ClearFlights();
	// 0x40ac20
	void ClearTexts();
	// 0x40acb0: an error box when the layout does not load
	void LoadLayout();
	// 0x40ad70
	// 0x402520 inlined here: the copy of the layout's name (+0x20)
	std::string GetLayoutName() { return mLayout.mName; }
	// 0x40ada0
	void ClearHints();
	// 0x40ade0
	void Reset();
	// 0x40ae30: a saved game at theMove
	void Resume(int theMove);
	// 0x40ae60: the tiles fly in; theDeal deals them (with mDealt too), theKeep keeps the layout's state, theNewSeed
	// takes the clock as the seed
	void Start(bool theDeal, bool theKeep, bool theNewSeed);
	// 0x40af70: the shuffle (the tiles fly out, 0x40b0a0 deals them again)
	void Shuffle();
	// 0x40b0a0
	void Reshuffle();
	// 0x40b120
	void EndShuffle() { mPhase = 0; }
	// 0x40b3d0
	void DrawFlight(Flight* theFlight);
	// 0x40b650
	void ApplyMagnetLinks() { mLayout.ApplyMagnetLinks(); }
	// 0x40b690: the tiles' rects and the layout's offset
	void Measure();
	// 0x40b780: where the tiles fly in from
	void SetFlyIn(int theKind);
	// 0x40b900: the shuffle's places off the board
	void SetFlyOut();
	// 0x40b9a0
	void SwapFlyOut();
	// 0x40b9e0
	void DrawTile(Tile* theTile);
	// 0x40bc10: a face from the tile set, and the selected face over it
	void DrawFace(int theX, int theY, int theKind, bool theSelected, int theOffset);
	// 0x40c330: the clock starts at the first click
	void StartClock();
	// 0x40c350
	bool CanSelect(Tile* theTile);
	// 0x40c3b0: a special tile selected pairs only with a tile it can see
	bool CanPair(Tile* theTile);
	// 0x40c400
	void StartSelAnim();
	// 0x40c4d0
	void Take(Tile* theTile, Tile* theOther);
	// 0x40c6a0: the points of theCount tiles like theTile; theBonus adds 500 for the kind of the pair before
	void AddPoints(Tile* theTile, int theCount, bool theBonus);
	// 0x40c8b0: a tile flown off at the escape
	void AddEscapePoints(Tile* theTile);
	// 0x40c990
	void EndTake();
	// 0x40c9c0: the key takes a lock (KIND_KEY with KIND_LOCK)
	void TakeKeyAndLock(Tile* theTile, Tile* theOther);
	// 0x40cab0, 0x40cb60, 0x40cc00
	void JoinKeyAndLock();
	void FlyKeyAndLock();
	void EndKeyAndLock();
	// 0x40cc10: the joker (KIND_JOKER) takes every free tile of the other's kind
	void TakeJoker(Tile* theTile, Tile* theOther);
	// 0x40cd80: the magnet (KIND_MAGNET) takes the other's kind
	void TakeMagnet(Tile* theTile, Tile* theOther);
	// 0x40ce30, 0x40ced0
	void FlyMagnet();
	void EndMagnet();
	// 0x40cef0
	void FinishFlights();
	// 0x40cf20
	void Undo();
	// 0x40d1f0: the tray shows the pair taken at mMove
	void UpdateTray();
	// 0x40d2f0
	void UndoPair(Tile* theTile, Tile* theOther);
	// 0x40d3c0, 0x40d460, 0x40d510
	void EndUndo();
	void UndoMagnet();
	void EndUndoMagnet();
	// 0x40d560: the free pairs counted, the hints' order, the tips and the gold tiles' music
	void UpdateMoves();
	// 0x40d6e0
	void ShowHint(bool theAsked);
	// 0x40d870
	void StartHints();
	// 0x40d8b0: the gold tiles end the round
	void TakeGold(Tile* theTile, Tile* theOther);
	// 0x40da30, 0x40da70
	void EndGold();
	void FlyOff();
	// 0x40dc00
	void EndRound();
	// 0x40dea0
	void EndHighlight() { mHighlight = false; }
	// 0x40deb0
	void EndGoldInTray();
	// 0x40df00: Enter or space skips the tiles flying off
	void SkipFlyOff();
	// 0x40dfa0: the layout to save, or null
	Layout* GetSaveLayout();
	// 0x40dfd0: no free pair
	void CheckStuck();
	// 0x40e090
	void ClearSelection();
	// 0x40e0c0: a tip (OnTimer)
	void ShowTip(int theId) { AddTimer(theId, 100, 1, 1); }
};

} // namespace mahjongchina

#endif
