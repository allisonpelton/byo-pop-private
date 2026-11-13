#include "TrophyPageButton.h"

#include <string>

#include "Application.h"
#include "Creature.h"
#include "DrawUtil.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"

using namespace Sexy;

// The names of the trophies, and what each is for (TrophyText.inc is made by tools/trophytext.py from the exe; they are the game's own
// text, so the file is not kept in the repository)
#include "TrophyText.inc"

namespace
{

// How fast a page's time runs, and how fast its button gets ready, for each frame of time
const double TIME_SPEED = 0.013;
const double CLICK_SPEED = 0.01;

// The blue of the pages
const int BLUE_RED = 0x2e;
const int BLUE_GREEN = 0x92;
const int BLUE_BLUE = 0xf0;

// How fast the light turns with the page's time
const double LIGHT_TURN_RATE = 0.75;

// Where the name and the description of the trophy are, and where the button stands (it is a third of the way up from the
// bottom, and comes in from this far below)
const int NAME_Y = 100;
const int DESCRIPTION_Y = 0x1ea;
const int BUTTON_Y_OFFSET = 0x140;
const double BUTTON_SLIDE = 300.0;

// How much the button turns on its way in: three half turns
const double BUTTON_TURN = 9.42477796076938;

}

const char* Sexy::GetTrophyName(int theTrophy)
{
	return TROPHY_NAMES[theTrophy];
}

// port: the description tables are built from the exe by tools/trophytext.py
const char* Sexy::GetTrophyDescription(int theTrophy)
{
	return TROPHY_DESCRIPTIONS[theTrophy];
}

// 0x45f5e0, 0x4600a0
TrophyPageButton::TrophyPageButton(int theId, ButtonListener* theListener) :
	DialogButton(nullptr, theId, theListener)
{
	mTrophy = -1;
	mTime = 0.0;
	mClickProgress = 0.0;
}

// 0x45ffc0: the time runs on and, once it is past one, the page no longer needs what is behind it drawn first; the button gets ready
void TrophyPageButton::UpdateF(float theFrac)
{
	Widget::UpdateF(theFrac);

	mTime = theFrac * TIME_SPEED + mTime;
	if (mTime > 1.0 && mHasAlpha)
		mHasAlpha = false;

	if (mClickProgress < 1.0)
	{
		mClickProgress = theFrac * CLICK_SPEED + mClickProgress;
		if (!(mClickProgress < 1.0))
			mClickProgress = 1.0;
	}

	MarkDirty();
}

// 0x460120
void TrophyPageButton::DrawTrophyInfo(Graphics* g, int theOffsetX, int theOffsetY)
{
	g->SetFont(FONT_PTBANANASPLIT27);
	std::string aName = TROPHY_NAMES[mTrophy];
	g->DrawString(aName, gApp->mWidth / 2 - g->StringWidth(aName) / 2, NAME_Y);

	g->SetFont(FONT_EUROFURENCE14BOLD);
	std::string aDescription = TROPHY_DESCRIPTIONS[mTrophy];
	g->DrawString(aDescription, gApp->mWidth / 2 - g->StringWidth(aDescription) / 2, DESCRIPTION_Y);

	double aEase = Ease(mClickProgress, 0.5, 0.5);
	double aTurn = (1.0 - aEase) * BUTTON_TURN;
	int aButtonY = ToInt((double) (gApp->mHeight * 5 / 6 + BUTTON_Y_OFFSET) - aEase * BUTTON_SLIDE);
	g->DrawImageRotated(IMAGE_CONTINUE_BUTTON, gApp->mWidth / 2 - IMAGE_CONTINUE_BUTTON->mWidth / 2 + theOffsetX, aButtonY + theOffsetY, aTurn);
}

// 0x45fe60
void Sexy::DrawTrophyBackdrop(Graphics* g, double theTime, double theFade, bool theBlend)
{
	if (!theBlend)
		g->SetColor(Color(BLUE_RED, BLUE_GREEN, BLUE_BLUE));
	else
		g->SetColor(Color(BLUE_RED, BLUE_GREEN, BLUE_BLUE, ToInt(theFade * 255.0)));
	g->FillRect(0, 0, gApp->mWidth, gApp->mHeight);

	if (theTime > 1.0)
	{
		double aFade = theTime - 1.0;
		if (aFade < 1.0)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, ToInt(aFade * 255.0)));
		}

		int aHalfHeight = IMAGE_DIVINELIGHT->mHeight / 2;
		int aHalfWidth = IMAGE_DIVINELIGHT->mWidth / 2;
		g->DrawImageRotated(IMAGE_DIVINELIGHT, gApp->mWidth / 2 - aHalfWidth, gApp->mHeight / 2 - aHalfHeight, theTime * LIGHT_TURN_RATE,
			aHalfWidth, aHalfHeight, nullptr);
		g->SetColorizeImages(false);
	}
}
