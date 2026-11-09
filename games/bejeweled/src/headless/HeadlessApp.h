#ifndef __HEADLESSAPP_H__
#define __HEADLESSAPP_H__

// port(variant): the verifier's app (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9). Not part of the original game.
//
// The board code calls into WinDMApp (games/bejeweled/src/WinDMApp.h, the real header, whose base SexyApp is
// the shim in headless/shim). WinDMAppHeadless.cpp defines those members for an app with no window, sound,
// fonts, images, dialogs or files on disk:
//   - NewGame and ShowHighScoreWidget, which a board calls when its game ends (after New Game's fall, at the
//     game-over screen), end the replay being watched (WatchController::OnBoardEnd), else set the end flags
//     below;
//   - SetTimedMode sets the mode only (there are no mode buttons), KillDialog and DoInstructionDialogette do
//     nothing (the instructions dialog is not modal: it never pauses a board);
//   - files are kept in memory (SexyAppBase::mFiles);
//   - UpdateFrames is one frame: the replay player's step (WatchController::BeforeFrame), as at the start of
//     the game's WinDMApp::UpdateFrames; the caller then traces and deletes what SafeDeleteWidget was given.
// The data\diff.txt values start as the original game's (the replay player swaps a replay's in for each step).

#include "WinDMApp.h"

namespace Sexy
{

class HeadlessApp : public WinDMApp
{
public:
	bool			mNewGameCalled;         // a board called NewGame (New Game's fall ended) with no replay watched
	bool			mHighScoresCalled;      // a board called ShowHighScoreWidget (its game-over screen)

	HeadlessApp();
	virtual ~HeadlessApp();

	// The points font's ascent (Board::mPointsFont), which moves the score popups: a replay's ENV records it
	void			SetPointsAscent(int theAscent);
};

}

#endif
