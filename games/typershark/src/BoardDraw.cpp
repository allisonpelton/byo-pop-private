// BoardDraw.cpp: Board::Draw (the whole game screen: the sea, the sky, the creatures, the pop-ups, the pause and the level
// screens) and the helpers that only it (and a few event loops) call (job B2): the bonus formulas, the typing speed, the next
// rank goal, the rank title, the wave effect of the water.
//
// Draw's order of calls is the original's: the smoothers, the sea, the sky strip, the lists of creatures in the order the
// original draws them, then the pop-ups, the status text, the level and treasure screens, the pause and the fades. The
// wavy water has three paths as in the original: the plain picture (a slow machine, or a black-ish fade), the picture drawn
// row by row (DrawWavyImage) and, when the screen is 3D accelerated, strips of triangles (BuildWaterImage).
//
// The original calls _ftol (0x509890) wherever a double becomes an int; it truncates, which is `(int)`. Only GetTypingSpeed,
// which can divide by a time of zero, needs the CRT's answer for a value that does not fit (see Ftol below).

#include "Board.h"
#include "Enemy.h"
#include "Clam.h"
#include "Bubble.h"
#include "BonusText.h"
#include "BackgroundFish.h"
#include "ProjectileBoss.h"
#include "HudBar.h"
#include "WSTimer.h"
#include "LessonManager.h"
#include "UserProfile.h"
#include "GameText.h"
#include "MTRand.h"
#include "Res.h"
#include "Image.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "TriVertex.h"
#include <math.h>
#include <string.h>

using namespace Sexy;

// port: the CRT's _ftol (0x509890) keeps the low 32 bits of the truncated 64-bit integer, and answers 0 for a NaN or for a
// value that does not fit in 64 bits. Plain `(int)` of such a value is undefined in C++.
static int Ftol(double theValue)
{
	if (!(theValue > -9223372036854775808.0 && theValue < 9223372036854775808.0))
		return 0;
	return (int)(long long)theValue;
}

// 0x41d350
void Sexy::DrawTwoColorString(Graphics* g, int theX, int theY, const std::string& theText1, const Color& theColor1, Font* theFont1,
	const std::string& theText2, const Color& theColor2, Font* theFont2, int theWidth, bool theCenter)
{
	Font* aFont = theFont1;
	if (aFont == NULL)
		aFont = theFont2;
	std::string aBoth = theText1 + theText2;
	int aX = 0;
	if (theCenter)
		aX = (theWidth >> 1) - (aFont->StringWidth(aBoth) >> 1);
	aX = aX + theX;

	g->SetFont(aFont);
	g->SetColor(theColor1);
	int anOffset = 0;
	for (unsigned int i = 0; i < theText1.length(); i++)
	{
		std::string aChar(1, theText1[i]);
		g->DrawString(aChar, anOffset + aX, theY);
		anOffset = anOffset + aFont->StringWidth(aChar);
	}

	g->SetFont(theFont2);
	g->SetColor(theColor2);
	for (unsigned int i = 0; i < theText2.length(); i++)
	{
		std::string aChar(1, theText2[i]);
		g->DrawString(aChar, anOffset + aX, theY);
		anOffset = anOffset + theFont2->StringWidth(aChar);
	}
}

// 0x43fca0: (difficulty + 1) times the diver's level factor (its +0x114 plus one, but 11 once that is past 11) times 1000,
// at most 60000
int Board::GetLevelBonus()
{
	int aFactor = 11;
	if (mPlayer->mField114 < 12)
		aFactor = mPlayer->mField114 + 1;
	int aBonus = (mApp->mDifficulty + 1) * aFactor * 1000;
	if (aBonus > 60000)
		aBonus = 60000;
	return aBonus;
}

// 0x43fce0
int Board::GetBossBonus()
{
	switch (mApp->mDifficulty)
	{
	case 0:
		return 10000;
	case 1:
		return 30000;
	case 2:
		return 50000;
	case 3:
		return 70000;
	}
	return 100000;
}

// 0x43ff90: the words per minute of the level, adjusted for the mistakes (mField0f68 is the count of the letters typed right and
// mField0f6c the count of the mistakes: the quotient with a fifth of a word per letter). The two branches differ only in
// how the first product is rounded; the original computes the inverse of the minutes once and keeps it.
int Board::GetTypingSpeed()
{
	double aMinutes = (double)mTypeTime * 1.6666666666666667e-05;
	double anInverse = 1.0 / aMinutes;
	double aWords;
	if (aMinutes < 1.0)
		aWords = ((double)mField0f68 * anInverse * 0.2) / (anInverse * aMinutes);
	else
		aWords = (double)mField0f68 * 0.2 * anInverse;
	int aWordCount = Ftol(aWords);
	return Ftol((double)aWordCount * 0.75 - (double)mField0f6c * 0.2 * anInverse);
}

// 0x440010: the next rank's score: the steps of 150000 up to 0x594390, then the five fixed ones; -5 once the top is reached
int Board::GetNextGoal()
{
	int aScore = mPlayer->mScore;
	if (aScore < 0x594390)
		return (aScore / 150000 + 1) * 150000;
	if (aScore < 0x657890)
		return 0x657890;
	if (aScore < 0x71ad90)
		return 0x71ad90;
	if (aScore < 0x870a50)
		return 0x870a50;
	if (aScore < 0xa58ed0)
		return 0xa58ed0;
	return -5;
}

// 0x440290: ((the diver's +0x114 plus one, at most 8) + 4 times the difficulty) times (1000 per word in a row, at most 6, plus
// 500) plus 3000, at most 100000
int Board::GetFinishBonus()
{
	int aWords = mField0e8c;
	if (aWords > 6)
		aWords = 6;
	int aLevelFactor = mPlayer->mField114;
	if (aLevelFactor > 8)
		aLevelFactor = 8;
	else
		aLevelFactor = aLevelFactor + 1;
	int aBonus = (aLevelFactor + mApp->mDifficulty * 4) * (aWords * 1000 + 500) + 3000;
	if (aBonus > 100000)
		aBonus = 100000;
	return aBonus;
}

// 0x4423f0: not a bonus for a difficulty past 4: the original then writes a trace (0x4ad5f0, the demo recording; not ported)
// and returns 0
int Board::GetStreakBonus()
{
	switch (mApp->mDifficulty)
	{
	case 0:
		return (mField0f30 + 1) * 50;
	case 1:
		return mField0f30 * 100 + 150;
	case 2:
		return mField0f30 * 250 + 450;
	case 3:
		return mField0f30 * 600 + 1000;
	case 4:
		return mField0f30 * 1050 + 3800;
	}
	return 0;
}

// 0x446950: the title of the diver's score. The first 39 come from a table of 150000-point steps (0x560078), the next five
// from 0x560114 to 0x560124.
std::string Board::GetRankTitle()
{
	static const unsigned int sRankTitles[39] =
	{
		0x0052f010, 0x0052f008, 0x0052effc, 0x0052eff4, 0x0052efe8, 0x0052efdc, 0x0052efd4, 0x0052efc8, 0x0052efbc,
		0x0052efb0, 0x0052efa4, 0x0052ef98, 0x0052ef8c, 0x0052ef80, 0x0052ef74, 0x0052ef64, 0x0052ef54, 0x0052ef44,
		0x0052ef38, 0x0052ef2c, 0x0052ef20, 0x0052ef14, 0x0052ef0c, 0x0052ef00, 0x0052eef0, 0x0052eee0, 0x0052eed0,
		0x0052eec8, 0x0052eebc, 0x0052eeac, 0x0052ee9c, 0x0052ee8c, 0x0052ee78, 0x0052ee6c, 0x0052ee58, 0x0052ee48,
		0x0052ee38, 0x0052ee28, 0x0052ee1c
	};

	int aScore = mPlayer->mScore;
	if (aScore < 0)
		return GT(0x0052f928);
	if (aScore < 0x594390)
		return GT(sRankTitles[aScore / 150000]);
	if (aScore < 0x5dd770)
		return GT(0x0052ee10);
	if (aScore < 0x657890)
		return GT(0x0052ee04);
	if (aScore < 0x71ad90)
		return GT(0x0052edf8);
	if (aScore < 0x870a50)
		return GT(0x0052ede4);
	if (aScore < 0xa58ed0)
		return GT(0x0052edd4);
	if (aScore <= 0x1c9c380)
		return GT(0x0052f910);
	return GT(0x0052f904);
}

// 0x440330: the sea drawn as strips of two triangles on a 3D accelerated screen. Two pictures of the water (the second one
// 480 pixels further down than the first, both moving with mUpdateCnt) are cut into strips of 16 rows; the left and right
// edges of a strip are shifted along a sine of the row, and the picture is stretched over the strip through its u
// coordinates. The vertices are floats; a value that the original keeps in an x87 register stays a double here.
// The original's Graphics argument is not used.
void Board::BuildWaterImage(Graphics* g)
{
	const float aWaveStep = 0.026179917f;
	double aDoubleAmp = mWaveAmp + mWaveAmp;
	Image* aWater = mApp->mImages[IMAGE_WATER1];
	int aHeight = aWater->mHeight;
	float anInvWidth = (float)(1.0 / (double)aWater->GetWidth());
	float anInvHeight = (float)(1.0 / (double)aWater->GetHeight());

	for (int i = 0; i < 2; i++)
	{
		int aQuarter = mUpdateCnt / 4;
		int aBase = 480 - aQuarter % 480;
		if (i == 0)
			aBase = aBase - 480;

		if (mApp->Is3DAccelerated() && aHeight > 0)
		{
			int aStripEnd = 16;
			int aPrevEnd;
			do
			{
				aQuarter = mUpdateCnt / 4;
				int aTop = aStripEnd - 16;
				double aTopSine = sin((double)(aTop + aQuarter) * aWaveStep);
				float aTopLeft = (float)((sin((double)(aTop + mUpdateCnt) * aWaveStep) + aTopSine) * mWaveAmp + aDoubleAmp);
				double aBottomSine = sin((double)(aStripEnd + aQuarter) * aWaveStep);
				double aBottomLeft = (sin((double)(aStripEnd + mUpdateCnt) * aWaveStep) + aBottomSine) * mWaveAmp + aDoubleAmp;

				float aTopY = (float)(aBase - 16 + aStripEnd);
				float aBottomY = (float)(aStripEnd + aBase);
				float aTopV = (float)((double)aTop * anInvHeight);
				float aBottomV = (float)((double)aStripEnd * anInvHeight);
				float aTopU1 = (float)((double)aTopLeft * anInvWidth);
				float aTopU2 = (float)(((double)aTopLeft + 640.0) * anInvWidth);
				float aBottomU2 = (float)((aBottomLeft + 640.0) * anInvWidth);
				float aBottomU1 = (float)(aBottomLeft * anInvWidth);

				TriVertex aTriangles[2][3];
				aTriangles[0][0] = TriVertex(0.0f, aTopY, aTopU1, aTopV);
				aTriangles[0][1] = TriVertex(640.0f, aTopY, aTopU2, aTopV);
				aTriangles[0][2] = TriVertex(640.0f, aBottomY, aBottomU2, aBottomV);
				aTriangles[1][0] = TriVertex(0.0f, aTopY, aTopU1, aTopV);
				aTriangles[1][1] = TriVertex(0.0f, aBottomY, aBottomU1, aBottomV);
				aTriangles[1][2] = TriVertex(640.0f, aBottomY, aBottomU2, aBottomV);
				g->DrawTrianglesTex(aWater, aTriangles, 2);

				aPrevEnd = aStripEnd;
				aStripEnd = aStripEnd + 16;
			} while (aPrevEnd < aHeight);
		}
	}
}

// 0x441ac0: draws theImage one row at a time; row r is cut from the picture starting at a column that follows the sum of two
// entries of mSinTable (one stepped by mWavePhase1 / 8, the other by mWavePhase2 / 4), so the picture seems to ripple.
// A picture that starts above the screen skips its first rows; one that ends below it is not cut to the screen the way
// one would think: it keeps (height - theY) rows (kept from the original).
void Board::DrawWavyImage(Graphics* g, Image* theImage, int theX, int theY)
{
	int aRows = theImage->mHeight;
	double aDoubleAmp = mWaveAmp + mWaveAmp;
	int aSkip = 0;
	if (theY < 0)
	{
		aSkip = -theY;
		aRows = aRows + theY;
		theY = 0;
	}
	else if (aRows + theY > mHeight)
		aRows = aRows - theY;
	if (aRows <= 0)
		return;

	int aWidth = (int)(aDoubleAmp + 640.0);
	int aBaseY = theY - aSkip;
	do
	{
		int anIndex1 = ((mWavePhase1 + aSkip) >> 3) % 360;
		int anIndex2 = ((mWavePhase2 + aSkip) >> 2) % 360;
		int aShift = (int)((mSinTable[anIndex1] + mSinTable[anIndex2]) * mWaveAmp + aDoubleAmp);
		Rect aSrc(aShift, aSkip, aWidth, 1);
		g->DrawImage(theImage, theX, aBaseY + aSkip, aSrc);
		aSkip++;
		aRows--;
	} while (aRows != 0);
}

// 0x44d200 slot 16
void Board::Draw(Graphics* g)
{
	Widget::Draw(g);		// 0x47bf40
	mDrawCount++;

	// the rating popup's y is eased over the last six changes (mFrameSmoother); when the average would overshoot, it snaps
	double aDelta = mRating->mY - mFrameSmoother.mPrev;
	mFrameSmoother.mPrev = mRating->mY;
	mFrameSmoother.mSamples[mFrameSmoother.mIndex] = aDelta;
	mFrameSmoother.mIndex = (mFrameSmoother.mIndex + 1) % 6;
	double aSum = 0.0;
	for (int i = 0; i < 6; i++)
		aSum = aSum + mFrameSmoother.mSamples[i];
	double anAverage = aSum * 0.16666666666666666;
	double aNext = anAverage + mRating->mSmoothY;
	double anError = fabs((anAverage + aNext) - mRating->mY);
	if (fabs(anAverage) <= anError)
		aNext = mRating->mY;
	mRating->mSmoothY = aNext;

	// the same for each of the two strips of sea
	for (int i = 0; i < 2; i++)
	{
		Smoother* aSmoother = &mSmoothers[i];
		aDelta = mBgLayers[i].mY - aSmoother->mPrev;
		aSmoother->mPrev = mBgLayers[i].mY;
		aSmoother->mSamples[aSmoother->mIndex] = aDelta;
		aSmoother->mIndex = (aSmoother->mIndex + 1) % 6;
		aSum = 0.0;
		for (int j = 0; j < 6; j++)
			aSum = aSum + aSmoother->mSamples[j];
		anAverage = aSum * 0.16666666666666666;
		aNext = anAverage + aSmoother->mSmoothed;
		anError = fabs((anAverage + aNext) - mBgLayers[i].mY);
		if (fabs(anAverage) <= anError)
			aNext = mBgLayers[i].mY;
		aSmoother->mSmoothed = aNext;
	}

	// the sea
	if (mField0d20 > -70.0 || mLessonManager->IsBonusLevel())
	{
		if (mApp->Is3DAccelerated() && mFastCPU && mFadeAlpha <= 0xaa)
			BuildWaterImage(g);
		else
		{
			for (int i = 0; i < 2; i++)
			{
				if (mFastCPU && mFadeAlpha <= 0xaa)
					DrawWavyImage(g, mApp->mImages[IMAGE_WATER1], (int)mBgLayers[i].mX, (int)mSmoothers[i].mSmoothed);
				else
					g->DrawImage(mApp->mImages[IMAGE_WATER1], (int)mBgLayers[i].mX, (int)mSmoothers[i].mSmoothed);
			}
		}
	}

	// the three mountain ranges and the floor, or in a bonus level the sand and the coral shelf
	if ((double)mHeight > mField0e10 && mField0e10 != -1.0 && !mLessonManager->IsBonusLevel())
	{
		g->DrawImage(mApp->mImages[IMAGE_MOUNTAIN3], 0, (int)mField0d20);
		g->DrawImage(mApp->mImages[IMAGE_MOUNTAIN2], 0, (int)mField0d18);
		g->DrawImage(mApp->mImages[IMAGE_MOUNTAIN1], 0, (int)mField0d10);
		g->DrawImage(mApp->mImages[IMAGE_OCEANFLOOR], 0, (int)mField0e10);
	}
	else if (mLessonManager->IsBonusLevel())
	{
		g->DrawImage(mApp->mImages[IMAGE_SAND], 0, (int)mField0e10);
		g->DrawImage(mApp->mImages[IMAGE_CORALSHELF], 0x7e, (int)mField0e10);
	}

	for (unsigned int i = 0; i < mBackgroundFish.size(); i++)
		mBackgroundFish[i]->Draw(g);

	// the labels of the depth ruler, down to the first one below the screen
	g->SetFont(mApp->mFontMyriadCondensed12Bold);
	g->SetColor(Color(255, 255, 255));
	for (unsigned int i = 0; i < mDepthMarkers.size(); i++)
	{
		DepthMarker* aMarker = mDepthMarkers[i];
		if (!(aMarker->mX < 0.0))
		{
			g->DrawString(aMarker->mText, (int)aMarker->mX, (int)aMarker->mY);
			if ((double)mHeight < aMarker->mY)
				break;
		}
	}

	// the hint of the first game: while the player has not seen the tutorial, the diver is in state 0
	if (!mApp->mUser->mFlag1dc && mPlayer->mState == 0)
	{
		g->SetFont(mApp->mFontRockwell13);
		g->SetColor(Color(255, 255, 255, mField0ea4));
		WriteCenteredLine(g, 0x91, GT(0x005306b4));
		WriteCenteredLine(g, (int)((double)g->GetFont()->GetHeight() + 145.0), GT(0x00530680));
	}

	for (unsigned int i = 0; i < mBonusTexts.size(); i++)
		mBonusTexts[i]->Draw(g);
	for (unsigned int i = 0; i < mPiranhas.size(); i++)
		mPiranhas[i]->Draw(g);
	for (unsigned int i = 0; i < mSharks.size(); i++)
		mSharks[i]->Draw(g);
	for (unsigned int i = 0; i < mBonusCreatures.size(); i++)
		mBonusCreatures[i]->Draw(g);
	mPlayer->Draw(g);
	if (mBoss != NULL)
		mBoss->Draw(g);
	for (unsigned int i = 0; i < mBubbles.size(); i++)
		mBubbles[i]->Draw(g);

	// the sky above the water, while the diver is still near the surface: the picture follows the level
	if (!((double)mSurfaceHeight < mDivePos))
	{
		Image* aSky;
		if (mLessonManager->mAbyssMode)
			aSky = mApp->mImages[IMAGE_TOPBACKGROUND];
		else
		{
			switch (mLessonManager->mLevel)
			{
			case 0:
			case 1:
			case 2:
			case 3:
			case 6:
				aSky = mApp->mImages[IMAGE_TOPBACKGROUND];
				break;
			case 4:
			case 5:
			case 0xe:
			case 0x11:
			case 0x12:
			case 0x13:
			case 0x19:
				aSky = mApp->mImages[IMAGE_TOPBACKGROUND_VOLCANO];
				break;
			case 7:
			case 8:
			case 0xc:
			case 0xd:
			case 0x10:
			case 0x18:
			case 0x1c:
				aSky = mApp->mImages[IMAGE_TOPBACKGROUND_SHIPWRECK];
				break;
			case 9:
			case 0xa:
			case 0xb:
			case 0xf:
			case 0x14:
			case 0x16:
			case 0x17:
			case 0x1f:
				aSky = mApp->mImages[IMAGE_TOPBACKGROUND_LIGHTHOUSE];
				break;
			case 0x15:
			case 0x1a:
			case 0x1b:
			case 0x20:
				aSky = mApp->mImages[IMAGE_TOPBACKGROUND_ICEBERG];
				break;
			default:
				aSky = mApp->mImages[IMAGE_TOPBACKGROUND_STORM];
			}
		}
		g->DrawImage(aSky, 0, (int)-mDivePos);
	}

	// the rating popup: the medal and the accuracy of the last group of words, or (no medal) just the accuracy text
	if (mRating->mActive)
	{
		if (mRating->mImage != NULL)
		{
			std::string aText;
			if (mRating->mPercent == 0)
				aText = "";
			else
				aText = StrFormat(GTC(0x00530668), mRating->mPercent);		// "Accuracy: %d%%"
			g->SetFont(mApp->mFontRockwell13);
			int aTextX = (mApp->mWidth >> 1) - (mApp->mFontRockwell13->StringWidth(aText) >> 1);
			g->SetColor(Color(255, 255, 255, mRating->mAlpha));
			g->DrawString(aText, aTextX, 150);
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, mRating->mAlpha));
			Image* aMedal = mRating->mImage;
			g->DrawImage(aMedal, (mApp->mFontRockwell13->StringWidth(aText) >> 1) - (aMedal->mWidth >> 1) + aTextX, 150);
			g->SetColorizeImages(false);
			if (mRating->mBonus > 0)
				aText = StrFormat(GTC(0x00530650), mRating->mBonus);		// "Accuracy Bonus: %dpts"
			else
				aText = GT(0x00530638);										// "Less than 95%, no bonus"
			aTextX = (mApp->mWidth >> 1) - (mApp->mFontRockwell13->StringWidth(aText) >> 1);
			int aTextY = mRating->mImage->mHeight + 160;
			g->SetColor(Color(255, 255, 255, mRating->mAlpha));
			g->DrawString(aText, aTextX, aTextY);
		}
		else
		{
			std::string aText = StrFormat(GTC(0x00530668), mRating->mPercent);
			g->SetFont(mApp->mFontRockwell13);
			g->SetColor(Color(255, 255, 255, mRating->mAlpha));
			g->DrawString(aText, (int)mRating->mX, (int)mRating->mY);
		}
	}

	int aState = mPlayer->mState;
	if (aState == 4 && mFlag0f80 && mRating->mAlpha == 0)
	{
		// state 4, the bonus level of the air tank (or of the clams): the instructions, then the tank that fills up
		g->SetFont(mApp->mFontRockwell13);
		int aTankY;
		if (!mLessonManager->IsBonusLevel())
		{
			Image* aBanner = mApp->mImages[IMAGE_SHIPWRECKBONUS];
			g->DrawImage(aBanner, (mApp->mWidth >> 1) - (aBanner->mWidth >> 1), 0x32);
			std::string aText = GT(0x00530600);		// "Type as many words as you can before your air runs out!"
			g->SetColor(Color(255, 255, 255));
			WriteCenteredLine(g, g->GetFont()->GetHeight() + mSurfaceHeight, aText);
			aTankY = mSurfaceHeight + g->GetFont()->GetHeight() * 2;
		}
		else
		{
			g->SetFont(mApp->mFontSwissCheesed24);
			g->SetColor(Color(0xfd, 0xa0, 0x3d));
			WriteCenteredLine(g, 0x3c, GT(0x005305d4));		// "SECRET BONUS LEVEL!"
			g->SetFont(mApp->mFontRockwell13);
			std::string aText = GT(0x00530598);			// "Type the letter on a clam to open it before time runs out."
			g->SetColor(Color(255, 255, 255));
			WriteCenteredLine(g, 0x55, aText);
			aTankY = g->GetFont()->GetAscent() * 2 + 0x55;
			for (unsigned int i = 0; i < mClams.size(); i++)
				mClams[i]->Draw(g);
		}
		Image* aGauge = mApp->mImages[IMAGE_AIRGAUGE];
		int aGaugeX = mWidth / 2 - aGauge->GetWidth() / 2;
		g->DrawImage(aGauge, aGaugeX, aTankY);
		g->SetColor(Color(0, 255, 0, mField0f44));
		g->FillRect(aGaugeX + 0x13, aTankY + 7, (int)mField0f38, 0xf);
		g->SetFont(mApp->mFontRockwell13);
		g->SetColor(Color(255, 255, 255));
		std::string aTankText = GT(0x005305e8);		// "FILLING UP AIR TANK!"
		g->DrawString(aTankText, ((aGauge->mWidth >> 1) - (mApp->mFontRockwell13->StringWidth(aTankText) >> 1)) + aGaugeX, aTankY + 0x2d);
	}
	else if (aState == 5)
	{
		// state 5, the bonus level of the air tank: the same banner, the streak of the word typed, the tank
		g->SetFont(mApp->mFontRockwell13);
		g->SetColor(Color(0, 255, 0));
		int aTankY = mSurfaceHeight + g->GetFont()->GetHeight() * 2;
		if (!mLessonManager->IsBonusLevel())
		{
			Image* aBanner = mApp->mImages[IMAGE_SHIPWRECKBONUS];
			g->DrawImage(aBanner, (mApp->mWidth >> 1) - (aBanner->mWidth >> 1), 0x32);
			std::string aText = GT(0x00530600);		// "Type as many words as you can before your air runs out!"
			g->SetColor(Color(255, 255, 255));
			WriteCenteredLine(g, g->GetFont()->GetHeight() + mSurfaceHeight, aText);
			if (mString0f10 != "" && !mPaused)
			{
				// the value of the word under the diver, then the word with the part typed in another colour
				g->SetFont(mApp->mFontRockwell13);
				int aFontHeight = g->GetFont()->GetHeight();
				std::string aValue = StrFormat(GTC(0x0052b19c), GetStreakBonus());
				DrawTwoColorString(g, 0, mFloorY - aFontHeight * 4, GT(0x00530580), Color(0, 255, 0), mApp->mFontRockwell13,		// "Word value: "
					aValue + GT(0x00530590), Color(255, 194, 0), mApp->mFontRockwell13, mWidth, true);								// " pts"
				aFontHeight = mApp->mFontMyriadCondensed28BoldGL->GetHeight();
				std::string aRest = mString0f10.substr(mField0f2c, mString0f10.length());
				std::string aDone = mString0f10.substr(0, mField0f2c);
				DrawTwoColorString(g, 0, mFloorY - aFontHeight * 2, aDone, Color(0, 230, 0), mApp->mFontMyriadCondensed28BoldGL,
					aRest, Color(255, 255, 255), mApp->mFontMyriadCondensed28Bold, mWidth, true);
			}
		}
		else
		{
			g->SetFont(mApp->mFontSwissCheesed24);
			g->SetColor(Color(0xfd, 0xa0, 0x3d));
			WriteCenteredLine(g, 0x3c, GT(0x005305d4));		// "SECRET BONUS LEVEL!"
			g->SetFont(mApp->mFontRockwell13);
			g->SetColor(Color(255, 255, 255));
			WriteCenteredLine(g, 0x55, GT(0x00530598));		// "Type the letter on a clam to open it before time runs out."
			WriteCenteredLine(g, g->GetFont()->GetAscent() + 0x55, GT(0x00530518));		// "Match pairs of same colored pearls to win!"
			aTankY = g->GetFont()->GetAscent() * 2 + 0x55;
			for (unsigned int i = 0; i < mClams.size(); i++)
				mClams[i]->Draw(g);
		}
		Image* aGauge = mApp->mImages[IMAGE_AIRGAUGE];
		int aGaugeX = mWidth / 2 - aGauge->GetWidth() / 2;
		g->DrawImage(aGauge, aGaugeX, aTankY);
		g->SetColor(Color(0, 255, 0, mField0f44));
		g->FillRect(aGaugeX + 0x13, aTankY + 7, (int)mField0f38, 0xf);
	}
	else if (aState == 6 || aState == 0xc)
	{
		// states 6 and 0xc, the treasure (or the secret item of a bonus level) is shown
		Font* aFont = mApp->mFontMyriadCondensed14Bold;
		if (!mLessonManager->IsBonusLevel())
		{
			g->SetFont(aFont);
			g->SetColor(Color(255, 255, 255, mField0f04));
			WriteCenteredLine(g, 0x41, GT(0x00530438));		// "You found"
			int anAscent = g->GetFont()->GetAscent();
			if (mFlag0f08)
			{
				g->SetColorizeImages(true);
				g->SetColor(Color(255, 255, 255, mField0f04));
			}
			int aTreasureIndex = mApp->GetTreasureIndex();
			Image* aTreasure = mApp->mImages[aTreasureIndex];
			int aTreasureX = (mWidth >> 1) - (aTreasure->mWidth >> 1);
			int aTreasureY = (int)((double)anAscent + 65.0);
			g->DrawImage(mApp->mImages[mApp->GetTreasureIndex()], aTreasureX, aTreasureY);
			if (mFlag0f08)
				g->SetColorizeImages(false);

			// three sparkles (the position and the frame of each are mField0ec0, in groups of three)
			g->SetDrawMode(1);
			g->SetColor(Color(255, 255, 255, mField0f04));
			g->SetColorizeImages(true);
			Image* aSparkle = mApp->mImages[IMAGE_SPARKLE_LARGE];
			Rect aSrc1(mField0ec0[2] * 40, 0, 0x28, aSparkle->mHeight);
			g->DrawImage(mApp->mImages[IMAGE_SPARKLE_LARGE], mField0ec0[0] + aTreasureX, mField0ec0[1] + aTreasureY, aSrc1);
			Rect aSrc2(mField0ec0[5] * 40, 0, 0x28, mApp->mImages[IMAGE_SPARKLE_LARGE]->mHeight);
			g->DrawImage(mApp->mImages[IMAGE_SPARKLE_LARGE], mField0ec0[3] + aTreasureX, mField0ec0[4] + aTreasureY, aSrc2);
			Rect aSrc3(mField0ec0[8] * 40, 0, 0x28, mApp->mImages[IMAGE_SPARKLE_LARGE]->mHeight);
			g->DrawImage(mApp->mImages[IMAGE_SPARKLE_LARGE], mField0ec0[6] + aTreasureX, mField0ec0[7] + aTreasureY, aSrc3);
			g->SetColorizeImages(false);
			g->SetDrawMode(0);

			// "the <gem> <treasure>!" (0x530430, 0x52b294 and 0x53042c are "the ", " " and "!")
			std::string aTreasureName = mApp->GetTreasureName();
			std::string aGemName = mApp->GetGemName();
			std::string aText = GT(0x00530430) + aGemName + " " + aTreasureName + "!";
			g->SetColor(Color(255, 255, 255, mField0f04));
			WriteCenteredLine(g, (int)((double)mApp->mImages[mApp->GetTreasureIndex()]->mHeight + (double)anAscent + 120.0), aText);

			aText = StrFormat(GTC(0x00530424), GetLevelBonus());		// "+%d pts"
			g->SetColor(Color(0, 255, 0, mField0f04));
			WriteCenteredLine(g, (int)((double)mApp->mImages[mApp->GetTreasureIndex()]->mHeight + (double)(anAscent * 2) + 120.0), aText);

			g->SetColor(Color(255, 255, 255, mField0f04));
			if (mPlayer->mState == 0xc)
				aText = StrFormat(GTC(0x00530410), GetBossBonus());		// "Bonus points: %d"
			else
				aText = StrFormat(GTC(0x00530400), mField0f30);			// "Bonus words: %d"
			WriteCenteredLine(g, (int)((double)mApp->mImages[mApp->GetTreasureIndex()]->mHeight + (double)(anAscent * 3) + 120.0), aText);
			if (mPlayer->mState != 0xc)
			{
				aText = StrFormat(GTC(0x00530410), mField0f0c);
				WriteCenteredLine(g, (int)((double)mApp->mImages[mApp->GetTreasureIndex()]->mHeight + (double)(anAscent * 4) + 120.0), aText);
			}
		}
		else
		{
			g->SetFont(aFont);
			g->SetColor(Color(255, 255, 255));
			if (mPairsFound > 0)
			{
				WriteCenteredLine(g, 0x3c, GT(0x005303e4));		// "You found a secret item!"
				g->DrawImage(mTreasureImage, (mWidth >> 1) - (mTreasureImage->mWidth >> 1), 0x4b);
			}
			else
				WriteCenteredLine(g, 0x3c, GT(0x005303ac));		// "Time's up! Try again next time to uncover secret items."
			for (unsigned int i = 0; i < mClams.size(); i++)
				mClams[i]->Draw(g);
		}
	}
	else if (aState == 7)
	{
		// state 7, the level is complete: the sonar screen with the typing speed, the accuracy, the bonus and the rank
		int aAccuracy = Ftol((double)mField0f68 / (double)(mField0f6c + mField0f68) * 100.0);
		g->SetColor(Color(0, 0, 0, 0x80));
		g->FillRect(0, 0x19, mWidth, (int)((double)mHeight - 25.0));
		Image* aSonar = mApp->mImages[IMAGE_SONAR_BG];
		if (mField0ef8 == 0xff)
		{
			g->DrawImage(aSonar, 0x140 - (aSonar->mWidth >> 1), 0x17);
			g->SetFont(mApp->mFontMyriadCondensed14Bold);
			g->SetColor(Color(0xff, 0xa3, 0x24));
			g->DrawString(GT(0x00530508), 0xf5, 0x7b);		// "Level Complete!"
			g->SetColor(Color(255, 255, 255));
			int aLineY = g->GetFont()->GetAscent() + 0x7b;
			g->DrawString(GT(0x005304f8), 0xbe, aLineY);	// "Adjusted WPM:"
			g->SetColor(Color(0xff, 0xa3, 0x24));
			int aTypingSpeed = GetTypingSpeed();
			std::string aValue;
			if (aTypingSpeed < 0)
				aValue = GT(0x0052e7d0);					// "N/A"
			else
				aValue = StrFormat(GTC(0x0052b19c), aTypingSpeed);
			g->DrawString(aValue, g->GetFont()->StringWidth(GT(0x005304e8)) + 0xbe, aLineY);		// the width of "Adjusted WPM: "
			aLineY = aLineY + g->GetFont()->GetAscent();
			g->SetColor(Color(255, 255, 255));
			g->DrawString(GT(0x005304dc), 0xbe, aLineY);	// "Accuracy:"
			aValue = StrFormat(GTC(0x0052fee8), aAccuracy);		// "%d%%"
			g->SetColor(Color(0xff, 0xa3, 0x24));
			g->DrawString(aValue, g->GetFont()->StringWidth(GT(0x005304e8)) + 0xbe, aLineY);
			aLineY = aLineY + g->GetFont()->GetAscent();
			if (aAccuracy >= 0x5f)
			{
				if (mOceanDepth != -1)
				{
					g->SetFont(mApp->mFont12pt);
					g->SetColor(Color(0x3c, 0xa0, 0x78));
					g->DrawString(StrFormat(GTC(0x005304c0), GetFinishBonus()), 0xbe, aLineY);		// "Accuracy Bonus +%d points "
					if (mField0e8c > 1)
					{
						aLineY = aLineY + mApp->mFontMyriadCondensed14Bold->GetAscent();
						g->DrawString(StrFormat(GTC(0x0053049c), mField0e8c), 0xbe, aLineY);			// "95%% or higher accuracy %d times"
					}
				}
			}
			else if (mOceanDepth != -1)
			{
				g->SetFont(mApp->mFont12pt);
				g->SetColor(Color(0x3c, 0xa0, 0x78));
				g->DrawString(GT(0x0053047c), 0xbe, aLineY);		// "Get over 94% accuracy for bonus"
			}
			aLineY = aLineY + mApp->mFontMyriadCondensed14Bold->GetAscent();
			g->SetFont(mApp->mFontMyriadCondensed14Bold);
			g->SetColor(Color(255, 255, 255));
			g->DrawString(GT(0x00530474), 0xbe, aLineY);			// "RANK: "
			g->SetColor(Color(0xff, 0xa3, 0x24));
			std::string aTitle = GetRankTitle();
			g->DrawString(aTitle, g->GetFont()->StringWidth(GT(0x00530474)) + 0xbe, aLineY);
			aLineY = aLineY + g->GetFont()->GetAscent();
			g->SetFont(mApp->mFontRockwell13);
			g->SetColor(Color(255, 255, 255));
			std::string aNext;
			if (GetNextGoal() < 0)
				aNext = GT(0x0053045c);								// "Top rank achieved!!!"
			else
				aNext = StrFormat(GTC(0x00530444), GetNextGoal());	// "Next rank at %d points"
			g->DrawString(aNext, 0xbe, aLineY);
			int aSonarX = 0x140 - (aSonar->mWidth >> 1);
			if (mImage0ee8 != NULL)
				g->DrawImage(mImage0ee8, aSonarX + 0xb6, 0x32);
			if (mImage0eec != NULL)
				g->DrawImage(mImage0eec, aSonarX + 0xb6, 0xcd);
			if (mImage0ef0 != NULL)
				g->DrawImage(mImage0ef0, aSonarX + 0x1b, 0xcd);
			if (mImage0ef4 != NULL)
				g->DrawImage(mImage0ef4, aSonarX + 0x1b, 0x32);
		}
		else
		{
			// the sonar pictures fade in before the text appears
			int aSonarX = 0x140 - (aSonar->mWidth >> 1);
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, mField0ef8));
			g->DrawImage(mImage0ee8, aSonarX + 0xb6, 0x32);
			g->DrawImage(mImage0eec, aSonarX + 0xb6, 0xcd);
			aSonarX = aSonarX + 0x1b;
			g->DrawImage(mImage0ef0, aSonarX, 0xcd);
			g->DrawImage(mImage0ef4, aSonarX, 0x32);
			g->SetColorizeImages(false);
		}
	}

	// the meter of the boss's hit points
	ProjectileBoss* aBoss = dynamic_cast<ProjectileBoss*>(mBoss);
	if (mBoss != NULL && aBoss->mHitPoints > 0)
	{
		double aRatio = (double)aBoss->mHitPoints / (double)aBoss->mField10c;
		g->DrawImage(mApp->mImages[IMAGE_BOSS_METER], 0x7c, 0x26);
		Rect aSrc(0, 0, (int)(aRatio * 294.0), 0x14);
		g->DrawImage(mApp->mImages[IMAGE_BOSS_METER_FILL], 0xa0, 0x30, aSrc);
	}

	// the banner text, fading
	if (!mBannerText.empty() && mBannerLife > 0)
	{
		g->SetFont(mApp->mFontRockwell13);
		g->SetColor(Color(255, 255, 255, mBannerLife));
		WriteCenteredLine(g, mFloorY - g->GetFont()->GetHeight(), mBannerText);
	}

	// the pause: the screen darkens, and unless a dialog is open the words PAUSED and how to go on
	if (mPaused)
	{
		g->SetColor(Color(0, 0, 0, 0x80));
		g->FillRect(0, 0x19, mWidth, (int)((double)mHeight - 25.0));
		g->SetColor(Color(255, 255, 255, 255));
		if (mApp->mOpenDialogCount <= 0 && mPlayer->mState != 8)
		{
			g->SetFont(mApp->mFontSwissCheesed24);
			WriteCenteredLine(g, mHeight / 2, GT(0x00530228));		// "PAUSED"
			g->SetFont(mApp->mFontRockwell13);
			g->SetColor(Color(200, 200, 200));
			std::string aHelp;
			if (mApp->mUser->mSpacePauses)
				aHelp = GT(0x00530544);		// "Press space or click the pause button to continue"
			else
				aHelp = GT(0x00530388);		// "Click the pause button to continue"
			WriteCenteredLine(g, mHeight / 2 + mApp->mFontMyriadCondensed14Bold->GetHeight(), aHelp);
		}
	}

	mHudBar->Draw(g);

	// the white flash of a key press, and the black fade-in of the level
	if (mFlashAlpha > 0)
	{
		g->SetColor(Color(255, 255, 255, mFlashAlpha));
		g->FillRect(0, 0, mWidth, mHeight);
	}
	if (mFadeAlpha > 0)
	{
		g->SetColor(Color(0, 0, 0, mFadeAlpha));
		g->FillRect(Rect(0, 0, mWidth, mHeight));
	}

	// the title that fades out after the level starts: "Level N" (a picture), or in a bonus level the secret level's name
	if (mField0e54 > 0 && !mPaused)
	{
		if (!mLessonManager->IsBonusLevel())
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, mField0e54));
			Rect aSrc(0, 0, 400, 0x32);
			g->DrawImage(mApp->mImages[IMAGE_TEXTOVERS], (mWidth >> 1) - 200, ((mHeight - 0x18) >> 1) - 0x19, aSrc);
			g->SetColorizeImages(false);
		}
		else
		{
			g->SetFont(mApp->mFontSwissCheesed24);
			g->SetColor(Color(0xfd, 0xa0, 0x3d, mField0e54));
			WriteCenteredLine(g, (mHeight >> 1) - (mApp->mFontSwissCheesed24->GetHeight() >> 1), GT(0x005305d4));		// "SECRET BONUS LEVEL!"
		}
	}
}
