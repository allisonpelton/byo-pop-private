#include "BlackShark.h"
#include "WinWSApp.h"
#include "Image.h"
#include "MTRand.h"
#include "Res.h"

using namespace Sexy;

// Not translated (vtordisp thunks and the part destructors): 0x403510 (part destructor), 0x4035a0, 0x4035b0, 0x4035c0,
// 0x4035d0, 0x4035e0, 0x406550, 0x406ee0, 0x40dfc0, 0x40dfd0 (jump to Shark's and GameObject's functions with the
// pointer adjusted), 0x409d70 (deleting destructor), 0x409da0 (destructor of the whole object)

// 0x410f50: Shark (0x4101d0, GameObject 0x4611b0 and Enemy 0x40a3f0 first), then the BlackShark part.
// The pictures are the ones the export names *_RED_*: the original's own ids (the black shark uses ids 0x40, 0x41 and
// 0x48; RedShark the ones named *_BLACK_*)
BlackShark::BlackShark(WinWSApp* theApp) :
	Shark(theApp)
{
	mTextOffsetY = 4;
	mTextOffsetX = 0;
	mImages[0] = mApp->mImages[IMAGE_SHARK_RED_SWIM];
	mImages[3] = mApp->mSharkImages[2][3];
	mImages[2] = mApp->mImages[IMAGE_SHARK_RED_DEATH];
	mImages[4] = mApp->mSharkImages[2][4];
	mImages[1] = mApp->mImages[IMAGE_SHARK_RED_SHOCK];
	mImage = mImages[0];
}

BlackShark::~BlackShark()
{
}

// 0x407030: returns a copy of the string at +8
std::string BlackShark::GetWord2()
{
	return mWord2;
}

// 0x4035f0 (Shark part slot 0): Shark::IsSwimming also answers true for a shark that is not swimming but still has a
// word, unless mFlag50 is set
bool BlackShark::IsSwimming()
{
	if (mState != 0 && mState != 6)
	{
		if (mWord.empty() || mFlag50)
			return false;
	}
	return true;
}

// 0x4065d0: Shark::SetState (0x404c20) with two changes. The shocked state (5) is done here, with Shark's pictures and
// sizes, and the sample is 35 (hurt) while the word is not empty instead of 3 (dies). Dying (3) with a word left (the
// second word) does not kill the shark: it swims again in state 0 with a new random frame
void BlackShark::SetState(int theState)
{
	if (theState == 6)
	{
		if (mState != 0 && mState != 6)
			return;
	}
	else if (theState == 5)
	{
		if (mState == 4)
			return;

		mState = 5;
		mFrame = 0;
		mImage = mImages[1];
		mFlag30 = true;
		mField28 = 7;
		mField2c = 6;
		if (!mFlag50)
		{
			if (mWord.empty())
				mApp->PlaySample(SOUND_SHARKDIES);
			else
				mApp->PlaySample(SOUND_SHARKHURT);
		}
		mFrameHeight = mImages[1]->GetHeight();
		mFrameWidth = mImages[1]->GetWidth();
		mFrame = 0;
		mImage = mImages[1];
		mFrameOffset = mFrame * mFrameWidth;
		return;
	}

	Shark::SetState(theState);

	// port: the original also has a branch for theState == 5 here (mFrame = 0, mImage = mImages[1]); the state 5 has
	// returned above, so it is never taken
	if (theState == 6)
		mImage = mImages[0];
	else if (theState == 3)
	{
		mImage = mImages[2];
		if (!mWord.empty() && !mFlag50)
		{
			mFrameDelay = 11;
			mVelY = 0.0;
			mState = 0;
			mNumFrames = 20;
			mFrame = gGameRand->NextNoAssert() % mNumFrames;
			mImage = mImages[0];
			mFrameWidth = mImages[0]->GetWidth() / 20;
			mFrameHeight = mImages[0]->GetHeight();
		}
	}
	else if (theState == 1)
		mImage = mImages[3];
	else if (theState == 4)
		mImage = mImages[0];

	mFrameOffset = mFrame * mFrameWidth;
}

// 0x409fb0: Enemy::TypeLetter (0x40a5b0) with a second word. When the last letter is typed the points of the word are
// kept (mField54) and event 1 pushed; a second word then becomes the word and typing starts again at its first letter,
// without one the word is cleared and the creature lets go of its target
void BlackShark::TypeLetter()
{
	mNumTyped++;
	if (mNumTyped == (int)mWord.size())
	{
		mField54 = GetPoints();
		mEvents.push_back(1);
		if (mWord2.empty())
		{
			mWord = "";
			mTyped = "";
			mRemaining = "";
			ReleaseTarget();
			if (mAttacker != NULL)
				mAttacker->SetTarget(NULL);
			return;
		}
		mWord = mWord2;
		mNumTyped = 0;
	}
	mTyped = mWord.substr(0, mNumTyped);
	mRemaining = mWord.substr(mNumTyped, mWord.size());
}
