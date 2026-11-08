#include "Enemy.h"
#include "WinWSApp.h"
#include "ImageFont.h"

using namespace Sexy;

// 0x40a3f0: GameObject (0x4611b0) first, then the Enemy part
Enemy::Enemy(WinWSApp* theApp) :
	mFlag08(false),
	mApp(theApp),
	mTarget(NULL),
	mAttacker(NULL),
	mFlag18(false),
	mFlag19(false),
	mFlag1a(false),
	mFlag1b(true),
	mFlag1c(true),
	mFlag1d(true),
	mField20(0),
	mField24(0),
	mField28(0),
	mField2c(0),
	mFlag30(false),
	mFlag50(false),
	mField54(0),
	mNumTyped(0),
	mImages(6, (Image*)NULL),
	mImage(NULL),
	mFlagD0(false)
{
	mFont = mApp->mFontMyriadCondensed15;
	mTypedFont = mApp->mFontMyriadCondensed15b;
}

Enemy::~Enemy()
{
}

// 0x403130: each letter is worth 1 to 4 (the common letters little, the rare ones more), the word is worth the sum times 15
int Enemy::GetPoints()
{
	int aSum = 0;
	unsigned int i = 0;
	if (mWord.size() != 0)
	{
		do
		{
			switch (mWord[i])
			{
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
			case 'C': case 'O': case 'P': case 'Q': case 'W': case 'X': case 'Z':
				aSum += 3;
				break;
			default:
				aSum += 4;
				break;
			case 'A': case 'D': case 'E': case 'F': case 'G': case 'H': case 'J': case 'K': case 'L': case 'S':
				aSum += 1;
				break;
			case 'B': case 'I': case 'M': case 'N': case 'R': case 'T': case 'U': case 'V': case 'Y':
				aSum += 2;
			}
			i++;
		} while (i < mWord.size());
	}
	return aSum * 15;
}

// 0x4011f0
double Enemy::GetSpeed()
{
	return mVelX;
}

// 0x40a5b0: the last letter is never counted: when the word is complete the creature reports event 1 and lets go of its
// target, and mNumTyped stays at the last position
void Enemy::TypeLetter()
{
	int aOld = mNumTyped;
	mNumTyped = aOld + 1;
	if (mNumTyped == (int)mWord.size())
	{
		mNumTyped = aOld;
		mEvents.push_back(1);
		ReleaseTarget();
		if (mAttacker != NULL)
			mAttacker->SetTarget(NULL);
	}
	mTyped = mWord.substr(0, mNumTyped);
	mRemaining = mWord.substr(mNumTyped, mWord.size());
}

// 0x4022d0: let go of the creature this one is tied to. A target that is waiting to the right of the screen, still at
// state 0 and hidden, is put at the right edge and shown
void Enemy::ReleaseTarget()
{
	if (mTarget != NULL)
	{
		mTarget->SetAttacker(NULL);
		double aWidth = (double)mApp->mWidth;
		if (mTarget->mState == 0 && aWidth < mTarget->mX && !mTarget->mVisible)
		{
			mTarget->SetX(aWidth);
			mTarget->SetVisible(true);
		}
		mTarget = NULL;
	}
}
