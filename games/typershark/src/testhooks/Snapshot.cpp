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
	const char* p = theName;
	std::string aLast;
	if (*p == 'N' || (*p >= '0' && *p <= '9'))
	{
		if (*p == 'N')
			p++;
		while (*p >= '0' && *p <= '9')
		{
			int aLength = 0;
			while (*p >= '0' && *p <= '9')
				aLength = aLength * 10 + (*p++ - '0');
			aLast.assign(p, strnlen(p, (size_t) aLength));
			p += aLast.size();
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
	BotCreature c;
	c.mKind = ClassName(typeid(*theEnemy).name());
	c.mList = theList;
	c.mWord = theEnemy->mWord;
	c.mIsTarget = theBoard->mTarget == theEnemy;
	c.mTyped = c.mIsTarget ? (int) theBoard->mTyped.size() : theEnemy->mNumTyped;
	c.mX = theEnemy->mX;
	c.mY = theEnemy->mY;
	c.mState = theEnemy->mState;
	c.mVisible = theEnemy->mVisible;
	c.mBlocked = theEnemy->mFlag19;
	return c;
}

std::vector<BotCreature> Sexy::TestCollectCreatures(Board* b)
{
	std::vector<BotCreature> aList;
	for (size_t i = 0; i < b->mSharks.size(); i++)
		aList.push_back(MakeCreature(b, b->mSharks[i], "sharks"));
	for (size_t i = 0; i < b->mPiranhas.size(); i++)
		aList.push_back(MakeCreature(b, b->mPiranhas[i], "piranhas"));
	for (size_t i = 0; i < b->mClams.size(); i++)
	{
		BotCreature c = MakeCreature(b, b->mClams[i], "clams");
		Clam* aClam = dynamic_cast<Clam*>(b->mClams[i]);
		c.mType = aClam != NULL ? aClam->mType : -1;
		c.mIsTarget = b->mOpenClam == b->mClams[i];
		aList.push_back(c);
	}
	for (size_t i = 0; i < b->mBonusCreatures.size(); i++)
		aList.push_back(MakeCreature(b, b->mBonusCreatures[i], "bonus"));
	if (b->mBoss != NULL)
	{
		aList.push_back(MakeCreature(b, b->mBoss, "boss"));
		ProjectileBoss* aBoss = dynamic_cast<ProjectileBoss*>(b->mBoss);
		if (aBoss != NULL)
		{
			aList.back().mType = aBoss->mHitPoints;		// a boss's hit points
			for (size_t i = 0; i < aBoss->mShots.size(); i++)
				aList.push_back(MakeCreature(b, aBoss->mShots[i], "shots"));
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
	BoardSnapshot s;
	s.mWidth = app->mWidth;
	s.mHeight = app->mHeight;

	// a dialog over everything: the obvious step is the OK of a one-button message or hint box
	std::string aDialogName;
	Dialog* aDialog = OpenDialog(app, &aDialogName);
	if (aDialog != NULL)
	{
		int aX, aY;
		if ((aDialogName == "message" || aDialogName == "hint" || aDialogName == "credits" || aDialogName == "nowindowed") &&
			aDialog->mNoButton == NULL && ButtonCentre(aDialog, aDialog->mYesButton, &aX, &aY))
		{
			s.mPromptWhat = "dialog";
			s.mPrompt.push_back(BotStep::Click(aX, aY));
			s.mPrompt.push_back(BotStep::Wait(1));
		}
		return s;
	}

	Board* b = app->mBoard;
	if (b != NULL && b->mVisible)
	{
		if (b->mPaused || app->mLessonManager == NULL)
			return s;
		int aState = b->mPlayer->mState;
		bool aBonusLevel = app->mLessonManager->IsBonusLevel();
		if (aState == 7 || (aState == 6 && aBonusLevel))
		{
			// the level is won: the space bar goes back to the map
			s.mPromptWhat = "continue";
			s.mPrompt.push_back(BotStep::Key(" "));
			s.mPrompt.push_back(BotStep::Wait(1));
			return s;
		}
		if (aState != 0 && aState != 5 && aState != 4 && aState != 9)
			return s;
		if (aState == 4 && b->mFlag0f80)
			return s;

		std::vector<BotCreature> aAll = TestCollectCreatures(b);
		if (aBonusLevel)
		{
			if (aState != 5)
				return s;
			s.mPhase = BoardSnapshot::PHASE_CLAMS;
			for (size_t i = 0; i < aAll.size(); i++)
				if (aAll[i].mList == "clams")
					s.mClams.push_back(aAll[i]);
			return s;
		}
		if (aState == 5)
		{
			s.mPhase = BoardSnapshot::PHASE_STREAK;
			s.mStreakText = b->mString0f10;
			s.mStreakPos = b->mField0f2c;
			return s;
		}
		if (b->mFlag0f80 && b->mBoss == NULL)
			return s;
		if (b->mBoss != NULL)
		{
			ProjectileBoss* aBoss = dynamic_cast<ProjectileBoss*>(b->mBoss);
			if (aBoss != NULL && aBoss->mHitPoints == 0)
				return s;
		}

		// the creatures in the order KeyChar tries them: sharks, piranhas, bonus creatures, boss shots
		static const char* const aOrder[] = { "sharks", "piranhas", "bonus", "shots" };
		for (int aList = 0; aList < 4; aList++)
			for (size_t i = 0; i < aAll.size(); i++)
				if (aAll[i].mList == aOrder[aList])
				{
					if (aAll[i].mIsTarget)
						s.mTarget = (int) s.mCreatures.size();
					s.mCreatures.push_back(aAll[i]);
				}
		s.mTypedCount = (int) b->mTyped.size();
		s.mPhase = BoardSnapshot::PHASE_WORDS;
		return s;
	}

	MapScreen* aMap = app->mMapScreen;
	if (aMap != NULL && aMap->mVisible && !aMap->mDisabled)
	{
		if (aMap->mFlag224)
		{
			s.mPromptWhat = "map";
			s.mPrompt.push_back(BotStep::Key(" "));
			s.mPrompt.push_back(BotStep::Wait(1));
		}
		else if (!aMap->mFlag234 && !aMap->mVector1b4.empty())
		{
			// a fork: the chest's letter picks the way
			s.mPromptWhat = "map";
			s.mPrompt.push_back(BotStep::Key(std::string(1, aMap->mVector1b4[0]->mLetter)));
			s.mPrompt.push_back(BotStep::Wait(1));
		}
		return s;
	}

	// the typing tutor: the rest of the line that is being typed
	TypingTutor* aTutor = app->mTypingTutor;
	if (aTutor != NULL && app->mStatsScreen == NULL && app->mHighScoreScreen == NULL && aTutor->mVisible &&
		aTutor->mState != TypingTutor::STATE_FINISHED && aTutor->mLesson != NULL && aTutor->mTask < (int) aTutor->mLesson->mTasks.size())
	{
		TutorExercise* anExercise = aTutor->mLesson->mTasks[aTutor->mTask];
		if (anExercise->mCurText < (int) anExercise->mTexts.size())
		{
			s.mPhase = BoardSnapshot::PHASE_STREAK;
			s.mPromptWhat = "tutor";
			s.mStreakText = anExercise->mTexts[anExercise->mCurText];
			s.mStreakPos = anExercise->mCurChar;
		}
	}
	return s;
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

	Board* b = app->mBoard;
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

	if (b != NULL)
	{
		Player* p = b->mPlayer;
		AddInt(j, "score", p->mScore);
		AddInt(j, "lives", p->mField110);
		AddInt(j, "depth", b->mScore);
		AddInt(j, "timer", b->mGameTimer->Elapsed());
		AddInt(j, "typeTime", b->mTypeTime);
		AddInt(j, "diver", p->mState);
		AddDouble(j, "emp", p->mEmpCharge);
		AddBool(j, "paused", b->mPaused);
		AddBool(j, "won", p->mState == 7 || p->mState == 0xc || (p->mState == 6 && aLessons != NULL && aLessons->IsBonusLevel()));
		int aEnemies = (int) (b->mSharks.size() + b->mPiranhas.size() + b->mClams.size() + b->mBonusCreatures.size());
		if (b->mBoss != NULL)
			aEnemies++;
		AddInt(j, "enemy", aEnemies);

		j += ",\"board\":{";
		j += "\"width\":" + StrFormat("%d", app->mWidth);
		AddString(j, "typed", b->mTyped);
		AddBool(j, "clear", b->mFlag0f80);		// no wave on the screen (the next one is due, or the keys are ignored)
		AddDouble(j, "divePos", b->mDivePos);
		AddDouble(j, "floorY", b->mField0e10);
		AddInt(j, "oceanDepth", b->mOceanDepth);
		AddBool(j, "canType", TestTakeSnapshot(app).mPhase != BoardSnapshot::PHASE_NONE);
		if (b->mBannerLife > 0 && !b->mBannerText.empty())
			AddString(j, "banner", b->mBannerText);
		j += ",\"creatures\":[";
		std::vector<BotCreature> aAll = TestCollectCreatures(b);
		for (size_t i = 0; i < aAll.size(); i++)
		{
			const BotCreature& c = aAll[i];
			if (i > 0)
				j += ",";
			j += "{\"kind\":" + JsonQuote(c.mKind);
			AddString(j, "list", c.mList);
			AddString(j, "word", c.mWord);
			AddInt(j, "typed", c.mTyped);
			AddDouble(j, "x", c.mX);
			AddDouble(j, "y", c.mY);
			AddInt(j, "state", c.mState);
			AddBool(j, "target", c.mIsTarget);
			if (c.mList == "clams")
				AddInt(j, "type", c.mType);
			if (c.mList == "boss")
				AddInt(j, "hp", c.mType);
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
		j += StrFormat(",\"map\":{\"chosen\":%d,\"arrived\":%s,\"final\":%s,\"chests\":\"", aMap->mLevel, aMap->mFlag224 ? "true" : "false",
			aMap->mFinal ? "true" : "false");
		for (size_t i = 0; i < aMap->mVector1b4.size(); i++)
			j += aMap->mVector1b4[i]->mLetter;
		j += "\"}";
	}

	AddBool(j, "over", aOver);
	return j + "}";
}
