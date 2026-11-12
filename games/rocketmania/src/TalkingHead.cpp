// The talking head and the hover tips (RocketManiaGame 0x413aa0-0x414000, 0x4161a0-0x416eb0). See TalkingHead.h.

#include "TalkingHead.h"
#include "Effects.h"
#include "Board.h"
#include "RocketApp.h"
#include "GameText.h"
#include "Common.h"
#include "Graphics.h"
#include "Image.h"
#include "ImageFont.h"
#include "SoundManager.h"
#include "SoundInstance.h"
#include "WidgetManager.h"
#include <SDL.h>
#include <math.h>

#include "HeadData.inc"

using namespace Sexy;

// the fall of the original's float to int conversion (__ftol): truncation
static int Ftol(double theValue)
{
	return (int) theValue;
}

// ---------------------------------------------------------------------------------------------------------
// TalkingHead

// 0x4161a0
TalkingHead::TalkingHead(RocketApp* theApp) :
	ButtonWidget(0, NULL),
	mApp(theApp),
	mTrail(NULL),
	mFlying(false),
	mTargetX(0),
	mTargetY(0),
	mShadow(true),
	mSlideSpeed(0),
	mSlidePos(0),
	mSlideDelay(0),
	mHidden(false),
	mSkipDraw(false),
	mTilt(0.0),
	mTiltSpeed(0.004),
	mPosX(0.0),
	mPosY(0.0),
	mVelX(0.0),
	mVelY(0.0),
	mSway(20.0),
	mMood(0),
	mMoodFrame(0),
	mMoodStep(0),
	mMouth(0),
	mIdle(true),
	mVoice(NULL)
{
	// the head is only to be looked at: no mouse, no finger, no focus
	mMouseVisible = false;
	mDoFinger = false;
	mWantsFocus = false;
	mHasTransparencies = true;
	Place(0, 0);
	SetMood(0x11);
}

// 0x4162f0: the voice line stops and goes
TalkingHead::~TalkingHead()
{
	if (mVoice != NULL)
	{
		mVoice->Stop();
		mVoice->Release();
		mVoice = NULL;
	}
}

// 0x416360: a mood starts at its first mouth
void TalkingHead::SetMood(int theMood)
{
	mMood = theMood;
	mMoodStep = 0;
	mMoodFrame = 0;
	mMouth = sHeadMoods[theMood][0];
	MarkDirtyFull();
}

// 0x416630
void TalkingHead::Place(int theX, int theY)
{
	mTargetX = theX;
	mTargetY = theY;
	mFlying = false;
	mHidden = false;
}

// 0x416660: the widget is put 20 above the place it is told; the head's numbers follow the widget
void TalkingHead::Move(int theNewX, int theNewY)
{
	Widget::Move(theNewX, theNewY - 0x14);
	mPosX = (double) mX;
	mPosY = (double) mY;
	Place(mX, mY);
}

// 0x416aa0
void TalkingHead::Say(int theSound)
{
	if (mVoice != NULL)
	{
		mVoice->Stop();
		mVoice->Release();
		mVoice = NULL;
	}
	mVoice = mApp->mSoundManager->GetSoundInstance(theSound);
	if (mVoice != NULL)
		mVoice->Play(false, false);
}

// 0x416e00: climbing (up or down) and past the delay
bool TalkingHead::IsSliding() const
{
	return mSlideSpeed != 0 && mSlideDelay == 0;
}

// 0x416e20: all the way up and still
bool TalkingHead::IsInHole() const
{
	return mSlidePos == 599 && mSlideSpeed == 0 && mSlideDelay == 0;
}

// 0x416e50
void TalkingHead::StartSlide(bool theUp, int theDelay)
{
	if (mSlideSpeed == 0)
		mSlideDelay = theDelay;
	mSlideSpeed = theUp ? 12 : -12;
}

// 0x416e80
void TalkingHead::SetSlide(bool theUp)
{
	mSlideDelay = 0;
	mSlideSpeed = 0;
	mSlidePos = theUp ? 599 : 0;
}

// 0x416de0
void TalkingHead::Surface()
{
	Move(-10, 0x122);
	mWidgetManager->BringToFront(this);
}

// 0x4166b0
void TalkingHead::Update()
{
	ButtonWidget::Update();

	if ((int) SDL_GetTicks() - (int) mApp->mLastUserInputTick < 120001)
		mIdle = false;
	else if (!mIdle)
		mIdle = true;

	if (0 < mSlideDelay)
		mSlideDelay--;
	if (IsSliding())
	{
		mSlidePos = mSlidePos + mSlideSpeed;
		if (mSlidePos < 0)
		{
			mSlidePos = 0;
			mSlideSpeed = 0;
			mHidden = true;
		}
		else if (599 < mSlidePos)
		{
			mSlidePos = 599;
			mSlideSpeed = 0;
			Surface();
		}
	}

	// the pendulum
	double aOldTilt = mTilt;
	mTilt = aOldTilt + mTiltSpeed;
	mTiltSpeed = mTiltSpeed - (aOldTilt + mTiltSpeed) * 0.0004;

	if (!mFlying)
	{
		// the springs
		double aPullX = ((double) mTargetX - mPosX) * 0.0025;
		double aNewVelX = (aPullX + mVelX) * 0.94;
		mVelX = aNewVelX;
		mTilt = aPullX * 0.01 + mTilt * 0.99;
		double aNewVelY = (((double) mTargetY - mPosY) * 0.0025 + mVelY) * 0.94;
		mVelY = aNewVelY;
		mPosX = aNewVelX + mPosX;
		mPosY = aNewVelY + mPosY;
	}
	else
	{
		// the ride in: fast to the top of the screen, slow down from there on a swing
		mTilt = mTilt * 0.99;
		double aStep;
		if (0.0 <= mPosY)
			aStep = 1.33;
		else
			aStep = 3.0;
		mPosY = mPosY + aStep;
		mPosX = sin(mPosY * 0.0125) * 20.0 + (double) mTargetX;
		if ((double) mTargetY == mPosY)
			mFlying = false;
	}

	if (mUpdateCnt % 8 == 0 || 1.0 < fabs(mVelX) || 1.0 < fabs(mVelY))
		MarkDirty();

	int aX;
	int aY;
	if (0.1 <= mVelX || 0.1 <= mVelY)
	{
		aX = Ftol(mPosX);
		aY = Ftol(mPosY);
	}
	else
	{
		// at rest it still floats a little
		int aCount = mUpdateCnt;
		aX = Ftol(sin((double) (aCount + 0x1bb) * 0.02) * 2.0 + mPosX);
		aY = Ftol(sin((double) aCount * 0.027777777777777776) + mPosY);
	}
	Resize(aX, aY, mWidth, mHeight);

	if (mMood != -1)
	{
		mMoodFrame++;
		if (sHeadMoods[mMood][mMoodStep * 2 + 1] * 2 <= mMoodFrame)
		{
			mMoodFrame = 0;
			mMoodStep++;
			mMouth = sHeadMoods[mMood][mMoodStep * 2];
			if (mMouth == -1)
			{
				mMood = -1;
				mMouth = 4;
			}
			MarkDirtyFull();
		}
	}

	// now and then it makes a face of its own
	int aRand = GlobalRand();
	if (aRand % 250 == 0 && mMood == -1)
		SetMood(GlobalRand() % 5 + 0x12);

	mSway = sin((mVelY + mVelX) * 0.2) * 10.0 + 15.0;
}

// 0x416b10
void TalkingHead::Draw(Graphics* g)
{
	if (mHidden)
	{
		mSkipDraw = false;
		return;
	}
	if (mSkipDraw)
	{
		mSkipDraw = false;
		return;
	}

	Image** anImages = mApp->mImages;
	if (mShadow && !IsSliding() && !IsInHole())
		g->DrawImage(anImages[IMAGE_SHADOW], 0, 0x28);
	if (!IsSliding() && !IsInHole())
		g->DrawImage(anImages[IMAGE_BODY], Ftol(mSway), 0x14);

	if (IsSliding())
		return;

	// the head: the seven mouths, then the two turned heads
	Image* aHead;
	if (mMouth < 7)
		aHead = mApp->mHeadFrames[mMouth];
	else if (mMouth == 7)
		aHead = mApp->mHeadRotate1;
	else
		aHead = mApp->mHeadRotate2;

	if (!IsInHole())
	{
		if (0.005 <= fabs(mTilt))
			g->DrawImageRotated(aHead, Ftol(mSway), Ftol(40.0 - mSway), mTilt);
		else
			g->DrawImage(aHead, Ftol(mSway), Ftol(40.0 - mSway));
	}
	else
	{
		// in the hole the head is shown smaller
		Rect aSrc(0, 0, aHead->GetWidth(), aHead->GetHeight());
		Rect aDest(0, 0, Ftol((double) aHead->GetWidth() * 0.8), Ftol((double) aHead->GetHeight() * 0.8));
		g->DrawImage(aHead, aDest, aSrc);
	}
}

// 0x416d50: the board draws the hole where the head climbs out (the picture changes with how far it is)
void TalkingHead::DrawHole(Graphics* g)
{
	Image** anImages = mApp->mImages;
	if (!IsSliding() && mSlidePos == 0)
	{
		g->DrawImage(anImages[IMAGE_OUTOFHOLE01], 0xc, 0x143);
		return;
	}
	if (IsSliding())
		g->DrawImage(anImages[IMAGE_OUTOFHOLE01 + mSlidePos / 100], 0xc, 0x143);
}

// 0x4163a0: the level ends with the head riding in on a rocket (its flames are an effect of the board)
void TalkingHead::FlyIn(EffectManager* theEffects)
{
	if (theEffects->Has(mTrail))
		return;
	mTilt = 0.0;
	mTiltSpeed = 0.004;
	Move(200, -200);
	Place(0x46, 0x136);
	SetShadow(true);
	mFlying = true;
	mTrail = theEffects->Add(0, false);
	mTrail->mLife = 300;
	mTrail->mc0 = 2;
	SetMood(0);
	mApp->PlaySample(8);
}

// 0x416470: while it flies the flames stay with it and the rows of the board that it passes go
void TalkingHead::Follow(EffectManager* theEffects)
{
	if (theEffects == NULL)
		return;
	if (theEffects->Has(mTrail))
	{
		mTrail->mX = mPosX + 145.0;
		mTrail->mY = mPosY + 95.0;
		mApp->mBoard->F428de0(Ftol(mPosY + 75.0));
		return;
	}
	mTrail = NULL;
}

// 0x4164f0
void TalkingHead::FlyOut(EffectManager* theEffects)
{
	Place(0x1a4, 0x50);
	theEffects->Remove(mTrail);
	mTrail = NULL;
}

// 0x416520
void TalkingHead::GoToRest(bool theClimb)
{
	if (IsInHole())
		return;
	if (!IsSliding())
	{
		Place(-200, 100);
		mTilt = 0.0;
		mTiltSpeed = 0.004;
	}
	if (theClimb)
		StartSlide(true, 100);
}

// 0x416590
void TalkingHead::Arrive()
{
	Move(-200, -200);
	Place(400, 0x96);
	SetShadow(true);
	mTilt = 0.0;
	mTiltSpeed = 0.004;
	StartSlide(false, 100);
}

// 0x4165f0
void TalkingHead::StepAside()
{
	Place(-300, -180);
	SetShadow(true);
	mTiltSpeed = 0.004;
	mTilt = 0.0;
}

// ---------------------------------------------------------------------------------------------------------
// HoverTip

// 0x413aa0
HoverTip::HoverTip(RocketApp* theApp) :
	mApp(theApp),
	mRestCount(0),
	mLastX(-1),
	mLastY(-1),
	mRegion(-1),
	mShown(0)
{
}

// 0x413ad0
int HoverTip::Hit(int theX, int theY)
{
	int aRegion = 0;
	while (theX < sTipRegions[aRegion][0] || theY < sTipRegions[aRegion][1] || sTipRegions[aRegion][2] < theX ||
		sTipRegions[aRegion][3] < theY)
	{
		aRegion++;
		if (aRegion == 7)
			return -1;
	}
	if (aRegion == 1)
	{
		if (mApp->mMode != 1)
			return -1;
	}
	else if (aRegion == 2 && mApp->mMode == 2)
		return -1;
	return aRegion;
}

// 0x413b40
void HoverTip::Update(int theX, int theY)
{
	if (mApp->mNoHints)
	{
		mRegion = -1;
		return;
	}
	if (mRegion != -1)
	{
		if (Hit(theX, theY) != mRegion)
		{
			mLastX = theX;
			mLastY = theY;
			mRegion = -1;
			mRestCount = 0;
			return;
		}
		mLastX = theX;
		mLastY = theY;
		mRestCount++;
		mShown++;
		return;
	}

	int aLastX = mLastX;
	if (theX == aLastX && theY == mLastY)
		mRestCount++;
	else
	{
		mRegion = -1;
		mRestCount = 0;
	}
	if (0x50 < mRestCount && aLastX != -1 && mLastY != -1)
	{
		mRegion = Hit(aLastX, mLastY);
		mShown = 0;
	}
	mLastX = theX;
	mLastY = theY;
}

// 0x414000
void HoverTip::Draw(Graphics* g)
{
	if (!mApp->mNoHints && mRegion != -1)
		DrawTip(g, mRegion);
}

// 0x413c00: the picture of the tip fades in over 16 updates, then the eight lines of its text, centered on the picture
void HoverTip::DrawTip(Graphics* g, int theRegion)
{
	Image** anImages = mApp->mImages;
	bool aFading = mShown < 0x40;
	if (aFading)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(0xff, 0xff, 0xff, mShown << 2));
	}
	int aPlaceX = sTipPlaces[theRegion][0];
	int aPlaceY = sTipPlaces[theRegion][1];
	int aTextY = 0;
	switch (sTipFrames[theRegion])
	{
	case 0:
		g->DrawImage(mApp->mBubbleFlipH, aPlaceX, aPlaceY);
		break;
	case 1:
		g->DrawImage(anImages[IMAGE_POPUPBUBBLE], aPlaceX, aPlaceY);
		break;
	case 2:
		aTextY = 0x29;
		g->DrawImage(mApp->mBubbleFlipV, aPlaceX, aPlaceY);
		break;
	case 3:
	{
		// the bubble without its tail, closed with a line
		Image* aBubble = anImages[IMAGE_POPUPBUBBLE];
		int aCut = aBubble->GetHeight() - 0x2e;
		g->DrawImage(aBubble, aPlaceX, aPlaceY, Rect(0, 0, aBubble->GetWidth(), aCut));
		int anAlpha = mShown * 4;
		if (0xfe < anAlpha)
			anAlpha = 0xff;
		g->SetColor(Color(0x13, 0x8f, 0x68, anAlpha));
		g->FillRect(Rect(aPlaceX + 0x46, aBubble->GetHeight() - 0x30 + aPlaceY, aBubble->GetWidth() - 0x8c, 2));
		break;
	}
	default:
		break;
	}

	if (mShown < 0x40)
		g->SetColor(Color(0x1d, 0x60, 0x4e, mShown << 2));
	else
	{
		g->SetColorizeImages(false);
		g->SetColor(Color(0x1d, 0x60, 0x4e));
	}
	g->SetFont(mApp->mFonts[FONT_ARIAL10]);

	int aTable = mApp->mMode == 2 ? 1 : 0;
	int aLineY = 0;
	for (int aLine = 0; aLine < 8; aLine++)
	{
		const SexyString& aText = GT(sTipLines[aTable][theRegion][aLine]);
		int aWidth = g->GetFont()->StringWidth(aText);
		g->DrawString(aText, (aPlaceX + 0x5a - aWidth / 2) - 3, aPlaceY + aLineY + 0x23 + aTextY);
		aLineY += 0x18;
	}
	g->SetColorizeImages(false);
}
