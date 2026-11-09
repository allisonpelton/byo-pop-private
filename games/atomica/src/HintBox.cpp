// The tutorial boxes and help popups of Atomica Deluxe (see HintBox.h).

#include "HintBox.h"
#include "AtomicaApp.h"
#include "Board.h"
#include "GameText.h"
#include "Graphics.h"
#include "Grid.h"
#include "ImageFont.h"
#include "Support.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{
	// one line of a hint: a colour for the lines that follow, or a text at a height from the box's top
	enum
	{
		HOP_COLOR,
		HOP_TEXT
	};

	struct HintOp
	{
		unsigned char	mKind;
		unsigned char	mR;
		unsigned char	mG;
		unsigned char	mB;
		short			mY;
		unsigned int	mText;		// the exe's address of the text
		unsigned int	mPrefix;	// the text before a lit word: the line starts after its width (0: at the left)
	};

	struct HintLines
	{
		const HintOp*	mOps;
		int				mCount;
	};

#include "HintData.inc"

	// an arrow at a place on the screen: first a black one four pixels down and right, then the coloured one
	struct HintArrow
	{
		int				mSprite;	// the app's mTipGraphics
		int				mX;
		int				mY;
		unsigned char	mR;
		unsigned char	mG;
		unsigned char	mB;
	};

	const HintArrow kArrows0[] = {{0, 0xc4, 0x144, 255, 0, 0}, {4, 0x167, 0xe8, 0, 255, 0}};
	const HintArrow kArrows1[] = {{0, 0xc3, 0x20, 255, 0, 0}};
	const HintArrow kArrows3[] = {{0, 0x156, 0x2d, 255, 0, 0}};
	const HintArrow kArrows4[] = {{1, 0xdc, 0x17e, 255, 0, 0}};
	const HintArrow kArrows5[] = {{1, 0x14c, 0x177, 255, 0, 0}};
	const HintArrow kArrows6[] = {{2, 0x71, 0x8b, 255, 0, 0}};
	const HintArrow kArrowsChrome[] = {{0, 0x158, 0x113, 255, 0, 0}};	// 0xd and 0xe
	const HintArrow kArrowsBomb[] = {{1, 0x158, 0x185, 255, 0, 0}};		// 0x12 and 0x13
	const HintArrow kArrows14[] = {{0, 0x15c, 0x2a, 255, 0, 0}};
}

// 0x42a310: the box's place and size by hint; a tutorial box has the check box, a popup has not. The game pauses.
HintBox::HintBox(AtomicaApp* theApp, Grid* theGrid, int theHint) :
	mApp(theApp),
	mGrid(theGrid),
	mHint(theHint),
	mBoxX(0),
	mBoxY(0),
	mBoxWidth(0),
	mBoxHeight(0),
	mAlpha(0),
	mNever(false),
	mGone(false),
	mHasCheck(true)
{
	mApp->mPauseCount++;
	mHasTransparencies = true;
	Widget::Resize(0, 0, 0x280, 0x1e0);
	mGrid->mHintShown[theHint] = true;
	switch (theHint)
	{
	case 0:
		mBoxX = 0x1e;
		mBoxY = 0x3c;
		mBoxWidth = 0x140;
		mBoxHeight = 0x9b;
		break;
	case 1:
		mBoxX = 0xe1;
		mBoxY = 100;
		mBoxWidth = 0x140;
		mBoxHeight = 0xf0;
		break;
	case 2:
		mBoxX = 0xe1;
		mBoxY = 100;
		mBoxWidth = 0x140;
		mBoxHeight = 0xd2;
		break;
	case 3:
		mBoxX = 0xe1;
		mBoxY = 0x96;
		mBoxWidth = 0x140;
		mBoxHeight = 0xcf;
		break;
	case 4:
		mBoxX = 0xe1;
		mBoxY = 0x50;
		mBoxWidth = 0x140;
		mBoxHeight = 0xd7;
		break;
	case 5:
		mBoxX = 0xe1;
		mBoxY = 0x96;
		mBoxWidth = 0x140;
		mBoxHeight = 0xb9;
		break;
	case 6:
		mBoxX = 0xc3;
		mBoxY = 0x51;
		mBoxWidth = 0x14a;
		mBoxHeight = 0xcd;
		break;
	case 7:
		mBoxX = 0xe1;
		mBoxY = 0x50;
		mBoxWidth = 0x145;
		mBoxHeight = 0xff;
		break;
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
		// beside the element atom that wants to act: on its right, or on its left on the right half of the screen
		if (mGrid->mI4b8 < 0x141)
		{
			mBoxX = mGrid->mI4b8 + 0x14;
		}
		else
		{
			mBoxX = mGrid->mI4b8 - 0x154;
			if (mBoxX < 0)
				mBoxX = 0;
		}
		mBoxY = mGrid->mI4bc - 0x32;
		mBoxWidth = 0x131;
		mBoxHeight = 0x87;
		if (mBoxY < 0)
			mBoxY = 0;
		if (0x159 < mBoxY)
			mBoxY = 0x159;
		break;
	case 0xd:
		mBoxX = 0x2b;
		mBoxY = 0x2f;
		mBoxWidth = 0x136;
		mBoxHeight = 0xdc;
		break;
	case 0xe:
		mBoxX = 0x35;
		mBoxY = 0x43;
		mBoxWidth = 0x13b;
		mBoxHeight = 0xd7;
		break;
	case 0xf:
		mBoxX = 0xe1;
		mBoxY = 100;
		mBoxWidth = 0x131;
		mBoxHeight = 0x9b;
		break;
	case 0x10:
		mBoxX = 0xe1;
		mBoxY = 200;
		mBoxWidth = 0x131;
		mBoxHeight = 0x87;
		break;
	case 0x11:
		mHasCheck = false;
		mBoxX = 0xa3;
		mBoxY = 0xbf;
		mBoxWidth = 0x1c2;
		mBoxHeight = 0xf5;
		break;
	case 0x12:
	case 0x13:
		mHasCheck = false;
		mBoxX = 0xc3;
		mBoxY = 0x78;
		mBoxWidth = 0x1a4;
		mBoxHeight = 0x10e;
		break;
	case 0x14:
		mHasCheck = false;
		mBoxX = 0xe3;
		mBoxY = 0x6e;
		mBoxWidth = 0x159;
		mBoxHeight = 0x10e;
		break;
	case 0x15:
	case 0x16:
		mHasCheck = false;
		mBoxX = 0xe3;
		mBoxY = 0x6e;
		mBoxWidth = 0x140;
		mBoxHeight = 0xb9;
		break;
	case 0x17:
	case 0x19:
		mBoxX = 0xe1;
		mBoxY = 200;
		mBoxWidth = 0x131;
		mBoxHeight = 0xb4;
		break;
	case 0x18:
		mBoxX = 0xe1;
		mBoxY = 100;
		mBoxWidth = 0x131;
		mBoxHeight = 0x109;
		break;
	case 0x1a:
	case 0x1b:
		mBoxX = 0xe1;
		mBoxY = 100;
		mBoxWidth = 0x145;
		mBoxHeight = 0x11d;
		break;
	}
	mContinue = new ButtonWidget(0, this);
	mContinue->mBtnNoDraw = true;
	mContinue->mDoFinger = true;
	mContinue->Resize(mBoxX + mBoxWidth - 0x55, mBoxY + mBoxHeight - 0x1e, 0x46, 0xf);
	mCheck = new ButtonWidget(1, this);
	mCheck->mBtnNoDraw = true;
	mCheck->mDoFinger = true;
	mCheck->Resize(mBoxX + 0xc, mBoxY + mBoxHeight - 0x1f, 0x13, 0x13);
	if (!mHasCheck)
		mCheck->SetVisible(false);
	mApp->mHintBoxes.push_back(this);
	mApp->mWidgetManager->AddWidget(this);
}

// 0x42a0b0 / 0x432c00: the game goes on; a ticked check box keeps the hint away in the games to come
HintBox::~HintBox()
{
	for (size_t i = 0; i < mApp->mHintBoxes.size(); i++)
		if (mApp->mHintBoxes[i] == this)
		{
			mApp->mHintBoxes.erase(mApp->mHintBoxes.begin() + i);
			break;
		}
	if (mContinue != NULL)
	{
		mApp->SafeDeleteWidget(mContinue);
		mApp->SafeDeleteWidget(mCheck);
		mContinue = NULL;
		mCheck = NULL;
	}
	mApp->mPauseCount--;
	if (mApp->mPauseCount < 0)
		mApp->mPauseCount = 0;
	if ((mApp->mBoard != NULL) && (mApp->mBoard->mGrid != NULL) && (mApp->mWidgetManager != NULL))
		mApp->mWidgetManager->SetFocus(mApp->mBoard->mGrid);
	if (mNever)
		mApp->mHintsShown[mHint] = true;
}

// 0x42a1e0: a hint that was ticked away in an earlier game, or with the hints switched off, goes at once
void HintBox::AddedToManager(WidgetManager* theWidgetManager)
{
	if (mApp->mHintsShown[mHint] || !mApp->mShowTips)
		mGone = true;
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mContinue);
	theWidgetManager->AddWidget(mCheck);
	theWidgetManager->BringToFront(this);
	theWidgetManager->BringToFront(mContinue);
	theWidgetManager->BringToFront(mCheck);
	theWidgetManager->AddBaseModal(this);
}

// 0x42a270
void HintBox::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mContinue);
	theWidgetManager->RemoveWidget(mCheck);
	mApp->SafeDeleteWidget(this);
}

// 0x42a180: fades in; goes when the Continue button was pressed (or the main menu came up)
void HintBox::Update()
{
	mAlpha += 8;
	if (mAlpha > 0xff)
		mAlpha = 0xff;
	if (mGone || (mApp->mMenu != NULL))
	{
		mApp->mWidgetManager->RemoveBaseModal(this);
		mApp->mWidgetManager->RemoveWidget(this);
		mGone = false;
	}
}

// 0x42a2e0: the check box ticks and unticks
void HintBox::ButtonPress(int theId)
{
	mApp->mSndCapClick->Play();
	if (theId == 1)
		mNever = !mNever;
}

// 0x42a2c0
void HintBox::ButtonDepress(int theId)
{
	if (theId == 0)
		mGone = true;
}

// 0x42a810
void HintBox::Draw(Graphics* g)
{
	int anAlpha75 = (int) ((double) mAlpha * 0.75);
	int anAlpha78 = (int) ((double) mAlpha * 0.78);
	g->SetColor(Color(0, 0, 0, anAlpha75));
	g->FillRect(mBoxX, mBoxY, mBoxWidth, mBoxHeight);
	g->SetColor(Color(0, 255, 0, anAlpha78));
	g->FillRect(mBoxX + 2, mBoxY + 2, mBoxWidth - 4, 8);
	g->FillRect(mBoxX + 2, mBoxY + mBoxHeight - 10, mBoxWidth - 4, 8);
	g->FillRect(mBoxX + 2, mBoxY + 10, 8, mBoxHeight - 0x14);
	g->FillRect(mBoxX + mBoxWidth - 10, mBoxY + 10, 8, mBoxHeight - 0x14);
	g->SetColor(Color(0, 0x96, 0, anAlpha78));
	g->FillRect(mBoxX + 0xc, mBoxY + 0xc, mBoxWidth - 0x18, 0x11);
	g->SetFont(mApp->mMonitorFontSmall);
	// the Continue button: lit while the mouse is over it and not down
	g->SetColor(Color((mContinue->mIsOver && !mContinue->mIsDown) ? 0x96 : 0, 0x96, 0, anAlpha78));
	g->FillRect(mBoxX + mBoxWidth - 0x55, mBoxY + mBoxHeight - 0x1e, 0x46, 0xf);
	if (mHasCheck)
	{
		g->SetColor(Color(mCheck->mIsOver ? 200 : 0, 200, 0, anAlpha78));
		g->FillRect(mBoxX + 0xc, mBoxY + mBoxHeight - 0x1f, 0x13, 0x13);
	}
	if (mNever)
	{
		g->SetColor(Color(255, 255, 100, mAlpha));
		g->FillRect(mBoxX + 0x10, mBoxY + mBoxHeight - 0x1b, 0xb, 0xb);
	}
	g->SetColor(Color(255, 255, 255, mAlpha));
	g->DrawString(GT(0x4b2bd4), mBoxX + mBoxWidth - 0x52, mBoxY + mBoxHeight - 0x14);
	if (mHasCheck)
	{
		// the words beside the check box are a picture
		g->SetColorizeImages(true);
		g->SetColor(Color(0, 255, 0, mAlpha));
		Sprite* aTip = mApp->mCancelTip;
		if (aTip->mImage != NULL)
			g->DrawImage(aTip->mImage, mBoxX + 0x22, mBoxY + mBoxHeight - 0x1d, aTip->mSrc);
		g->SetColorizeImages(false);
	}

	// the hint's lines
	Font* aFont = mApp->mMonitorFontSmall;
	g->SetFont(aFont);
	int aLeft = mBoxX + 0xe;
	for (int i = 0; i < kHintLines[mHint].mCount; i++)
	{
		const HintOp& anOp = kHintLines[mHint].mOps[i];
		if (anOp.mKind == HOP_COLOR)
		{
			g->SetColor(Color(anOp.mR, anOp.mG, anOp.mB, mAlpha));
			continue;
		}
		int aX = aLeft;
		if (anOp.mPrefix != 0)
			aX += aFont->StringWidth(GT(anOp.mPrefix));
		g->DrawString(GT(anOp.mText), aX, mBoxY + anOp.mY);
	}

	// the arrows
	const HintArrow* anArrows = NULL;
	int aCount = 1;
	HintArrow aGridArrow = {3, 0, 0, 255, 0, 0};
	switch (mHint)
	{
	case 0:
		anArrows = kArrows0;
		aCount = 2;
		break;
	case 1:
		anArrows = kArrows1;
		break;
	case 3:
		anArrows = kArrows3;
		break;
	case 4:
		anArrows = kArrows4;
		break;
	case 5:
		anArrows = kArrows5;
		break;
	case 6:
		anArrows = kArrows6;
		break;
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
		// at the element atom
		aGridArrow.mX = mGrid->mI4b8 - 0x36;
		aGridArrow.mY = mGrid->mI4bc - 0x26;
		anArrows = &aGridArrow;
		break;
	case 0xd:
	case 0xe:
		anArrows = kArrowsChrome;
		break;
	case 0x12:
	case 0x13:
		anArrows = kArrowsBomb;
		break;
	case 0x14:
		anArrows = kArrows14;
		break;
	}
	if (anArrows != NULL)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(0, 0, 0, mAlpha));
		for (int i = 0; i < aCount; i++)
			mApp->mTipGraphics[anArrows[i].mSprite]->Draw(g, anArrows[i].mX + 4, anArrows[i].mY + 4);
		for (int i = 0; i < aCount; i++)
		{
			g->SetColor(Color(anArrows[i].mR, anArrows[i].mG, anArrows[i].mB, mAlpha));
			mApp->mTipGraphics[anArrows[i].mSprite]->Draw(g, anArrows[i].mX, anArrows[i].mY);
		}
		g->SetColorizeImages(false);
	}
}
