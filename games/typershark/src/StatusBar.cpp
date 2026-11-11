#include "StatusBar.h"
#include "Board.h"
#include "FadeButton.h"
#include "LessonManager.h"
#include "Player.h"
#include "TooltipWidget.h"
#include "WinWSApp.h"
#include "GameText.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "Image.h"
#include "DDImage.h"
#include "ImageFont.h"
#include "Res.h"

using namespace Sexy;

// 0x448a40: the widget takes no mouse of its own (the buttons are separate widgets); the buttons are made when it joins
// the manager; the hint starts as the "READY!" text
StatusBar::StatusBar(WinWSApp* theApp)
{
	mMouseVisible = false;
	mApp = theApp;
	mQuitButton = NULL;
	mPauseButton = NULL;
	mOptionsButton = NULL;	// port: the original leaves +0x64 unset until AddedToManager
	mTooltip = new TooltipWidget();
	mHint = GT(0x0052fb5c);
}

// 0x442920: the buttons' own image copies (made by AddedToManager) go first, then the buttons and the tooltip; the
// original reads each button's image without testing the button, so the bar has to have joined a manager (port: tested)
StatusBar::~StatusBar()
{
	if (mPauseButton != NULL && mPauseButton->mButtonImage != NULL)
		delete mPauseButton->mButtonImage;
	if (mQuitButton != NULL && mQuitButton->mButtonImage != NULL)
		delete mQuitButton->mButtonImage;
	if (mOptionsButton != NULL && mOptionsButton->mButtonImage != NULL)
		delete mOptionsButton->mButtonImage;
	if (mPauseButton != NULL)
		delete mPauseButton;
	if (mQuitButton != NULL)
		delete mQuitButton;
	if (mOptionsButton != NULL)
		delete mOptionsButton;
	if (mTooltip != NULL)
		delete mTooltip;
}

// 0x450720 slot 1: the hint tooltip (at 15, 415, sized to its text, 500 milliseconds before it shows) joins the manager
// in front, then the quit, options and pause buttons are made in that order. Each has the bar's image behind it copied
// to its button image (the copy is cut at x with y = the bar's own y + 4, as the original does: the same value is the
// button's y in the window), the down and over images of its own and the hand cursor
void StatusBar::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);

	Rect aRect(0, 0, 0, 0);
	mTooltip->Resize(0xf, 0x19f, 1, 1);
	mTooltip->SetTextAndFont(GT(0x005306f8), mApp->mFontRockwell13);
	mTooltip->mShowDelay = 500;
	theWidgetManager->AddWidget(mTooltip);
	theWidgetManager->BringToFront(mTooltip);

	mQuitButton = new FadeButton(QUIT_BUTTON_ID, mApp);
	mQuitButton->Resize(0x229, mY + 4, 0x51, 0x1e);
	mQuitButton->mDownImage = mApp->mImages[IMAGE_STATUSBARX_QUIT_DOWN];
	mQuitButton->mOverImage = mApp->mImages[IMAGE_STATUSBARX_QUIT_OVER];
	aRect.mY = mY + 4;
	aRect.mX = 0x229;
	aRect.mWidth = 0x51;
	aRect.mHeight = 0x1e;
	mQuitButton->mButtonImage = mApp->CopyImage(mApp->mImages[IMAGE_STATUSBARX], aRect);
	mQuitButton->mDoFinger = true;
	theWidgetManager->AddWidget(mQuitButton);

	mOptionsButton = new FadeButton(OPTIONS_BUTTON_ID, mApp);
	mOptionsButton->Resize(0x1d3, mY + 4, 0x51, 0x1e);
	mOptionsButton->mDownImage = mApp->mImages[IMAGE_STATUSBARX_OPTIONS_DOWN];
	mOptionsButton->mOverImage = mApp->mImages[IMAGE_STATUSBARX_OPTIONS_OVER];
	aRect.mY = mY + 4;
	aRect.mX = 0x1d2;
	aRect.mWidth = 0x51;
	aRect.mHeight = 0x1e;
	mOptionsButton->mButtonImage = mApp->CopyImage(mApp->mImages[IMAGE_STATUSBARX], aRect);
	mOptionsButton->mDoFinger = true;
	theWidgetManager->AddWidget(mOptionsButton);

	mPauseButton = new FadeButton(PAUSE_BUTTON_ID, mApp);
	mPauseButton->Resize(0x17c, mY + 4, 0x52, 0x1e);
	aRect.mY = mY + 4;
	aRect.mX = 0x17c;
	aRect.mWidth = 0x52;
	aRect.mHeight = 0x1e;
	mPauseButton->mButtonImage = mApp->CopyImage(mApp->mImages[IMAGE_STATUSBARX], aRect);
	mPauseButton->mOverImage = mApp->mImages[IMAGE_STATUSBARX_PAUSE_OVER];
	mPauseButton->mDownImage = mApp->mImages[IMAGE_STATUSBARX_PAUSE_DOWN];
	mPauseButton->mDoFinger = true;
	theWidgetManager->AddWidget(mPauseButton);
}

// 0x4413e0 slot 2
void StatusBar::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mPauseButton);
	theWidgetManager->RemoveWidget(mQuitButton);
	theWidgetManager->RemoveWidget(mOptionsButton);
	theWidgetManager->RemoveWidget(mTooltip);
}

// 0x441390 slot 3: the buttons stay in front of the bar (pause, quit, options in that order)
void StatusBar::OrderInManagerChanged()
{
	Widget::OrderInManagerChanged();
	mApp->mWidgetManager->PutInfront(mPauseButton, this);
	mApp->mWidgetManager->PutInfront(mQuitButton, this);
	mApp->mWidgetManager->PutInfront(mOptionsButton, this);
}

// 0x441430 slot 4: the buttons follow
void StatusBar::SetVisible(bool isVisible)
{
	Widget::SetVisible(isVisible);
	mPauseButton->SetVisible(isVisible);
	mOptionsButton->SetVisible(isVisible);
	mQuitButton->SetVisible(isVisible);
}

// 0x441470 slot 10: the buttons follow
void StatusBar::SetDisabled(bool isDisabled)
{
	Widget::SetDisabled(isDisabled);
	mPauseButton->SetDisabled(isDisabled);
	mOptionsButton->SetDisabled(isDisabled);
	mQuitButton->SetDisabled(isDisabled);
}

// 0x448b00 slot 17: once the zapper is charged, every 1500th update (0x5dc, counted by the widget's mUpdateCnt) the hint
// changes: "READY!" becomes "PRESS ENTER TO FIRE!" unless the diver is in state 12, 9, 5, 6 or 7 or the level is a bonus
// level, anything else goes back to "READY!". The bar is repainted while the board is running and its HUD is active
void StatusBar::Update()
{
	Widget::Update();

	int aState = mApp->mBoard->mPlayer->mState;		// 0x412890 is the board's +0xd68
	if (1.0 <= mApp->mBoard->mPlayer->mEmpCharge && mUpdateCnt % 0x5dc == 0)
	{
		// 0x43fbb0 is the app's IsBonusLevel: false while there is no lesson manager
		if (mHint == GT(0x0052fb5c) && aState != 0xc && aState != 9 && aState != 5 && aState != 6 && aState != 7 &&
			!(mApp->mLessonManager != NULL && mApp->mLessonManager->IsBonusLevel()))
			mHint = GT(0x0052fb64);
		else
			mHint = GT(0x0052fb5c);
	}

	if (!mApp->mBoard->mPaused && mApp->mBoard->IsHudActive())
		MarkDirty();
}

// 0x448c00 slot 16: the charge meter (228 pixels wide at x = 0x75, y = 8: the charged part from the meter image, the rest
// dark grey), the bar image over it, then in half-transparent white the hint (or "CHARGING" while the zapper is not
// charged) at y = 0x1a. The hint is not drawn while the board's zapper timer is paused
void StatusBar::Draw(Graphics* g)
{
	int aMeter = (int)(mApp->mBoard->mPlayer->mEmpCharge * 228.0);
	Image* aMeterImage = mApp->mImages[IMAGE_STATUSBARMETER];
	g->DrawImage(aMeterImage, 0x75, 8, Rect(0, 0, aMeter, aMeterImage->mHeight));
	g->SetColor(Color(0x2b, 0x2b, 0x2b));
	g->FillRect(aMeter + 0x75, 8, 0xe4 - aMeter, mApp->mImages[IMAGE_STATUSBARMETER]->mHeight);
	g->DrawImage(mApp->mImages[IMAGE_STATUSBARX], 0, 0);
	g->SetColor(Color(0xff, 0xff, 0xff, 0x80));
	g->SetFont(mApp->mFontMyriadCondensed15);

	if (1.0 <= mApp->mBoard->mPlayer->mEmpCharge || mApp->mBoard->IsEmpTimerPaused())
	{
		if (!mApp->mBoard->IsEmpTimerPaused())
		{
			int aX;
			if (mHint == GT(0x0052fb5c))
				aX = 0xcc;
			else
				aX = 0x99;
			g->DrawString(mHint, aX, 0x1a);
		}
		return;
	}

	g->DrawString(GT(0x0052fb7c), 0xc1, 0x1a);
}

// 0x443eb0 slot 23: the hint tooltip starts waiting when it has a text, unless the board is paused
void StatusBar::MouseEnter()
{
	Widget::MouseEnter();
	if (mTooltip->mText != "")
		mTooltip->SetArmed(!mApp->mBoard->mPaused);
}

// 0x443f80 slot 24
void StatusBar::MouseLeave()
{
	Widget::MouseLeave();
	if (mTooltip->mText != "")
		mTooltip->SetArmed(false);
}

// 0x441f10: no tooltip is waiting or showing (the three buttons', then the hint's, which is also made invisible and
// sent to the back), and the bar is repainted
void StatusBar::HideButtons()
{
	mPauseButton->mTooltip->SetArmed(false);
	mOptionsButton->mTooltip->SetArmed(false);
	mQuitButton->mTooltip->SetArmed(false);
	mTooltip->SetArmed(false);
	mTooltip->SetVisible(false);
	mApp->mWidgetManager->BringToBack(mTooltip);
	MarkDirty();
}

// 0x441f80: the hint tooltip waits (or stops), is shown or hidden, and goes to the front of the manager (or the back)
void StatusBar::ShowButton(bool on)
{
	mTooltip->SetArmed(on);
	mTooltip->SetVisible(on);
	if (on)
	{
		mApp->mWidgetManager->BringToFront(mTooltip);
		MarkDirty();
		return;
	}
	mApp->mWidgetManager->BringToBack(mTooltip);
	MarkDirty();
}
