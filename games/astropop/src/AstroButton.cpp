#include "AstroButton.h"
#include "AstroPopApp.h"
#include "ButtonListener.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "SoundInstance.h"
#include "SoundManager.h"

using namespace Sexy;

namespace
{
	const int NO_PITCH = 500000000; // 0x1dcd6500

	// The cel of an animation: always the first when it has a single frame
	inline int CelOf(const FrameAnim& theAnim)
	{
		return (theAnim.mNumFrames == 1) ? 0 : theAnim.mFrame;
	}
}

// 0x454c10
AstroButton::AstroButton(int theId, ButtonListener* theListener) :
	ButtonWidget(theId, theListener)
{
	mDrawEnabled = true;
	mGlow = false;
	mStopOtherSounds = true;

	mAnimIdle.SetStopped(false);
	mAnimIdle.mPingPong = false;
	mAnimIdle.SetDelay(10);
	mAnimIdle.SetNumFrames(1);
	mAnimIdle.mHoldLast = false;

	mAnimOver.SetStopped(true);
	mAnimOver.mPingPong = false;
	mAnimOver.SetDelay(10);
	mAnimOver.SetNumFrames(1);
	mAnimOver.mHoldLast = false;

	mAnimDown.SetStopped(true);
	mAnimDown.mPingPong = false;
	mAnimDown.SetDelay(10);
	mAnimDown.SetNumFrames(1);
	mAnimDown.mHoldLast = false;

	mSound = NULL;
	mHitMask = NULL;
	mHitMaskWidth = 0;
	mSoundId = -1;
	mButtonImage = NULL;
	mOverImage = NULL;
	mDownImage = NULL;
	mDoFinger = true;
	mSoundPitch = NO_PITCH;
	mUseTintNormal = false;
	mUseTintOver = false;
	mTintOver = Color(0, 0, 0, 0);
	mTintNormal = Color(0, 0, 0, 0);
}

// 0x454e00 / 0x454e30
AstroButton::~AstroButton()
{
}

// 0x43bda0
void AstroButton::Draw(Graphics* g)
{
	if (!mDrawEnabled)
		return;

	int aLabelX = 0;
	int aLabelY = 0;
	if (mFont != NULL)
	{
		aLabelX = (mWidth - mFont->StringWidth(mLabel)) / 2;
		aLabelY = (mHeight - mFont->GetAscent() / 6 + mFont->GetAscent() - 1) / 2;
	}

	Image* anImage = mOverImage;
	if (anImage == NULL)
		anImage = mButtonImage;

	if (mDisabled)
		ButtonWidget::Draw(g);
	else if (!mAnimIdle.mStopped && mButtonImage != NULL)
		g->DrawImageCel(mButtonImage, 0, 0, CelOf(mAnimIdle));
	else if (!mAnimDown.mStopped)
	{
		// pressed: the down image, or the over image one pixel down and right
		if (mDownImage != NULL)
		{
			if (!mGlow && mUseTintOver)
			{
				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				g->SetColorizeImages(true);
				g->SetColor(mTintOver);
			}
			g->DrawImageCel(mDownImage, 0, 0, CelOf(mAnimDown));
			if (!mGlow && mUseTintOver)
			{
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
				g->SetColorizeImages(false);
			}
		}
		else if (anImage != NULL)
		{
			if (!mGlow && mUseTintOver)
			{
				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				g->SetColorizeImages(true);
				g->SetColor(mTintOver);
			}
			g->DrawImageCel(anImage, 1, 1, CelOf(mAnimOver));
			if (!mGlow && mUseTintOver)
			{
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
				g->SetColorizeImages(false);
			}
		}
	}
	else if (!mAnimOver.mStopped && anImage != NULL)
	{
		if (!mGlow && mUseTintNormal)
		{
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColorizeImages(true);
			g->SetColor(mTintNormal);
		}
		g->DrawImageCel(anImage, 0, 0, CelOf(mAnimOver));
		if (!mGlow && mUseTintNormal)
		{
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			g->SetColorizeImages(false);
		}
	}

	// The glow: the same picture again, additive in white with a pulsing alpha (mIsOver is set meanwhile)
	if (mGlow && !mIsOver)
	{
		int aPhase = mUpdateCnt % 100;
		mIsOver = true;
		if (aPhase > 0x32)
			aPhase = 100 - aPhase;
		g->SetColor(Color(255, 255, 255, (aPhase * 255) / 50));
		g->SetColorizeImages(true);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);

		bool aTryPressed = true;
		if (mDisabled)
		{
			ButtonWidget::Draw(g);
			aTryPressed = false;
		}
		else if (!mAnimIdle.mStopped)
		{
			if (mButtonImage != NULL)
			{
				g->DrawImageCel(mButtonImage, 0, 0, CelOf(mAnimIdle));
				aTryPressed = false;
			}
			else if (mOverImage != NULL)
			{
				int anAlpha = mUpdateCnt % 0xfe;
				if (anAlpha > 0x7f)
					anAlpha = 0xfe - anAlpha;
				g->SetColor(Color(255, 255, 255, anAlpha));
				g->DrawImageCel(mOverImage, 0, 0, 0);
				aTryPressed = false;
			}
		}

		if (aTryPressed)
		{
			bool aTryOver = true;
			if (!mAnimDown.mStopped)
			{
				if (mDownImage != NULL)
				{
					g->DrawImageCel(mDownImage, 0, 0, CelOf(mAnimDown));
					aTryOver = false;
				}
				else if (anImage != NULL)
				{
					g->DrawImageCel(anImage, 1, 1, CelOf(mAnimOver));
					aTryOver = false;
				}
			}
			if (aTryOver && !mAnimOver.mStopped && anImage != NULL)
				g->DrawImageCel(anImage, 0, 0, CelOf(mAnimOver));
		}

		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColorizeImages(false);
		mIsOver = false;
	}

	if (mFont != NULL)
	{
		g->SetColor(mColors[mIsOver ? 1 : 0]);
		g->SetFont(mFont);
		g->DrawString(mLabel, aLabelX, aLabelY);
	}
}

// 0x434a20
void AstroButton::Update()
{
	ButtonWidget::Update();
	if (!mDisabled)
	{
		mAnimIdle.Update();
		mAnimOver.Update();
		mAnimDown.Update();
		if (mAnimIdle.ConsumeFrameChanged() || mAnimOver.ConsumeFrameChanged() || mAnimDown.ConsumeFrameChanged() ||
			mGlow)
			MarkDirty();
	}
}

// 0x43bc00
// port: the original's ButtonWidget::MouseEnter calls a slot of the listener that is empty for the menu's, and this function
// calls the mouse-enter slot (6) itself, once. The port's ButtonWidget::MouseEnter calls ButtonMouseEnter already, so a second
// call here would make the menu's secret order (a count of the buttons touched, one step each) fail at its second step.
void AstroButton::MouseEnter()
{
	ButtonWidget::MouseEnter();

	if (mSound == NULL || mSound->IsPlaying())
	{
		// a new instance of the sample, or the plain sample
		if (mSoundId != -1)
		{
			if (mSoundPitch == NO_PITCH)
				gApp->PlaySample(mSoundId);
			else
			{
				SoundInstance* anInstance = gApp->mSoundManager->GetSoundInstance(mSoundId);
				if (anInstance != NULL)
				{
					anInstance->AdjustPitch((double) mSoundPitch);
					anInstance->Play(false, true);
				}
			}
		}
	}
	else
	{
		// the owner's instance is idle: the sounds listed with it are stopped first (when one of them plays and the
		// flag says so; otherwise the whole sound is skipped)
		bool aSkip = false;
		for (size_t i = 0; i < mOtherSounds.size(); i++)
		{
			if (mOtherSounds[i]->IsPlaying())
			{
				if (!mStopOtherSounds)
				{
					aSkip = true;
					break;
				}
				mOtherSounds[i]->Stop();
			}
		}
		if (!aSkip)
		{
			if (mSoundPitch != NO_PITCH)
				mSound->AdjustPitch((double) mSoundPitch);
			mSound->Play(false, false);
		}
	}

	if (mAnimOver.mStopped)
	{
		if (!mIsDown)
		{
			mAnimOver.Reset();
			mAnimOver.SetStopped(false);
			mAnimIdle.SetStopped(true);
			return;
		}
	}
	else if (!mIsDown)
		return;

	if (mAnimDown.mStopped)
	{
		if (mDownImage != NULL)
			mAnimOver.SetStopped(true);
		else
		{
			mAnimOver.Reset();
			mAnimOver.SetStopped(false);
		}
		mAnimIdle.SetStopped(true);
		mAnimDown.Reset();
		mAnimDown.SetStopped(false);
	}
}

// 0x4348f0 (port: the listener's mouse-leave slot is called by ButtonWidget::MouseLeave, as for the enter above)
void AstroButton::MouseLeave()
{
	ButtonWidget::MouseLeave();
	mAnimOver.SetStopped(true);
	mAnimDown.SetStopped(true);
	mAnimIdle.Reset();
	mAnimIdle.SetStopped(false);
}

// 0x434940
void AstroButton::MouseDown(int x, int y, int theClickCount)
{
	ButtonWidget::MouseDown(x, y, theClickCount);
	if (mDownImage != NULL)
		mAnimOver.SetStopped(true);
	else
	{
		mAnimOver.Reset();
		mAnimOver.SetStopped(false);
	}
	mAnimIdle.SetStopped(true);
	mAnimDown.Reset();
	mAnimDown.SetStopped(false);
}

// 0x4349c0
void AstroButton::MouseUp(int x, int y)
{
	ButtonWidget::MouseUp(x, y);
	if (mIsOver)
	{
		mAnimDown.SetStopped(true);
		mAnimIdle.SetStopped(true);
		mAnimOver.Reset();
		mAnimOver.SetStopped(false);
	}
}

// 0x4348a0
bool AstroButton::IsPointVisible(int x, int y)
{
	if (x > mWidth || y > mHeight || x < 0 || y < 0)
		return false;
	if (mHitMask == NULL)
		return true;
	return mHitMask[mHitMaskWidth * y + x] == -1;
}
