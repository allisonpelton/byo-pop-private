#include "XWidget.h"
#include "TooltipWidget.h"
#include "MapScreen.h"
#include "WidgetManager.h"
#include "SexyAppBase.h"
#include "Graphics.h"
#include "Image.h"
#include "Font.h"

using namespace Sexy;

// 0x4435a0
XWidget::XWidget(SexyAppBase* theApp, Widget* theOwner, Image* theImage, char theLetter)
{
	mApp = theApp;
	mImage = theImage;
	mLetter = theLetter;
	mOwner = theOwner;
	mAlpha = 255;
	mHasAlpha = true;
	mFadingUp = false;
	mLevel = 0;	// port: the original leaves +0x68 for the creating screen to set
	mTooltip = new TooltipWidget();
	mTooltip->mShowDelay = 0;
	mTooltip->mHideDelay = 90000;
	mTooltip->SetArmed(false);
}

// 0x443680: the tooltip, then the Widget part
XWidget::~XWidget()
{
	delete mTooltip;
}

// 0x440f00 slot 1
void XWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	mDoFinger = true;
	theWidgetManager->AddWidget(mTooltip);
	theWidgetManager->BringToFront(mTooltip);
}

// 0x440f30 slot 2
void XWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mTooltip);
}

// 0x441d70 slot 13: the tooltip goes centred on the widget's x, a font height and 4 pixels above its top, sized to
// its text, and is pushed back inside the screen's width
void XWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);

	Font* aFont = mTooltip->mFont;
	if (aFont != NULL)
	{
		int aWidth = aFont->StringWidth(mTooltip->mText) + 8;
		int aHeight = aFont->GetAscent() + 6;
		int aFontHeight = aFont->GetHeight();
		Rect aRect;
		aRect.mX = mX - (mWidth >> 1);
		aRect.mY = (mY - aFontHeight) - 4;
		aRect.mWidth = aWidth;
		aRect.mHeight = aHeight;
		mTooltip->Resize(aRect);
	}

	TooltipWidget* aTip = mTooltip;
	if (aTip->mX + aTip->mWidth >= mApp->mWidth)
		aTip->mX = mApp->mWidth - aTip->mWidth;
	else if (aTip->mX < 0)
		aTip->mX = 1;
}

// 0x440fb0 slot 16
void XWidget::Draw(Graphics* g)
{
	if (mVisible && mAlpha > 0)
	{
		if (mAlpha < 0xff)
		{
			g->SetColor(Color(0xff, 0xff, 0xff, mAlpha));
			g->SetColorizeImages(true);
		}
		g->DrawImage(mImage, 0, 0);
		if (mAlpha < 0xff)
			g->SetColorizeImages(false);
	}
}

// 0x440f60 slot 17: no Widget::Update; the alpha swings between 150 and 255 while visible
void XWidget::Update()
{
	if (mVisible)
	{
		if (mFadingUp)
		{
			mAlpha += 2;
			if (mAlpha > 0xff)
			{
				mAlpha = 0xff;
				mFadingUp = false;
			}
		}
		else
		{
			mAlpha -= 2;
			if (mAlpha < 0x96)
				mFadingUp = true;
		}
		MarkDirtyFull();
	}
}

// 0x441e30 slot 23: the hand cursor, and the tooltip starts waiting (no Widget::MouseEnter)
void XWidget::MouseEnter()
{
	mApp->SetCursor(1);
	mTooltip->SetArmed(true);
}

// 0x441e50 slot 24
void XWidget::MouseLeave()
{
	mApp->SetCursor(0);
	mTooltip->SetArmed(false);
}

// added for W1
// 0x458cb0 slot 27 (no Widget::MouseDown): the owner is the map screen. A click on an X is noted (mLevelChosen); if no level
// has been chosen yet, this X stops fading (alpha 255), the owner's "continue" link shows, the owner takes this X's level
// and letter as the chosen ones and starts the level (a method of the map screen, written by its job: it takes the other
// X widgets away and sets up the level's route)
void XWidget::MouseDown(int x, int y, int theClickCount)
{
	MapScreen* aMap = (MapScreen*)mOwner;
	aMap->mLevelChosen = true;
	if (aMap->mLevel == -1)
	{
		mAlpha = 0xff;
		aMap->mContinueLink->SetVisible(true);
		aMap->mLevel = mLevel;
		aMap->mChosenLetter = mLetter;
		aMap->OnLevelChosen();	// the map screen's 0x4538e0 (with its helper 0x4537b0)
	}
}
