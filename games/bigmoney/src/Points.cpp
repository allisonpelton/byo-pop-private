#include "Points.h"
#include "WinBMApp.h"
#include "Graphics.h"
#include "Font.h"

using namespace Sexy;

// 0x417630
// The widget is sized to the text: theY is the baseline, theX is placed by theJustify.
Points::Points(WinBMApp* theApp, Font* theFont, const std::string& theText, int theX, int theY, int theLife, int theJustify)
{
	mMouseVisible = false;
	mApp = theApp;
	mFont = theFont;
	mText = theText;
	mLife = theLife;

	int aWidth = mFont->StringWidth(theText);
	int aX = theX;
	if (theJustify == JUSTIFY_CENTER)
		aX = theX - aWidth/2;
	else if (theJustify == JUSTIFY_RIGHT)
		aX = theX - aWidth;

	int aY = theY - mFont->GetAscent();
	mHue = 0;
	Widget::Resize(aX, aY, aWidth, mFont->GetHeight());
}

// 0x4175c0
Points::~Points()
{
}

// 0x4174f0
void Points::Draw(Graphics* g)
{
	Widget::Draw(g);

	double anAlpha = mLife * 0.05555555555555555; // 1/18, multiplied
	if (anAlpha > 1.0)
		anAlpha = 1.0;

	// Color(int) turns an alpha byte of 0 into 255 (not reachable: mLife > 0 while drawn)
	int aColor = (int)((mApp->HSLToRGB(mHue, 255, 128) & 0xffffff) | ((int)(anAlpha * 255.0) << 24));
	g->SetColor(Color(aColor));
	g->SetFont(mFont);
	g->DrawString(mText, 0, g->GetFont()->GetAscent());
}
