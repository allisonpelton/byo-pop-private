#include "CellButton.h"
#include "ButtonListener.h"
#include "Font.h"
#include "Graphics.h"
#include "Sprite.h"

using namespace Sexy;

// the colours the original's buttons are given (0x5304e8, one colour, zero: black)
static int gButtonColors[][3] = {{0, 0, 0}};

// 0x437520
CellButton::CellButton(int theId, ButtonListener* theButtonListener)
{
	mId = theId;
	mFont = NULL;
	mButtonCell = NULL;
	mOverCell = NULL;
	mDownCell = NULL;
	mDisabledCell = NULL;
	mInverted = false;
	mBtnNoDraw = false;
	mFrameNoDraw = false;
	mButtonListener = theButtonListener;
	SetColors(gButtonColors, 1);
}

// 0x4375c0
CellButton::~CellButton()
{
	delete mFont;
	mFont = NULL;
}

// 0x4373c0
void CellButton::SetFont(Font* theFont)
{
	delete mFont;
	mFont = theFont->Duplicate();
}

// 0x4373e0
void CellButton::SetDisabled(bool isDisabled)
{
	Widget::SetDisabled(isDisabled);
	if (mDisabledCell != NULL)
		MarkDirty();
}

// 0x437410
void CellButton::MouseEnter()
{
	Widget::MouseEnter();
	if (!mIsDown && (mOverCell == NULL))
		return;
	MarkDirtyFull();
}

// 0x437440
void CellButton::MouseLeave()
{
	Widget::MouseLeave();
	if (!mIsDown && (mOverCell == NULL))
		return;
	MarkDirtyFull();
}

// 0x437470
void CellButton::MouseDown(int x, int y, int theClickCount)
{
	Widget::MouseDown(x, y, theClickCount);
	if (!mDisabled)
		mButtonListener->ButtonPress(mId);
	MarkDirtyFull();
}

// 0x4374b0
void CellButton::MouseUp(int x, int y)
{
	Widget::MouseUp(x, y);
	if (mIsOver && !mDisabled)
		mButtonListener->ButtonDepress(mId);
	MarkDirtyFull();
}

// 0x4374f0 (the original does not call Widget::Update)
void CellButton::Update()
{
	if (mIsDown && mIsOver && !mDisabled)
		mButtonListener->ButtonDownTick(mId);
}

// 0x437670
void CellButton::Draw(Graphics* g)
{
	if (mBtnNoDraw)
		return;
	if (mButtonCell != NULL)
		SetDrawGraphics(g);
	// port: no fallback font (the original makes a 10-point "SansSerif" SysFont when a labelled button has none)

	bool isDown = mIsDown && mIsOver && !mDisabled;
	bool inverted = mInverted;
	int aFontX = 0;
	int aFontY = 0;
	if (mFont != NULL)
	{
		aFontX = (mWidth - mFont->StringWidth(mLabel)) / 2;
		aFontY = (mHeight + mFont->GetAscent() - mFont->GetAscent() / 6 - 1) / 2;
	}
	g->SetFont(mFont);

	if ((mButtonCell == NULL) && (mDownCell == NULL))
	{
		g->SetColor(Color(0xd4, 0xd4, 0xd4));
		g->FillRect(0, 0, mWidth, mHeight);
		if ((isDown ^ inverted) == false)
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
			return;
		}
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
	}
	else
	{
		if ((isDown ^ inverted) == false)
		{
			Cell* aCell;
			if ((mDisabled && ((aCell = mDisabledCell) != NULL)) ||
				((mIsOver || mIsDown) && ((aCell = mOverCell) != NULL)) ||
				((aCell = mButtonCell) != NULL))
				aCell->Draw(0, 0);
			g->SetColor(mColors[0]);
			g->DrawString(mLabel, aFontX, aFontY);
			return;
		}
		Cell* aCell = mButtonCell;
		int anOffset;
		if (mDownCell == NULL)
			anOffset = 1;
		else
		{
			anOffset = (mFrameNoDraw != true) ? 1 : 0;
			aCell = mDownCell;
		}
		aCell->Draw(anOffset, anOffset);
		g->SetColor(mColors[0]);
	}
	aFontY++;
	aFontX++;
	g->DrawString(mLabel, aFontX, aFontY);
}
