#include "TitleScreen.h"
#include "WinBMApp.h"
#include "Res.h"
#include "HyperlinkWidget.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "Image.h"
#include <math.h>

using namespace Sexy;

// The "Deluxe" badge's flash (Draw): theLum added to the luminosity of an HSL pixel, capped at 255
static ulong BoostLuminosity(ulong theHSL, int theLum)
{
	int aLum = (int)((theHSL >> 16) & 0xff) + theLum;
	if (aLum > 255)
		return theHSL | 0xff0000;
	return (theHSL & 0xff00ffff) | ((ulong)aLum << 16);
}

// 0x41b120
TitleScreen::TitleScreen(WinBMApp* theApp)
{
	mApp = theApp;

	mLoadProgress = 0.0;
	mLoadDone = false;

	mPlayLink = new HyperlinkWidget(PLAY_LINK_ID, this);
	mPlayLink->mUnderlineSize = 0;
	mPlayLink->SetFont(mApp->mFont1OutlineUnderlined);
	mPlayLink->SetVisible(false);
	mPlayLink->mColor = Color(32, 255, 32);

	mStripScroll = 0;
	mLogoAngle = 0.0;
	mLogoY = -mApp->mImages[IMAGE_BIGMONEYLOGO]->GetHeight();
	mMoneybagsAngle = 0.0;
	mMoneybagsDY = mApp->mImages[IMAGE_MRMONEYBAGS]->GetHeight();
	mBarAngle = 0.0;
	mBarDY = mApp->mImages[IMAGE_BARANDLOGO]->GetHeight();
	mCigarPhase = 0.0;
	mEyePhase = 0.0;

	// The "Deluxe" badge: an HSL copy of IMAGE_DELUXOR whose luminosity Draw raises by mDeluxeLum.
	MemoryImage* aSrcImage = dynamic_cast<MemoryImage*>(mApp->mImages[IMAGE_DELUXOR]);
	mDeluxeImage = new MemoryImage(mApp);
	mDeluxeImage->Create(aSrcImage->GetWidth(), aSrcImage->GetHeight());
	mDeluxeImage->SetImageMode(true, true);
	mDeluxeImage->SetVolatile(true);
	int aSize = mDeluxeImage->GetWidth() * mDeluxeImage->GetHeight();
	mDeluxeHSL = new ulong[aSize];
	mApp->RGBToHSL(aSrcImage->GetBits(), mDeluxeHSL, aSize);
	mDeluxeWork = new ulong[aSize];
	// port: a mod's HD badge gets the same HSL copy (HDPixels.h). SexyAppBase::RGBToHSL packs a saturation
	// of 256 for any colour with a zero channel, which spills into the luminosity and leaves the pixel grey.
	// The game's badge has no such colours, but upscaled art is full of them, so its zero channels are
	// raised to 1 first (an invisible change).
	mDeluxeHD = GetHDPixels(aSrcImage);
	if (mDeluxeHD.IsValid())
	{
		std::vector<ulong> aHDBits(mDeluxeHD.mBits.begin(), mDeluxeHD.mBits.end());
		for (ulong& aPixel : aHDBits)
		{
			for (int aShift = 0; aShift < 24; aShift += 8)
			{
				if (((aPixel >> aShift) & 0xff) == 0)
					aPixel |= (ulong) 1 << aShift;
			}
		}
		mDeluxeHDHSL.resize(aHDBits.size());
		mApp->RGBToHSL(&aHDBits[0], &mDeluxeHDHSL[0], (int) aHDBits.size());
	}
	mDeluxeLum = 256;
	mDeluxeDirty = false; // port: left uninitialised by the original (only read after Update has set it)
	mDeluxeShown = false;

	// Sets the link labels and moves Mr. Moneybags one step (mUpdateCnt becomes 1); mWidth is still 0 here.
	Update();
}

// 0x419630
TitleScreen::~TitleScreen()
{
	delete mPlayLink;
	delete[] mDeluxeHSL;
	delete[] mDeluxeWork;
	delete mDeluxeImage;
}

// 0x4196d0
void TitleScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mPlayLink);
}

// 0x419700
void TitleScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mPlayLink);
}

// 0x412c50 (COMDAT shared with HighScoreWidget: a jump to Widget::Resize)
void TitleScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
}

// 0x419730
void TitleScreen::ButtonDepress(int theId)
{
	mApp->TitleScreenIsFinished();
}

// 0x41a810
void TitleScreen::Update()
{
	Widget::Update();

	if (mUpdateCnt % 8 == 0)
	{
		mCigarPhase += 0.05;
		if (mCigarPhase > 1.0)
			mCigarPhase -= 1.0;

		mEyePhase += 0.06;
		if (mEyePhase > 1.0)
			mEyePhase -= 1.0;

		if (mStripScroll == 0)
			mStripScroll = 64;
		else
			mStripScroll--;

		if (mUpdateCnt >= 180)
			mDeluxeShown = true;

		if (mDeluxeShown && mDeluxeLum > 0)
		{
			mDeluxeLum -= 16;
			if (mDeluxeLum < 0)
				mDeluxeLum = 0;
			mDeluxeDirty = true;
		}

		MarkDirty();
	}

	// port: sin() for the original's x87 fsin (same doubles; may differ only at an exact truncation boundary).
	if (mUpdateCnt >= 25 && mLogoY != 0)
	{
		mLogoAngle += 0.016;
		if (mLogoAngle < 1.570795)
			mLogoY = -(int)(mApp->mImages[IMAGE_BIGMONEYLOGO]->GetHeight() * (1.0 - sin(mLogoAngle)));
		else
			mLogoY = 0;
		MarkDirty();
	}

	if (mMoneybagsDY != 0)
	{
		mMoneybagsAngle += 0.02;
		if (mMoneybagsAngle < 1.570795)
			mMoneybagsDY = (int)(mApp->mImages[IMAGE_MRMONEYBAGS]->GetHeight() * (1.0 - sin(mMoneybagsAngle)));
		else
			mMoneybagsDY = 0;
		MarkDirty();
	}

	if (mUpdateCnt >= 50 && mBarDY != 0)
	{
		mBarAngle += 0.025;
		if (mBarAngle < 1.570795)
			mBarDY = (int)(mApp->mImages[IMAGE_BARANDLOGO]->GetHeight() * (1.0 - sin(mBarAngle)));
		else
			mBarDY = 0;

		MarkDirty();
	}

	mPlayLink->mLabel = mApp->GetString("CLICK_HERE_TO_PLAY");
	int aPlayWidth = mPlayLink->mFont->StringWidth(mPlayLink->mLabel) + 8;
	mPlayLink->Resize(308 - aPlayWidth / 2, 435, aPlayWidth, 32);

	if (!mLoadDone && mUpdateCnt % 4 == 0)
	{
		mLoadProgress = mApp->GetLoadingThreadProgress();
		if (mLoadProgress == 1.0 && mApp->mLoaded)
		{
			mPlayLink->SetVisible(true);
			mLoadDone = true;
			mLoadProgress = 1.0;
		}
		MarkDirty();
	}

}

// 0x419950
void TitleScreen::Draw(Graphics* g)
{
	// The original also calls GetBoolean("RealArcade", false) here and ignores the result.
	Image** anImages = mApp->mImages;

	g->DrawImage(anImages[IMAGE_TITLEBACK], 0, 0);

	// Rows of dollar signs, every other row shifted 13 px, scrolling up 1 px per 8 updates (period 64).
	for (int i = 0; i < (mHeight + 31) / 32 + 2; i++)
		g->DrawImage(anImages[IMAGE_DOLLARSTRIP], (i % 2 != 0) ? 13 : 0, mStripScroll + 32*i - 64);

	Image* aMoneybags = anImages[IMAGE_MRMONEYBAGS];
	g->DrawImage(aMoneybags, 0, mMoneybagsDY + mHeight - aMoneybags->GetHeight() + 6);

	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, (int)(fabs(mCigarPhase - 0.5) * 510.0)));
	g->DrawImage(anImages[IMAGE_TITLECIGARTIP], 26, mMoneybagsDY + mHeight - aMoneybags->GetHeight() + 184);
	g->SetColorizeImages(false);

	g->SetColorizeImages(true);
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColor(Color(0, 200, 0, (int)(fabs(mEyePhase - 0.5) * 510.0)));
	g->DrawImage(anImages[IMAGE_TITLEEYEGLOW], 122, mMoneybagsDY + mHeight - aMoneybags->GetHeight() + 90);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColorizeImages(false);

	Image* aBar = anImages[IMAGE_BARANDLOGO];
	g->DrawImage(aBar, mWidth - aBar->GetWidth(), mHeight + mBarDY - aBar->GetHeight(), Rect(0, 0, 540, 149));

	// Rows 125..129 of the bar drawn once more with fading alphas (212, 170, 127, 85, 42).
	int aRow = 0;
	for (int anAlpha = 1275; anAlpha > 0; anAlpha -= 255)
	{
		Graphics aRowG(*g);
		aRowG.SetColorizeImages(true);
		aRowG.SetColor(Color(255, 255, 255, anAlpha / 6));
		aRowG.ClipRect(mWidth - aBar->GetWidth(), mHeight + mBarDY + aRow - aBar->GetHeight() + 125, 540, 1);
		aRowG.DrawImage(aBar, mWidth - aBar->GetWidth(), mHeight + mBarDY - aBar->GetHeight());
		aRowG.SetColorizeImages(false);
		aRow++;
	}

	g->DrawImage(anImages[IMAGE_BIGMONEYLOGO], 84, mLogoY - 8);

	if (mApp->mCobrandImage != NULL)
		g->DrawImage(mApp->mCobrandImage, 8, mHeight + mBarDY - mApp->mCobrandImage->GetHeight());

	Image* aGoldBar = anImages[IMAGE_GOLDBAR];
	if (mLoadProgress >= 1.0)
	{
		g->DrawImage(aGoldBar, 120, mBarDY + 381);
	}
	else if (mLoadProgress > 0.0)
	{
		int aBarWidth = (int)(mLoadProgress * 318.0 + 25.0);
		g->DrawImage(aGoldBar, 120, mBarDY + 381, Rect(0, 0, aBarWidth, 76));

		// A 16-column soft edge: alpha cos(i * pi/32) * 255 (port: cos() for the original's x87 fcos).
		for (int i = 0; i < 16; i++)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, (int)(cos((double)i * 0.0981746875) * 255.0)));
			g->DrawImage(aGoldBar, aBarWidth + i + 120, mBarDY + 381, Rect(aBarWidth + i, 0, 1, 76));
			g->SetColorizeImages(false);
		}
	}

	if (mDeluxeShown)
	{
		if (mDeluxeDirty)
		{
			ulong* aBits = mDeluxeImage->GetBits();
			int aSize = mDeluxeImage->GetWidth() * mDeluxeImage->GetHeight();
			for (int i = 0; i < aSize; i++)
				mDeluxeWork[i] = BoostLuminosity(mDeluxeHSL[i], mDeluxeLum);
			mApp->HSLToRGB(mDeluxeWork, aBits, aSize);
			// port: the same for a mod's HD badge (HDPixels.h)
			if (mDeluxeHD.IsValid())
			{
				std::vector<ulong> aWork(mDeluxeHDHSL.size());
				std::vector<ulong> anRGB(mDeluxeHDHSL.size());
				for (size_t i = 0; i < aWork.size(); i++)
					aWork[i] = BoostLuminosity(mDeluxeHDHSL[i], mDeluxeLum);
				mApp->HSLToRGB(&aWork[0], &anRGB[0], (int) aWork.size());
				mDeluxeHD.mBits.assign(anRGB.begin(), anRGB.end());
			}
			mDeluxeImage->BitsChanged();
			SetHDPixels(mDeluxeImage, mDeluxeHD);
			mDeluxeDirty = false;
		}
		g->DrawImage(mDeluxeImage, 478, 18);
	}

	// Status line: empty, but for PLEASE_WAIT on the first run
	g->SetFont(mApp->mFont3Outline);
	SexyString aText;
	int aTextY = 397;

	if (mApp->mTimesPlayed == 0 && mLoadProgress < 1.0)
		aText = mApp->GetString("PLEASE_WAIT");

	// port: the original formats these through sprintf(buf, text) with no arguments; neither string has a '%'.
	int aTextWidth = g->GetFont()->StringWidth(aText);
	g->SetColor(Color(255, 255, 255));
	g->DrawString(aText, 308 - aTextWidth / 2, mBarDY + aTextY);

	g->SetFont(mApp->mFont3);
	g->SetColor(Color(255, 255, 255));
	SexyString aVersion = mApp->GetString("VERSION_") + _S(" ") + StringToSexyStringFast(mApp->mProductVersion);
	g->DrawString(aVersion, mWidth - g->GetFont()->StringWidth(aVersion) - 16, 211);
}
