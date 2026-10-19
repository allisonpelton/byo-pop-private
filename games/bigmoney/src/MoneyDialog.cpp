#include "MoneyDialog.h"
#include "WinBMApp.h"

#include "Graphics.h"
#include "Image.h"

using namespace Sexy;

// 0x414e00
MoneyDialog::MoneyDialog(WinBMApp* theApp, Image* theComponentImage, Image* theButtonComponentImage, int theId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode) :
	Dialog(theComponentImage, theButtonComponentImage, theId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode)
{
	mLightPhase = 0.0;
	mApp = theApp;
}

// 0x414e60
MoneyDialog::~MoneyDialog()
{
}

// 0x414e80: the light chain moves (and the dialog repaints) only while the app has the focus
void MoneyDialog::Update()
{
	Dialog::Update();

	if (!mApp->mHasFocus)
		return;

	double aPhase = mLightPhase + 0.0005;
	mLightPhase = aPhase;
	if (aPhase > 1.0)
		mLightPhase = aPhase - 1.0;
	MarkDirty();
}

// 0x414f00: a black panel, 40 lights running clockwise around it (top edge left to right first), the
// Dialog frame and texts on top, then an additive glow on each light outside two horizontal bands
void MoneyDialog::Draw(Graphics* g)
{
	int aLightX[NUM_LIGHTS] = {};
	int aLightY[NUM_LIGHTS] = {};

	g->SetColor(Color(0, 0, 0));
	g->FillRect(8, 8, mWidth - 16, mHeight - 16);

	for (int i = 0; i < NUM_LIGHTS; i++)
	{
		double t = i * 0.025 + mLightPhase;
		if (t > 1.0)
			t -= 1.0;

		// x87: t * 4.0 first, then times the int edge length converted to double; _ftol truncates
		int x;
		int y;
		if (t < 0.25)
		{
			x = (int) (t * 4.0 * (double) (mWidth - 39)) + 18;
			y = 14;
		}
		else if (t < 0.5)
		{
			x = mWidth - 21;
			y = (int) ((t - 0.25) * 4.0 * (double) (mHeight - 34)) + 14;
		}
		else if (t < 0.75)
		{
			x = (int) ((0.75 - t) * 4.0 * (double) (mWidth - 39)) + 18;
			y = mHeight - 20;
		}
		else
		{
			x = 18;
			y = (int) ((1.0 - t) * 4.0 * (double) (mHeight - 34)) + 14;
		}

		aLightX[i] = x;
		aLightY[i] = y;
		g->DrawImage(mApp->mImages[IMAGE_DIALOGLIGHT], x - 6, y - 6);
	}

	Dialog::Draw(g);

	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	for (int i = 0; i < NUM_LIGHTS; i++)
	{
		int y = aLightY[i];
		if ((y >= 28) && (y <= 54))
			continue;
		if ((y >= mHeight - 56) && (y <= mHeight - 30))
			continue;
		g->DrawImage(mApp->mImages[IMAGE_DIALOGGLOW], aLightX[i] - 10, y - 10);
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}
