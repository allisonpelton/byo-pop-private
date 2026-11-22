#include "LoadingScreen.h"
#include "NcApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "HyperlinkWidget.h"
#include "ImageFont.h"
#include "Image.h"
#include "DDImage.h"
#include "MemoryImage.h"
#include <vector>
#include "WidgetManager.h"
#include <math.h>

using namespace Sexy;

static const char* gLoadingImageNames[3] = { "loadinggraphic", "loadingbar", "loadingbarempty" };

// port: the user's wish of 2026-11-18 ("Remove the 'Deluxe' text from the main screen of any game it may appear in, to avoid
// misleading people about whose game version they're playing"). The ribbon under the logo says DELUXE in outlined letters.
// They are painted over as the picture is loaded, from the picture's own pixels (the ribbon is left blank): the letters'
// strokes inside the ribbon are the dark thin lines that a closing of the luminance removes, and each is filled with the
// mean of the ribbon's pixels around it. Nothing of the picture is stored anywhere else.
static void ClearBannerText(MemoryImage* theImage)
{
	if (theImage == NULL || theImage->GetWidth() != 640 || theImage->GetHeight() != 480)
		return;
	ulong* aBits = theImage->GetBits();
	if (aBits == NULL)
		return;

	// the ribbon's inside, without its outline and the hair that overlaps it
	static const int aPolygon[][2] = { {222, 138}, {250, 132}, {262, 128}, {280, 124}, {300, 123}, {340, 122}, {360, 125}, {378, 128},
		{400, 131}, {414, 136}, {414, 164}, {380, 160}, {356, 148}, {318, 149}, {290, 160}, {224, 167} };
	const int aPoints = sizeof(aPolygon) / sizeof(aPolygon[0]);
	const int aLeft = 206, aTop = 111, aWidth = 224, aHeight = 69;

	std::vector<unsigned char> aInside(aWidth * aHeight, 0);
	for (int y = 0; y < aHeight; y++)
		for (int x = 0; x < aWidth; x++)
		{
			int aX = aLeft + x, aY = aTop + y;
			bool anIn = false;
			for (int i = 0, j = aPoints - 1; i < aPoints; j = i++)
			{
				int aXi = aPolygon[i][0], aYi = aPolygon[i][1], aXj = aPolygon[j][0], aYj = aPolygon[j][1];
				if (((aYi > aY) != (aYj > aY)) && (aX < (aXj - aXi) * (aY - aYi) / (double) (aYj - aYi) + aXi))
					anIn = !anIn;
			}
			aInside[y * aWidth + x] = anIn;
		}

	std::vector<int> aLuma(aWidth * aHeight), aTmp(aWidth * aHeight), aClosed(aWidth * aHeight);
	for (int i = 0; i < aWidth * aHeight; i++)
	{
		ulong aPixel = aBits[(aTop + i / aWidth) * 640 + aLeft + i % aWidth];
		aLuma[i] = (299 * ((aPixel >> 16) & 255) + 587 * ((aPixel >> 8) & 255) + 114 * (aPixel & 255)) / 1000;
	}
	// a closing with a 9 x 9 window: a maximum, then a minimum
	for (int aPass = 0; aPass < 2; aPass++)
	{
		const std::vector<int>& aFrom = aPass == 0 ? aLuma : aTmp;
		std::vector<int>& aTo = aPass == 0 ? aTmp : aClosed;
		for (int y = 0; y < aHeight; y++)
			for (int x = 0; x < aWidth; x++)
			{
				int aBest = aFrom[y * aWidth + x];
				for (int dy = -4; dy <= 4; dy++)
					for (int dx = -4; dx <= 4; dx++)
					{
						int aX = x + dx < 0 ? 0 : (x + dx >= aWidth ? aWidth - 1 : x + dx);
						int aY = y + dy < 0 ? 0 : (y + dy >= aHeight ? aHeight - 1 : y + dy);
						int aValue = aFrom[aY * aWidth + aX];
						if (aPass == 0 ? aValue > aBest : aValue < aBest)
							aBest = aValue;
					}
				aTo[y * aWidth + x] = aBest;
			}
	}

	std::vector<unsigned char> aStroke(aWidth * aHeight, 0), aMask(aWidth * aHeight, 0);
	for (int i = 0; i < aWidth * aHeight; i++)
		aStroke[i] = aInside[i] && (aClosed[i] - aLuma[i] > 14);
	for (int y = 0; y < aHeight; y++)
		for (int x = 0; x < aWidth; x++)
		{
			bool aNear = false;
			for (int dy = -1; dy <= 1 && !aNear; dy++)
				for (int dx = -1; dx <= 1; dx++)
					if (x + dx >= 0 && x + dx < aWidth && y + dy >= 0 && y + dy < aHeight && aStroke[(y + dy) * aWidth + x + dx])
					{
						aNear = true;
						break;
					}
			aMask[y * aWidth + x] = aNear && aInside[y * aWidth + x];
		}

	// each stroke pixel takes the mean of the known pixels around it, from the outside in
	std::vector<double> aColor(aWidth * aHeight * 3);
	for (int i = 0; i < aWidth * aHeight; i++)
	{
		ulong aPixel = aBits[(aTop + i / aWidth) * 640 + aLeft + i % aWidth];
		aColor[i * 3] = (aPixel >> 16) & 255;
		aColor[i * 3 + 1] = (aPixel >> 8) & 255;
		aColor[i * 3 + 2] = aPixel & 255;
	}
	std::vector<unsigned char> aKnown(aWidth * aHeight);
	for (int i = 0; i < aWidth * aHeight; i++)
		aKnown[i] = !aMask[i];
	for (int aRound = 0; aRound < 40; aRound++)
	{
		std::vector<int> aFilled;
		std::vector<double> aValues;
		for (int y = 0; y < aHeight; y++)
			for (int x = 0; x < aWidth; x++)
			{
				if (!aMask[y * aWidth + x])
					continue;
				double aSum[3] = { 0, 0, 0 };
				int aCount = 0;
				for (int dy = -2; dy <= 2; dy++)
					for (int dx = -2; dx <= 2; dx++)
					{
						int aX = x + dx, aY = y + dy;
						if (aX < 0 || aX >= aWidth || aY < 0 || aY >= aHeight || !aKnown[aY * aWidth + aX])
							continue;
						for (int c = 0; c < 3; c++)
							aSum[c] += aColor[(aY * aWidth + aX) * 3 + c];
						aCount++;
					}
				if (aCount >= 3)
				{
					aFilled.push_back(y * aWidth + x);
					for (int c = 0; c < 3; c++)
						aValues.push_back(aSum[c] / aCount);
				}
			}
		if (aFilled.empty())
			break;
		for (size_t i = 0; i < aFilled.size(); i++)
		{
			for (int c = 0; c < 3; c++)
				aColor[aFilled[i] * 3 + c] = aValues[i * 3 + c];
			aKnown[aFilled[i]] = 1;
			aMask[aFilled[i]] = 0;
		}
	}

	for (int y = 0; y < aHeight; y++)
		for (int x = 0; x < aWidth; x++)
			if (aInside[y * aWidth + x])
			{
				int i = y * aWidth + x;
				int aR = (int) (aColor[i * 3] + 0.5), aG = (int) (aColor[i * 3 + 1] + 0.5), aB = (int) (aColor[i * 3 + 2] + 0.5);
				aBits[(aTop + y) * 640 + aLeft + x] = 0xFF000000 | (aR << 16) | (aG << 8) | aB;
			}
	theImage->BitsChanged();
}

// 0x4e77e0 (settings from 0x418960)
// 0x418800
LoadingScreen::LoadingScreen(NcApp* theApp)
{
	mApp = theApp;
	mFade = 255.0;
	mDelay = 100;
	mLoadDone = false;
	mProgress = 0.0;
	mProgressCap = 1.0;

	// 0x4e7540: the three pictures of the Nc framework
	for (int i = 0; i < 3; i++)
	{
		SexyString aPath = SexyString("NcFramework\\images\\") + gLoadingImageNames[i];
		mImages[i] = mApp->GetImage(aPath);
		if (mImages[i] == NULL)
		{
			mApp->Popup("Failed to load " + aPath);
			mApp->mLoadingFailed = true;
			break;
		}
	}

	if (mImages[0] != NULL)
		ClearBannerText(dynamic_cast<MemoryImage*>(mImages[0]));

	mLinkFont = new ImageFont(mApp, "data\\Praitor36.txt");

	mPlayLink = new HyperlinkWidget(PLAY_LINK_ID, this);
	mPlayLink->mUnderlineSize = 0;
	mPlayLink->SetFont(mLinkFont);
	mPlayLink->SetVisible(false);
	mPlayLink->mColor = Color(235, 235, 235);
	mPlayLink->mOverColor = Color(255, 255, 255);
}

// 0x4e6a50
LoadingScreen::~LoadingScreen()
{
	delete mPlayLink;
	delete mLinkFont;
	for (int i = 0; i < 3; i++)
		delete mImages[i];
}

// 0x4e67b0
void LoadingScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mPlayLink);
}

// 0x4e67e0
void LoadingScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mPlayLink);
}

// 0x4e6810: the bar fills from the left by 20 + 332 * progress pixels; its end is feathered with 16 columns whose
// alpha is the original's truncated cosine (255 for the first, 0 for the others)
void LoadingScreen::DrawBar(Graphics* g)
{
	int aBarY = 0x1bb;
	int aBarX = (mWidth - mImages[1]->GetWidth()) / 2;

	if (mProgress < 1.0)
	{
		g->DrawImage(mImages[2], aBarX, aBarY);
		if (mProgress > 0.0)
		{
			int aFill = 0x14 - (int)(mProgress * -332.0);
			g->DrawImage(mImages[1], aBarX, aBarY, Rect(0, 0, aFill, mImages[1]->GetHeight()));
			for (int i = 0; i < 0x10; i++)
			{
				g->SetColorizeImages(true);
				int anAlpha = (int)cos(i * 0.0981746875) * 0xff;
				g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
				g->DrawImage(mImages[1], aBarX + aFill + i, aBarY, Rect(aFill + i, 0, 1, mImages[1]->GetHeight()));
				g->SetColorizeImages(false);
			}
		}
		return;
	}

	// finished: the full bar, fading out after a while
	if (!mLoadDone)
		g->DrawImage(mImages[1], aBarX, aBarY);
	else if (mFade > 0.0)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(0xff, 0xff, 0xff, (int)mFade));
		g->DrawImage(mImages[1], aBarX, aBarY);
		g->SetColorizeImages(false);
	}
}

// 0x4e6f30
void LoadingScreen::Draw(Graphics* g)
{
	Widget::Draw(g);
	g->DrawImage(mImages[0], 0, 0);
	DrawBar(g);
}

// 0x418820 + 0x4e6f70
void LoadingScreen::Update()
{
	// only while the window has the focus
	if (!mApp->mHasFocus)
		return;

	Widget::Update();

	if (mLoadDone)
	{
		if (mDelay == 0)
		{
			if (mFade > 0.0)
			{
				double aFade = mFade;
				mFade = aFade - 1.4;
				if (aFade - 1.4 < 0.0)
					mFade = 0.0;
				MarkDirty();
			}
		}
		else
			mDelay--;
		mPlayLink->SetVisible(true);
	}
	else
	{
		// port: asked at every update (the original asks at every fourth: a time step the loading need not wait for)
		mProgress = mApp->GetLoadingThreadProgress();
		if (mProgress == 1.0 && mApp->mLoaded)
		{
			mPlayLink->SetVisible(true);
			mLoadDone = true;
			mProgress = 1.0;
		}
		MarkDirty();
	}

	// the play link lies over the finished bar
	mPlayLink->mLabel = GT(0x5288f8);
	int aWidth = mImages[1]->GetWidth();
	mPlayLink->Resize(0x140 - aWidth / 2, 0x1ae, aWidth, 0x2e);
}

// 0x4e73c0
void LoadingScreen::ButtonDepress(int theId)
{
	if (theId == PLAY_LINK_ID)
		mApp->PlayClicked();
}
