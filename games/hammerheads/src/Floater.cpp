#include "Floater.h"

#include <climits>
#include <cmath>
#include <cstdio>

#include "Application.h"
#include "Color.h"
#include "DDImage.h"
#include "DrawUtil.h"
#include "Font.h"
#include "GameScreen.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "Popper.h"
#include "Rect.h"
#include "Res.h"
#include "Score.h"
#include "Stats.h"
#include "Creature.h"

using namespace Sexy;

static const double HALF_PI = 1.5707963267948966;
static const double HALF_PI_FLOAT = 1.5707963705062866;		// pi / 2 as a float, the exe's constant for most of the ease curves
static const double PI = 3.141592653589793;
static const double PI_FLOAT = 3.1415927410125732;
static const double TWO_PI_FLOAT = 6.2831854820251465;

// What the exe's _ftol gives: the value cut toward zero, or the "indefinite" integer when it does not fit
static std::string FormatInt(const char* theFormat, int theValue)
{
	char aBuffer[64];
	snprintf(aBuffer, sizeof(aBuffer), theFormat, theValue);
	return aBuffer;
}

// 0x4444b0
double Sexy::RampBetween(double theTime, double theHigh, double theLow)
{
	if (theHigh <= theTime)
		return 1.0;
	if (theTime <= theLow)
		return 0.0;
	return (theTime - theLow) / (theHigh - theLow);
}

// 0x468820: the vector turned by the angle
static void RotateVector(double* theVector, double theAngle)
{
	double aCos = cos(theAngle);
	double aSin = sin(theAngle);
	double aX = theVector[0];
	theVector[0] = aCos * theVector[0] - aSin * theVector[1];
	theVector[1] = aCos * theVector[1] + aSin * aX;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Floater

// 0x444db0
Floater::Floater(int theType, double theX, double theY, const char* theText, int theStyle) :
	Object(0x400),
	mType(theType),
	mX(theX),
	mY(theY),
	mScrollDivisor(2500.0),
	mImage(nullptr),
	mTextImage(nullptr),
	mTimeLeft(0.0),
	mVelocity(0.0),
	mStyle(theStyle),
	mScored(false),
	mSoundDone(false),
	mSpeed(1.0),
	mWaveMisses(0),
	mWaveSwings(0),
	mWaveIndex(0),
	mWavePercent(0),
	mWaveBonus(0),
	mLevelSmashed(0),
	mLevelChain(0),
	mLevelSpeed(0),
	mPendingPoints(0)
{
	if (theText != nullptr)
		mText = theText;

	switch (mType)
	{
	case FLOATER_RISING_TEXT:
		mTimeLeft = 2.3;
		break;

	case FLOATER_SCORE_FLY:
	case FLOATER_SCORE_POP:
	case FLOATER_SCORE_BIG:
	case FLOATER_GAME_OVER:
	{
		if (mType == FLOATER_SCORE_FLY || mType == FLOATER_SCORE_POP)
			mTimeLeft = 1.0;
		else if (mType == FLOATER_SCORE_BIG)
			mTimeLeft = 2.0;
		else
			mTimeLeft = 5.0;

		// the text drawn in its font into a picture of its own, in white
		Font* aFont = (mStyle == 1) ? FONT_PTBANANASPLIT27INGAMEUI : FONT_PTBANANASPLIT27;
		int aWidth = aFont->StringWidth(mText) + 4;
		DDImage* anImage = new DDImage(gApp->mDDInterface);
		anImage->Create(aWidth, aFont->mHeight + 4);
		mTextImage = anImage;

		Graphics aTextGraphics(anImage);
		aTextGraphics.SetFont(aFont);
		aTextGraphics.SetColorizeImages(false);
		aTextGraphics.SetColor(Color(255, 255, 255, 255));
		aTextGraphics.DrawString(mText, 2, aFont->GetHeight() - 2);
		mImage = mTextImage;

		if (mType == FLOATER_SCORE_BIG)
			mLayers = 0x800;
		if (mType == FLOATER_GAME_OVER)
			gApp->PlaySample(SOUND_STING_GAME_OVER);
		break;
	}

	case FLOATER_SHATTER:
	case FLOATER_SHATTER_SIDE:
		mTimeLeft = 4.0;
		break;

	case FLOATER_STRIKE:
	case FLOATER_STRIKE_BIG:
		mTimeLeft = 0.5;
		mLayers = 0x80;
		break;

	case FLOATER_STRIKE_WEAK:
		mTimeLeft = 1.0;
		mLayers = 0x80;
		break;

	case FLOATER_MISS:
		mTimeLeft = (mStyle != 1) ? 1.0 : 0.6;
		mLayers = 0x80;
		break;

	case FLOATER_HINT_CLICK:
		mLayers = 0x800;
		mTimeLeft = 3.0;
		gApp->PlaySample(SOUND_HINT_POPUP);
		break;

	case FLOATER_HINT_DONT_MISS:
		mLayers = 0x800;
		mTimeLeft = 6.0;
		gApp->PlaySample(SOUND_HINT_POPUP);
		break;

	case FLOATER_HINT_PICK:
		mLayers = 0x800;
		mTimeLeft = 5.0;
		gApp->PlaySample(SOUND_HINT_POPUP);
		break;

	case FLOATER_BROKEN_HEART:
		mTimeLeft = 1.2;
		mLayers = 0x80;
		break;

	case FLOATER_CUPID_HEART:
		mTimeLeft = 1.0;
		break;

	case FLOATER_CRYSTAL_GLOW:
		mTimeLeft = 3.0;
		break;

	case FLOATER_WAVE_RESULT:
		mLayers = 0x400;
		mTimeLeft = 7.0;
		gApp->mMusicInterface->FadeOutAll(true, 0.01);
		gApp->PlaySample(SOUND_STING_END_OF_WAVE);
		break;

	case FLOATER_LEVEL_COMPLETE:
		mTimeLeft = 13.0;
		mLayers = 0x400;
		break;

	case FLOATER_FLASH:
		mTimeLeft = 0.5;
		mLayers = 0x800;
		break;

	default:
		break;
	}
}

// 0x444ac0
Floater::~Floater()
{
	switch (mType)
	{
	case FLOATER_SCORE_FLY:
	case FLOATER_SCORE_POP:
	case FLOATER_SCORE_BIG:
		delete mTextImage;
		break;

	case FLOATER_GAME_OVER:
		delete mTextImage;
		if (gApp->mGameScreen != nullptr)
			gApp->DoGameOver();
		break;

	case FLOATER_WAVE_RESULT:
		if (mWaveIndex == 0)
			gApp->StartMusic(1, false);
		GetStats()->Set(STAT_SPEED_TIMER, 0.0);
		break;

	case FLOATER_LEVEL_COMPLETE:
		gApp->StartMusic(1, false);
		GetStats()->Set(STAT_SPEED_TIMER, 0.0);
		break;
	}

	if (mPendingPoints != 0)
		GetScore()->AddQuiet(mPendingPoints);
}

// 0x4449c0
void Floater::SetImage(Image* theImage)
{
	mImage = theImage;
}

// 0x4449d0
void Floater::SetSpeed(double theSpeed)
{
	mSpeed = theSpeed;
}

// 0x4449e0
void Floater::SetWaveResult(int theMisses, int theSwings, int thePoints, int theWave)
{
	mWaveMisses = theMisses;
	mWaveSwings = theSwings;
	mWaveIndex = theWave;
	if (theSwings == 0)
		mWavePercent = 0;
	else
		mWavePercent = ToInt((double)(theSwings - theMisses) / (double)theSwings * 100.0);

	if (mWavePercent >= 0x62)
		mWaveBonus = ToInt((double)thePoints * 0.2);
	else if (mWavePercent >= 0x5f)
		mWaveBonus = ToInt((double)thePoints * 0.1);
	else
		mWaveBonus = 0;
}

// 0x444a80
void Floater::SetLevelCounts(int theSmashed, int theChain, int theSpeed)
{
	mLevelSmashed = theSmashed;
	mLevelChain = theChain;
	mLevelSpeed = theSpeed;
}

// 0x444ab0
void Floater::SetPendingPoints(int thePoints)
{
	mPendingPoints = thePoints;
}

// 0x444500
void Floater::Update(float theFrac)
{
	if (mType == FLOATER_RISING_TEXT)
	{
		mY = mY - ((double)gApp->mHeight / mScrollDivisor) * (double)theFrac;
	}
	else if (mType == FLOATER_BROKEN_HEART)
	{
		mX = (double)theFrac * 0.1 + mX;
		mY = mVelocity + mY;
		mVelocity = (double)theFrac * 0.01 + mVelocity;
	}
	else if (mType == FLOATER_SHATTER || mType == FLOATER_SHATTER_SIDE)
	{
		// the points are awarded when the picture has come together, a fifth of the way through
		if (!(0.65 <= mTimeLeft * 0.25) && !mScored)
		{
			if (mStyle != 0)
			{
				double anOffset = (mType == FLOATER_SHATTER) ? (double)(IMAGE_SPEED_BONUS->mHeight / 8) + mY : mY - 25.0;
				int aY = ToInt(anOffset);
				int aX = ToInt(mX);
				GetScore()->AddPoints(mStyle, aX, aY, 1);
			}
			mScored = true;
		}
	}
	else if (mType == FLOATER_WAVE_RESULT && 1.5 <= mTimeLeft && mTimeLeft < 5.5)
	{
		// the bar has filled up to the wave's accuracy: the bonus is paid and the burst of stars goes off
		double aFill = 1.0 - (mTimeLeft - 1.5) * 0.25;
		if (aFill <= 0.6)
			aFill = aFill * 1.6666666666666667;
		else
			aFill = 1.0;

		if ((double)mWavePercent * 0.01 <= aFill)
		{
			if (!mScored)
			{
				if (mWaveBonus != 0)
				{
					int aY = -30 - ToInt((double)gApp->mHeight * -0.5);
					GetScore()->AddPoints(mWaveBonus, gApp->mWidth / 2, aY, 1);
				}
				mScored = true;
			}

			if (!mSoundDone)
			{
				const double aDirection[2] = { 0.1, -0.1 };
				mSoundDone = true;
				Popper* aPopper = MakePopper(500.0, 210.0, POPPER_STARS, aDirection);
				aPopper->mLayers = 0x800;
				if ((double)mWavePercent == 100.0)
					gApp->PlaySample(SOUND_VOICE_BONUS_PERFECT);
				else if (mWavePercent >= 0x5f)
					gApp->PlaySample(SOUND_VOICE_BONUS_WELLDONE);
			}
		}
	}

	if (0.0 < mTimeLeft)
	{
		double anOldTime = mTimeLeft;
		mTimeLeft = mTimeLeft - ((double)theFrac * mSpeed) * 0.01;
		if (mTimeLeft <= 0.0)
		{
			mTimeLeft = 0.0;
			Kill();
		}

		if (mType == FLOATER_LEVEL_COMPLETE)
		{
			// the sounds of the summary and the points of its three lines go off as the time passes their marks
			static const double aSoundTimes[4] = { 6.0, 4.9, 4.4, 3.9 };
			static const double aPointTimes[4] = { 6.0, 4.5, 4.0, 3.5 };
			static const int aRowY[4] = { 0, 270, 340, 410 };

			for (int i = 0; i < 4; i++)
			{
				if (aSoundTimes[i] < anOldTime && mTimeLeft <= aSoundTimes[i])
				{
					int aSound = SOUND_HINT_POPUP;
					if (i == 0)
					{
						int aLevel = gApp->mGameScreen->mLevel;
						int anApplause = aLevel % 4;
						if (anApplause < 0)
							anApplause = 0;
						static int* anApplauseSounds[4] = { &SOUND_CROWD_APPLAUSE_1, &SOUND_CROWD_APPLAUSE_2,
							&SOUND_CROWD_APPLAUSE_3, &SOUND_CROWD_APPLAUSE_4 };
						aSound = *anApplauseSounds[anApplause];
					}
					gApp->PlaySample(aSound);
				}
			}

			for (int i = 1; i < 4; i++)
			{
				if (aPointTimes[i] < anOldTime && mTimeLeft <= aPointTimes[i])
				{
					int aPoints;
					if (i == 1)
						aPoints = mLevelSmashed * 100;
					else if (i == 2)
						aPoints = mLevelChain * 200;
					else
						aPoints = mLevelSpeed * 100;
					GetScore()->AddPoints(aPoints, gApp->mWidth - 70, aRowY[i] - 65, 1);
				}
			}
		}
	}
}

// 0x444c20: the wave's accuracy as a bar that fills up to its percentage, with the percentage counting up beside it
void Floater::DrawBar(Graphics* g, int theX, int theY, double thePercent, double theFill)
{
	double aPercentFraction = thePercent * 0.01;
	double aFill = aPercentFraction;
	if (theFill < aPercentFraction)
		aFill = theFill;

	g->SetColor(Color(255, 255, 255, 255));
	g->SetColorizeImages(false);
	DrawFillBar(g, IMAGE_WAVE_BAR_EMPTY, IMAGE_WAVE_BAR, theX - IMAGE_WAVE_BAR_EMPTY->mWidth / 2,
		theY - IMAGE_WAVE_BAR_EMPTY->mHeight / 2, aFill, 5, 3);

	g->SetColor(Color(255, 255, 255, 255));
	g->SetFont(FONT_KIDPRINT19BOLD);
	g->SetColorizeImages(false);
	double aRatio = theFill / aPercentFraction;
	if (!(aRatio != aRatio) && 1.0 < aRatio)
		aRatio = 1.0;
	std::string aText = FormatInt("%d%%", ToInt(aRatio * thePercent));
	int aWidth = g->GetFont()->StringWidth(aText);
	g->DrawString(aText, theX - aWidth + 0x71, theY + 5);
}

// 0x445740: the start of the text, as many of its letters as the fraction allows
static void DrawTypedText(Graphics* g, const std::string& theText, int theX, int theY, double theFraction)
{
	int aCount = ToInt((double)theText.length() * theFraction);
	g->DrawString(theText.substr(0, aCount), theX, theY);
}

// 0x445800
void Floater::Draw(Graphics* g)
{
	Graphics aG(*g);
	const double aTime = mTimeLeft;

	switch (mType)
	{
	case FLOATER_RISING_TEXT:
	{
		Font* aFont = FONT_ARIAL10B;
		int aWidth = aFont->StringWidth(mText);
		int aHeight = aFont->GetHeight();
		int aY = ToInt(mY - (double)aHeight * 0.5);
		int aX = ToInt(mX - (double)aWidth * 0.5);
		int anAlpha = ToInt(sin((aTime * 0.4347826086956522) * HALF_PI_FLOAT) * 255.0);

		// the text blinks between white and pink
		double aBlend = fabs(sin(aTime * 4.0));
		double aKeep = 1.0 - aBlend;
		double aBlue = 255.0 * aKeep + 200.0 * aBlend;
		double aGreen = 255.0 * aKeep + 200.0 * aBlend;
		double aRed = 255.0 * aKeep + 255.0 * aBlend;
		if (255.0 < aBlue)
			aBlue = 255.0;
		if (255.0 < aGreen)
			aGreen = 255.0;
		if (255.0 < aRed)
			aRed = 255.0;
		Color aColor(ToInt(aRed), ToInt(aGreen), ToInt(aBlue), anAlpha);
		Rect aClip(aX, aY, aWidth, aHeight);
		aFont->DrawString(&aG, aX, aY, mText, aColor, aClip);
		break;
	}

	case FLOATER_STRIKE:
	case FLOATER_STRIKE_WEAK:
	case FLOATER_MISS:
	case FLOATER_HINT_CLICK:
	case FLOATER_HINT_DONT_MISS:
	case FLOATER_HINT_PICK:
	case FLOATER_CRYSTAL_GLOW:
	{
		Image* anImage = nullptr;
		aG.SetColorizeImages(true);
		switch (mType)
		{
		case FLOATER_STRIKE:
			aG.SetColor(Color(255, 255, 255, ToInt(sin(aTime * HALF_PI) * 255.0)));
			anImage = IMAGE_STRIKE;
			break;
		case FLOATER_STRIKE_WEAK:
			aG.SetColor(Color(255, 255, 255, ToInt(sin(aTime * HALF_PI) * 255.0)));
			anImage = IMAGE_STRIKEINEFFECTIVE;
			break;
		case FLOATER_MISS:
			aG.SetColor(Color(255, 255, 255, ToInt(sin(aTime * HALF_PI) * 255.0)));
			anImage = IMAGE_MISS;
			break;
		case FLOATER_HINT_CLICK:
			aG.SetColor(Color(255, 255, 255, ToInt(sin(aTime * 0.5235987755982988) * 255.0)));
			anImage = IMAGE_INGAMEUI_HELP_CLICKHERE;
			break;
		case FLOATER_HINT_DONT_MISS:
			aG.SetColor(Color(255, 255, 255, ToInt(sin(aTime * 0.2617993877991494) * 255.0)));
			anImage = IMAGE_INGAMEUI_HELP_DONTMISS;
			break;
		case FLOATER_HINT_PICK:
			aG.SetColor(Color(255, 255, 255, ToInt(sin(aTime * 0.3141592653589793) * 255.0)));
			anImage = (gApp->mGameMode == 0 || gApp->mGameMode == 1) ? IMAGE_INGAMEUI_HELP_PICKCOINSANDHEARTS
				: IMAGE_INGAMEUI_HELP_PICKCOINS;
			break;
		case FLOATER_CRYSTAL_GLOW:
		{
			double aPart = aTime * 0.3333333333333333;
			if (aPart < 0.5)
			{
				aG.SetColorizeImages(true);
				aG.SetColor(Color(255, 255, 255, ToInt(sin(aPart * PI) * 255.0)));
			}
			anImage = IMAGE_CRYSTAL_BALL_GLOW;
			break;
		}
		}
		int aY = ToInt(mY - (double)(anImage->mHeight / 2));
		int aX = ToInt(mX - (double)(anImage->mWidth / 2));
		aG.DrawImage(anImage, aX, aY);
		break;
	}

	case FLOATER_STRIKE_BIG:
	{
		aG.SetColorizeImages(true);
		aG.SetColor(Color(255, 255, 255, ToInt(sin(aTime * HALF_PI) * 255.0)));
		Image* anImage = IMAGE_STRIKE;
		int aWidth = ToInt((double)anImage->mWidth * 2.5);
		int aHeight = ToInt((double)anImage->mHeight * 2.5);
		int aY = ToInt(mY - (double)(aHeight / 2));
		int aX = ToInt(mX - (double)(aWidth / 2));
		aG.DrawImage(anImage, aX, aY, aWidth, aHeight);
		break;
	}

	case FLOATER_BROKEN_HEART:
	{
		aG.SetColorizeImages(true);
		aG.SetColor(Color(255, 255, 255, ToInt(sin(aTime * HALF_PI) * 255.0)));
		aG.DrawImageF(IMAGE_BROKENHEART, (float)(mX - (double)(IMAGE_BROKENHEART->mWidth / 2)),
			(float)(mY - (double)(IMAGE_BROKENHEART->mHeight / 2)));
		break;
	}

	case FLOATER_CUPID_HEART:
	case FLOATER_FLASH:
	{
		Image* anImage;
		double aScale;
		if (mType == FLOATER_FLASH)
		{
			anImage = IMAGE_FLASH;
			aScale = cos(((aTime + aTime) * 6.283185307179586) + PI) + 1.0;
		}
		else
		{
			anImage = IMAGE_CUPIDO_HEART_EFFECT;
			if (aTime < 0.5)
			{
				aG.SetColorizeImages(true);
				aG.SetColor(Color(255, 255, 255, ToInt(sin(aTime * PI) * 255.0)));
			}
			aScale = aTime * 0.666 + 1.0;
		}
		int aHeight = ToInt((double)anImage->mHeight * aScale);
		int aWidth = ToInt((double)anImage->mWidth * aScale);
		int aY = ToInt(mY - (double)(anImage->mHeight / 2) * aScale);
		int aX = ToInt(mX - (double)(anImage->mWidth / 2) * aScale);
		aG.DrawImage(anImage, aX, aY, aWidth, aHeight);
		break;
	}

	case FLOATER_SCORE_FLY:
	case FLOATER_SCORE_BIG:
	{
		// the score grows, then flies to the score display and shrinks
		ScoreDisplayPos aTarget = GetScoreDisplayPos();
		double aDeltaY = aTarget.mY - mY;
		double aDeltaX = aTarget.mX - mX;
		aG.SetColorizeImages(false);

		double aFly;
		double aGrow;
		if (mType == FLOATER_SCORE_FLY)
		{
			aFly = sin(Ease(aTime, 0.0, 0.9) * HALF_PI_FLOAT);
			aGrow = 0.5;
			if (0.3 < aTime)
				aGrow = (sin(Ease((1.0 - aTime) * 1.4285714285714286, 0.3, 0.7) * PI_FLOAT) + 1.0) * 0.5;
		}
		else
		{
			aFly = sin(Ease(aTime * 0.5, 0.4, 0.9) * HALF_PI_FLOAT);
			aGrow = 0.0;
			double aHalf = aTime * 0.5;
			if (0.5 < aHalf)
				aGrow = sin(Ease((1.0 - aHalf) + (1.0 - aHalf), 0.3, 0.7) * PI_FLOAT) * 0.5;
		}
		double aX = (1.0 - aFly) * aDeltaX + mX;
		double aY = aDeltaY * (1.0 - aFly) + mY;
		double aScale = aFly + aGrow;

		if ((double)mImage->mWidth * aScale > 5.0)
		{
			int aHeight = ToInt(aScale * (double)mImage->mHeight);
			int aWidth = ToInt((double)mImage->mWidth * aScale);
			int aDrawY = ToInt(aY - (aScale * (double)mImage->mHeight) * 0.5);
			int aDrawX = ToInt(aX - ((double)mImage->mWidth * aScale) * 0.5);
			aG.DrawImage(mImage, aDrawX, aDrawY, aWidth, aHeight);
		}
		break;
	}

	case FLOATER_SCORE_POP:
	{
		aG.SetColorizeImages(false);
		double aScale = sin(Ease(aTime, 0.1, 0.1) * PI_FLOAT) * 0.5 + 1.0;
		if (5.0 < (double)mImage->mWidth * aScale)
		{
			if (aTime < 0.5)
			{
				aG.SetColorizeImages(true);
				aG.SetColor(Color(255, 255, 255, ToInt(aTime * 510.0)));
			}
			double aScaledWidth = (double)mImage->mWidth * aScale;
			int aHeight = ToInt((double)mImage->mHeight * aScale);
			int aWidth = ToInt(aScaledWidth);
			int aY = ToInt(mY - ((double)mImage->mHeight * aScale) * 0.5);
			int aX = ToInt(mX - aScaledWidth * 0.5);
			aG.DrawImage(mImage, aX, aY, aWidth, aHeight);
			aG.SetColorizeImages(false);
		}
		break;
	}

	case FLOATER_GAME_OVER:
	{
		// drops in, stays, then falls away
		double aUnit;
		double aScale;
		double aY;
		if (aTime <= 3.0)
		{
			aUnit = aTime / 3.0;
			double aRise = Ease(aUnit, 0.1, 0.9);
			aScale = sin(aRise * HALF_PI) + 1.5;
			double aDrop = Ease(aUnit, 0.5, 0.5);
			aY = (mY - (1.0 - aDrop) * 200.0) - 100.0;
			if (aUnit < 0.2)
			{
				aG.SetColorizeImages(true);
				aG.SetColor(Color(255, 255, 255, ToInt((aUnit * 5.0) * 255.0)));
			}
		}
		else
		{
			aUnit = (aTime - 3.0) / 2.0;
			double aRise = Ease(aUnit, 0.1, 0.9);
			aScale = sin((aRise + 1.0) * HALF_PI) + 1.5;
			double aDrop = Ease(aUnit, 0.5, 0.5);
			aY = (aDrop * 400.0 - 100.0) + mY;
			if (0.8 < aUnit)
			{
				aG.SetColorizeImages(true);
				aG.SetColor(Color(255, 255, 255, ToInt(((aUnit - 0.8) * 5.000000000000001) * 255.0)));
			}
		}
		double aScaledWidth = (double)mImage->mWidth * aScale;
		int aHeight = ToInt((double)mImage->mHeight * aScale);
		int aWidth = ToInt(aScaledWidth);
		int aDrawY = ToInt(aY - ((double)mImage->mHeight * aScale) * 0.5);
		int aDrawX = ToInt(mX - aScaledWidth * 0.5);
		aG.DrawImage(mImage, aDrawX, aDrawY, aWidth, aHeight);
		aG.SetColorizeImages(false);
		break;
	}

	case FLOATER_SHATTER:
	case FLOATER_SHATTER_SIDE:
	{
		// the picture is cut into 25 tiles that fly apart (to the front of the screen) and spin as it comes and goes
		int aTileWidth = mImage->mWidth / 5;
		int aTileHeight = mImage->mHeight / 5;
		Rect aTiles[25];
		for (int aRow = 0; aRow < 5; aRow++)
		{
			for (int aCol = 0; aCol < 5; aCol++)
			{
				Rect& aTile = aTiles[aRow * 5 + aCol];
				aTile = Rect(aCol * aTileWidth, aRow * aTileHeight, aTileWidth, aTileHeight);
				if (aRow == 4)
					aTile.mHeight = mImage->mHeight % 5 + aTileHeight;
				if (aCol == 4)
					aTile.mWidth = mImage->mWidth % 5 + aTileWidth;
			}
		}

		double aProgress = aTime * 0.25;
		double aSpread;
		int aShift = 0;
		if (aProgress <= 0.8)
		{
			if (aProgress <= 0.2)
			{
				double aRaw = (0.2 - aProgress) * 5.0;
				aSpread = Ease(aRaw, 0.3, 1.0);
				if (mType == FLOATER_SHATTER_SIDE)
					aShift = ToInt(aRaw * 100.0);
			}
			else
			{
				aSpread = 0.0;
			}
		}
		else
		{
			double aRaw = (aProgress - 0.8) * 5.0;
			aSpread = Ease(aRaw, 0.3, 0.7);
			if (mType == FLOATER_SHATTER_SIDE)
				aShift = ToInt(aRaw * -100.0);
		}

		int aBaseX = ToInt(mX - (double)mImage->mWidth * 0.5);
		int aBaseY = ToInt(mY - (double)mImage->mHeight * 0.5);
		int anAlpha = ToInt(((1.0 - aSpread) * (1.0 - aSpread)) * 255.0);
		aG.SetColorizeImages(true);
		aG.SetColor(Color(255, 255, 255, anAlpha));
		double aScale = aSpread * 10.0 + 1.0;
		double anAngle = (aSpread * aSpread) * TWO_PI_FLOAT;

		for (int i = 0; i < 25; i++)
		{
			const Rect& aTile = aTiles[i];
			int aCenterX = aTile.mWidth / 2 + aBaseX + aTile.mX;
			int aCenterY = aTile.mY + aTile.mHeight / 2 + aBaseY;
			double aVector[2];
			aVector[0] = ((double)aCenterX - mX) * aScale;
			aVector[1] = ((double)aCenterY - mY) * aScale;
			RotateVector(aVector, anAngle);
			int aDrawX = ToInt((aVector[0] + mX) - (double)(aTile.mWidth / 2));
			int aDrawY = ToInt((aVector[1] + mY) - (double)(aTile.mHeight / 2));
			aG.DrawImage(mImage, aShift + aDrawX, aDrawY, aTile);
		}
		break;
	}

	case FLOATER_WAVE_RESULT:
	{
		aG.SetColorizeImages(true);
		int anAlpha;
		int aYOffset;
		double aBarFill;
		if (1.5 <= aTime)
		{
			if (5.5 <= aTime)
			{
				double aCos = cos((aTime - 5.5) * 1.0471975511965976);
				anAlpha = ToInt(255.0 * aCos);
				aYOffset = ToInt((1.0 - aCos) * -500.0);
				aBarFill = 0.0;
			}
			else
			{
				aYOffset = 0;
				anAlpha = 0xff;
				aBarFill = 1.0 - (aTime - 1.5) * 0.25;
				if (aBarFill <= 0.6)
					aBarFill = aBarFill * 1.6666666666666667;
				else
					aBarFill = 1.0;
			}
		}
		else
		{
			double aSin = sin(aTime * 1.0471975511965976);
			anAlpha = ToInt(255.0 * aSin);
			aYOffset = ToInt((1.0 - aSin) * 500.0);
			aBarFill = 1.0;
		}

		Image* aBanner = IMAGE_WAVE_GOLD;
		if (mWavePercent < 0x62)
		{
			aBanner = IMAGE_WAVE_SILVER;
			if (mWavePercent < 0x5f)
				aBanner = IMAGE_WAVE_BRONZE;
		}

		aG.SetColor(Color(255, 255, 255, anAlpha));
		aG.DrawImage(aBanner, (gApp->mWidth - aBanner->mWidth) / 2, (gApp->mHeight - aBanner->mHeight) / 2 + aYOffset);

		std::string aTitle;
		std::string aDetail;
		Font* aDetailFont;
		if (aBanner == IMAGE_WAVE_GOLD)
		{
			aTitle = "Double Bonus";
			aDetail = FormatInt("%d", ToInt((double)mWaveBonus * aBarFill));
			aDetailFont = FONT_KIDPRINT27BOLD;
		}
		else if (aBanner == IMAGE_WAVE_SILVER)
		{
			aTitle = "Accuracy Bonus";
			aDetail = FormatInt("%d", ToInt((double)mWaveBonus * aBarFill));
			aDetailFont = FONT_KIDPRINT27BOLD;
		}
		else
		{
			aTitle = FormatInt("Less than %d%%,", 95);
			aDetail = "no bonus awarded";
			aDetailFont = FONT_KIDPRINT22BOLD;
		}

		aG.SetFont(aDetailFont);
		if (anAlpha > 0xff)
			anAlpha = 0xff;
		aG.SetColor(Color(255, 255, 255, anAlpha));
		int aDetailWidth = aG.GetFont()->StringWidth(aDetail);
		aG.DrawString(aDetail, (gApp->mWidth - aDetailWidth) / 2, gApp->mHeight / 2 + aYOffset + 0x1e);
		aG.SetFont(FONT_KIDPRINT22BOLD);
		int aTitleWidth = aG.GetFont()->StringWidth(aTitle);
		aG.DrawString(aTitle, (gApp->mWidth - aTitleWidth) / 2, gApp->mHeight / 2 + aYOffset + 3);

		DrawBar(&aG, gApp->mWidth / 2, gApp->mHeight / 2 + aYOffset - 0x3e, (double)mWavePercent, aBarFill);

		if (mWaveMisses != 0)
		{
			aG.SetFont(FONT_ARIAL10B);
			int aMissAlpha = ToInt(((double)anAlpha * aBarFill) * 0.5);
			if (aMissAlpha > 0x80)
				aMissAlpha = 0x80;
			aG.SetColor(Color(0, 0, 0, aMissAlpha));
			std::string aMisses = (mWaveMisses == 1) ? std::string("(1 miss)") : FormatInt("(%d misses)", mWaveMisses);
			int aMissesWidth = aG.GetFont()->StringWidth(aMisses);
			aG.DrawString(aMisses, (gApp->mWidth - aMissesWidth) / 2, gApp->mHeight / 2 + aYOffset - 0x23);
		}
		break;
	}

	case FLOATER_LEVEL_COMPLETE:
	{
		if (6.0 <= aTime)
			return;

		double aAmount;
		if (aTime < 0.5)
			aAmount = aTime + aTime;
		else if (aTime <= 5.5)
			aAmount = 1.0;
		else
			aAmount = 1.0 - ((aTime - 5.5) + (aTime - 5.5));
		double aSin = sin(aAmount * HALF_PI);

		aG.SetColor(Color(0, 0, 0, ToInt(aSin * 192.0)));
		aG.FillRect(0, 0, gApp->mWidth, gApp->mHeight);
		aG.SetColorizeImages(true);
		int anAlpha = ToInt(aSin * 255.0);
		aG.SetColor(Color(255, 255, 255, anAlpha));
		aG.SetFont(FONT_KIDPRINT27BOLD);
		Color aLabelColor(255, 255, 255, anAlpha);
		Color aNumberColor(255, 255, 200, anAlpha);
		aG.DrawImage(IMAGE_LEVEL_COMPLETED, (gApp->mWidth - IMAGE_LEVEL_COMPLETED->mWidth) / 2, 0x78);

		// each line types its label out and then shows its count; the lines begin 0.5 apart
		struct Row
		{
			const char*	mLabel;
			double		mHigh;
			double		mLow;
			int			mY;
			int			mCount;
		};
		Row aRows[3] = {
			{ "Gnomes smashed:", 6.0, 5.0, 0x10e, mLevelSmashed },
			{ "Longest chain of hammer hits:", 5.5, 4.5, 0x154, mLevelChain },
			{ "Speed bonuses:", 5.0, 4.0, 0x19a, mLevelSpeed }
		};
		for (int i = 0; i < 3; i++)
		{
			double aTyped = 1.0 - RampBetween(aTime, aRows[i].mHigh, aRows[i].mLow);
			aG.SetColor(aLabelColor);
			DrawTypedText(&aG, aRows[i].mLabel, 0xaa, aRows[i].mY, aTyped);
			if (aTyped == 1.0)
			{
				std::string aCount = FormatInt("%d", aRows[i].mCount);
				aG.SetColor(aNumberColor);
				int aWidth = aG.GetFont()->StringWidth(aCount);
				aG.DrawString(aCount, gApp->mWidth - aWidth - 0xaa, aRows[i].mY);
			}
		}
		break;
	}

	default:
		return;
	}
}

// 0x4456b0
Floater* Sexy::MakeFloater(int theType, double theX, double theY, const char* theText, int theStyle,
	const std::string& theName)
{
	Floater* aFloater = new Floater(theType, theX, theY, theText, theStyle);
	aFloater->SetName(theName);
	return aFloater;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The floaters that fly from a cubby to a counter. They share one update (0x447c60): the time runs down, the frame
// advances 0.7 a step.

// 0x447c60 (the shared update; each class keeps its time left at +0xa0 and its frame at +0xb0)
static void UpdateFlyer(Object* theObject, double* theTimeLeft, double* theFrame, float theFrac)
{
	if (0.0 < *theTimeLeft)
	{
		*theTimeLeft = *theTimeLeft - (double)theFrac * 0.01;
		if (*theTimeLeft <= 0.0)
		{
			*theTimeLeft = 0.0;
			theObject->Kill();
		}
	}
	*theFrame = *theFrame + 0.7;
}

// Where a flyer is at a point of its flight, 0 to 1: it jumps up by theLift first, then flies straight to its target
// (0x447dc0 and 0x448e40). theFirstPart is set while it is still in the jump.
static void FlyerPosition(double theT, double theX0, double theY0, double theX1, double theY1, double theLift, double& theX,
	double& theY, bool& theFirstPart)
{
	double aJumpedY = theY0 - theLift;
	double aDeltaX = theX1 - theX0;
	double aDeltaY = theY1 - aJumpedY;
	double aJumpFraction = theLift / (sqrt(aDeltaY * aDeltaY + aDeltaX * aDeltaX) + theLift);
	if (!(theT <= aJumpFraction))
	{
		double aAlong = (theT - aJumpFraction) / (1.0 - aJumpFraction);
		double aMoveX = (theX1 - theX0) * aAlong;
		theY = (theY0 - theLift) + (theY1 - aJumpedY) * aAlong;
		theX = theX0 + aMoveX;
		theFirstPart = false;
	}
	else
	{
		theX = theX0;
		theY = theY0 - (theT / aJumpFraction) * theLift;
		theFirstPart = true;
	}
}

// 0x447cf0
FloaterCoin::FloaterCoin(double theX0, double theY0, double theX1, double theY1, double theFrame, double theLift,
	const Rect& theClip) :
	Object(0x80),
	mX0(theX0),
	mY0(theY0),
	mX1(theX1),
	mY1(theY1),
	mTimeLeft(0.75),
	mDuration(0.75),
	mFrame(theFrame),
	mLift(theLift),
	mClip(theClip),
	mGrow(false),
	mKeepSize(false)
{
}

// 0x447c50
FloaterCoin::~FloaterCoin()
{
}

// 0x447c60
void FloaterCoin::Update(float theFrac)
{
	UpdateFlyer(this, &mTimeLeft, &mFrame, theFrac);
}

// 0x447cc0
void FloaterCoin::SetDuration(double theDuration)
{
	mTimeLeft = theDuration;
	mDuration = theDuration;
}

// 0x447ce0
void FloaterCoin::SetKeepSize(bool theKeep)
{
	mKeepSize = theKeep;
}

// 0x447dc0
void FloaterCoin::Draw(Graphics* g)
{
	Graphics aG(*g);

	double aT = Ease((mDuration - mTimeLeft) / mDuration, 0.5, 0.5);
	double aX;
	double aY;
	bool aFirstPart;
	FlyerPosition(aT, mX0, mY0, mX1, mY1, mLift, aX, aY, aFirstPart);
	if (aFirstPart && mClip.mWidth > 0)
		aG.ClipRect(mClip);

	double aGrow = aT;
	if (!mGrow)
		aGrow = mKeepSize ? 0.0 : 1.0 - aT;

	int aCelHeight = IMAGE_COIN->mHeight / IMAGE_COIN->mNumRows;
	double aSize = (double)aCelHeight * ((aGrow + 1.0) * 0.5);
	Rect aDest(ToInt(aX), ToInt(aY), ToInt(aSize), ToInt(aSize));
	aG.DrawImageCel(IMAGE_COIN, aDest, ToInt(mFrame) % 20, 0);
}

// 0x448030
FloaterCoin* Sexy::MakeFloaterCoin(double theX0, double theY0, double theX1, double theY1, double theFrame, double theLift,
	int theClipX, int theClipY, int theClipWidth, int theClipHeight)
{
	FloaterCoin* aFloater = new FloaterCoin(theX0, theY0, theX1, theY1, theFrame, theLift,
		Rect(theClipX, theClipY, theClipWidth, theClipHeight));
	aFloater->SetName("");
	return aFloater;
}

// 0x448130
FloaterHeart::FloaterHeart(double theX0, double theY0, double theX1, double theY1, double theFrame) :
	Object(0x80),
	mX0(theX0),
	mY0(theY0),
	mX1(theX1),
	mY1(theY1),
	mTimeLeft(0.75),
	mDuration(0.75),
	mFrame(theFrame)
{
}

// 0x4481a0
FloaterHeart::~FloaterHeart()
{
}

// 0x447c60
void FloaterHeart::Update(float theFrac)
{
	UpdateFlyer(this, &mTimeLeft, &mFrame, theFrac);
}

// 0x4481d0
void FloaterHeart::Draw(Graphics* g)
{
	Graphics aG(*g);

	double aT = Ease((mDuration - mTimeLeft) / mDuration, 0.5, 0.5);
	double aX = (mX1 - mX0) * aT + mX0;
	double aY = (mY1 - mY0) * aT + mY0;
	double aScale = aT * 0.25 + 1.0;
	double aWidth = (double)IMAGE_HEART->GetCelWidth() * aScale;
	double aHeight = (double)IMAGE_HEART->GetCelHeight() * aScale;

	if (aT > 0.9)
	{
		aG.SetColorizeImages(true);
		aG.SetColor(Color(255, 255, 255, ToInt((1.0 - (aT - 0.9) * 10.000000000000002) * 255.0)));
	}

	Rect aDest(ToInt(aX - aWidth * 0.5), ToInt(aY - aHeight * 0.5), ToInt(aWidth), ToInt(aHeight));
	aG.DrawImageCel(IMAGE_HEART, aDest, ToInt(mFrame) % 11, 0);
	aG.SetColorizeImages(false);
}

// 0x448380
FloaterHeart* Sexy::MakeFloaterHeart(double theX0, double theY0, double theX1, double theY1, double theFrame)
{
	FloaterHeart* aFloater = new FloaterHeart(theX0, theY0, theX1, theY1, theFrame);
	aFloater->SetName("");
	return aFloater;
}

// 0x4484e0
FloaterPhoto::FloaterPhoto(double theX, double theY, Image* theImage) :
	Object(0x80),
	mX(theX),
	mY(theY),
	mTimeLeft(3.0),
	mImage(theImage)
{
}

// 0x448460
FloaterPhoto::~FloaterPhoto()
{
}

// 0x448470
void FloaterPhoto::Update(float theFrac)
{
	if (0.0 < mTimeLeft)
	{
		mTimeLeft = mTimeLeft - (double)theFrac * 0.01;
		if (mTimeLeft <= 0.0)
		{
			mTimeLeft = 0.0;
			Kill();
		}
	}
}

// 0x4485c0. The picture comes in from the point it was taken at, growing as it moves to the middle of the screen; it
// then drops away and "Snapshot Saved!" is written under it. (Read from the exe's code with its arguments recovered by
// hand: to be checked when the look of the game is compared.)
void FloaterPhoto::Draw(Graphics* g)
{
	Graphics aG(*g);
	aG.SetFont(FONT_PTBANANASPLIT27);

	double anElapsed = 3.0 - mTimeLeft;
	int aFrameWidth = mImage->mWidth / 3;
	double aTextFade;
	if (anElapsed > 2.1)
	{
		double aMove = Ease((0.9000000000000004 - mTimeLeft) * 1.1111111111111107, 0.5, 0.95);
		double aFromY = (double)-mImage->mHeight;
		double aToY = (double)(gApp->mHeight / 2);
		double aY = (aToY - aFromY) * aMove + aFromY;
		double aCenterX = (double)(gApp->mWidth / 2);
		aG.DrawImage(mImage, ToInt(aCenterX - (double)aFrameWidth * 0.5), ToInt(aY - (double)mImage->mHeight * 0.5),
			Rect(0, 0, aFrameWidth, mImage->mHeight));
		aTextFade = aMove;
	}
	else
	{
		double aMove = Ease(anElapsed * 0.4761904761904763, 0.05, 0.5);
		double aScale = 0.75 * aMove + 0.25;
		double aWidth = (double)aFrameWidth * aScale;
		double aHeight = (double)mImage->mHeight * aScale;
		double aX = ((double)(gApp->mWidth / 2) - mX) * aMove + mX;
		double aY = ((double)(gApp->mHeight / 2) - mY) * aMove + mY;
		aG.SetColor(Color(255, 255, 255, 0x80 - ToInt(128.0 * aMove)));
		aG.SetColorizeImages(true);
		aG.DrawImage(mImage, Rect(ToInt(aX - aWidth * 0.5), ToInt(aY - aHeight * 0.5), ToInt(aWidth), ToInt(aHeight)),
			Rect(0, 0, aFrameWidth, mImage->mHeight));
		aTextFade = 2.0;		// the text is not shown yet
	}

	aG.SetColor(Color(255, 255, 255, ToInt((1.0 - aTextFade) * 255.0)));
	std::string aText = "Snapshot Saved!";
	int aWidth = aG.GetFont()->StringWidth(aText);
	aG.DrawString(aText, gApp->mWidth / 2 - aWidth / 2, mImage->mHeight / 2 + gApp->mHeight / 2);
}

// 0x4484e0
FloaterPhoto* Sexy::MakeFloaterPhoto(double theX, double theY, Image* theImage)
{
	FloaterPhoto* aFloater = new FloaterPhoto(theX, theY, theImage);
	aFloater->SetName("");
	return aFloater;
}

// 0x448a70
FloaterRotating::FloaterRotating(double theX0, double theY0, double theX1, double theY1, Image* theImage, int theCelRow,
	int theCelCol, double theStartAngle) :
	Object(0x80),
	mX0(theX0),
	mY0(theY0),
	mX1(theX1),
	mY1(theY1),
	mTimeLeft(1.0),
	mDuration(1.0),
	mImage(theImage),
	mCelRow(theCelRow),
	mCelCol(theCelCol),
	mStartAngle(theStartAngle)
{
}

// 0x448b00
FloaterRotating::~FloaterRotating()
{
}

// 0x447c60
void FloaterRotating::Update(float theFrac)
{
	double aUnusedFrame = 0.0;
	UpdateFlyer(this, &mTimeLeft, &aUnusedFrame, theFrac);
}

// 0x448b10
void FloaterRotating::Draw(Graphics* g)
{
	double aT = (mDuration - mTimeLeft) / mDuration;
	double aX = (mX1 - mX0) * aT + mX0;
	double aY = (mY1 - mY0) * aT + mY0;
	double aCelWidth = (double)mImage->GetCelWidth();
	double aCelHeight = (double)mImage->GetCelHeight();
	Rect aCel = mImage->GetCelRect(mCelCol, mCelRow);
	g->DrawImageRotatedF(mImage, (float)(aX - aCelWidth * 0.5), (float)(aY - aCelHeight * 0.5),
		(TWO_PI_FLOAT - mStartAngle) * aT + mStartAngle, &aCel);
}

// 0x448c30
FloaterRotating* Sexy::MakeFloaterRotating(double theX0, double theY0, double theX1, double theY1, Image* theImage,
	int theCelRow, int theCelCol, double theStartAngle)
{
	FloaterRotating* aFloater = new FloaterRotating(theX0, theY0, theX1, theY1, theImage, theCelRow, theCelCol, theStartAngle);
	aFloater->SetName("");
	return aFloater;
}

// 0x448d30
FloaterShrinking::FloaterShrinking(double theX0, double theY0, double theX1, double theY1, Image* theImage, int theCelRow,
	int theCelCol, double theLift, const Rect& theClip) :
	Object(0x80),
	mX0(theX0),
	mY0(theY0),
	mX1(theX1),
	mY1(theY1),
	mTimeLeft(0.75),
	mDuration(0.75),
	mLift(theLift),
	mClip(theClip),
	mCelRow(theCelRow),
	mCelCol(theCelCol),
	mImage(theImage)
{
}

// 0x448d20
FloaterShrinking::~FloaterShrinking()
{
}

// 0x447c60
void FloaterShrinking::Update(float theFrac)
{
	double aUnusedFrame = 0.0;
	UpdateFlyer(this, &mTimeLeft, &aUnusedFrame, theFrac);
}

// 0x448e40
void FloaterShrinking::Draw(Graphics* g)
{
	Graphics aG(*g);

	double aT = Ease((mDuration - mTimeLeft) / mDuration, 0.5, 0.5);
	double aX;
	double aY;
	bool aFirstPart;
	FlyerPosition(aT, mX0, mY0, mX1, mY1, mLift, aX, aY, aFirstPart);
	if (aFirstPart && mClip.mWidth > 0)
		aG.ClipRect(mClip);

	double aScale = ((1.0 - aT) + 1.0) * 0.5;
	double aWidth = (double)mImage->GetCelWidth() * aScale;
	double aHeight = (double)mImage->GetCelHeight() * aScale;
	Rect aDest(ToInt(aX), ToInt(aY), ToInt(aWidth), ToInt(aHeight));
	aG.DrawImage(mImage, aDest, mImage->GetCelRect(mCelCol, mCelRow));
}

// 0x449090
FloaterShrinking* Sexy::MakeFloaterShrinking(double theX0, double theY0, double theX1, double theY1, Image* theImage,
	int theCelRow, int theCelCol, double theLift, int theClipX, int theClipY, int theClipWidth, int theClipHeight)
{
	FloaterShrinking* aFloater = new FloaterShrinking(theX0, theY0, theX1, theY1, theImage, theCelRow, theCelCol, theLift,
		Rect(theClipX, theClipY, theClipWidth, theClipHeight));
	aFloater->SetName("");
	return aFloater;
}
