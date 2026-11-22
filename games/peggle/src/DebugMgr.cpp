// DebugMgr.cpp: see DebugMgr.h.

#include "DebugMgr.h"

#include "Ball.h"
#include "NotTranslated.h"

using namespace Sexy;

// 0x43ffa0
DebugMgr::DebugMgr(Board* theBoard)
{
	mBoard = theBoard;
	mUnknown08 = nullptr;
	mUnknown04 = false;
	mFeverOn = false;
	mFlippersOn = false;
	Clear();
	mNumGoalPegsLeft = -1;
	mNumPegsLeft = -1;
}

// 0x43e470
DebugMgr::~DebugMgr()
{
}

// 0x43e4a0
void DebugMgr::Clear()
{
	mUnknown08 = nullptr;
	mUnknown1C = 0;
	mUnknown20 = false;
	mUnknown21 = false;
}

// 0x446670
void DebugMgr::SyncNumDebugPegs()
{
	// (the original also counts the calls in a global at 0x650a38, which nothing reads)
	if (mNumPegsLeft >= 0 || mNumGoalPegsLeft >= 0)
		NOT_TRANSLATED("DebugMgr::SyncNumDebugPegs with a number of pegs to leave");
}

// 0x464930
void DebugMgr::InitLevel()
{
	SyncNumDebugPegs();
	if (mFeverOn)
		NOT_TRANSLATED("DebugMgr::SyncFeverState");
	if (mFlippersOn)
		NOT_TRANSLATED("DebugMgr::SyncFlipperState");
}

// 0x44dcd0
void DebugMgr::Update()
{
	NOT_TRANSLATED("DebugMgr::Update");
}
