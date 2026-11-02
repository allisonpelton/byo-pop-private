// Sexy::TwistCheckbox (TwistCheckbox.h).
#include "TwistCheckbox.h"

#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "WinTwistApp.h"
#include "X86Math.h"

using namespace Sexy;

TwistCheckbox::TwistCheckbox(Image* theUncheckedImage, Image* theCheckedImage, int theId,
	CheckboxListener* theListener) :
	Checkbox(theUncheckedImage != nullptr ? theUncheckedImage : IMAGE_MAINMENU_RADIOBUTTONCUSTOMCURSORS,
		theCheckedImage != nullptr ? theCheckedImage : IMAGE_MAINMENU_RADIOBUTTONCUSTOMCURSORS, theId, theListener)
{
	mWidth = mCheckedImage->GetCelWidth();
	mHeight = mCheckedImage->GetCelHeight();
	if (mCheckedImage == mUncheckedImage) {
		mCheckedRect = mCheckedImage->GetCelRect(0);
		mUncheckedRect = mCheckedImage->GetCelRect(1);
	}
	mClip = false;
}

TwistCheckbox::~TwistCheckbox()
{
}

void TwistCheckbox::Draw(Graphics* g)
{
	g->SetColorizeImages(true);
	g->SetColor(Color::White);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	if (!mChecked)
		g->DrawImage(mUncheckedImage, 0, 0, mUncheckedRect);
	else
		g->DrawImage(mCheckedImage, 0, 0, mCheckedRect);

	int aPasses = mIsOver ? 1 : 0;
	if (!mIsDown)
		aPasses++;
	g->SetColor(Color(255, 255, 255, 128));
	while (0 < aPasses) {
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		if (!mChecked)
			g->DrawImage(mUncheckedImage, 0, 0, mUncheckedRect);
		else
			g->DrawImage(mCheckedImage, 0, 0, mCheckedRect);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		aPasses--;
	}

	int aX = Ftol((double)mCheckedRect.mWidth * 1.25);
	g->SetColor(Color::White);
	Font* aFont = FONT_DIALOG_CHECKBOX;
	g->SetFont(aFont);
	int aCentre = mHeight / 2 + (aFont->GetHeight() - aFont->GetAscent()) / 2;
	int aY = Ftol((double)gApp->mScale * 10.0 + (double)aCentre);
	g->DrawString(mLabel, aX, aY);
}
