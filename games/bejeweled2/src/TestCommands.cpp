// port: the standard test commands (docs/TEST_COMMANDS.md), reached only from a test's -script or live command channel
// through WinDMApp::HandleScriptCommand. A command that is never given changes nothing, draws no random number and
// moves no state. Each answers on stdout with one line: STATE {...}, MOVE {...}, OK <command> or
// ERROR <command>: <reason>. They act through the game's own code (the title screen's play link, the menu's own start,
// DoMainMenu, the board's own GameOver). The game's notes list them (GAME_NOTES.md, "Test commands").

#include "WinDMApp.h"
#include <climits>
#include <SDL.h>
#include "Board.h"
#include "Piece.h"
#include "MainMenu.h"
#include "TitleScreen.h"
#include "Profile.h"
#include "UserDialogs.h"
#include "HelpScreen.h"
#include "OptionsDialog.h"
#include "HyperlinkWidget.h"
#include "GalaxyParser.h"
#include "PuzzleStateOverlay.h"
#include "HighScoreWidget.h"
#include "EffectOverlay.h"
#include "Hyperspace.h"
#include "Dialog.h"
#include "DialogButton.h"
#include "Checkbox.h"
#include "WidgetManager.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace Sexy;

namespace
{

void Reply(const std::string& theLine)
{
	printf("%s\n", theLine.c_str());
	fflush(stdout);
}

std::string Words(const ScriptCommand& theCommand)
{
	return theCommand.mCommand + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs);
}

void ReplyOK(const ScriptCommand& theCommand)
{
	Reply("OK " + Words(theCommand));
}

void ReplyError(const ScriptCommand& theCommand, const std::string& theReason)
{
	Reply("ERROR " + Words(theCommand) + ": " + theReason);
}

const char* const kModeNames[9] = {"classic", "action", "puzzle", "endless", "twilight", "hyper", "cognito", "finity",
	"hidden"};

int ModeFromName(const std::string& theName)
{
	for (int i = 0; i < 9; i++)
	{
		if (theName == kModeNames[i])
			return i;
	}
	return -1;
}

const char* PhaseName(int theState)
{
	static const char* const kNames[] = {"new", "idle", "swapping", "swap back", "commit swap", "clearing",
		"game over fall", "7", "level complete wait", "9", "whirlpool", "level transition", "board dropping",
		"post warp", "hyper swap done", "pod change", "puzzle solved", "puzzle stuck", "galaxy", "fade",
		"secret finished", "galaxy slide 1", "galaxy slide 2", "galaxy slide 3"};
	return (theState >= 0 && theState < (int)(sizeof(kNames) / sizeof(kNames[0]))) ? kNames[theState] : "?";
}

// a gem's type as one character: 0 to 6 the colours, the special gems by their number (7, 8, 9 as letters a, b, c)
char TypeChar(int theType)
{
	if (theType < 0)
		return '.';
	if (theType < 7)
		return (char)('0' + theType);
	return (char)('a' + (theType - 7));
}

}  // namespace

// The screen a driver sees: the loading screen and the title, the menu, a dialog over either, the game (with the
// warp, the galaxy map and the end screen as parts of it) and the high-score screen after a game
std::string WinDMApp::TestScreen()
{
	if (mTitleScreen != NULL)
		return mTitleScreen->mLoadingCompleted ? "loading" : "title";
	if (!mDialogMap.empty())
		return "dialog";
	if (mHighScoreWidget != NULL)
		return "highscores";
	if (mBoard != NULL)
	{
		if (mMainMenu != NULL && mBoard->mScreenState != Board::SCREEN_SHOWN)
			return "menu";
		return "game";
	}
	if (mMainMenu != NULL)
		return "menu";
	return "loading";
}

// the counters that open the secret modes (Profile::IsModeUnlocked)
static void UnlockAll(Profile* theProfile)
{
	if (theProfile->mModeLevel[0] < 17)
		theProfile->mModeLevel[0] = 17;
	if (theProfile->mModeLevel[1] < 8)
		theProfile->mModeLevel[1] = 8;
	if (theProfile->mTotal7C < 80)
		theProfile->mTotal7C = 80;
	if (theProfile->mModeLevel[3] < 280)
		theProfile->mModeLevel[3] = 280;
}

bool gTestNoSparkle = false;  // set by the test command nosparkle (BoardEffects.cpp)

// -trace (a test flag): the same four values that origrun's --watch reads in the original (docs/ORIGRUN.md), in the same lines
// ("watch 1: 0 -> 10 at update 312"), so tools/origcmp pairs them value by value: 1 the board's score, 2 its level, 3 its state
// and 4 the position in its generator's state vector, each "-" while there is no board. Logged in the update in which a value
// differs from the last one logged, with the update count the framework keeps (+0x484 in the original).
void WinDMApp::TraceUpdate()
{
	static const int WATCHES = 8;
	static int sLast[WATCHES] = {INT_MIN, INT_MIN, INT_MIN, INT_MIN, INT_MIN, INT_MIN, INT_MIN, INT_MIN};  // INT_MIN: no board
	static bool sFirst = true;
	int aValue[WATCHES] = {INT_MIN, INT_MIN, INT_MIN, INT_MIN, INT_MIN, INT_MIN, INT_MIN, INT_MIN};
	if (mBoard != NULL)
	{
		uint32_t aState[624];
		int anIndex = 0;
		mBoard->mRand.GetState(aState, &anIndex);
		aValue[0] = mBoard->mScore;
		aValue[1] = mBoard->mLevel;
		aValue[2] = mBoard->mState;
		aValue[3] = anIndex;
		// watches 6 to 8 are for debugging (compare.py --pair 2=6 and so on, with WATCHES=... for the original): the board's +0x1730,
		// +0x1714 and +0x1768
		aValue[5] = mBoard->mDropDelay;
		aValue[6] = mBoard->mScreenState;
		aValue[7] = mBoard->m1768;
	}
	{
		// watch 5: the global generator's index, which the original's [0x67cf50 + 0x9c0] is (the draws of Rand())
		uint32_t aState[624];
		int anIndex = 0;
		GetRandMTState(aState, &anIndex);
		aValue[4] = anIndex;
	}
	for (int i = 0; i < WATCHES; i++)
	{
		if (!sFirst && aValue[i] == sLast[i])
			continue;
		std::string anOld = sFirst ? "?" : (sLast[i] == INT_MIN ? "-" : StrFormat("%d", sLast[i]));
		std::string aNew = aValue[i] == INT_MIN ? "-" : StrFormat("%d", aValue[i]);
		SDL_Log("watch %d: %s -> %s at update %d", i + 1, anOld.c_str(), aNew.c_str(), mUpdateCount);
		sLast[i] = aValue[i];
	}
	sFirst = false;
}

// what mTestGoto holds besides a mode (0 to 8)
enum { TEST_GOTO_OPTIONS = -2, TEST_GOTO_HELP = -3, TEST_GOTO_HIGHSCORES = -4 };

// "goto <mode>" from anywhere: every update takes the step the screen in front calls for, as a player's clicks would
// (the title's play link, the name of a first user, the welcome and other messages, the menu's mode button)
void WinDMApp::TestAdvanceGoto()
{
	if (mTitleScreen != NULL)
	{
		if (!mTitleScreen->mLoadingCompleted)
			LoadingCompleted();
		return;
	}
	if (!mDialogMap.empty())
	{
		CreateUserDialog* aUser = dynamic_cast<CreateUserDialog*>(GetDialog(DIALOG_CREATE_USER));
		if (aUser != NULL)
		{
			if (aUser->GetName().empty())
				aUser->SetName("Tester");
			FinishCreateUserDialog(true);
		}
		else if (mDialogMap.begin()->first == DIALOG_OPTIONS)
			FinishOptionsDialog(true);  // its Done button
		else if (mDialogMap.begin()->first == DIALOG_HELP)
		{
			// its Continue button once it has slid in; it slides out (and kills itself) on its own after that
			HelpScreen* aHelp = static_cast<HelpScreen*>(mDialogMap.begin()->second);
			if (aHelp->mState == HelpScreen::STATE_SHOWN)
				FinishHelpScreen(false);
		}
		else
			KillDialog(mDialogMap.begin()->first);
		return;
	}
	if (mHighScoreWidget != NULL)
	{
		// its Back button (a press while it is already closing does nothing)
		mHighScoreWidget->ButtonDepress(HighScoreWidget::BUTTON_BACK);
		return;
	}
	if (mBoard != NULL || mMainMenu == NULL)
	{
		// from a game (or its end screen): back to the menu, as the game's own menu button does
		DoMainMenu();
		return;
	}
	if (mMainMenu->mState != MainMenu::STATE_READY || mMainMenu->mWelcomeTimer > 0 || mMainMenu->mUserDialogTimer > 0 ||
		mMainMenu->mStartTimer > 0)
		return;
	if (mProfile == NULL || mProfile->mName.empty())
	{
		DoCreateUserDialog();
		return;
	}
	int aMode = mTestGoto;
	mTestGoto = -1;
	if (mTestUnlock)
	{
		UnlockAll(mProfile);
		mTestUnlock = false;
	}
	if (aMode == TEST_GOTO_OPTIONS)
		DoOptions();
	else if (aMode == TEST_GOTO_HELP)
		DoHelpScreen();
	else if (aMode == TEST_GOTO_HIGHSCORES)
		DoHighScoreWidget();
	else
	{
		mMainMenu->SelectMode(aMode);
		mMainMenu->StartGame();
	}
}

// "name":[x,y]: the middle of a visible widget, in game pixels (what a driver clicks)
static void AddWidgetPos(std::string& theList, const char* theName, Widget* theWidget)
{
	if (theWidget == NULL || !theWidget->mVisible)
		return;
	theList += StrFormat("%s\"%s\":[%d,%d]", theList.empty() ? "" : ",", theName, theWidget->mX + theWidget->mWidth / 2,
		theWidget->mY + theWidget->mHeight / 2);
}

bool WinDMApp::HandleTestCommand(const ScriptCommand& theCommand)
{
	const std::string& aCommand = theCommand.mCommand;
	const std::string& anArgs = theCommand.mArgs;
	if (aCommand != "state" && aCommand != "goto" && aCommand != "level" && aCommand != "unlock" && aCommand != "set" &&
		aCommand != "win" && aCommand != "lose" && aCommand != "seed" && aCommand != "move" && aCommand != "nosparkle")
		return false;

	std::string aScreen = TestScreen();
	Board* aBoard = (mBoard != NULL && mTitleScreen == NULL) ? mBoard : NULL;
	bool aPlaying = aBoard != NULL && mHighScoreWidget == NULL && aScreen == "game";

	if (aCommand == "state")
	{
		std::string aLine = "STATE {\"screen\":\"" + aScreen + "\"" + StrFormat(",\"updates\":%d", mUpdateCount);
		if (mMainMenu != NULL)
			aLine += StrFormat(",\"menu\":%d", (int)mMainMenu->mState);
		std::string aDialogs;
		for (DialogMap::iterator anItr = mDialogMap.begin(); anItr != mDialogMap.end(); ++anItr)
			aDialogs += StrFormat(aDialogs.empty() ? "%d" : ",%d", anItr->first);
		aLine += ",\"dialogs\":[" + aDialogs + "]";
		// the buttons of the dialog that is on top (the highest id), the middle of each, in game pixels
		if (!mDialogMap.empty())
		{
			Dialog* aDialog = mDialogMap.rbegin()->second;
			std::string aButtons;
			// the options dialog's second button, which leaves a game for the menu
			OptionsDialog* anOptions = dynamic_cast<OptionsDialog*>(aDialog);
			Widget* aList[3] = {aDialog->mYesButton, aDialog->mNoButton, anOptions != NULL ? (Widget*)anOptions->mMainMenuButton : NULL};
			const char* aNames[3] = {"yes", "no", "mainmenu"};
			for (int i = 0; i < 3; i++)
			{
				if (aList[i] == NULL || !aList[i]->mVisible)
					continue;
				aButtons += StrFormat("%s\"%s\":[%d,%d]", aButtons.empty() ? "" : ",", aNames[i],
					aList[i]->mX + aList[i]->mWidth / 2, aList[i]->mY + aList[i]->mHeight / 2);
			}
			aLine += ",\"buttons\":{" + aButtons + "}";
			// the options dialog's three display checkboxes: the middle of each and whether it is checked
			if (anOptions != NULL)
			{
				Checkbox* aBoxes[3] = {anOptions->mFullscreenCheckbox, anOptions->m3DCheckbox, anOptions->mHighResCheckbox};
				const char* aBoxNames[3] = {"fullscreen", "accel", "highres"};
				std::string aChecks;
				for (int i = 0; i < 3; i++)
					aChecks += StrFormat("%s\"%s\":[%d,%d,%d]", i == 0 ? "" : ",", aBoxNames[i], aBoxes[i]->mX + aBoxes[i]->mWidth / 2,
						aBoxes[i]->mY + aBoxes[i]->mHeight / 2, aBoxes[i]->IsChecked() ? 1 : 0);
				aLine += ",\"checks\":{" + aChecks + "}";
			}
		}
		// the widgets of the screen in front that a click reaches (not those of a dialog: "buttons" above): the title's play
		// link, the menu's buttons, the board's buttons
		if (mDialogMap.empty())
		{
			std::string aWidgets;
			if (mTitleScreen != NULL)
				AddWidgetPos(aWidgets, "play", mTitleScreen->mPlayLink);
			else if (aScreen == "highscores" && mHighScoreWidget != NULL)
			{
				// back to the menu (or the game), and play again (or reset the scores when it was opened from the menu)
				AddWidgetPos(aWidgets, "back", mHighScoreWidget->mBackButton);
				AddWidgetPos(aWidgets, "again", mHighScoreWidget->mActionButton);
			}
			else if (aBoard != NULL && aScreen == "game")
			{
				AddWidgetPos(aWidgets, "menu", aBoard->mMenuButton);
				AddWidgetPos(aWidgets, "hint", aBoard->mHintButton);
				AddWidgetPos(aWidgets, "third", aBoard->mThirdButton);
				AddWidgetPos(aWidgets, "tutorial1", aBoard->mTutorialButton1);
				AddWidgetPos(aWidgets, "tutorial2", aBoard->mTutorialButton2);
				AddWidgetPos(aWidgets, "playagain", aBoard->mPlayAgainButton);
				AddWidgetPos(aWidgets, "mainmenu", aBoard->mMainMenuButton);
			}
			else if (aScreen == "menu" && mMainMenu != NULL)
			{
				AddWidgetPos(aWidgets, "classic", mMainMenu->mClassicButton);
				AddWidgetPos(aWidgets, "action", mMainMenu->mActionButton);
				AddWidgetPos(aWidgets, "puzzle", mMainMenu->mPuzzleButton);
				AddWidgetPos(aWidgets, "endless", mMainMenu->mEndlessButton);
				AddWidgetPos(aWidgets, "secret", mMainMenu->mSecretButton);
				AddWidgetPos(aWidgets, "options", mMainMenu->mOptionsButton);
				AddWidgetPos(aWidgets, "scores", mMainMenu->mScoresButton);
				AddWidgetPos(aWidgets, "quit", mMainMenu->mQuitButton);
				AddWidgetPos(aWidgets, "user", mMainMenu->mUserLink);
			}
			if (!aWidgets.empty())
				aLine += ",\"widgets\":{" + aWidgets + "}";
			// whether the menu's secret button is on (a mode button then starts its secret mode)
			if (aScreen == "menu" && mMainMenu != NULL && mMainMenu->mSecretButton != NULL)
				aLine += mMainMenu->mSecretButton->mSelected ? ",\"secret\":true" : ",\"secret\":false";
			// whether the menu has a player yet (the new player's dialog opens a while after the menu)
			if (aScreen == "menu")
				aLine += (mProfile != NULL && !mProfile->mName.empty()) ? ",\"user\":true" : ",\"user\":false";
		}
		aLine += mIsWindowed ? ",\"windowed\":true" : ",\"windowed\":false";
		aLine += Is3DAccelerated() ? ",\"is3d\":true" : ",\"is3d\":false";
		aLine += mHasFocus ? ",\"focus\":true" : ",\"focus\":false";
		if (aBoard != NULL)
		{
			int aMode = mGameMode;
			bool aBusy = aBoard->mState != Board::STATE_IDLE || aBoard->mHyperspace != NULL || aBoard->mPauseCount > 0 ||
				aBoard->mInterfacePauseCount > 0 || aBoard->mUnpauseDelay > 0 || !mDialogMap.empty() ||
				aBoard->mScreenState != Board::SCREEN_SHOWN || mHighScoreWidget != NULL;
			aLine += StrFormat(",\"mode\":\"%s\",\"modeid\":%d,\"level\":%d,\"levelindex\":%d,\"score\":%d,\"phase\":\"%s\","
				"\"phaseid\":%d,\"tick\":%d,\"busy\":%s,\"paused\":%s",
				(aMode >= 0 && aMode < 9) ? kModeNames[aMode] : "?", aMode, aBoard->mNextLevel, aBoard->mLevel,
				aBoard->mScore, PhaseName(aBoard->mState), aBoard->mState, aBoard->mUpdateCnt, aBusy ? "true" : "false",
				(aBoard->mPauseCount > 0 || aBoard->mInterfacePauseCount > 0) ? "true" : "false");
			aLine += StrFormat(",\"hyperspace\":%s,\"tutorial\":%s,\"over\":%s", aBoard->mHyperspace != NULL ? "true" : "false",
				aBoard->mIsPlanetIntro ? "true" : "false",
				(aBoard->mState == Board::STATE_GAME_OVER_FALL || aBoard->mState == Board::STATE_7 || mHighScoreWidget != NULL) ? "true" : "false");
			if (aBoard->mSelectedPiece != NULL)
				aLine += StrFormat(",\"selected\":[%d,%d]", aBoard->mSelectedPiece->mCol, aBoard->mSelectedPiece->mRow);
			// the board: a row a string, a gem by its type's character, . for none
			std::string aRows;
			for (int aRow = 0; aRow < 8; aRow++)
			{
				aRows += aRow == 0 ? "\"" : ",\"";
				for (int aCol = 0; aCol < 8; aCol++)
				{
					Piece* aPiece = aBoard->mBoard[aRow][aCol];
					aRows += aPiece != NULL ? TypeChar(aPiece->mType) : '.';
				}
				aRows += "\"";
			}
			aLine += ",\"board\":[" + aRows + "]";
			// the gems with the flame flag (+0x70), a row a string: f for a flame gem, . for the others
			std::string aFlames;
			for (int aRow = 0; aRow < 8; aRow++)
			{
				aFlames += aRow == 0 ? "\"" : ",\"";
				for (int aCol = 0; aCol < 8; aCol++)
				{
					Piece* aPiece = aBoard->mBoard[aRow][aCol];
					aFlames += (aPiece != NULL && aPiece->m70) ? 'f' : '.';
				}
				aFlames += "\"";
			}
			aLine += ",\"flames\":[" + aFlames + "]";
		}
		else
			aLine += StrFormat(",\"modeid\":%d", mGameMode);
		Reply(aLine + "}");
		return true;
	}

	// nosparkle: Board::AddAlphaSparkle returns at once (origcmp patches the original the same way, so that the 640 by 480 scaled
	// image that its pixel test reads, which the port does not have, does not decide how many generator values the sweep uses)
	if (aCommand == "nosparkle")
	{
		gTestNoSparkle = true;
		ReplyOK(theCommand);
		return true;
	}

	if (aCommand == "seed")
	{
		if (anArgs.empty())
		{
			ReplyError(theCommand, "seed <n>");
			return true;
		}
		SRand((ulong)strtoul(anArgs.c_str(), NULL, 0));  // the same SRand as -seed=<n>
		ReplyOK(theCommand);
		return true;
	}

	if (aCommand == "unlock")
	{
		if (anArgs != "all")
		{
			ReplyError(theCommand, "only \"unlock all\"");
			return true;
		}
		if (mProfile == NULL || mProfile->mName.empty())
		{
			// no user yet: the goto that creates one unlocks when the menu is ready
			mTestUnlock = true;
			ReplyOK(theCommand);
			return true;
		}
		UnlockAll(mProfile);
		// the menu shows the secret button (and the secret modes' pictures) as it does after a change of profile
		if (mMainMenu != NULL)
		{
			mMainMenu->SyncProfile();
			mMainMenu->ResizeComponents();
		}
		ReplyOK(theCommand);
		return true;
	}

	if (aCommand == "goto")
	{
		if (anArgs == "credits")
		{
			ReplyError(theCommand, "the credits are no screen of their own: they roll after the last level of Classic");
			return true;
		}
		int aMode = ModeFromName(anArgs);
		if (anArgs == "options")
			aMode = TEST_GOTO_OPTIONS;
		else if (anArgs == "help")
			aMode = TEST_GOTO_HELP;
		else if (anArgs == "highscores")
			aMode = TEST_GOTO_HIGHSCORES;
		if (anArgs != "title" && anArgs != "menu" && aMode == -1)
		{
			ReplyError(theCommand, "unknown screen (menu, options, help, highscores, classic, action, puzzle, endless, "
				"twilight, hyper, cognito, finity, hidden)");
			return true;
		}
		if (anArgs == "title")
		{
			ReplyError(theCommand, "the game has no title screen to return to");
			return true;
		}
		if (aMode != -1)
		{
			// a mode, or the options, help or high scores: taken one update at a time from whatever screen is in front
			// (TestAdvanceGoto), so it works from launch (title, first user, welcome) as well as from the menu and from a game
			mTestGoto = aMode;
			ReplyOK(theCommand);
			return true;
		}
		if (aScreen == "loading")
		{
			ReplyError(theCommand, "the game is still loading");
			return true;
		}
		if (aScreen == "title")
		{
			// the play link's own click (the title screen slides away; the menu follows)
			if (!mTitleScreen->mLoadingCompleted)
				LoadingCompleted();
			ReplyOK(theCommand);
			return true;
		}
		// close what is open over the menu or the game, then go
		while (!mDialogMap.empty())
		{
			int anId = mDialogMap.begin()->first;
			if (!KillDialog(anId))
				break;
		}
		if (!mDialogMap.empty())
		{
			ReplyError(theCommand, "a dialog does not close");
			return true;
		}
		// menu (the modes were taken above)
		if (mMainMenu == NULL || mBoard != NULL)
			DoMainMenu();
		ReplyOK(theCommand);
		return true;
	}

	if (aCommand == "move")
	{
		// The first move that FindMove finds, the swap of two gems as two clicks, with the first at a place chosen by a
		// hash of the board and the score so that a game does not repeat one move forever. FindMove only looks (it puts
		// the gems back), so asking changes nothing, draws no random number of the game's and the same question gets the
		// same answer. While the board is busy it says to wait.
		if (mTestGoto != -1)
		{
			// a goto is still on its way to the mode (title, first user, menu)
			Reply("MOVE {\"kind\":\"wait\",\"input\":[{\"wait\":10}]}");
			return true;
		}
		if (!mDialogMap.empty() && aBoard != NULL && mHighScoreWidget == NULL)
		{
			// a message in front of the game (a one-time hint or explanation): its button, as a click. The "yes" button is
			// the OK or Continue of these; any other dialog gets its "no" (the quit and delete questions never answer yes)
			int anId = mDialogMap.rbegin()->first;
			Dialog* aDialog = mDialogMap.rbegin()->second;
			bool aYesIsOK = anId == 13 || anId == 23 || anId == 24 || anId == 25 || anId == 26 || anId == 36 || anId == 38 ||
				anId == 39;
			Widget* aButton = NULL;
			Widget* aList[2] = {aYesIsOK ? aDialog->mYesButton : aDialog->mNoButton, aYesIsOK ? aDialog->mNoButton : aDialog->mYesButton};
			for (int i = 0; i < 2 && aButton == NULL; i++)
			{
				if (aList[i] != NULL && aList[i]->mVisible)
					aButton = aList[i];
			}
			// a dialog that slides in or out (the How To Play screen, about 100 updates each way) is waited for: the button
			// must stand still between two questions, and the help screen must be fully shown
			static int sLastId = -1, sLastX = 0, sLastY = 0;
			bool aMoving = aButton == NULL;
			if (aButton != NULL)
			{
				aMoving = anId != sLastId || aButton->mX != sLastX || aButton->mY != sLastY;
				sLastId = anId;
				sLastX = aButton->mX;
				sLastY = aButton->mY;
				HelpScreen* aHelp = dynamic_cast<HelpScreen*>(aDialog);
				if (aHelp != NULL && aHelp->mState != HelpScreen::STATE_SHOWN)
					aMoving = true;
			}
			if (aMoving)
				Reply("MOVE {\"kind\":\"wait\",\"input\":[{\"wait\":10}]}");
			else
				Reply(StrFormat("MOVE {\"kind\":\"dialog\",\"id\":%d,\"input\":[{\"move\":[%d,%d]},{\"click\":[%d,%d]},{\"wait\":4}]}",
					anId, aButton->mX + aButton->mWidth / 2, aButton->mY + aButton->mHeight / 2,
					aButton->mX + aButton->mWidth / 2, aButton->mY + aButton->mHeight / 2));
			return true;
		}
		if (!aPlaying)
		{
			// a game that is starting or ending (the menu fading out, the end screen) is waited for; with no game, no move
			if (aBoard != NULL && mHighScoreWidget == NULL)
				Reply("MOVE {\"kind\":\"wait\",\"input\":[{\"wait\":10}]}");
			else
				Reply("MOVE null");
			return true;
		}
		if (mGameMode == Board::MODE_PUZZLE || mGameMode == Board::MODE_COGNITO)
		{
			// the planet's intro puzzle: the tutorial names every square to click, one after another (the gem, then the
			// place it goes to), and ends on two buttons ("Show Again" and "Continue")
			if (aBoard->mIsPlanetIntro && aBoard->mState == Board::STATE_IDLE && aBoard->mHyperspace == NULL &&
				aBoard->mPauseCount == 0 && aBoard->mInterfacePauseCount == 0 && aBoard->mUnpauseDelay == 0)
			{
				Widget* aContinue = aBoard->mTutorialButton2;
				if (aContinue != NULL)
				{
					// the buttons slide in: the same place twice running
					static int sLastContinueX = -1, sLastContinueY = -1;
					bool aStill = aContinue->mX == sLastContinueX && aContinue->mY == sLastContinueY && aContinue->mVisible &&
						aContinue->mWidth > 0;
					sLastContinueX = aContinue->mX;
					sLastContinueY = aContinue->mY;
					int aX = aContinue->mX + aContinue->mWidth / 2, aY = aContinue->mY + aContinue->mHeight / 2;
					if (aStill)
						Reply(StrFormat("MOVE {\"kind\":\"tutorial\",\"input\":[{\"move\":[%d,%d]},{\"click\":[%d,%d]},{\"wait\":4}]}",
							aX, aY, aX, aY));
					else
						Reply("MOVE {\"kind\":\"wait\",\"input\":[{\"wait\":10}]}");
					return true;
				}
				if (aBoard->mTutorialStep < (int)aBoard->mTutorialMoves.size())
				{
					const TutorialMove& aStep = aBoard->mTutorialMoves[aBoard->mTutorialStep];
					int aX = ScaleNum(aBoard->GetColX(aStep.mCol) + 42, 0), aY = ScaleNum(aBoard->GetRowY(aStep.mRow) + 42, 0);
					Reply(StrFormat("MOVE {\"kind\":\"tutorial\",\"input\":[{\"move\":[%d,%d]},{\"click\":[%d,%d]},{\"wait\":4}]}",
						aX, aY, aX, aY));
					return true;
				}
				Reply("MOVE {\"kind\":\"wait\",\"input\":[{\"wait\":10}]}");
				return true;
			}
			// between puzzles (the dome with its five pods): the first pod that is not solved, chosen with two clicks on it (the
			// first shows the pod, the second loads its puzzle); with all five solved, the dome itself (it warps on to the
			// next planet)
			if (aBoard->mPuzzleOverlay != NULL && aBoard->mState == Board::STATE_PUZZLE_SOLVED &&
				aBoard->mScreenState == Board::SCREEN_SHOWN && aBoard->mHyperspace == NULL && aBoard->mPendingPod == -1 &&
				aBoard->mPauseCount == 0 && aBoard->mInterfacePauseCount == 0 && aBoard->mUnpauseDelay == 0)
			{
				PuzzleStateOverlay* anOverlay = aBoard->mPuzzleOverlay;
				int aPod = -1;
				for (int i = 0; i < 5 && aPod == -1; i++)
				{
					if (!anOverlay->mPods[i].mSolved)
						aPod = i;
				}
				// the middle of the places that hit the pod (or the dome)
				long aSumX = 0, aSumY = 0, aHits = 0;
				for (int y = 0; y < 360; y += 4)
				{
					for (int x = 0; x < 330; x += 4)
					{
						bool aHit = aPod >= 0 ? anOverlay->GetPodAt(x, y) == aPod : anOverlay->MouseOverDome(x, y);
						if (aHit)
						{
							aSumX += x;
							aSumY += y;
							aHits++;
						}
					}
				}
				if (aHits == 0)
				{
					Reply("MOVE null");
					return true;
				}
				int aX = (int)(aSumX / aHits), aY = (int)(aSumY / aHits);
				if (aPod >= 0)
					Reply(StrFormat("MOVE {\"kind\":\"pod\",\"pod\":%d,\"input\":[{\"move\":[%d,%d]},{\"wait\":2},{\"click\":[%d,%d]},{\"wait\":20},"
						"{\"click\":[%d,%d]},{\"wait\":20}]}", aPod, aX, aY, aX, aY, aX, aY));
				else
					Reply(StrFormat("MOVE {\"kind\":\"dome\",\"input\":[{\"move\":[%d,%d]},{\"wait\":2},{\"click\":[%d,%d]},{\"wait\":20}]}",
						aX, aY, aX, aY));
				return true;
			}
			// the puzzle modes: the bot follows the puzzle's own solution file (the moves the hint shows)
			if (aBoard->mState != Board::STATE_IDLE || aBoard->mSelectedPiece != NULL || aBoard->mHyperspace != NULL ||
				aBoard->mPauseCount > 0 || aBoard->mInterfacePauseCount > 0 || aBoard->mUnpauseDelay > 0 ||
				aBoard->mPendingPod != -1)
			{
				Reply("MOVE {\"kind\":\"wait\",\"input\":[{\"wait\":10}]}");
				return true;
			}
			unsigned int aWords[64];
			int aWordCount = aBoard->FindPuzzleValidMoves(aWords, 64);
			if (aWordCount == 0)
			{
				Reply("MOVE null");
				return true;
			}
			// the first valid move; the byte is column, row and direction (left, up, right, down)
			static const int aStep[4][2] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};
			unsigned char aByte = aBoard->TranslateMoveByte((unsigned char)aWords[0]);
			int aCol1 = aByte & 7, aRow1 = aByte >> 3 & 7;
			int aCol2 = aCol1 + aStep[aByte >> 6][0], aRow2 = aRow1 + aStep[aByte >> 6][1];
			Piece* aFirst = aBoard->mBoard[aRow1][aCol1];
			if (aFirst == NULL || aCol2 < 0 || aCol2 > 7 || aRow2 < 0 || aRow2 > 7)
			{
				Reply("MOVE null");
				return true;
			}
			int aX1 = ScaleNum((int)(aFirst->mX + 42.0f), 0), aY1 = ScaleNum((int)(aFirst->mY + 42.0f), 0);
			int aX2, aY2;
			Piece* aSecond = aBoard->mBoard[aRow2][aCol2];
			if (aSecond != NULL)
			{
				aX2 = ScaleNum((int)(aSecond->mX + 42.0f), 0);
				aY2 = ScaleNum((int)(aSecond->mY + 42.0f), 0);
			}
			else
			{
				// an empty square: the neighbour's place, one square (84 pixels) from the first gem (the second click's
				// vertical direction is reversed on a flipped board)
				aX2 = aX1 + (aCol2 - aCol1) * ScaleNum(84, 0);
				aY2 = aY1 + (aRow2 - aRow1) * ScaleNum(84, 0) * (aBoard->mFlipped ? -1 : 1);
			}
			Reply(StrFormat("MOVE {\"kind\":\"swap\",\"from\":[%d,%d],\"to\":[%d,%d],\"input\":[{\"move\":[%d,%d]},"
				"{\"click\":[%d,%d]},{\"move\":[%d,%d]},{\"click\":[%d,%d]},{\"wait\":4}]}",
				aCol1, aRow1, aCol2, aRow2, aX1, aY1, aX1, aY1, aX2, aY2, aX2, aY2));
			return true;
		}
		if (aBoard->mState == Board::STATE_GAME_OVER_FALL)
		{
			Reply("MOVE null");
			return true;
		}
		if (aBoard->mState != Board::STATE_IDLE || aBoard->mSelectedPiece != NULL || aBoard->mHyperspace != NULL ||
			aBoard->mPauseCount > 0 || aBoard->mInterfacePauseCount > 0 || aBoard->mUnpauseDelay > 0)
		{
			Reply("MOVE {\"kind\":\"wait\",\"input\":[{\"wait\":10}]}");
			return true;
		}
		int aCount = 0;
		while (aBoard->FindMove(NULL, aCount, true, true))
			aCount++;
		if (aCount == 0)
		{
			Reply("MOVE null");
			return true;
		}
		unsigned aHash = 2166136261u;  // FNV-1a over the board's gems and the score
		for (int aRow = 0; aRow < 8; aRow++)
		{
			for (int aCol = 0; aCol < 8; aCol++)
			{
				Piece* aPiece = aBoard->mBoard[aRow][aCol];
				aHash = (aHash ^ (unsigned)(aPiece != NULL ? aPiece->mType + 1 : 0)) * 16777619u;
			}
		}
		aHash = (aHash ^ (unsigned)aBoard->mScore) * 16777619u;
		int aMove[4];
		aBoard->FindMove(aMove, (int)(aHash % (unsigned)aCount), true, true);
		Piece* aPiece1 = aBoard->mBoard[aMove[1]][aMove[0]];
		Piece* aPiece2 = aBoard->mBoard[aMove[3]][aMove[2]];
		int aX1 = ScaleNum((int)(aPiece1->mX + 42.0f), 0), aY1 = ScaleNum((int)(aPiece1->mY + 42.0f), 0);
		int aX2 = ScaleNum((int)(aPiece2->mX + 42.0f), 0), aY2 = ScaleNum((int)(aPiece2->mY + 42.0f), 0);
		// the input as a hand makes it: the pointer to the first gem and a click, the pointer to the second and a click
		// (TEST_COMMANDS.md's steps), then a few updates for the swap to begin
		Reply(StrFormat("MOVE {\"kind\":\"swap\",\"from\":[%d,%d],\"to\":[%d,%d],\"input\":[{\"move\":[%d,%d]},"
			"{\"click\":[%d,%d]},{\"move\":[%d,%d]},{\"click\":[%d,%d]},{\"wait\":4}]}",
			aMove[0], aMove[1], aMove[2], aMove[3], aX1, aY1, aX1, aY1, aX2, aY2, aX2, aY2));
		return true;
	}

	// The rest act on a game in progress
	if (!aPlaying)
	{
		ReplyError(theCommand, "no game is on (screen " + aScreen + ")");
		return true;
	}

	if (aCommand == "set")
	{
		size_t aSpace = anArgs.find(' ');
		if (aSpace == std::string::npos)
		{
			ReplyError(theCommand, "set <name> <value>");
			return true;
		}
		std::string aName = anArgs.substr(0, aSpace);
		int aValue = atoi(anArgs.c_str() + aSpace + 1);
		if (aName == "score")
			aBoard->mScore = aValue;
		else
		{
			ReplyError(theCommand, "can set score");
			return true;
		}
		ReplyOK(theCommand);
		return true;
	}

	if (aCommand == "lose")
	{
		// the game's own ending, as when no move is left
		aBoard->mEffectOverlay->DoStrechyText("NO MOVES!", 0x50);
		aBoard->GameOver(200);
		ReplyOK(theCommand);
		return true;
	}

	// win and level: the level's goal is met, and the board's own update starts the whirlpool (Board::DoUpdate: the bar is
	// full, state 8, then 9, then DoWhirlpool); in the puzzle modes win is the puzzle's own ending
	if (aCommand == "win" || aCommand == "level")
	{
		if (aBoard->mState != Board::STATE_IDLE)
		{
			ReplyError(theCommand, std::string("the board is busy (") + PhaseName(aBoard->mState) + ")");
			return true;
		}
		if (mGameMode == Board::MODE_PUZZLE || mGameMode == Board::MODE_COGNITO)
		{
			if (aCommand == "level")
				ReplyError(theCommand, "the puzzle modes have no levels");
			else
			{
				aBoard->PuzzleCompleted();
				ReplyOK(theCommand);
			}
			return true;
		}
		if (aCommand == "level")
		{
			int aLevel = atoi(anArgs.c_str());
			if (aLevel < 2)
			{
				ReplyError(theCommand, "level <n>, n from 2: the game goes up to it from the level before");
				return true;
			}
			// the whirlpool's end makes mLevel and mNextLevel one more and sets the new level's values
			aBoard->mLevel = aLevel - 2;
			aBoard->mNextLevel = aLevel - 1;
		}
		int aGoal = aBoard->m15eec + (int)aBoard->m15ef8;  // the level's target score plus the drained time
		if (aBoard->mScore < aGoal)
			aBoard->mScore = aGoal;
		if (aBoard->m15e40 < aGoal)
			aBoard->m15e40 = aGoal;  // the shown score that the bar is made from
		ReplyOK(theCommand);
		return true;
	}

	ReplyError(theCommand, aCommand + " is not implemented yet");
	return true;
}

// port: the framework's commands came first; the standard test commands next
bool WinDMApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	if (HandleTestCommand(theCommand))
		return true;
	return SexyApp::HandleScriptCommand(theCommand);
}
