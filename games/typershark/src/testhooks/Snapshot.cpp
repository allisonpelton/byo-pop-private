// port: test hooks, the read side: the `state` command's JSON, the snapshot of the board the bot looks at, and the walk over
// the board's creature lists that both use. Nothing here changes the game or draws a random number.

#include "TestHooks.h"
#include "WinWSApp.h"
#include "Board.h"
#include "Enemy.h"
#include "Player.h"
#include "Clam.h"
#include "BonusCreature.h"
#include "Projectile.h"
#include "ProjectileBoss.h"
#include "LessonManager.h"
#include "MapScreen.h"
#include "XWidget.h"
#include "TypingTutor.h"
#include "StatsScreen.h"
#include "HighScoreScreen.h"
#include "TitleScreen.h"
#include "MenuScreen.h"
#include "UserProfile.h"
#include "WSTimer.h"
#include "Dialog.h"
#include "DialogButton.h"
#include "WidgetManager.h"
#include "Common.h"

#include <cstdio>
#include <cstring>
#include <typeinfo>

using namespace Sexy;

namespace Sexy
{

// the topmost first
const TestDialog gTestDialogs[] = {
	{ DIALOG_CREDITS, "credits" },
	{ DIALOG_GAME_OVER, "gameover" },
	{ DIALOG_WIN_GAME, "wingame" },
	{ DIALOG_ABYSS_MODE, "abyssmode" },
	{ DIALOG_DIFFICULTY, "difficulty" },
	{ DIALOG_OPTIONS, "options" },
	{ DIALOG_NEW_USER, "newuser" },
	{ DIALOG_SELECT_USER, "selectuser" },
	{ DIALOG_DELETE_USER, "deleteuser" },
	{ 0x30b, "nowindowed" },
	{ 0x3f3, "hint" },
	{ DIALOG_MESSAGE, "message" }
};
const int gNumTestDialogs = (int) (sizeof(gTestDialogs) / sizeof(gTestDialogs[0]));

}  // namespace Sexy

static Dialog* OpenDialog(WinWSApp* theApp, std::string* theName)
{
	for (int i = 0; i < gNumTestDialogs; i++)
	{
		Dialog* aDialog = theApp->GetDialog(gTestDialogs[i].mId);
		if (aDialog != NULL)
		{
			if (theName != NULL)
				*theName = gTestDialogs[i].mName;
			return aDialog;
		}
	}
	return NULL;
}

std::string Sexy::TestOpenDialog(WinWSApp* theApp)
{
	std::string aName;
	OpenDialog(theApp, &aName);
	return aName;
}

std::string Sexy::TestScreen(WinWSApp* theApp, std::string* theBase)
{
	std::string aBase = "none";
	if (theApp->mStatsScreen != NULL)
		aBase = "stats";
	else if (theApp->mHighScoreScreen != NULL)
		aBase = "highscores";
	else if (theApp->mTypingTutor != NULL)
		aBase = "tutor";
	else if (theApp->mBoard != NULL && theApp->mBoard->mVisible)
		aBase = "board";
	else if (theApp->mMapScreen != NULL && theApp->mMapScreen->mVisible)
		aBase = "map";
	else if (theApp->mMenuScreen != NULL)
		aBase = "menu";
	else if (theApp->mTitleScreen != NULL)
		aBase = "title";
	if (theBase != NULL)
		*theBase = aBase;

	std::string aDialog = TestOpenDialog(theApp);
	return aDialog.empty() ? aBase : aDialog;
}

// the class name of an object from its type_info name (Itanium "N4Sexy9RedSharkE", plain "9RedShark" or MSVC "class Sexy::RedShark")
static std::string ClassName(const char* theName)
{
	const char* aCursor = theName;
	std::string aLast;
	if (*aCursor == 'N' || (*aCursor >= '0' && *aCursor <= '9'))
	{
		if (*aCursor == 'N')
			aCursor++;
		while (*aCursor >= '0' && *aCursor <= '9')
		{
			int aLength = 0;
			while (*aCursor >= '0' && *aCursor <= '9')
				aLength = aLength * 10 + (*aCursor++ - '0');
			aLast.assign(aCursor, strnlen(aCursor, (size_t) aLength));
			aCursor += aLast.size();
		}
		return aLast;
	}
	const char* aColons = strrchr(theName, ':');
	const char* aSpace = strrchr(theName, ' ');
	const char* aStart = aColons != NULL ? aColons + 1 : (aSpace != NULL ? aSpace + 1 : theName);
	return aStart;
}

static BotCreature MakeCreature(Board* theBoard, Enemy* theEnemy, const char* theList)
{
	BotCreature aCreature;
	aCreature.mKind = ClassName(typeid(*theEnemy).name());
	aCreature.mList = theList;
	aCreature.mWord = theEnemy->mWord;
	aCreature.mIsTarget = theBoard->mTarget == theEnemy;
	aCreature.mTyped = aCreature.mIsTarget ? (int) theBoard->mTyped.size() : theEnemy->mNumTyped;
	aCreature.mX = theEnemy->mX;
	aCreature.mY = theEnemy->mY;
	aCreature.mState = theEnemy->mState;
	aCreature.mVisible = theEnemy->mVisible;
	aCreature.mBlocked = theEnemy->mStealthy;
	return aCreature;
}

std::vector<BotCreature> Sexy::TestCollectCreatures(Board* theBoard)
{
	std::vector<BotCreature> aList;
	for (size_t i = 0; i < theBoard->mSharks.size(); i++)
		aList.push_back(MakeCreature(theBoard, theBoard->mSharks[i], "sharks"));
	for (size_t i = 0; i < theBoard->mPiranhas.size(); i++)
		aList.push_back(MakeCreature(theBoard, theBoard->mPiranhas[i], "piranhas"));
	for (size_t i = 0; i < theBoard->mClams.size(); i++)
	{
		BotCreature c = MakeCreature(theBoard, theBoard->mClams[i], "clams");
		Clam* aClam = dynamic_cast<Clam*>(theBoard->mClams[i]);
		c.mType = aClam != NULL ? aClam->mType : -1;
		c.mIsTarget = theBoard->mOpenClam == theBoard->mClams[i];
		aList.push_back(c);
	}
	for (size_t i = 0; i < theBoard->mBonusCreatures.size(); i++)
		aList.push_back(MakeCreature(theBoard, theBoard->mBonusCreatures[i], "bonus"));
	if (theBoard->mBoss != NULL)
	{
		aList.push_back(MakeCreature(theBoard, theBoard->mBoss, "boss"));
		ProjectileBoss* aBoss = dynamic_cast<ProjectileBoss*>(theBoard->mBoss);
		if (aBoss != NULL)
		{
			aList.back().mType = aBoss->mHitPoints;		// a boss's hit points
			for (size_t i = 0; i < aBoss->mShots.size(); i++)
				aList.push_back(MakeCreature(theBoard, aBoss->mShots[i], "shots"));
		}
	}
	return aList;
}

// the centre of a dialog's button in game pixels
static bool ButtonCentre(Dialog* theDialog, DialogButton* theButton, int* theX, int* theY)
{
	if (theButton == NULL || !theButton->mVisible)
		return false;
	int aX = theButton->mX + theButton->mWidth / 2;
	int aY = theButton->mY + theButton->mHeight / 2;
	if (theButton->mParent == theDialog)
	{
		aX += theDialog->mX;
		aY += theDialog->mY;
	}
	*theX = aX;
	*theY = aY;
	return true;
}

BoardSnapshot Sexy::TestTakeSnapshot(WinWSApp* app)
{
	BoardSnapshot aSnapshot;
	aSnapshot.mWidth = app->mWidth;
	aSnapshot.mHeight = app->mHeight;

	// a dialog over everything: the obvious step is the OK of a one-button message or hint box
	std::string aDialogName;
	Dialog* aDialog = OpenDialog(app, &aDialogName);
	if (aDialog != NULL)
	{
		int aX, aY;
		if ((aDialogName == "message" || aDialogName == "hint" || aDialogName == "credits" || aDialogName == "nowindowed") &&
			aDialog->mNoButton == NULL && ButtonCentre(aDialog, aDialog->mYesButton, &aX, &aY))
		{
			aSnapshot.mPromptWhat = "dialog";
			aSnapshot.mPrompt.push_back(BotStep::Click(aX, aY));
			aSnapshot.mPrompt.push_back(BotStep::Wait(1));
		}
		return aSnapshot;
	}

	Board* aBoard = app->mBoard;
	if (aBoard != NULL && aBoard->mVisible)
	{
		if (aBoard->mPaused || app->mLessonManager == NULL)
			return aSnapshot;
		int aState = aBoard->mPlayer->mState;
		bool aBonusLevel = app->mLessonManager->IsBonusLevel();
		if (aState == 7 || (aState == 6 && aBonusLevel))
		{
			// the level is won: the space bar goes back to the map
			aSnapshot.mPromptWhat = "continue";
			aSnapshot.mPrompt.push_back(BotStep::Key(" "));
			aSnapshot.mPrompt.push_back(BotStep::Wait(1));
			return aSnapshot;
		}
		if (aState != 0 && aState != 5 && aState != 4 && aState != 9)
			return aSnapshot;
		if (aState == 4 && aBoard->mScreenClear)
			return aSnapshot;

		std::vector<BotCreature> aAll = TestCollectCreatures(aBoard);
		if (aBonusLevel)
		{
			if (aState != 5)
				return aSnapshot;
			aSnapshot.mPhase = BoardSnapshot::PHASE_CLAMS;
			for (size_t i = 0; i < aAll.size(); i++)
				if (aAll[i].mList == "clams")
					aSnapshot.mClams.push_back(aAll[i]);
			return aSnapshot;
		}
		if (aState == 5)
		{
			aSnapshot.mPhase = BoardSnapshot::PHASE_STREAK;
			aSnapshot.mStreakText = aBoard->mBonusWord;
			aSnapshot.mStreakPos = aBoard->mBonusWordPos;
			return aSnapshot;
		}
		if (aBoard->mScreenClear && aBoard->mBoss == NULL)
			return aSnapshot;
		if (aBoard->mBoss != NULL)
		{
			ProjectileBoss* aBoss = dynamic_cast<ProjectileBoss*>(aBoard->mBoss);
			if (aBoss != NULL && aBoss->mHitPoints == 0)
				return aSnapshot;
		}

		// the creatures in the order KeyChar tries them: sharks, piranhas, bonus creatures, boss shots
		static const char* const aOrder[] = { "sharks", "piranhas", "bonus", "shots" };
		for (int aList = 0; aList < 4; aList++)
			for (size_t i = 0; i < aAll.size(); i++)
				if (aAll[i].mList == aOrder[aList])
				{
					if (aAll[i].mIsTarget)
						aSnapshot.mTarget = (int) aSnapshot.mCreatures.size();
					aSnapshot.mCreatures.push_back(aAll[i]);
				}
		aSnapshot.mTypedCount = (int) aBoard->mTyped.size();
		aSnapshot.mPhase = BoardSnapshot::PHASE_WORDS;
		return aSnapshot;
	}

	MapScreen* aMap = app->mMapScreen;
	if (aMap != NULL && aMap->mVisible && !aMap->mDisabled)
	{
		if (aMap->mLevelChosen)
		{
			aSnapshot.mPromptWhat = "map";
			aSnapshot.mPrompt.push_back(BotStep::Key(" "));
			aSnapshot.mPrompt.push_back(BotStep::Wait(1));
		}
		else if (!aMap->mFinalScrollStarted && !aMap->mChests.empty())
		{
			// a fork: the chest's letter picks the way
			aSnapshot.mPromptWhat = "map";
			aSnapshot.mPrompt.push_back(BotStep::Key(std::string(1, aMap->mChests[0]->mLetter)));
			aSnapshot.mPrompt.push_back(BotStep::Wait(1));
		}
		return aSnapshot;
	}

	// the typing tutor: the rest of the line that is being typed
	TypingTutor* aTutor = app->mTypingTutor;
	if (aTutor != NULL && app->mStatsScreen == NULL && app->mHighScoreScreen == NULL && aTutor->mVisible &&
		aTutor->mState != TypingTutor::STATE_FINISHED && aTutor->mLesson != NULL && aTutor->mTask < (int) aTutor->mLesson->mTasks.size())
	{
		TutorExercise* anExercise = aTutor->mLesson->mTasks[aTutor->mTask];
		if (anExercise->mCurText < (int) anExercise->mTexts.size())
		{
			aSnapshot.mPhase = BoardSnapshot::PHASE_STREAK;
			aSnapshot.mPromptWhat = "tutor";
			aSnapshot.mStreakText = anExercise->mTexts[anExercise->mCurText];
			aSnapshot.mStreakPos = anExercise->mCurChar;
		}
	}
	return aSnapshot;
}

static void AddInt(std::string& theJson, const char* theKey, int theValue)
{
	char aBuffer[64];
	snprintf(aBuffer, sizeof(aBuffer), ",\"%s\":%d", theKey, theValue);
	theJson += aBuffer;
}

static void AddBool(std::string& theJson, const char* theKey, bool theValue)
{
	theJson += StrFormat(",\"%s\":%s", theKey, theValue ? "true" : "false");
}

static void AddDouble(std::string& theJson, const char* theKey, double theValue)
{
	theJson += StrFormat(",\"%s\":%.3f", theKey, theValue);
}

static void AddString(std::string& theJson, const char* theKey, const std::string& theValue)
{
	theJson += StrFormat(",\"%s\":", theKey) + JsonQuote(theValue);
}

std::string Sexy::TestStateJson(WinWSApp* app)
{
	std::string aBase;
	std::string aScreen = TestScreen(app, &aBase);
	std::string j = "{\"screen\":" + JsonQuote(aScreen);
	if (aScreen != aBase)
		AddString(j, "base", aBase);

	Dialog* aDialog = OpenDialog(app, NULL);
	if (aDialog != NULL)
	{
		AddString(j, "dialogHeader", aDialog->mDialogHeader);
		AddString(j, "dialogLines", aDialog->mDialogLines);
	}

	if (app->mUser != NULL)
		AddString(j, "user", app->mUser->mName);

	Board* aBoard = app->mBoard;
	LessonManager* aLessons = app->mLessonManager;
	TypingTutor* aTutor = app->mTypingTutor;
	bool aAbyss = aLessons != NULL && aLessons->mAbyssMode;
	if (aTutor != NULL)
		AddString(j, "mode", "tutor");
	else if (aLessons != NULL)
		AddString(j, "mode", aAbyss ? "abyss" : "adventure");
	if (aLessons != NULL && aTutor == NULL)
	{
		AddInt(j, "difficulty", app->mDifficulty);
		AddInt(j, "level", aLessons->mLevel);
		if (aLessons->mLevel == -1)
			AddInt(j, "endless", aLessons->mEndlessCount);
		AddInt(j, "wave", aLessons->mWave);
	}

	bool aOver = app->GetDialog(DIALOG_GAME_OVER) != NULL || app->GetDialog(DIALOG_WIN_GAME) != NULL;

	if (aBoard != NULL)
	{
		Player* p = aBoard->mPlayer;
		AddInt(j, "score", p->mScore);
		AddInt(j, "lives", p->mLives);
		AddInt(j, "depth", aBoard->mScore);
		AddInt(j, "timer", aBoard->mGameTimer->Elapsed());
		AddInt(j, "typeTime", aBoard->mTypeTime);
		AddInt(j, "diver", p->mState);
		AddDouble(j, "emp", p->mEmpCharge);
		AddBool(j, "paused", aBoard->mPaused);
		AddBool(j, "won", p->mState == 7 || p->mState == 0xc || (p->mState == 6 && aLessons != NULL && aLessons->IsBonusLevel()));
		int aEnemies = (int) (aBoard->mSharks.size() + aBoard->mPiranhas.size() + aBoard->mClams.size() + aBoard->mBonusCreatures.size());
		if (aBoard->mBoss != NULL)
			aEnemies++;
		AddInt(j, "enemy", aEnemies);

		j += ",\"board\":{";
		j += "\"width\":" + StrFormat("%d", app->mWidth);
		AddString(j, "typed", aBoard->mTyped);
		AddBool(j, "clear", aBoard->mScreenClear);		// no wave on the screen (the next one is due, or the keys are ignored)
		AddDouble(j, "divePos", aBoard->mDivePos);
		AddDouble(j, "floorY", aBoard->mOceanFloorY);
		AddInt(j, "oceanDepth", aBoard->mOceanDepth);
		AddBool(j, "canType", TestTakeSnapshot(app).mPhase != BoardSnapshot::PHASE_NONE);
		if (aBoard->mBannerLife > 0 && !aBoard->mBannerText.empty())
			AddString(j, "banner", aBoard->mBannerText);
		j += ",\"creatures\":[";
		std::vector<BotCreature> aAll = TestCollectCreatures(aBoard);
		for (size_t i = 0; i < aAll.size(); i++)
		{
			const BotCreature& aCreature = aAll[i];
			if (i > 0)
				j += ",";
			j += "{\"kind\":" + JsonQuote(aCreature.mKind);
			AddString(j, "list", aCreature.mList);
			AddString(j, "word", aCreature.mWord);
			AddInt(j, "typed", aCreature.mTyped);
			AddDouble(j, "x", aCreature.mX);
			AddDouble(j, "y", aCreature.mY);
			AddInt(j, "state", aCreature.mState);
			AddBool(j, "target", aCreature.mIsTarget);
			if (aCreature.mList == "clams")
				AddInt(j, "type", aCreature.mType);
			if (aCreature.mList == "boss")
				AddInt(j, "hp", aCreature.mType);
			j += "}";
		}
		j += "]}";
	}
	else if (aTutor != NULL)
	{
		AddInt(j, "level", aTutor->mLessons != NULL ? aTutor->mLessons->mCurrent : -1);
		AddInt(j, "enemy", (int) aTutor->mCreatures.size());
		j += ",\"tutor\":{";
		j += StrFormat("\"state\":%d,\"task\":%d,\"errors\":%d,\"correct\":%d,\"wpm\":%d,\"adjustedWpm\":%d,\"accuracy\":%d,\"timeLeft\":%d",
			aTutor->mState, aTutor->mTask, aTutor->mErrors, aTutor->mCorrectKeys, aTutor->mRawWPM, aTutor->mAdjustedWPM,
			aTutor->mAccuracy, aTutor->mTimeLeft);
		AddString(j, "expected", std::string(1, aTutor->mExpectedChar));
		j += "}";
		aOver = aTutor->mState == TypingTutor::STATE_FINISHED;
	}

	MapScreen* aMap = app->mMapScreen;
	if (aMap != NULL && aMap->mVisible)
	{
		j += StrFormat(",\"map\":{\"chosen\":%d,\"arrived\":%s,\"final\":%s,\"chests\":\"", aMap->mLevel, aMap->mLevelChosen ? "true" : "false",
			aMap->mFinal ? "true" : "false");
		for (size_t i = 0; i < aMap->mChests.size(); i++)
			j += aMap->mChests[i]->mLetter;
		j += "\"}";
	}

	AddBool(j, "over", aOver);
	return j + "}";
}
