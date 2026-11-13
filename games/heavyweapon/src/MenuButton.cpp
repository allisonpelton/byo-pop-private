#include "MenuButton.h"
#include "HeavyWeaponApp.h"
#include "ImageFont.h"
#include "Graphics.h"
#include "DDImage.h"
#include "Sprite.h"

using namespace Sexy;

namespace
{
	const int MOUSE_INSET = 4;		// the mouse area is the picture's whole cel less this on each side
	const int PAN_SCALE = 3;
	const int SCREEN_CENTER_X = 0x140;
}

// 0x4258a0: it takes the pictures and font of the game's buttons; the picture is given afterwards
MenuButton::MenuButton(HeavyWeaponApp* theApp, ButtonListener* theListener, int theId) :
	ButtonWidget(theId, theListener)
{
	mApp = theApp;
	mDoFinger = true;
	mMouseInsets = Insets(MOUSE_INSET, MOUSE_INSET, MOUSE_INSET, MOUSE_INSET);
	mGlow = 0;
	mGlowSpeed = 0;
	mSprite = NULL;
	mButtonImage = theApp->mSprites[IMAGE_BUTTONBRACKET]->mImage;
	mOverImage = theApp->mSprites[IMAGE_BUTTON]->mImage;
	mDownImage = theApp->mSprites[IMAGE_BUTTONGLOW]->mImage;
	SetFont(theApp->mFonts[FONT_NORMAL]);
}

// 0x425ad0 (body 0x425980)
MenuButton::~MenuButton()
{
}

// 0x425a60 slot 16: the picture added over the menu at the glow's grey level
void MenuButton::Draw(Graphics* g)
{
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColorizeImages(true);
	g->SetColor(Color(mGlow, mGlow, mGlow));
	mSprite->Draw(g, 0, 0, false, 0, 0, false);
	g->SetColorizeImages(false);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// 0x425990 slot 17
void MenuButton::Update()
{
	ButtonWidget::Update();
	mGlow += mGlowSpeed;
	if (mGlow > GLOW_MAX)
	{
		mGlow = GLOW_MAX;
		mGlowSpeed = 0;
	}
	if (mGlow < 0)
	{
		mGlow = 0;
		mGlowSpeed = 0;
	}
}

// 0x4259e0 slot 24: a sound panned by where the button is
void MenuButton::MouseEnter()
{
	ButtonWidget::MouseEnter();
	mApp->PlaySample(SOUND_MAPOVER, (mWidth / 2 - SCREEN_CENTER_X + mX) * PAN_SCALE);
	mGlowSpeed = GLOW_SPEED;
}

// 0x425a20 slot 25
void MenuButton::MouseLeave()
{
	ButtonWidget::MouseLeave();
	mApp->PlaySample(SOUND_MAPOVER, (mWidth / 2 - SCREEN_CENTER_X + mX) * PAN_SCALE);
	mGlowSpeed = -GLOW_SPEED;
}
