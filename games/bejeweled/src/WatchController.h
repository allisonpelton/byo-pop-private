#ifndef __WATCHCONTROLLER_H__
#define __WATCHCONTROLLER_H__

// port(variant): plays a replay in the game (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.11, 6.8). Not part
// of the original game. Stage 3b: started with -replay=<file> (WinDMApp::HandleCmdLineParam); the replay
// browser, the speeds and the REPLAY END screen come with stage 13.
//
// Watching replaces the game the way a mode switch does: TitleScreenIsFinished starts the watch instead of
// its two new games. Each segment of the replay gets a playback board (Board::mPlaybackOnly, made here, not
// by NewGame) that owns an MTRand from the segment's RNG start (Board::InitVariant), takes no input, never
// saves, counts or deletes anything, and only pauses when the replay says so (Board::Pause). The widget
// manager does not update it: BeforeFrame (from WinDMApp::UpdateFrames, before the frame's own update) runs
// the playback loop of 6.8 once per frame, one board update each time, so the board ticks as the recorded
// one did:
//   apply every event stamped with the board's tick (the gates of 6.3; a CHECK compares the LOGIC hash)
//   stop at the end of a segment (the next one gets a new board and LOADs the save the last SAVE made), or
//   at the replay's END (the game-over screen, New Game, or its tick), comparing END
//   a board that is paused with no event left at its tick to unpause it, a settle without a CHECK, a gate
//   that fails or a hash that differs make the replay corrupt
//   one Board::Update
// The calls the board makes into the app when its game ends (NewGame after New Game, ShowHighScoreWidget)
// end the watch instead (OnBoardEnd). The app state the board reads and changes (gGameConfig with the
// replay's rules, the 12 diff.txt values, mShowInstructions and mTimesPlayed from the segment's ENV) is
// swapped in for each step and swapped out after it, so the player's own (the registry a shutdown writes,
// the next game's rules) never changes; the mode buttons show the replay's mode until the watch stops.
//
// stdout gets "REPLAY ..." lines (the oracle's playback cases read them):
//   REPLAY start <file>: <segments> segments, <events> events, END <reason> tick <t>
//   REPLAY segment <k>: <normal|timed>, <seed n [user]|global stream state>[, LOAD <rng>[ (its own save)]]
//   REPLAY end: ok, <c> CHECKs, END <reason> tick <t> score <s> level <l> gems <g> draws <d>
//   REPLAY end: CORRUPT at segment <k> tick <t> event <i> <op>: <why>
//   REPLAY <file>: cannot play: <why>
// At the end the last board stays on screen. -replayquit quits then (the CLI and the oracle); otherwise New
// Game or a mode button ends the watch: the mode is restored and NewGame + ConfirmLoadGame offer the saved
// game, as 4.11 says (the buttons do the same while it plays). The watch holds while a modal dialog is open.

#include "Replay.h"
#include "GameConfig.h"
#include "Buffer.h"

#include <string>

namespace Sexy
{

class WinDMApp;
class Board;
struct Piece;

class WatchController
{
public:
	WatchController(WinDMApp* theApp, const std::string& theFileName);

	// WinDMApp hooks
	bool			Start();                        // TitleScreenIsFinished: true when the watch (or -replayquit) took over
	void			BeforeFrame();                  // UpdateFrames, before the frame's own update
	bool			OnBoardEnd(bool isGameOverScreen); // NewGame / ShowHighScoreWidget: true when the playback board called it
	bool			ButtonDepress(int theId);       // the side-bar buttons: true when the watch took the press
	bool			IsWatching() const { return (mStatus == STATUS_PLAYING) || (mStatus == STATUS_ENDED); }
	// Whether theBoard is the last board of a replay that has ended: it only stays on screen. The test hooks
	// (-trace, -codeccheck) see no settles of it: its last SAVE may have dropped a swap to STATE_PLAYING without
	// an update, which the recorded board, deleted right after, never showed.
	bool			IsEndedBoard(const Board* theBoard) const;

	// The headless verifier (games/bejeweled/src/headless, games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9) plays a
	// replay it holds in memory with this same player: Start reads theBytes instead of the file (its name is then
	// only the one the REPLAY lines show; the bytes are taken, not copied). Afterwards: the REPLAY line it ended
	// with ("REPLAY end: ..." or "REPLAY <file>: cannot play: ..."), the CHECKs that matched, the events applied,
	// and the file as Start read it (NULL when it was no well-formed replay), so the verifier parses it once.
	void			SetBytes(std::string theBytes) { mBytes.swap(theBytes); mHasBytes = true; }
	const std::string& GetResult() const { return mResult; }
	int				GetChecks() const { return mChecks; }
	int				GetEventsApplied() const { return mEventsApplied; }
	const Replay::File* GetFile() const { return mIsRead ? &mFile : NULL; }

private:
	enum Status
	{
		STATUS_IDLE,        // before Start
		STATUS_PLAYING,
		STATUS_ENDED,       // the replay ended (or was corrupt); its last board stays on screen
		STATUS_STOPPED      // back to the game
	};

	// The app state a playback board reads and changes; the other side's copy while not swapped in
	struct AppState
	{
		bool		mShowInstructions = false;
		int			mTimesPlayed = 0;
		GameConfig	mConfig;
		DiffValues	mDiff;
	};

	WinDMApp*		mApp;
	std::string		mFileName;
	bool			mHasBytes;          // SetBytes: the replay's bytes, instead of the file's
	std::string		mBytes;
	Replay::File	mFile;
	bool			mIsRead;            // Replay::Read took the file (mFile is all of it)
	GameRules		mRules;             // the replay's RULE
	Status			mStatus;
	AppState		mOther;             // the replay's state while the player's is in the app, and the other way round
	bool			mSwappedIn;
	bool			mSavedTimedMode;    // the player's mode, restored when the watch stops

	Board*			mBoard;             // the current segment's playback board (== mApp->mBoard)
	size_t			mSegment;
	size_t			mEvent;
	int				mLastState;         // the board's state after its last update (settles, as the recorder sees them)
	int				mCheckDue;          // the tick the board settled at, whose CHECK must come first; -1 for none
	bool			mBoardEnded;        // the board called NewGame (New Game's fall is over)
	bool			mNewGameApplied;    // a NEW_GAME event was applied in the last segment
	Buffer			mLoadSave;          // what the next LOAD reads
	bool			mLastSaveExact;     // the last SAVE's bytes were compared with its hash (and a sidecar's SVHA is)
	bool			mPrevSeeded;        // the segment before ended on an own seeded RNG ...
	Replay::RngState mPrevRng;          // ... in this state at its last SAVE
	int				mChecks;            // CHECKs that matched
	int				mEventsApplied;
	std::string		mResult;            // the "REPLAY end" line

	bool			Load(std::string* theError);
	void			Exchange();         // swaps mOther with the app's state
	void			CreateBoard();      // the current segment's board (swapped in)
	bool			Step();             // one iteration of 6.8's loop; false when the replay stops
	bool			Apply(const Replay::Event& theEvent);
	bool			NextSegment();
	bool			Finish();
	bool			Corrupt(const std::string& theWhy, const Replay::Event* theEvent = NULL);
	void			Stop();             // back to the game: the player's mode
	Piece*			CellPiece(int64_t theCol, int64_t theRow);
};

}

#endif
