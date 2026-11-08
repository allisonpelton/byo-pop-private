#ifndef __BOARD_H__
#define __BOARD_H__

// Board.h: the game board (Sexy::Board, 0x59483b-0x5ae34e). TODO: a stub until it is translated.

#include "Widget.h"
#include "ButtonListener.h"

namespace Sexy
{

class WinDMApp;

class EffectOverlay
{
public:
	bool			mFlagC8;
	void			DoPortal() {}
};

class Board : public Widget, public ButtonListener
{
public:
	WinDMApp*		mApp;               // +0x8c
	EffectOverlay*	mEffectOverlay;
	bool			mShowHelp;
	int				mMessageTimer;
	int				mFlag1764;
	int				mFlag28;
	bool			mPitchDown;
	bool			mPitchUp;
	bool			mUserPaused;
	int				mMode1714;          // +0x1714
	int				mMode170C;          // +0x170c
	int				mHintDialogUp;      // +0x1950

public:
	Board(WinDMApp* theApp) { mApp = theApp; mEffectOverlay = new EffectOverlay(); mShowHelp = false; mMessageTimer = 0;
		mFlag1764 = mFlag28 = 0; mMode1714 = mMode170C = mHintDialogUp = 0; mPitchDown = mPitchUp = mUserPaused = false; }
	virtual void	ButtonDepress(int theId) override {}

	void			ShowNow() {}
	void			HideNow() {}
	void			Show() {}
	bool			IsGameInProgress() { return false; }
	bool			LoadGame() { return false; }
	void			InitialBackdropProcessing() {}
	void			NewGame() {}
	void			DeleteSavedGame() {}
	void			UndoUntilSolvable() {}
	void			TryShowHint() {}
	void			ShowNextPuzzleMove() {}
	static bool		HasSavedGame() { return false; }
	void			Pause(bool theFlag) {}
	void			Unpause(bool theFlag) {}
};

}

#endif
