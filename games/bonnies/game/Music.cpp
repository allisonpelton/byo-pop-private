// The music (see Music.h).

#include "Music.h"
#include "BonniesApp.h"
#include "Exe.h"
#include "GameScreen.h"
#include "Globals.h"
#include "Host.h"
#include "Platform.h"
#include "Sound.h"

namespace bonnies
{

MusicTrack gTracks[10];

static uint32_t ticks()
{
	return host()->getTicks();
}

// 0x40dec0
Music::Music()
{
	// The table as the static initializers left it
	for (int i = 0; i < 10; i++)
	{
		uint32_t anAddress = 0x473c70 + i * 0x2c;
		gTracks[i].mPlayed = exeI32(anAddress);
		gTracks[i].mKind = exeI32(anAddress + 4);
		gTracks[i].mModule = exeI32(anAddress + 8);
		gTracks[i].mBegin = exeI32(anAddress + 0xc);
		gTracks[i].mEnd = exeI32(anAddress + 0x10);
		gTracks[i].mStartOrder = exeI32(anAddress + 0x14);
		gTracks[i].mStopOrder = exeI32(anAddress + 0x18);
		gTracks[i].mName = Str(exeGlobalString(anAddress + 0x1c));
		gTracks[i].mStr24 = Str(exeGlobalString(anAddress + 0x24));
	}
	mField00 = 1;
	mKind = 0;
	mIndex = 0;
	mCount = 0;
	mFadeStart = 0;
	mFadeEnd = 0;
	mNextKind = 0;
	mTime = ticks();
	mEndedTime = 0;
}

// 0x42f0a0
Music::~Music()
{
	mField00 = 0;
}

// 0x40ddc0
void Music::play(int theIndex)
{
	if (-1 < theIndex && (unsigned)theIndex < 10 && isInactive() == 0)
	{
		mKind = gTracks[theIndex].mKind;
		mIndex = theIndex;
		gTracks[theIndex].mPlayed = mCount;
		mCount++;
		if (!(gPlatform->getMusicVolume() < 0.0099999998f))
		{
			if (gTracks[theIndex].mModule != 0)
				gSound->playMusic(gTracks[theIndex].mBegin, gTracks[theIndex].mEnd, gTracks[theIndex].mStartOrder,
					gTracks[theIndex].mStopOrder, 1.0f);
			else
				gSound->playStream(gTracks[theIndex].mName, 1.0f);
		}
		mTime = ticks();
		mEndedTime = 0;
	}
}

// 0x40de90
void Music::stop()
{
	mKind = 0;
	gSound->stopMusic();
	mEndedTime = 0;
	mTime = ticks();
}

// 0x40def0
void Music::select(int theKind, int theNow)
{
	if (theKind == mKind && gSound->isMusicPlaying())
		return;
	if (isInactive() != 0)
		return;
	// (the original also asks 0x42a4d0, which answers 0, when theKind is 2)
	if (((theKind == 2 && gPlatform->Platform::isPaused() != 0) || gSound->isMusicPlaying()) && theNow == 0)
	{
		mFadeStart = ticks();
		mNextKind = theKind;
		mFadeEnd = ticks() + (mKind != 1 ? 0x4b0 : 0) + 400;
		return;
	}
	mFadeStart = 0;
	mFadeEnd = 0;
	mNextKind = 0;
	int aBest = -1;
	int aBestScore = -999999999;
	for (int i = 0; i < 10; i++)
	{
		if (gTracks[i].mKind == theKind)
		{
			int aScore = randomRange(0, 400) + gTracks[i].mPlayed * -100;
			if (i == mIndex)
				aScore = aScore - 500000;
			if (aBestScore < aScore)
			{
				aBestScore = aScore;
				aBest = i;
			}
		}
	}
	if (aBest == -1)
	{
		mKind = 0;
		gSound->stopMusic();
		mEndedTime = 0;
		mTime = ticks();
		return;
	}
	play(aBest);
}

// 0x40e020
void Music::setVolume(float theFactor)
{
	gSound->setMusicVolume(theFactor);
	mTime = ticks();
	if (!(gPlatform->getMusicVolume() < 0.0099999998f) && !gSound->isMusicPlaying() && mKind != 0)
		select(mKind, 0);
}

// 0x40e080
void Music::update()
{
	if (mFadeStart != 0)
	{
		if (mKind == 1)
		{
			if (gPlatform->Platform::isPaused() != 0)
				return;
			select(mNextKind, 1);
			return;
		}
		if ((int)(mFadeEnd + 600) < (int)ticks())
		{
			select(mNextKind, 1);
			return;
		}
		if ((int)(ticks() - mTime) < 0x32)
			return;
		int aSpan = 1;
		if (1 <= (int)(mFadeEnd - mFadeStart))
			aSpan = (int)(mFadeEnd - mFadeStart);
		double aLeft = (double)(int)(mFadeEnd - ticks()) / aSpan + 1.0f;
		double aVolume = (aLeft * aLeft - 1.0f) * 0.33333334f;
		if (aVolume < 0.0099999998f)
		{
			stop();
			mTime = ticks();
			return;
		}
		if (1.0f < aVolume)
			aVolume = 1.0f;
		gSound->setMusicVolume((float)aVolume);
		mTime = ticks();
		return;
	}
	if (mEndedTime != 0)
	{
		if (mKind == 1)
		{
			if (gGameScreen != nullptr)
				select(3, 0);
			else if (gBonnies->mState != 1)
				select(2, 0);
		}
		if (mKind == 3 || mKind == 2)
		{
			if (!gSound->isMusicPlaying())
			{
				if ((int)(mEndedTime + 700) < (int)ticks())
					select(mKind, 0);
			}
			else
				mEndedTime = 0;
		}
	}
	if (499 < (int)(ticks() - mTime))
	{
		mTime = ticks();
		if (!gSound->isMusicPlaying() && mEndedTime == 0)
			mEndedTime = ticks();
	}
}

} // namespace bonnies
