#include "BracketButton.h"
#include "HeavyWeaponApp.h"
#include "ImageFont.h"
#include "Font.h"
#include "Graphics.h"
#include "DDImage.h"
#include "Sprite.h"

using namespace Sexy;

namespace
{
	const int LABEL_Y = 0x15;			// the label's y in the button
	const int MOUSE_INSET_X = 0x15;		// the sides of the picture that do not take the mouse
	const int MOUSE_INSET_Y = 6;
}

// 0x41cec0: the bracket, glow and pressed glow are IMAGE_BUTTONBRACKET, IMAGE_BUTTON and IMAGE_BUTTONGLOW
BracketButton::BracketButton(HeavyWeaponApp* theApp, ButtonListener* theListener, int theId) :
	ButtonWidget(theId, theListener)
{
	mApp = theApp;
	mDoFinger = true;
	mButtonImage = theApp->mSprites[IMAGE_BUTTONBRACKET]->mImage;
	mOverImage = theApp->mSprites[IMAGE_BUTTON]->mImage;
	mDownImage = theApp->mSprites[IMAGE_BUTTONGLOW]->mImage;
	mMouseInsets = Insets(MOUSE_INSET_X, MOUSE_INSET_Y, MOUSE_INSET_X, MOUSE_INSET_Y);
	SetFont(mApp->mFonts[FONT_NORMAL]);
}

// 0x41cfb0
void BracketButton::SetTint(const Color& theTint)
{
	mTint = theTint;
}

// 0x41d010 slot 16: the bracket, the tinted glow, the label centred, and the pressed glow when held (the picture
// moves down a pixel while it is)
void BracketButton::Draw(Graphics* g)
{
	g->DrawImageBox(Rect(0, 0, mWidth, mHeight), mButtonImage);
	if (mIsDown)
		g->Translate(0, 1);
	g->SetColorizeImages(true);
	g->SetColor(mTint);
	g->DrawImageBox(Rect(0, 0, mWidth, mHeight), mOverImage);
	int aLabelWidth = mFont->StringWidth(mLabel);
	mFont->DrawString(g, mWidth / 2 - aLabelWidth / 2, LABEL_Y, mLabel, Color(0xff, 0xff, 0xff, 0xff), Rect(0, 0, 0, 0));
	g->SetColor(Color(200, 200, 200, 0xff));
	if (mIsDown)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->DrawImageBox(Rect(0, 0, mWidth, mHeight), mDownImage);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
	g->SetColorizeImages(false);
}
