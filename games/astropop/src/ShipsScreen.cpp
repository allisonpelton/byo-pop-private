// ShipsScreen: the ships that fly up the main menu (see ShipsScreen.h).

#include "ShipsScreen.h"
#include "AstroPopApp.h"
#include "Graphics.h"
#include "Image.h"
#include "MainMenu.h"
#include "Res.h"
#include "SmallObjects.h"

using namespace Sexy;

// 0x434ac0: the word is the picture of the "go" that announces a level
ShipsScreen::ShipsScreen(MainMenu* theMenu)
{
	mHue = 0;
	mY = 0x1e0;
	mGlow = 0;
	mGlowStep = 0x10;
	mMenu = theMenu;
	mGo = new LevelUpText(IMAGE_GS_GO, 0);
}

// 0x434b60 (the body of the screen's deleting destructor): the lettering of "go" is the screen's own
ShipsScreen::~ShipsScreen()
{
	delete mGo;
}

namespace
{

// 0x4b83f0: a frame of a strip of pictures: down the rows when it has more than one, along the columns when it has not
void DrawFrame(Graphics* g, Image* theImage, int theX, int theY, int theFrame)
{
	if (theImage->mNumRows > 1)
		g->DrawImageCel(theImage, theX, theY, 0, theFrame);
	else
		g->DrawImageCel(theImage, theX, theY, theFrame, 0);
}

const int kShipX[4] = {0x8f, 0xf3, 0x157, 0x1bb};
const int kFlameX[4] = {0x8a, 0xee, 0x152, 0x1b6};

void DrawFlames(Graphics* g, int theY)
{
	for (int i = 0; i < 4; i++)
		DrawFrame(g, IMAGE_GS_SHIP1_WARPJET, kFlameX[i], theY - 0x37, 8);
}

}

// 0x434bc0: the background (the colour half transparent, so that the menu shows through it: the original sets the colour's alpha
// to 0x80 after making it), the four ships, their flames (drawn again brighter over themselves, additively) and the word
void ShipsScreen::Draw(Graphics* g)
{
	Color aBackground((int) (gApp->HSLToRGB(mHue, 0xff, 0x80) | 0xff000000));
	aBackground.mAlpha = 0x80;
	g->SetColor(aBackground);
	g->FillRect(0, 0, mWidth, mHeight);
	Image* aShips[4] = {IMAGE_GS_SHIP1, IMAGE_GS_SHIP2, IMAGE_GS_SHIP3, IMAGE_GS_SHIP4};
	for (int i = 0; i < 4; i++)
		DrawFrame(g, aShips[i], kShipX[i], mY, 2);

	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	DrawFlames(g, mY);
	g->SetColorizeImages(true);
	g->SetColor(Color(mGlow, mGlow, mGlow));
	DrawFlames(g, mY);
	g->SetColorizeImages(false);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);

	if (mGo->mActive != 0)
		mGo->DrawAt(g, 0x140, 0xf0);
}

// 0x436270: the ships rise by 4 and the background's colour moves on; they are gone when the last of them (with its flame)
// has left the top, and the menu then takes its screen away
void ShipsScreen::Update()
{
	Widget::Update();
	mHue = (mHue + 4) & 0xff;
	mY -= 4;
	mGlow += mGlowStep;
	if (mGlow < 0x100)
	{
		if (mGlow < 0)
		{
			mGlow = 0;
			mGlowStep = -mGlowStep;
		}
	}
	else
	{
		mGlow = 0xff;
		mGlowStep = -mGlowStep;
	}

	if (mY <= -(IMAGE_GS_SHIP1_WARPJET->GetHeight() + IMAGE_GS_SHIP1->GetHeight()))
	{
		mMenu->CloseGoScreen();
		return;
	}
	if (mGo->mActive != 0)
		mGo->Update();
	MarkDirty();
}
