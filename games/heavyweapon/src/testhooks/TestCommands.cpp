// port: the standard test commands (docs/TEST_COMMANDS.md), reached only from a test's -script, -realinput or
// -commands through HandleScriptCommand. A command that is never given changes nothing and draws no random number.
// Each answers on stdout with one line: STATE {...}, MOVE {...}, OK <command> or ERROR <command>: <reason>. They act
// through the game's own code. memreport is the framework's.
//
//   state                  STATE {"screen","dialog","mode","level","maxlevel","score","lives","nukes","timer",
//                          "length","boss","bosshealth","bossup","enemies","tankx","tankdead","fire","paused","suspended",
//                          "updates","over","appupdates"}: dialog is the highest dialog id up (-1 for none), level is
//                          1-based, boss is -1 before the boss comes, else its place in the board's enemy list, bossup
//                          is 1 while the boss can be hit (in place and not dying), over is true while the game-over box
//                          is up
//   goto <screen>          title, menu, options, help, highscores, credits, campaign (a new campaign, its first map),
//                          survival (a new survival game), armory (the armory before the next level)
//   level <n>              campaign level n (1..19, the last the final boss's) now, with the player's upgrades, skipping the armory and the map
//   unlock all             the player has reached the campaign's end: survival and every level are open
//   set <key> <value>      score, lives, nukes, timer (updates played), level (1-based, for the next start), progress
//                          (1-based: the player's saved campaign stands before that level, as after passing the one
//                          before it, and is saved: the main menu's campaign then offers to continue it)
//   win                    the level's time jumps to the boss's warning, so its boss comes; with the boss up, it is killed
//                          (an error while the boss is on its way)
//   lose                   the tank is killed with no tank left (an error while no tank is on the field)
//   seed <n>               Rand() and the game's own generator seeded as at the start with -seed=n
//   move                   MOVE {"x","y","fire","target","input","updates"} from the bot (testhooks/Bot.cpp); between
//                          fights MOVE {"click","input","updates"}, the button that goes on (a message box's OK, a play
//                          tip's OK, the help's PLAY, the results, the armory's plus buttons until every point is spent
//                          and then its GO, the map's link); else MOVE null
//
// Two more for the identical-play comparison with the original (tools/idplay/idplay.py):
//   trace <file>|off       from the next update on, a line a update in <file>: the state the update before left, the values
//                          idplay reads from the original's memory (TraceFrame)
//   botrecord <file> [<period> <residue>...]
//                          the bot plays: its every move goes in as script input (move, click, down, up) at once, and each
//                          input goes into <file> as "<update> <command> <x> <y>", so the same input can be given to the
//                          original; it asks the bot again 4 updates after a move, 20 after a click and 10 when there is
//                          none, and with a period only at an update whose number modulo it is one of the residues (the
//                          next such update): the updates before which the original under origrun reads its input

#include "HeavyWeaponApp.h"
#include "Armory.h"
#include "ArmoryArrowButton.h"
#include "ArmoryWeaponPanel.h"
#include "Board.h"
#include "Bot.h"
#include "CreditsScreen.h"
#include "Enemy.h"
#include "HelpScreen.h"
#include "HighScoresScreen.h"
#include "MainMenu.h"
#include "MapScreen.h"
#include "MessageDialog.h"
#include "MTRand.h"
#include "OptionsDialog.h"
#include "PlayTipDialog.h"
#include "Projectile.h"
#include "ResultsScreen.h"
#include "BracketButton.h"
#include "DialogButton.h"
#include "HyperlinkWidget.h"
#include "TitleScreen.h"
#include "WidgetManager.h"
#include "Compat2004.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>

using namespace Sexy;

namespace
{

const int FIRST_LEVEL = 1;
const int LAST_LEVEL = HeavyWeaponApp::FINAL_LEVEL + 1;	// the final boss's level, 1-based
const int BOT_MOVE_UPDATES = 4;		// a bot's move holds for this many updates before the next is asked
const int BOT_SCREEN_UPDATES = 20;	// and a click that goes on from a screen between fights
const int BOT_IDLE_UPDATES = 10;	// botrecord: the wait when the bot has no move
const int TRACE_FLUSH_UPDATES = 256;	// the trace is written out this often
const int TRACED_ITEMS = 3;			// the trace gives the places of this many of each of the board's bullets, enemies and
									// projectiles (idplay.py, TRACED_ITEMS)

const int MAX_STEP_PERIOD = 32;		// botrecord's period: its residues are the bits of an unsigned int

FILE* sRecordFile = NULL;			// botrecord's file: each input it sent, by update
int sStepPeriod = 0;				// botrecord: with a period the bot is asked only at updates whose number modulo it is
unsigned int sStepResidues = 0;		// one of these (bits): the updates before which the original reads its input

void Reply(const std::string& theLine)
{
	printf("%s\n", theLine.c_str());
	fflush(stdout);
}

std::string CommandText(const ScriptCommand& theCommand)
{
	return theCommand.mCommand + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs);
}

void ReplyOK(const ScriptCommand& theCommand)
{
	Reply("OK " + CommandText(theCommand));
}

void ReplyError(const ScriptCommand& theCommand, const std::string& theReason)
{
	Reply("ERROR " + CommandText(theCommand) + ": " + theReason);
}

template <class T> T* FindWidget(HeavyWeaponApp* theApp)
{
	for (Widget* aWidget : theApp->mWidgetManager->mWidgets)
	{
		T* aFound = dynamic_cast<T*>(aWidget);
		if (aFound != NULL)
			return aFound;
	}
	return NULL;
}

// The game's board: a Board on the manager that is not the help screen's demonstration (HelpScreen is a Board)
Board* FindGameBoard(HeavyWeaponApp* theApp)
{
	for (Widget* aWidget : theApp->mWidgetManager->mWidgets)
	{
		Board* aBoard = dynamic_cast<Board*>(aWidget);
		if (aBoard != NULL && dynamic_cast<HelpScreen*>(aWidget) == NULL)
			return aBoard;
	}
	return NULL;
}

// The screen the player sees: the widgets on the manager, not the app's pointers (the original leaves some dangling)
std::string ScreenName(HeavyWeaponApp* theApp)
{
	Board* aBoard = FindGameBoard(theApp);
	if (FindWidget<HelpScreen>(theApp) != NULL)
		return "help";
	if (aBoard != NULL)
	{
		if (theApp->GetDialog(Board::DIALOG_GAME_OVER) != NULL)
			return "gameover";
		if (aBoard->mResultsScreen != NULL)
			return "results";
		return "game";
	}
	if (FindWidget<Armory>(theApp) != NULL)
		return "armory";
	if (FindWidget<MapScreen>(theApp) != NULL)
		return "map";
	if (FindWidget<HighScoresScreen>(theApp) != NULL)
		return "highscores";
	if (FindWidget<CreditsScreen>(theApp) != NULL)
		return "credits";
	if (FindWidget<MainMenu>(theApp) != NULL)
		return "menu";
	if (FindWidget<TitleScreen>(theApp) != NULL)
		return "title";
	return "none";
}

// A test folder has no player: the one the new-player box would add (UserNameDialog::Accept)
void EnsureUser(HeavyWeaponApp* theApp)
{
	theApp->KillDialog(HeavyWeaponApp::DIALOG_USER_NAME);
	if (!theApp->mUsers.empty())
		return;
	UserRecord aRecord;
	memset(&aRecord, 0, sizeof(aRecord));
	strcpy(aRecord.mName, "Test");
	theApp->mUsers.push_back(aRecord);
	theApp->mCurrentUser = 0;
	theApp->mIsNewUser = true;
	theApp->SaveUsers();
}

// Every screen before a game goes, as its buttons take it away. False while the game is still loading: the title's
// intro is ended (as a click ends it), which starts the loading, and the command is to be sent again.
bool LeaveScreens(HeavyWeaponApp* theApp)
{
	TitleScreen* aTitle = FindWidget<TitleScreen>(theApp);
	if (aTitle != NULL)
	{
		if (aTitle->mFlashWidget != NULL)
			aTitle->FlashAnimEnded(0);
		if (!theApp->mLoadingThreadCompleted)
			return false;
		// what UpdateFrames does once the loading is done, before the title can go
		if (!theApp->mLoadingCompleteShown)
		{
			theApp->mLoadingCompleteShown = true;
			aTitle->LoadingComplete();
		}
		aTitle->ButtonDepress(TitleScreen::BUTTON_PLAY);
	}
	EnsureUser(theApp);
	MainMenu* aMenu = FindWidget<MainMenu>(theApp);
	if (aMenu != NULL)
	{
		theApp->mWidgetManager->RemoveWidget(aMenu);
		theApp->SafeDeleteWidget(aMenu);
		theApp->mMainMenu = NULL;
	}
	HighScoresScreen* aScores = FindWidget<HighScoresScreen>(theApp);
	if (aScores != NULL)
	{
		theApp->mWidgetManager->RemoveWidget(aScores);
		theApp->SafeDeleteWidget(aScores);
	}
	CreditsScreen* aCredits = FindWidget<CreditsScreen>(theApp);
	if (aCredits != NULL)
	{
		theApp->mWidgetManager->RemoveWidget(aCredits);
		theApp->SafeDeleteWidget(aCredits);
	}
	// the help screen goes as its OK takes it away
	HelpScreen* aHelp = FindWidget<HelpScreen>(theApp);
	if (aHelp != NULL)
	{
		theApp->mWidgetManager->RemoveWidget(aHelp);
		theApp->SafeDeleteWidget(aHelp);
	}
	return true;
}

void ReplyState(HeavyWeaponApp* theApp)
{
	Board* aBoard = FindGameBoard(theApp);
	std::ostringstream aStream;
	aStream << "STATE {\"screen\":\"" << ScreenName(theApp) << "\"";
	int aDialog = -1;
	if (!theApp->mDialogMap.empty())
		aDialog = theApp->mDialogMap.rbegin()->first;
	aStream << ",\"dialog\":" << aDialog;
	bool isSurvival = aBoard != NULL && aBoard->mIsSurvival;
	aStream << ",\"mode\":\"" << (isSurvival ? "survival" : "campaign") << "\"";
	aStream << ",\"level\":" << theApp->mLevel + 1;
	int aMaxLevel = theApp->mUsers.empty() ? 0 : theApp->CurrentUser().mMaxLevel;
	aStream << ",\"maxlevel\":" << aMaxLevel;
	aStream << ",\"score\":" << theApp->mScore;
	aStream << ",\"lives\":" << theApp->mLives;
	aStream << ",\"nukes\":" << theApp->mPowerUps[HeavyWeaponApp::POWERUP_NUKES];
	if (aBoard != NULL)
	{
		int aBossIndex = -1;
		int anIndex = 0;
		for (ObjectList<Enemy>::Node* aNode = aBoard->mEnemies.mHead->mNext; aNode != NULL; aNode = aNode->mNext, anIndex++)
		{
			if (aNode->mItem == aBoard->mBoss)
				aBossIndex = anIndex;
		}
		aStream << ",\"timer\":" << aBoard->mLevelTime << ",\"length\":" << aBoard->mLevelLength;
		aStream << ",\"boss\":" << aBossIndex;
		aStream << ",\"bosshealth\":" << (aBoard->mBoss != NULL ? aBoard->mBoss->mHealth : 0.0);
		aStream << ",\"bossup\":" << ((aBoard->mBoss != NULL && aBoard->mBoss->mIsAlive && aBoard->mBossDeath == NULL) ? 1 : 0);
		aStream << ",\"enemies\":" << aBoard->mEnemies.mCount;
		aStream << ",\"tankx\":" << aBoard->mTankX;
		aStream << ",\"tankdead\":" << aBoard->mTankDeadTime;
		aStream << ",\"fire\":" << (aBoard->mMouseDown ? 1 : 0);
		aStream << ",\"paused\":" << (aBoard->mPaused ? 1 : 0);
		aStream << ",\"suspended\":" << (aBoard->mSuspended ? 1 : 0);
		aStream << ",\"updates\":" << aBoard->mUpdateCnt;
	}
	aStream << ",\"over\":" << (theApp->GetDialog(Board::DIALOG_GAME_OVER) != NULL ? "true" : "false");
	aStream << ",\"appupdates\":" << theApp->mUpdateCount << "}";
	Reply(aStream.str());
}

// The button a player clicks to go on from a screen between fights: a play tip's OK, the help's PLAY, the level's
// results, the armory's GO, the map's "invade" link; NULL when none is up and showing
Widget* OnwardButton(HeavyWeaponApp* theApp, Board* theBoard)
{
	PlayTipDialog* aTip = (PlayTipDialog*) theApp->GetDialog(PlayTipDialog::DIALOG_ID);
	MessageDialog* aMessage = (MessageDialog*) theApp->GetDialog(MessageDialog::DIALOG_ID);
	Widget* aButton = NULL;
	HelpScreen* aHelp = FindWidget<HelpScreen>(theApp);
	Armory* anArmory = FindWidget<Armory>(theApp);
	MapScreen* aMap = FindWidget<MapScreen>(theApp);
	if (aMessage != NULL)
		aButton = aMessage->mOkButton;
	else if (aTip != NULL)
		aButton = aTip->mOkButton;
	else if (aHelp != NULL)
		aButton = aHelp->mOkButton;
	else if (theBoard != NULL && theBoard->mResultsScreen != NULL)
		aButton = theBoard->mResultsScreen->mAdvanceButton;
	else if (anArmory != NULL)
	{
		// the armory's GO refuses while points are left, so the bot spends them first
		int anUpgrade = ChooseArmoryUpgrade(anArmory);
		aButton = (anUpgrade >= 0) ? (Widget*) anArmory->mPanels[anUpgrade]->mPlusButton : (Widget*) anArmory->mGoButton;
	}
	else if (aMap != NULL)
		aButton = aMap->mInvadeLink;
	if (aButton == NULL || !aButton->mVisible || aButton->mWidgetManager == NULL)
		return NULL;
	return aButton;
}

void ReplyMove(HeavyWeaponApp* theApp)
{
	Board* aBoard = FindGameBoard(theApp);
	Widget* aButton = OnwardButton(theApp, aBoard);
	if (aButton != NULL)
	{
		int x = aButton->mX + aButton->mWidth / 2;
		int y = aButton->mY + aButton->mHeight / 2;
		std::ostringstream aStream;
		aStream << "MOVE {\"click\":[" << x << "," << y << "],\"input\":[{\"move\":[" << x << "," << y
				<< "]},{\"click\":[" << x << "," << y << "]}],\"updates\":" << BOT_SCREEN_UPDATES << "}";
		Reply(aStream.str());
		return;
	}
	BotMove aMove;
	if (ScreenName(theApp) != "game" || !ChooseBotMove(aBoard, &aMove))
	{
		Reply("MOVE null");
		return;
	}
	std::ostringstream aStream;
	aStream << "MOVE {\"x\":" << aMove.mX << ",\"y\":" << aMove.mY << ",\"fire\":" << (aMove.mFire ? 1 : 0)
			<< ",\"target\":" << aMove.mTarget << ",\"input\":[{\"move\":[" << aMove.mX << "," << aMove.mY << "]}";
	if (aMove.mFire && !aBoard->mMouseDown)
		aStream << ",{\"press\":[" << aMove.mX << "," << aMove.mY << "]}";
	else if (!aMove.mFire && aBoard->mMouseDown)
		aStream << ",{\"release\":[" << aMove.mX << "," << aMove.mY << "]}";
	aStream << "],\"updates\":" << BOT_MOVE_UPDATES << "}";
	Reply(aStream.str());
}

// botrecord: whether the bot may be asked at theUpdate, and the first update from theUpdate on at which it may
bool IsBotStepUpdate(int theUpdate)
{
	return sStepPeriod == 0 || (sStepResidues & (1u << (theUpdate % sStepPeriod))) != 0;
}

int NextBotStepUpdate(int theUpdate)
{
	while (!IsBotStepUpdate(theUpdate))
		theUpdate++;
	return theUpdate;
}

// botrecord: one input now, through the script, and its line in the file
void RecordInput(HeavyWeaponApp* theApp, const char* theCommand, int x, int y)
{
	std::ostringstream anArgs;
	anArgs << x << " " << y;
	theApp->AddScriptCommand(theApp->mUpdateCount, theCommand, anArgs.str());
	fprintf(sRecordFile, "%d %s %s\n", theApp->mUpdateCount, theCommand, anArgs.str().c_str());
	fflush(sRecordFile);
}

// botrecord's step: the bot's move goes in as input; returns the updates until the bot is asked again
int RecordBotMove(HeavyWeaponApp* theApp)
{
	Board* aBoard = FindGameBoard(theApp);
	Widget* aButton = OnwardButton(theApp, aBoard);
	if (aButton != NULL)
	{
		int x = aButton->mX + aButton->mWidth / 2;
		int y = aButton->mY + aButton->mHeight / 2;
		RecordInput(theApp, "move", x, y);
		RecordInput(theApp, "click", x, y);
		return BOT_SCREEN_UPDATES;
	}
	BotMove aMove;
	if (ScreenName(theApp) != "game" || !ChooseBotMove(aBoard, &aMove))
		return BOT_IDLE_UPDATES;
	RecordInput(theApp, "move", aMove.mX, aMove.mY);
	if (aMove.mFire && !aBoard->mMouseDown)
		RecordInput(theApp, "down", aMove.mX, aMove.mY);
	else if (!aMove.mFire && aBoard->mMouseDown)
		RecordInput(theApp, "up", aMove.mX, aMove.mY);
	return BOT_MOVE_UPDATES;
}

// A double's two halves as the original's memory holds them: low word, then high word
void DoubleWords(double theValue, int* theLow, int* theHigh)
{
	uint64_t aBits;
	memcpy(&aBits, &theValue, sizeof(aBits));
	*theLow = (int)(uint32_t)aBits;
	*theHigh = (int)(uint32_t)(aBits >> 32);
}

// The trace's places of a board list's first TRACED_ITEMS items: " <prefix><n>x=<high word of mX> <prefix><n>y=...", 0 for
// the items the list does not have
template <class T> void WriteTracedItems(FILE* theFile, char thePrefix, ObjectList<T>& theList)
{
	typename ObjectList<T>::Node* aNode = theList.mHead->mNext;
	for (int i = 1; i <= TRACED_ITEMS; i++)
	{
		int aLow = 0, anXHigh = 0, aYHigh = 0;
		if (aNode != NULL)
		{
			DoubleWords(aNode->mItem->mX, &aLow, &anXHigh);
			DoubleWords(aNode->mItem->mY, &aLow, &aYHigh);
			aNode = aNode->mNext;
		}
		fprintf(theFile, " %c%dx=%d %c%dy=%d", thePrefix, i, anXHigh, thePrefix, i, aYHigh);
	}
}

bool ReadInt(const std::string& theText, int* theValue)
{
	char* anEnd = NULL;
	long aValue = strtol(theText.c_str(), &anEnd, 10);
	if (theText.empty() || anEnd == NULL || *anEnd != '\0')
		return false;
	*theValue = (int)aValue;
	return true;
}

}

// The trace's line for the update that just ended: what idplay.py reads from the original's memory after the same update,
// each 32-bit value as a signed int (the generators' indexes and first words, the board's values, 0 where there is none)
void HeavyWeaponApp::TraceFrame()
{
	uint32_t aState[MTRAND_N];
	int aRandIndex;
	GetRandMTState(aState, &aRandIndex);
	int aRandFirstWord = (int) aState[0];
	int aGameRandIndex;
	mRand.GetState(aState, &aGameRandIndex);
	int aGameRandFirstWord = (int) aState[0];
	int aLevelTime = 0, aTankXLow = 0, aTankXHigh = 0, aTankDeadTime = 0, aDestroyed = 0, aSpawned = 0, anEnemyCount = 0;
	int aBossHealthLow = 0, aBossHealthHigh = 0;
	int aMouseX = 0, aMouseY = 0, aMouseDown = 0, aBullets = 0, anExplosions = 0, aProjectiles = 0, aCraters = 0;
	int aTankShots = 0, aCasings = 0, aSparks = 0, aParticles = 0;
	if (mBoard != NULL)
	{
		aLevelTime = mBoard->mLevelTime;
		DoubleWords(mBoard->mTankX, &aTankXLow, &aTankXHigh);
		aTankDeadTime = mBoard->mTankDeadTime;
		aDestroyed = mBoard->mEnemiesDestroyed;
		aSpawned = mBoard->mEnemiesSpawned;
		anEnemyCount = mBoard->mEnemies.mCount;
		if (mBoard->mBoss != NULL)
			DoubleWords(mBoard->mBoss->mHealth, &aBossHealthLow, &aBossHealthHigh);
		aMouseX = mBoard->mMouseX;
		aMouseY = mBoard->mMouseY;
		aMouseDown = mBoard->mMouseDown ? 1 : 0;
		aBullets = mBoard->mBullets.mCount;
		anExplosions = mBoard->mExplosions.mCount;
		aProjectiles = mBoard->mProjectiles.mCount;
		aCraters = mBoard->mCraters.mCount;
		aTankShots = mBoard->mTankShots.mCount;
		aCasings = mBoard->mShellCasings.mCount;
		aSparks = mBoard->mSparks.mCount;
		aParticles = mBoard->mParticles.mParticles.mCount;
	}
	fprintf(mTraceFile, "u=%d loaded=%d fidx=%d f0=%d gidx=%d g0=%d board=%d score=%d level=%d lives=%d nukes=%d ltime=%d "
		"tankxl=%d tankxh=%d tankdead=%d killed=%d spawned=%d enemies=%d boss=%d bosshpl=%d bosshph=%d mousex=%d mousey=%d "
		"mousedown=%d bullets=%d explosions=%d projectiles=%d craters=%d tankshots=%d casings=%d sparks=%d particles=%d",
		mUpdateCount, mLoadingCompleteShown ? 1 : 0, aRandIndex, aRandFirstWord, aGameRandIndex, aGameRandFirstWord,
		mBoard != NULL ? 1 : 0, mScore, mLevel, mLives, mPowerUps[POWERUP_NUKES], aLevelTime, aTankXLow, aTankXHigh,
		aTankDeadTime, aDestroyed, aSpawned, anEnemyCount, (mBoard != NULL && mBoard->mBoss != NULL) ? 1 : 0, aBossHealthLow,
		aBossHealthHigh, aMouseX, aMouseY, aMouseDown, aBullets, anExplosions, aProjectiles, aCraters, aTankShots, aCasings,
		aSparks, aParticles);
	// the first few of the board's bullets, enemies and projectiles, each by the high words of its mX and mY
	if (mBoard != NULL)
	{
		WriteTracedItems(mTraceFile, 'b', mBoard->mBullets);
		WriteTracedItems(mTraceFile, 'e', mBoard->mEnemies);
		WriteTracedItems(mTraceFile, 'p', mBoard->mProjectiles);
	}
	else
	{
		for (char aPrefix : { 'b', 'e', 'p' })
		{
			for (int i = 1; i <= TRACED_ITEMS; i++)
				fprintf(mTraceFile, " %c%dx=0 %c%dy=0", aPrefix, i, aPrefix, i);
		}
	}
	fprintf(mTraceFile, "\n");
	if (mUpdateCount % TRACE_FLUSH_UPDATES == 0)
		fflush(mTraceFile);
}

bool HeavyWeaponApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	const std::string& aCommand = theCommand.mCommand;
	std::string anArgs = theCommand.mArgs;
	Board* aBoard = FindGameBoard(this);
	if (aCommand == "trace")
	{
		if (mTraceFile != NULL)
			fclose(mTraceFile);
		mTraceFile = NULL;
		if (anArgs != "off")
		{
			mTraceFile = fopen(anArgs.c_str(), "w");
			if (mTraceFile == NULL)
			{
				ReplyError(theCommand, "cannot write " + anArgs);
				return true;
			}
		}
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "botrecord")
	{
		// with a file it starts; without, it is the bot's next step, which each step schedules
		if (!anArgs.empty())
		{
			std::istringstream anArgStream(anArgs);
			std::string aFile;
			anArgStream >> aFile;
			int aPeriod = 0;
			unsigned int aResidues = 0;
			if (anArgStream >> aPeriod)
			{
				int aResidue;
				while (anArgStream >> aResidue)
				{
					if (aPeriod < 1 || aPeriod > MAX_STEP_PERIOD || aResidue < 0 || aResidue >= aPeriod)
						break;
					aResidues |= 1u << aResidue;
				}
				if (aResidues == 0 || !anArgStream.eof())
				{
					ReplyError(theCommand, "botrecord <file> [<period> <residue>...]");
					return true;
				}
			}
			if (sRecordFile != NULL)
				fclose(sRecordFile);
			sRecordFile = fopen(aFile.c_str(), "w");
			if (sRecordFile == NULL)
			{
				ReplyError(theCommand, "cannot write " + aFile);
				return true;
			}
			sStepPeriod = aPeriod;
			sStepResidues = aResidues;
			ReplyOK(theCommand);
		}
		else if (sRecordFile == NULL)
		{
			ReplyError(theCommand, "botrecord <file> starts it");
			return true;
		}
		if (!IsBotStepUpdate(mUpdateCount))
		{
			AddScriptCommand(NextBotStepUpdate(mUpdateCount), "botrecord", "");
			return true;
		}
		AddScriptCommand(NextBotStepUpdate(mUpdateCount + RecordBotMove(this)), "botrecord", "");
		return true;
	}

	if (aCommand == "state")
	{
		ReplyState(this);
		return true;
	}
	if (aCommand == "move")
	{
		ReplyMove(this);
		return true;
	}
	if (aCommand == "seed")
	{
		int aSeed;
		if (!ReadInt(anArgs, &aSeed))
		{
			ReplyError(theCommand, "a number is needed");
			return true;
		}
		// as Init and WinMain seed them (HeavyWeaponApp::Init, Start)
		mRandSeed = aSeed;
		CrtSRand((unsigned int) aSeed);
		MTRand aRand;
		SeedMTRand2004(aRand, (unsigned long) (unsigned int) aSeed);
		uint32_t aState[MTRAND_N];
		int anIndex;
		aRand.GetState(aState, &anIndex);
		SetRandMTState(aState, anIndex);
		SeedMTRand2004(mRand, (unsigned long) (unsigned int) Rand());
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "goto")
	{
		if (anArgs == "title")
		{
			ReplyError(theCommand, "the title screen only comes up when the game starts");
			return true;
		}
		if (aBoard != NULL)
		{
			ReplyError(theCommand, "a game is on: lose or win it first");
			return true;
		}
		if (anArgs != "options" && !LeaveScreens(this))
		{
			ReplyError(theCommand, "still loading: send it again");
			return true;
		}
		if (anArgs == "menu")
		{
			ShowMainMenu();
			KillDialog(DIALOG_USER_NAME);
		}
		else if (anArgs == "options")
			AddDialog(DIALOG_OPTIONS, new OptionsDialog(this));
		else if (anArgs == "help")
		{
			ShowHelp(static_cast<ButtonListener*>(NULL));
		}
		else if (anArgs == "highscores")
		{
			const int HIGH_SCORES_LIST_SIZE = 10; // MainMenu's
			mWidgetManager->AddWidget(new HighScoresScreen(this, false, HIGH_SCORES_LIST_SIZE));
		}
		else if (anArgs == "credits")
		{
			ShowCredits();
		}
		else if (anArgs == "campaign")
		{
			NewGame(false);
		}
		else if (anArgs == "survival")
		{
			NewGame(true);
		}
		else if (anArgs == "armory")
		{
			if (FindWidget<MapScreen>(this) != NULL)
			{
				MapScreen* aMap = FindWidget<MapScreen>(this);
				mWidgetManager->RemoveWidget(aMap);
				SafeDeleteWidget(aMap);
				mMapScreen = NULL;
			}
			LoadWaves(false);
			ShowArmory();
		}
		else
		{
			ReplyError(theCommand, "unknown screen");
			return true;
		}
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "level")
	{
		int aLevel;
		if (!ReadInt(anArgs, &aLevel) || aLevel < FIRST_LEVEL || aLevel > LAST_LEVEL)
		{
			ReplyError(theCommand, "a level from 1 to 19 is needed");
			return true;
		}
		if (aBoard != NULL)
		{
			ReplyError(theCommand, "a game is on: lose or win it first");
			return true;
		}
		if (!LeaveScreens(this))
		{
			ReplyError(theCommand, "still loading: send it again");
			return true;
		}
		Armory* anArmory = FindWidget<Armory>(this);
		if (anArmory != NULL)
		{
			mWidgetManager->RemoveWidget(anArmory);
			SafeDeleteWidget(anArmory);
		}
		MapScreen* aMap = FindWidget<MapScreen>(this);
		if (aMap != NULL)
		{
			mWidgetManager->RemoveWidget(aMap);
			SafeDeleteWidget(aMap);
		}
		mMapScreen = NULL;
		mLevel = aLevel - 1;
		LoadWaves(false);
		StartLevel();
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
		EnsureUser(this);
		CurrentUser().mMaxLevel = NUM_LEVELS_PLUS_ONE;
		SaveUsers();
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "set")
	{
		std::string aKey = anArgs.substr(0, anArgs.find(' '));
		std::string aText = anArgs.find(' ') == std::string::npos ? "" : anArgs.substr(anArgs.find(' ') + 1);
		int aValue;
		if (!ReadInt(aText, &aValue))
		{
			ReplyError(theCommand, "set <key> <number>");
			return true;
		}
		if (aKey == "score")
			mScore = aValue;
		else if (aKey == "lives")
			mLives = aValue;
		else if (aKey == "nukes")
			mPowerUps[POWERUP_NUKES] = aValue;
		else if (aKey == "level")
			mLevel = aValue - 1;
		else if (aKey == "progress")
		{
			if (aValue < FIRST_LEVEL || aValue > LAST_LEVEL)
			{
				ReplyError(theCommand, "a level from 1 to 19 is needed");
				return true;
			}
			EnsureUser(this);
			UserRecord& aUser = CurrentUser();
			aUser.mLevel = (unsigned short)(aValue - 1);
			if ((int)aUser.mMaxLevel < aValue - 1)
				aUser.mMaxLevel = (unsigned short)(aValue - 1);
			SaveUsers();
		}
		else if (aKey == "timer")
		{
			if (aBoard == NULL)
			{
				ReplyError(theCommand, "no game is on");
				return true;
			}
			aBoard->mLevelTime = aValue;
		}
		else
		{
			ReplyError(theCommand, "unknown key");
			return true;
		}
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "win")
	{
		if (aBoard == NULL || aBoard->mIsSurvival)
		{
			ReplyError(theCommand, "no campaign level is on");
			return true;
		}
		// before the boss, the level's time jumps to where the board puts up the boss's warning (it waits there until the
		// field is clear of enemy shots), and the boss comes as at the end of the waves
		if (aBoard->mBoss == NULL && aBoard->mBossWarning == NULL &&
			aBoard->mLevelTime < aBoard->mLevelLength - Board::BOSS_WARNING_TIME)
			aBoard->mLevelTime = aBoard->mLevelLength - Board::BOSS_WARNING_TIME;
		else if (aBoard->mBoss == NULL)
		{
			ReplyError(theCommand, "the boss is on its way: send it again once it is up");
			return true;
		}
		else if (aBoard->mBoss->mIsAlive && aBoard->mBossDeath == NULL)
		{
			// the board's collision pass kills an enemy whose health is below 0 at its next hit; a boss's own
			// Killed starts its death
			aBoard->mBoss->mHealth = -1.0;
			aBoard->mBoss->Killed();
		}
		else
		{
			ReplyError(theCommand, aBoard->mBossDeath != NULL ? "the boss is already dying" :
				"the boss is still coming in: send it again once it can be hit");
			return true;
		}
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "lose")
	{
		if (aBoard == NULL)
		{
			ReplyError(theCommand, "no game is on");
			return true;
		}
		if (aBoard->mTankDeadTime != 0)
		{
			ReplyError(theCommand, "the tank is not on the field");
			return true;
		}
		// no tank left after this one: the board ends the game when the tank's wait after its loss runs out
		mLives = 0;
		aBoard->KillTank();
		ReplyOK(theCommand);
		return true;
	}
	return false;
}
