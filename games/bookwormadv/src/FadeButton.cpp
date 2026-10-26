#include "FadeButton.h"

#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"

using namespace Sexy;

FadeButton::FadeButton(int theId, ButtonListener* theListener)
	: ButtonWidget(theId, theListener)
{
	mDoFinger = true;
}

void FadeButton::SetupImage(Image* theImage)
{
	if (theImage == nullptr) {
		if (mWidth == 0 && mFont != nullptr && mButtonImage != nullptr)
			Resize(mX, mY, mFont->StringWidth(mLabel), mButtonImage->mHeight);
	} else {
		mButtonImage = theImage;
		mOverImage = theImage;
		mDownImage = theImage;
		mDisabledImage = theImage;
		Resize(0, 0, theImage->GetCelWidth(), theImage->GetCelHeight());
	}
	if (mFont == nullptr)
		SetFont(FONT_LARGE_OUTLINE);
	SetColor(COLOR_LABEL, Color(255, 255, 255));
	SetColor(COLOR_LABEL_HILITE, Color(255, 255, 0));
	if (theImage == nullptr)
		return;
	int aCelWidth = theImage->GetCelWidth();
	int aCelHeight = theImage->GetCelHeight();
	mNormalRect = Rect(0, 0, aCelWidth, aCelHeight);
	mOverRect = Rect(aCelWidth, 0, aCelWidth, aCelHeight);
	mDownRect = Rect(aCelWidth * 2, 0, aCelWidth, aCelHeight);
	mDisabledRect = Rect(0, 0, aCelWidth, aCelHeight);
	if (theImage->mNumCols > 3)
		mDisabledRect = Rect(aCelWidth * 3, 0, aCelWidth, aCelHeight);
	mHasAlpha = true;
	mHasTransparencies = true;
	mFadeSpeed = 20;
	mFadeOnDisable = true;
	mFadeOnEnter = true;
	mFadeOnLeave = true;
	mFadeOnDown = true;
	mFadeOnUp = true;
	mDoFinger = true;
}

void FadeButton::FadeStep()
{
	if (mFade < 255 && mUpdateCnt % mFadeEvery == 0) {
		mFade += mFadeSpeed;
		if (mFade > 255)
			mFade = 255;
	}
}

void FadeButton::Update()
{
	ButtonWidget::Update();
	if (mIsOver || (mFadeOnLeave && mFade > 0 && mFade < 255))
		MarkDirty();
	if (mGlow && !mIsOver && !mIsDown) {
		mGlowAlpha += mGlowDelta;
		if ((mGlowAlpha >= mGlowMax && mGlowDelta > 0) || (mGlowAlpha <= mGlowMin && mGlowDelta < 0)) {
			mGlowAlpha = mGlowDelta > 0 ? mGlowMax : mGlowMin;
			mGlowDelta = -mGlowDelta;
		}
	}
	if (mDisabled) {
		if (mFadeOnDisable)
			FadeStep();
		return;
	}
	if (mIsOver && !mIsDown && (mFadeOnEnter || mFadeOnUp))
		FadeStep();
	else if (mIsDown && mFadeOnDown)
		FadeStep();
	else if (!mIsOver && mFadeOnLeave)
		FadeStep();
	// port: the original also stepped the fade when +0x129 was set, which SetDisabled sets only in a case that
	// cannot happen (being enabled while disabled)
}

void FadeButton::DrawButtonImage(Graphics* g, Image* theImage, const Rect& theRect, int x, int y)
{
	if (theRect.mWidth == 0) {
		if (!mStretch)
			g->DrawImage(theImage, x, y);
		else
			g->DrawImage(theImage, Rect(x, y, mWidth, mHeight), Rect(0, 0, theImage->mWidth, theImage->mHeight));
	} else {
		if (!mStretch)
			g->DrawImage(mButtonImage, x, y, theRect);
		else
			g->DrawImage(mButtonImage, Rect(x, y, mWidth, mHeight), theRect);
	}
}

void FadeButton::Draw(Graphics* g)
{
	if (mBtnNoDraw)
		return;
	int aLabelX = 0;
	int aLabelY = 0;
	if (mFont != nullptr) {
		aLabelX = (mWidth - mFont->StringWidth(mLabel)) / 2;
		aLabelY = (mHeight - mFont->GetAscent() / 6 - 1 + mFont->GetAscent()) / 2;
	}
	g->SetFont(mFont);
	if (mButtonImage == nullptr && mDownImage == nullptr) {
		ButtonWidget::Draw(g);
		return;
	}

	// the new look's picture, drawn at mFade over the old one's
	auto aFadeIn = [&](Image* theOld, const Rect& theOldRect, Image* theNew, const Rect& theNewRect) {
		if (theOld != nullptr)
			DrawButtonImage(g, theOld, theOldRect, 0, 0);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, mFade));
		DrawButtonImage(g, theNew, theNewRect, 0, 0);
		g->SetColorizeImages(false);
	};

	bool isDown = (mIsDown && mIsOver && !mDisabled) ^ mInverted;
	if (isDown) {
		if (HaveButtonImage(mDownImage, mDownRect)) {
			if (mFade == 255)
				DrawButtonImage(g, mDownImage, mDownRect, 0, 0);
			else
				aFadeIn(HaveButtonImage(mOverImage, mOverRect) ? mOverImage : nullptr, mOverRect, mDownImage, mDownRect);
		} else if (HaveButtonImage(mButtonImage, mNormalRect)) {
			if (mFade == 255)
				DrawButtonImage(g, mButtonImage, mNormalRect, 0, 0);
			else
				aFadeIn(nullptr, mNormalRect, mButtonImage, mNormalRect);
		}
	} else if (mDisabled && HaveButtonImage(mDisabledImage, mDisabledRect)) {
		DrawButtonImage(g, mDisabledImage, mDisabledRect, 0, 0);
	} else if ((mIsOver || mIsDown) && HaveButtonImage(mOverImage, mOverRect)) {
		if (mFade == 255) {
			DrawButtonImage(g, mOverImage, mOverRect, 0, 0);
		} else {
			bool aWasDown = (mClicked || mIsDown) && HaveButtonImage(mDownImage, mDownRect);
			aFadeIn(aWasDown ? mDownImage : mButtonImage, aWasDown ? mDownRect : mNormalRect, mOverImage, mOverRect);
		}
	} else if (HaveButtonImage(mButtonImage, mNormalRect)) {
		if (mFade != 255) {
			aFadeIn(HaveButtonImage(mOverImage, mOverRect) ? mOverImage : nullptr, mOverRect, mButtonImage, mNormalRect);
		} else {
			DrawButtonImage(g, mButtonImage, mNormalRect, 0, 0);
			if (mGlow) {
				g->PushState();
				g->SetColorizeImages(true);
				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				g->SetColor(Color(255, 255, 255, mGlowAlpha));
				DrawButtonImage(g, mButtonImage, mNormalRect, 0, 0);
				g->PopState();
			}
		}
	}

	if (mIsOver && mColors.size() > 1)
		g->SetColor(mColors[COLOR_LABEL_HILITE]);
	else if (!mColors.empty())
		g->SetColor(mColors[COLOR_LABEL]);
	g->DrawString(mLabel, aLabelX + 1, aLabelY + 1 + mLabelOffsetY);
}

void FadeButton::SetDisabled(bool isDisabled)
{
	ButtonWidget::SetDisabled(isDisabled);
	mFade = mFadeOnDisable ? 0 : 255;
	if (mHideMouseWhenDisabled)
		mMouseVisible = !isDisabled;
}

void FadeButton::MouseEnter()
{
	ButtonWidget::MouseEnter();
	mFade = mFadeOnEnter ? 0 : 255;
	mClicked = false;
	MarkDirty();
}

void FadeButton::MouseLeave()
{
	ButtonWidget::MouseLeave();
	mFade = mFadeOnLeave ? 0 : 255;
	MarkDirty();
}

void FadeButton::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	ButtonWidget::MouseDown(x, y, theBtnNum, theClickCount);
	mFade = mFadeOnDown ? 0 : 255;
	MarkDirty();
}

void FadeButton::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
	ButtonWidget::MouseUp(x, y, theBtnNum, theClickCount);
	mFade = mFadeOnUp ? 0 : 255;
	if (mIsOver)
		mClicked = true;
	MarkDirty();
}
