// InterfaceMgr.cpp: see InterfaceMgr.h. The drawing and the updates are not translated yet.
//
// The original keeps its tweakable numbers in ModVal calls (each returns the number it was compiled with); they are named
// constants here with the place of the call in the original's source (file, line, id).

#include "InterfaceMgr.h"

#include "Board.h"
#include "Common.h"
#include "HighScoreMgr.h"
#include "LogicMgr.h"
#include "MemoryImage.h"
#include "NotTranslated.h"
#include "PlayerInfo.h"
#include "Res.h"
#include "SexyAppBase.h"
#include "ThunderballApp.h"

using namespace Sexy;

static const int NUM_MAIN_IMAGES = 5;
static const int NUM_EXTRA_IMAGES = 4;
static const int MAX_BALLS_SHOWN = 15;
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
		mColumnBalls[i].mUnknown4 = 0;
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
		aBall.mUnknown4 = 0;
		mColumnBalls.push_back(aBall);
		aY -= aSpacing;
	}
}

// 0x4439b0: the pictures that name the stage and the level (the drawing is not translated yet)
void InterfaceMgr::SyncStageNum()
{
	NOT_TRANSLATED_DRAWING("InterfaceMgr::SyncStageNum");
}

// 0x4518b0
void InterfaceMgr::Update()
{
	NOT_TRANSLATED("InterfaceMgr::Update");
}
