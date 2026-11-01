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
class IslandMap;
class TempleScreen;
class PuzzleCompletedScreen;
class StoryScreen;
class HelpScreen;
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
	HelpScreen*				mHelpScreen;        // +0x67c "how to play", over the board or the menu (0x4328c0)
	TempleScreen*			mTempleScreen;      // +0x680 the puzzles of one temple, over a snapshot of the map (0x432420)
	LoadingScreen*			mLoadingScreen;     // +0x684 (Init)
	MainMenu*				mMainMenu;          // +0x688
	IslandMap*				mIslandMap;         // +0x68c (0x432600)
	PuzzleCompletedScreen*	mPuzzleCompleted;   // +0x690 (0x4326c0)
	Widget*					mMoreGames;         // +0x694 (0x432780)
	Widget*					mHintOverlay;       // +0x698
	Widget*					mHintScreen;        // +0x69c (0x432820)
	StoryScreen*			mStoryScreen;       // +0x6a0 (0x432960)
	Color*					mColorDark;         // +0x6ac (0x1a, 0x5e, 0x49)
	Color*					mColorMid;          // +0x6b0 (0x39, 0x79, 0x68)
	Color*					mColorLight;        // +0x6b4 (0x49, 0x8c, 0x7a)
	MemoryImage*			mHeads[NUM_HEADS][NUM_HEAD_EXPRESSIONS]; // +0x6b8: the gods' heads, each with its four expressions
	LevelData*				mLevelData;         // +0x768 (Init: 0x42abd0)
	UserDirectory*			mUserDirectory;     // +0x76c (Init: 0x43da10)
	ProgressManager*		mProgress;          // +0x770 (Init: 0x421380)
	int						mCurrentTemple;     // +0x780 the temple the player is in (an index of gTemples)
	std::string				mLastSolvedUuid;    // +0x77c the puzzle just solved: its picture comes in on the temple screen
	// +0x774 SessionLog (0x43aac0) is the original's statistics file for the developers; not ported

public:
	PixelusApp();
	virtual ~PixelusApp();

	virtual void			Init() override;                                // 0x4336f0 slot 17
	virtual void			LoadingThreadProc() override;                   // 0x433950 slot 18
	virtual void			PlayClicked() override;                         // 0x433b90 slot 79
	virtual void			HandlePaused(bool theWasPaused, bool theIsPaused, int theCount) override; // 0x4333a0 slot 84
	virtual Dialog*			NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
								const SexyString& theDialogLines, const SexyString& theDialogFooter,
								int theButtonMode) override;                // 0x4331e0 slot 21
	virtual void			ButtonDepress(int theId) override;              // 0x435310 slot 5
	virtual void			UpdateFrames() override;                        // 0x4e2f80: the answered quit question ends the game
	virtual void			HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override;

	// the screens (each removes the one before it)
	void					ShowBoard(const std::string& theLevel, int thePuzzle);  // 0x432550
	void					LeaveBoard();                                   // 0x433910: back to the temple the puzzle came from
	void					ShowResetDialog();                              // 0x434e40
	void					ShowGameMenu();                                 // 0x4194d0
	void					ShowHint();                                     // 0x433050 (0x432820)
	void					PuzzleCompleted(Board* theBoard);               // 0x435250: the board's last tile went down
	void					ContinueAfterPuzzle(int theOpenedTemple, bool theTempleDone, bool theGameDone);	// the tablet's button
	void					ShowMainMenu();                                 // 0x40b1c0
	void					PlayGame();                                     // 0x432a40: the menu's play button
	void					ShowUserSelect();                               // 0x432e40 (dialog 0x12)
	void					ShowNewUser();                                  // 0x432ec0 (dialog 0x13)
	void					ShowMoreGames();                                // 0x434710
	void					ShowOptions();                                  // 0x432a10 (0x4194d0 with 0)
	void					ShowOptionsDialog(bool theInGame);              // 0x4194d0
	void					ShowCredits();                                  // 0x432a00 (0x402290)
	void					ShowHelp();                                     // 0x4328c0
	void					CloseHelp();                                    // 0x432c20
	void					OptionsToMainMenu();                            // 0x4191e0 (500+1) with 0x433e70: the puzzle is saved and left
	void					ShowQuitDialog();                               // 0x4e3970
	void					ShowIslandMap();                                // 0x432600
	void					BackToMainMenu();                               // 0x433c90: from the island
	void					EnterTemple(int theTemple);                     // 0x4170d0: a click on a temple of the island
	void					ShowTemple();                                   // 0x432420 (the island is kept as its backdrop)
	void					BackToIsland();                                 // 0x432ac0
	void					StartPuzzle(bool theRestart);                   // 0x433820: from the temple to the board
	void					ShowResumeQuestion();                           // 0x434f70 (dialog 0x20)
	void					ResumeAnswer(bool theResume);                   // 0x433de0
	void					SkipTutorialOK();                               // 0x4101d0
	void					ShowStory(bool theFromMenu, int theMode, int theTo = 0);	// 0x432960: mode 0 the introduction, 1 a temple has opened, 2 the current temple again, 3 the ending
	void					EndStory(bool theToMenu);                       // 0x432d40 / 0x432d80
	void					ShowRenameDialog(const std::string& theName);   // 0x432fc0 (dialog 0x15)
	void					ShowDeleteQuestion(const std::string& theName); // 0x434bc0 (dialog 0x14)
	void					SelectUserOK(bool theOK);                       // 0x433f20: the answer of "Who are you?"
	void					NewUserOK(bool theOK);                          // 0x434860: the answer of the new-player dialog
	void					DeleteUserOK(bool theOK);                       // 0x434cf0
	void					RenameUserOK(bool theOK);                       // 0x4350a0
	void					StyleDialog(class GameDialog* theDialog);       // what 0x4331e0 does to every dialog

	// Test hooks (testhooks/Autoplay.cpp): the bot that plays the puzzles with the best known solutions
	bool					mAutoplay;
	int						mAutoplayPuzzles;
	int						mAutoplayCap;
	std::string				mAutoplayLog;
	std::string				mAutoplayShots;
	std::string				mAutoplayShow;          // -autoplayshow=<level>:<puzzle>[:<moves>:<ms>]: open that puzzle (with that record), photograph it, quit
	struct AutoplayState*	mAutoplayState;
	void					AutoplayUpdate();
	void					AutoplayShot(const std::string& theFileName) { SaveScreenshot(theFileName); }
};

extern PixelusApp* gApp;

}

#endif
