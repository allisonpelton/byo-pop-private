#include "LoaderScreen.h"
#include "AstroPopApp.h"
#include "ButtonWidget.h"
#include "Graphics.h"
#include "Font.h"
#include "MemoryImage.h"
#include "WidgetManager.h"
#include "GameText.h"

using namespace Sexy;

namespace
{
	// The colour each slide's silhouette strip is tinted with (static Colors of the original, 0x5a5148 + 16 * n;
	// the initialisers at 0x557xxx fill them, tools/initvals.py reads them)
	const int kSlideTints[4][4] = {
		{200, 50, 60, 200},
		{150, 130, 40, 200},
		{80, 110, 255, 200},
		{175, 0, 175, 200}
	};

	const int PLAY_BUTTON_ID = 2200; // 0x898
}

// 0x45c0f0
LoaderScreen::LoaderScreen(AstroPopApp* theApp)
{
	mSlides.clear();
	mCurSlide = 0;
	mDirY = 0;
	mDirX = 0;
	mApp = theApp;

	mPlayButton = new ButtonWidget(PLAY_BUTTON_ID, theApp);
}

// 0x445b10
LoaderScreen::~LoaderScreen()
{
	delete mPlayButton;
	for (size_t i = 0; i < mSlides.size(); i++)
	{
		delete mSlides[i]->mTinted;
		delete mSlides[i];
	}
}

// 0x4548e0
void LoaderScreen::MakeSlides()
{
	mPlayButton->mButtonImage = IMAGE_LOADER_CLICK_TO_PLAY;
	mPlayButton->mOverImage = IMAGE_LOADER_CLICK_TO_PLAY;

	for (int aSlideIndex = 0; aSlideIndex < 4; aSlideIndex++)
	{
		LoaderSlide* aSlide = new LoaderSlide();
		aSlide->mPicture = GetImageById(0x9b + aSlideIndex);
		aSlide->mSilhouette = GetImageById(0xa3 + aSlideIndex);
		aSlide->mBigText = GetImageById(0x9f + aSlideIndex);

		// The strip at half size, tinted: the big text is scaled into a scratch image, then drawn colourized
		aSlide->mTinted = new MemoryImage(mApp);
		aSlide->mTinted->Create((int) (aSlide->mBigText->mWidth * 0.5f), (int) (aSlide->mBigText->mHeight * 0.5f));
		MemoryImage* aScratch = new MemoryImage(mApp);
		aScratch->Create((int) (aSlide->mBigText->mWidth * 0.5f), (int) (aSlide->mBigText->mHeight * 0.5f));

		Graphics aScratchGraphics(aScratch);
		aScratchGraphics.DrawImage(aSlide->mBigText, Rect(0, 0, aScratch->mWidth, aScratch->mHeight),
			Rect(0, 0, aSlide->mBigText->mWidth, aSlide->mBigText->mHeight));

		Graphics aTintedGraphics(aSlide->mTinted);
		aTintedGraphics.SetColorizeImages(true);
		aTintedGraphics.SetColor(Color(kSlideTints[aSlideIndex][0], kSlideTints[aSlideIndex][1], kSlideTints[aSlideIndex][2], kSlideTints[aSlideIndex][3]));
		aTintedGraphics.DrawImage(aScratch, 0, 0);
		aTintedGraphics.SetColorizeImages(false);

		delete aScratch; // port: the original never frees it

		aSlide->mFade = 0xff;
		aSlide->mBigTextX = -aSlide->mBigText->mWidth;
		aSlide->mTintedX = 0;
		mSlides.push_back(aSlide);
	}

	mCurSlide = (int) ((unsigned int) Rand() % (unsigned int) mSlides.size());
	mDirY = -1;
	mDirX = 1;
}

// 0x445c10
void LoaderScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);

	mPlayButton->SetVisible(false);
	mPlayButton->mHasTransparencies = true;
	mPlayButton->Resize(0xbe, 0x1ae, 0x124, 0x24);
	theWidgetManager->AddWidget(mPlayButton);
	mPlayButton->mDoFinger = true;
	mPlayButton->SetDisabled(true);
}

// 0x434860
void LoaderScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mPlayButton);
}

// 0x445df0
void LoaderScreen::Draw(Graphics* g)
{
	g->DrawImage(IMAGE_LOADER_SCREEN, 0, 0);

	int aBarWidth = (int) (mApp->GetLoadingThreadProgress() * IMAGE_LOADER_BAR->mWidth);
	if (aBarWidth > 0)
		g->DrawImage(IMAGE_LOADER_BAR, 0x94, 0x187, Rect(0, 0, aBarWidth, IMAGE_LOADER_BAR->mHeight));

	if (!mPlayButton->mVisible)
		g->DrawImage(IMAGE_LOADER_LOADINGTXT, 0xd7, 0x1ac);

	LoaderSlide* aSlide = mSlides[mCurSlide];
	bool is3D = mApp->Is3DAccelerated();

	g->DrawImage(aSlide->mSilhouette, 0x153, 0xa9);
	if (aSlide->mFade > 0)
	{
		if (is3D)
		{
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColorizeImages(true);
			g->SetColor(Color(aSlide->mFade, aSlide->mFade, aSlide->mFade));
		}
		g->DrawImage(aSlide->mSilhouette, 0x153, 0xa9);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColorizeImages(false);
	}

	for (int i = 0; i < 6; i++)
	{
		if (aSlide->mFade > 0 && is3D)
		{
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColorizeImages(true);
			int v = 0xff - aSlide->mFade;
			g->SetColor(Color(v, v, v));
		}
		g->DrawImage(aSlide->mTinted, (aSlide->mTinted->mWidth + 3) * i + aSlide->mTintedX, 0xec);
		if (aSlide->mFade > 0)
		{
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			g->SetColorizeImages(false);
		}
	}

	for (int i = 0; i < 3; i++)
	{
		if (is3D && aSlide->mFade > 0)
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		int v = 0xff;
		if (is3D)
			v = 0xff - aSlide->mFade;
		g->SetColor(Color(v, v, v, 0x3c));
		g->DrawImage(aSlide->mBigText, (aSlide->mBigText->mWidth + 6) * i + aSlide->mBigTextX, 0x112);
		g->SetColorizeImages(false);
		if (aSlide->mFade > 0)
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}

	g->DrawImage(aSlide->mPicture, 0x1c5, 0x93);
	if (aSlide->mFade > 0 && is3D)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		g->SetColor(Color(aSlide->mFade, aSlide->mFade, aSlide->mFade));
		g->DrawImage(aSlide->mPicture, 0x1c5, 0x93);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColorizeImages(false);
	}

	// The registered build's line where the register button sat (the button is out of the port)
	g->SetFont(FONT_DIALOG_BOX);
	g->SetColor(Color(255, 255, 255));
	if (mApp->mIsRegistered && !mApp->mBuildUnlocked)
	{
		const SexyString& aText = GT(0x5649bc); // THANKS FOR REGISTERING!
		g->DrawString(aText, (IMAGE_LOADER_BAR->mWidth / 2 - FONT_DIALOG_BOX->StringWidth(aText) / 2) + 0x94,
			0x16b + 0x14);
	}
}

// 0x43ba50 + 0x43bb40
void LoaderScreen::Update()
{
	Widget::Update();

	LoaderSlide* aSlide = mSlides[mCurSlide];
	int aNext = 0;

	if (mUpdateCnt % 600 == 0)
	{
		// the next slide, the scroll directions turn around
		aNext = (mCurSlide == (int) mSlides.size() - 1) ? 0 : mCurSlide + 1;
		mDirX = -mDirX;
		mDirY = -mDirY;
		MarkDirty();
		mSlides[aNext]->mBigTextX = (mDirX > 0) ? -aSlide->mBigText->mWidth : 0;
		mSlides[aNext]->mTintedX = (mDirY >= 0) ? -aSlide->mTinted->mWidth : 0;
		mCurSlide++;
		if (mCurSlide >= (int) mSlides.size())
		{
			mCurSlide = 0;
			for (size_t i = 0; i < mSlides.size(); i++)
				mSlides[i]->mFade = 0xff;
		}
		aSlide = mSlides[mCurSlide];
	}

	if (aSlide->mFade > 0)
	{
		aSlide->mFade -= 2;
		if (aSlide->mFade < 0)
			aSlide->mFade = 0;
	}

	if (mApp->Is3DAccelerated())
	{
		if (mUpdateCnt % 2 == 0)
		{
			aSlide->mBigTextX += mDirX;
			if (mDirX > 0)
			{
				if (aSlide->mBigTextX >= 0)
					aSlide->mBigTextX = -aSlide->mBigText->mWidth;
			}
			else if (mDirX < 0)
			{
				if (aSlide->mBigTextX <= -aSlide->mBigText->mWidth)
					aSlide->mBigTextX = 0;
			}
		}

		aSlide->mTintedX += mDirY;
		if (mDirY < 0)
		{
			if (aSlide->mTintedX <= -aSlide->mTinted->mWidth)
				aSlide->mTintedX = 0;
		}
		else if (mDirY > 0)
		{
			if (aSlide->mTintedX >= 0)
				aSlide->mTintedX = -aSlide->mTinted->mWidth;
		}
		MarkDirty();
	}
}
