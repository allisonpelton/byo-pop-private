#ifndef __REPLAYRECORDER_H__
#define __REPLAYRECORDER_H__

// port(variant): records one board's game as a replay (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.11,
// 6.2-6.5). Not part of the original game.
//
// With features.replays, WinDMApp::NewGame gives every board it creates a recorder (Board::mRecorder; the
// board owns it). Its hooks in the translated Board functions only read the game: they never draw from an
// RNG, change a field play reads, or log. Recording points (6.3), each stamped with the board tick
// (mUpdateCnt) at which it happened, i.e. after update T and before T+1, where playback applies it:
//   SELECT c r     MouseDown's selection branch      DESELECT       MouseDown with the right button
//   TARGET c r     MouseDown's tail, MouseDrag       HINT           ShowHint, inside its if
//   PAUSE/UNPAUSE  Pause/Unpause (not the pair SaveGame makes: SAVE replays it)
//   NEW_GAME       RequestNewGame                    HOVER x y      MouseMove/MouseLeave (features.replays.hover)
//   XRAND n        Update's entry, before the pause checks: draws from the global stream that were not the
//                  board's (a board on the global stream only; the game-over HighScoreWidget's tip is after
//                  the board's last draw, so outside the replay)
//   SAVE kind h    SaveGame (1), the shutdown snapshot (2, never in the live recording)
//   LOAD           the end of a LoadGame that succeeded (Board::GameLoadedVariant)
//   CHECK h        Update's end, when the board settles into STATE_PLAYING: the LOGIC hash (StateCodec)
// The END record is taken when the game-over screen appears (no moves, time up), at ~Board after New Game
// (new game) or when a board is dropped unsaved (abandoned). A game that is saved goes on in the sidecar.
//
// Session mode (a board on the global stream): the segment starts from the global stream's state at the
// constructor (Board::mRngStart) and XRAND keeps the outside draws. The LOGIC hash then needs the stream as
// the board sees it: a mirror MTRand advanced by the XRAND and board draws, which also leaves out the draws
// made after the board's last update (the tip), as playback does.
//
// Files: finished replays go to the replay folder (GameConfig::mReplayDir: <user dir>/replays natively,
// /persist/replays on the web, or -replaydir), named <yyyymmdd>-<hhmmss>-<mode>-<score>[-n][-keep].bjr;
// games without a move are dropped. "-keep" marks a replay that qualified for a local high-score table:
// pruning keeps those and the newest features.replays.keep others (newest by the time the file was written).
// The sidecar (savegame.bjr next to savegame.dat, Board::WriteSaveSidecar) holds the game so far whenever a
// recorded game is saved. When that save goes away unresumed (No in the load dialog, or another board's save
// replaces it), the sidecar's game becomes a finished replay (END saved).

#include "Replay.h"

#include <string>

namespace Sexy
{

class WinDMApp;
class Board;
class MTRand;
struct Piece;

class ReplayRecorder
{
public:
	ReplayRecorder(WinDMApp* theApp, Board* theBoard);
	~ReplayRecorder();

	// Board hooks
	void			SyncExternalRand();
	void			AfterUpdate();
	void			OnSelect(Piece* thePiece);
	void			OnDeselect();
	void			OnTarget(Piece* thePiece);
	void			OnTwist(int theCol, int theRow, int theDir);   // stage 10: TWIST (Board::MouseDownVariant, MouseUpVariant)
	void			OnSlide(int theAxis, int theLine, int theStart, int theK, int theOffset); // stage 10: SLIDE (Board::MouseUpVariant)
	void			OnHint();
	void			OnUndo();                       // stage 6: UNDO (Board::RequestUndo)
	void			OnPause(bool theUserPause);
	void			OnUnpause(bool theUserPause);
	void			OnNewGame();
	void			OnHover(int theX, int theY);
	void			OnGameOver();
	void			BeginSave();
	std::string		EndSave(const std::string& theSave);           // records SAVE; returns the sidecar to write
	void			OnLoad(const std::string& theSave, const Replay::File* theSidecar); // theSidecar: one that goes with theSave, or NULL
	void			OnBoardDestroyed();

	bool			Loaded() const { return mLoaded; }
	// LOAD went on with the sidecar's replay (OnLoad): its segments are this replay's first
	bool			ContinuedSidecar() const { return mContinued; }
	// The sidecar a shutdown would leave now with theSave (a SaveGameSnapshot): the game so far ending in
	// SAVE(snapshot); "" once the game has ended
	std::string		SnapshotSidecar(const std::string& theSave);
	// Whether that sidecar is small enough for the web backup, which keeps the whole user dir in one
	// localStorage item (SexyAppBase::UpdateUserDataBackup): a recording of more events (hours with the hover
	// channel, about 7 bytes each) leaves only the RNG there (Board::SidecarSnapshot)
	enum { BACKUP_MAX_EVENTS = 100000 };
	bool			FitsBackup() const { return EventCount() <= (size_t) BACKUP_MAX_EVENTS; }
	size_t			EventCount() const;
	// True the first time only: the first sidecar this board writes replaces the one there is, whose game
	// is kept first (Board::WriteSaveSidecar)
	bool			FirstSidecar();
	// A sidecar's game that was saved, whose save goes away (the resume was declined: DeleteSaveGame without
	// a LOAD; or this board's first save replaces it), becomes a finished replay
	void			KeepSidecarGame(const std::string& theSidecar, const std::string& theSave);

	// The replay folder's files: a finished replay written under its name, then the folder pruned
	static bool		WriteFinished(const Replay::File& theFile, bool isTimed, bool isPinned);
	static void		Prune(const std::string& theDir, int theKeep);
	static bool		IsReplayName(const std::string& theName, bool* isPinned, int* theCopy = NULL);

private:
	WinDMApp*		mApp;
	Board*			mBoard;
	Replay::File	mFile;              // header, rules and the segments before this board's (a resumed game)
	Replay::Segment	mSegment;           // this board's
	bool			mHover;             // features.replays.hover
	bool			mEnded;             // END taken, or the game handed to the sidecar: nothing more is recorded
	bool			mNewGame;           // NEW_GAME was recorded
	int				mEndReason;         // how the game ended (OnGameOver)
	bool			mSaving;            // inside SaveGame
	bool			mSaved;             // the last event is a SAVE: the sidecar holds the game
	bool			mLoaded;            // LOAD was recorded
	bool			mContinued;         // ... and the sidecar's replay went on (OnLoad)
	bool			mWroteSidecar;      // this board has written a sidecar (FirstSidecar)
	int				mLastState;         // the board state after the last update (CHECK at a settle)
	int				mMoves;             // TARGET events with a cell, TWIST and SLIDE events: this board's and the resumed ones
	int				mLastHoverX;
	int				mLastHoverY;
	// Session mode
	bool			mSession;           // the board draws from the global stream
	uint32_t		mRandBase;          // Sexy::GetRandCount() when the board's stream started
	uint32_t		mDrawBase;          // Board::mBoardDraws then
	uint32_t		mXrandLogged;       // outside draws recorded since
	MTRand*			mMirror;            // the stream as the board sees it
	uint32_t		mMirrorDraws;

	void			Add(int theOp, int64_t theArg0 = 0, int64_t theArg1 = 0);
	void			AddCell(int theOp, Piece* thePiece);
	void			StartStream();
	const MTRand*	HashRng();          // the RNG the LOGIC hash takes (the mirror in session mode, else NULL)
	std::string		Sidecar(const std::string& theSave, int theSaveKind, bool isSnapshot);
	void			Finish(int theReason, bool isPinned);
	std::string		Meta();
};

}

#endif
