// WinDMAppImages.cpp: the images WinDMApp makes from the gem art when the game loads (0x5dc96d, 0x5dcb1b).
#include "WinDMApp.h"

#include "Graphics.h"
#include "MemoryImage.h"

using namespace Sexy;

// The gems shrinking as they disappear: for each of the 7 colors, 6 frames, the gem drawn smaller and smaller into a
// square of its own size (0x5dc96d). The first resource id of the gem images is 0x5c.
void WinDMApp::MakeShrunkenGems()
{
	mShrunkenGems = new MemoryImage**[7];
	for (int aColor = 0; aColor < 7; aColor++) {
		mShrunkenGems[aColor] = new MemoryImage*[6];
		int aFrame = 0;
		do {
			MemoryImage* anImage = new MemoryImage(this);
			anImage->Create(ScaleNumUp(84, 0), ScaleNumUp(84, 0));
			int aSize = (int)((1.0 - (aFrame + 1) / 7.0) * 84.0) | 1;   // (the original's expression, in the x87's double)
			Graphics g(anImage);
			Rect aSrcRect(0, 0, ScaleNum(84, 0), ScaleNum(84, 0));
			int aPos = ScaleNum((84 - aSize) / 2, 0);
			Rect aDestRect(aPos, aPos, ScaleNumUp(aSize, 0), ScaleNumUp(aSize, 0));
			g.DrawImage(GetImageById(IMAGE_GEM0_ID + aColor), aDestRect, aSrcRect);
			anImage->PurgeBits();
			mShrunkenGems[aColor][aFrame] = anImage;
			aFrame++;
		} while (aFrame < 6);
		mCompletedLoadingThreadTasks++;
	}
}

// The strip of the gems with the white of the "bomb" fading in, ten steps for each color (0x5dcb1b). Made once.
void WinDMApp::GenerateBombifiedStillStrip()
{
	if (mBombifiedStillStrip != nullptr)
		return;
	mBombifiedStillStrip = new MemoryImage(this);
	mBombifiedStillStrip->Create(ScaleNumUp(840, 0), ScaleNumUp(588, 0));
	mBombifiedStillStrip->SetImageMode(true, true);
	Graphics g(mBombifiedStillStrip);
	int aColor = 0;
	for (int y = 0; y < 588; y += 84) {
		int i = 1;
		for (int x = 0; x < 840; x += 84) {
			float aFrac = (float)(i / 10.0);
			Rect aSrcRect(0, 0, ScaleNum(84, 0), ScaleNum(84, 0));
			g.DrawImage(GetImageById(IMAGE_GEM0_ID + aColor), ScaleNum(x, 0), ScaleNum(y, 0), aSrcRect);
			int aGray = (int)((double)aFrac * 255.0);
			g.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g.SetColorizeImages(true);
			g.SetColor(Color(aGray, aGray, aGray));
			Rect anAddRect(0, ScaleNum(84, 0) * aColor, ScaleNumUp(84, 0), ScaleNumUp(84, 0));
			g.DrawImage(IMAGE_GEM_ADD, ScaleNum(x, 0), ScaleNum(y, 0), anAddRect);
			g.SetColorizeImages(false);
			g.SetDrawMode(Graphics::DRAWMODE_NORMAL);
			i++;
		}
		aColor++;
	}
	mBombifiedStillStrip->PurgeBits();
}
