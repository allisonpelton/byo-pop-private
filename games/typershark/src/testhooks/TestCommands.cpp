// port: the standard test commands (docs/TEST_COMMANDS.md), reached only from a test's -script / -commands lines through
// WinWSApp::HandleScriptCommand. A command that is never given changes nothing and draws no random number. Each answers on the
// log (stdout) with one line: STATE {...}, MOVE {...}, OK <command> or ERROR <command>: <reason>. They act through the game's
// own code: the buttons' ids (WinWSApp::ButtonDepress), the app's Show* functions, the map screen's level choice and the
// board's own endings. The move comes from Bot.cpp, a pure function of a snapshot of the board, so a bot mod can reuse it.
//
//   state                  STATE {...}: screen, base, mode, level, score, lives, enemy, timer, over, paused, depth, board {...}
//   goto <screen>          title | menu | options | credits | highscores | stats | map | tutor | adventure [0..4] | abyss
//   level <n>              adventure: site n (0..35) through the map screen's choice and StartBoard; abyss: depth n in feet;
//                          tutor: lesson n
//   unlock all             every typing lesson passed and the deepest abyss depth set, the profile written
//   set <name> <value>     score | lives | timer (ms) | depth | emp (0..1) | hints (0/1)
//   win, lose              the level is won (the diver's level-won state) / the last life is lost (Board::PlayerEaten)
//   seed <n>               the game's generator and the C library's (the piranhas')
//   move                   MOVE {...} the bot's next move, or MOVE null
//   shot <file.png>        the screen as the last draw left it, saved without drawing again (needs -drawevery; see the command)

#include "WinWSApp.h"
#include "TestHooks.h"
#include "Board.h"
#include "Player.h"
#include "Enemy.h"
#include "LessonManager.h"
#include "MapScreen.h"
#include "MenuScreen.h"
#include "TitleScreen.h"
#include "TypingTutor.h"
#include "OptionsDialog.h"
#include "NewUserDialog.h"
#include "SelectUserDialog.h"
#include "DeleteUserDialog.h"
#include "UserProfile.h"
#include "UserProfileMgr.h"
#include "WSTimer.h"
#include "Res.h"
#include "EditWidget.h"
#include "WidgetManager.h"
#include "Common.h"
#include "Compat2004.h"
#include "MTRand.h"
#include "DDInterface.h"

#include <stb_image_write.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace Sexy;

#ifdef TS_LEAKCOUNT
namespace Sexy { void LeakCountReport(const char* theTag, bool theSites); }	// LeakCount.cpp
#endif

namespace
{

void Reply(const std::string& theLine)
{
	printf("%s\n", theLine.c_str());
	fflush(stdout);
}

void ReplyError(const ScriptCommand& theCommand, const std::string& theReason)
{
	Reply("ERROR " + theCommand.mCommand + ": " + theReason);
}

bool ParseInt(const std::string& theText, int* theValue)
{
	if (theText.empty())
		return false;
	char* anEnd = NULL;
	long aValue = strtol(theText.c_str(), &anEnd, 10);
	if (*anEnd != 0)
		return false;
	*theValue = (int) aValue;
	return true;
}

// every dialog the game opens is closed, with the app's own pointers to them and the flags that say what a dialog is asking
void CloseDialogs(WinWSApp* app)
{
	for (int i = 0; i < gNumTestDialogs; i++)
		if (app->GetDialog(gTestDialogs[i].mId) != NULL)
			app->KillDialog(gTestDialogs[i].mId);
	app->mMessageDialog = NULL;
	app->mDialog61c = NULL;
	app->mHintDialog = NULL;
	app->mAbyssModeDialog = NULL;
	app->mOptionsDialog = NULL;
	app->mDeleteUserDialog = NULL;
	app->mOpenDialogCount = 0;
	app->mFlag660 = false;
	app->mFlag670 = false;
	app->mFlag68c = false;
	app->mFlag68d = false;
	app->mFlag910 = false;
}

// back to the menu screen from wherever the game is, by the game's own ways: the title screen's play button, the Close buttons
// of the stats and high score screens, ResetGame (which ends a game and shows the menu) and ShowMenuScreen (which also takes
// the typing tutor away)
bool ClearToMenu(WinWSApp* app, std::string* theError)
{
	if (!app->mLoaded)
	{
		*theError = "the game is still loading";
		return false;
	}
	CloseDialogs(app);
	if (app->mTitleScreen != NULL)
		app->ButtonDepress(TitleScreen::CONTINUE_ID);
	if (app->mStatsScreen != NULL)
		app->ButtonDepress(DIALOG_STATS_CLOSE);
	if (app->mHighScoreScreen != NULL)
		app->ButtonDepress(DIALOG_HIGHSCORE_CLOSE);
	if (app->mBoard != NULL)
		app->ResetGame(false);
	if (app->mMenuScreen == NULL)
		app->ShowMenuScreen();
	return true;
}

// a player to play with: the one the game has, else "Tester", made the way the game does (the new-user dialog's OK, and "no" to
// the typing test it offers)
bool EnsureUser(WinWSApp* app, std::string* theError)
{
	if (app->mUser != NULL)
		return true;
	if (app->mProfileMgr->LoadUser("Tester") != NULL)
	{
		// a Tester from an earlier run: the select dialog's OK
		app->mProfileMgr->RegisterDefaultUserName("Tester");
		app->mUser = app->mProfileMgr->LoadUser("Tester");
		if (app->mUser != NULL)
			app->mFlag8f8 = (app->mUser->mFlag1dc == false);
	}
	else
	{
		if (app->GetDialog(DIALOG_NEW_USER) == NULL)
			app->ShowNewUserDialog("");
		NewUserDialog* aDialog = dynamic_cast<NewUserDialog*>(app->GetDialog(DIALOG_NEW_USER));
		if (aDialog == NULL)
		{
			*theError = "the new-user dialog did not open";
			return false;
		}
		aDialog->mNameEdit->mString = "Tester";
		app->ButtonDepress(0xc1d);			// NewUserDialog's OK
		if (app->GetDialog(DIALOG_MESSAGE) != NULL)
			app->ButtonDepress(0xdac);		// "ASSESS WPM?": no
	}
	if (app->mUser == NULL)
	{
		*theError = "no player could be made";
		return false;
	}
	return true;
}

// a new game from the menu, as its button does: the Adventure button and the difficulty dialog's choice, or the Abyss button
bool StartMode(WinWSApp* app, bool theAbyss, int theDifficulty, std::string* theError)
{
	if (!ClearToMenu(app, theError) || !EnsureUser(app, theError))
		return false;
	CloseDialogs(app);
	if (!theAbyss)
	{
		app->ButtonDepress(MenuScreen::ADVENTURE_BUTTON_ID);
		if (app->GetDialog(DIALOG_MESSAGE) != NULL)
			app->ButtonDepress(0xdac);		// a saved game: "CONTINUE?" answered no, which opens the difficulty dialog
		if (app->GetDialog(DIALOG_DIFFICULTY) == NULL)
		{
			*theError = "the difficulty dialog did not open";
			return false;
		}
		app->ButtonDepress(600 + theDifficulty);
	}
	else
	{
		// the dive starts at the surface: no "CONTINUE?" question about the deepest depth
		int aBestDepth = app->mUser->mBestDepth;
		app->mUser->mBestDepth = 0;
		app->ButtonDepress(MenuScreen::ABYSS_BUTTON_ID);
		app->mUser->mBestDepth = aBestDepth;
	}
	if (app->mBoard == NULL)
	{
		*theError = "the game did not start";
		return false;
	}
	return true;
}

bool InAbyss(WinWSApp* app)
{
	return app->mLessonManager != NULL && app->mLessonManager->mAbyssMode;
}

}  // namespace

bool WinWSApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	const std::string& aCommand = theCommand.mCommand;
	const std::string& anArgs = theCommand.mArgs;
	size_t aSpace = anArgs.find(' ');
	std::string aFirst = anArgs.substr(0, aSpace);
	size_t aRestStart = aSpace == std::string::npos ? std::string::npos : anArgs.find_first_not_of(' ', aSpace);
	std::string aRest = aRestStart == std::string::npos ? "" : anArgs.substr(aRestStart);
	std::string anError;
	WinWSApp* app = this;

	if (aCommand == "state")
	{
		Reply("STATE " + TestStateJson(app));
		return true;
	}

	if (aCommand == "move")
	{
		Reply(BotMoveToJson(ChooseMove(TestTakeSnapshot(app))));
		return true;
	}

	// shot <file.png>: the screen as the last draw left it, read back and saved without drawing again. The framework's
	// `screenshot` draws every widget once more before it reads the screen, and in this game a draw is a step: Board::Draw
	// eases the two strips of sea and the rating popup once a draw, MapScreen::Draw the final scroll, and a widget that is
	// painted over itself blends its half-transparent pixels again (the status bar while the board is paused). So every
	// `screenshot` puts the port one step ahead of the original, whose shots origrun takes from the window. With -drawevery
	// every update is drawn before the next line of the script runs, so the screen holds the frame of the update before.
	if (aCommand == "shot")
	{
		if (anArgs.empty() || !mDrawEvery)
		{
			ReplyError(theCommand, anArgs.empty() ? "shot <file.png>" : "needs -drawevery (the screen must hold the last update's frame)");
			return true;
		}
		std::vector<ulong> aBits((size_t) mWidth * mHeight);
		if (!mDDInterface->ReadScreen(&aBits[0]))
		{
			ReplyError(theCommand, "cannot read the screen");
			return true;
		}
		std::vector<uchar> aRGBA((size_t) mWidth * mHeight * 4);
		for (int i = 0; i < mWidth * mHeight; i++)
		{
			aRGBA[i * 4 + 0] = (uchar) (aBits[i] >> 16);
			aRGBA[i * 4 + 1] = (uchar) (aBits[i] >> 8);
			aRGBA[i * 4 + 2] = (uchar) aBits[i];
			aRGBA[i * 4 + 3] = 255;
		}
		if (!stbi_write_png(anArgs.c_str(), mWidth, mHeight, 4, &aRGBA[0], mWidth * 4))
			ReplyError(theCommand, "cannot write the file");
		else
			Reply(StrFormat("OK shot %s (update %d)", anArgs.c_str(), mUpdateCount));
		return true;
	}

#ifdef TS_LEAKCOUNT
	// leak <tag> [sites]: the scratch leak-counting build only (LeakCount.cpp): prints "LEAK <tag> live=..." and, with sites,
	// writes the live blocks by allocation site to leak_<tag>.sites
	if (aCommand == "leak")
	{
		LeakCountReport(aFirst.empty() ? "x" : aFirst.c_str(), aRest == "sites");
		return true;
	}

	// the user flows of the leak scenarios, which have no screen coordinates to click: the same handlers a click reaches
	//   press <id>                       the app's ButtonDepress
	//   dyes <dialog id> | dno <id>      the dialog's yes / no button (the dialog's own ButtonDepress, which calls the app's)
	//   dlist <dialog id> <line> [n]     a click (n = 1, 2) on a line of the select-user or delete-user dialog's list
	//   dname <text>                     the text of the new-user dialog's edit box
	if (aCommand == "press")
	{
		int anId;
		if (!ParseInt(aFirst, &anId))
			anId = (int) strtol(aFirst.c_str(), NULL, 0);
		ButtonDepress(anId);
		Reply("OK press " + anArgs);
		return true;
	}
	if (aCommand == "dyes" || aCommand == "dno")
	{
		Dialog* aDialog = GetDialog((int) strtol(aFirst.c_str(), NULL, 0));
		if (aDialog == NULL)
		{
			ReplyError(theCommand, "no such dialog");
			return true;
		}
		aDialog->ButtonDepress(aDialog->mId + (aCommand == "dyes" ? 2000 : 3000));
		Reply("OK " + aCommand + " " + anArgs);
		return true;
	}
	if (aCommand == "dlist")
	{
		int aDialogId = (int) strtol(aFirst.c_str(), NULL, 0);
		size_t aSecond = aRest.find(' ');
		int aLine = (int) strtol(aRest.substr(0, aSecond).c_str(), NULL, 0);
		int aClicks = aSecond == std::string::npos ? 1 : (int) strtol(aRest.substr(aSecond + 1).c_str(), NULL, 0);
		if (aDialogId == DIALOG_SELECT_USER && GetDialog(aDialogId) != NULL)
			((SelectUserDialog*) GetDialog(aDialogId))->ListClicked(0, aLine, aClicks);
		else if (aDialogId == DIALOG_DELETE_USER && GetDialog(aDialogId) != NULL)
			((DeleteUserDialog*) GetDialog(aDialogId))->ListClicked(0, aLine, aClicks);
		else
		{
			ReplyError(theCommand, "no such dialog");
			return true;
		}
		Reply("OK dlist " + anArgs);
		return true;
	}
	if (aCommand == "dname")
	{
		NewUserDialog* aDialog = dynamic_cast<NewUserDialog*>(GetDialog(DIALOG_NEW_USER));
		if (aDialog == NULL)
		{
			ReplyError(theCommand, "no new-user dialog");
			return true;
		}
		aDialog->mNameEdit->mString = anArgs;
		Reply("OK dname " + anArgs);
		return true;
	}
#endif

	if (aCommand == "seed")
	{
		int aSeed;
		if (!ParseInt(aFirst, &aSeed))
		{
			ReplyError(theCommand, "seed <number>");
			return true;
		}
		SeedMTRand2004(*gGameRand, (unsigned long) aSeed);
		CrtSRand((unsigned int) aSeed);
		Reply("OK seed " + anArgs);
		return true;
	}

	if (aCommand == "goto")
	{
		if (!mLoaded)
		{
			ReplyError(theCommand, "the game is still loading");
			return true;
		}
		if (aFirst == "title")
		{
			if (mTitleScreen == NULL)
			{
				if (!ClearToMenu(app, &anError))
				{
					ReplyError(theCommand, anError);
					return true;
				}
				RemoveMenuScreen();
				if (mMapScreen != NULL)
				{
					mWidgetManager->RemoveWidget(mMapScreen);
					SafeDeleteWidget(mMapScreen);
					mMapScreen = NULL;
				}
				// as Init makes it
				mTitleScreen = new TitleScreen(this);
				mTitleScreen->Resize(0, 0, mWidth, mHeight);
				mTitleScreen->AddControls();
				mWidgetManager->AddWidget(mTitleScreen);
				mTitleScreen->MarkDirty();
			}
			Reply("OK goto title");
			return true;
		}

		if (aFirst == "adventure" || aFirst == "abyss")
		{
			int aDifficulty = 2;		// the middle of the five (easy, normal, hard, expert, X-treme)
			if (!aRest.empty() && (!ParseInt(aRest, &aDifficulty) || aDifficulty < 0 || aDifficulty > 4 || aFirst == "abyss"))
			{
				ReplyError(theCommand, aFirst == "abyss" ? "abyss takes no difficulty" : "difficulty is 0..4");
				return true;
			}
			if (!StartMode(app, aFirst == "abyss", aDifficulty, &anError))
			{
				ReplyError(theCommand, anError);
				return true;
			}
			Reply("OK goto " + anArgs);
			return true;
		}

		if (aFirst == "map")
		{
			if (mBoard == NULL || mLessonManager == NULL)
			{
				if (!StartMode(app, false, 2, &anError))
				{
					ReplyError(theCommand, anError);
					return true;
				}
			}
			if (InAbyss(app))
			{
				ReplyError(theCommand, "the abyss has no map");
				return true;
			}
			CloseDialogs(app);
			if (mBoard->mVisible)
				ReturnToMap();		// a level is over: the map comes back
			Reply("OK goto map");
			return true;
		}

		if (aFirst == "credits" && mOptionsDialog != NULL && GetDialog(DIALOG_OPTIONS) == mOptionsDialog)
		{
			// the options are open already: closing them would have their destructor close the credits box again (later)
			if (GetDialog(DIALOG_CREDITS) == NULL)
				mOptionsDialog->ButtonDepress(OptionsDialog::CREDITS_BUTTON_ID);
			Reply("OK goto credits");
			return true;
		}

		if (aFirst == "menu" || aFirst == "options" || aFirst == "credits" || aFirst == "highscores" || aFirst == "stats" ||
			aFirst == "tutor")
		{
			if (!ClearToMenu(app, &anError) || !EnsureUser(app, &anError))
			{
				ReplyError(theCommand, anError);
				return true;
			}
			CloseDialogs(app);
			if (aFirst == "options" || aFirst == "credits")
			{
				ButtonDepress(MenuScreen::OPTIONS_BUTTON_ID);
				if (aFirst == "credits" && mOptionsDialog != NULL)
					mOptionsDialog->ButtonDepress(OptionsDialog::CREDITS_BUTTON_ID);
			}
			else if (aFirst == "highscores")
				ButtonDepress(MenuScreen::HIGH_SCORES_BUTTON_ID);
			else if (aFirst == "stats")
				ButtonDepress(MenuScreen::STATS_LINK_ID);
			else if (aFirst == "tutor")
				ButtonDepress(MenuScreen::TUTOR_BUTTON_ID);
			Reply("OK goto " + aFirst);
			return true;
		}

		ReplyError(theCommand, "goto title | menu | options | credits | highscores | stats | map | tutor | adventure [0..4] | abyss");
		return true;
	}

	if (aCommand == "level")
	{
		int aLevel;
		if (!ParseInt(aFirst, &aLevel) || aLevel < 0)
		{
			ReplyError(theCommand, "level <n>: a site 0..35 (adventure), a depth in feet (abyss) or a lesson 0..18 (tutor)");
			return true;
		}

		if (mTypingTutor != NULL && mStatsScreen == NULL && mHighScoreScreen == NULL)
		{
			// the typing tutor: its Next and Previous buttons
			if (aLevel > 18 || mTypingTutor->mLessons == NULL)
			{
				ReplyError(theCommand, "the tutor has the lessons 0..18");
				return true;
			}
			CloseDialogs(app);
			while (mTypingTutor->mLessons->mCurrent < aLevel)
				mTypingTutor->ButtonDepress(TypingTutor::NEXT_BUTTON_ID);
			while (mTypingTutor->mLessons->mCurrent > aLevel)
				mTypingTutor->ButtonDepress(TypingTutor::PREV_BUTTON_ID);
			Reply("OK level " + anArgs);
			return true;
		}

		if (mBoard == NULL || mLessonManager == NULL)
		{
			if (!StartMode(app, false, 2, &anError))
			{
				ReplyError(theCommand, anError);
				return true;
			}
		}

		if (!InAbyss(app) && GetDialog(DIALOG_GAME_OVER) != NULL)
			ButtonDepress(0x16832);		// the game is lost: the game-over dialog's "play again" (RestartBoard)
		CloseDialogs(app);
		if (InAbyss(app))
		{
			// as the "CONTINUE?" dialog's yes does: a new board, then the depth
			mAbyssDepth = aLevel;
			mUser->SetField1e4((aLevel / 1000) % 4 - 1);
			CreateBoard();
			if (aLevel > 0)
				mBoard->SetDepth(aLevel);
			Reply("OK level " + anArgs);
			return true;
		}

		if (aLevel > 35 || mMapScreen == NULL || mStatusBar == NULL)
		{
			ReplyError(theCommand, "the adventure has the sites 0..35");
			return true;
		}
		if (aLevel > 0 && !mUser->mFlag1dc)
		{
			// a player who has not killed the tutorial's two sharks (the first level's words) never gets a wave on the other
			// levels: the dive waits for them. The tutorial counts as seen.
			mUser->mFlag1dc = true;
			mFlag8f8 = false;
			mProfileMgr->WriteUserProfileToRegistry(mUser, false, false);
		}
		// the level in progress ends as it does when the diver reaches the surface (ReturnToMap: the board's results go to the
		// game's totals and the map comes back), the site is chosen on the map (OnLevelChosen), the ship has arrived, and the
		// start button begins the level
		if (mBoard != NULL && mBoard->mVisible)
			ReturnToMap();
		mMapScreen->mLevel = aLevel;
		mMapScreen->OnLevelChosen();
		mMapScreen->mFlag224 = true;
		ButtonDepress(0x1cf3);
		Reply("OK level " + anArgs);
		return true;
	}

	if (aCommand == "unlock")
	{
		if (aFirst != "all")
		{
			ReplyError(theCommand, "unlock all");
			return true;
		}
		if (!EnsureUser(app, &anError))
		{
			ReplyError(theCommand, anError);
			return true;
		}
		// the game has no locked levels: the map is sailed, so what a profile holds is the typing lessons passed and the deepest
		// abyss depth (the lines of the "CONTINUE?" dialog)
		for (int i = 0; i < 19; i++)
			mUser->mField94[i] = 1;
		mUser->mFlag1dc = true;		// the opening tutorial (the two sharks of the first level) is seen
		mFlag8f8 = false;
		if (mUser->mBestDepth < 2400)
			mUser->mBestDepth = 2400;
		mProfileMgr->WriteUserProfileToRegistry(mUser, false, false);
		Reply("OK unlock all");
		return true;
	}

	if (aCommand == "set")
	{
		std::string aValue = aRest;
		int aNumber = 0;
		double aReal = atof(aValue.c_str());
		bool aIsNumber = ParseInt(aValue, &aNumber);
		if (aFirst == "hints")
		{
			if (mUser == NULL || !aIsNumber)
			{
				ReplyError(theCommand, "set hints 0|1, with a player");
				return true;
			}
			SetUserHints(aNumber != 0);
			Reply("OK set " + anArgs);
			return true;
		}
		if (aFirst == "tutorial")
		{
			// 0: the opening tutorial of the first level is to be played again, 1: it is seen
			if (mUser == NULL || !aIsNumber)
			{
				ReplyError(theCommand, "set tutorial 0|1, with a player");
				return true;
			}
			mUser->mFlag1dc = aNumber != 0;
			mFlag8f8 = (mUser->mFlag1dc == false);
			mProfileMgr->WriteUserProfileToRegistry(mUser, false, false);
			Reply("OK set " + anArgs);
			return true;
		}
		if (aFirst != "score" && aFirst != "lives" && aFirst != "timer" && aFirst != "depth" && aFirst != "emp")
		{
			ReplyError(theCommand, "can set score, lives, timer (milliseconds), depth, emp (0..1), hints and tutorial; enemy and the rest are read only");
			return true;
		}
		if (mBoard == NULL || mBoard->mPlayer == NULL || aValue.empty())
		{
			ReplyError(theCommand, "set " + aFirst + " <value>, in a game");
			return true;
		}
		if (aFirst == "score")
			mBoard->mPlayer->SetScore((int) aReal);
		else if (aFirst == "lives")
			mBoard->mPlayer->mField110 = (int) aReal;
		else if (aFirst == "timer")
			mBoard->mGameTimer->mStart = mBoard->mGameTimer->mUpdateCnt - (int) aReal - mBoard->mGameTimer->mPausedTotal;
		else if (aFirst == "depth")
			mBoard->SetDepth((int) aReal);
		else
			mBoard->mPlayer->mEmpCharge = aReal;
		Reply("OK set " + anArgs);
		return true;
	}

	if (aCommand == "win")
	{
		if (mBoard == NULL || !mBoard->mVisible || mLessonManager == NULL || mBoard->mPlayer == NULL)
		{
			ReplyError(theCommand, "no level is running");
			return true;
		}
		if (InAbyss(app))
		{
			ReplyError(theCommand, "the abyss has no level end (use lose, or set depth)");
			return true;
		}
		Board* b = mBoard;
		int aState = b->mPlayer->mState;
		if (aState == 8 || GetDialog(DIALOG_GAME_OVER) != NULL)
		{
			ReplyError(theCommand, "the game is over");
			return true;
		}
		if (aState == 0xc || aState == 7 || aState == 6)
		{
			ReplyError(theCommand, "the level is won already");
			return true;
		}
		// the level-won state, as Board::Update enters it when the diver is back at the bottom after the treasure
		b->mPlayer->SetFlag118(false);
		b->mPlayer->SetState(0xc);
		b->mEmpTimer->SetDisabled(true);
		b->mTimer0f00->SetDisabled(false);
		b->mTimer0f00->Reset();
		PlaySample(SOUND_WINGEM);
		b->mStats.mReserved[7] = b->mGameTimer->Elapsed();
		b->mPlayer->AddScore(b->GetLevelBonus());
		Reply("OK win");
		return true;
	}

	if (aCommand == "lose")
	{
		if (mBoard == NULL || !mBoard->mVisible || mBoard->mPlayer == NULL)
		{
			ReplyError(theCommand, "no game is running");
			return true;
		}
		if (mBoard->mPlayer->mState == 8 || GetDialog(DIALOG_GAME_OVER) != NULL)
		{
			ReplyError(theCommand, "the game is over");
			return true;
		}
		// the last life: the diver is eaten with none left, which opens the game-over dialog
		mBoard->mPlayer->mField110 = 0;
		mBoard->PlayerEaten();
		Reply("OK lose");
		return true;
	}

	return false;
}
