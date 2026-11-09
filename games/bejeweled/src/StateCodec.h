#ifndef __STATECODEC_H__
#define __STATECODEC_H__

// port(variant): one field table for the board state (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.6). Not
// part of the original game. StateCodec.cpp lists every Board field and every Piece field once, in
// declaration order, with flags saying which encodings take it:
//
//   FULL      everything, the raw 0x78-byte Piece records included, for -tracehash and the oracle. Same
//             platform only (raw doubles from libm). Its bytes are exactly the board part of the stage-0
//             -tracehash FULL hash (testhooks/StateHash.cpp hashes this encoding), so the oracle's hash.txt
//             comparison with the frozen stage-0 build proves the table holds every field of that list.
//   LOGIC     the state later play depends on, portable across native and wasm: the board tick, the state,
//             level, score and stat fields, the multiplier and drain doubles as bits, bar width and colour,
//             the timers play reads, pause counts, input delay, play seconds, the selection and swap
//             pieces as cells, the RNG Board::Rand draws from (seed, draw count, MT words and index), the
//             variant state; per cell present, colour, kind, col, row, the velocities, swap offsets, dying
//             and dying frame, and the position outside states 2, 3 and 7 (where libm's cos/sin set it).
//             Left out: hover glow, spin and sparkle, mHole*, popups, the banner, the hint-glow and
//             move-score alphas, the mouse fields, the tutorial bob, sound throttles. The replay's CHECK and
//             END hash it (FNV-1a 64).
//   SNAPSHOT  a decodable copy of every field FULL hashes that can be restored (not the banner images'
//             presence or the parked orphan count), with explicit layout and RNG presence: the round trip
//             of 6.6 (write, restore into a scratch board, write again), and the base of the v2
//             save chunks (the V2 flag marks their fields).
//   UNDO      an undo snapshot (6.7): the layout, per cell present, colour and kind (in
//             STATE_PLAYING every gem is at rest), the UNDO-flagged fields and the RNG as SNAPSHOT has it.
//             Decoding it into the board it came from is the undo: fresh pieces at their rest positions, the
//             cosmetic fields zero; every other field stays as it is.
//
// Values are little-endian: ints as 4 bytes, bools as 1, doubles as their IEEE-754 bits; pointers to
// pieces as cells (row * cols + col, -1 for none, rows * cols and up for a piece outside the grid: the stale
// selection NewBoard drops, GAME_NOTES 3.6).

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace Sexy
{

class Board;
class MTRand;

class StateCodec
{
public:
	enum Profile
	{
		PROFILE_FULL,
		PROFILE_LOGIC,
		PROFILE_SNAPSHOT,
		PROFILE_UNDO
	};

	enum Flags
	{
		F_FULL = 1,
		F_LOGIC = 2,
		F_SNAP = 4,
		F_V2 = 8,       // stage 5: the v2 save
		F_UNDO = 16,    // stage 6: undo snapshots
		F_DERIVED = 32  // FULL only, cannot be restored (whether an image exists, a container size)
	};

	// theRng: for LOGIC, the RNG to hash instead of the one Board::Rand draws from (a recorder's mirror of
	// the global stream, ReplayRecorder). skipDerived: FULL without the F_DERIVED rows (the round trip).
	// theRows: the offset at which each row starts, with its name (diagnostics).
	static std::string	Encode(Board* theBoard, int theProfile, const MTRand* theRng = NULL, bool skipDerived = false,
							std::vector<std::pair<size_t, const char*> >* theRows = NULL);

	// A SNAPSHOT (or with theProfile PROFILE_UNDO an UNDO encoding) into theBoard, whose layout must match.
	// Replaces its pieces and its own RNG. False, with theError set and theBoard possibly half written, for
	// bytes that are not a whole snapshot.
	static bool			Decode(Board* theBoard, const std::string& theBytes, std::string* theError, int theProfile = PROFILE_SNAPSHOT);

	static uint64_t		Fnv1a(const std::string& theBytes);
	static uint64_t		LogicHash(Board* theBoard, const MTRand* theRng = NULL);

	// The round trip of 6.6: SNAPSHOT of theBoard restored into theScratch (a Board made with SCRATCH_BOARD,
	// given theBoard's rules and layout here), then written again: the bytes, the FULL encoding without its
	// derived rows and the LOGIC hash must all match. "" when they do, else what differs.
	static std::string	RoundTrip(Board* theBoard, Board* theScratch);

	// Stage 6: theBoard's UNDO encoding decoded into theScratch (a copy of it: RoundTrip just passed) and written
	// again: the same bytes. "" when they are, else what differs.
	static std::string	UndoRoundTrip(Board* theBoard, Board* theScratch);
};

}

#endif
