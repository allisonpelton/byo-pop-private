#include "FadeButton.h"
#include "TooltipWidget.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "Image.h"
#include "Font.h"

using namespace Sexy;

// 0x443440
FadeButton::FadeButton(int theId, ButtonListener* theListener) :
	ButtonWidget(theId, theListener)
{
	mAlpha = 255;
	mFadeStep = 27;
	mFadeInterval = 1;
	mReleasedOver = false;	// port: the original leaves +0xe4 unset until the first MouseEnter or MouseUp
	mFadeOnEnter = false;
	mFadeOnLeave = false;
	mFadeOnDown = false;
	mFadeOnUp = false;
	mTooltipOn = true;
	mTooltip = new TooltipWidget();
}

// 0x443540: the tooltip is deleted before the ButtonWidget part
FadeButton::~FadeButton()
{
	delete mTooltip;
}

// 0x440e70 slot 1
void FadeButton::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mTooltip);
	theWidgetManager->BringToFront(mTooltip);
}

// 0x440eb0 slot 2
void FadeButton::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mTooltip);
}

// 0x440ee0 slot 3: the tooltip stays above its button
void FadeButton::OrderInManagerChanged()
{
	mWidgetManager->BringToFront(mTooltip);
}

// 0x441d40 slot 10
void FadeButton::SetDisabled(bool isDisabled)
{
	ButtonWidget::SetDisabled(isDisabled);
	mTooltip->SetArmed(false);
}

// the end of 0x441c10 and 0x441ca0: the tooltip sits centred on the button's x, a font height and 10 pixels
// above its top, sized to its text
void FadeButton::PositionTooltip()
{
	Font* aFont = mTooltip->mFont;
	if (aFont != NULL)
	{
		int aWidth = aFont->StringWidth(mTooltip->mText) + 8;
		int aHeight = aFont->GetAscent() + 6;
		int aFontHeight = aFont->GetHeight();
		Rect aRect;
		aRect.mX = mX - (mWidth >> 1);
		aRect.mY = (mY - aFontHeight) - 10;
		aRect.mWidth = aWidth;
		aRect.mHeight = aHeight;
		mTooltip->Resize(aRect);
	}
}

// 0x441c10 slot 12
void FadeButton::Resize(const Rect& theRect)
{
	Widget::Resize(theRect);
	PositionTooltip();
}

// 0x441ca0 slot 13
void FadeButton::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	PositionTooltip();
}

// 0x446b10 slot 16: ButtonWidget::Draw's layout, with the fading image drawn over the one below it
void FadeButton::Draw(Graphics* g)
{
	if (mBtnNoDraw)
		return;

	// 0x446b42: the original makes a SansSerif 10 SysFont here when it has no font and a label; port: no SysFont

	bool isDown = mIsDown && mIsOver && !mDisabled;
	isDown ^= mInverted;

	int aFontX = 0;
	int aFontY = 0;
	if (mFont != NULL)
	{
		aFontX = (mWidth - mFont->StringWidth(mLabel)) / 2;
		aFontY = (mHeight - mFont->GetAscent() / 6 + mFont->GetAscent() - 1) / 2;
	}

	g->SetFont(mFont);

	if (mButtonImage == NULL && mDownImage == NULL)
	{
		// no images: the 3D frame in fixed colours (as the framework's ButtonWidget does in 2004)
		g->SetColor(Color(0xd4, 0xd4, 0xd4));
		g->FillRect(0, 0, mWidth, mHeight);
		if (isDown)
		{
			g->SetColor(Color(0, 0, 0));
			g->FillRect(0, 0, mWidth - 1, 1);
			g->FillRect(0, 0, 1, mHeight - 1);
			g->SetColor(Color(0xff, 0xff, 0xff));
			g->FillRect(0, mHeight - 1, mWidth, 1);
			g->FillRect(mWidth - 1, 0, 1, mHeight);
			g->SetColor(Color(0x84, 0x84, 0x84));
			g->FillRect(1, 1, mWidth - 3, 1);
			g->FillRect(1, 1, 1, mHeight - 3);
			g->SetColor(Color(0, 0, 0));
			g->DrawString(mLabel, aFontX + 1, aFontY + 1);
		}
		else
		{
			g->SetColor(Color(0xff, 0xff, 0xff));
			g->FillRect(0, 0, mWidth - 1, 1);
			g->FillRect(0, 0, 1, mHeight - 1);
			g->SetColor(Color(0, 0, 0));
			g->FillRect(0, mHeight - 1, mWidth, 1);
			g->FillRect(mWidth - 1, 0, 1, mHeight);
			g->SetColor(Color(0x84, 0x84, 0x84));
			g->FillRect(1, mHeight - 2, mWidth - 2, 1);
			g->FillRect(mWidth - 2, 1, 1, mHeight - 2);
			g->SetColor(Color(0, 0, 0));
			g->DrawString(mLabel, aFontX, aFontY);
		}
		return;
	}

	if (!isDown)
	{
		if (mDisabled && mDisabledImage != NULL)
			g->DrawImage(mDisabledImage, 0, 0);
		else if ((mIsOver || mIsDown) && mOverImage != NULL)
		{
			if (mAlpha == 0xff)
				g->DrawImage(mOverImage, 0, 0);
			else
			{
				// 0x446ec8: the down image underneath after a mouse-up over the button, or while held
				if (mReleasedOver || mIsDown)
					g->DrawImage(mDownImage, 0, 0);
				g->SetColorizeImages(true);
				g->SetColor(Color(0xff, 0xff, 0xff, mAlpha));
				g->DrawImage(mOverImage, 0, 0);
				g->SetColorizeImages(false);
			}
		}
		else if (mButtonImage != NULL)
		{
			if (mAlpha == 0xff)
				g->DrawImage(mButtonImage, 0, 0);
			else
			{
				g->DrawImage(mOverImage, 0, 0);
				g->SetColorizeImages(true);
				g->SetColor(Color(0xff, 0xff, 0xff, mAlpha));
				g->DrawImage(mButtonImage, 0, 0);
				g->SetColorizeImages(false);
			}
		}
		g->SetColor(mColors[COLOR_LABEL]);
		g->DrawString(mLabel, aFontX, aFontY);
	}
	else
	{
		if (mDownImage != NULL)
		{
			if (mAlpha == 0xff)
				g->DrawImage(mDownImage, 0, 0);
			else
			{
				g->DrawImage(mOverImage, 0, 0);
				g->SetColorizeImages(true);
				g->SetColor(Color(0xff, 0xff, 0xff, mAlpha));
				g->DrawImage(mDownImage, 0, 0);
				g->SetColorizeImages(false);
			}
		}
		else
		{
			if (mAlpha != 0xff)
			{
				g->SetColorizeImages(true);
				g->SetColor(Color(0xff, 0xff, 0xff, mAlpha));
				g->DrawImage(mButtonImage, 0, 0);
				g->SetColorizeImages(false);
			}
			else
				g->DrawImage(mButtonImage, 0, 0);
		}
		g->SetColor(mColors[COLOR_LABEL]);
		g->DrawString(mLabel, aFontX + 1, aFontY + 1);
	}
}

// 0x440d60 slot 17: each of the three conditions below raises mAlpha by mFadeStep every mFadeInterval updates
void FadeButton::Update()
{
	ButtonWidget::Update();

	if (mIsOver || (mFadeOnLeave && 0 < mAlpha && mAlpha < 0xff))
		MarkDirtyFull();

	if (mDisabled)
		return;

	if (mIsOver && !mIsDown && (mFadeOnEnter || mFadeOnUp))
	{
		if (mUpdateCnt % mFadeInterval != 0)
			return;
		mAlpha += mFadeStep;
	}
	else if (mIsDown && mFadeOnDown)
	{
		if (mUpdateCnt % mFadeInterval != 0)
			return;
		mAlpha += mFadeStep;
	}
	else if (!mIsOver && mFadeOnLeave)
	{
		if (mUpdateCnt % mFadeInterval != 0)
			return;
		mAlpha += mFadeStep;
	}
	else
		return;

	if (mAlpha > 0xff)
		mAlpha = 0xff;
}

// 0x443c50 slot 23
void FadeButton::MouseEnter()
{
	ButtonWidget::MouseEnter();
	mAlpha = mFadeOnEnter ? 0 : 255;
	if (mTooltip->mText != "")
		mTooltip->SetArmed(mTooltipOn);
	mReleasedOver = false;
	MarkDirty();
}

// 0x443d40 slot 24
void FadeButton::MouseLeave()
{
	ButtonWidget::MouseLeave();
	mAlpha = mFadeOnLeave ? 0 : 255;
	if (mTooltip->mText != "")
		mTooltip->SetArmed(false);
	MarkDirty();
}

// 0x440cc0 slot 27 (the 2004 MouseDown(x, y, clickCount)): the fade restarts from 0 when it runs while the button is
// held, else the over image is complete
void FadeButton::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	ButtonWidget::MouseDown(x, y, theBtnNum, theClickCount);
	mAlpha = mFadeOnDown ? 0 : 255;
	MarkDirty();
}

// 0x440d10 slot 30 (the 2004 MouseUp(x, y))
void FadeButton::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
	ButtonWidget::MouseUp(x, y, theBtnNum, theClickCount);
	mAlpha = mFadeOnUp ? 0 : 255;
	if (mIsOver)
		mReleasedOver = true;
	MarkDirty();
}
