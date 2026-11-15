#ifndef __BOOKWORM_REPLAY_H__
#define __BOOKWORM_REPLAY_H__

// port(mod): replays of BookWorm games (.bwr), mod "replays" (games/bookworm/mods/replays, feature "replays":
// { "keep": 50 }; the user's idea, after Big Money's games/bigmoney/src/Replay.h). Not in the original.
//
// Every game played from its start is recorded and written when it reaches its game-over summary, to the replay
// folder (BookWormApp::mReplayDir: native <user dir>/replays, web /bwpersist/replays; -replaydir=). The newest
// "keep" replays stay, plus every one whose game made a high-score table ("-keep" in its name). A game resumed
// from a save, or left for a new one, is not kept.
//
// Watching (BookWormApp::StartWatch): -replay=<file> (the web page's Replays panel passes ?replay=), or R on the
// game-over summary for the game just played. Keys while watching: 1, 2, 3, 4 for 1x, 2x, 4x, 8x, space to pause,
// Esc (or MENU) to stop. A watch never saves, counts a game, writes the word journal or enters a high score, and it
// needs the same gameplay mods as the recording (the same ModManager::GetScoreKey).
//
// Why this is enough to play a game again: from its start, a board is deterministic in its ticks (Board::mUpdateCnt,
// which stands still while the board is paused, and so do Lex and the overlay). Its draws come from gGameRand (the
// letters, fire, special tiles, bonus words), its own mRand (drop heights, Lex's chatter, particles, a shaking
// tile's jitter, which decides when burning tiles are gone) and the C library's rand() (Lex's idle animations,
// which decide when Lex is free, and so when he chatters). So a replay holds:
//   - the start, taken in the board's constructor just before it deals: both generators whole, the letter
//     generator's program-wide memory (Board::GetGlobalState), the app's next bonus word, a fresh seed for rand(),
//     the mode, the daily board's date and the options and one-time hints Lex reads; and at the first tick the
//     overlay's effect generator (made with the overlay, after the constructor: the embers of the tiles in flight,
//     whose end decides when the game-over page comes);
//   - the player's moves at the time they came in: the selection whenever it changed (however it was made: clicks,
//     drags, typing, a hint), a word submitted, a scramble, an undo, a level's Continue, and the mouse coming onto
//     Lex, leaving him or pressing him (while it rests on him his glow rises and his animation and rand() stand
//     still, which decides when he is calm, and so when the game-over page comes). The time is 2 x tick for a
//     move made in a tick (Board::Update after its Widget::Update: an undo or a hint asked for while the tiles
//     moved) and 2 x tick + 1 for one made after it (the input); a watch makes each at the same place;
//   - rand() seeded again where Lex updated other than once between two ticks (the game left for the menu and
//     resumed, say), so the watch, which has no such gaps, stays in step;
//   - the end: its tick, score, level and words, which a watch checks ("replay: verified" in the log).
// What else the mouse does (Lex's hover hints, a balloon held open or closed early) is not recorded: it changes
// only what Lex's balloon shows.
//
// File, little-endian: "BWRP", u16 format = 1, u16 flags = 0, then records { u32 tag, u32 len, payload }:
//   META  UTF-8 JSON for people and tools, not read by the game: date, mode, score, level, words, mods
//   GAME  u8 mode, u8 appFlags (APP_*), u32 crtSeed, u32 dailyDate (0 for none), i32 lastLetterIdx,
//         i32 lastRefillLevel, u16 n + n bytes: the next bonus word, 3 x (624 u32 state words + u32 index): gGameRand,
//         the board's mRand and the overlay's effect generator, u16 n + n bytes: the score key ("" without gameplay mods)
//   EVTS  u32 count, then per event: varint time delta, u8 op, varint argument count, the arguments as zigzag varints
//   END   u32 endTick, i32 score, i32 level (from 0), i32 words
// GAME and END are required; unknown records are skipped. A file over 16 MB or with more than 4M events is refused.

#include "MTRand.h"
#include <cstdint>
#include <string>
#include <vector>

namespace Sexy
{

class Board;
class BookWormApp;

namespace Replay
{

enum
{
	FORMAT = 1,
	MAX_FILE_SIZE = 16 * 1024 * 1024,
	MAX_ITEMS = 4000000,
	MAX_ARGS = 64
};

enum Op
{
	OP_SELECT = 1,      // the selected tiles in order, each row * BOARD_COLS + col (none: cleared)
	OP_SUBMIT = 2,      // x y selectMode: Board::Submit(x, y) with Board::mSelectMode so
	OP_SCRAMBLE = 3,    // Board::Scramble, one that goes ahead (a refused one only shows a dialog)
	OP_UNDO = 4,        // UndoStack::Restore
	OP_NEXT = 5,        // how: a level's summary left, 0 by a board button (Board::ButtonDepress), 1 by Continue
	                    // (LevelUpWidget::Continue)
	OP_CRT = 6,         // seed: CrtSRand(seed), before this tick's updates
	OP_LEX = 7,         // the mouse and Lex: 0 it left him, 1 it is over him, 2 it pressed him (his glow drops)
	NUM_OPS = 8
};

enum AppFlags
{
	APP_SHOW_DEFS = 0x01,
	APP_SHOW_HINTS = 0x02,
	APP_WATCH_OUT_HINT = 0x04,
	APP_FIRE_ALERT_HINT = 0x08,
	APP_GREEN_TILE_HINT = 0x10,
	APP_SCRAMBLE_HINT = 0x20
};

struct Event
{
	uint32_t	mTime = 0;     // 2 x tick, + 1 after the tick
	int			mOp = 0;
	std::vector<int32_t> mArgs;
};

// An MTRand's whole state (MTRand::GetState)
struct RandState
{
	uint32_t	mWords[MTRAND_N];
	int			mIndex = 0;

	void		Get(const MTRand& theRand);
	bool		Set(MTRand* theRand) const;
};

struct File
{
	std::string	mMeta;
	int			mMode = 0;
	uint32_t	mAppFlags = 0;
	uint32_t	mCrtSeed = 0;
	uint32_t	mDailyDate = 0;
	int32_t		mLastLetterIdx = -1;
	int32_t		mLastRefillLevel = -1;
	std::string	mNextBonusWord;
	RandState	mGameRand;
	RandState	mBoardRand;
	RandState	mEffectRand;        // OverlayWidget::mEffectMgr's, at the first tick
	std::string	mScoreKey;
	std::vector<Event> mEvents;
	bool		mHasEnd = false;
	uint32_t	mEndTick = 0;
	int32_t		mScore = 0;
	int32_t		mLevel = 0;
	int32_t		mWords = 0;

	void		Write(std::vector<uint8_t>* theOut) const;
	bool		Read(const std::vector<uint8_t>& theData, std::string* theError);
};

bool		LoadFile(const std::string& thePath, File* theFile, std::string* theError);
bool		SaveFile(const std::string& thePath, const File& theFile);

}

// A game being recorded: Board owns it from its constructor (a new game with the feature on) until its game over
// (Finish), LoadGame (a resumed game) or the board goes
class ReplayRecorder
{
public:
	explicit ReplayRecorder(Board* theBoard);

	void		Start();                    // the constructor, just before SetupBoard: the start, rand() seeded
	// Board::Update: before its tick (the selection, rand() where Lex skipped), in it (after Widget::Update: moves
	// from here are the tick's) and at its end (the selection; moves from here come after the tick)
	void		BeforeTick();
	void		InTick();
	void		AfterTick();
	void		AddOp(int theOp, const std::vector<int32_t>& theArgs = std::vector<int32_t>());   // after the selection
	// The game is over: writes the replay (pinned: its game made a high-score table), prunes the folder, and
	// returns the file's path ("" when it could not be written)
	std::string	Finish(bool thePinned);

	Replay::File mFile;

private:
	void		Flush();                    // the selection and the mouse over Lex, where they changed

	Board*		mBoard;
	std::vector<int32_t> mSelection;        // the selection last recorded
	bool		mLexOver;                   // the mouse over Lex, as last recorded
	unsigned int mLexUpdates;               // gLexCrtUpdates at the last tick
	bool		mTicked;
	bool		mInTick;
};

// The replay a watch's board plays (Board::mPlayer)
class ReplayPlayer
{
public:
	ReplayPlayer(Board* theBoard, const Replay::File& theFile);

	void		Start();                    // the constructor, just before SetupBoard: the recording's start
	// The moves recorded up to now, made as the player made them: Board::Update before its tick (the moves after the
	// last tick) and in it (isInTick, after Widget::Update: the tick's own)
	void		Apply(bool isInTick);
	// The board reached its game-over summary: compares it with the recording's end and logs it
	void		CheckEnd();

	Replay::File mFile;
	size_t		mNextEvent = 0;
	bool		mStarted = false;           // the first tick set the overlay's effect generator
	bool		mLexOver = false;           // the recording's mouse over Lex (Lex::Update reads it while watching)
	bool		mApplying = false;          // a recorded move is being made (the watch's input guards let it by)
	bool		mDesync = false;
	bool		mEnded = false;
	bool		mVerified = false;

private:
	Board*		mBoard;
};

// Lex's updates that drew from rand() (Lex::Update), counted for ReplayRecorder::Tick
extern unsigned int gLexCrtUpdates;

}

#endif
