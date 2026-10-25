#ifndef __REPLAY_H__
#define __REPLAY_H__

// port(variant): the replay file, .bjr (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.4), and the sidecar of
// a save in progress (7.3), which is the same format. Not part of the original game. Framework-free, so the
// game, the unit tests (games/bejeweled/tools/rulestest) and later the headless verifier share it.
//
// Everything is little-endian:
//   "BJRP"  u16 format = 1  u16 flags (bit0 hover, bit1 ranked, bit2 multi-segment)  u32 engineAbi
//   records { u32 tag (4 ASCII bytes in file order), u32 len, payload }:
//     RULE  GameRules::Canonical() bytes (the tunables and the 12 diff.txt values included)
//     RLID  32 bytes: SHA-256 of RULE
//     META  UTF-8 JSON, not verified: name, date, mods ["id@version"], build, platform, title
//     SESS  ranked session token (ranked games only)
//     LDSV  the save bytes the LOAD of the next SEGM reads, when the replay does not hold the segment
//           that wrote them (a resume without a matching sidecar: such a replay can be watched, not ranked)
//     RNGS  the RNG the LOAD of the next SEGM leaves the board on: u32 seed (0: the global stream),
//           u8 userSeeded, u8 ranked, 624 x u32 mt, i32 mti. In a sidecar it is the last record before
//           SVHA: the RNG the save was made with, which the resume restores
//     SEGM  one per board lifetime: u8 mode (0 normal, 1 timed), RNG start (u8 kind: 0 = u32 seed |
//           1 = 624 x u32 mt + i32 mti, the global stream at the constructor), u8 userSeeded,
//           ENV (u8 showInstructions, i32 timesPlayed, i32 pointsAscent), u32 count + events
//           { varint tickDelta, u8 op, zigzag-varint args (as many as the op takes) }
//     SVHA  sidecar only: u32, the first 4 bytes (little-endian) of SHA-256 of the save it goes with
//     END   u8 reason (0 abandoned, 1 no moves, 2 time up, 3 new game, 4 gold time up, 5 saved), u32 endTick,
//           i32 score, i32 level, i32 gems, u64 LOGIC hash, u32 board draws. Always the last record.
// The records come in this order, each at most once where it stands:
//   [RULE] [RLID] [META] [SESS], then per segment [LDSV] [RNGS] SEGM, then [RNGS] [SVHA] END
// Unknown tags are skipped wherever they are. RNGS and LDSV belong to the SEGM that follows them; an RNGS
// after the last SEGM is the sidecar's (File::mTrailingRng). Flag bit0 (hover): every segment has the hover
// channel (a game resumed under the other features.replays.hover setting has it off).

#include <cstdint>
#include <string>
#include <vector>

namespace Sexy
{

namespace Replay
{

enum
{
	FORMAT = 1,
	// Bumped whenever variant simulation output changes (6.4). 2: stage 5, kept invalid moves, the noMoves rule,
	// the v2 save and the variant boards' MOVE fields in the LOGIC hash. The original rules are frozen, so a
	// replay of them from an earlier engine still plays (CheckPlayable). games/bejeweled/tools/bjr.py repeats both
	// values (the verifier's tests check that they agree)
	ENGINE_ABI = 2,
	FIRST_ENGINE_ABI = 1,
	MT_WORDS = 624,
	RULES_ID_SIZE = 32,
	MAX_ARGS = 5,
	// What a reader takes on: Read refuses a larger file, or more events in all, before it allocates for them
	// (a long game with the hover channel at one event per tick is about 2M events and 13 MB in 6 hours), and
	// more segments than MAX_SEGMENTS before it makes the next one (each holds two MT states, 5 KB, however
	// small its record: a game gets one per resume, and the recorder starts a fresh replay before this many)
	MAX_FILE_SIZE = 16 * 1024 * 1024,
	MAX_EVENTS = 4000000,
	MAX_SEGMENTS = 1000
};

enum Flags
{
	FLAG_HOVER = 1,
	FLAG_RANKED = 2,
	FLAG_MULTI_SEGMENT = 4
};

// The commands and markers of a segment (6.3), with the number of arguments each takes
enum Op
{
	OP_SELECT = 1,      // c r
	OP_DESELECT = 2,
	OP_TARGET = 3,      // c r (-1 -1: no cell)
	OP_TWIST = 4,       // c r dir
	OP_SLIDE = 5,       // axis line start k off
	OP_HINT = 6,
	OP_PAUSE = 7,       // u
	OP_UNPAUSE = 8,     // u
	OP_UNDO = 9,
	OP_NEW_GAME = 10,
	OP_XRAND = 11,      // n
	OP_HOVER = 12,      // x y (-1 -1: left)
	OP_SAVE = 13,       // kind (SAVE_GAME, SAVE_SNAPSHOT), u32 save hash
	OP_LOAD = 14,
	OP_CHECK = 15,      // u64 LOGIC hash (as the int64 with the same bits)
	NUM_OPS = 16
};

enum EndReason
{
	END_ABANDONED = 0,
	END_NO_MOVES = 1,
	END_TIME_UP = 2,
	END_NEW_GAME = 3,
	END_GOLD_TIME_UP = 4,
	END_SAVED = 5
};

enum RngKind
{
	RNG_SEED = 0,
	RNG_STATE = 1
};

enum SaveKind
{
	SAVE_GAME = 1,      // Board::SaveGame
	SAVE_SNAPSHOT = 2   // Board::SaveGameSnapshot (the web backup, the "save" script command)
};

int					OpArgCount(int theOp);      // -1 for an unknown op
const char*			OpName(int theOp);          // "SELECT", ...; "?" for an unknown op
uint32_t			Tag(const char* theName);   // the 4 bytes of theName as a little-endian u32
std::string			TagName(uint32_t theTag);

struct Event
{
	uint32_t		mTick = 0;      // board tick (Board::mUpdateCnt) after the update it follows
	int				mOp = 0;
	int64_t			mArgs[MAX_ARGS] = {0, 0, 0, 0, 0};
};

// An RNG as RNGS and the v2 save's chunk store it (mSeed 0: the global stream)
struct RngState
{
	uint32_t		mSeed = 0;
	bool			mUserSeeded = false;
	bool			mRanked = false;
	uint32_t		mMt[MT_WORDS] = {};
	int32_t			mMti = 0;
};

// What a board's RNG started from (the SEGM's RNG start)
struct RngStart
{
	int				mKind = RNG_SEED;
	uint32_t		mSeed = 0;                  // RNG_SEED
	uint32_t		mMt[MT_WORDS] = {};         // RNG_STATE
	int32_t			mMti = 0;
};

struct Segment
{
	int				mMode = 0;                  // 0 normal, 1 timed: the board's mode at its constructor
	RngStart		mRng;
	bool			mUserSeeded = false;
	bool			mShowInstructions = false;  // ENV: WinDMApp's values the board's tick-150 block reads (the
	int32_t			mTimesPlayed = 0;           // instructions or GO), as at the start of that update
	int32_t			mPointsAscent = 0;
	std::vector<Event> mEvents;
	bool			mHasLoadRng = false;        // the RNGS before it
	RngState		mLoadRng;
	bool			mHasLoadSave = false;       // the LDSV before it
	std::string		mLoadSave;
};

struct EndInfo
{
	int				mReason = END_ABANDONED;
	uint32_t		mEndTick = 0;
	int32_t			mScore = 0;
	int32_t			mLevel = 0;
	int32_t			mGems = 0;
	uint64_t		mLogicHash = 0;
	uint32_t		mBoardDraws = 0;
};

struct File
{
	int				mFormat = FORMAT;
	int				mFlags = 0;
	uint32_t		mEngineAbi = ENGINE_ABI;
	bool			mHasRules = false;
	std::string		mRules;                     // RULE
	bool			mHasRulesId = false;
	uint8_t			mRulesId[RULES_ID_SIZE] = {};
	bool			mHasMeta = false;
	std::string		mMeta;
	bool			mHasSession = false;
	std::string		mSession;
	std::vector<Segment> mSegments;
	bool			mHasTrailingRng = false;    // an RNGS after the last SEGM (a sidecar's)
	RngState		mTrailingRng;
	bool			mHasSaveHash = false;       // SVHA
	uint32_t		mSaveHash = 0;
	EndInfo			mEnd;                       // END (always written; Read fails without it)
};

// The bytes of theFile in the record order above
std::string			Write(const File& theFile);

// Parses a whole file. False, with theError set, for anything that is not exactly a well-formed file:
// more than MAX_FILE_SIZE bytes, a wrong magic or format, a record running past the end, a record out of
// the order above or repeated, a payload of the wrong size, an unknown op, a varint over 10 bytes, an event
// count the payload cannot hold or more than MAX_EVENTS events in all, more than MAX_EVENTS_PER_TICK events
// at one tick of a segment (as they are parsed: CheckPlayable's bound, so a file that asks for more stops
// early), more than MAX_SEGMENTS segments, an END reason other than 0..5, no END or data after it. Never reads
// outside theBytes; it allocates for a segment's events once its payload has been found to hold that many
// (sizeof(Event), 48 bytes, per 2 bytes of the file at most: 192 MB for MAX_EVENTS) and for each segment
// as it comes (5 KB each, MAX_SEGMENTS at most).
bool				Read(const std::string& theBytes, File* theFile, std::string* theError = NULL);

// Building blocks, exposed for the tests
void				PutVarint(std::string* theOut, uint64_t theValue);
bool				GetVarint(const std::string& theBytes, size_t* thePos, size_t theEnd, uint64_t* theValue);
uint64_t			ZigZag(int64_t theValue);
int64_t				UnZigZag(uint64_t theValue);

// The u32 a sidecar and a SAVE event keep of a save: the first 4 bytes of its SHA-256, little-endian
uint32_t			SaveHash(const void* theData, size_t theSize);

// Number of TARGET events that name a cell (attempted moves) in theSegment
int					CountMoves(const Segment& theSegment);

// Whether the replay player of this build (WatchController, games/bejeweled/docs/CODE_MODS_DESIGN.md 6.8)
// can start theFile on a theCols x theRows board: this engine's ABI (any from FIRST_ENGINE_ABI for a replay of
// the original rules, isOriginalRules: their simulation never changes) and a RULE record; one segment at least;
// every segment in mode 0 or 1, from a seed other than 0 or a global-stream state, user-seeded only with a
// seed; only the commands this build's game makes (UNDO since stage 6; no TWIST or SLIDE yet), their arguments in
// range (cells of the board, TARGET -1 -1, pause flags 0/1, 1..MAX_XRAND outside draws, save kinds, int
// positions); in every segment after the first, and only with its RNGS (and in the first only with its
// LDSV), one LOAD at tick 0, whose bytes are the segment's LDSV (a save CheckSave takes) or the SAVE that
// ends the segment before; every segment another follows ending in a SAVE; SAVEs only at the segment's last
// tick (the game saves as its board goes away: Shutdown and ~Board, a snapshot sidecar's SAVE after them), at
// most MAX_SAVES_PER_SEGMENT of them; no event after the END tick; no CHECK at tick 0. Bounds on the work it
// can ask for: at most MAX_TICKS board ticks in all (each segment up to its last event, the last one up to
// END), at most MAX_EVENTS_PER_TICK events at one tick of a segment, at most MAX_XRAND outside draws in the
// whole file, SAVEs (a save made and hashed, 40 us each) as above. The
// RNG states themselves (MTRand refuses some) and everything that depends on play are checked while it
// plays. False with theError set.
enum
{
	MAX_XRAND = 1000000,            // outside draws, in one XRAND and in the whole file (real games: a few)
	MAX_TICKS = 8640000,            // 24 hours of board ticks in all segments (the leaderboard's cap is 6 hours, 6.10)
	MAX_EVENTS_PER_TICK = 10000,    // events stamped with one tick (a paused board's tick holds them all)
	MAX_SAVES_PER_SEGMENT = 4       // the game makes 3 at most: Shutdown's, ~Board's, a snapshot sidecar's
};
bool				CheckPlayable(const File& theFile, int theCols, int theRows, std::string* theError = NULL,
						bool isOriginalRules = false);

// Whether theSave is a save the original rules' LoadGame (Board.cpp, 0x4cca98) can take from an LDSV record
// without trusting it: version 1, exactly the length its present cells make on a theCols x theRows board,
// and every value inside what a game in progress can have, where the board draws, indexes or loops with it
// (piece colour 0..6 and the cell it is in, the animation frames, bools 0 or 1, finite doubles, the state,
// level, bar, vortex and wipe values). LoadGame takes any savegame.dat as the original does; a replay's
// bytes come from someone else. With theRulesId (the replay's RLID) a v2 save of those rules passes too
// (stage 5, below): its header names them and the board's shape (a full theCols x theRows board, level shape
// 0), the same fields follow, then its chunks (ReadVariantSave; the MOVE counts not below 0). Which of the two
// a board takes is LoadGame's to say: a variant board only v2, any other only v1. False with theError set.
bool				CheckSave(const std::string& theSave, int theCols, int theRows, std::string* theError = NULL,
						const uint8_t* theRulesId = NULL);

// The v2 save of a variant board (games/bejeweled/docs/CODE_MODS_DESIGN.md 7.2; Board::WriteSaveGame and
// Board::LoadGame, the board's part in BoardSave.cpp), little-endian:
//   u32 version 0x101, u8 rulesId[32], u16 cols, u16 rows, i32 levelShape, u8 mask[(cols * rows + 7) / 8] (bit i,
//   least significant first in each byte: cell (i % cols, i / cols) is playable), then the v1 field sequence
//   from mTimedMode on (Board.cpp), its cells over rows x cols (a hole absent), then chunks { u32 tag (4 ASCII
//   bytes in file order), u32 len, data } up to END (len 0), which ends the file; a reader skips unknown tags:
//     RNGS  the RNG the game goes on with, as the replay's RNGS record (u32 seed, 0: the global stream; u8
//           userSeeded, u8 ranked, 624 x u32 mt, i32 mti)
//     MOVE  i32 moves, i32 invalidMoves, i32 undosUsed
//   Later stages add KIND, GOLD, GTIM (7.2), DIRT and GRAV.
// A v1 save (version 1) has the v1 field sequence right after its version.
enum
{
	SAVE_VERSION = 1,
	SAVE_VERSION_VARIANT = 0x101,
	SAVE_HEAD_SIZE = 9,         // the v1 fields before the cells: mTimedMode, mLevel, mDisplayLevel
	SAVE_TAIL_SIZE = 116        // the v1 fields after the cells, mState to mLevelTextShown
};

struct VariantSave
{
	uint8_t			mRulesId[RULES_ID_SIZE] = {};
	int				mCols = 0;
	int				mRows = 0;
	int32_t			mLevelShape = 0;            // index into the rules' board.levels (0 without levels)
	std::vector<bool> mPlayable;                // mCols * mRows, row-major: a cell, not a hole
	RngState		mRng;                       // RNGS
	int32_t			mMoves = 0;                 // MOVE
	int32_t			mInvalidMoves = 0;
	int32_t			mUndosUsed = 0;
	size_t			mBodyStart = 0;             // ReadVariantSave: where the v1 field sequence starts (mTimedMode)
	size_t			mBodyEnd = 0;               // and where the chunks start
};

// The bytes after the version up to the v1 field sequence, and the chunks after it (RNGS, MOVE, END)
std::string			VariantSaveHeader(const VariantSave& theSave);
std::string			VariantSaveChunks(const VariantSave& theSave);

// Splits a whole v2 save into its header, its v1 field sequence (walked by the present bytes: the cells it
// holds and where it ends) and its chunks. False with theError set for anything else: not version 0x101, a
// size outside 3..16, a hole with a gem, a part cut short, a known chunk of the wrong size or twice, no RNGS
// or no MOVE, no END, or data after it. The values themselves are not checked (CheckSave does).
bool				ReadVariantSave(const std::string& theSave, VariantSave* theOut, std::string* theError = NULL);

// Whether a save (v1 or v2) is of a timed game: its mTimedMode byte. False when it is neither.
bool				SaveIsTimed(const std::string& theSave, bool* isTimed);

}

}

#endif
