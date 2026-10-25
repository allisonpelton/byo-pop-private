// port(variant): the undo hotspot; see UndoOrb.h. Not part of the original game.

#include "UndoOrb.h"
#include "WinDMApp.h"
#include "Board.h"
#include "Graphics.h"
#include "DDImage.h"
#include "ImageFont.h"
#include "MemoryImage.h"

#include <cmath>

using namespace Sexy;

UndoOrb::UndoOrb(WinDMApp* theApp, int theId) : ButtonWidget(theId, theApp)
{
	mApp = theApp;
	mDrawnState = -1;
	mGlow = NULL;
	mMouseVisible = false;
	mDoFinger = false;
	Resize(ORB_X, ORB_Y, ORB_WIDTH, ORB_HEIGHT);
}

UndoOrb::~UndoOrb()
{
	delete mGlow;
}

// Only a board with undo shows the orb and takes the mouse there; the hand cursor only while an undo is possible
void UndoOrb::Update()
{
	ButtonWidget::Update();
	Board* aBoard = mApp->mBoard;
	bool isShown = (aBoard != NULL) && aBoard->mVariant.mUndo;
	bool canUndo = isShown && aBoard->CanUndo();
	mMouseVisible = isShown;
	mDoFinger = canUndo;
	int aState = isShown ? ((canUndo ? 1 : 0) + (mIsOver ? 2 : 0) + 4 * (aBoard->UndosLeft() + 1)) : -1;
	if (aState != mDrawnState)
	{
		mDrawnState = aState;
		MarkDirty();
	}
}

void UndoOrb::Draw(Graphics* g)
{
	Board* aBoard = mApp->mBoard;
	if ((aBoard == NULL) || !aBoard->mVariant.mUndo)
		return;
	if (aBoard->CanUndo())
	{
		if (mGlow == NULL)
		{
			mGlow = new MemoryImage(mApp);
			mGlow->Create(mWidth, mHeight);
			ulong* aBits = mGlow->GetBits();
			for (int y = 0; y < mHeight; y++)
			{
				for (int x = 0; x < mWidth; x++)
				{
					double aDistance = sqrt((x + 0.5 - (BALL_X - ORB_X)) * (x + 0.5 - (BALL_X - ORB_X)) + (y + 0.5 - (BALL_Y - ORB_Y)) * (y + 0.5 - (BALL_Y - ORB_Y)));
					double anAlpha = (BALL_RADIUS - aDistance) / 3.0;
					anAlpha = (anAlpha < 0.0) ? 0.0 : (anAlpha > 1.0) ? 1.0 : anAlpha;
					aBits[y * mWidth + x] = ((ulong) (anAlpha * 255.0 + 0.5) << 24) | 0xFFFFFF;
				}
			}
			mGlow->BitsChanged();
		}
		// The ball lights up, more under the mouse
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 150, 150, mIsOver ? 200 : 90));
		g->DrawImage(mGlow, 0, 0);
		g->SetColorizeImages(false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
	int aLeft = aBoard->UndosLeft();
	if (aLeft >= 0)
	{
		// Like the level number in the right-hand orb (Board::Draw)
		SexyString aText = StrFormat("%d", aLeft);
		g->SetFont(mApp->mHeaderFont);
		g->SetColor(Color::Black);
		g->DrawString(aText, mWidth / 2 - g->GetFont()->StringWidth(aText) / 2, DIGIT_Y - mY);
	}
}
