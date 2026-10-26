#include "Dynomite.h"
#include "FossilizedApp.h"

using namespace Sexy;

// 0x404030 (work in progress: the fields the translated code uses so far)
Dynomite::Dynomite()
{
	mApp = NULL;
	mBoard5c = NULL;
	mDifficulty = 0;
	mGameType = 0;
	mField130 = false;
	mField131 = false;
	mPauseCount = 0;
}

// 0x4041e0 (TODO)
Dynomite::~Dynomite()
{
}

// 0x404fb0
void Dynomite::SetPaused(bool isPaused)
{
	if (isPaused == true)
		mPauseCount++;
	else
		mPauseCount--;
	if (mPauseCount < 0)
		mPauseCount = 0;
	SetActive(mPauseCount == 0);
}

// 0x4047e0 (TODO)
void Dynomite::SetActive(bool isActive)
{
}

// 0x405010 (TODO: the 0x1c-byte dynamite, 0x407e00, at +0x154)
void Dynomite::MakeDynamite()
{
}

// 0x40ad90 (TODO)
void Dynomite::Hint()
{
}
