#include "ArmoryArrowButton.h"
#include "HeavyWeaponApp.h"
#include "ImageFont.h"
#include "Graphics.h"
#include "DDImage.h"
#include "Sprite.h"

using namespace Sexy;

namespace
{
	const int GLOW_SPEED = 12;
	const int GLOW_MAX = 0xff;
}

// 0x4300c0: its mouse area is the picture's whole cel (insets of 4), and it takes the pictures and font of the game's buttons
ArmoryArrowButton::ArmoryArrowButton(HeavyWeaponApp* theApp, ButtonListener* theListener, int theId, int theColumn) :
	ButtonWidget(theId, theListener)
{
	mApp = theApp;
	mDoFinger = true;
	mMouseInsets = Insets(4, 4, 4, 4);
	mGlow = 0;
	mGlowSpeed = 0;
	mColumn = theColumn;
	mRow = theId;
	mButtonImage = theApp->mSprites[IMAGE_BUTTONBRACKET]->mImage;
	mOverImage = theApp->mSprites[IMAGE_BUTTON]->mImage;
	mDownImage = theApp->mSprites[IMAGE_BUTTONGLOW]->mImage;
	SetFont(theApp->mFonts[FONT_NORMAL]);
}

// 0x430260 slot 16: the cel, added at the glow's grey level
void ArmoryArrowButton::Draw(Graphics* g)
{
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColorizeImages(true);
	g->SetColor(Color(mGlow, mGlow, mGlow));
	mApp->mSprites[IMAGE_UPGRADEBTNS]->Draw(g, 0, 0, false, mColumn, mRow, false);
	g->SetColorizeImages(false);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// 0x4301c0 slot 17
void ArmoryArrowButton::Update()
{
	ButtonWidget::Update();
	int aGlow = mGlow;
	mGlow = aGlow + mGlowSpeed;
	if (aGlow + mGlowSpeed > GLOW_MAX)
	{
		mGlow = GLOW_MAX;
		mGlowSpeed = 0;
	}
	if (mGlow < 0)
	{
		mGlow = 0;
		mGlowSpeed = 0;
	}
	MarkDirty();
}

// 0x430220 slot 24
void ArmoryArrowButton::MouseEnter()
{
	ButtonWidget::MouseEnter();
	mGlowSpeed = GLOW_SPEED;
}

// 0x430240 slot 25
void ArmoryArrowButton::MouseLeave()
{
	ButtonWidget::MouseLeave();
	mGlowSpeed = -GLOW_SPEED;
}
