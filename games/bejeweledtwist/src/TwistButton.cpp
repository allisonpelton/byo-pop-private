// Sexy::TwistButton (TwistButton.h).
#include "TwistButton.h"

#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "WinTwistApp.h"
#include "X86Math.h"

using namespace Sexy;

TwistButton::TwistButton(int theId, Image* theImage, ButtonListener* theListener) : ButtonWidget(theId, theListener)
{
	mOverAlpha = 0.5;
	mDoFinger = true;
	mHasAlpha = true;
	mClip = false;
	mColor = Color::White;
	// The original also sets GetColor(0) to Color::Black: with no colors set that is the default color, black already
	mLabelOffsetX = Ftol((double)gApp->mScale * 0.0);
	mLabelOffsetY = Ftol((double)gApp->mScale * 0.0);
	mButtonImage = theImage;
	if (theImage != nullptr)
		Resize(0, 0, theImage->GetCelWidth(), theImage->GetCelHeight());
}

TwistButton::~TwistButton()
{
}

void TwistButton::Update()
{
	Widget::Update();
}

void TwistButton::Draw(Graphics* g)
{
	if (mDeferDraw) {
		DeferOverlay(mDeferPriority);
		return;
	}
	DrawButton(g);
}

void TwistButton::DrawOverlay(Graphics* g)
{
	DrawButton(g);
}

void TwistButton::DrawButton(Graphics* g)
{
	if (mBtnNoDraw || mButtonImage == nullptr)
		return;
	g->SetColorizeImages(true);
	if (mIcon != nullptr) {
		g->SetColor(Color::White);
		g->DrawImage(mIcon, mWidth / 2 - mIcon->mWidth / 2 + mIconOffsetX, mHeight / 2 - mIcon->mHeight / 2 + mIconOffsetY);
	}
	int aCel;
	if (mDisabled)
		aCel = 3;
	else if (mIsDown && mIsOver)
		aCel = 2;
	else
		aCel = (mIsOver || mHighlighted) ? 1 : 0;
	Color aColor = mColor;
	if (mButtonImage->mNumRows * mButtonImage->mNumCols - 1 < aCel) {
		if (!mDisabled) {
			aCel = mButtonImage->mNumRows * mButtonImage->mNumCols - 1;
		} else {
			// no disabled cel: the normal one at half the colour
			aColor.mRed /= 2;
			aColor.mBlue /= 2;
			aColor.mGreen /= 2;
			aCel = 0;
		}
	}
	g->SetColor(aColor);
	g->DrawImageCel(mButtonImage, Rect(0, 0, mWidth, mHeight), aCel);
	g->SetColorizeImages(false);
	if (mFont != nullptr && !mLabel.empty()) {
		int aLabelWidth = mFont->StringWidth(mLabel);
		int anAscent = mFont->GetAscent();
		int anAscent2 = mFont->GetAscent();
		g->SetColor(GetColor(0));
		g->SetFont(mFont);
		g->DrawString(mLabel, (mWidth - aLabelWidth) / 2 + mLabelOffsetX + 1,
			(anAscent2 + (mHeight - anAscent / 4) - 1) / 2 - Ftol((double)gApp->mScale * -0.0));
	}
}

void TwistButton::MouseEnter()
{
	// The original says "TwistButton::MouseEnter() for ..." with OutputDebugString (mName or the address)
	ButtonWidget::MouseEnter();
	if (mPlayMouseOverSound && gApp->mHasFocus)
		PlayMouseOverSound();
}

void TwistButton::MouseLeave()
{
	// The original says "TwistButton::MouseLeave() for ..." with OutputDebugString
	ButtonWidget::MouseLeave();
	if (!gApp->mHasFocus)
		mOverAlpha = 0;
	MarkDirty();
}

void TwistButton::PlayMouseOverSound()
{
	gApp->PlaySoundEx(SOUND_MOUSEOVER1, 0, (float)((float)mId * 0.0), 0, true, true);
}
