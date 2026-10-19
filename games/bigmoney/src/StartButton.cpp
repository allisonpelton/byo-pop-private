#include "StartButton.h"
#include "WinBMApp.h"
#include "Res.h"
#include "Graphics.h"
#include "Image.h"

using namespace Sexy;

// 0x419260
StartButton::StartButton(WinBMApp* theApp, int theId, ButtonListener* theListener) :
	ButtonWidget(theId, theListener)
{
	mHiliteAlpha = 0.0;
	mArrowFrame = 0;
	mApp = theApp;
	mDoFinger = true;
	mHiliteSpeed = 0.05;
	mUnused130 = 0; // port: left unset by the original (never read)
}

// 0x4192c0
StartButton::~StartButton()
{
}

// 0x4192d0
void StartButton::Update()
{
	ButtonWidget::Update();

	if (!mIsOver && !mIsDown)
	{
		// Idle: the arrows chase every 16 updates and the highlighted text fades out.
		if (mUpdateCnt % 16 == 0)
		{
			mArrowFrame = (mArrowFrame + 1) % 8;
			MarkDirty();
		}

		if (mHiliteAlpha > 0.0)
		{
			mHiliteAlpha -= 0.1;
			MarkDirty();
		}
	}
	else
	{
		if (mArrowFrame != 5)
		{
			mArrowFrame = 5;
			MarkDirty();
		}

		if (mHiliteAlpha < 1.0)
		{
			mUnused130 = 0;
			mHiliteAlpha += 0.1;
			MarkDirty();
		}
	}

	if (mHiliteAlpha < 0.0)
		mHiliteAlpha = 0.0;
	else if (mHiliteAlpha > 1.0)
		mHiliteAlpha = 1.0;
}

// 0x4193f0
void StartButton::Draw(Graphics* g)
{
	int anAlpha = (int)(mHiliteAlpha * 255.0);

	if (IsButtonDown())
	{
		// Pressed: shift by one pixel and repaint the selector background under the button first.
		g->Translate(1, 1);
		g->DrawImage(mApp->mImages[IMAGE_SELECTBACK], 0, 0, Rect(7, 429, 455, 38));
	}

	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, 255 - anAlpha));
	g->DrawImage(mApp->mImages[IMAGE_STARTTEXT], 87, -7, Rect(0, 0, 280, 50));
	g->SetColor(Color(255, 255, 255, anAlpha));
	g->DrawImage(mApp->mImages[IMAGE_STARTTEXT], 87, -7, Rect(0, 50, 280, 50));
	g->SetColorizeImages(false);

	// Frames 0..2 light one arrow on each side, frame 5 lights all three (3, 4, 6 and 7 none).
	for (int i = 0; i < 3; i++)
	{
		Rect aSrcRect = (mArrowFrame == i || mArrowFrame == 5) ? Rect(0, 30, 30, 30) : Rect(0, 0, 30, 30);
		g->DrawImage(mApp->mImages[IMAGE_STARTARROWS], 22*i + 16, 4, aSrcRect);
		g->DrawImage(mApp->mStartArrowsMirrored, mWidth - 22*i - 46, 4, aSrcRect);
	}

	if (IsButtonDown())
		g->Translate(-1, -1);
}
