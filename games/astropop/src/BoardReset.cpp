// Board: the reset for a new game (the numbers every game starts from, the backdrop's pictures)

#include "Board.h"
#include "Brick.h"
#include "AstroPopApp.h"
#include "ResourceManager.h"
#include "Image.h"
#include "Res.h"
#include <vector>

using namespace Sexy;

// 0x450250: everything a new game (or a new try of a level) starts from. It draws random numbers: three different
// pictures of the backdrop (IMAGE_BG_OBJECT1 .. 7) and where the pictures are across the screen.
void Board::ResetGame()
{
	m1990 = 0;
	m8e7 = true;
	m1aac = 0;
	m8eb = false;
	m8ec = false;
	m8ed = false;
	mAnim5e0.Reset();
	mAnim5e0.SetNumFrames(0x3f);
	mAnim5e0.SetDelay(1);
	mAnim5e0.mPingPong = true;
	mAnim5e0.mMaxLoops = 6;
	mAnim5e0.mLoopCount = 0;
	mAnim5e0.SetStopped(true);
	m1960 = 0;
	m1980 = 0;
	m196c = 0;
	m8de = false;
	m921 = false;
	m91f = true;
	m1b44 = 0;
	m964 = 200;
	m91e = false;
	m1b3c = 0;
	m911 = false;
	m91b = false;
	m91c = false;
	gFlag5A5100 = false;
	m1b24 = 0;
	m917 = false;
	m1984 = 0;
	m1b18 = 0;
	m1b10 = 0;
	m1aa8 = 0;
	m1aa4 = 0;
	m1978 = mScore;
	m907 = true;
	m1934 = 0;
	m1938 = 0;
	m8ea = false;
	m1944 = 0;
	mFlag8E9 = false;
	mFlag8E8 = false;
	m924 = true;
	m1b54 = 0;
	m1950 = 0;
	m1b68 = 0;
	m1b64 = 0;
	m1b60 = 0;
	m194c = 2;
	m1994 = 0;
	m1a3c = 0;
	m8e3 = true;
	m1b5c = 0;
	m8e4 = false;
	m1988 = 0;
	m90e = false;
	m1964 = 0;
	m90d = false;
	m8e5 = false;
	m8e6 = false;
	m3c0 = Color(100, 100, 100);
	m1a98 = -500000000;
	m1a9c = -500000000;
	m1bf8 = -1.0f;
	m1bfc = 1.0f;
	m1980 = 0;
	m1934 = 0;
	m1ab4 = 0;
	m1a2c = 0;
	m91a = false;
	m8f8 = false;
	m198c = 0xa6;
	m8ef = false;
	m8fb = false;
	m8f0 = false;
	m909 = false;
	m97c = 0;
	m8f4 = false;
	m8fa = false;
	m1a48 = -65536;
	m6ec = -1;
	m6e8 = -1;
	m1a68 = 0;
	m1a50 = 0;
	m8fe = false;
	m905 = false;
	m1a4c = -1;
	m8fd = false;
	m8fc = false;
	m2f8 = NULL;
	m19fc = 0;
	m1a44 = -1;
	m1a10 = 0;
	m1a40 = -1;
	m8f6 = false;
	m8f5 = false;
	m1a14 = 0;
	m8f9 = false;
	m7c8 = 0;
	m7c4 = 0;
	m7b8 = 0;
	m7bc = 0;
	m7c0 = 0;
	m8f3 = false;
	m8f2 = false;
	m8f1 = false;
	m200.clear();
	m19d0 = 0;
	m19dc = 0;
	if (m1bbc < mField193C && 8 < m19d0)
		m19d4 = 8;
	m1a30 = 0;

	m1f0.clear();
	for (int i = 5; i != 0; i--)
	{
		int aCount = 2000;
		if (0x12 < mField193C || m1a60 == 2)
			aCount = 0;
		m1f0.push_back(aCount);
	}
	m1928 = 0;
	m192c = 0;
	m19b4 = 0;
	m19bc = 500;
	m930 = 0;
	m934 = 0;
	m8ee = false;
	m19d8 = -1;
	m1924 = 0;

	// the pictures of the backdrop that are in use are dropped (so that they are loaded again)
	for (unsigned int i = 0; i < m190.size(); i++)
	{
		int anId = m190[i].mImageId;
		if (anId != -1)
			mApp->mResourceManager->DeleteImage(GetStringIdById(anId));
	}

	// three different pictures out of seven (the first draw is thrown away)
	std::vector<Image*> aPictures;
	AstroRand();
	int aFirst = ((int) AstroRand() % 500000000) % 7 + 0x18e;
	int aSecond;
	do
	{
		aSecond = ((int) AstroRand() % 500000000) % 7 + 0x18e;
	}
	while (aFirst == aSecond);
	int aThird = aSecond;
	while (aThird == aSecond || aThird == aFirst)
		aThird = ((int) AstroRand() % 500000000) % 7 + 0x18e;
	aPictures.push_back(LoadImageById(mApp->mResourceManager, aSecond));
	aPictures.push_back(LoadImageById(mApp->mResourceManager, aFirst));
	aPictures.push_back(LoadImageById(mApp->mResourceManager, aThird));

	for (unsigned int i = 0; i < m190.size(); i++)
	{
		StarRec* aStar = &m190[i];
		aStar->mImage = aPictures[i];
		if (i == 0)
			aStar->mImageId = aSecond;
		else if (i == 1)
			aStar->mImageId = aFirst;
		else
			aStar->mImageId = aThird;
		aStar->mX = (float) (((int) AstroRand() % 500000000) % (0x1b8 - aStar->mImage->mWidth) + 0xa6);
		aStar->mY = (float) ((int) i * -900 + 100);
		aStar->m60 = 0;
		aStar->mSpeed = 0.1f;
	}
	m1a80 = 0;
}
