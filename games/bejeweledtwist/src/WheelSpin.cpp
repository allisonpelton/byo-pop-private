#include "WheelSpin.h"

#include "Board.h"
#include "EffectsManager.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "Piece.h"
#include "Profile.h"
#include "Res.h"
#include "SexyMatrix.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "TwistButton.h"
#include "TwistUtil.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>

using namespace Sexy;

WheelSpin* Sexy::gWheelSpin = nullptr;

namespace {

const float kTwoPi = 6.2831855f;  // 0x850c58

// The x of an image placed by its offset on the 1920-wide art, centred in the window
int ArtX(int theId)
{
	return (gApp->mWidth - Ftol((double)gApp->mScale * 1920.0)) / 2 +
		Ftol((double)GetImageOffsetX(theId) * (double)gApp->mScale);
}

int ArtY(int theId)
{
	return Ftol((double)gApp->mScale * (double)GetImageOffsetY(theId));
}

}  // namespace

WheelSpin::WheelSpin(Board* theBoard, Piece* thePiece)
	: mBoard(theBoard), mPiece(thePiece)
{
	mRand.SRand(Rand85B970());
	mSiren = gApp->mSoundManager->GetSoundInstance(SOUND_SIREN);
	mSirenVolume.SetCurve(GTS(0x7f2ff4));
	mCache = nullptr;
	mSimulating = false;
	if (mSiren != nullptr) {
		mSiren->SetVolume(0);
		mSiren->Play(true, false);
	}
	mPaused = false;
	mUnknownC28 = nullptr;
	// (with Direct3D the manager would also draw through the board's distortion, its +0xa0)
	mFXManager = new EffectsManager();
	mGemScale = 1.0f;
	mPiece->mJustMade = 0;
	mPiece->mDevilGlow = 0;
	mPiece->mUnknown108 = 0;
	mPiece->mUnknown2F6 = true;
	mLeftArm = IMAGE_DEFUSER_LEFTARM;
	mRightArm = IMAGE_DEFUSER_RIGHTARM;
	mUnknownD2C = -mLeftArm->mWidth;
	mUnknownD30 = mBoard->mWidth;
	int aSpun = mBoard->mUnknown21DC;
	float aShare = 0.85f;
	if ((float)(aSpun < 0 ? 0 : aSpun) * 0.15 + 0.15 <= 0.85)
		aShare = (float)((float)(aSpun < 0 ? 0 : aSpun) * 0.15 + 0.15);
	mUnknown1B4 = aShare;
	aShare = (float)((float)(mBoard->mUnknown21DC + 1) * 0.15 + 0.15);
	if (0.85 < aShare)
		aShare = 0.85f;
	mUnknown1B8 = aShare;
	mLose = mBoard->mUnknown21E0 - 1 <= 0;
	// The original says "FTL" or "FTW" with OutputDebugString
	mSkulls = 0;
	mUnknown18C = (float)(((double)fabsf(RandSigned()) * 0.012 + 0.04) * (double)kTwoPi);
	// A skull for each wheel spun before, the bomb's colour in the other slots (each half of the wheel the same)
	for (int i = 0; i < 4; i++) {
		int aSlot;
		if (i < mBoard->mUnknown21DC) {
			aSlot = -1;
			mSkulls = i + 1;
		} else {
			aSlot = mPiece->mColor;
		}
		mSlots[i + 4] = aSlot;
		mSlots[i] = aSlot;
		mSlotColors[i] = aSlot < 0 ? Color(90, 90, 90) : gGemColors[mPiece->mColor];
		mSlotColors[i + 4] = mSlotColors[i];
	}
	// The slots by the skulls (both sets end as the bomb's colour; their colours stay)
	if (mSkulls < 3) {
		mSlots[mSkulls + 4] = -1;
		mSlots[mSkulls] = -1;
	} else {
		mSlots[(mSkulls + 4) & 7] = mPiece->mColor;
		mSlots[mSkulls] = mSlots[(mSkulls + 4) & 7];
	}
	mSlots[(mSkulls + 4) & 7] = mPiece->mColor;
	mSlots[mSkulls] = mSlots[(mSkulls + 4) & 7];
	mPickedSlot = 0;
	mOppositeSlot = 0;
	mUnknownD64 = false;
	mCounter = 0;
	mAngle = (float)((float)mSkulls * kTwoPi * 0.125 - (double)kTwoPi * 0.35);  // (set again below)
	mUnknown1D0 = 1.0f;
	mUnknown1CC = 0;
	mArms = 0;
	mUnknown254 = 0;
	mUnknown25C = 0;
	mUnknownD70 = 1.0f;
	mUnknown258 = 2.0f;
	for (int i = 0; i < 8; i++) {
		mSkullAdded[i] = false;
		mUnknownC54[i] = true;
		mUnknownC5C[i] = 0;
		mUnknownC7C[i] = 1.0f;
	}
	mAngle = (float)((float)-mBoard->mUnknown21DC * 3.1415927f * 0.125);
	Resize(0, 0, gApp->mWidth, gApp->mHeight);
	mButton = new TwistButton(0, IMAGE_DEFUSER_CLICKTOSTOP, this);
	mButton->mDeferDraw = true;
	mButton->mDeferPriority = 0x33;
	mButton->SetDisabled(true);
	AddWidget(mButton);
	mArrive.SetCurve(GTS(0x7f2fbc));
	mOpen.SetCurve(GTS(0x7f2fa0), &mArrive);
	mUnknownDF8.SetCurve(GTS(0x7f2f80), &mArrive);
	mGlow.SetCurve(GTS(0x7f2f60), &mArrive);
	mUnfold.SetCurve(GTS(0x7f2f44), &mArrive);
	mWheelAlpha.SetCurve(GTS(0x7f2f28), &mArrive);
	// (with Direct3D the alert ring's curve is 0x7f2f04)
	mAlert.SetCurve(GTS(0x7f2edc), &mArrive);
	mIconAlpha.SetCurve(GTS(0x7f2ebc), &mArrive);
	gWheelSpin = this;
}

WheelSpin::~WheelSpin()
{
	gWheelSpin = nullptr;
	gApp->mProfile->Save();
	if (mUnknownC28 != nullptr)
		mUnknownC28->Release();
	if (mSiren != nullptr)
		mSiren->Release();
	delete mFXManager;
	delete mCache;
	RemoveAllWidgets(true, false);
}

void WheelSpin::Resize(int theX, int theY, int theWidth, int theHeight)
{
	TwistWidget::Resize(theX, theY, theWidth, theHeight);
	mFXManager->Resize(0, 0, gApp->mWidth, gApp->mHeight);
	mCenterX = mWidth / 2;
	mCenterY = mHeight / 2 - 40;
}

void WheelSpin::Reset()
{
	mTickDelay = 0;
	mUnknownD78 = 1.0f;
	mTicks = 0;
	mState = 0;
	mUnknown164 = 0;
	mZoomIn.SetCurve(GTS(0x7f3014));
	mSpeed = 0;
	mUnknownD74 = 0;
	mUnknownD58 = 4.0f;
	mUnknown168 = 0;
	mUnknown16C = false;
	mCenterX = mWidth / 2;
	mUnknownD68 = 1.0f;
	mCenterY = 0;
	// The shift that brings the bomb's middle to the window's
	mPanX = (float)(mWidth / 2 - mPiece->mCol * gGemWidth - gGemWidth / 2 - mBoard->GetColX(0));
	float aLift = (float)((double)gApp->mScale * -18.0);
	mPanY = (float)(mHeight / 2 - mPiece->mRow * gGemHeight - gGemHeight / 2 - mBoard->GetRowY(0)) + aLift;
}

void WheelSpin::AddedToManager(WidgetManager* theWidgetManager)
{
	TwistWidget::AddedToManager(theWidgetManager);
	Reset();
	mPaused = true;
	mBoard->MouseMove(-1, -1);
	mBoard->Pause(false);
}

void WheelSpin::RemovedFromManager(WidgetManager* theWidgetManager)
{
	TwistWidget::RemovedFromManager(theWidgetManager);
	mBoard->mUnknown21E0--;
	mBoard->mUnknown21DC++;
	gApp->mUnknownC74[52]++;  // app +0xd44: wheels spun
	if (mPaused) {
		mBoard->Unpause(false);
		mPaused = true;  // (sic: the original sets it again, not clears it)
	}
	mClicked = false;
}

void WheelSpin::MouseMove(int x, int y)
{
	mMouseX = x;
	mMouseY = y;
}

void WheelSpin::StartSpin()
{
	mState = 6;
	mSpeed = kTwoPi * 0.01f;
	if (mButton != nullptr)
		mButton->SetDisabled(true);
}

void WheelSpin::ButtonDepress(int theId)
{
	if (mState != 5)
		return;
	// Spin ahead with ever later brakes until the wheel stops on a skull when the game is lost, on a gem when not
	mSavedAngle = mAngle;
	mSimulating = true;
	int aTry = 0;
	do {
		int aRand = (int)mRand.Next();
		mBrake = (float)(((double)(float)(aRand % 10000 - 5000) / 5000.0) * 0.05 + 1.0);
		mCoast = (float)((double)(float)aTry * 0.01);
		mAngle = mSavedAngle;
		StartSpin();
		while (mState < 9)
			UpdateState();
		if (mLose == (mState == 13)) {
			mCoast = (float)((double)(float)aTry * 0.01);
			break;
		}
		aTry++;
	} while (aTry < 150);
	mAngle = mSavedAngle;
	mSimulating = false;
	StartSpin();
}

void WheelSpin::Update()
{
	TwistWidget::Update();
	if (mState < 12) {
		float aRise = mUnknown164 + 0.02f;
		if (1.0f < aRise)
			aRise = 1.0f;
		mUnknown164 = aRise;
		if (mPiece != nullptr && 1.0f <= mPiece->mDrawScale) {
			mPiece->mUnknown29C *= 0.25f;
			mPiece->Update();
		}
	}
	mArrive.IncInVal();
	mPiece->mUnknown29C = (float)mGlow.GetOutVal();
	mPiece->Update();
	if (7 < mState && mState != 15 && mResult == 13) {
		// Sparks fly from the bomb that is about to go off
		double aGlow = mGlow.GetOutVal();
		for (int aCount = Ftol((double)RandFloat() * (aGlow * 16.0)); 0 < aCount; aCount--) {
			int aDir = Rand() % 4;
			float aSpeed = (float)(mGlow.GetOutVal() * 3.0 + 1.0);
			Effect* aSpark = mPiece->AddSpark(true, aSpeed,
				(float)((double)aDir * 3.1415927410125732 * 0.5 + 0.7853981852531433));
			mFXManager->AddEffect(aSpark);
			if (mBoard->mUnknown268C < 1.2f && RandFloat() < 0.3f) {
				Effect* anEffect = mFXManager->AllocEffect(0);
				anEffect->mImage = IMAGE_ANGRYPARTICLE;
				anEffect->mIsAdditive = true;
				anEffect->mX = aSpark->mX;
				anEffect->mY = aSpark->mY;
				anEffect->mDX = (float)(((double)RandSigned() * 0.10000000149011612 + 0.5) * (double)aSpark->mDX);
				anEffect->mDY = (float)(((double)RandSigned() * 0.10000000149011612 + 0.5) * (double)aSpark->mDY);
				anEffect->mGravity = 0;
				anEffect->mUnknown9C = (float)((double)RandSigned() * (double)kTwoPi);
				anEffect->mUnknownA0 = (float)((double)RandSigned() * (double)kTwoPi);
				anEffect->mUnknownA4 = (float)(((double)fabsf(RandSigned()) * 3.0 + 1.0) * 0.04500000178813934);
				anEffect->mUnknownA8 = (float)(((double)fabsf(RandSigned()) * 3.0 + 1.0) * 0.04500000178813934);
				anEffect->mAlpha = 1.0f;
				double aFade = (double)fabsf(RandSigned());
				anEffect->mDAlpha = (float)((aFade + aFade + 4.0) * -0.003);
				anEffect->mScale = 0.1f;
				anEffect->mDScale = 0.02f;
				mFXManager->AddEffect(anEffect);
			}
		}
	}
	mFXManager->Update();
	UpdateState();
	mSirenVolume.IncInVal();
	if (mSiren != nullptr)
		mSiren->SetVolume(mSirenVolume.GetOutVal());
	int anOffY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(IMAGE_DEFUSER_CLICKTOSTOP_ID));
	double aDrop = mUnknownDF8.GetOutVal();
	int aDropBy = Ftol((double)gApp->mScale * 500.0);
	int anArtX = Ftol((double)gApp->mScale * 1920.0);
	int anOffX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(IMAGE_DEFUSER_CLICKTOSTOP_ID));
	mButton->Resize((gApp->mWidth - anArtX) / 2 + anOffX, Ftol((double)aDropBy * aDrop + (double)anOffY),
		IMAGE_DEFUSER_CLICKTOSTOP->GetCelWidth(), IMAGE_DEFUSER_CLICKTOSTOP->GetCelHeight());
	mPiece->mUnknown2F6 = false;
	MarkDirty();
}


void WheelSpin::UpdateState()
{
	bool aSparkle = false;
	if (mSimulating)
		mCounter = 0;
	for (int i = 0; i < 8; i++) {
		float aFall = mUnknownC5C[i] - 0.04f;
		if (mUnknownC5C[i] - 0.04f < 0.0f)
			aFall = 0.0f;
		mUnknownC5C[i] = aFall;
	}
	if (mState == 5 || mState == 6) {
		double aRise = (double)mUnknownD74 + 0.019999999552965164;
		mUnknownD74 = (float)(1.0 < aRise ? 1.0 : aRise);
	}
	if (mState < 7 && 4 < mState) {
		double aRise = (double)mUnknown25C + 0.019999999552965164;
		mUnknown25C = (float)(1.0 < aRise ? 1.0 : aRise);
	}
	float aSegment = (float)Ftol((double)mAngle / (0.125 * (double)kTwoPi));
	switch (mState) {
	case 0: {
		// The board zooms in on the bomb
		if (!mZoomIn.IncInVal())
			mState++;
		float aZoom = (float)sin(mZoomIn.GetOutVal() * 3.141592653589793 * 0.5);
		mBoard->mUnknown268C = (float)((double)aZoom * 0.5 + 1.0);
		mBoard->mShakeX = mPanX * aZoom;
		mBoard->mShakeY = mPanY * aZoom;
		break;
	}
	case 1:
		if (mOpen.GetOutVal() == 1.0) {
			mState = 2;
			gApp->PlaySample(SOUND_X_SPINNERAPPEARS1);
		}
		break;
	case 2: {
		double aFall = (double)mUnknown258 - 0.014999999664723873;
		float aNew = (float)(aFall < 1.0 ? 1.0 : aFall);
		mUnknown258 = aNew;
		float aRise = (float)((double)mUnknown254 + 0.019999999552965164);
		mUnknown254 = aRise;
		if (1.0f <= aRise)
			mUnknown254 = 1.0f;
		if (aNew == 1.0f) {
			int aSkulls = mSkulls;
			if (aSkulls < 3) {
				// This spin's skull goes on the wheel
				mState = 3;
				float anAngle = (float)((double)aSkulls * 0.7853981852531433 + (double)mAngle);
				mUnknownD50 = anAngle;
				mUnknownD4C = (float)((double)anAngle - 0.7853981852531433);
				mSlots[aSkulls + 4] = -1;
				mSlots[mSkulls] = -1;
				mSkullAdded[mSkulls + 4] = true;
				mSkullAdded[mSkulls] = true;
				mSkullAdd.SetCurve(GTS(0x7f3210));
			} else {
				mState = 4;
				mCounter = 100;
			}
		}
		break;
	}
	case 3:
		if (mSkullAdd.CheckInThreshold(0.25))
			gApp->PlaySample(SOUND_SKULL_ADDED);
		if (!mSkullAdd.IncInVal())
			mState++;
		break;
	case 4:
		mCounter--;
		if (0 < mCounter)
			break;
		{
			float aClose = (float)((double)mArms + 0.019999999552965164);
			mArms = aClose;
			if (1.0f <= aClose) {
				mArms = 1.0f;
				mState = 5;
			}
		}
		// fall through
	case 5: {
		// It spins, faster, until the button stops it (at once with -autoplay or on the board's +0x2629)
		mUnknownD68 = fabsf((float)sin((double)(mUpdateCnt % 300) / 300.0 * (double)kTwoPi));
		float aSpeed = (float)((double)kTwoPi * 8.5e-05 + (double)mSpeed);
		if ((double)kTwoPi * 0.015 < (double)aSpeed)
			aSpeed = (float)((double)kTwoPi * 0.015);
		mSpeed = aSpeed;
		float aReady = (float)((double)kTwoPi * 0.012);
		if (aReady <= aSpeed) {
			mButton->SetDisabled(false);
			if (!mSimulating && (gApp->mAutoPlayMode >= 0 || mBoard->mUnknown2629))
				ButtonDepress(0);
		}
		float anAngle = mSpeed + mAngle;
		mAngle = anAngle;
		if (kTwoPi <= anAngle)
			mAngle = anAngle - kTwoPi;
		break;
	}
	case 6: {
		// It slows, ticking, then stops on the slot nearest the pointer
		if (!mSimulating && mSpeed < 1.08f) {
			mTickDelay = (float)((double)mTickDelay - 0.01);
			if (mTickDelay <= 0.0f) {
				float aSide;
				float aPitch;
				if ((mTicks & 1) == 0) {
					aSide = -0.25f;
					aPitch = 0.0f;
				} else {
					aSide = 0.25f;
					aPitch = -4.0f;
				}
				int aMid = mWidth / 2;
				gApp->PlaySoundAt(SOUND_TEMP_TIMPANI, Ftol((double)mWidth * (double)aSide * 0.5 + (double)aMid),
					aPitch, 0.0f);
				float aDelay = (float)((double)mSpeed * 6.0);
				mTicks++;
				if (aDelay < 0.08f)
					aDelay = 0.08f;
				mTickDelay = aDelay;
			}
		}
		float anAngle = mSpeed + mAngle;
		mAngle = anAngle;
		if (kTwoPi <= anAngle)
			mAngle = anAngle - kTwoPi;
		mUnknownD64 = true;
		if (mCoast <= 0.0f) {
			float aRate = mSpeed <= 0.01f ? 8e-05f : 0.0003f;
			mSpeed = mSpeed - mBrake * aRate;
		} else {
			mCoast = (float)((double)mCoast - 0.01);
		}
		if (mSpeed <= 0.0f) {
			int aPicked = 0;
			float aStep = kTwoPi * 0.125f;
			float aBest = kTwoPi;
			float aBestAngle = 0;
			float aSlotAngle = mAngle;
			for (int i = 0; i < 8; i++) {
				float aSigned = aSlotAngle;
				if (3.1415927f < aSlotAngle)
					aSigned = aSlotAngle - kTwoPi;
				float aDist = fabsf(0.0f - aSigned);
				if (aDist < aBest) {
					aPicked = (i + 2) & 7;
					aBestAngle = aSlotAngle;
					aBest = aDist;
				}
				aSlotAngle = aSlotAngle + aStep;
				if (kTwoPi <= aSlotAngle)
					aSlotAngle = aSlotAngle - kTwoPi;
			}
			mPickedSlot = aPicked;
			mOppositeSlot = (aPicked + 4) & 7;
			mTargetAngle = aBestAngle;
			float aTurn = 0.0f - aBestAngle;
			if (aTurn <= 3.1415927f) {
				if (aTurn < -3.1415927f)
					aTurn = aTurn + kTwoPi;
			} else {
				aTurn = aTurn - kTwoPi;
			}
			mTargetAngle = mAngle + aTurn;
			mSpeed = 0;
			mState++;
		}
		break;
	}
	case 7:
		// It turns back to the slot's middle
		if (mAngle <= mTargetAngle) {
			mSpeed = (float)((double)mSpeed - 0.0025);
			float anAngle = mAngle - mSpeed;
			mAngle = anAngle;
			if (mTargetAngle <= anAngle) {
				mState = 8;
				mAngle = mTargetAngle;
			}
		} else {
			mSpeed = (float)((double)mSpeed + 0.0025);
			float anAngle = mAngle - mSpeed;
			mAngle = anAngle;
			if (anAngle <= mTargetAngle) {
				mState = 8;
				mAngle = mTargetAngle;
			}
		}
		break;
	case 8:
		// (+0xc28 is never set)
		if (mSlots[mPickedSlot] < 0) {
			mZoomIn.SetCurve(GTS(0x7f31d4));
			mState = 13;
			mResult = 13;
			if (mSimulating)
				goto done;
			mSirenVolume.SetCurve(GTS(0x7f31b0));
			mSlots[mPickedSlot] = -2;
			mSlots[mOppositeSlot] = -2;
			gApp->PlaySample(SOUND_X_LOSE);
			mUnknownD5C = 1.0f;
			mCounter = 200;
			mUnknownD60 = 1.0f;
		} else {
			mZoomIn.SetCurve(GTS(0x7f3190));
			mState = 9;
			mResult = 9;
			if (mSimulating)
				goto done;
			mSirenVolume.SetCurve(GTS(0x7f3170));
			gApp->PlaySample(SOUND_X_WIN);
			mPiece->mBombCount = 20;
			mGemGap = (float)Ftol((double)gApp->mScale * 260.0);
			mWobbleAngle = 0;
			mWobble = 0.5f;
			mSlots[mPickedSlot] = -2;
			mSlots[mOppositeSlot] = -2;
			mCounter = 100;
		}
		break;
	case 9: {
		// Won: the bomb's halves close in and it wobbles
		aSparkle = true;
		double aFade = (double)mUnknown254 - 0.019999999552965164;
		float aNew = (float)(aFade < 0.0 ? 0.0 : aFade);
		mUnknown254 = aNew;
		aFade = (double)mUnknown25C - 0.019999999552965164;
		mUnknown25C = (float)(aFade < 0.0 ? 0.0 : aFade);
		mUnknownD78 = aNew;
		mUnknownD74 = aNew;
		if (aNew == 0.0f) {
			int aWidth = Ftol((double)mBoard->mUnknown268C * (double)gGemWidth);
			float anOld = mGemGap;
			float aGap = mGemGap - 2.0f;
			mGemGap = aGap;
			float aMin = (float)aWidth;
			if (aGap <= aMin) {
				aSparkle = false;
				if (aMin < anOld)
					gApp->PlaySample(SOUND_X_WIN2);
				mGemGap = aMin;
				mWobbleAngle = (float)((double)kTwoPi * 0.04 + (double)mWobbleAngle);
				float aWobble = (float)((double)mWobble - 0.0035);
				mWobble = aWobble;
				if (aWobble <= 0.0f) {
					mWobble = 0;
					mCounter = 50;
					mState = 10;
					gApp->PlaySample(SOUND_COMBO_2);
				}
				mGemScale = (float)sin((double)mWobbleAngle) * mWobble + 1.0f;
			}
		}
		break;
	}
	case 10:
		mGemScale = (float)((double)mGemScale * 0.95);
		mCounter--;
		if (mCounter < 1) {
			mPiece->mBlasted = true;
			mState = 11;
			mCounter = 50;
		} else if (mCounter == 49) {
			gApp->PlaySample(SOUND_VOICE_BOMBDISARMED);
		}
		break;
	case 11:
		// Everything goes
		mCounter--;
		if (mCounter < 1) {
			mArrive.SetCurve(GTS(0x7f30c8));
			mOpen.SetCurve(GTS(0x7f30ac), &mArrive);
			mUnknownDF8.SetCurve(GTS(0x7f3090), &mArrive);
			mAlert.SetCurve(GTS(0x7f3074), &mArrive);
			mUnfold.SetCurve(GTS(0x7f305c), &mArrive);
			mWheelAlpha.SetCurve(GTS(0x7f3040), &mArrive);
			gApp->PlaySoundEx(SOUND_SPINNER_UNLOCK, 0, 0, mLose ? 1.0f : 0.2f, true, true);
			mState++;
		}
		break;
	case 12: {
		// The board zooms out again
		mBoard->mShakeX = (float)((double)mBoard->mShakeX * 0.95);
		if (fabsf(mBoard->mShakeX) < 2.0f)
			mBoard->mShakeX = 0;
		mBoard->mShakeY = (float)((double)mBoard->mShakeY * 0.95);
		if (fabsf(mBoard->mShakeY) < 2.0f)
			mBoard->mShakeY = 0;
		float aClose = mArms - 0.02f;
		if (aClose < 0.0f)
			aClose = 0.0f;
		mArms = aClose;
		bool aDone = !mZoomIn.IncInVal();
		float aZoom = (float)mZoomIn.GetOutVal();
		mBoard->mUnknown268C = (float)((double)aZoom * 0.5 + 1.0);
		float aPan = 0.0f;
		if (0.0f < aZoom)
			aPan = aZoom * aZoom;
		mBoard->mShakeX = mPanX * aPan;
		mBoard->mShakeY = mPanY * aPan;
		if (aDone) {
			mBoard->mUnknown26CC = mPiece;
			if (mResult == 13) {
				mBoard->EndGame(true);
				mCounter = 50;
				mState = 15;
				if (mPaused) {
					mBoard->Unpause(false);
					mPaused = true;  // (sic)
				}
			} else {
				mCounter = 0;
				mState = 14;
			}
		}
		break;
	}
	case 13:
		// Lost: the picked skull grows, the bomb glows angry
		mSkullScale.SetCurve(GTS(0x7f3144));
		mSkullColor.SetCurve(GTS(0x7f3128), &mSkullScale);
		mGlow.SetCurve(GTS(0x7f310c), &mSkullScale);
		mBoard->mUnknown2898.SetCurve(GTS(0x7f30e8));
		mState = 11;
		mCounter = 150;
		break;
	case 14: {
		// Every other counter on the board counts up, one every 20 updates
		int aCount = gBoard->mCols * gBoard->mRows;
		for (int i = 0; i < aCount; i++) {
			Piece* aPiece = gBoard->mGrid[i];
			if (aPiece == nullptr)
				continue;
			aPiece->mCurve1A0.IncInVal();
			if (mCounter == 0) {
				aPiece->mJustMade = 0;
				aPiece->mDevilGlow = 0;
			} else if (0 < aPiece->mJustMade) {
				float aGlow = aPiece->mDevilGlow - 0.02f;
				aPiece->mDevilGlow = aGlow;
				if (aGlow <= 0.0f) {
					aPiece->mJustMade--;
					aPiece->mDevilGlow = aPiece->mJustMade != 0 ? 1.0f : 0.0f;
				}
			}
		}
		mCounter++;
		if (mCounter < 151) {
			if (mCounter % 20 != 0 || mCounter < 26 || 144 < mCounter)
				break;
			bool anAny = false;
			for (int i = 0; i < aCount; i++) {
				Piece* aPiece = gBoard->mGrid[i];
				if (aPiece != nullptr && aPiece != mPiece &&
					(aPiece->IsFlagSet(Piece::FLAG_BOMB) || aPiece->IsFlagSet(Piece::FLAG_DEVIL))) {
					aPiece->mBombCount++;
					anAny = true;
					aPiece->MakeCountImage();
					aPiece->mDevilGlow = 1.0f;
					aPiece->mJustMade = 1;
					aPiece->mCurve1A0.SetCurve(GTS(0x7dd424));
				}
			}
			if (anAny) {
				gApp->PlaySoundEx(SOUND_BLITZ_COUNTUP, 0, (float)(mCounter / 15) * 1.5f, 0, true, true);
				break;
			}
		}
		mClicked = true;
		break;
	}
	case 15:
		mCounter--;
		if (mCounter < 0)
			mClicked = true;
		break;
	}
	if (!mSimulating) {
		// A click as each slot passes the pointer
		if ((float)Ftol((double)mAngle / ((double)kTwoPi * 0.125)) != aSegment)
			gApp->PlaySoundEx(mState == 5 ? SOUND_X_WHEELSPIN : SOUND_X_WHEELSPIN2, 0, 0, 0.5f, true, true);
		if (1 < mState) {
			float aClose = (float)sin((double)mArms * 3.1415927f * 0.5);
			float aMid = (float)(mWidth / 2);
			float aLeft = (float)mLeftArm->mWidth;
			mUnknownD2C = Ftol((((double)aMid - (double)aLeft * 0.12999999523162842) - (double)aLeft) -
				(double)aLeft * (1.0 - (double)aClose));
			float aRight = (float)mRightArm->mWidth;
			mUnknownD30 = Ftol(0.12999999523162842 * (double)aRight + (double)aMid +
				(double)aRight * (1.0 - (double)aClose));
		}
		mUnknownD24 = Ftol((double)mLeftArm->mWidth * 0.7350000143051147 + (double)mUnknownD2C);
		mUnknownD28 = Ftol((double)mRightArm->mWidth * 0.2680000066757202 + (double)mUnknownD30);
	}
done:
	if (aSparkle && (mUpdateCnt % 2) == 0) {
		// Sparkles between the bomb's halves
		float aX = (float)(mWidth / 2) - mGemGap;
		for (int i = 0; i < 3; i++) {
			Effect* anEffect = mFXManager->AllocEffect(17);
			anEffect->mAngle = (float)((double)RandSigned() * 3.1415927410125732);
			anEffect->mDAngle = (float)((double)RandSigned() * 3.1415927410125732 * 0.009999999776482582);
			float aRadius = (float)((double)((float)gGemWidth * fabsf(RandSigned())) * 0.55);
			anEffect->mX = (float)((double)cosf(anEffect->mAngle) * (double)aRadius + (double)aX);
			float aSin = sinf(anEffect->mAngle);
			anEffect->mIsAdditive = true;
			anEffect->mOverlay = true;
			anEffect->mY = aSin * aRadius + (float)(mHeight / 2);
			anEffect->mScale = 0.75f;
			anEffect->mDScale = 0.03f;
			mFXManager->AddEffect(anEffect);
			aX = aX + mGemGap;
		}
	}
	mSkullScale.IncInVal();
}

void WheelSpin::Draw(Graphics* g)
{
	TwistWidget::Draw(g);
	mWidgetManager->FlushDeferredOverlayWidgets(0x7fffffff);
	DeferOverlay(0x32);
}

void WheelSpin::DrawHole(Graphics* g, int theX, int theY, float theDark, float theSize)
{
	// (the app's, 0x7607d9; with Direct3D it draws the hole as a textured quad)
	Image* aHole = IMAGE_TRANSPARENT_HOLE;
	int aWidth = Ftol((double)aHole->mWidth / (double)theSize * 6.0);
	int aHeight = Ftol((double)aHole->mHeight / (double)theSize * 6.0);
	int aX = theX - aWidth / 2;
	int aY = theY - aHeight / 2;
	g->SetColorizeImages(true);
	int anAlpha = Ftol((double)theDark * 255.0);
	g->SetColor(Color(255, 255, 255, anAlpha));
	g->DrawImage(aHole, Rect(aX, aY, aWidth, aHeight), Rect(0, 0, aHole->mWidth, aHole->mHeight));
	g->SetColor(Color(0, 0, 0, anAlpha));
	g->FillRect(0, 0, gApp->mWidth, aY);
	g->FillRect(0, aY, aX, aHeight);
	g->FillRect(aWidth + aX, aY, gApp->mWidth - aWidth - aX, aHeight);
	g->FillRect(0, aHeight + aY, gApp->mWidth, gApp->mHeight - aHeight - aY);
}

void WheelSpin::MakeCache()
{
	// The screen as it is (the board, its overlays and with them this wheel) with the hole, the alert ring and the
	// arms, so the board need not be drawn while the wheel stands. The original's DDImage has no alpha.
	MemoryImage* aCache = new MemoryImage(gApp);
	mCache = aCache;
	aCache->SetImageMode(false, false);
	aCache->Create(mWidth, mHeight);
	Graphics aG(aCache);
	aG.SetFastStretch(true);
	aG.SetLinearBlend(false);
	aG.PushState();
	// (the music visualiser, app +0xc6c, would draw first; the port has none)
	ModalFlags aFlags;
	mWidgetManager->InitModalFlags(&aFlags);
	gApp->mBoard->DrawAll(&aFlags, &aG);
	Graphics* aCurG = mWidgetManager->mCurG;
	mWidgetManager->mCurG = &aG;
	mWidgetManager->FlushDeferredOverlayWidgets(0x7fffffff);
	mWidgetManager->mCurG = aCurG;
	aG.PopState();
	double anOpen = mOpen.GetOutVal();
	int aMidY = mHeight / 2;
	float aSize = (float)(1.0 / ((1.0 - anOpen) * 3.5 + 0.35));
	float aMidX = (float)(mWidth / 2);
	float aDark = (float)((double)mUnknown1D0 * 200.0 / 255.0);
	DrawHole(&aG, Ftol((double)aMidX), Ftol((double)(float)aMidY), aDark, aSize);
	aG.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	aG.SetColor(Color(255, 255, 255, Ftol(mAlert.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
	int aRingY = Ftol((double)GetImageOffsetY(IMAGE_DEFUSER_ALERTRING_ID) * (double)gApp->mScale) +
		Ftol((double)gApp->mScale * -6.0);
	aG.DrawImage(IMAGE_DEFUSER_ALERTRING, ArtX(IMAGE_DEFUSER_ALERTRING_ID) - Ftol((double)gApp->mScale * 49.0), aRingY,
		IMAGE_DEFUSER_ALERTRING->mWidth, IMAGE_DEFUSER_ALERTRING->mHeight);
	aG.PushState();
	int aTopY = ArtY(IMAGE_DEFUSER_DANGERRING_TOP_ID);
	aG.DrawImage(IMAGE_DEFUSER_DANGERRING_TOP, ArtX(IMAGE_DEFUSER_DANGERRING_TOP_ID), aTopY);
	int aCenter = Ftol((double)gApp->mScale * 536.0);
	int aBottomY = Ftol((double)gApp->mScale * 4.0) +
		Ftol((double)GetImageOffsetY(IMAGE_DEFUSER_DANGERRING_TOP_ID) * (double)gApp->mScale);
	aG.DrawImageRotated(IMAGE_DEFUSER_DANGERRING_TOP, ArtX(IMAGE_DEFUSER_DANGERRING_TOP_ID), aBottomY,
		3.1415927410125732, aCenter, aCenter, nullptr);
	aG.PopState();
	aG.SetDrawMode(Graphics::DRAWMODE_NORMAL);
	// The arms but for the wheel's middle 400 pixels, which DrawOverlay draws over the cache
	Graphics aLeftG(aG);
	aLeftG.ClipRect(0, 0, mWidth / 2 - 200, mHeight);
	int anY = ArtY(IMAGE_DEFUSER_LEFTARM_ID);
	int anOffX = Ftol((double)GetImageOffsetX(IMAGE_DEFUSER_LEFTARM_ID) * (double)gApp->mScale);
	int anArtW = Ftol((double)gApp->mScale * 1920.0);
	double anArrive = mArrive.GetOutVal();
	int anArtLeft = (gApp->mWidth - anArtW) / 2;
	int aSlide = Ftol((double)gApp->mScale * 1500.0);
	aLeftG.DrawImage(IMAGE_DEFUSER_LEFTARM, Ftol((double)anArtLeft - (double)aSlide * anArrive + (double)anOffX), anY);
	Graphics aRightG(aG);
	aRightG.ClipRect(mWidth / 2 + 200, 0, mWidth / 2 - 200, mHeight);
	anY = ArtY(IMAGE_DEFUSER_RIGHTARM_ID);
	anOffX = Ftol((double)GetImageOffsetX(IMAGE_DEFUSER_RIGHTARM_ID) * (double)gApp->mScale);
	anArtW = Ftol((double)gApp->mScale * 1920.0);
	anArrive = mArrive.GetOutVal();
	anArtLeft = (gApp->mWidth - anArtW) / 2;
	aSlide = Ftol((double)gApp->mScale * 1500.0);
	aRightG.DrawImage(IMAGE_DEFUSER_RIGHTARM, Ftol((double)anArtLeft + (double)aSlide * anArrive + (double)anOffX),
		anY);
	int aBottomArmY = Ftol((double)Ftol((double)gApp->mScale * 500.0) * mUnknownDF8.GetOutVal() +
		(double)ArtY(IMAGE_DEFUSER_BOTTOMARM_ID));
	aG.DrawImage(IMAGE_DEFUSER_BOTTOMARM, ArtX(IMAGE_DEFUSER_BOTTOMARM_ID), aBottomArmY);
	// (DirectDraw's lock and unlock of the surface; and the music visualiser is hidden too)
	gApp->mBoard->SetVisible(false);
	gApp->ClearUpdateBacklog(false);
}

void WheelSpin::DrawOverlay(Graphics* g, int thePriority)
{
	bool aMadeCache = false;
	if (mArrive.GetOutVal() == 0.0 && mOpen.GetOutVal() == 1.0) {
		// The wheel stands: the screen behind it is kept
		if (mCache == nullptr) {
			MakeCache();
			aMadeCache = true;
		}
	} else if (mCache != nullptr) {
		delete mCache;
		mCache = nullptr;
		mBoard->SetVisible(true);
		// (and the music visualiser's)
	}
	if (mCache != nullptr)
		g->DrawImage(mCache, 0, 0);
	g->SetColorizeImages(true);
	g->SetColor(Color::White);
	double anOpen = mOpen.GetOutVal();
	int aMidY = mHeight / 2;
	float aSize = (float)(1.0 / ((1.0 - anOpen) * 3.5 + 0.35));
	float aMidX = (float)(mWidth / 2);
	float aMidYF = (float)aMidY;
	if (mCache == nullptr && mOpen.GetOutVal() != 0.0) {
		float aDark = (float)((double)mUnknown1D0 * 200.0 / 255.0);
		DrawHole(g, Ftol((double)aMidX), Ftol((double)aMidYF), aDark, aSize);
	}
	double anAlpha = mWheelAlpha.GetOutVal() * 5.0;
	if (!(1.0 < anAlpha))
		anAlpha = mWheelAlpha.GetOutVal() * 5.0;
	else
		anAlpha = 1.0;
	float aWheelAlpha = (float)anAlpha;
	float aScale = gApp->mScale;
	Transform aTransform;
	// The wheel: a glass slice for each slot with the bomb's gem, or a skull's slice with its icon
	for (int i = 0; i < 8; i++) {
		Color aGlassColor(0, 0, 0, 0);
		Color aSkullColor(0, 0, 0, 0);
		float anAngle = (float)(((double)i - 0.5) * -6.2831854820251465 * 0.125 - (double)mAngle);
		double anUnfold = mUnfold.GetOutVal();
		if (anUnfold < 1.0) {
			float aFrom = i / 4 == 0 ? -1.19f : -4.3f;
			double aPrev = mUnfold.GetOutVal();
			anAngle = (float)(mUnfold.GetOutVal() * (double)anAngle + (1.0 - aPrev) * (double)aFrom);
		}
		if (mSlots[i] == -1) {
			if (mSkullAdded[i]) {
				double aShown = mSkullAdd.GetOutVal() * (double)aWheelAlpha;
				if (!(1.0 < aShown))
					aShown = mSkullAdd.GetOutVal() * (double)aWheelAlpha;
				else
					aShown = 1.0;
				aSkullColor = Color(255, 255, 255, Ftol(aShown * 255.0));
				double aGlass = (1.0 - mSkullAdd.GetOutVal()) * (double)aWheelAlpha;
				if (0.0 <= aGlass)
					aGlass = (1.0 - mSkullAdd.GetOutVal()) * (double)aWheelAlpha;
				else
					aGlass = 0.0;
				aGlassColor = Color(255, 255, 255, Ftol(aGlass * 255.0));
			} else {
				float aShown = 1.0f < aWheelAlpha ? 1.0f : aWheelAlpha;
				aSkullColor = Color(255, 255, 255, Ftol((double)aShown * 255.0));
			}
		} else {
			float aShown = 1.0f < aWheelAlpha ? 1.0f : aWheelAlpha;
			aGlassColor = Color(255, 255, 255, Ftol((double)aShown * 255.0));
		}
		if (aGlassColor.mAlpha != 0) {
			g->SetColor(aGlassColor);
			aTransform.Reset();
			float aDY = (float)((double)aScale * -233.0);
			aTransform.Translate((float)Ftol((double)aScale * -122.0), aDY);
			double aSliceAngle = (double)anAngle - 0.7853981852531433;
			aTransform.RotateRad((float)aSliceAngle);
			aTransform.Translate((float)(mWidth / 2), (float)((double)gApp->mScale * 570.0));
			g->DrawImageTransform(IMAGE_DEFUSER_GLASSPIE, aTransform, 0, 0);
			if (aGlassColor.mAlpha != 0) {
				g->SetColor(aGlassColor);
				aTransform.Reset();
				float aGemScale = (float)((double)aSize * 0.20000000298023224);
				aTransform.Scale(aGemScale, aGemScale);
				aTransform.Translate((float)Ftol((double)gApp->mScale * 0.0), (float)((double)gApp->mScale * -250.0));
				aTransform.RotateRad((float)(aSliceAngle + 0.3799999952316284));
				aTransform.Translate((float)(mWidth / 2), (float)((double)gApp->mScale * 570.0));
				g->DrawImageTransform(IMAGE_GEMS, aTransform, IMAGE_GEMS->GetCelRect(mPiece->mColor), 0, 0);
			}
		}
		if (aSkullColor.mAlpha != 0) {
			g->SetColor(aSkullColor);
			for (int aPass = 0; aPass < 2; aPass++) {
				aTransform.Reset();
				float aDY = (float)((double)gApp->mScale * -233.0);
				aTransform.Translate((float)Ftol((double)gApp->mScale * 117.0), aDY);
				aTransform.RotateRad(anAngle);
				aTransform.Translate((float)(mWidth / 2), (float)((double)gApp->mScale * 570.0));
				g->DrawImageTransform(IMAGE_DEFUSER_SKULLPIE, aTransform, 0, 0);
				// A new skull flashes while its curve is past 1
				if (!(1.0 < mSkullAdd.GetOutVal()) || !mSkullAdded[i])
					break;
				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				g->SetColor(Color(255, 255, 255, Ftol((mSkullAdd.GetOutVal() - 1.0) * 255.0)));
			}
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			if (aSkullColor.mAlpha != 0) {
				g->SetColor(aSkullColor);
				aTransform.Reset();
				float aDY = (float)((double)gApp->mScale * -220.0);
				aTransform.Translate((float)Ftol((double)gApp->mScale * 80.0), aDY);
				aTransform.RotateRad((float)((double)anAngle + 0.0));
				aTransform.Translate((float)(mWidth / 2), (float)((double)gApp->mScale * 570.0));
				g->DrawImageTransform(IMAGE_DEFUSER_SKULLICON_SMALL, aTransform, 0, 0);
			}
		}
	}
	if (mCache == nullptr && mAlert.GetOutVal() != 0.0) {
		// The alert ring and the danger ring (its bottom half the top turned round)
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color(255, 255, 255, Ftol(mAlert.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
		int aRingY = Ftol((double)gApp->mScale * -6.0) +
			Ftol((double)GetImageOffsetY(IMAGE_DEFUSER_ALERTRING_ID) * (double)gApp->mScale);
		g->DrawImage(IMAGE_DEFUSER_ALERTRING, ArtX(IMAGE_DEFUSER_ALERTRING_ID) - Ftol((double)gApp->mScale * 49.0),
			aRingY, IMAGE_DEFUSER_ALERTRING->mWidth, IMAGE_DEFUSER_ALERTRING->mHeight);
		g->PushState();
		g->DrawImage(IMAGE_DEFUSER_DANGERRING_TOP, ArtX(IMAGE_DEFUSER_DANGERRING_TOP_ID),
			ArtY(IMAGE_DEFUSER_DANGERRING_TOP_ID));
		int aCenter = Ftol((double)gApp->mScale * 536.0);
		int aBottomY = Ftol((double)GetImageOffsetY(IMAGE_DEFUSER_DANGERRING_TOP_ID) * (double)gApp->mScale) +
			Ftol((double)gApp->mScale * 4.0);
		g->DrawImageRotated(IMAGE_DEFUSER_DANGERRING_TOP, ArtX(IMAGE_DEFUSER_DANGERRING_TOP_ID), aBottomY,
			3.1415927410125732, aCenter, aCenter, nullptr);
		g->PopState();
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColor(Color::White);
	// The arms and the lights on them, which turn with the update count
	float aCount = (float)mUpdateCnt;
	float aLeftTurn = (float)((double)aCount * 0.0625);
	float aRightTurn = (float)((double)aCount * -0.0625 - 1.5707963705062866);
	int anArtLeft;
	int aSlide;
	{
		Graphics anArmG(*g);
		if (mCache != nullptr)
			anArmG.ClipRect(mWidth / 2 - 200, 0, 400, mHeight);
		int anY = ArtY(IMAGE_DEFUSER_LEFTARM_ID);
		int anOffX = Ftol((double)GetImageOffsetX(IMAGE_DEFUSER_LEFTARM_ID) * (double)gApp->mScale);
		int anArtW = Ftol((double)gApp->mScale * 1920.0);
		double anArrive = mArrive.GetOutVal();
		anArtLeft = (gApp->mWidth - anArtW) / 2;
		aSlide = Ftol((double)gApp->mScale * 1500.0);
		anArmG.DrawImage(IMAGE_DEFUSER_LEFTARM, Ftol((double)anArtLeft - (double)aSlide * anArrive + (double)anOffX), anY);
		anY = ArtY(IMAGE_DEFUSER_LIGHTBASE_LEFT_ID);
		anOffX = Ftol((double)GetImageOffsetX(IMAGE_DEFUSER_LIGHTBASE_LEFT_ID) * (double)gApp->mScale);
		anArrive = mArrive.GetOutVal();
		g->DrawImageRotated(IMAGE_DEFUSER_LIGHTBASE_LEFT,
			Ftol((double)anArtLeft - (double)aSlide * anArrive + (double)anOffX), anY, (double)aLeftTurn, nullptr);
		anY = ArtY(IMAGE_DEFUSER_RIGHTARM_ID);
		anOffX = Ftol((double)GetImageOffsetX(IMAGE_DEFUSER_RIGHTARM_ID) * (double)gApp->mScale);
		anArrive = mArrive.GetOutVal();
		anArmG.DrawImage(IMAGE_DEFUSER_RIGHTARM, Ftol((double)aSlide * anArrive + (double)anArtLeft + (double)anOffX),
			anY);
		anY = ArtY(IMAGE_DEFUSER_LIGHTBASE_RIGHT_ID);
		anOffX = Ftol((double)GetImageOffsetX(IMAGE_DEFUSER_LIGHTBASE_RIGHT_ID) * (double)gApp->mScale);
		anArrive = mArrive.GetOutVal();
		g->DrawImageRotated(IMAGE_DEFUSER_LIGHTBASE_RIGHT,
			Ftol((double)aSlide * anArrive + (double)anArtLeft + (double)anOffX), anY, (double)aRightTurn, nullptr);
		if (mCache == nullptr) {
			int aDrop = Ftol((double)gApp->mScale * 500.0);
			anArmG.DrawImage(IMAGE_DEFUSER_BOTTOMARM, ArtX(IMAGE_DEFUSER_BOTTOMARM_ID),
				Ftol((double)aDrop * mUnknownDF8.GetOutVal() + (double)ArtY(IMAGE_DEFUSER_BOTTOMARM_ID)));
		}

		if (mArrive.GetOutVal() == 0.0) {
			// The icons over the slots: skulls, and the bomb's gems
			g->PushState();
			if (mState < 9) {
				int aClipH = Ftol((double)gApp->mScale * 248.0);
				int aClipW = Ftol((double)gApp->mScale * 772.0);
				int aClipY = Ftol((double)gApp->mScale * 450.0);
				int aClipX = (gApp->mWidth - Ftol((double)gApp->mScale * 1920.0)) / 2 - Ftol((double)gApp->mScale * -579.0);
				g->ClipRect(aClipX, aClipY, aClipW, aClipH);
			}
			g->SetColor(Color(255, 255, 255, Ftol(mIconAlpha.GetOutVal() * 255.0 * (double)aWheelAlpha)));
			for (int i = 0; i < 8; i++) {
				float anAngle = (float)(((double)i - 0.5) * -6.2831854820251465 * 0.125 - (double)mAngle);
				if (mUnfold.GetOutVal() < 1.0) {
					float aFrom = i / 4 == 0 ? -1.19f : -4.3f;
					double aPrev = mUnfold.GetOutVal();
					anAngle = (float)(mUnfold.GetOutVal() * (double)anAngle + (1.0 - aPrev) * (double)aFrom);
				}
				int aSlot = mSlots[i];
				if (aSlot == -2) {
					if (mState != 9 && mState != 10) {
						// The picked skull
						g->SetColor(Color(255, 255, 255, Ftol(mSkullColor.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
						aTransform.Reset();
						float aSkullScale = (float)mSkullScale.GetOutVal();
						aTransform.Scale(aSkullScale, (float)mSkullScale.GetOutVal());
						double aTurn = (double)anAngle + 0.0;
						aTransform.RotateRad((float)-aTurn);
						aTransform.Translate((float)Ftol((double)gApp->mScale * 85.0), (float)((double)gApp->mScale * -200.0));
						aTransform.RotateRad((float)aTurn);
						aTransform.Translate((float)(mWidth / 2), (float)((double)gApp->mScale * 570.0));
						g->DrawImageTransform(IMAGE_DEFUSER_SKULLICON_LARGE, aTransform, 0, 0);
					} else {
						// The picked gem, the bomb's colour, as it wobbles
						anAngle = (float)((double)anAngle - 0.7853981852531433);
						float aGemScale = mBoard->mUnknown268C * mGemScale;
						aTransform.Reset();
						double aTurn = (double)anAngle + 0.3799999952316284;
						aTransform.RotateRad((float)-aTurn);
						float aHalf = (float)((double)aGemScale * 0.5);
						aTransform.Scale(aHalf, aHalf);
						aTransform.Translate((float)Ftol((double)gApp->mScale * 0.0), (float)((double)gApp->mScale * -240.0));
						aTransform.RotateRad((float)aTurn);
						aTransform.Translate((float)(mWidth / 2), (float)((double)gApp->mScale * 570.0));
						if (0.1 < (double)aGemScale)
							g->DrawImageTransform(IMAGE_GEMS, aTransform, IMAGE_GEMS->GetCelRect(mPiece->mColor), 0, 0);
					}
				} else if (mState < 9) {
					if (aSlot == -1) {
						aTransform.Reset();
						aTransform.Scale(1.0f, 1.0f);
						double aTurn = (double)anAngle + 0.0;
						aTransform.RotateRad((float)-aTurn);
						aTransform.Translate((float)Ftol((double)gApp->mScale * 85.0), (float)((double)gApp->mScale * -200.0));
						aTransform.RotateRad((float)aTurn);
						aTransform.Translate((float)(mWidth / 2), (float)((double)gApp->mScale * 570.0));
						g->DrawImageTransform(IMAGE_DEFUSER_SKULLICON_LARGE, aTransform, 0, 0);
					} else {
						anAngle = (float)((double)anAngle - 0.7853981852531433);
						aTransform.Reset();
						double aTurn = (double)anAngle + 0.3799999952316284;
						aTransform.RotateRad((float)-aTurn);
						aTransform.Scale(0.75f, 0.75f);
						aTransform.Translate((float)Ftol((double)gApp->mScale * 0.0), (float)((double)gApp->mScale * -240.0));
						aTransform.RotateRad((float)aTurn);
						aTransform.Translate((float)(mWidth / 2), (float)((double)gApp->mScale * 570.0));
						g->DrawImageTransform(IMAGE_GEMS, aTransform, IMAGE_GEMS->GetCelRect(aSlot), 0, 0);
					}
				}
			}
			g->PopState();
			if (mState < 9)
				g->DrawImage(IMAGE_DEFUSER_SELECTOVERLAY, ArtX(IMAGE_DEFUSER_SELECTOVERLAY_ID),
					ArtY(IMAGE_DEFUSER_SELECTOVERLAY_ID));
		}
		if (!mPiece->mUnknown2F6) {
			// The bomb itself, over the wheel at the board's zoom (and its wobble)
			if (0.1 < (double)mGemScale) {
				g->PushState();
				if (1.0f != mGemScale)
					g->SetScale(mGemScale, mGemScale, (float)(mWidth / 2), (float)(mHeight / 2));
				g->Translate(Ftol((double)mBoard->mShakeX), Ftol((double)mBoard->mShakeY));
				mBoard->DrawNukeGem(g, mPiece);
				g->PopState();
			}
		}
		// The light beams
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color(255, 255, 255, 0x80));
		g->SetColor(Color(255, 255, 255, 0x80));
		int aBeamCenter = Ftol((double)gApp->mScale * 300.0);
		anY = ArtY(IMAGE_DEFUSER_LIGHTBEAM_LEFT_ID);
		anOffX = Ftol((double)GetImageOffsetX(IMAGE_DEFUSER_LIGHTBEAM_LEFT_ID) * (double)gApp->mScale);
		anArtLeft = (gApp->mWidth - Ftol((double)gApp->mScale * 1920.0)) / 2;
		anArrive = mArrive.GetOutVal();
		aSlide = Ftol((double)gApp->mScale * 1500.0);
		g->DrawImageRotated(IMAGE_DEFUSER_LIGHTBEAM_LEFT,
			Ftol((double)anArtLeft - (double)aSlide * anArrive + (double)anOffX), anY, (double)aLeftTurn, aBeamCenter,
			aBeamCenter, nullptr);
		aBeamCenter = Ftol((double)gApp->mScale * 300.0);
		anY = ArtY(IMAGE_DEFUSER_LIGHTBEAM_RIGHT_ID);
		anOffX = Ftol((double)GetImageOffsetX(IMAGE_DEFUSER_LIGHTBEAM_RIGHT_ID) * (double)gApp->mScale);
		anArrive = mArrive.GetOutVal();
		aSlide = Ftol((double)gApp->mScale * 1500.0);
		g->DrawImageRotated(IMAGE_DEFUSER_LIGHTBEAM_RIGHT,
			Ftol((double)aSlide * anArrive + (double)anArtLeft + (double)anOffX), anY, (double)aRightTurn, aBeamCenter,
			aBeamCenter, nullptr);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		mFXManager->Draw(g);
		if (aMadeCache) {
			// The button's overlay went into the cache this time: it is drawn here
			g->PushState();
			g->Translate(mButton->mX - mX, mButton->mY - mY);
			mButton->DrawButton(g);
			g->PopState();
		}
	}
}
