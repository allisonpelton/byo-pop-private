// PegInfo.cpp: see PegInfo.h. The original keeps its tweakable numbers in ModVal calls (each returns the number it was
// compiled with); they are named constants here, each with the line of PegInfo.cpp its call names.

#include "PegInfo.h"

#include "DataSync.h"
#include "PhysObj.h"

using namespace Sexy;

static const int CRUMBLE_START = 3;			   // the counter of a crumbling peg runs from here (set where the peg is hit)
static const int CRUMBLE_END = 100;			   // and the piece goes inactive when it reaches this
static const int FLASH_END = 100;			   // a flash that counts up ends here
static const int FLASH_TYPE_BLINK = 10;		   // the kind of flash that counts down
static const int FLASH_BLINK_LENGTH = 50;	   // line 152 (PegInfo.cpp:61): the updates of one blink

// 0x4765f0
PegInfo::PegInfo()
{
	mHit = false;
	mUnknown17 = false;
	mCrumble = false;
	mHitTime = 0;
	mFlashTime = 0;
	mFlashType = 0;
	mCrumbleTime = 0;
	mKind = 1;
	mVariable = true;
}

// 0x476650
void PegInfo::SetHit(bool theHit)
{
	mHit = theHit;
	mHitTime = theHit ? 1 : 0;
}

// 0x476670
void PegInfo::DoFlash(int theFlashType)
{
	mFlashTime = 1;
	mFlashType = theFlashType;
}

// 0x476690
void PegInfo::DoFlashBlink(int theBlinks)
{
	mFlashTime = FLASH_BLINK_LENGTH * theBlinks - 1;
	mFlashType = FLASH_TYPE_BLINK;
}

// 0x479680
void PegInfo::Update(PhysObj* thePiece)
{
	if (mHitTime != 0)
	{
		mHitTime++;
		if (mCrumble && mCrumbleTime >= CRUMBLE_START && mCrumbleTime < CRUMBLE_END)
		{
			mCrumbleTime++;
			if (mCrumbleTime == CRUMBLE_END)
				thePiece->SetActive(false);
		}
	}

	if (mFlashTime != 0)
	{
		if (mFlashType == FLASH_TYPE_BLINK)
		{
			mFlashTime--;
			if (mFlashTime < 1)
			{
				mFlashType = 0;
				mFlashTime = 0;
			}
		}
		else
		{
			mFlashTime++;
			if (mFlashTime > FLASH_END)
				mFlashTime = 0;
		}
	}
}

// 0x479560
void PegInfo::SyncState(DataSync& theSync)
{
	if (theSync.IsReading())
	{
		mHitTime = 0;
		mFlashType = 0;
		mFlashTime = 0;
		mCrumbleTime = 0;
	}

	bool aHasFlashTime = mFlashTime != 0;
	bool aHasFlashType = mFlashType != 0;
	bool aHasCrumbleTime = mCrumbleTime != 0;
	bool aHasHitTime = mHitTime != 0;

	theSync.SyncByte(mKind);
	theSync.SyncBoolBit(mHit);
	theSync.SyncBoolBit(mVariable);
	theSync.SyncBoolBit(aHasHitTime);
	theSync.SyncBoolBit(mCrumble);
	theSync.SyncBoolBit(aHasFlashTime);
	theSync.SyncBoolBit(aHasFlashType);
	theSync.SyncBoolBit(mUnknown17);
	theSync.SyncBoolBit(aHasCrumbleTime);
	theSync.EndBit();

	if (aHasHitTime)
		theSync.SyncLong(mHitTime);
	if (aHasFlashTime)
		theSync.SyncLong(mFlashTime);
	if (aHasFlashType)
		theSync.SyncByte(mFlashType);
	if (aHasCrumbleTime)
		theSync.SyncByte(mCrumbleTime);
}
