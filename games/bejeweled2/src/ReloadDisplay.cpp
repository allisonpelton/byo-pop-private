// ReloadDisplay.cpp: ReloadDisplay (ReloadDisplay.h).
#include "ReloadDisplay.h"

#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "WinDMApp.h"

using namespace Sexy;

// 0x5d3dec (the original keeps the app at +0x88; mDrawn is the port's)
ReloadDisplay::ReloadDisplay(WinDMApp* theApp)
{
	mApp = theApp;
	mDrawn = false;
}

// 0x5d3e12: the original polls its reload thread here. The port has no thread: it runs the reload in this update, once the
// display has been drawn, and then does what the original does when the thread is done.
void ReloadDisplay::Update()
{
	Widget::Update();
	if (mApp->mReloading && mDrawn) {
		mApp->mReloading = false;
		mApp->ReloadThreadProc();
		if (!mApp->mLoadingFailed) {
			mApp->DoneReloading();
			return;
		}
		mApp->Shutdown();
	}
	if ((mUpdateCnt & 7) == 0)
		MarkDirty();
}

// 0x5d3ea8: the picture of the loading screen, with the lit part of the bar as far as the loading has come
void ReloadDisplay::Draw(Graphics* g)
{
	mDrawn = true;
	g->SetColor(Color(0, 0, 0));
	g->FillRect(0, 0, mWidth, mHeight);
	int aY = (mHeight - IMAGE_RELOAD_BKG->GetHeight()) / 2;
	g->DrawImage(IMAGE_RELOAD_BKG, (mWidth - IMAGE_RELOAD_BKG->GetWidth()) / 2, aY);
	int aX = (mWidth - IMAGE_RELOAD_OVERLAY->GetWidth()) / 2;
	aY = (mHeight - IMAGE_RELOAD_OVERLAY->GetHeight()) / 2;
	g->DrawImage(IMAGE_RELOAD_OVERLAY, aX, aY);
	int aLit = (IMAGE_RELOAD_OVERLAY_LIT->GetWidth() * mApp->mCompletedLoadingThreadTasks) / mApp->mNumLoadingThreadTasks;
	if (IMAGE_RELOAD_OVERLAY_LIT->GetWidth() < aLit)
		aLit = IMAGE_RELOAD_OVERLAY_LIT->GetWidth();
	g->DrawImage(IMAGE_RELOAD_OVERLAY_LIT, aX, aY, Rect(0, 0, aLit, IMAGE_RELOAD_OVERLAY_LIT->GetHeight()));
}
