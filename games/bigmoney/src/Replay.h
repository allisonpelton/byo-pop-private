#ifndef __BIGMONEY_REPLAY_H__
#define __BIGMONEY_REPLAY_H__

// port(mods): replays of Big Money games (.bmr). Not in the original.
//
// With the "replays" feature on (ModRules.h: "features": { "replays": { "keep": 50 } }), every game played
// from its start is recorded, and written when it reaches its game-over screen, to the replay folder
// (WinBMApp::mReplayDir: native <user dir>/replays, web /bmpersist/replays; -replaydir=). The newest "keep"
// replays stay, plus every one whose score made a local high-score table ("-keep" in its name). A game that
// is abandoned (New Game, quitting) or resumed from a save is not kept.
//
// Watching (WinBMApp::StartWatch): -replay=<file> (the web page's Replays panel passes ?replay=), or R on
// the game-over screen for the game just played. Keys while watching: 1, 2, 3, 4 for 1x, 2x, 4x, 8x, space
// to pause, Escape (or the New Game button) to stop. A watch never saves, counts a game or enters a high
// score, and it needs the same gameplay mods as the recording (the same ModManager::GetScoreKey).
//
// Why this is enough to play a game again: the board is deterministic from its start. Its generator
// (Board::mRand) is seeded once, in the constructor, and everything a board tick does follows from the
// board's state and the player's input. So a replay holds:
//   - the start: the game type and skill, the generator's seed, the seed the player chose (if any), and the
//     app's five one-time flags the board reads (its hints; mBagCollectedOnce also moves money bags);
//   - the player's input at the board tick it came in (Board::mUpdateCnt, which does not advance while the
//     board is paused or frozen, so pauses need no record): clicks on coins, mouse moves (the hover
//     highlight, whose flood fill shares the coins' marks with the game) and the "play next level" button;
//   - the bag columns of each level, which an unseeded game draws from the CRT rand() (Board::BagColumnRand)
//     that other parts of the game share;
//   - the end: its tick, score, level and coins cleared, which a watch checks.
// Effects that draw from rand() elsewhere (sparkles, flying bills, Mr. Money's idle moves) can differ in a
// watch; nothing in play depends on them.
//
// File, little-endian: "BMRP", u16 format = 1, u16 flags = 0, then records { u32 tag, u32 len, payload }:
//   META  UTF-8 JSON for people and tools, not read by the game: date, mode, skill, score, level, seed, mods
//   GAME  u8 gameType, u8 skill, u8 userSeeded, u8 appFlags (APP_*), u32 rngSeed (the seed of Board::mRand,
//         1.2 seeding), u32 gameSeed, u16 n + n bytes: the score key ("" without gameplay mods)
//   EVTS  u32 count, then per event: varint tick delta, u8 op, the op's arguments as zigzag varints
//   BAGS  u32 count, u16 each: the bag-column draws in order
//   END   u32 endTick, i32 score, i32 level (from 0), i32 coinsCleared
// GAME and END are required; unknown records are skipped. A file over 16 MB or with more than 4M events or
// bag draws is refused.

#include <cstdint>
#include <string>
#include <vector>

namespace Sexy
{

class Board;
class WinBMApp;

namespace Replay
{

enum
{
	FORMAT = 1,
	MAX_FILE_SIZE = 16 * 1024 * 1024,
	MAX_ITEMS = 4000000
};

enum Op
{
	OP_CLICK = 1,   // x y: Board::MouseDown while playing
	OP_MOVE = 2,    // x y: Board::MouseMove
	OP_LEAVE = 3,   // Board::MouseLeave
	OP_NEXT = 4,    // the "play next level" button (Board::ButtonDepress)
	OP_HINT = 5,    // H: a Puzzle solver hint (Board::KeyChar; PuzzleSolver.h). A click on the solver's status
	                // line is an OP_CLICK.
	OP_UNDO = 6,    // U: takes back the last click (the Puzzle solver's undo; Board::KeyChar). A click on the
	                // solver's undo line is an OP_CLICK.
	OP_POCKET = 7,  // x y: a Pickpocket lifts the small group there (Board::MouseDown, UsePickpocket): a right
	                // click on it, or a click while one is armed. Arming one (P, or a click on its counter) is
	                // not recorded.
	NUM_OPS = 8
};

enum AppFlags
{
	APP_SHOW_INSTRUCTOR = 0x01,
	APP_HAS_CLICKED_GROUP = 0x02,
	APP_DANGER_HINT_SHOWN = 0x04,
	APP_BAG_COLLECTED_ONCE = 0x08,
	APP_BOMB_SEEN_ONCE = 0x10
};

int			OpArgCount(int theOp);      // -1 for an unknown op

struct Event
{
	uint32_t	mTick = 0;
	int			mOp = 0;
	int32_t		mX = 0;
	int32_t		mY = 0;
};

struct File
{
	std::string	mMeta;
	int			mGameType = 0;
	int			mSkill = 1;
	bool		mUserSeeded = false;
	uint32_t	mAppFlags = 0;
	uint32_t	mRngSeed = 0;
	uint32_t	mGameSeed = 0;
	std::string	mScoreKey;
	std::vector<Event> mEvents;
	std::vector<uint16_t> mBagColumns;
	bool		mHasEnd = false;
	uint32_t	mEndTick = 0;
	int32_t		mScore = 0;
	int32_t		mLevel = 0;
	int32_t		mCoins = 0;

	void		Write(std::vector<uint8_t>* theOut) const;
	bool		Read(const std::vector<uint8_t>& theData, std::string* theError);
};

bool		LoadFile(const std::string& thePath, File* theFile, std::string* theError);
bool		SaveFile(const std::string& thePath, const File& theFile);

}

// A game being recorded: Board owns it from its constructor (a new game with the feature on) until the game
// is over (Finish) or the board goes
class ReplayRecorder
{
public:
	ReplayRecorder(WinBMApp* theApp, uint32_t theRngSeed, bool theUserSeeded, uint32_t theGameSeed);

	// Input at theTick; a mouse move right after another at the same tick replaces it (only the last
	// position of a run of moves counts for the hover)
	void		AddEvent(uint32_t theTick, int theOp, int theX = 0, int theY = 0);
	void		AddBagColumn(int theValue);
	// The game is over: writes the replay (pinned: its score made a local table), prunes the folder, and
	// returns the file's path ("" when it could not be written)
	std::string	Finish(Board* theBoard, bool thePinned);

	Replay::File mFile;

private:
	WinBMApp*	mApp;
};

// The replay a playback board plays (Board::mPlayer)
class ReplayPlayer
{
public:
	explicit ReplayPlayer(const Replay::File& theFile);

	// The input recorded at theTick, given to theBoard as the player's was (Board::Update, before the tick)
	void		Apply(Board* theBoard, uint32_t theTick);
	// The next bag-column draw (Board::BagColumnRand); a missing one is a desync
	int			NextBagColumn(uint32_t theTick);
	// The board reached its game-over screen at theTick: compares it with the recording's end and logs it
	void		CheckEnd(Board* theBoard, uint32_t theTick);

	Replay::File mFile;
	size_t		mNextEvent = 0;
	size_t		mNextBag = 0;
	bool		mDesync = false;
	bool		mEnded = false;
	bool		mVerified = false;
};

}

#endif
