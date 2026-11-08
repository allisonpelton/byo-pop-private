#include "TooltipWidget.h"
#include "WSTimer.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "Font.h"

using namespace Sexy;

// 0x443010
TooltipWidget::TooltipWidget()
{
	mVisible = false;
	mTimer = new WSTimer();
	mTimer->SetDisabled(true);
	mShowDelay = 1000;
	mHideDelay = 2000;
	mTextColor = Color(0, 0, 0);
	mBackColor = Color(255, 255, 225);
	mArmed = true;
	mTipOn = false;
	mShown = false;
	mFont = NULL;
}

// 0x443150: the original frees mTimer through its deleting destructor, then the text
TooltipWidget::~TooltipWidget()
{
	delete mTimer;
}

// 0x441600 slot 1: no Widget::AddedToManager (the port's AddWidget has set mWidgetManager already)
void TooltipWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	theWidgetManager->AddWidget(mTimer);
}

// 0x441620 slot 2
void TooltipWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	theWidgetManager->RemoveWidget(mTimer);
}

// 0x441640: arming restarts the timer and makes the tooltip a live, visible widget; disarming pauses the timer and
// hides and disables it
void TooltipWidget::SetArmed(bool on)
{
	if (on)
	{
		mTimer->SetDisabled(false);
		mTimer->Reset();
		mShown = false;
		mDisabled = false;
		mTipOn = false;
		mArmed = true;
		mVisible = true;
		return;
	}
	mTimer->SetDisabled(true);
	mArmed = false;
	mVisible = false;
	mTipOn = false;
	mDisabled = true;
}

// added for W1
// 0x449870: the text and the font are set, and the tooltip is resized to the text: the text's width plus 8 by the font's
// ascent plus 6, at its own x and y (the original passes the text by value and calls Widget's Resize directly)
void TooltipWidget::SetTextAndFont(SexyString theText, Font* theFont)
{
	mText = theText;
	mFont = theFont;
	int aWidth = mFont->StringWidth(mText);
	int aHeight = mFont->GetAscent();
	Rect aRect(mX, mY, aWidth + 8, aHeight + 6);
	Widget::Resize(aRect);
}

// 0x4431d0 slot 16 (the leading call of the export is the empty Widget::Draw)
void TooltipWidget::Draw(Graphics* g)
{
	if (mText != "" && mTipOn && mVisible && !mDisabled)
	{
		g->SetFont(mFont);
		g->SetColor(Color(0, 0, 0));
		g->FillRect(Rect(0, 0, mWidth, mHeight));
		g->SetColor(mBackColor);
		g->FillRect(Rect(1, 1, mWidth - 2, mHeight - 2));
		g->SetColor(mTextColor);
		g->DrawString(mText, 3, mFont->GetAscent());
	}
}

// 0x4416a0 slot 17: after mShowDelay of resting time the tip appears (mTimer restarts), after mHideDelay it
// disappears and the timer is paused; a visible tip is repainted every update
void TooltipWidget::Update()
{
	Widget::Update();

	if (mArmed && !mShown && !mTipOn && mTimer->Elapsed() >= mShowDelay)
	{
		mTipOn = true;
		mShown = true;
		mTimer->Reset();
		MarkDirty();
		return;
	}

	if (!mTipOn)
		return;

	if (mShown && mTimer->Elapsed() >= mHideDelay)
	{
		mTipOn = false;
		mShown = false;
		mTimer->SetDisabled(true);
	}

	if (mTipOn)
		MarkDirty();
}
