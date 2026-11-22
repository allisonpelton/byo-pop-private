// LogicMgr.cpp: see LogicMgr.h. Only the making, the emptying and the start of a level are translated so far.
//
// The original keeps its tweakable numbers in ModVal calls (each returns the number it was compiled with); they are named
// constants here with the place of the call in the original's source (file, line, id).

#include "LogicMgr.h"

#include "AIMgr.h"
#include "Ball.h"
#include "Board.h"
#include "CharacterMgr.h"
#include "Common.h"
#include "DebugMgr.h"
#include "EffectMgr.h"
#include "EndLevelDialog.h"
#include "FloatingTextMgr.h"
#include "Res.h"
#include "WidgetManager.h"
#include "Gun.h"
#include "Hole.h"
#include "InterfaceMgr.h"
#include "NotTranslated.h"
#include "PegInfo.h"
#include "Poly.h"
#include "PhysObj.h"
#include "PlayerInfo.h"
#include "SoundInstance.h"
#include "ThunderButton.h"
#include "SoundMgr.h"
#include "ThunderMath.h"
#include "ThunderballApp.h"

#include <algorithm>
#include <cmath>

using namespace Sexy;

static const int INIT_LEVEL_PICTURE_WAIT = 30;	  // the updates a level with a full-size picture waits before its pegs come on (ModValInt, LogicMgr.cpp 1272, 4522)
static const int PRESHOT_HINT_TIME_NEW = 50;	  // the update the first hint comes at in the first game (ModValInt, LogicMgr.cpp 1279, 4608)
static const int PRESHOT_HINT_TIME = 140;		  // and later (1280, 4609)
static const int PRESHOT_DEFAULT_BLINKS = 2;	  // the blinks of a hint when the rank says nothing of them (1281, 4614)
static const int PRESHOT_BLINKS = 5;			  // the blinks of a hint (1282 to 1285)
static const int PRESHOT_MANY_GOAL_PEGS = 10;	  // a level with more orange pegs than this blinks none of them (1286, 4636)
static const int PRESHOT_LAST_PEGS_TIME = 75;	  // the update the last orange pegs flash at (1287, 4648)
static const int PRESHOT_FEW_GOAL_PEGS = 6;		  // 'last' is this many or fewer (1288, 4654)
static const int COMPUTER_THINK_TIME = 100;		  // the update the computer starts to think at, 150 while a replay is kept
static const int COMPUTER_THINK_TIME_REPLAY = 150;
static const int POST_SHOT_WAIT = 0x95;			  // the updates a game of two waits after a ball (a mode of two checks the ball has been shot for this long)
static const float BEGIN_TURN_X = 320.0f;		  // the middle of the board, where the turn's texts start
static const float BEGIN_TURN_Y = 240.0f;
static const int POWERUP_TURNS_GIVEN = 1000;	  // the turns a power-up that is given lasts
static const int NUM_POWERUPS = 14;
static const int EFFECT_TYPE_FEVER_SPARK = 0x16;	  // the effects of this type go when the power-up that shows them ends
static const int FREE_BALL_COVER_TIME = 0x104;	  // (the original's mUnknown134, which counts the free-ball cover's time)
static const int FREE_BALL_COVER_TIME_FEVER = 200;
static const int FREE_BALL_COVER_TIME_FLIPPERS = 0x82;
static const int TIP_FEW_GOAL_PEGS = 10;		  // the tip of the orange pegs comes for a level that has this many or fewer (ModValInt, LogicMgr.cpp 1105, 1748)
static const int CHARACTER_DIALOG_TEXT_TIME = 200;	  // the update the start of a level's text goes at when the level has a picture (ModValInt, LogicMgr.cpp 1278, 4597)
static const int FREE_BALL_UPDATE_DIVISOR = 2;	  // the catcher's pieces change on the updates that are not a multiple of this (ModValInt, LogicMgr.cpp 1261, 4256)
static const int FEVER_ROW = 4;					  // the row of the fever power's turns left
static const int BUMPER_HOLE_COUNTDOWN = 300;	  // (ModValInt, LogicMgr.cpp 1078, 1463)
static const int FLIPPER_COUNTDOWN = 180;		  // (1079, 1559)
static const int SHOT_NOT_YET = -1000;			  // the "time" of a shot that has not started
static const int SHOT_METER_PEGS = 25;			  // the shot meter counts the orange pegs against the usual 25
static const int PEG_FLASH_BLINK = 10;			  // a peg that blinks (the hint)
static const int FLIPPER_ROW = 2;				  // the row of the flippers' turns left
static const int TIME_BOMB_ROW = 11;			  // and the time bomb's
static const int SHOT_BOOST_START = 1500;		  // the update of the shot a click speeds it from (the original's global at 0x649ac4)
static const int FIREBALL_ROW = 12;				  // the rows of mUnknown1D4 (a count for each player) that are the fireball's turns left
static const int ZEN_ROW = 7;					  // and the zen ball's
static const int CANNON_COCK_DELAY = 8;			  // the updates a cocked cannon waits before it may fire (ModValInt, LogicMgr.cpp 1412, 6042)
static const float MOUSE_WHEEL_MOVE = 0.1f;			  // the base move of the mouse wheel (ModValFloat, LogicMgr.cpp 1053, 67)
static const int TWO_PLAYER_BALLS = 6;				  // the balls each of two players starts with (ModValInt, LogicMgr.cpp 1058, 1150)
static const int ONE_PLAYER_BALLS = 10;
static const float AIM_LEAD_FACTOR = 1.3e-06f;		  // how much of the distance and the angle off the vertical is added to the aim (ModValFloat, LogicMgr.cpp 1056, 813)
static const int POWERUP_MIN_DISTANCE = 150;		  // the green pegs are at least this far apart (ModValInt, LogicMgr.cpp 1057, 1044)
static const float POWERUP_MAX_Y = 370.0f;			  // a green peg is picked from the pegs above this line first
static const int DEFAULT_NUM_GOAL_PEGS = 25;
static const int NUM_SHUFFLE_SWAPS = 1000;			  // 0x43d430
static const int NUM_CLICK_RECORDS_RESERVED = 60000;
static const float DEFAULT_BALL_RADIUS = 6.0f;		  // 0x649d78
static const float GRAVITY = 0.05f;					  // 0x649d80
static const float ANGLE_CONSTRAINT = 83.0f;		  // degrees to each side of straight down that the cannon cannot point at (ThunderCommon.cpp 123, 125)

// the original's globals at 0x649c30 and 0x650a3c: who shot first in the last two-player level, and how many levels running
static int gLastFirstPlayer = -1;
static int gFirstPlayerRepeats = 0;

bool PegSerialLess::operator()(const SharedPtr<PhysObj>& theLeft, const SharedPtr<PhysObj>& theRight) const
{
	return theLeft->mSerialNumber < theRight->mSerialNumber;
}

// 0x4610d0
LogicMgr::LogicMgr(Board* theBoard)
{
	mCornerText = "";
	mBoard = theBoard;
	mUnknown330 = 0;
	mUnknown334 = 0;
	mUnknown338 = 0;
	mUnknown33C = 0;
	for (int i = 0; i < 11; i++)
		mUnknown370[i] = 0;
	mUnknown1CC = 0;
	mUnknown1D0 = 0;
	mUnknown54 = false;
	mUnknown21 = false;
	mSlowMoSound = nullptr;
	mFadeSound = nullptr;
	Clear(true, false);
}

LogicMgr::~LogicMgr()
{
}

// 0x436fb0
void LogicMgr::SetState(int theState)
{
	mState = theState;
	mStateTime = 0;
}

// 0x4372c0
void LogicMgr::KillSlowMoSound()
{
	if (mSlowMoSound != nullptr)
	{
		mSlowMoSound->Release();
		mSlowMoSound = nullptr;
	}
}

// 0x458ab0
void LogicMgr::FadeOutSound()
{
	if (mFadeSound != nullptr)
	{
		mBoard->mSoundMgr->AddFadeSound(mFadeSound);
		mFadeSound = nullptr;
	}
}

// 0x4370b0: the number the points of a shot are multiplied by, for the number of goal pegs left
int LogicMgr::CalcScoreMult(int theNumPegsLeft) const
{
	if (theNumPegsLeft < 1 && !mUnknownFB)
		return 100;
	if (theNumPegsLeft < 4)
		return 10;
	if (theNumPegsLeft < 7)
		return 5;
	if (theNumPegsLeft < 11)
		return 3;
	return theNumPegsLeft < 16 ? 2 : 1;
}

// 0x437110
int LogicMgr::CalcMusicIntensity(int theNumPegsLeft)
{
	if (theNumPegsLeft < 4)
		return 6;
	if (theNumPegsLeft < 7)
		return 5;
	if (theNumPegsLeft < 11)
		return 4;
	if (theNumPegsLeft < 16)
		return 3;
	return theNumPegsLeft < 21 ? 2 : 1;
}

// 0x45d900
void LogicMgr::Clear(bool theNewGame, bool theKeepLists)
{
	mCornerText = ""; // (the original makes a "pegsongs/" string here and does not use it)
	SetState(0);
	mUnknown0C = 0;
	mUnknown10 = 0;
	mUnknown14 = 0;
	mUnknown18 = false;
	mUnknown19 = false;
	mUnknown1A = false;
	mUnknown1B = false;
	mUnknown1C = false;
	mUnknown1D = false;
	mUnknown1E = false;
	mUnknown1F = false;
	mUnknown20 = false;
	mUnknown21 = false;
	mUnknown24 = 0;
	mUnknown28 = 0;
	mUnknown2C = 0;
	mUnknown30 = 0;
	mUnknown34 = 0;
	mUnknown38 = 0;
	mUnknown3C = 0;
	mUnknown40 = 0;
	mUnknown44 = 0;
	mUnknown48 = 0;
	mUnknown4C = 0;
	mUnknown50 = 0;
	mUnknown68 = false;
	mUnknown69 = false;
	mUnknown100 = false;
	mUnknown101 = false;
	mUnknown120 = 0;
	mUnknownA0 = 0;
	mUnknownA4 = 0;
	mUnknownA8 = false;
	mUnknownA9 = false;
	mUnknownAA = false;
	mUnknown70 = MOUSE_WHEEL_MOVE;
	mGunAngle = 0;
	mUnknownE4 = 0;
	mUnknownE8 = 0;
	mUnknownEC = false;
	mUnknownED = false;
	mUnknownF0 = 0;
	mUnknownF4 = false;
	mUnknownF5 = false;
	mUnknownF6 = false;
	mUnknownF7 = false;
	mUnknownF8 = false;
	mUnknownF9 = false;
	mUnknownFA = false;
	mUnknownFC = false;
	mUnknownFD = false;
	mUnknownFE = false;
	mUnknownFF = false;
	mUnknown16C = 0;
	mUnknown108 = 0;
	mUnknown10C = 0;
	mUnknown110 = 0;
	mScoreMult = 0;
	mMusicIntensity = 0;
	mUnknown11C = 0;
	mUnknown14C = -1;
	mUnknown150 = -1;
	mUnknown330 = 0;
	mUnknown334 = 0;
	mUnknown338 = 0;
	mUnknown33C = 0;
	mCurPlayer = 0;
	mUnknown12C = 0;
	mUnknown134 = 0;
	mUnknown138 = 0;
	mUnknown13C = 0;
	mUnknown140 = 0;
	mUnknown144 = 0;
	mUnknown148 = 0;
	mUnknown154 = 0;
	mUnknown158 = 0;
	mUnknown15C = 0;
	mLevelType = 0;
	mUnknown164 = 0;
	mUnknown168 = 0;
	mUnknown320 = 0;
	mUnknown324 = 0;

	KillSlowMoSound();
	FadeOutSound();

	if (!theKeepLists)
	{
		mScorePeg = nullptr;
		mPegs.clear();
		mGoalPegs.clear();
		mOtherPegs.clear();
	}
	mUnknown34C.clear();
	for (int i = 0; i < 11; i++)
		mUnknown370[i] = 0;
	mClickEvents.clear();
	mClickRecords.clear();
	mClickRecords.reserve(NUM_CLICK_RECORDS_RESERVED);
	mUnknownB4.clear();
	for (int i = 0; i < 14; i++)
	{
		mUnknown1D4[i][0] = 0;
		mUnknown1D4[i][1] = 0;
	}
	for (int aPlayer = 0; aPlayer < 2; aPlayer++)
	{
		(&mUnknown58)[aPlayer] = 0;
		(&mUnknown60)[aPlayer] = 0;
		mPlayerStats[aPlayer].Reset();
	}

	if (theNewGame)
	{
		mUnknownFB = false;
		mTwoPlayer = false;
		for (int aPlayer = 0; aPlayer < 2; aPlayer++)
		{
			mScore[aPlayer] = 0;
			mScoreAtStart[aPlayer] = 0;
			mNumBalls[aPlayer] = 0;
			mCharacter[aPlayer] = 0;
		}
		mPlayerName[0] = "Player1";
		mPlayerName[1] = "Player2";
		mUnknown1C4 = 1;
		mUnknown1C8 = 1;
		mUnknown244 = false;
		mUnknown245 = false;
		mUnknown74 = 0;
		mUnknown78 = 0;
		mNumGoalPegs = 0;
	}
	else
	{
		for (int aPlayer = 0; aPlayer < 2; aPlayer++)
			mScoreAtStart[aPlayer] = mScore[aPlayer];
	}
}

// 0x436fd0: the angle in [0, 2 pi), and when it is in the band below the cannon (a ball cannot be shot downwards) the nearer edge of the band
void LogicMgr::SetGunAngle(float theAngle)
{
	mGunAngle = GetCanonicalAngleRad(theAngle);
	float aBandStart = (float)(((double)(float)(90.0 - ANGLE_CONSTRAINT) * (double)PI_AS_FLOAT) / 180.0);
	float aBandEnd = (float)(((double)(float)(ANGLE_CONSTRAINT + 90.0) * (double)PI_AS_FLOAT) / 180.0);
	if (aBandStart < mGunAngle && mGunAngle < aBandEnd)
	{
		float aMiddle = (float)(((double)aBandEnd + aBandStart) * 0.5);
		mGunAngle = mGunAngle <= aMiddle ? aBandStart : aBandEnd;
	}
}

// 0x43ece0: the angle the cannon takes to hit where the mouse is. A shot is a parabola, so the aim is looked for among angles
// about the straight line to the mouse: the one whose ball passes nearest to the mouse is kept.
void LogicMgr::CalcGunAngle(bool theSetAtOnce)
{
	if (mState == LOGIC_STATE_UNKNOWN10 || mUnknownEC)
		return;
	mUnknown1F = false;
	Gun* aGun = mBoard->mGun.get();
	if (aGun == nullptr)
	{
		mGunAngle = 0;
		return;
	}

	float aGunX = aGun->mX;
	float aGunY = aGun->mY;
	float aMouseX = (float)mBoard->GetBoardMouseX();
	float aMouseY = (float)mBoard->GetBoardMouseY();
	float aDeltaX = aMouseX - aGunX;
	float aAngle = (float)std::atan2((double)(aGunY - aMouseY), (double)aDeltaX);
	float aOffVertical = (float)std::fabs(std::atan2((double)aDeltaX, (double)(aMouseY - aGunY)));
	float aDistSquared = (float)((double)(aMouseY - aGunY) * (aMouseY - aGunY) + (double)aDeltaX * aDeltaX);
	float aLead = (float)((double)AIM_LEAD_FACTOR * aDistSquared * aOffVertical);
	if (aLead > 1.0f)
		aLead = 1.0f;
	if (aDeltaX < 0)
		aLead = -aLead;
	aAngle = aAngle + aLead;

	float aBestAngle = aAngle;
	float aBestError = 1000000.0f;
	float aSpeed = aGun->mLaunchSpeed;
	float aStartX = (float)((((double)aGun->mBarrelLength + aGunX) + aGun->mBaseShift) + DEFAULT_BALL_RADIUS);
	float aStartY = aGunY + 1.0f;
	if ((((double)aGun->mBarrelLength + aGun->mBaseShift) + DEFAULT_BALL_RADIUS + 10.0) < Distance(aStartX, aStartY, aMouseX, aMouseY))
	{
		for (int i = -10; i < 10; i++)
		{
			float aTry = (float)((double)i * 0.01f + aAngle);
			float aPointX = aStartX;
			float aPointY = aStartY;
			RotateXY(&aPointX, &aPointY, aGunX, aGunY, aTry);
			float aToMouseX = aMouseX - aPointX;
			float aToMouseY = aMouseY - aPointY;
			float aCosine = (float)std::fabs(std::cos((double)aTry));
			float anError;
			if (aCosine < 0.001f)
				anError = aToMouseX == 0 ? 0.0f : 1000000.0f;
			else
			{
				float aTime = (float)((double)aToMouseX / ((double)aCosine * aSpeed));
				if (aTime < 0)
					anError = 1000000.0f;
				else
				{
					double aFall = (0.5 * GRAVITY * aTime) * aTime;
					double aRise = (std::sin((double)aTry) * aTime) * aSpeed;
					anError = (float)std::fabs((aFall - aRise) - aToMouseY);
				}
			}
			if (anError < aBestError)
			{
				aBestAngle = aTry;
				aBestError = anError;
			}
		}
	}

	SetGunAngle(aBestAngle);
	if (theSetAtOnce)
		aGun->SetAngle(mGunAngle, false);
}

// 0x440500: the level's pegs are hidden until the level has come in
void LogicMgr::BeginInitLevel()
{
	if (mPegs.empty())
		return;
	SetState(LOGIC_STATE_INIT_LEVEL);
	for (std::list<SharedPtr<PhysObj> >::iterator anIter = mPegs.begin(); anIter != mPegs.end(); ++anIter)
		(*anIter)->mVisible = false;
	mUnknown40 = 0;
	mUnknown44 = 0;
	mUnknown3C = 0;
	mUnknown34 = 0;
}

// 0x440700
void LogicMgr::ActivateFreeBallCover(bool theActive)
{
	if (mUnknownF6 && theActive)
		return;
	for (std::list<SharedPtr<PhysObj> >::iterator anIter = mBoard->mObjects.begin(); anIter != mBoard->mObjects.end(); ++anIter)
	{
		if ((*anIter)->mId == "freeballcover")
			(*anIter)->SetActiveWithGrowAnim(theActive);
	}
}

// 0x43d430: puts the pegs in a random order, by a thousand swaps of two places
static void ShufflePegs(std::vector<SharedPtr<PhysObj> >& thePegs)
{
	if (thePegs.size() <= 1)
		return;
	for (int i = 0; i < NUM_SHUFFLE_SWAPS; i++)
	{
		int aFirst = Rand() % (int)thePegs.size();
		int aSecond = Rand() % (int)thePegs.size();
		std::swap(thePegs[aFirst], thePegs[aSecond]);
	}
}

// 0x457960: every peg of the level is made active and kept in the lists. The pegs whose colour is fixed go to the lists as they are
// (the orange ones to the goal pegs), the others are shuffled and the first of them (25, or what the level says) become orange.
void LogicMgr::MakeGoalPegs()
{
	std::vector<SharedPtr<PhysObj> > aCandidates;
	for (std::list<SharedPtr<PhysObj> >::iterator anIter = mBoard->mObjects.begin(); anIter != mBoard->mObjects.end(); ++anIter)
	{
		PhysObj* anObject = anIter->get();
		if (anObject->mPegInfo == nullptr)
			continue;
		PegInfo* aPegInfo = anObject->mPegInfo.get();
		aPegInfo->mCrumbleTime = 0;
		aPegInfo->SetHit(false);
		anObject->SetActive(true);
		mPegs.push_back(*anIter);
		if (!aPegInfo->mVariable)
		{
			if (aPegInfo->mKind == 2)
				mGoalPegs.insert(*anIter);
			else
				mOtherPegs.insert(*anIter);
		}
		else
		{
			anObject->SetPegKind(1);
			aCandidates.push_back(*anIter);
		}
	}

	ShufflePegs(aCandidates);
	size_t aNumGoalPegs = mNumGoalPegs != 0 ? mNumGoalPegs : DEFAULT_NUM_GOAL_PEGS;
	while (mGoalPegs.size() < aNumGoalPegs && !aCandidates.empty())
	{
		mGoalPegs.insert(aCandidates.back());
		aCandidates.pop_back();
	}
	for (size_t i = 0; i < aCandidates.size(); i++)
		mOtherPegs.insert(aCandidates[i]);

	for (PegSet::iterator anIter = mOtherPegs.begin(); anIter != mOtherPegs.end(); ++anIter)
	{
		if ((*anIter)->mPegInfo->mKind != 1 && !mBoard->mApp->OnMaxLevel())
			(*anIter)->SetPegKind(1);
	}
	for (PegSet::iterator anIter = mGoalPegs.begin(); anIter != mGoalPegs.end(); ++anIter)
		(*anIter)->SetPegKind(2);
}

// 0x458110: the purple peg, which is one of the blue ones, from the third level of the adventure on
void LogicMgr::MakeScorePeg()
{
	if (mBoard->mApp->GetMaxLevel() < 3)
		return;

	std::vector<SharedPtr<PhysObj> > aCandidates;
	bool aFound = false;
	for (PegSet::iterator anIter = mOtherPegs.begin(); anIter != mOtherPegs.end(); ++anIter)
	{
		int aKind = (*anIter)->mPegInfo->mKind;
		if (aKind == 3)
		{
			if (*anIter != mScorePeg)
			{
				mScorePeg = *anIter;
				aFound = true;
				break;
			}
		}
		else if (aKind == 1)
			aCandidates.push_back(*anIter);
	}
	if (aFound)
		return;

	if (mScorePeg != nullptr && !aCandidates.empty())
	{
		if (mScorePeg->mPegInfo->mKind != 2 && !mScorePeg->mPegInfo->mHit)
			mScorePeg->SetPegKind(1);
		mScorePeg = nullptr;
	}
	if (!aCandidates.empty())
	{
		int anIndex = Rand() % (int)aCandidates.size();
		mScorePeg = aCandidates[anIndex];
		mScorePeg->SetPegKind(3);
		if (mState != 0)
			mScorePeg->mPegInfo->DoFlash(2);
	}
}

// 0x457dc0: the green pegs (one in a game of two players, two otherwise), from the second level on
void LogicMgr::MakePowerupPeg()
{
	if (mBoard->mApp->GetMaxLevel() < 2)
		return;

	float aLastX = -1000.0f;
	float aLastY = -1000.0f;
	int aNeeded = mTwoPlayer ? 1 : 2;
	int anExisting = 0;
	std::vector<SharedPtr<PhysObj> > aCandidates;
	for (PegSet::iterator anIter = mOtherPegs.begin(); anIter != mOtherPegs.end(); ++anIter)
	{
		int aKind = (*anIter)->mPegInfo->mKind;
		if (aKind == 4)
		{
			aLastX = (*anIter)->GetXPos();
			aLastY = (*anIter)->GetYPos();
			anExisting++;
		}
		else if (aKind == 1 && (*anIter)->GetYPos() < POWERUP_MAX_Y)
			aCandidates.push_back(*anIter);
	}

	aNeeded -= anExisting;
	if (aNeeded <= 0)
		return;
	if (aCandidates.empty())
	{
		for (PegSet::iterator anIter = mOtherPegs.begin(); anIter != mOtherPegs.end(); ++anIter)
		{
			if ((*anIter)->mPegInfo->mKind == 1 && (*anIter)->mPegInfo->mVariable)
				aCandidates.push_back(*anIter);
		}
	}
	if (aCandidates.empty())
		return;

	ShufflePegs(aCandidates);
	int aMinDistanceSquared = POWERUP_MIN_DISTANCE * POWERUP_MIN_DISTANCE;
	for (int aPass = 0; aPass < 2; aPass++)
	{
		int aRemaining = aNeeded;
		if (aRemaining <= 0)
			break;
		for (size_t i = 0; i < aCandidates.size(); i++)
		{
			PhysObj* aPeg = aCandidates[i].get();
			if (aPeg->mPegInfo->mKind == 4)
				continue;
			if (aPass == 0)
			{
				float aDeltaX = aLastX - aPeg->GetXPos();
				float aDeltaY = aLastY - aPeg->GetYPos();
				float aDistanceSquared = (float)((double)(float)(aDeltaX * aDeltaX) + (float)(aDeltaY * aDeltaY));
				if ((double)aMinDistanceSquared > aDistanceSquared)
					continue;
				aLastX = aPeg->GetXPos();
				aLastY = aPeg->GetYPos();
			}
			aPeg->SetPegKind(4);
			if (mState != 0)
				aPeg->mPegInfo->DoFlash(2);
			aRemaining--;
			if (aRemaining == 0)
				break;
		}
		aNeeded = aRemaining;
	}
}

// 0x45dda0
void LogicMgr::InitLevel(bool theResetScores, bool theRandomizeStart, bool theKeepLists)
{
	ThunderballApp* anApp = mBoard->mApp;
	int aPlayer = mCurPlayer;
	if (aPlayer != 0 && aPlayer != 1)
		aPlayer = 0;

	mBoard->mAIMgr->Clear();
	Clear(false, theKeepLists);

	if (anApp->mCurrentPlayer != nullptr)
	{
		mUnknown15C = anApp->mCurrentPlayer->mMaxStage;
		mLevelType = anApp->mCurrentPlayer->mUnknown208;
	}

	if (theResetScores)
	{
		for (int i = 0; i < 2; i++)
		{
			mNumBalls[i] = mTwoPlayer ? TWO_PLAYER_BALLS : ONE_PLAYER_BALLS;
			mScore[i] = 0;
			mScoreAtStart[i] = 0;
		}
	}
	mUnknown134 = 0;

	if (!theKeepLists)
	{
		// the flippers are drawn last (a stable move of them to the end of the list)
		std::list<SharedPtr<PhysObj> > aFlippers;
		std::list<SharedPtr<PhysObj> > aOthers;
		for (std::list<SharedPtr<PhysObj> >::iterator anIter = mBoard->mObjects.begin(); anIter != mBoard->mObjects.end(); ++anIter)
		{
			if ((*anIter)->mId == "flipper")
				aFlippers.push_back(*anIter);
			else
				aOthers.push_back(*anIter);
		}
		aOthers.splice(aOthers.end(), aFlippers);
		mBoard->mObjects.swap(aOthers);
	}
	else
	{
		for (std::list<SharedPtr<PhysObj> >::iterator anIter = mPegs.begin(); anIter != mPegs.end(); ++anIter)
		{
			PegInfo* aPegInfo = (*anIter)->mPegInfo.get();
			aPegInfo->mHitTime = 0;
			aPegInfo->mCrumbleTime = 0;
			aPegInfo->mFlashTime = 0;
			aPegInfo->mFlashType = 0;
		}
	}

	CalcGunAngle(true);

	if (!mBoard->EditingLevel() && !theKeepLists)
	{
		MakeGoalPegs();
		MakeScorePeg();
		MakePowerupPeg();
	}

	mScoreMult = CalcScoreMult((int)mGoalPegs.size());
	mMusicIntensity = CalcMusicIntensity((int)mGoalPegs.size());
	mUnknownF6 = false;
	mUnknownF7 = false;
	mUnknown24 = mTwoPlayer ? 1 : 0;

	if (!mTwoPlayer)
		mCurPlayer = 0;
	else if (!theRandomizeStart)
		mCurPlayer = aPlayer;
	else
	{
		// who shoots first is drawn, but never the same player three levels running
		int aFirst = Rand() % 2;
		mCurPlayer = aFirst;
		if (aFirst == gLastFirstPlayer)
		{
			gFirstPlayerRepeats++;
			if (gFirstPlayerRepeats > 1)
			{
				mCurPlayer = 1 - aFirst;
				gFirstPlayerRepeats = 1;
				gLastFirstPlayer = 1 - aFirst;
			}
		}
		else
		{
			gFirstPlayerRepeats = 1;
			gLastFirstPlayer = aFirst;
		}
	}

	ActivateFreeBallCover(false);
	mBoard->mInterfaceMgr->SettleDown();
	mBoard->mCharacterMgr->SetCurCharacter(mCharacter[mCurPlayer]);

	if (!mPegs.empty())
	{
		if (!theResetScores && mBoard->mResetCount != 1)
			mBoard->mInterfaceMgr->SetNumBalls(mNumBalls[mCurPlayer]);
		else
		{
			mBoard->mInterfaceMgr->SetNumBalls(0);
			mBoard->mInterfaceMgr->AddTopBalls(mNumBalls[mCurPlayer]);
		}
	}

	if (mTwoPlayer && theRandomizeStart)
	{
		// the fever's holes pay half in a game of two
		for (std::list<SharedPtr<PhysObj> >::iterator anIter = mBoard->mObjects.begin(); anIter != mBoard->mObjects.end(); ++anIter)
		{
			if ((*anIter)->mKind == KIND_HOLE && (*anIter)->mId == "fever")
			{
				Hole* aHole = static_cast<Hole*>(anIter->get());
				aHole->mScore /= 2;
			}
		}
	}
	BeginInitLevel();
}

// 0x4612d0
void LogicMgr::ReInitLevel()
{
	NOT_TRANSLATED("LogicMgr::ReInitLevel");
}

// 0x43f020
void LogicMgr::SetCharacters(int theFirst, int theSecond)
{
	mCharacter[0] = theFirst;
	mCharacter[1] = theSecond;
	mBoard->mCharacterMgr->SetCurCharacter(mCharacter[mCurPlayer]);
}

// the flags at +0x244 and +0x245 are one for each player: the computer plays that player (read as +0x244 + the player)
bool LogicMgr::IsComputerTurn() const
{
	return mCurPlayer == 0 ? mUnknown244 : mUnknown245;
}

// 0x43d580
bool LogicMgr::ClearedLevel() const
{
	if (!mUnknownFB)
		return mGoalPegs.size() == 0;
	return mGoalPegs.size() == 0 && mOtherPegs.size() == 0;
}

// 0x43d5d0
bool LogicMgr::MouseMove()
{
	if (mUnknownE8 == 0 && !IsComputerTurn())
	{
		mUnknown70 = MOUSE_WHEEL_MOVE;
		mBoard->mGun->mUnknown17C = 1000.0f;
		mUnknown1F = true;
	}
	return true;
}

// 0x43d620
bool LogicMgr::MouseDrag(int theX, int theY)
{
	if (mUnknownE8 == 0 && !IsComputerTurn() && (theX != mUnknown98 || theY != mUnknown9C))
	{
		mUnknown98 = -10000;
		mUnknown9C = -10000;
		mUnknown70 = MOUSE_WHEEL_MOVE;
		mBoard->mGun->mUnknown17C = 1000.0f;
		mUnknown1F = true;
	}
	return true;
}

// 0x4730d0
void LogicMgr::Update()
{
	if (mUnknownF5 && mState == 2)
	{
		while (!mClickEvents.empty() && mClickEvents.front().mTime <= mStateTime)
		{
			ClickEvent aClick = mClickEvents.front();
			if (!aClick.mIsUp)
				MouseDown(0, 0, aClick.mRight ? -1 : 1, true, false);
			else
				MouseUp(0, 0, aClick.mRight ? -1 : 1, false);
			mClickEvents.pop_front();
		}
	}
	if (mUnknownE8 != 0)
		mUnknownE8--;
	UpdateGun();
	if (mBoard->mUnknownC2)
		return;
	mStateTime++;
	if (mUnknown134 != 0)
		UpdateFreeBall();
	if (mUnknown0C != 0 && --mUnknown0C == 0)
		RemoveHitPegs();
	switch (mState)
	{
	case 1: UpdatePreShot(); break;
	case 2: UpdateShot(); break;
	case 3: UpdatePostShot(); break;
	case 4: UpdateTotalMiss(); break;
	case 5: UpdateLevelDone(); break;
	case 6: UpdatePostPostShot(); break;
	case 7: UpdateShotExtender(); break;
	case 8: UpdateInitLevel(); break;
	case 9: UpdateCharacterDialog(); break;
	case 10: UpdateZenShot(); break;
	}
}

// 0x43d6a0
void LogicMgr::MouseWheel(int theDelta)
{
	NOT_TRANSLATED("LogicMgr::MouseWheel");
}

// 0x4372e0
void LogicMgr::DoFeverMissed()
{
	NOT_TRANSLATED("LogicMgr::DoFeverMissed");
}

// 0x472810: a click. In the state 1 a click of a human player cocks the cannon; in the state 2 the clicks are kept for the replay
bool LogicMgr::MouseDown(int theX, int theY, int theButton, bool theReplayed, bool theForced)
{
	mUnknown98 = theX;
	mUnknown9C = theY;
	if (theButton < 0 && mState != LOGIC_STATE_INIT_LEVEL)
	{
		CalcCornerDisplay();
		return false;
	}
	if (mUnknownF5)
	{
		if (!theReplayed)
			return true;
		theForced = true;
	}
	if (mState == 5 && mUnknownFD && mStateTime < 100)
		NOT_TRANSLATED("LogicMgr::MouseDown, the click that skips the level's end");
	bool aRecorded = false;
	if (!mUnknownF5 && mState == 2)
	{
		ClickEvent aClick = {mStateTime - (theForced ? 1 : 0), false, theButton < 0};
		mClickEvents.push_back(aClick);
		aRecorded = true;
	}
	if (0 < theButton)
	{
		if (mState == 1 && !IsComputerTurn())
		{
			if (mUnknown138 == 0)
				mUnknownED = true;
			return true;
		}
		if (mState != 2)
		{
			if (!mClickEvents.empty() && aRecorded)
				mClickEvents.pop_back();
			SpeedTransition();
			return true;
		}
		if (mUnknown1D)
		{
			mUnknownF7 = true;
			mUnknown1D = SHOT_BOOST_START < mUnknown38;
			return true;
		}
		if (!mBoard->mSlowMo)
		{
			if (IsComputerTurn() && !theForced)
			{
				if (!mClickEvents.empty() && aRecorded)
					mClickEvents.pop_back();
				return true;
			}
		}
		else if (!mUnknownF5)
		{
			mUnknownF9 = false;
			if (mBoard->mUnknownC8 != 0)
				mBoard->mUnknownC8 = 1;
		}
		if (mUnknown20)
		{
			mUnknown20 = false;
			PlayerInfo* aPlayer = mBoard->mApp->mCurrentPlayer;
			if (!mUnknownF5 && aPlayer != nullptr && aPlayer->mUnknown54 < 100)
			{
				aPlayer->mUnknown54++;
				aPlayer->mDirty = true;
			}
		}
		mUnknown1E = true;
	}
	if (mUnknown1D4[FLIPPER_ROW][mCurPlayer] < 1)
	{
		if (mState == 2)
		{
			if (mUnknown1D4[TIME_BOMB_ROW][mCurPlayer] < 1)
			{
				if (mBoard->mApp->GetMaxLevel() == 0 && 0 < theButton && !mUnknownF6 && !mBoard->mSlowMo)
				{
					NOT_TRANSLATED("LogicMgr::MouseDown, the text that says one ball is fired at a time (FloatingText)");
				}
			}
			else
				DoTimeBomb();
		}
	}
	else
		FlipperClick(true);
	return true;
}

// 0x45ebb0
bool LogicMgr::MouseUp(int theX, int theY, int theButton, bool theReplayed)
{
	if (mUnknownF5)
		theReplayed = true;
	bool aRecorded = false;
	if (!mUnknownF5 && mState == 2)
	{
		ClickEvent aClick = {mStateTime, true, theButton < 0};
		mClickEvents.push_back(aClick);
		aRecorded = true;
	}
	bool aWasDown = mUnknown1E;
	if (0 < theButton)
		mUnknown1E = false;
	if (!IsComputerTurn() || theReplayed)
	{
		if (0 < mUnknown1D4[FLIPPER_ROW][mCurPlayer] && 0 < theButton && aWasDown)
			FlipperClick(false);
		CalcCornerDisplay();
	}
	else if (aRecorded && !mClickEvents.empty())
		mClickEvents.pop_back();
	return true;
}

// 0x448370: the corner shows the strongest of what is on, as text in the one string
void LogicMgr::CalcCornerDisplay()
{
	if (mBoard->mWidgetManager->IsRightButtonDown() && !mBoard->mInstantReplayButton->mIsOver && mState != 2)
		mCornerText = "Fast Forward";
	else if (mUnknown68)
		mCornerText = "Triple Score";
	else if (0 < mUnknown1D4[ZEN_ROW][mCurPlayer])
		mCornerText = "ZenBall";
	else if (0 < mUnknown1D4[FIREBALL_ROW][mCurPlayer])
		mCornerText = "Fireball";
	else if (mUnknown69)
		mCornerText = "Magic Hat";
	else
		mCornerText = "";
}

void LogicMgr::SpeedTransition()
{
	NOT_TRANSLATED("LogicMgr::SpeedTransition");
}

void LogicMgr::DoTimeBomb()
{
	NOT_TRANSLATED("LogicMgr::DoTimeBomb");
}

// 0x46d630: the cannon follows the aim; a click cocks it, and a cocked cannon with a ball fires
void LogicMgr::UpdateGun()
{
	Gun* aGun = mBoard->mGun.get();
	if (aGun == nullptr)
		return;
	if (mUnknown1F && !mUnknownF5)
	{
		mUnknown1F = false;
		EndLevelDialog* aDialog = mBoard->mEndLevelDialog;
		if ((aDialog->mWidgetManager == nullptr || !aDialog->mVisible) && !mBoard->mUnknownEA)
			CalcGunAngle(false);
	}
	if (aGun->SetAngle(mGunAngle, false) && aGun->mBall && 0 < mUnknown1D4[FIREBALL_ROW][mCurPlayer] && mState == 1)
		NOT_TRANSLATED("LogicMgr::UpdateGun, the fireball's sparks");
	if (mUnknownF0 < 1)
	{
		if (mUnknownED)
		{
			mUnknownED = false;
			mUnknownEC = true;
			mUnknownA0 = mBoard->mWidgetManager->mLastMouseX;
			mUnknownA4 = mBoard->mWidgetManager->mLastMouseY;
			mBoard->mSoundMgr->AddSound(SOUND_CANNONCOCK, 0, 0, 0, 1, -1.0f);
			if (mUnknown1D4[ZEN_ROW][mCurPlayer] < 1)
			{
				mUnknownF0 = CANNON_COCK_DELAY;
				mUnknownEE = CheckDoStealthHelperShot();
				return;
			}
		}
	}
	else
	{
		mUnknownF0--;
		mUnknownED = false;
	}
	if (mUnknownF0 == 0 && mUnknownEC && aGun->mBall && !mBoard->mUnknownC2 && mState != LOGIC_STATE_UNKNOWN10)
	{
		mUnknownA9 = aGun->NeedDrawMouseClick();
		if (mUnknown1D4[ZEN_ROW][mCurPlayer] < 1)
		{
			mUnknownEC = false;
			mUnknownED = false;
			mUnknownF0 = 0;
			SharedPtr<Ball> aBall = aGun->Fire(true);
			mBoard->AddObj(SharedPtr<PhysObj>(aBall.get()));
			BeginShot(true);
			if (mUnknownEE)
				aGun->SetAngle(mGunAngle, true);
			mUnknownE4 = 0;
			mBoard->mSoundMgr->AddSound(aBall->mFireball ? SOUND_FIREBALLSHOT : SOUND_CANNONSHOT, 1);
			AddShotBurst(aBall->mX, aBall->mY, mGunAngle);
		}
		else
		{
			SetState(LOGIC_STATE_UNKNOWN10);
			aGun->SetBouncyGuide(true);
			aGun->mUnknown1A2 = true;
			aGun->SetAngle(mGunAngle, true);
			aGun->UpdateBouncyGuide();
		}
		mUnknownF4 = false;
	}
}

// 0x46ac70: the ball is out: the shot's counters start again, the orange pegs stop blinking, a ball is taken from the column
void LogicMgr::BeginShot(bool theRecord)
{
	mUnknownA8 = false;
	mUnknownAA = false;
	mUnknownFE = false;
	mUnknownF7 = false;
	mUnknown6C = SHOT_NOT_YET;
	SetState(LOGIC_STATE_SHOT);
	mUnknownB4.clear();
	mBoard->BeginShot();
	mPlayerStats[mCurPlayer].mTotalShots++;
	for (PegSet::iterator anItr = mGoalPegs.begin(); anItr != mGoalPegs.end(); ++anItr)
	{
		PegInfo* aPegInfo = (*anItr)->mPegInfo.get();
		if (aPegInfo != nullptr && aPegInfo->mFlashType == PEG_FLASH_BLINK)
		{
			aPegInfo->mFlashType = 0;
			aPegInfo->mFlashTime = 0;
		}
	}
	if (mUnknown34 != 0)
	{
		mBoard->mFloatingTextMgr->EraseText(mUnknown34);
		mUnknown34 = 0;
	}
	IncNumBalls(-1, 0, true);
	mUnknown13C = SHOT_NOT_YET;
	mUnknown140 = -1;
	mUnknown14C = -1;
	mUnknown150 = -1;
	mUnknown2C = Rand();
	mUnknown30 = Rand();
	InterfaceMgr* anInterface = mBoard->mInterfaceMgr.get();
	anInterface->mUnknown38 = -1;
	anInterface->mUnknown30 = SHOT_METER_PEGS - (int)mGoalPegs.size();
	anInterface->mUnknown54 = 0;
	anInterface->mUnknown161 = false;
	mClickEvents.clear();
	mClickRecords.clear();
	if (theRecord)
		mBoard->GetReplayPoint();
}

// 0x45d880: the player in turn has that many balls more (fewer when negative; never below none); the column shows the new ones
void LogicMgr::IncNumBalls(int theBalls, int theDelay, bool theAtBottom)
{
	mNumBalls[mCurPlayer] += theBalls;
	if (0 < theBalls)
	{
		if (!theAtBottom)
			mBoard->mInterfaceMgr->AddTopBalls(theBalls);
		else
			mBoard->mInterfaceMgr->AddBottomBall(theDelay);
	}
	if (mNumBalls[mCurPlayer] < 0)
		mNumBalls[mCurPlayer] = 0;
}

// 0x440830: the free ball catcher's pieces grow or shrink (their movers' radius moves one a step, every other update) to the size mUnknown134 says;
// when none has to move the size is forgotten
void LogicMgr::UpdateFreeBall()
{
	if (mBoard->mUpdateCount % FREE_BALL_UPDATE_DIVISOR == 0)
		return;
	bool aMoved = false;
	for (std::list<SharedPtr<PhysObj> >::iterator anItr = mBoard->mObjects.begin(); anItr != mBoard->mObjects.end(); ++anItr)
	{
		PhysObj* anObject = anItr->get();
		if (!(anObject->mId == "freeballcover" || anObject->mId == "freeball" || anObject->mId == "bumperhole") || !anObject->mMover)
			continue;
		Mover* aMover = anObject->mMover.get();
		float aSize = (float)aMover->mRadius;
		float aGoal = (float)mUnknown134;
		if (aGoal == aSize || aMover->mPeriod <= 0)
			continue;
		aMoved = true;
		int aNewSize;
		if (aGoal < aSize)
		{
			aNewSize = (int)(aSize - 1.0);
			if (aNewSize < mUnknown134)
				aNewSize = mUnknown134;
		}
		else
		{
			aNewSize = (int)(aSize + 1.0);
			if (mUnknown134 < aNewSize)
				aNewSize = mUnknown134;
		}
		aMover->mRadius = aNewSize;
	}
	if (!aMoved)
		mUnknown134 = 0;
}

// (address in the header)
void LogicMgr::RemoveHitPegs()
{
	NOT_TRANSLATED("LogicMgr::RemoveHitPegs");
}

// 0x466f10: the hints of a level the player has not started: at the update 50 (a first game's first shot) or 140 (otherwise)
// the pegs the level's rank points at blink, at 75 the last few orange pegs flash, and a computer player starts thinking
void LogicMgr::UpdatePreShot()
{
	int aMaxLevel = mBoard->mApp->GetMaxLevel();
	int aShots = mPlayerStats[0].mTotalShots;
	if ((mStateTime == PRESHOT_HINT_TIME_NEW && aShots == 0) || (mStateTime == PRESHOT_HINT_TIME && 1 <= aShots))
	{
		int aKind = 2;
		int aBlinks = PRESHOT_DEFAULT_BLINKS;
		if (!mBoard->mApp->OnMaxLevel() || aShots != 0)
		{
			if (aMaxLevel == 0 || aMaxLevel == 1)
				aBlinks = PRESHOT_BLINKS;
		}
		else if (aMaxLevel == 0 || aMaxLevel == 1)
			aBlinks = PRESHOT_BLINKS;
		else if (aMaxLevel == 2 || (aMaxLevel == (aMaxLevel / 5) * 5 && 0 < aMaxLevel && aMaxLevel < 26))
		{
			aKind = 4;
			aBlinks = PRESHOT_BLINKS;
		}
		else if (aMaxLevel == 3)
		{
			aKind = 3;
			aBlinks = PRESHOT_BLINKS;
		}
		if (PRESHOT_MANY_GOAL_PEGS < (int)mGoalPegs.size() && ((aMaxLevel != 0 && aMaxLevel != 1) || 0 < aShots))
			aKind = 0;
		for (std::list<SharedPtr<PhysObj> >::iterator anItr = mPegs.begin(); anItr != mPegs.end(); ++anItr)
		{
			PhysObj* aPeg = anItr->get();
			if (aPeg->mPegInfo && aPeg->mPegInfo->mKind == aKind && aPeg->mVisible)
				aPeg->mPegInfo->DoFlashBlink(aBlinks);
		}
	}
	if (mStateTime == PRESHOT_LAST_PEGS_TIME && (int)mGoalPegs.size() <= PRESHOT_FEW_GOAL_PEGS)
	{
		for (PegSet::iterator anItr = mGoalPegs.begin(); anItr != mGoalPegs.end(); ++anItr)
		{
			PegInfo* aInfo = (*anItr)->mPegInfo.get();
			if (aInfo != nullptr && !aInfo->mHit)
				aInfo->DoFlash(1);
		}
	}
	if (1 < mUnknown138 && ++mUnknown138 == 30)
		mUnknown138 = 0;
	int aThinkTime = mBoard->mReplayCount == 0 ? COMPUTER_THINK_TIME : COMPUTER_THINK_TIME_REPLAY;
	if (IsComputerTurn() && aThinkTime <= mStateTime && !mBoard->mAIMgr->mUnknown4D)
		NOT_TRANSLATED("LogicMgr::UpdatePreShot, the computer starts thinking (AIMgr::BeginThinking)");
}

// (address in the header)
void LogicMgr::UpdateShot()
{
	NOT_TRANSLATED("LogicMgr::UpdateShot");
}

// (address in the header)
void LogicMgr::UpdatePostShot()
{
	NOT_TRANSLATED("LogicMgr::UpdatePostShot");
}

// (address in the header)
void LogicMgr::UpdateTotalMiss()
{
	NOT_TRANSLATED("LogicMgr::UpdateTotalMiss");
}

// (address in the header)
void LogicMgr::UpdateLevelDone()
{
	NOT_TRANSLATED("LogicMgr::UpdateLevelDone");
}

// 0x471600
void LogicMgr::UpdatePostPostShot()
{
	if ((!mTwoPlayer || POST_SHOT_WAIT < mStateTime) && mBoard->mInterfaceMgr->mUnknown160)
	{
		if (ClearedLevel())
			DoLevelDone();
		else
			BeginTurn(true);
	}
}

// 0x44b690
void LogicMgr::UpdateShotExtender()
{
	if (mBoard->mInterfaceMgr->mUnknown160)
		BeginTurn2();
}

// 0x4484a0: the level's pegs come on one to an update (once the picture is in, 30 updates after the start), then the player is
// asked for the master and the stage
void LogicMgr::UpdateInitLevel()
{
	if (mBoard->mUnknownE8 && mStateTime < INIT_LEVEL_PICTURE_WAIT)
		return;
	bool aShown = false;
	for (std::list<SharedPtr<PhysObj> >::iterator anItr = mPegs.begin(); anItr != mPegs.end() && !aShown; ++anItr)
	{
		PhysObj* aPeg = anItr->get();
		if (!aPeg->mVisible && aPeg->mCollidable)
		{
			aPeg->mVisible = true;
			if (aPeg->mPegInfo->mKind != 1)
				aPeg->mPegInfo->DoFlash(1);
			aShown = true;
		}
	}
	if (aShown)
		return;
	PlayerInfo* aPlayer = mBoard->mApp->mCurrentPlayer;
	if (mTwoPlayer && !mUnknown21 && aPlayer != nullptr && aPlayer->mUnknownEE)
		NOT_TRANSLATED("LogicMgr::UpdateInitLevel, the Duel Rules dialog");
	if (mBoard->mApp->GetDialogCount() == 0)
	{
		if (0 < mUnknown28)
		{
			mUnknown28--;
			return;
		}
		CheckDoCharacterDialog();
	}
}


// 0x4709f0: once the master and stage are chosen the first turn begins
void LogicMgr::UpdateCharacterDialog()
{
	if (1 < mStateTime && mBoard->mInterfaceMgr->mUnknown160)
	{
		if (!mBoard->mUnknownEA)
		{
			FinishInitLevelText();
			BeginTurn(false);
		}
		else if (CHARACTER_DIALOG_TEXT_TIME <= mStateTime && mUnknown34 != 0)
			FinishInitLevelText();
	}
}

// (address in the header)
void LogicMgr::UpdateZenShot()
{
	NOT_TRANSLATED("LogicMgr::UpdateZenShot");
}

// 0x467ca0: in the adventure, a player who has beaten no adventure yet is given a helper shot when the balls left are as few as
// the level's rank says (the first levels, with no starting level asked for, get the most)
bool LogicMgr::CheckDoStealthHelperShot()
{
	PlayerInfo* aPlayer = mBoard->mApp->mCurrentPlayer;
	if (mBoard->mGameMode != GAME_MODE_ADVENTURE || aPlayer == nullptr || 1 <= aPlayer->mAdventuresBeaten)
		return false;
	ThunderballApp* anApp = mBoard->mApp;
	int aRank = mLevelType - 1;
	int aBalls = 0;
	if (anApp->mStartStage == 0)
	{
		aRank = mLevelType;
		if (anApp->mStartLevel == 0)
			aBalls = 6;
	}
	if (aRank < 4 || 2 < aBalls)
	{
		if (aRank < 3 || 1 < aBalls)
		{
			if (1 < aRank && aBalls == 0)
				aBalls = 1;
		}
		else
			aBalls = 2;
	}
	else
		aBalls = 3;
	if (mNumBalls[mCurPlayer] <= aBalls)
		NOT_TRANSLATED("LogicMgr::DoHelperShot");
	return false;
}

// 0x46a9c0
void LogicMgr::DoLevelDone()
{
	NOT_TRANSLATED("LogicMgr::DoLevelDone");
}

// 0x46c220: the texts of the start of the level are made to go on their own way (they are the floating texts the level's start made)
void LogicMgr::FinishInitLevelText()
{
	if (mUnknown34 != 0 || mUnknown3C != 0)
		NOT_TRANSLATED("LogicMgr::FinishInitLevelText with a text (FloatingText)");
}

// 0x437360: asks for the stage (an adventure's first ten levels) or the master, and waits in the state 9
void LogicMgr::CheckDoCharacterDialog()
{
	bool aFirstTime = mUnknown100 == false;
	mUnknown100 = false;
	if (mBoard->mApp->mCurrentPlayer != nullptr)
	{
		int aMode = mBoard->mGameMode;
		if (aMode == GAME_MODE_ADVENTURE)
		{
			if (mUnknown15C < 10 /* the player's stage */)
				NOT_TRANSLATED("Board::DoStageDialog");
			NOT_TRANSLATED("Board::DoCharacterDialog");
		}
		else if (aMode == GAME_MODE_QUICK_PLAY || aMode == GAME_MODE_DUEL || aMode == GAME_MODE_CHALLENGE)
			NOT_TRANSLATED("Board::DoCharacterDialog");
	}
	(void)aFirstTime;
	SetState(9);
}

// 0x46e880: a turn begins: the player in turn (the other one when theNextPlayer), the texts, the powers, the pegs that change, a ball in the cannon
void LogicMgr::BeginTurn(bool theNextPlayer)
{
	mBoard->mCharacterMgr->SetYinYangEye(false);
	if (mUnknownF5)
	{
		SetState(5);
		return;
	}
	int aBallsLeft = mNumBalls[0];
	if (mTwoPlayer && 0 < mNumBalls[0])
		aBallsLeft = 1;
	else if (mTwoPlayer)
		aBallsLeft = mNumBalls[1];
	if (aBallsLeft < 1)
	{
		DoLevelDone();
		return;
	}
	if (mUnknown1E)
	{
		if (0 < mUnknown1D4[FLIPPER_ROW][mCurPlayer])
			FlipperClick(false);
		mUnknown1E = false;
	}
	if (mTwoPlayer)
	{
		bool aWasComputer = IsComputerTurn();
		if (theNextPlayer)
		{
			mCurPlayer = 1 - mCurPlayer;
			if (mNumBalls[mCurPlayer] < 1)
				mCurPlayer = 1 - mCurPlayer;
		}
		if (aWasComputer && !IsComputerTurn())
			CalcGunAngle(true);
		mBoard->mInterfaceMgr->SetNumBalls(mNumBalls[mCurPlayer]);
	}
	mBoard->mCharacterMgr->SetCurCharacter(mCharacter[mCurPlayer]);
	DoBeginTurnText();
	DoBeginTurnTip(-1);
	if (!mTwoPlayer && -1 < mUnknown320)
		mUnknown320 = -mUnknown320;
	else
		mUnknown320 = 0;
	mUnknown138 = 0;
	mUnknown108 = 0;
	mUnknown10C = 0;
	mUnknown110 = 0;
	mUnknown11C = 0;
	mUnknown0C = 0;
	mUnknown10 = 0;
	mUnknown14 = 0;
	mUnknown50 = 0;
	mUnknown18 = false;
	mUnknown19 = false;
	mUnknown1A = false;
	mUnknownF8 = false;
	mUnknown20 = false;
	mUnknown68 = false;
	mUnknown69 = false;
	mUnknownFA = false;
	mUnknown55 = false;
	mBoard->mGun->mUnknown1A2 = false;
	mUnknown330 = BEGIN_TURN_X;
	mUnknown338 = BEGIN_TURN_X;
	mUnknown334 = BEGIN_TURN_Y;
	mUnknown33C = BEGIN_TURN_Y;
	ActivatePowerups();
	if (0 < mUnknown12C)
	{
		mUnknown12C = 0;
		MakeScorePeg();
	}
	if (mUnknown1C)
	{
		mUnknown1C = false;
		if (0 < mUnknown24)
		{
			mUnknown24--;
			MakePowerupPeg();
		}
	}
	if (0 < mUnknown1D4[ZEN_ROW][mCurPlayer])
		mUnknown55 = true;
	CalcCornerDisplay();
	BeginTurn2();
}

// 0x44b5b0: the turn's counters are cleared, a ball is in the cannon, the state is 1 (before the shot)
void LogicMgr::BeginTurn2()
{
	if (mUnknownF5)
	{
		SetState(0);
		return;
	}
	mBoard->mAIMgr->Clear();
	SetState(1);
	if (!mBoard->mDebugMgr->mFeverOn)
		ActivateFreeBall(true);
	mUnknown158 = 0;
	mUnknown154 = 0;
	mUnknown3C = 0;
	mUnknown40 = 0;
	mUnknown38 = 0;
	mUnknown44 = 0;
	mUnknown48 = 0;
	mUnknown4C = 0;
	mUnknown138 = 1;
	mUnknown144 = -1;
	mUnknownF7 = false;
	mUnknown1D = false;
	mUnknown1E = false;
	mUnknownF9 = true;
	mBoard->Reload();
	InterfaceMgr* anInterface = mBoard->mInterfaceMgr.get();
	if (!anInterface->mColumnBalls.empty())
	{
		anInterface->mUnknown160 = false;
		anInterface->mBallColumnSink = 1;
	}
	else
		mUnknown138 = 2;
	if (0 < mUnknown1D4[FIREBALL_ROW][mCurPlayer])
		NOT_TRANSLATED("LogicMgr::BeginTurn2, the fireball in the cannon");
	if (mUnknown54)
		SetWearHat(true);
}

// 0x461560: the player in turn's counters of power-ups tick down and each power-up is switched on or off by its counter
void LogicMgr::ActivatePowerups()
{
	int aGiven = mCurPlayer == 0 ? mUnknown1CC : mUnknown1D0;
	if (aGiven != 0)
		mUnknown1D4[aGiven][mCurPlayer] = POWERUP_TURNS_GIVEN;
	for (int aPower = 0; aPower < NUM_POWERUPS; aPower++)
	{
		if (0 < mUnknown1D4[aPower][mCurPlayer])
			mUnknown1D4[aPower][mCurPlayer]--;
		ActivatePowerup(aPower, mUnknown1D4[aPower][mCurPlayer] != 0);
	}
	int& aTripleScore = mCurPlayer == 0 ? mUnknown58 : mUnknown5C;
	if (aTripleScore != 0)
	{
		mUnknown68 = true;
		aTripleScore--;
	}
	int& aMagicHat = mCurPlayer == 0 ? mUnknown60 : mUnknown64;
	if (aMagicHat != 0)
	{
		mUnknown69 = true;
		aMagicHat--;
	}
}

// 0x4612e0
void LogicMgr::ActivatePowerup(int thePower, bool theActive)
{
	switch (thePower)
	{
	case 1:
		if (mState != 2)
			mBoard->mGun->SetBouncyGuide(theActive);
		break;
	case 2:
		if (!theActive)
			FlipperClick(false);
		else
			NOT_TRANSLATED("LogicMgr::ClearFlipperSpace");
		for (std::list<SharedPtr<PhysObj> >::iterator anItr = mBoard->mObjects.begin(); anItr != mBoard->mObjects.end(); ++anItr)
		{
			PhysObj* anObject = anItr->get();
			if (anObject->mId == "flipper")
			{
				anObject->SetActiveWithGrowAnim(theActive);
				if (theActive && mState == 2 && anObject->mKind == KIND_POLY)
					static_cast<Poly*>(anObject)->mCountdown = FLIPPER_COUNTDOWN;
				if (!anObject->mMover)
					anObject->mCollidable = false;
			}
		}
		break;
	case 4:
		if (!theActive)
			mBoard->mEffectMgr->EraseAllOfType(EFFECT_TYPE_FEVER_SPARK);
		else if (mState == 2)
			ActivateFreeBall(true);
		break;
	case 7:
		if (theActive && mState != 2)
			mBoard->mCharacterMgr->SetYinYangEye(true);
		break;
	case 10:
		mUnknown148 = theActive ? 100 : 0;
		break;
	case 12:
		if (mState != 2)
			ActivateFreeBallCover(theActive);
		break;
	}
	if (thePower == 2 || thePower == 4)
	{
		if (mUnknown1D4[FLIPPER_ROW][mCurPlayer] != 0)
			mUnknown134 = FREE_BALL_COVER_TIME_FLIPPERS;
		else
			mUnknown134 = mUnknown1D4[4][mCurPlayer] != 0 ? FREE_BALL_COVER_TIME_FEVER : FREE_BALL_COVER_TIME;
	}
}

// 0x440580: the pieces named bumperhole (the bumper holes) and freeball (the ball catcher) are switched on or off. The bumper holes
// grow in when the fever power is on.
void LogicMgr::ActivateFreeBall(bool theActive)
{
	if (mUnknownF6 && theActive)
		return;
	bool aGrow = theActive && 0 < mUnknown1D4[FEVER_ROW][mCurPlayer];
	for (std::list<SharedPtr<PhysObj> >::iterator anItr = mBoard->mObjects.begin(); anItr != mBoard->mObjects.end(); ++anItr)
	{
		PhysObj* anObject = anItr->get();
		if (anObject->mId == "bumperhole")
		{
			anObject->SetActiveWithGrowAnim(aGrow);
			if (anObject->mKind == KIND_POLY && anObject->mSubId == 1 && mState == 2)
				static_cast<Poly*>(anObject)->mCountdown = BUMPER_HOLE_COUNTDOWN;
			if (anObject->mSubId == 1)
				anObject->mCollidable = false;
			else
				anObject->mVisible = false;
		}
		else if (anObject->mId == "freeball")
		{
			anObject->SetActive(theActive);
			if (anObject->mKind != KIND_HOLE)
			{
				if (!anObject->mForeground)
					anObject->mVisible = false;
				else
					anObject->mCollidable = false;
			}
		}
	}
}

// 0x469f30: a turn with fewer than 4 balls left (or of two players) says how many are left
void LogicMgr::DoBeginTurnText()
{
	if (mTwoPlayer || mNumBalls[mCurPlayer] < 4)
		NOT_TRANSLATED("LogicMgr::DoBeginTurnText (FloatingText)");
}

// 0x46a370: the tips of a new player: which one comes, by what the player did and the level
void LogicMgr::DoBeginTurnTip(int theTip)
{
	PlayerInfo* aPlayer = mBoard->mApp->mCurrentPlayer;
	if (aPlayer == nullptr || mTwoPlayer || IsComputerTurn())
		return;
	int aMaxLevel = mBoard->mApp->GetMaxLevel();
	if (theTip < 0)
	{
		if (mUnknown19 && aPlayer->SetTip(0))
			theTip = 0;
		else if (mUnknown1A && aPlayer->SetTip(1))
			theTip = 1;
		else if (mUnknown1B && aPlayer->SetTip(2))
			theTip = 2;
		else if (mNumBalls[0] == 4 && aPlayer->SetTip(3))
			theTip = 3;
		else if (mBoard->mGameMode == GAME_MODE_ADVENTURE && aMaxLevel == 0 && (int)mGoalPegs.size() <= TIP_FEW_GOAL_PEGS &&
				 aPlayer->SetTip(4))
			theTip = 4;
		else if (2 <= mUnknown1D4[1][0] && aPlayer->SetTip(5))
			theTip = 5;
		else if (aMaxLevel <= 9 && ((mUnknown1B && aPlayer->SetTip(7)) || (mUnknownAA && aPlayer->SetTip(8))))
			theTip = 8;
	}
	if (theTip >= 0)
		NOT_TRANSLATED("LogicMgr::DoBeginTurnTip, the tip's text (FloatingText)");
}

// 0x43d4d0
void LogicMgr::SetWearHat(bool theWear)
{
	mUnknown54 = theWear;
	if (mBoard->mGun->mBall)
		NOT_TRANSLATED("Ball::SetHat");
}

// 0x45ea70: the flippers' paddles go up or down; with none there is nothing
void LogicMgr::FlipperClick(bool theDown)
{
	for (std::list<SharedPtr<PhysObj> >::iterator anItr = mBoard->mObjects.begin(); anItr != mBoard->mObjects.end(); ++anItr)
	{
		PhysObj* anObject = anItr->get();
		if (anObject->mId == "flipper" && anObject->mMover && 0 < anObject->mMover->mPeriod)
		{
			// a flipper at the start of its cycle is not let go (down) and one that is let go moves on (the mover's own functions 0x479980, 0x4799a0)
			if (!theDown && anObject->mMoverTime % anObject->mMover->mPeriod < 1)
				continue;
			NOT_TRANSLATED("LogicMgr::FlipperClick moving a flipper");
		}
	}
}
