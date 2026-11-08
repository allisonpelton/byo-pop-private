#include "WhitePiranha.h"
#include "WinWSApp.h"
#include "Image.h"
#include "MTRand.h"
#include "Res.h"

using namespace Sexy;

// Not translated (vtordisp thunks): 0x410e30, 0x410e40, 0x410e50, 0x410e60, 0x410f30, 0x410f40 (jump to Piranha's and
// GameObject's functions with the pointer adjusted), 0x410ed0 (deleting destructor)

// 0x410c70: Piranha (0x40fca0, GameObject 0x4611b0 and Enemy 0x40a3f0 first), then the WhitePiranha part
WhitePiranha::WhitePiranha(WinWSApp* theApp, int theType) :
	Piranha(theApp, theType)
{
	mImages[0] = mApp->mImages[IMAGE_PIRANHA_WHITE_SWIM];
	mImages[2] = mApp->mImages[IMAGE_PIRANHA_WHITE_DEATH];
	mImages[3] = mApp->mPiranhaImages[6][3];
	mImage = mImages[0];
}

WhitePiranha::~WhitePiranha()
{
}

// 0x410e70: the one place where the second word is set from outside, by a string passed by value
void WhitePiranha::SetWord2(std::string theWord)
{
	mWord2 = theWord;
}

// 0x405f50 (Piranha part slot 0): true while swimming (state 0); in the other states only when there is a word and
// mFlag50 is not set. (Piranha::IsSwimming also wants the piranha visible.)
bool WhitePiranha::IsSwimming()
{
	if (mState != 0)
	{
		if (mWord.empty() || mFlag50)
			return false;
	}
	return true;
}

// 0x405e10: Piranha::SetState (0x404950); dying (3) with a word left does not kill the piranha: it swims again in
// state 0 with a new random frame. mFrameOffset is not recomputed (Piranha::SetState set it for the death picture)
void WhitePiranha::SetState(int theState)
{
	Piranha::SetState(theState);
	if (theState == 3)
	{
		if (!mWord.empty() && !mFlag50)
		{
			mFrameDelay = 5;
			mVelY = 0.0;
			mState = 0;
			mNumFrames = 20;
			mFrame = gGameRand->NextNoAssert() % mNumFrames;
			mImage = mImages[0];
			mFrameWidth = mImages[0]->GetWidth() / 20;
			mFrameHeight = mImages[0]->GetHeight();
		}
	}
}

// 0x40d9c0: replaces Enemy::TypeLetter and does not count a letter up. Event 1 is pushed. With a second word it
// becomes the word (mNumTyped 0) and mWord2 is cleared; without one the creature lets go of its target and the word
// is cleared, with mNumTyped counted down by one. The points of the word that is left (mField54) are worked out last
void WhitePiranha::TypeLetter()
{
	mEvents.push_back(1);
	if (mWord2.empty())
	{
		ReleaseTarget();
		if (mAttacker != NULL)
			mAttacker->SetTarget(NULL);
		mNumTyped--;
		mWord = "";
	}
	else
	{
		mWord = mWord2;
		mNumTyped = 0;
		mWord2 = "";
	}
	if (!mWord.empty())
		mField54 = GetPoints();
}
