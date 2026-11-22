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
#include "Gun.h"
#include "Hole.h"
#include "InterfaceMgr.h"
#include "NotTranslated.h"
#include "PegInfo.h"
#include "PlayerInfo.h"
#include "SoundInstance.h"
#include "SoundMgr.h"
#include "ThunderMath.h"
#include "ThunderballApp.h"

#include <algorithm>
#include <cmath>

using namespace Sexy;

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
	mSongFolder = "";
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
	mSongFolder = ""; // (the original makes a "pegsongs/" string here and does not use it)
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
	mUnknownC4.clear();
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
