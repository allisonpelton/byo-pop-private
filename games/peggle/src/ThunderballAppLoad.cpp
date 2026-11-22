// ThunderballAppLoad.cpp: the loading thread of Peggle's application class and the pictures it makes from the ones it loads
// (bricks, pegs, fever stars). The framework runs LoadingThreadProc once the first frame is up (there are no threads in
// this port); the loading screen shows how far it has come through mCompletedLoadingThreadTasks.

#include "ThunderballApp.h"

#include "BlendedImage.h"
#include "DDImage.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "Res.h"
#include "ResourceManager.h"

using namespace Sexy;

// The groups of the resources the loading thread loads, in the order it loads them (the exe's table has "Upsell" last, the
// pictures of the registration offers: not in this port).
static const char* const LOADING_THREAD_GROUPS[] = {
	"LoadingThread", "Game", "MainMenu", "LevelSelect", "Help", "Interface", "IP", "Dialog", "Character", "LuckySpin"};
static const int NUM_LOADING_THREAD_GROUPS = sizeof(LOADING_THREAD_GROUPS) / sizeof(LOADING_THREAD_GROUPS[0]);

// The fever stars are made in three sizes (the original's ModValInt calls at ThunderballApp.cpp lines 162 to 164).
static const int FEVER_STAR_SIZES[NUM_FEVER_STAR_SIZES] = {20, 30, 40};

// The peg picture has its eight pegs one below the other, 20 pixels each. The game re-cuts it so each has a margin of a pixel
// round it (22 by 22), which the pictures drawn between pixels need (ThunderballApp.cpp line 161's ModValInt is 1).
static const int PEG_CEL_SIZE = 20;
static const int PEG_CEL_SIZE_WITH_MARGIN = 22;
static const int PEG_IMAGE_HEIGHT_WITH_MARGIN = PEG_CEL_SIZE_WITH_MARGIN * NUM_PEG_IMAGES;

static_assert(IMAGE_BALLPEG_ID + 1 == IMAGE_BALLPEGCB_ID, "the colorblind peg picture follows the plain one");

// 0x421a70
void ThunderballApp::LoadingThreadProc()
{
	// the load is as long as the number of resources (the original also times each of them when a test flag is set: not here)
	mNumLoadingThreadTasks = 0;
	for (int i = 0; i < NUM_LOADING_THREAD_GROUPS; i++)
		mNumLoadingThreadTasks += mResourceManager->GetNumResources(LOADING_THREAD_GROUPS[i]);

	for (int i = 0; i < NUM_LOADING_THREAD_GROUPS; i++)
	{
		const char* aGroup = LOADING_THREAD_GROUPS[i];
		mResourceManager->StartLoadResources(aGroup);
		while (mResourceManager->LoadNextResource())
		{
			if (mShutdown)
				return;
			mCompletedLoadingThreadTasks++;
		}
		if (mShutdown)
			return;

		if (mResourceManager->HadError() || !ExtractResourcesByName(mResourceManager, aGroup))
		{
			ShowResourceError(false);
			mLoadingFailed = true;
			return;
		}
	}

	CopyBrickTextures();
	CopyPegTextures();
	MakeFeverStars();
}

// 0x4081d0: a picture of its own for each cel of the brick picture
void ThunderballApp::CopyBrickTextures()
{
	Image* aBrickImage = gColorblind ? IMAGE_BRICKCB : IMAGE_BRICK;
	int aCelHeight = aBrickImage->GetCelHeight();
	int aWidth = aBrickImage->mWidth;
	for (int i = 0; i < NUM_BRICK_IMAGES; i++)
	{
		delete mBrickImages[i];
		mBrickImages[i] = CopyImage(aBrickImage, Rect(0, i * aCelHeight, aWidth, aCelHeight));
	}
	mBrickGlowImage = IMAGE_BRICKGLOW;
}

// 0x408280: the star pictures stretched to each of their sizes
void ThunderballApp::MakeFeverStars()
{
	for (int aColor = 0; aColor < NUM_FEVER_STAR_COLORS; aColor++)
	{
		Image* aStarImage = nullptr;
		switch (aColor)
		{
		case FEVER_STAR_GLOW:
			aStarImage = IMAGE_FEVERGLOWSTAR;
			break;
		case FEVER_STAR_RED:
			aStarImage = IMAGE_FEVERSTAR_RED;
			break;
		case FEVER_STAR_GREEN:
			aStarImage = IMAGE_FEVERSTAR_GREEN;
			break;
		case FEVER_STAR_BLUE:
			aStarImage = IMAGE_FEVERSTAR_BLUE;
			break;
		}

		for (int aSize = 0; aSize < NUM_FEVER_STAR_SIZES; aSize++)
		{
			delete mFeverStars[aColor][aSize];
			MemoryImage* aImage = new MemoryImage(this);
			aImage->Create(FEVER_STAR_SIZES[aSize], FEVER_STAR_SIZES[aSize]);
			aImage->SetImageMode(true, true);

			Graphics g(aImage);
			g.SetFastStretch(false);
			g.DrawImage(aStarImage, Rect(0, 0, aImage->mWidth, aImage->mHeight), Rect(0, 0, aStarImage->mWidth, aStarImage->mHeight));
			mFeverStars[aColor][aSize] = aImage;
		}
	}
}

// 0x41c610: the peg picture re-cut with a margin round each peg (the first time), then the pictures the pegs are drawn with
void ThunderballApp::CopyPegTextures()
{
	Image* aPegImage = gColorblind ? IMAGE_BALLPEGCB : IMAGE_BALLPEG;

	if (aPegImage->mWidth == PEG_CEL_SIZE)
	{
		MemoryImage* aNewImage = new MemoryImage(this);
		aNewImage->Create(PEG_CEL_SIZE_WITH_MARGIN, PEG_IMAGE_HEIGHT_WITH_MARGIN);
		aNewImage->CopyAttributes(aPegImage);
		aNewImage->mFilePath = aPegImage->mFilePath;

		Graphics g(aNewImage);
		int aSourceY = 0;
		for (int aDestY = 1; aDestY < PEG_IMAGE_HEIGHT_WITH_MARGIN + 1; aDestY += PEG_CEL_SIZE_WITH_MARGIN)
		{
			g.DrawImage(aPegImage, 1, aDestY, Rect(0, aSourceY, PEG_CEL_SIZE, PEG_CEL_SIZE));
			aSourceY += PEG_CEL_SIZE;
		}
		// the resource manager takes the new picture and deletes the old one
		ReplaceImageById(mResourceManager, gColorblind ? IMAGE_BALLPEGCB_ID : IMAGE_BALLPEG_ID, aNewImage);
		aPegImage = aNewImage;
	}

	int aPegHeight = aPegImage->mHeight / NUM_PEG_IMAGES;
	for (int i = 0; i < NUM_PEG_IMAGES; i++)
	{
		delete gPegImages[i];
		gPegImages[i] = new BlendedImage();
		gPegImages[i]->Create(aPegImage, Rect(0, i * aPegHeight, aPegImage->mWidth, aPegHeight));
	}

	delete gPegShadowImage;
	gPegShadowImage = new BlendedImage();
	gPegShadowImage->Create(IMAGE_PEGSHADOW);
}

// 0x41c810
bool ThunderballApp::SetColorblind(bool theColorblind)
{
	if (theColorblind == gColorblind)
		return false;
	gColorblind = theColorblind;
	CopyBrickTextures();
	CopyPegTextures();
	return true;
}
