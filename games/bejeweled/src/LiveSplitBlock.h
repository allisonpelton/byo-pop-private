#ifndef __LIVESPLITBLOCK_H__
#define __LIVESPLITBLOCK_H__

// port(variant): the LiveSplit block, layout 1 (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.15). A tagged,
// read-only block in the game's .data that the published auto splitter
// (games/bejeweled/tools/livesplit/BejeweledDeluxe-byopop.asl) finds and reads from outside the process.
// Shared by the game's observer (LiveSplitObserver.cpp, i686) and games/bejeweled/tools/livesplit/lsprobe
// (x86_64, reads the 32-bit game as LiveSplit does), so it uses fixed-width types only and no pointers.
//
// Little-endian, naturally aligned, 0xB0 bytes:
//   0x00 u8  magic[20]      "BYOPOP-LIVESPLIT" F0 F1 F2 F3
//   0x14 u32 gameId         0x58444A42: the bytes "BJDX" in memory
//   0x18 u16 layoutVersion  1;   0x1A u16 size 0xB0
//   0x1C u32 self           the block's own address once active, else 0
//   0x20 u32 seq            odd while the game writes
//   0x24 u32 active         1 while features.livesplit is on;   0x28 u32 updates (heartbeat)
//   0x2C u32 flags          LIVESPLIT_FLAG_*
//   0x30 u32 boardSerial    the current board, 0 = none
//   0x34 u32 goCount, goSerial;  0x3C levelClears, levelClearSerial;  0x44 gameOvers, gameOverSerial
//   0x4C u32 deathsNoMoves, deathsTimeUp, deathsNewGame, deathsOther   (they add up to gameOvers)
//   0x5C i32 tick, state, level, displayLevel, displayScore, score, boardLevelClears, goTick (-1 before GO)
//   0x7C i32 displayScoreMax  the board's highest displayScore up to its game-over update, then frozen
//   0x80 u8  rulesId[32];  0xA0 u64 qpc (SDL_GetPerformanceCounter at this write);  0xA8 u64 qpcFrequency
//
// Append-only: fields are never moved, resized or given a new meaning. An append bumps layoutVersion and
// size; a reader for layout N accepts layoutVersion >= N and size >= its size and reads only its bytes. An
// incompatible change gets a new magic.
//
// Writing (the game): seq++ (odd), release fence, the fields, release fence, seq++ (even). Reading (the
// script, lsprobe): seq -> s1, the 0xB0 bytes, seq -> s2; the copy counts when s1 == s2, s1 is even and
// the copy's own seq is s1 (x86 keeps the order of stores and of loads).

#include <cstddef>
#include <cstdint>
#include <cstdio>

// The magic as a brace list of bytes: the block's definition is the only place in the game that spells
// it, so it occurs exactly once in the exe (games/bejeweled/tools/livesplit/check_asl_offsets.py checks
// it). Not a string literal: that would put a second copy in .rdata.
#define LIVESPLIT_MAGIC_BYTES { 0x42, 0x59, 0x4F, 0x50, 0x4F, 0x50, 0x2D, 0x4C, 0x49, 0x56, 0x45, 0x53, 0x50, 0x4C, 0x49, 0x54, \
	0xF0, 0xF1, 0xF2, 0xF3 }

enum : uint32_t
{
	LIVESPLIT_MAGIC_SIZE = 20,
	LIVESPLIT_GAME_ID = 0x58444A42u,    // "BJDX" in memory (a multi-character constant would be 0x424A4458)
	LIVESPLIT_LAYOUT_VERSION = 1,
	LIVESPLIT_SIZE_V1 = 0xB0,

	LIVESPLIT_FLAG_TIMED = 0x01,        // a Timetrial board
	LIVESPLIT_FLAG_RESUMED = 0x02,      // the board resumed a saved game (its GO is not counted)
	LIVESPLIT_FLAG_VANILLA = 0x04,      // no score key (no gameplay mod) and no seed the player chose
	LIVESPLIT_FLAG_CHOSEN_SEED = 0x08,  // a seed the player chose (-gameseed, ?seed=)
	LIVESPLIT_FLAG_OWN_RNG = 0x10,      // the board owns an MTRand
	LIVESPLIT_FLAG_REPLAY = 0x20,       // a replay is playing (stage 3; never set yet)
	LIVESPLIT_FLAG_GAME_OVER = 0x40,    // the board had its game over (latched per board)
	LIVESPLIT_FLAG_PAUSED = 0x80        // the board is paused (any pause: dialog, focus, the player)
};

struct LiveSplitBlockV1
{
	uint8_t		magic[LIVESPLIT_MAGIC_SIZE];
	uint32_t	gameId;
	uint16_t	layoutVersion;
	uint16_t	size;
	uint32_t	self;
	uint32_t	seq;
	uint32_t	active;
	uint32_t	updates;
	uint32_t	flags;
	uint32_t	boardSerial;
	uint32_t	goCount;
	uint32_t	goSerial;
	uint32_t	levelClears;
	uint32_t	levelClearSerial;
	uint32_t	gameOvers;
	uint32_t	gameOverSerial;
	uint32_t	deathsNoMoves;
	uint32_t	deathsTimeUp;
	uint32_t	deathsNewGame;
	uint32_t	deathsOther;
	int32_t		tick;
	int32_t		state;
	int32_t		level;
	int32_t		displayLevel;
	int32_t		displayScore;
	int32_t		score;
	int32_t		boardLevelClears;
	int32_t		goTick;
	int32_t		displayScoreMax;
	uint8_t		rulesId[32];
	uint64_t	qpc;
	uint64_t	qpcFrequency;
};

static_assert(offsetof(LiveSplitBlockV1, magic) == 0x00, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, gameId) == 0x14, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, layoutVersion) == 0x18, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, size) == 0x1A, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, self) == 0x1C, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, seq) == 0x20, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, active) == 0x24, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, updates) == 0x28, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, flags) == 0x2C, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, boardSerial) == 0x30, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, goCount) == 0x34, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, goSerial) == 0x38, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, levelClears) == 0x3C, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, levelClearSerial) == 0x40, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, gameOvers) == 0x44, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, gameOverSerial) == 0x48, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, deathsNoMoves) == 0x4C, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, deathsTimeUp) == 0x50, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, deathsNewGame) == 0x54, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, deathsOther) == 0x58, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, tick) == 0x5C, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, state) == 0x60, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, level) == 0x64, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, displayLevel) == 0x68, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, displayScore) == 0x6C, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, score) == 0x70, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, boardLevelClears) == 0x74, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, goTick) == 0x78, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, displayScoreMax) == 0x7C, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, rulesId) == 0x80, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, qpc) == 0xA0, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, qpcFrequency) == 0xA8, "LiveSplit layout 1");
static_assert(sizeof(LiveSplitBlockV1) == LIVESPLIT_SIZE_V1, "LiveSplit layout 1");

// One block as a line of "key=value" fields in block order, for the game's -livesplitlog and lsprobe's
// log (games/bejeweled/tools/livesplit/check_lslog.py compares the two). The header (magic, gameId,
// layoutVersion, size) is checked by whoever reads the block and not repeated. maskVolatile prints "*"
// for self, qpc and qpcFrequency (the address changes with ASLR, the clock with the machine), so the
// game's log is deterministic.
inline int FormatLiveSplitBlock(const LiveSplitBlockV1& b, bool maskVolatile, char* theBuffer, size_t theSize)
{
	char aRules[65];
	for (int i = 0; i < 32; i++)
		snprintf(aRules + 2 * i, 3, "%02x", (unsigned) b.rulesId[i]);
	char aSelf[16];
	char aQpc[24];
	char aQpcFrequency[24];
	if (maskVolatile)
	{
		snprintf(aSelf, sizeof(aSelf), "*");
		snprintf(aQpc, sizeof(aQpc), "*");
		snprintf(aQpcFrequency, sizeof(aQpcFrequency), "*");
	}
	else
	{
		snprintf(aSelf, sizeof(aSelf), "%08x", (unsigned) b.self);
		snprintf(aQpc, sizeof(aQpc), "%llu", (unsigned long long) b.qpc);
		snprintf(aQpcFrequency, sizeof(aQpcFrequency), "%llu", (unsigned long long) b.qpcFrequency);
	}
	return snprintf(theBuffer, theSize,
		"seq=%u active=%u updates=%u flags=%02x board=%u go=%u/%u lc=%u/%u over=%u/%u deaths=%u,%u,%u,%u "
		"tick=%d state=%d level=%d dlevel=%d dscore=%d score=%d blc=%d gotick=%d dsmax=%d rules=%s self=%s qpc=%s qpcf=%s",
		(unsigned) b.seq, (unsigned) b.active, (unsigned) b.updates, (unsigned) b.flags, (unsigned) b.boardSerial,
		(unsigned) b.goCount, (unsigned) b.goSerial, (unsigned) b.levelClears, (unsigned) b.levelClearSerial,
		(unsigned) b.gameOvers, (unsigned) b.gameOverSerial, (unsigned) b.deathsNoMoves, (unsigned) b.deathsTimeUp,
		(unsigned) b.deathsNewGame, (unsigned) b.deathsOther, (int) b.tick, (int) b.state, (int) b.level, (int) b.displayLevel,
		(int) b.displayScore, (int) b.score, (int) b.boardLevelClears, (int) b.goTick, (int) b.displayScoreMax, aRules, aSelf,
		aQpc, aQpcFrequency);
}

#endif
