// InterfaceMgr.cpp: see InterfaceMgr.h. The drawing and the updates are not translated yet.
//
// The original keeps its tweakable numbers in ModVal calls (each returns the number it was compiled with); they are named
// constants here with the place of the call in the original's source (file, line, id).

#include "InterfaceMgr.h"

#include <cmath>

#include "Board.h"
#include "Common.h"
#include "EndLevelDialog.h"
#include "HighScoreMgr.h"
#include "LogicMgr.h"
#include "MemoryImage.h"
#include "NotTranslated.h"
#include "PlayerInfo.h"
#include "Res.h"
#include "SexyAppBase.h"
#include "ThunderButton.h"
#include "ThunderballApp.h"

using namespace Sexy;

static const int NUM_MAIN_IMAGES = 5;
static const int NUM_EXTRA_IMAGES = 4;
static const int MAX_BALLS_SHOWN = 15;
static const int MAX_VISIBLE_BALLS = 25;		// the balls the column can have in view (ModValInt, InterfaceMgr.cpp 936, 65)
static const int SCORE_DIFF_SMALL = 100;		// the score on show rolls by a step that grows with how far it is from the score
static const int SCORE_DIFF_MEDIUM = 1000;
static const int SCORE_DIFF_LARGE = 10000;
static const int SCORE_DIFF_BIG = 100000;
static const int SCORE_DIFF_DOWN_BIG = 100000;
static const int METER_DIFF_LARGE = 25000;		// the shot meter's own limit for the step of 200
static const int SCORE_STEP_SMALL = 10;
static const int SCORE_STEP_MEDIUM = 100;
static const int SCORE_STEP_LARGE = 200;
static const int SCORE_STEP_HUGE = 1000;
static const int SCORE_STEP_BIG = 5000;
static const int SHOT_MARK_1 = 25000;			// the marks the shot meter holds at on its way up
static const int SHOT_MARK_2 = 75000;
static const int SHOT_MARK_3 = 125000;
static const int SHOT_METER_HOLD = 400;			// the updates the meter stays after a shot (ModValInt, InterfaceMgr.cpp 1046, 1497)
static const int BLINK_COUNTER_END = 0x77;		// the updates a blink counter runs (1050, 1847)
static const int FEVER_LIGHT_PERIOD = 10;		// (1051, 1930)
static const int FEVER_LIGHT_ON = 20;			// (1052, 1934)
static const int BALL_SHIFT_STEP = 3;			// how fast the column's shift goes back
static const int BALL_SINK_KICK = 0x14;			// the update of the sinking that kicks the balls up
static const int BALL_SINK_END = 0x3d;			// and the one the sinking ends at
static const float BALL_KICK_FIRST = -15.0f;
static const float BALL_KICK = -1.5f;
static const float BALL_GRAVITY = 0.15f;		// (1047, 1592)
static const float BALL_BOUNCE = -0.35f;		// (1048, 1613)
static const float BALL_BOUNCE_MIN_SPEED = 1.0f;
static const float BALL_MOVING_GAP = 5.0f;
static const float BALL_SHARE = 0.75f;			// (1049, 1633)
static const float BALL_TOP_RESTING = 130.0f;
static const int BALL_COLUMN_BOTTOM = 410;			// where the lowest ball is (ModValInt, InterfaceMgr.cpp 932, 28)
static const int BALL_COLUMN_STEP = 3;				// ... and how far the column moves for a ball more (933, 28)
static const int BALL_SPACING_EXTRA = 0;			// added to the ball picture's height (935, 58)
static const int BALL_PICTURE_CHOICE = 0;			// 0 for the interface's ball, the other for the plain one (934, 47)
static const int BALL_SINK_LIMIT_A = 21;			// a column sunk less than this moves the balls down by twice its sink
static const int BALL_SINK_LIMIT_B = 30;
static const int ADD_BALL_TOP = 100;				// where the balls that drop in start (952, 454)
static const int ADD_BALL_SOUND_FIRST = 9;			// 949, 449
static const int ADD_BALL_SOUND_STEP = 4;			// 950, 449
static const int ADD_BALL_SOUND_DELAY = 20;			// 951, 449
static const int ADD_BALL_SOUND_MAX_BALLS = 3;		// the sound plays when fewer balls than this are added
static const int DEFAULT_UNKNOWN_F0 = 10;

// 0x4515a0
InterfaceMgr::InterfaceMgr(Board* theBoard)
{
	mApp = theBoard->mApp;
	mBoard = theBoard;
	mUnknownF0 = DEFAULT_UNKNOWN_F0;
	mImages[0] = new MemoryImage(gSexyAppBase);
	mImages[1] = new MemoryImage(gSexyAppBase);
	mImages[2] = new MemoryImage(gSexyAppBase);
	mImages[3] = new MemoryImage(gSexyAppBase);
	mImages[4] = new MemoryImage(gSexyAppBase);
	mImages[0]->Create(88, 600);
	mImages[2]->Create(624, 74);
	mImages[3]->Create(110, 70);
	mImages[1]->Create(88, 600);
	mImages[4]->Create(624, 14);
	for (int i = 0; i < NUM_EXTRA_IMAGES; i++)
	{
		mUnknown20[i] = new MemoryImage(gSexyAppBase);
		mUnknown20[i]->SetImageMode(true, true);
	}
	mStageImageIndex = -1;
	Clear();
}

InterfaceMgr::~InterfaceMgr()
{
	for (int i = 0; i < NUM_MAIN_IMAGES; i++)
		delete mImages[i];
	for (int i = 0; i < NUM_EXTRA_IMAGES; i++)
		delete mUnknown20[i];
}

// 0x447260
void InterfaceMgr::Clear()
{
	mUnknown30 = -1;
	mUnknown34 = 0;
	mUnknown38 = -1;
	mUnknown3C = 0;
	mUnknown40 = -1;
	mUnknown48 = 0;
	mUnknown4C = 0;
	mUnknown50 = 0;
	mUnknown54 = 0;
	mBallColumnSink = 0;
	mBallColumnShift = 0;
	mUnknown60 = 0;
	mBestScore = 0;
	mBestName = "";
	mUnknown160 = true;
	mUnknown161 = false;
	mScoreShown[1] = 0;
	mScoreShown[0] = 0;
	mUnknown9C = 0;
	mUnknownA0 = "";
	mUnknownBC = "";
	mUnknownDC = 0;
	mUnknownD8 = 0;
	mNumBallsShownCopy = 0;
	mNumBallsShown = 0;
	mUnknown6C = 0;
	mUnknown44 = 0;
	mUnknown68 = 0;
	mUnknownF8 = 0;
	mUnknownE4 = 0;
	mUnknownE0 = 0;
	mColumnBalls.clear();
}

// 0x4725f0
void InterfaceMgr::InitLevel()
{
	UpdateHighScore();
	mStageNumber = mBoard->mApp->mStartStage + 1;
	mLevelNumber = mBoard->mApp->mStartLevel + 1;
}

// 0x4724e0: the best score of the level (its table is named by the level's file name) and who made it
void InterfaceMgr::UpdateHighScore()
{
	PlayerInfo* aPlayer = mBoard->mApp->mCurrentPlayer;
	mUnknown68 = aPlayer == nullptr ? 0 : aPlayer->mUnknown204;
	mBestScore = 0;
	mBestName = "";
	std::string aTable = GetFileName(mBoard->mLevelName, true);
	if (!aTable.empty())
	{
		HighScoreList* aScores = mBoard->mApp->mHighScoreMgr.GetScores(aTable, false);
		if (aScores != nullptr)
		{
			mBestScore = aScores->front().mScore;
			mBestName = aScores->front().mName;
		}
	}
}

// 0x448190
void InterfaceMgr::CheckHighScoreSync()
{
	if (mBestScore > 0 && mBestScore < mScoreShown[0] && mBoard->mGameMode != GAME_MODE_DEMO)
	{
		mBestScore = mScoreShown[0];
		PlayerInfo* aPlayer = mApp->mCurrentPlayer;
		if (aPlayer != nullptr)
			mBestName = aPlayer->mName;
	}
}

// 0x4482d0: the score on show is the score (with no roll-up)
void InterfaceMgr::SettleDown()
{
	LogicMgr* aLogic = mBoard->mLogicMgr.get();
	mScoreShown[0] = aLogic->mScore[0];
	mScoreShown[1] = aLogic->mScore[1];
	CheckHighScoreSync();
}

// 0x436700 and 0x436720: how far apart the balls of the column are
static int GetBallSpacing()
{
	Image* aBallImage = BALL_PICTURE_CHOICE == 0 ? IMAGE_INT_BALL : IMAGE_BALL;
	return BALL_SPACING_EXTRA + aBallImage->mHeight;
}

// 0x4366a0
int InterfaceMgr::GetBallBottom() const
{
	int aBottom = BALL_COLUMN_BOTTOM + mBallColumnShift + BALL_COLUMN_STEP * mNumBallsShownCopy;
	if (mBallColumnSink != 0)
	{
		if (mBallColumnSink < BALL_SINK_LIMIT_A)
			return aBottom + mBallColumnSink * 2;
		if (mBallColumnSink < BALL_SINK_LIMIT_B)
			aBottom = aBottom + (BALL_SINK_LIMIT_B - mBallColumnSink) * 2;
	}
	return aBottom;
}

// 0x457870: the column shows that many balls (at most 15), standing in their places
void InterfaceMgr::SetNumBalls(int theNumBalls)
{
	mColumnBalls.resize(theNumBalls);
	int aNumShown = theNumBalls > MAX_BALLS_SHOWN ? MAX_BALLS_SHOWN : theNumBalls;
	mNumBallsShownCopy = aNumShown;
	mNumBallsShown = aNumShown;
	mBallColumnShift = 0;
	int aY = GetBallBottom();
	int aSpacing = GetBallSpacing();
	for (size_t i = 0; i < mColumnBalls.size(); i++)
	{
		mColumnBalls[i].mY = (float)aY;
		aY -= aSpacing;
		mColumnBalls[i].mSpeed = 0;
	}
	mUnknown160 = true;
}

// 0x45c860: that many balls drop in from the top
void InterfaceMgr::AddTopBalls(int theNumBalls)
{
	mUnknown160 = false;
	int aNumColumnBalls = (int)mColumnBalls.size();
	int aSoundDelay = ADD_BALL_SOUND_DELAY + ADD_BALL_SOUND_STEP * (ADD_BALL_SOUND_FIRST + (theNumBalls - aNumColumnBalls));
	if (aSoundDelay > -1 && theNumBalls < ADD_BALL_SOUND_MAX_BALLS)
		NOT_TRANSLATED("InterfaceMgr::AddTopBalls with fewer than three balls (SoundMgr::AddSound)");
	int aSpacing = GetBallSpacing();
	int aY = ADD_BALL_TOP;
	for (int i = 0; i < theNumBalls; i++)
	{
		ColumnBall aBall;
		aBall.mY = (float)aY;
		aBall.mSpeed = 0;
		mColumnBalls.push_back(aBall);
		aY -= aSpacing;
	}
}

// 0x4439b0: the pictures that name the stage and the level (the drawing is not translated yet)
void InterfaceMgr::SyncStageNum()
{
	NOT_TRANSLATED_DRAWING("InterfaceMgr::SyncStageNum");
}

// 0x436740 (ModValInt, InterfaceMgr.cpp 936, 65): the balls the column can show
static int GetMaxVisibleBalls()
{
	return MAX_VISIBLE_BALLS;
}

// 0x43c770
int InterfaceMgr::GetNumVisibleBalls() const
{
	int aNumBalls = (int)mColumnBalls.size();
	int aMax = GetMaxVisibleBalls();
	return aNumBalls < aMax ? aNumBalls : aMax;
}

// 0x4517c0: the ball (theDelay 0: at once) goes to the bottom of the column, the others fall to make room; theDelay waits that many updates
void InterfaceMgr::AddBottomBall(int theDelay)
{
	mUnknown160 = false;
	if (mUnknown60 != 0)
	{
		mUnknown60 = 0;
		AddBottomBall(0);
	}
	if (theDelay != 0)
	{
		mUnknown60 = theDelay;
		return;
	}
	int aSpacing = GetBallSpacing();
	ColumnBall aBall;
	aBall.mY = 0;
	aBall.mSpeed = 0;
	mColumnBalls.insert(mColumnBalls.begin(), aBall);
	mBallColumnShift += aSpacing;
	mColumnBalls[0].mY = (float)GetBallBottom();
	int aNumVisible = GetNumVisibleBalls();
	for (int i = 0; i < aNumVisible; i++)
		mColumnBalls[i].mSpeed = -1.0f;
}

// 0x4481d0: each player's score on show rolls up (or down) towards the real one, faster the further away
void InterfaceMgr::UpdateScoreDisp()
{
	if (mUnknown9C != 0)
	{
		mUnknown9C--;
		return;
	}
	LogicMgr* aLogic = mBoard->mLogicMgr.get();
	for (int aPlayer = 0; aPlayer < 2; aPlayer++)
	{
		int aShown = mScoreShown[aPlayer];
		int aScore = aLogic->mScore[aPlayer];
		if (aShown == aScore)
			continue;
		if (aShown < aScore)
		{
			int aDiff = aScore - aShown;
			if (aLogic->mState == 5)
				aShown += SCORE_STEP_BIG;
			else if (aDiff <= SCORE_DIFF_BIG)
			{
				if (aDiff <= SCORE_DIFF_LARGE)
				{
					if (aDiff <= SCORE_DIFF_MEDIUM)
						aShown += aDiff <= SCORE_DIFF_SMALL ? SCORE_STEP_SMALL : SCORE_STEP_MEDIUM;
					else
						aShown += SCORE_STEP_LARGE;
				}
				else
					aShown += SCORE_STEP_HUGE;
			}
			else
				aShown += SCORE_STEP_BIG;
			if (aScore < aShown)
				aShown = aScore;
		}
		else
		{
			int aDiff = aShown - aScore;
			if (SCORE_DIFF_DOWN_BIG < aDiff)
				aShown -= SCORE_STEP_BIG;
			if (aDiff <= SCORE_DIFF_LARGE)
			{
				if (aDiff <= SCORE_DIFF_MEDIUM)
					aShown -= aDiff <= SCORE_DIFF_SMALL ? SCORE_STEP_SMALL : SCORE_STEP_MEDIUM;
				else
					aShown -= SCORE_STEP_LARGE;
			}
			else
				aShown -= SCORE_STEP_HUGE;
			if (aShown < aScore)
				aShown = aScore;
		}
		mScoreShown[aPlayer] = aShown;
		if (aPlayer == 0)
			CheckHighScoreSync();
	}
}

// 0x436c90: the shot meter (the points of the shot so far) rolls up to the shot's score; it stops at the marks of 25,000, 75,000 and 125,000
void InterfaceMgr::UpdateShotMeter()
{
	LogicMgr* aLogic = mBoard->mLogicMgr.get();
	if (aLogic->mState == 2 && !aLogic->mUnknownF6)
	{
		int aShotScore = aLogic->mUnknown108;
		if (aShotScore < SHOT_MARK_3 || SHOT_MARK_3 - 1 < mUnknown48)
		{
			if (aShotScore < SHOT_MARK_2 || SHOT_MARK_2 - 1 < mUnknown48)
			{
				if (aShotScore < SHOT_MARK_1 || SHOT_MARK_1 - 1 < mUnknown48)
					mUnknown4C = aShotScore;
				else
				{
					mUnknown48 = SHOT_MARK_1;
					mUnknown4C = SHOT_MARK_1;
					mUnknown161 = true;
				}
			}
			else
			{
				mUnknown48 = SHOT_MARK_2;
				mUnknown4C = SHOT_MARK_2;
				mUnknown161 = true;
			}
		}
		else
		{
			mUnknown48 = SHOT_MARK_3;
			mUnknown4C = SHOT_MARK_3;
			mUnknown161 = true;
		}
		if (mUnknown50 < aLogic->mUnknown108)
			mUnknown50 = aLogic->mUnknown108;
	}
	if (mUnknown54 != 0)
	{
		mUnknown54++;
		if (SHOT_METER_HOLD <= mUnknown54)
		{
			if (mUnknown48 == 0)
				mUnknown50 = 0;
			mUnknown54 = 0;
		}
	}
	int aShown = mUnknown48;
	if (aShown == mUnknown4C)
		return;
	int aGoal = mUnknown4C;
	if (aShown < aGoal)
	{
		int aDiff = aGoal - aShown;
		if (aDiff <= SCORE_DIFF_BIG)
		{
			if (aDiff <= METER_DIFF_LARGE)
			{
				if (aDiff <= SCORE_DIFF_MEDIUM)
					aShown += aDiff <= SCORE_DIFF_SMALL ? SCORE_STEP_SMALL : SCORE_STEP_MEDIUM;
				else
					aShown += SCORE_STEP_LARGE;
			}
			else
				aShown += SCORE_STEP_HUGE;
		}
		else
			aShown += SCORE_STEP_BIG;
		mUnknown48 = aShown;
		if (aGoal < mUnknown48)
			mUnknown48 = aGoal;
	}
	else
	{
		int aDiff = aShown - aGoal;
		if (SCORE_DIFF_DOWN_BIG < aDiff)
			mUnknown48 = aShown - SCORE_STEP_BIG;
		if (aDiff <= METER_DIFF_LARGE)
			mUnknown48 -= SCORE_STEP_LARGE;
		else
			mUnknown48 -= SCORE_STEP_HUGE;
		if (mUnknown48 < aGoal)
			mUnknown48 = aGoal;
	}
	if (mUnknown48 != 0)
	{
		if (SHOT_MARK_1 - 1 < mUnknown48)
			mUnknown161 = true;
		return;
	}
	if (mUnknown54 == 0)
		mUnknown50 = 0;
	mUnknown161 = false;
}

// 0x443cf0: the column's balls fall into their places and bounce off each other (a ball that has been added drops, one that has been used sinks)
void InterfaceMgr::UpdateBalls()
{
	if (mColumnBalls.empty() || mUnknown160)
	{
		if (mBallColumnSink == 0)
			return;
		mBoard->mLogicMgr->mUnknown138 = 2;
		mBallColumnSink = 0;
		return;
	}
	if (mBallColumnShift != 0 && (mBallColumnShift -= BALL_SHIFT_STEP) < 0)
		mBallColumnShift = 0;
	int aNumVisible = GetNumVisibleBalls();
	if (mBallColumnSink != 0 && ++mBallColumnSink == BALL_SINK_KICK)
	{
		float aSpeed = BALL_KICK_FIRST;
		for (int i = aNumVisible - 1; i >= 0; i--)
		{
			mColumnBalls[i].mSpeed = aSpeed;
			aSpeed = BALL_KICK;
		}
	}
	float aGravity = BALL_GRAVITY;
	int aBottom = GetBallBottom();
	int aSpacing = GetBallSpacing();
	int aNumToShow = (int)mColumnBalls.size();
	if (aNumToShow > MAX_BALLS_SHOWN)
		aNumToShow = MAX_BALLS_SHOWN;
	if (BALL_SINK_KICK - 1 < mBallColumnSink && 0 < aNumToShow)
		aNumToShow--;
	if (aNumToShow == 0)
		mNumBallsShown = 0;
	bool aMoving = false;
	ColumnBall& aFirst = mColumnBalls[0];
	aFirst.mY = aFirst.mY + aFirst.mSpeed;
	float aFloor = (float)aBottom;
	if (aFirst.mY <= aFloor)
	{
		if (aFirst.mY < aFloor)
		{
			aMoving = true;
			aFirst.mSpeed = aFirst.mSpeed + aGravity;
		}
	}
	else
	{
		aFirst.mY = aFloor;
		if (BALL_BOUNCE_MIN_SPEED <= fabsf(aFirst.mSpeed))
			aFirst.mSpeed = BALL_BOUNCE * aFirst.mSpeed;
		else
			aFirst.mSpeed = 0;
		if (mNumBallsShown < 1)
			mNumBallsShown = 1;
	}
	if (1 < aNumVisible)
	{
		for (int i = 1; i < aNumVisible; i++)
		{
			ColumnBall& aBall = mColumnBalls[i];
			ColumnBall& aBelow = mColumnBalls[i - 1];
			float aGap = (float)(((double)aBelow.mY - ((double)aBall.mY + aBall.mSpeed)) - aSpacing);
			if (0.0f <= aGap)
			{
				if (!aMoving && BALL_MOVING_GAP < aGap)
					aMoving = true;
				aBall.mSpeed = aGravity + aBall.mSpeed;
				aBall.mY = aBall.mSpeed + aBall.mY;
			}
			else
			{
				aBall.mY = aBelow.mY - (float)aSpacing;
				float aShare = (float)((double)BALL_SHARE * 0.5 * ((double)fabsf(aBall.mSpeed) + fabsf(aBelow.mSpeed)));
				if (aShare < 1.0f)
					aShare = 0.0f;
				if (0.0f < aBall.mSpeed)
					aBall.mSpeed = 0.0f;
				if (aBelow.mSpeed < 0.0f)
					aBelow.mSpeed = 0.0f;
				aBall.mSpeed = aBall.mSpeed - aShare;
				aBelow.mSpeed = aBelow.mSpeed + aShare;
				if (!aMoving && mNumBallsShown < i + 1)
					mNumBallsShown = i + 1;
			}
		}
	}
	if (aNumToShow < mNumBallsShown || (0 < mNumBallsShown && 3 < aNumToShow - mNumBallsShown))
		mNumBallsShown = aNumToShow;
	if (BALL_SINK_KICK < mBallColumnSink && (mColumnBalls.back().mY < BALL_TOP_RESTING || BALL_SINK_END <= mBallColumnSink))
	{
		mBallColumnSink = 0;
		if (!mColumnBalls.empty())
			mColumnBalls.pop_back();
		int aSize = (int)mColumnBalls.size();
		if (GetMaxVisibleBalls() <= aSize)
		{
			ColumnBall& aLast = mColumnBalls[GetMaxVisibleBalls() - 1];
			aLast.mY = BALL_TOP_RESTING;
			aLast.mSpeed = 0;
		}
		mBoard->mLogicMgr->mUnknown138 = 2;
	}
	if (mBallColumnShift == 0 && mBallColumnSink == 0 && mUnknown60 == 0)
	{
		mUnknown160 = true;
		int aVisible = GetNumVisibleBalls();
		for (int i = 0; i < aVisible; i++)
			if (mColumnBalls[i].mSpeed != 0.0f)
				mUnknown160 = false;
	}
	if (mNumBallsShownCopy == mNumBallsShown)
		return;
	if (mNumBallsShownCopy < mNumBallsShown)
	{
		mNumBallsShownCopy++;
		if (mNumBallsShownCopy == mNumBallsShown && aNumToShow < mNumBallsShownCopy)
		{
			mNumBallsShown = aNumToShow;
			mUnknown160 = false;
			return;
		}
	}
	else
		mNumBallsShownCopy--;
	mUnknown160 = false;
}

// 0x4518b0
void InterfaceMgr::Update()
{
	int* aCounters[2][2] = {{&mUnknownD8, &mUnknownE0}, {&mUnknownDC, &mUnknownE4}};
	for (int i = 0; i < 2; i++)
	{
		if (*aCounters[i][0] != 0 && 300 < ++*aCounters[i][0])
			*aCounters[i][0] = 0;
		if (*aCounters[i][1] != 0)
		{
			++*aCounters[i][1];
			if (BLINK_COUNTER_END < *aCounters[i][1])
				*aCounters[i][1] = 0;
		}
	}
	UpdateScoreDisp();
	if (mUnknown60 != 0 && --mUnknown60 == 0)
		AddBottomBall(0);
	UpdateShotMeter();
	UpdateBalls();
	if (mUnknown34 != 0 && 0xb3 < ++mUnknown34)
		mUnknown34 = 0;
	if (mUnknownF8 != 0 && 0x78 < ++mUnknownF8)
		mUnknownF8 = 0;
	int aFeverFirst = mUnknown30;
	if (aFeverFirst != -1 && ++mUnknown3C == 10)
	{
		mUnknown3C = 0;
		int aLast = 25 - (int)mBoard->mLogicMgr->mGoalPegs.size();
		if (aFeverFirst < aLast)
		{
			if (mUnknown38 == -1)
				mUnknown38 = aFeverFirst;
			else
				mUnknown38++;
			if (aLast <= mUnknown38)
			{
				mUnknown3C = -30;
				mUnknown38 = aFeverFirst;
			}
		}
		else
			mUnknown38 = -1;
	}
	ThunderButton* aButton = mBoard->mInstantReplayButton;
	int aBlink = mUnknownF0;
	if (!aButton->mDisabled)
	{
		if (0 < aBlink && (mUnknownF0 = aBlink - 1) == 0)
			aButton->Blink(mBoard->mLogicMgr->mUnknownF6 ? 8 : 1, false);
	}
	else if (aBlink < 10)
		mUnknownF0 = aBlink + 1;
	if (mUnknown6C != 0)
		mUnknown6C--;
	if (mUnknown44 != 0 && mBoard->mEndLevelDialog->mWidgetManager == nullptr)
	{
		mUnknown44++;
		int aDivisor = FEVER_LIGHT_PERIOD;
		for (int i = 0; i < 25; i++)
		{
			if ((mUnknown44 / aDivisor) % 6 == i % 6)
				mUnknownFC[i] = FEVER_LIGHT_ON;
			else if (0 < mUnknownFC[i])
				mUnknownFC[i]--;
		}
	}
}
