#include "ZoomWipeWidget.h"

#include "Application.h"
#include "DrawUtil.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{

// The scale a picture starts at, how much it goes down for each frame of time, and how many times its size the picture is when the
// scale is one
const double START_SCALE = 60.0;
const double SCALE_SPEED = 0.9;
const double FULL_ZOOM = 8.0;

// The first picture fades in while its scale is over this (from nothing at the start to solid at it), and below the second scale
// it is only the colour (the picture is bigger than the screen by then)
const double FADE_END_SCALE = 25.0;
const double FADE_RATE = 0.02857142857142857;
const double COLOUR_SCALE = 1.5;

}

// 0x462d00
ZoomWipeWidget::ZoomWipeWidget()
{
	mFrom = nullptr;
	mTo = nullptr;
	mScale = START_SCALE;
	mState = STATE_WAITING;
	mCallback = nullptr;
	SetVisible(false);
	mHasTransparencies = true;
	gApp->PlaySample(SOUND_UI_ZOOM_TRANSITION);
}

// 0x462eb0
void ZoomWipeWidget::Setup(Image* theFrom, Image* theTo, const Color& theColor, void (*theCallback)())
{
	mFrom = theFrom;
	mTo = theTo;
	mState = STATE_CLOSING;
	mColor = theColor;
	mCallback = theCallback;
	SetVisible(true);
}

// 0x462e80
void ZoomWipeWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	Resize(0, 0, gApp->mWidth, gApp->mHeight);
}

// 0x462db0: the scale runs down; at one the picture is done: the colour covers the screen, then the callback makes the next screen
// (the wipe is brought over it) and the hole opens, and at the end of that the wipe is over
void ZoomWipeWidget::UpdateF(float theFrac)
{
	Widget::UpdateF(theFrac);
	if (mState != STATE_WAITING)
	{
		MarkDirty();
		mScale = mScale - theFrac * SCALE_SPEED;
		if (mScale <= 1.0)
		{
			if (mState == STATE_CLOSING)
			{
				mState = STATE_COVERED;
				MarkDirtyFull();
				return;
			}

			bool anEnded;
			if (mState == STATE_COVERED)
			{
				mState = STATE_OPENING;
				mScale = START_SCALE;
				mCallback();
				gApp->mWidgetManager->BringToFront(this);
				anEnded = gApp->mQuitting;
			}
			else
				anEnded = mState == STATE_OPENING;

			if (anEnded)
				gApp->EndZoomWipe();
		}
	}
	MarkDirtyFull();
}

// 0x463060
void ZoomWipeWidget::Draw(Graphics* g)
{
	Widget::Draw(g);
	g->SetFastStretch(true);

	if (mState == STATE_CLOSING)
	{
		if (mScale > FADE_END_SCALE)
		{
			double anAlpha = (START_SCALE - mScale) * FADE_RATE;
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, ToInt(anAlpha * 255.0)));
		}
		if (mScale > COLOUR_SCALE)
		{
			DrawZoomedImage(g, mFrom);
			return;
		}
		g->SetColor(mColor);
		g->FillRect(Rect(0, 0, mWidth, mHeight));
	}
	else if (mState == STATE_COVERED)
	{
		g->SetColor(mColor);
		g->FillRect(Rect(0, 0, mWidth, mHeight));
	}
	else if (mState == STATE_OPENING)
	{
		Rect aHole = DrawZoomedImage(g, mTo);
		g->SetColor(mColor);
		g->FillRect(0, 0, gApp->mWidth, aHole.mY);
		g->FillRect(0, aHole.mHeight + aHole.mY, gApp->mWidth, gApp->mHeight - aHole.mHeight - aHole.mY);
		g->FillRect(0, aHole.mY, aHole.mX, aHole.mHeight);
		g->FillRect(aHole.mWidth + aHole.mX, aHole.mY, gApp->mWidth - aHole.mWidth - aHole.mX, aHole.mHeight);
	}
}

// 0x462f30
Rect ZoomWipeWidget::DrawZoomedImage(Graphics* g, Image* theImage)
{
	Graphics aGraphics(*g);
	aGraphics.SetFastStretch(true);

	double aZoom = FULL_ZOOM / mScale;
	double aScaledHeight = theImage->mHeight * aZoom;
	int aHeight = ToInt(aScaledHeight);
	double aScaledWidth = theImage->mWidth * aZoom;
	int aWidth = ToInt(aScaledWidth);
	int aY = ToInt((double)(gApp->mHeight / 2) - aScaledHeight * 0.5);
	int aX = ToInt((double)(gApp->mWidth / 2) - aScaledWidth * 0.5);
	aGraphics.DrawImage(theImage, aX, aY, aWidth, aHeight);
	return Rect(aX, aY, aWidth, aHeight);
}
