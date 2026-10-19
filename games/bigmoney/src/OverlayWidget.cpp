#include "OverlayWidget.h"
#include "WinBMApp.h"
#include "Res.h"
#include "Graphics.h"
#include "Image.h"

using namespace Sexy;

// 0x417400
OverlayWidget::OverlayWidget(WinBMApp* theApp)
{
	mApp = theApp;
	mMouseVisible = false;
	mAlpha = 1.0;
	mHasAlpha = true;
}

// 0x4174d0 (body: the Widget dtor)
OverlayWidget::~OverlayWidget()
{
}

// 0x417440
void OverlayWidget::Draw(Graphics* g)
{
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, (int)(mAlpha * 255.0)));
	g->DrawImage(mApp->mImages[IMAGE_LEFTBAR], 0, 0);
	g->SetColorizeImages(false);
	g->DrawImage(mApp->mImages[IMAGE_FLOATLOGO], 15, 8);
}
