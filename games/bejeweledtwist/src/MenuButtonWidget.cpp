// Sexy::MenuButtonWidget (MenuButtonWidget.h).
#include "MenuButtonWidget.h"

#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "WinTwistApp.h"
#include "X86Math.h"

using namespace Sexy;

MenuButtonWidget::MenuButtonWidget(int theId, Image* theImage, ButtonListener* theListener)
	: TwistButton(theId, theImage, theListener)
{
	mApp = gApp;
	mImages[0] = theImage;
}

MenuButtonWidget::~MenuButtonWidget() = default;

void MenuButtonWidget::Update()
{
	Widget::Update();
	if (mFlip.mIncRate != 0.0) {
		double aBefore = mFlip.GetOutVal();
		mFlip.IncInVal();
		if ((float)aBefore < 0.9990000128746033 && mFlip.GetOutVal() >= 0.9990000128746033)
			Flip();
	}
	if (!mGlowOn) {
		mGlow = (float)(mGlow - 0.03999999910593033);
		if (mGlow <= 0.0f)
			mGlow = 0;
	} else {
		mGlow = (float)(mGlow + 0.03999999910593033);
		if (mGlow > 1.0f)
			mGlow = 1.0f;
	}

	// ButtonWidget::Update's part, written out
	if (mIsDown && mIsOver)
		mButtonListener->ButtonDownTick(mId);
	if (!mIsDown && !mIsOver && mOverAlpha > 0) {
		if (mOverAlphaSpeed > 0) {
			mOverAlpha -= mOverAlphaSpeed;
			if (mOverAlpha < 0)
				mOverAlpha = 0;
		} else {
			mOverAlpha = 0;
		}
		MarkDirty();
	} else if (mIsOver && mOverAlphaFadeInSpeed > 0 && mOverAlpha < 1.0) {
		mOverAlpha += mOverAlphaFadeInSpeed;
		if (mOverAlpha > 1.0)
			mOverAlpha = 1.0;
		MarkDirty();
	}
	MarkDirty();
}

void MenuButtonWidget::Draw(Graphics* g)
{
	// With Direct3D the clip takes in the screens around this one (the menu pans over a 3x3 field)
	g->mClipRect = Rect(0, 0, mApp->mWidth, mApp->mHeight);

	if (!mBtnNoDraw) {
		bool isDown = mIsDown && mIsOver && !mDisabled;
		isDown ^= mInverted;
		int aFontX = 0;
		int aFontY = -5;
		if (mFont != nullptr) {
			if (mLabelJustify == 0)
				aFontX = (mWidth - mFont->StringWidth(mLabel)) / 2;
			else if (mLabelJustify == 1)
				aFontX = mWidth - mFont->StringWidth(mLabel);
			int anAscent = mFont->GetAscent();
			aFontY = (mFont->GetAscent() + (mHeight - anAscent / 6) - 1) / 2;
		}
		g->SetFont(mFont);
		int aWidth = Ftol((double)mWidth * mScale);
		int aHeight = Ftol((double)mHeight * mScale);
		int anX = mWidth / 2 - aWidth / 2;
		int aY = mHeight / 2 - aHeight / 2;

		Image* anImage = nullptr;
		Image* anOverImage = nullptr;
		Rect aSrc(0, 0, 0, 0);
		Rect anOverSrc(0, 0, 0, 0);
		if (!isDown) {
			if (mDisabled && HaveButtonImage(mDisabledImage, mDisabledRect)) {
				anImage = mDisabledImage;
				aSrc = mDisabledRect;
			} else if (mOverAlpha > 0.0 && HaveButtonImage(mOverImage, mOverRect)) {
				if (HaveButtonImage(mButtonImage, mNormalRect) && mOverAlpha < 1.0) {
					anImage = mButtonImage;
					aSrc = mNormalRect;
				}
				anOverImage = mOverImage;
				anOverSrc = mOverRect;
			} else if ((mIsOver || mIsDown) && HaveButtonImage(mOverImage, mOverRect)) {
				anImage = mOverImage;
				aSrc = mOverRect;
			} else if (mButtonImage != nullptr && HaveButtonImage(mButtonImage, mNormalRect)) {
				anImage = mButtonImage;
				aSrc = mNormalRect;
			}
		} else if (HaveButtonImage(mDownImage, mDownRect)) {
			anImage = mDownImage;
			aSrc = mDownRect;
		} else if (HaveButtonImage(mOverImage, mOverRect)) {
			anImage = mOverImage;
			aSrc = mOverRect;
		} else {
			anImage = mButtonImage;
			aSrc = mNormalRect;
		}

		if (!mFlipped) {
			mFlip.GetOutVal();  // the original reads it and drops it
		} else {
			anImage = mImages[1];
			aSrc = anImage->GetCelRect(0);
			anOverImage = anImage;
			anOverSrc = anImage->GetCelRect(1);
			if (isDown)
				anOverSrc = anImage->GetCelRect(2);
			if (mDisabled && anImage->mNumRows * anImage->mNumCols > 3)
				aSrc = anImage->GetCelRect(3);
		}
		if (anImage != nullptr)
			g->DrawImage(anImage, Rect(anX, aY, aSrc.mWidth, aSrc.mHeight), aSrc);
		if (anOverImage != nullptr) {
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, Ftol(mOverAlpha * 255.0)));
			g->DrawImage(anOverImage, Rect(anX, aY, anOverSrc.mWidth, anOverSrc.mHeight), anOverSrc);
			g->SetColorizeImages(false);
		}
		g->SetColor(mColors[mIsOver ? 1 : 0]);
		g->DrawString(mLabel, aFontX, aFontY);
	}

	if (mShowCount) {
		g->SetFont(FONT_DIALOG_BUTTONS);
		SexyString aCount = StrFormat(_S("%d"), mCount);
		int aCountWidth = g->StringWidth(aCount);
		g->SetColor(Color(255, 255, 255));
		g->DrawString(aCount, (mWidth - aCountWidth) / 2, mHeight * 3 / 4);
	}

	float anOpen = mGlow;
	if (mFlip.mIncRate != 0.0 && !mFlip.HasBeenTriggered()) {
		double aVal = 0;
		if (0 <= 1.0 - mFlip.GetOutVal())
			aVal = 1.0 - mFlip.GetOutVal();
		anOpen = (float)aVal;
	}
	if (mCover != nullptr && anOpen < 1.0f) {
		// With Direct3D the doors (IMAGE_DOORS_*) are drawn with a shader instead of the cels
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, 255));
		int aCels = mCover->mNumRows * mCover->mNumCols;
		int aCel = Ftol((double)aCels * (1.0 - anOpen) - 1.0);
		if (aCel >= 0) {
			int aCelY = (mNormalRect.mHeight / 2 + mNormalRect.mY) - mCover->GetCelHeight() / 2;
			int aCelX = (mNormalRect.mWidth / 2 + mNormalRect.mX) - mCover->GetCelWidth() / 2;
			g->DrawImageCel(mCover, aCelX, aCelY, aCel);
		}
	}
}

void MenuButtonWidget::MouseEnter()
{
	if (mGlowOn)
		TwistButton::MouseEnter();
}

Rect MenuButtonWidget::GetInsetRect()
{
	return Rect(mX, mY, mWidth, mHeight);
}

void MenuButtonWidget::Flip()
{
	mFlipped = !mFlipped;
	Image* anImage = mImages[mFlipped ? 1 : 0];
	mDownImage = anImage;
	mOverImage = anImage;
	mButtonImage = anImage;
	mNormalRect = anImage->GetCelRect(0);
	mOverRect = anImage->GetCelRect(1);
	mDownRect = anImage->GetCelRect(2);
}

void MenuButtonWidget::SetFlipped(bool theFlipped, bool theNow)
{
	if (mImages[1] == nullptr || mFlipped == theFlipped)
		return;
	if (!theNow)
		mFlip.SetCurve(GTS(0x7da438));
	else
		Flip();
}
