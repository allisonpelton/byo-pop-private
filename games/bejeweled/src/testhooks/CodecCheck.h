#ifndef __CODECCHECK_H__
#define __CODECCHECK_H__

// port(variant): test hook for -codeccheck=<file> (see WinDMApp.h). Not part of the original game.
//
// The standing test of games/bejeweled/docs/CODE_MODS_DESIGN.md 6.6: after every update with a board,
// StateCodec writes the board's SNAPSHOT, restores it into a scratch board and writes it again; the bytes,
// the FULL encoding (without the rows that cannot be restored: whether banner images exist, the parked
// orphan count) and the LOGIC hash must all be the same; and while no game is over, the save the board
// would make (SaveGameSnapshot) must pass the check a replay's LDSV bytes get (Replay::CheckSave), so that
// check never refuses a save the game makes. At the first settle it also round-trips a copy of the board
// given what play reaches only rarely (CheckSynthetic). In STATE_PLAYING on a board with undo, the
// UNDO encoding restores into the copy and reads back the same (StateCodec::UndoRoundTrip). The file gets
// one line per settle (the TRACE settles: a new board, the state entering STATE_PLAYING, an undo; none of a
// watched replay's last board once it has ended), one per failure, and a summary when the app ends:
//   settle <update> tick <board tick> state <state>: ok, <bytes> bytes
//   synthetic: ok, <bytes> bytes
//   FAIL update <update> tick <board tick> state <state>: <what differs>      (or FAIL synthetic: ...)
//   codeccheck: <n> updates checked, <settles> settles, <failures> failures
// It only reads the live board: the scratch board is a separate object no widget manager knows, made once
// and kept (deleting it would close the live board's instructions dialog, ~Board).

#include <cstdio>
#include <string>

namespace Sexy
{

class WinDMApp;
class Board;

class CodecCheck
{
public:
	CodecCheck(WinDMApp* theApp, const std::string& theFileName);
	~CodecCheck();

	void			AfterUpdate();

private:
	WinDMApp*		mApp;
	FILE*			mFile;
	Board*			mScratch;
	Board*			mSynthetic;     // a second scratch board, for CheckSynthetic
	Board*			mLastBoard;
	int				mLastState;
	int				mLastUndos;     // the board's undos at the last update: an undo is a settle too (BoardTracer)
	int				mChecked;
	int				mSettles;
	int				mFailures;

	void			CheckSynthetic(Board* theBoard);
};

}

#endif
