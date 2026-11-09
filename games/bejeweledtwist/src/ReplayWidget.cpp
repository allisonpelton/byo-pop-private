// Sexy::ReplayWidget (ReplayWidget.h).
#include "ReplayWidget.h"

#include "Board.h"
#include "Graphics.h"
#include "Image.h"
#include "Profile.h"
#include "Res.h"
#include "TwistButton.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>

using namespace Sexy;

ReplayWidget::ReplayWidget()
{
	mClip = false;
	mTargetY = gBoard->mHeight;
	mPod = IMAGE_INGAMEUI_REPLAYPOD;
	mButton = new TwistButton(0x2a, IMAGE_INGAMEUI_REPLAYPODBUTTON, this);
	if (mPod != nullptr)
		Widget::Resize(0, 0, mPod->mWidth, mPod->mHeight);
}

ReplayWidget::~ReplayWidget()
{
	// The original never deletes its button
	delete mButton;
}

void ReplayWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	TwistWidget::AddedToManager(theWidgetManager);
	mY = gBoard->mHeight;
	if (mPod != nullptr) {
		// The button sits where resources.xml puts it over the pod
		int aButtonY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(IMAGE_INGAMEUI_REPLAYPODBUTTON_ID));
		int aPodY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(GetIdByImage(mPod)));
		int aButtonX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(IMAGE_INGAMEUI_REPLAYPODBUTTON_ID));
		int aPodX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(GetIdByImage(mPod)));
		mButton->Resize(aButtonX - aPodX, aButtonY - aPodY, mButton->mButtonImage->GetCelWidth(),
			mButton->mButtonImage->GetCelHeight());
	}
	AddWidget(mButton);
}

void ReplayWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	RemoveWidget(mButton);
	TwistWidget::RemovedFromManager(theWidgetManager);
}

void ReplayWidget::Update()
{
	TwistWidget::Update();
	if (mState == STATE_RISING) {
		mY -= 5;
		if (mY <= mTargetY) {
			mY = mTargetY;
			mTargetY = Ftol((double)gBoard->mHeight - (double)mHeight * 0.8999999761581421);
			mState = STATE_SETTLING;
		}
	} else if (mState == STATE_SETTLING) {
		mY += 2;
		if (mTargetY <= mY) {
			mY = mTargetY;
			mState = STATE_UP;
		}
	} else if (mState == STATE_GOING) {
		mY += 8;
		if (gBoard->mHeight < mY) {
			mY = gBoard->mHeight;
			mState = STATE_UP;
			mVisible = false;
		}
	}
	if (0.0f < mBobWait) {
		mBobWait = (float)((double)mBobWait - 0.009999999776482582);
	} else {
		float aNext = (float)((double)mBobPhase + 0.029999999329447746);
		mBobPhase = aNext;
		if (1.0f <= aNext) {
			mBobPhase = 0;
			mBobSide = (mBobSide + 1) % 2;
			mBobWait = 1.0f;
		}
	}
}

void ReplayWidget::Draw(Graphics* g)
{
	if (mPod == nullptr)
		return;
	g->SetColorizeImages(false);
	g->DrawImage(mPod, 0, 0);
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColorizeImages(true);
	// The glow pulses every 400 updates
	float anAngle = (float)((double)(mUpdateCnt % 400) / 400.0 * 3.1415927410125732);
	float aPulse = std::fabs(std::sin(anAngle));
	g->SetColor(Color(255, 255, 255, Ftol((double)aPulse * 255.0)));
	Image* aGlow = IMAGE_INGAMEUI_REPLAYPOD_GLOW;
	int aY = Ftol((double)gApp->mScale * 50.0);
	int anOffsetX = Ftol((double)gApp->mScale * -0.0);
	g->DrawImage(aGlow, -(aGlow->mWidth / 2) - anOffsetX + mWidth / 2, aY);
}

void ReplayWidget::DrawAll(ModalFlags* theFlags, Graphics* g)
{
	// It bobs while it is up, a cycle every 150 updates
	float aBob = std::sin((float)((double)(mUpdateCnt % 150) / 150.0 * (double)6.2831855f));
	float aTransY = g->mTransY;
	if (mPopped) {
		int anAmplitude = Ftol((double)gApp->mScale * 10.0);
		g->mTransY = (float)anAmplitude * aBob + g->mTransY;
	}
	TwistWidget::DrawAll(theFlags, g);
	g->mTransY = aTransY;
}

void ReplayWidget::ButtonDepress(int)
{
	// The move again: the game now is kept for when the board is left (+0x23b4), and the rank and +0x160 go back to
	// what they were before the move
	gApp->PlaySample(SOUND_BUTTONPRESS1);
	gBoard->SaveGame(&gBoard->mReplaySave, false);
	gApp->mProfile->mRank = gBoard->mSavedRank;
	gApp->mProfile->mRankPoints = gBoard->mSavedRankPoints;
	gBoard->StartReplay(gBoard->mSaveBuffer, gBoard->mReplayStartFrame, gBoard->mReplayEndFrame, true);
	mVisible = false;
}

void ReplayWidget::Show()
{
	Board* aBoard = gBoard;
	if (mPopped || aBoard->mTipShowing)
		return;
	int aX;
	int aY;
	aBoard->GetCenter(&aX, &aY);
	if (aBoard->mBoardFade == 0.0f)
		aX = aBoard->mWidth / 2;
	mX = aX - mWidth / 2;
	gApp->PlaySample(SOUND_REPLAY_POPUP);
	mState = STATE_RISING;
	mPopped = true;
	mVisible = true;
	mTargetY = gBoard->mHeight - mHeight;
}
