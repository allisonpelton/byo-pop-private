#ifndef __MAHJONG_BOARD_H__
#define __MAHJONG_BOARD_H__

// The board (0x40a200..0x40e060 (mahjongchina), 0x40a380..0x40e100 (mahjongjapan), vtable 0x51bc90 (mahjongchina),
// 0x51cd08 (mahjongjapan), 0xb78 bytes): the play layer's object that holds the layout and the deal, draws the tiles,
// takes the pairs the player picks and flies them into the tray, undoes, shuffles, gives hints, and plays the escape:
// the two gold tiles (KIND_GOLD) end a round, and the tiles still on the board fly off for points. The play layer
// (PlayLayer.h, 0x405280 (mahjongchina), 0x404fe0 (mahjongjapan)) owns it and shows the score, time and moves it
// reports.

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

namespace mahjong
{

// 0x40a200 (mahjongchina), 0x40a380 (mahjongjapan) (vtable 0x51bc88 (mahjongchina), 0x51cd00 (mahjongjapan), 0x28
// bytes): a saved game, the layout as it is (0x40a250 (mahjongchina), 0x40a3d0 (mahjongjapan))
class SavedGame : public jb::Streamable
{
public:
	int mMode = 0;      // +0x04 the play layer's mode
	int mLevel = 0;     // +0x08 the level, or the layout's place in the list
	int mProgress = 0;  // +0x0c the escape's progress (0x53229c (mahjongchina), 0x532e3c (mahjongjapan))
	int mScore = 0;     // +0x10
	int mTime = 0;      // +0x14 the panel's seconds (0x409bb0 (mahjongchina), 0x409cf0 (mahjongjapan))
	Layout* mLayout;    // +0x18
	int mMove = 0;      // +0x1c the board's move
	int mMagic = 0;     // +0x20 'MS'
	int mVersion = 0;   // +0x24

	// 0x40a200 (mahjongchina), 0x40a380 (mahjongjapan)
	SavedGame(Layout* theLayout) : mLayout(theLayout) {}
	// 0x40a220 (mahjongchina), 0x40a3a0 (mahjongjapan), 0x40a240 (mahjongchina), 0x40a3c0 (mahjongjapan)
	~SavedGame() override {}

	// 0x40a250 (mahjongchina), 0x40a3d0 (mahjongjapan)
	void Serialize(jb::Stream& theStream) override;
	// 0x40a2e0 (mahjongchina), 0x40a460 (mahjongjapan)
	bool IsValid() const { return mMagic == 0x4d53 && mVersion == 1; }
};

// 0x40a300 (mahjongchina), 0x40a480 (mahjongjapan) (0x20 bytes): a tile flying, into the tray, back onto the board or
// off it
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

	// 0x40a300 (mahjongchina), 0x40a480 (mahjongjapan)
	Flight(Tile* theTile, int theX, int theY, int theTime)
		: mTile(theTile), mKind(theTile->mKind), mFromX(theX), mFromY(theY), mTime(theTime), mLeft(theTime)
	{
	}
	// 0x40a330 (mahjongchina), 0x40a4b0 (mahjongjapan)
	Flight(Tile* theTile, int theKind, int theX, int theY, int theTime)
		: mTile(theTile), mKind(theKind), mFromX(theX), mFromY(theY), mTime(theTime), mLeft(theTime)
	{
	}
	// 0x40a360 (mahjongchina), 0x40a4e0 (mahjongjapan)
	void SetTo(int theX, int theY)
	{
		mToX = theX;
		mToY = theY;
	}
};

// 0x40a380 (mahjongchina), 0x40a500 (mahjongjapan), 0x40a400 (mahjongchina), 0x40a580 (mahjongjapan) (0x14 bytes):
// points or a word rising from where they were won
struct PointsText
{
	int mLeft = 800;    // +0x00
	int mX;             // +0x04
	int mY;             // +0x08
	std::string mText;  // +0x0c

	// 0x40a380 (mahjongchina), 0x40a500 (mahjongjapan): "+%d", or "%d" below 1
	PointsText(int thePoints, int theX, int theY);
	// 0x40a400 (mahjongchina), 0x40a580 (mahjongjapan)
	PointsText(const char* theText, int theX, int theY) : mX(theX), mY(theY), mText(theText) {}
};

class PlayLayer;

// What the board's flights are doing (Board::mFlightKind), with the step that follows when the first one lands
enum
{
	FLIGHT_TAKEN = 0,             // a pair to the tray: EndTake
	FLIGHT_ONTO_MAGNET = 1,       // the other tile onto the magnet: FlyMagnet
	FLIGHT_MAGNET_TO_TRAY = 2,    // EndMagnet
	FLIGHT_KEY_ONTO_LOCK = 3,     // JoinKeyAndLock
	FLIGHT_KEY_LOCK_JOINED = 4,   // FlyKeyAndLock
	FLIGHT_KEY_LOCK_TO_TRAY = 5,  // EndKeyAndLock
	FLIGHT_UNDO = 6,              // a pair back to the board: EndUndo
	FLIGHT_UNDO_MAGNET = 7,       // the magnet back: UndoMagnet
	FLIGHT_UNDO_MAGNET_OUT = 8,   // the tile it took out of it: EndUndoMagnet
	FLIGHT_GOLD = 9,              // the gold tiles: EndGold
	FLIGHT_BEFORE_FLY_OFF = 10,   // FlyOff
	FLIGHT_GOLD_TO_TRAY = 0xb,    // EndGoldInTray
};

// The board's tiles as a whole (Board::mPhase)
enum
{
	PHASE_FLY_IN = 0,        // the tiles fly in
	PHASE_SHUFFLE_OUT = 1,   // a shuffle's tiles fly out
	PHASE_SHUFFLE_BACK = 2,  // and back
};

// 0x40a430 (mahjongchina), 0x40a5b0 (mahjongjapan)
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
	int mPhase = PHASE_FLY_IN;    // +0xa8
	int mSelImage = 0;            // +0xac the selected faces (0x40a670 (mahjongchina), 0x40a7f0 (mahjongjapan))
	// +0xb0 every tile drawn on its back (0x40a670 (mahjongchina), 0x40a7f0 (mahjongjapan))
	jb::Image* mTiles = nullptr;
	Deal mDeal;                   // +0xb4
	Layout mLayout;               // +0xc4
	Tile* mSelected = nullptr;    // +0xec
	// +0xf0 the layout's offset on the board (0x40b650 (mahjongchina), 0x40b690 (mahjongjapan))
	int mOffX = 0;
	int mOffY = 0;                // +0xf4
	int mPairs = 0;               // +0xf8 the free pairs (0x414790 (mahjongchina), 0x414d10 (mahjongjapan))
	int mLeft = 0;                // +0xfc the tiles left
	int mHintIndex = 0;           // +0x100
	// +0x104 the kinds of the free pairs, shuffled (0x40d500 (mahjongchina), 0x40d560 (mahjongjapan))
	int mHintOrder[200];
	int mPhaseTime = 0;           // +0x424
	int mPhaseLeft = 0;           // +0x428
	int mSelFrame = 0;            // +0x42c
	uint32_t mSelFrameTime = 0;   // +0x430
	int mSelFrames = 0;           // +0x434
	int mSelAnim = 0;             // +0x438 the image the selected special tile is animated with, or 0
	Flight* mFlights[42];         // +0x43c
	int mFlightCount = 0;         // +0x4e4
	// +0x4e8 what is flying, so what happens when the first flight lands (0x40bd30 (mahjongchina),
	// 0x40bd70 (mahjongjapan))
	int mFlightKind = FLIGHT_TAKEN;
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
	// +0xb74 the covered tiles are shaded (0x5322a9 (mahjongchina), 0x532e49 (mahjongjapan))
	bool mShade = false;

	// 0x40a430 (mahjongchina), 0x40a5b0 (mahjongjapan)
	Board(int theMode);
	// 0x40a570 (mahjongchina), 0x40a6f0 (mahjongjapan), 0x40a590 (mahjongchina), 0x40a710 (mahjongjapan)
	~Board() override;

	void Draw() override;                                          // 0x40b000 (mahjongchina), 0x40b130 (mahjongjapan)
	void Init() override;                                          // 0x40a940 (mahjongchina), 0x40aa70 (mahjongjapan)
	void Clear() override;                                         // 0x40a650 (mahjongchina), 0x40a7d0 (mahjongjapan)
	bool OnCommand(int theCmd) override;                           // 0x40b3e0 (mahjongchina), 0x40b500 (mahjongjapan)
	bool OnLButtonDown(int theFlags, int theX, int theY) override;  // 0x40bff0 (mahjongchina), 0x40c030 (mahjongjapan)
	bool OnRButtonDown(int theFlags, int theX, int theY) override;  // 0x40c170 (mahjongchina), 0x40c1b0 (mahjongjapan)
	void OnChar(int theChar, int theRepeat, int theScan) override;  // 0x40b620 (mahjongchina), 0x40b660 (mahjongjapan)
	bool Update() override;                                        // 0x40bd30 (mahjongchina), 0x40bd70 (mahjongjapan)
	bool OnTimer(int theId, int theIndex) override;                // 0x40dc40 (mahjongchina), 0x40dcc0 (mahjongjapan)
	void OnEnabledChanged() override;                              // 0x40e020 (mahjongchina), 0x40e0e0 (mahjongjapan)

	PlayLayer* GetPlayLayer() { return (PlayLayer*)mLayer; }

	// 0x40a670 (mahjongchina), 0x40a7f0 (mahjongjapan): the tile set (the options' choice, 0-5)
	void SetStyle(int theStyle);
	// 0x40a910 (mahjongchina), 0x40aa40 (mahjongjapan): theSeed -1 is the clock
	void SetLayout(int theLayoutId, int theQuadPercent, uint32_t theSeed);
	// 0x40aab0 (mahjongchina), 0x40abd0 (mahjongjapan)
	void ClearFlights();
	// 0x40ab00 (mahjongchina), 0x40ac20 (mahjongjapan)
	void ClearTexts();
	// 0x40ab90 (mahjongchina), 0x40acb0 (mahjongjapan): an error box when the layout does not load
	void LoadLayout();
	// 0x40ac50 (mahjongchina), 0x40ad70 (mahjongjapan)
	// 0x402580 (mahjongchina), 0x402520 (mahjongjapan) inlined here: the copy of the layout's name (+0x20)
	std::string GetLayoutName() { return mLayout.mName; }
	// 0x40ac80 (mahjongchina), 0x40ada0 (mahjongjapan)
	void ClearHints();
	// 0x40acc0 (mahjongchina), 0x40ade0 (mahjongjapan)
	void Reset();
	// 0x40ad10 (mahjongchina), 0x40ae30 (mahjongjapan): a saved game at theMove
	void Resume(int theMove);
	// 0x40ad40 (mahjongchina), 0x40ae60 (mahjongjapan): the tiles fly in; theDeal deals them (with mDealt too), theKeep
	// keeps the layout's state, theNewSeed takes the clock as the seed
	void Start(bool theDeal, bool theKeep, bool theNewSeed);
	// 0x40ae50 (mahjongchina), 0x40af70 (mahjongjapan): the shuffle (the tiles fly out, 0x40af70 (mahjongchina),
	// 0x40b0a0 (mahjongjapan) deals them again)
	void Shuffle();
	// 0x40af70 (mahjongchina), 0x40b0a0 (mahjongjapan)
	void Reshuffle();
	// 0x40aff0 (mahjongchina), 0x40b120 (mahjongjapan)
	void EndShuffle() { mPhase = PHASE_FLY_IN; }
	// 0x40b2b0 (mahjongchina), 0x40b3d0 (mahjongjapan)
	void DrawFlight(Flight* theFlight);
	// 0x40b610 (mahjongchina), 0x40b650 (mahjongjapan)
	void ApplyMagnetLinks() { mLayout.ApplyMagnetLinks(); }
	// 0x40b650 (mahjongchina), 0x40b690 (mahjongjapan): the tiles' rects and the layout's offset
	void Measure();
	// 0x40b740 (mahjongchina), 0x40b780 (mahjongjapan): where the tiles fly in from
	void SetFlyIn(int theKind);
	// 0x40b8c0 (mahjongchina), 0x40b900 (mahjongjapan): the shuffle's places off the board
	void SetFlyOut();
	// 0x40b960 (mahjongchina), 0x40b9a0 (mahjongjapan)
	void SwapFlyOut();
	// 0x40b9a0 (mahjongchina), 0x40b9e0 (mahjongjapan)
	void DrawTile(Tile* theTile);
	// 0x40bbd0 (mahjongchina), 0x40bc10 (mahjongjapan): a face from the tile set, and the selected face over it
	void DrawFace(int theX, int theY, int theKind, bool theSelected, int theOffset);
	// 0x40c2f0 (mahjongchina), 0x40c330 (mahjongjapan): the clock starts at the first click
	void StartClock();
	// 0x40c310 (mahjongchina), 0x40c350 (mahjongjapan)
	bool CanSelect(Tile* theTile);
	// 0x40c370 (mahjongchina), 0x40c3b0 (mahjongjapan): a special tile selected pairs only with a tile it can see
	bool CanPair(Tile* theTile);
	// 0x40c3c0 (mahjongchina), 0x40c400 (mahjongjapan)
	void StartSelAnim();
	// 0x40c490 (mahjongchina), 0x40c4d0 (mahjongjapan)
	void Take(Tile* theTile, Tile* theOther);
	// 0x40c660 (mahjongchina), 0x40c6a0 (mahjongjapan): the points of theCount tiles like theTile; theBonus adds 500
	// for the kind of the pair before
	void AddPoints(Tile* theTile, int theCount, bool theBonus);
	// 0x40c860 (mahjongchina), 0x40c8b0 (mahjongjapan): a tile flown off at the escape
	void AddEscapePoints(Tile* theTile);
	// 0x40c940 (mahjongchina), 0x40c990 (mahjongjapan)
	void EndTake();
	// 0x40c970 (mahjongchina), 0x40c9c0 (mahjongjapan): the key takes a lock (KIND_KEY with KIND_LOCK)
	void TakeKeyAndLock(Tile* theTile, Tile* theOther);
	// 0x40ca60 (mahjongchina), 0x40cab0 (mahjongjapan), 0x40cb10 (mahjongchina), 0x40cb60 (mahjongjapan),
	// 0x40cbb0 (mahjongchina), 0x40cc00 (mahjongjapan)
	void JoinKeyAndLock();
	void FlyKeyAndLock();
	void EndKeyAndLock();
	// 0x40cbc0 (mahjongchina), 0x40cc10 (mahjongjapan): the joker (KIND_JOKER) takes every free tile of the other's
	// kind
	void TakeJoker(Tile* theTile, Tile* theOther);
	// 0x40cd30 (mahjongchina), 0x40cd80 (mahjongjapan): the magnet (KIND_MAGNET) takes the other's kind
	void TakeMagnet(Tile* theTile, Tile* theOther);
	// 0x40cde0 (mahjongchina), 0x40ce30 (mahjongjapan), 0x40ce80 (mahjongchina), 0x40ced0 (mahjongjapan)
	void FlyMagnet();
	void EndMagnet();
	// 0x40cea0 (mahjongchina), 0x40cef0 (mahjongjapan)
	void FinishFlights();
	// 0x40ced0 (mahjongchina), 0x40cf20 (mahjongjapan)
	void Undo();
	// 0x40d190 (mahjongchina), 0x40d1f0 (mahjongjapan): the tray shows the pair taken at mMove
	void UpdateTray();
	// 0x40d290 (mahjongchina), 0x40d2f0 (mahjongjapan)
	void UndoPair(Tile* theTile, Tile* theOther);
	// 0x40d360 (mahjongchina), 0x40d3c0 (mahjongjapan), 0x40d400 (mahjongchina), 0x40d460 (mahjongjapan),
	// 0x40d4b0 (mahjongchina), 0x40d510 (mahjongjapan)
	void EndUndo();
	void UndoMagnet();
	void EndUndoMagnet();
	// 0x40d500 (mahjongchina), 0x40d560 (mahjongjapan): the free pairs counted, the hints' order, the tips and the gold
	// tiles' music
	void UpdateMoves();
	// 0x40d690 (mahjongchina), 0x40d6e0 (mahjongjapan)
	void ShowHint(bool theAsked);
	// 0x40d810 (mahjongchina), 0x40d870 (mahjongjapan)
	void StartHints();
	// 0x40d850 (mahjongchina), 0x40d8b0 (mahjongjapan): the gold tiles end the round
	void TakeGold(Tile* theTile, Tile* theOther);
	// 0x40d9b0 (mahjongchina), 0x40da30 (mahjongjapan), 0x40d9f0 (mahjongchina), 0x40da70 (mahjongjapan)
	void EndGold();
	void FlyOff();
	// 0x40db80 (mahjongchina), 0x40dc00 (mahjongjapan)
	void EndRound();
	// 0x40dde0 (mahjongchina), 0x40dea0 (mahjongjapan)
	void EndHighlight() { mHighlight = false; }
	// 0x40ddf0 (mahjongchina), 0x40deb0 (mahjongjapan)
	void EndGoldInTray();
	// 0x40de40 (mahjongchina), 0x40df00 (mahjongjapan): Enter or space skips the tiles flying off
	void SkipFlyOff();
	// 0x40ded0 (mahjongchina), 0x40dfa0 (mahjongjapan): the layout to save, or null
	Layout* GetSaveLayout();
	// 0x40df00 (mahjongchina), 0x40dfd0 (mahjongjapan): no free pair
	void CheckStuck();
	// 0x40dfc0 (mahjongchina), 0x40e090 (mahjongjapan)
	void ClearSelection();
	// 0x40e000 (mahjongchina), 0x40e0c0 (mahjongjapan): a tip (OnTimer)
	void ShowTip(int theId) { AddTimer(theId, 100, 1, 1); }
};

} // namespace mahjong

#endif
