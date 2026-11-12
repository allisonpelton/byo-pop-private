// Points.cpp: the score text that rises from cleared gems (Sexy::Points, 0x5cf05a-0x5cf1eb).

#include "Points.h"
#include "WinDMApp.h"
#include "Graphics.h"
#include "Font.h"
#include "MemoryImage.h"
#include "Rect.h"

using namespace Sexy;

// 0x5cf1eb: the text is drawn into an image (4 pixels wider and higher than the text) at once; Board::DoPoints sets the
// sizes and the speeds afterwards
Points::Points(WinDMApp* theApp, Font* theFont, const std::string& theText, int theX, int theY, int theLife,
	int theJustify, Color theColor)
{
	mApp = theApp;
	mMouseVisible = false;
	mFont = theFont;
	mText = theText;
	mLife = theLife;

	float aScale = mApp->ScaleNum(1.0f, 0.0f);
	int aWidth = theFont->StringWidth(theText);
	int aX = theX;
	if (theJustify == 0)
		aX = theX - (int)(((double)aWidth / (double)aScale) * 0.5);
	else if (theJustify == 1)
		aX = theX - (int)((double)aWidth / (double)aScale);

	int anAscent = theFont->GetAscent();
	mVelocity = 0.0f;
	Resize(aX, theY - anAscent, aWidth, theFont->GetHeight());

	mImage = new MemoryImage(mApp);
	mImage->SetImageMode(true, true);
	mImage->Create(aWidth + 4, theFont->GetHeight() + 4);
	Graphics g(mImage);
	g.SetColor(theColor);
	g.SetFont(theFont);
	g.DrawString(theText, 2, theFont->GetAscent() + 1);
	mScale = 0.0f;
	mTargetScale = 0.0f;
	mFade = 0.0f;
	mRise = 0.0f;
}

// 0x5cf1cc, 0x5cf177 (the destructor proper: deletes the picture at +0xb4, then Widget's)
Points::~Points()
{
	delete mImage;
}

// 0x5cf05a: the image at the size it has now, centred in the widget, clear for the last 18 updates of its life
void Points::Draw(Graphics* g)
{
	Widget::Draw(g);
	bool aIs3D = mApp->Is3DAccelerated();

	double anAlpha = (double)mLife / 18.0;
	if (1.0 < anAlpha)
		anAlpha = 1.0;
	int aWidth = (int)((double)mImage->mWidth * mScale + 1.0);
	int aHeight = (int)((double)mImage->mHeight * mScale + 1.0);
	aWidth &= ~1;
	aHeight &= ~1;

	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, (int)(anAlpha * 255.0)));
	if (!aIs3D)
		g->SetFastStretch(true);

	Rect aDest((mWidth - aWidth) / 2, (mHeight - aHeight) / 2, aWidth, aHeight);
	Rect aSrc(0, 0, mImage->mWidth, mImage->mHeight);
	g->DrawImage(mImage, aDest, aSrc);

	if (!aIs3D)
		g->SetFastStretch(false);
	g->SetColorizeImages(false);
}
