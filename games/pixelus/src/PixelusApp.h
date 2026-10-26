#ifndef __PIXELUS_PIXELUSAPP_H__
#define __PIXELUS_PIXELUSAPP_H__

// PixelusApp.cpp: the application class of Pixelus Deluxe 1.0 and its screen flow (Pixelus.exe 0x4326c0-0x436000,
// WinMain 0x432360). Behaviour spec: docs/GAME_NOTES.md section 2.
//
// Original: class PixelusApp : public NcApp, vftable 0x5123a0 (87 slots), ctor 0x435880, data from +0x678:
// one pointer per screen (the board, the help screen, the temple, the loading screen, ...). Exactly one screen
// is on the widget manager at a time (plus the board under the help screen); each ShowXxx / the matching
// handler removes the old screen, deletes it later (SafeDeleteWidget) and makes the next one.
// Offsets below are the original's (code never depends on them).

#include "NcApp.h"
#include "Res.h"
#include "Color.h"
#include <string>

namespace Sexy
{

class Widget;
class Board;
class MainMenu;
class LoadingScreen;
class MemoryImage;
class LevelData;
class ProgressManager;
class UserDirectory;

class PixelusApp : public NcApp
{
public:
	enum
	{
		GAME_WIDTH = 640,
		GAME_HEIGHT = 480,
		NUM_HEAD_EXPRESSIONS = 4,
		NUM_HEADS = 11,
		NUM_LOADING_TASKS = 328
	};

	Board*					mBoard;             // +0x678 the puzzle screen (0x432550)
	Widget*					mHelpScreen;        // +0x67c over the board (0x4328c0)
	Widget*					mTempleScreen;      // +0x680 the puzzles of one temple, over a snapshot of the map (0x432420)
	LoadingScreen*			mLoadingScreen;     // +0x684 (Init)
	MainMenu*				mMainMenu;          // +0x688
	Widget*					mIslandMap;         // +0x68c (0x432600)
	Widget*					mPuzzleCompleted;   // +0x690 (0x4326c0)
	Widget*					mMoreGames;         // +0x694 (0x432780)
	Widget*					mHintOverlay;       // +0x698
	Widget*					mHintScreen;        // +0x69c (0x432820)
	Widget*					mStoryScreen;       // +0x6a0 (0x432960)
	Color*					mColorDark;         // +0x6ac (0x1a, 0x5e, 0x49)
	Color*					mColorMid;          // +0x6b0 (0x39, 0x79, 0x68)
	Color*					mColorLight;        // +0x6b4 (0x49, 0x8c, 0x7a)
	MemoryImage*			mHeads[NUM_HEADS][NUM_HEAD_EXPRESSIONS]; // +0x6b8: the gods' heads, each with its four expressions
	LevelData*				mLevelData;         // +0x768 (Init: 0x42abd0)
	UserDirectory*			mUserDirectory;     // +0x76c (Init: 0x43da10)
	ProgressManager*		mProgress;          // +0x770 (Init: 0x421380)
	// +0x774 SessionLog (0x43aac0) is the original's statistics file for the developers; not ported

public:
	PixelusApp();
	virtual ~PixelusApp();

	virtual void			Init() override;                                // 0x4336f0 slot 17
	virtual void			LoadingThreadProc() override;                   // 0x433950 slot 18
	virtual void			PlayClicked() override;                         // 0x433b90 slot 79
	virtual void			HandlePaused(bool thePaused, bool theWasPaused, int theCount) override; // 0x4333a0 slot 84
	virtual Dialog*			NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
								const SexyString& theDialogLines, const SexyString& theDialogFooter,
								int theButtonMode) override;                // 0x4331e0 slot 21
	virtual void			ButtonDepress(int theId) override;              // 0x435310 slot 5

	// the screens (each removes the one before it)
	void					ShowBoard(const std::string& theLevel, int thePuzzle);  // 0x432550
	void					LeaveBoard();                                   // 0x433910: back to the temple the puzzle came from
	void					ShowResetDialog();                              // 0x434e40
	void					ShowGameMenu();                                 // 0x4194d0
	void					ShowHint();                                     // 0x433050 (0x432820)
	void					PuzzleCompleted(Board* theBoard);               // 0x41b3c0 and what leads to it
	void					ShowMainMenu();                                 // 0x40b1c0
	void					PlayGame();                                     // 0x432a40: the menu's play button
	void					ShowUserSelect();                               // 0x432e40 (dialog 0x12)
	void					ShowNewUser();                                  // 0x432ec0 (dialog 0x13)
	void					ShowMoreGames();                                // 0x434710
	void					ShowOptions();                                  // 0x432a10 (0x4194d0 with 0)
	void					ShowQuitDialog();                               // 0x4e3970
	void					ShowIslandMap();                                // 0x432600
	void					ShowStory(bool theFromMenu, int theChapter);    // 0x432960
	void					ShowRenameDialog(const std::string& theName);   // 0x432fc0 (dialog 0x15)
	void					ShowDeleteQuestion(const std::string& theName); // 0x434bc0 (dialog 0x14)
	void					SelectUserOK(bool theOK);                       // 0x433f20: the answer of "Who are you?"
	void					NewUserOK(bool theOK);                          // 0x434860: the answer of the new-player dialog
	void					DeleteUserOK(bool theOK);                       // 0x434cf0
	void					RenameUserOK(bool theOK);                       // 0x4350a0
	void					StyleDialog(class GameDialog* theDialog);       // what 0x4331e0 does to every dialog
};

extern PixelusApp* gApp;

}

#endif
