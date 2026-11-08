#include "RedShark.h"
#include "WinWSApp.h"
#include "Image.h"
#include "Res.h"

using namespace Sexy;

// Not translated (vtordisp thunks and the part destructors): 0x404b80 (part destructor), 0x404c10, 0x406560,
// 0x406ef0 (SetState: BlackShark's), 0x40e030, 0x409970 (deleting destructor), 0x4099a0 (destructor of the whole
// object). GameObject slot 7 (0x4065d0) and Shark part slot 0 (0x4035f0) are BlackShark's.

// 0x411400: BlackShark (0x410f50) first, then the RedShark part. The pictures are the ones the export names
// *_BLACK_* (see BlackShark.cpp)
RedShark::RedShark(WinWSApp* theApp) :
	Shark(theApp),
	BlackShark(theApp)
{
	mTextOffsetX = 4;
	mTextOffsetY = 1;
	mTypedColor = Color(255, 255, 255);
	mTextColor = Color(0, 0, 0);
	mImages[0] = mApp->mImages[IMAGE_SHARK_BLACK_SWIM];
	mImages[3] = mApp->mSharkImages[1][3];
	mImages[2] = mApp->mImages[IMAGE_SHARK_BLACK_DEATH];
	mImages[4] = mApp->mSharkImages[1][4];
	mImages[1] = mApp->mImages[IMAGE_SHARK_BLACK_SHOCK];
	// the original looks up mSharkImages[1][4] once more here and drops the result
	mImage = mImages[0];
}

RedShark::~RedShark()
{
}

// 0x407170: returns a copy of the string at +0x24
std::string RedShark::GetWord3()
{
	return mWord3;
}

// 0x40b600: BlackShark::TypeLetter with a third word: when mWord is finished the second word is taken, when that is
// empty the third one; with both empty the word is cleared and the creature lets go of its target
void RedShark::TypeLetter()
{
	mNumTyped++;
	if (mNumTyped == (int)mWord.size())
	{
		mField54 = GetPoints();
		mEvents.push_back(1);
		if (mWord2.empty())
		{
			if (mWord3.empty())
			{
				mTyped = "";
				mRemaining = "";
				mWord = "";
				ReleaseTarget();
				if (mAttacker != NULL)
					mAttacker->SetTarget(NULL);
				return;
			}
			mWord = mWord3;
			mNumTyped = 0;
		}
		else
		{
			mWord = mWord2;
			mNumTyped = 0;
		}
	}
	mTyped = mWord.substr(0, mNumTyped);
	mRemaining = mWord.substr(mNumTyped, mWord.size());
}
