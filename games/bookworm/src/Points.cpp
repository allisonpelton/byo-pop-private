// Points.cpp: the floating "+N" and bonus texts (0x431950-0x431b50).
// Behaviour spec: re/bookworm/analysis/board_draw.md section 4.

#include "Points.h"
#include "Graphics.h"
#include "Font.h"

using namespace Sexy;

// 0x431a50
// The widget is sized to the text plus 5 px: theY is the baseline, theX is placed by theJustify, then the
// box is kept between x 226 and 638.
Points::Points(BookWormApp* theApp, Font* theFont, const std::string& theText, int theX, int theY, int theLife,
	int theJustify)
{
	mApp = theApp;
	mMouseVisible = false;
	mHasAlpha = true;
	mFont = theFont;
	mText = theText;
	mLife = theLife;

	int aWidth = mFont->StringWidth(theText) + 5;
	if (theJustify == JUSTIFY_CENTER)
		theX -= aWidth / 2;
	else if (theJustify == JUSTIFY_RIGHT)
		theX -= aWidth;

	if (theX + aWidth > MAX_RIGHT)
		theX = MAX_RIGHT - aWidth;
	else if (theX < MIN_X)
		theX = MIN_X;

	int anAscent = mFont->GetAscent();
	mHue = 0;
	Widget::Resize(theX, theY - anAscent, aWidth, mFont->GetHeight());
}

// 0x4319e0 (body), 0x431a20 (scalar deleting destructor, slot 0)
Points::~Points()
{
}

// 0x431950 slot 16
// White text, fading out over the last 10 updates of its life (no lower clamp: mLife > 0 while drawn).
void Points::Draw(Graphics* g)
{
	Widget::Draw(g);

	int anAlpha = (mLife * 255) / FADE_TICKS;
	if (anAlpha > 254)
		anAlpha = 255;

	g->SetColor(Color(255, 255, 255, anAlpha));
	g->SetFont(mFont);
	g->DrawString(mText, 0, g->GetFont()->GetAscent());
}
