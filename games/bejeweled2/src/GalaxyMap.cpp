// GalaxyMap.cpp: Sexy::GalaxyMap (GalaxyMap.h), the map of the puzzle galaxies.
//   Warp 0x5ba6e1, AddedToManager 0x5ba708, RemovedFromManager 0x5ba738, Resize 0x5ba768, ButtonPress 0x5ba84d,
//   ButtonDepress 0x5ba861, GetMoonPos 0x5bac12, GetPlanetAt 0x5bac5c, GetMoonAt 0x5bad42, MouseMove 0x5bae58,
//   Update 0x5bae99, CreateFontGlowImages 0x5bb561, Draw 0x5bb7f5, LoadPuzzleData 0x5bc1c9, MouseDown 0x5bc2fe,
//   constructor 0x5bcff3, Cleanup 0x5bd2c2, destructor 0x5bd3aa, Setup 0x5bd50a, SetupStub 0x5bd97e, StartSetup 0x5bd98a.
// The std::vector and std::string helpers of the original (the size, the range-checked at, push_back, erase, the copies) are
// the containers' own. The original is x87 code: a float it stores is rounded there and the sums are made in double.
//
// Worker thread: the original's StartSetup hands Setup to its worker thread, and Update and Draw wait or do nothing while
// mSettingUp is set (Sleep, and in Draw a wait for the thread). The port runs the task in line (WorkerThread::DoTask), so
// mSettingUp is never set when Update or Draw run; their checks are kept, without the Sleep and the wait.

#include "GalaxyMap.h"
#include "Board.h"
#include "WinDMApp.h"
#include "Profile.h"
#include "GalaxyParser.h"
#include "CustomResourceManager.h"
#include "PodDialog.h"
#include "WorkerThread.h"
#include "Res.h"
#include "Graphics.h"
#include "Font.h"
#include "Image.h"
#include "MemoryImage.h"
#include "DDImage.h"
#include "WidgetManager.h"
#include "Rect.h"
#include "Color.h"
#include "Common.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

using namespace Sexy;

namespace
{

// 0x62b548: where the picture of each of the five moons is, from the planet's picture (x, y)
const int kMoonOffset[5][2] =
{
	{ -12, -18 },
	{ -23,  12 },
	{   5,  34 },
	{  31,  12 },
	{  21, -18 }
};

// 0x66b2e8: the same for the moons of the sun. Update sets the numbers every time it runs (the table is zero before
// the first Update, as the original's data is)
int gSunMoonOffset[5][2];

// 0x5bac12: the place of the picture of a planet's moon on the map (before the scale)
void GetMoonPos(const MapPlanet& thePlanet, int* thePos, int theIndex)
{
	int aOffsetX;
	int aOffsetY;
	if (!thePlanet.mIsSun)
	{
		aOffsetX = kMoonOffset[theIndex][0];
		aOffsetY = kMoonOffset[theIndex][1];
	}
	else
	{
		aOffsetX = gSunMoonOffset[theIndex][0];
		aOffsetY = gSunMoonOffset[theIndex][1];
	}
	thePos[0] = aOffsetX + 0xf2 + thePlanet.mX;
	thePos[1] = aOffsetY + 200 + thePlanet.mY;
}

// the loop of Setup that gives a picture the alpha channel of its mask: the mask's alpha and the picture's color
// ((mask ^ bits) & 0xffffff) ^ mask, in the pictures that are the same size; false when they are not
bool ApplyAlphaMask(Image* theMask, Image* theImage)
{
	if (theMask->mWidth != theImage->mWidth || theMask->mHeight != theImage->mHeight)
		return false;

	int aCount = theMask->mHeight * theMask->mWidth;
	uint32_t* aMaskBits = (uint32_t*)((MemoryImage*)theMask)->GetBits();
	uint32_t* aBits = (uint32_t*)((MemoryImage*)theImage)->GetBits();
	if (aCount > 0)
	{
		do
		{
			*aBits = (((*aMaskBits ^ *aBits) & 0xffffff) ^ *aMaskBits);
			aBits++;
			aMaskBits++;
			aCount--;
		} while (aCount != 0);
	}
	((MemoryImage*)theImage)->BitsChanged();
	return true;
}

}

// 0x5bcff3
GalaxyMap::GalaxyMap(WinDMApp* theApp)
{
	mApp = theApp;
	mHasAlpha = true;
	mMapImage = NULL;
	// (the original leaves mPuzzleBoard and mSelectedPuzzle unset until Setup: zero here)
	memset(mPuzzleBoard, 0, sizeof(mPuzzleBoard));
	mSelectedPuzzle = 0;

	mWarpButton = new PodButton(0, this);
	mWarpButton->mGlowColor = Color(255, 255, 0);
	mWarpButton->mGlowImage = IMAGE_GALAXY_BUTTON_HILITE;
	mWarpButton->mButtonImage = IMAGE_GALAXY_WARP_BUTTON;

	mCloseButton = new PodButton(1, this);
	mCloseButton->mGlowColor = Color(255, 255, 0);
	mCloseButton->mGlowImage = IMAGE_GALAXY_BUTTON_HILITE;
	mCloseButton->mButtonImage = IMAGE_GALAXY_CLOSE_BUTTON;

	mTitleGlow = new MemoryImage(mApp);
	mTitleGlow->Create(gApp->ScaleNumUp(0x2c3, 0), gApp->ScaleNumUp(0x65, 0));
	mTitleGlow->SetImageMode(true, true);

	mPlanetGlow = new MemoryImage(mApp);
	mPlanetGlow->Create(gApp->ScaleNumUp(0x10e, 0), gApp->ScaleNumUp(0x38, 0));
	mPlanetGlow->SetImageMode(true, true);

	mPuzzleGlow = new MemoryImage(mApp);
	mPuzzleGlow->Create(gApp->ScaleNumUp(0xae, 0), gApp->ScaleNumUp(0x2f, 0));
	mPuzzleGlow->SetImageMode(true, true);

	mSettingUp = false;
}

// 0x5bd3aa (the original first waits for its worker thread, which the port does not have)
GalaxyMap::~GalaxyMap()
{
	delete mWarpButton;
	delete mCloseButton;
	delete mTitleGlow;
	delete mPlanetGlow;
	delete mPuzzleGlow;
	delete mMapImage;
	Cleanup();
}

// 0x5ba6e1
void GalaxyMap::Warp()
{
	mApp->mBoard->HideGalaxyMap();
}

// 0x5ba708
void GalaxyMap::AddedToManager(WidgetManager* theManager)
{
	Widget::AddedToManager(theManager);
	theManager->AddWidget(mWarpButton);
	theManager->AddWidget(mCloseButton);
}

// 0x5ba738
void GalaxyMap::RemovedFromManager(WidgetManager* theManager)
{
	Widget::RemovedFromManager(theManager);
	theManager->RemoveWidget(mWarpButton);
	theManager->RemoveWidget(mCloseButton);
}

// 0x5ba768: the two buttons stay at the top of the map as it slides
void GalaxyMap::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mWarpButton->Resize(gApp->ScaleNum(0xe6, 0), gApp->ScaleNum(0x8d, 0) + theY, gApp->ScaleNumUp(0xba, 0),
		gApp->ScaleNumUp(0x2d, 0));
	mCloseButton->Resize(gApp->ScaleNum(0x2ea, 0), gApp->ScaleNum(0x8d, 0) + theY, gApp->ScaleNumUp(0xba, 0),
		gApp->ScaleNumUp(0x2d, 0));
}

// 0x5ba84d
void GalaxyMap::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_CLICK);
}

// 0x5ba861: the warp button leaves for the puzzle that is picked, the close button for the one that was being played
void GalaxyMap::ButtonDepress(int theId)
{
	mApp->PlaySample(SOUND_PANEL_SLIDE);
	if (theId == 0)
	{
		Warp();
	}
	else
	{
		mSelectedPuzzle = mApp->mProfile->GetPuzzleProgress()->mPuzzleNum;
		mApp->mBoard->HideGalaxyMap();
	}
}

// 0x5bac5c: the planet whose picture the point is on (a circle round its middle, two pixels wider than the picture), -1
int GalaxyMap::GetPlanetAt(int theX, int theY)
{
	int aPlanetNum = 0;
	if ((int)mPlanets.size() > 0)
	{
		do
		{
			MapPlanet& aPlanet = mPlanets[aPlanetNum];
			if (aPlanet.mMoons.size() != 0)
			{
				int aHalf = aPlanet.mImage->mWidth / 2;
				int aDeltaX = (-aHalf + theX) - gApp->ScaleNum(aPlanet.mX + 0xf2, 0);
				int aDeltaY = (theY + -aHalf) - gApp->ScaleNum(aPlanet.mY + 200, 0);
				double aDistance = sqrt((double)(aDeltaY * aDeltaY + aDeltaX * aDeltaX));
				if (aDistance <= (double)(aHalf + 2))
					return aPlanetNum;
			}
			aPlanetNum++;
		} while (aPlanetNum < (int)mPlanets.size());
	}
	return -1;
}

// 0x5bad42: the puzzle (planet times 5 plus moon) whose moon the point is on, -1
int GalaxyMap::GetMoonAt(int theX, int theY)
{
	int aPlanetNum = 0;
	if ((int)mPlanets.size() > 0)
	{
		do
		{
			MapPlanet& aPlanet = mPlanets[aPlanetNum];
			int aMoonNum = 0;
			if ((int)aPlanet.mMoons.size() > 0)
			{
				do
				{
					MapMoon& aMoon = aPlanet.mMoons[aMoonNum];
					int aPos[2];
					GetMoonPos(aPlanet, aPos, aMoonNum);
					int aHalf = aMoon.mImage->mWidth / 2;
					int aDeltaX = (-aHalf + theX) - gApp->ScaleNum(aPos[0], 0);
					int aDeltaY = (theY + -aHalf) - gApp->ScaleNum(aPos[1], 0);
					double aDistance = sqrt((double)(aDeltaY * aDeltaY + aDeltaX * aDeltaX));
					if (aDistance <= (double)(aHalf + 2))
						return aPlanetNum * 5 + aMoonNum;
					aMoonNum++;
				} while (aMoonNum < (int)aPlanet.mMoons.size());
			}
			aPlanetNum++;
		} while (aPlanetNum < (int)mPlanets.size());
	}
	return -1;
}

// 0x5bae58
void GalaxyMap::MouseMove(int theX, int theY)
{
	int aPlanet = GetPlanetAt(theX, theY);
	int aMoon = GetMoonAt(theX, theY);
	if (aMoon == -1 && aPlanet == -1)
		mApp->SetCursor(CURSOR_POINTER);
	else
		mApp->SetCursor(CURSOR_HAND);
}

// 0x5bae99: the ring of the moon that is picked fades in and the others fade out
void GalaxyMap::Update()
{
	gSunMoonOffset[0][0] = 0x24;
	gSunMoonOffset[0][1] = 0x1c;
	gSunMoonOffset[1][0] = 0x13;
	gSunMoonOffset[1][1] = 0x59;
	gSunMoonOffset[2][0] = 0x47;
	gSunMoonOffset[2][1] = 0x7f;
	gSunMoonOffset[3][0] = 0x7b;
	gSunMoonOffset[3][1] = 0x59;
	gSunMoonOffset[4][0] = 0x68;
	gSunMoonOffset[4][1] = 0x1c;

	if (mApp->mHasFocus)
	{
		Widget::Update();
		if (!mSettingUp)
		{
			int aPlanetNum = 0;
			if ((int)mPlanets.size() > 0)
			{
				int aFirstPuzzle = 0;
				do
				{
					MapPlanet& aPlanet = mPlanets[aPlanetNum];
					int aMoonNum = 0;
					if ((int)aPlanet.mMoons.size() > 0)
					{
						int aPuzzle = aFirstPuzzle;
						do
						{
							MapMoon& aMoon = aPlanet.mMoons[aMoonNum];
							if (aPuzzle == mSelectedPuzzle)
							{
								float aGlow = (float)((double)aMoon.mGlow + (double)0.05f);
								aMoon.mGlow = aGlow;
								if (1.0 < (double)aGlow)
									aMoon.mGlow = 1.0f;
							}
							else
							{
								float aGlow = (float)((double)aMoon.mGlow - (double)0.03f);
								aMoon.mGlow = aGlow;
								if ((double)aGlow < 0.0)
									aMoon.mGlow = 0.0f;
							}
							aMoonNum++;
							aPuzzle++;
						} while (aMoonNum < (int)aPlanet.mMoons.size());
					}
					aPlanetNum++;
					aFirstPuzzle += 5;
				} while (aPlanetNum < (int)mPlanets.size());
			}
			MarkDirty();
		}
	}
}

// 0x5bb561: the pack's name, the planet's name and the puzzle's name, each in its glow font, centered on the strip
// that Draw puts the image on
void GalaxyMap::CreateFontGlowImages()
{
	mTitleGlow->Clear();
	Graphics aTitleGraphics(mTitleGlow);
	aTitleGraphics.SetColor(Color(255, 255, 255));
	aTitleGraphics.SetFont(FONT_CONTINUUM_MED_33_GLOW);
	std::string aPack = mApp->mGalaxyParser->mPuzzlePack;
	int aHalf = aTitleGraphics.GetFont()->StringWidth(aPack) / 2;
	int aX = gApp->ScaleNum(0x244, 0) - aHalf;
	int aY = gApp->ScaleNum(0x42, 0);
	aTitleGraphics.DrawString(aPack, aX - gApp->ScaleNum(0xe4, 0), aY);

	mPlanetGlow->Clear();
	Graphics aPlanetGraphics(mPlanetGlow);
	aPlanetGraphics.SetColor(Color(255, 255, 255));
	aPlanetGraphics.SetFont(FONT_CONTINUUM_MED_18_GLOW);
	aHalf = aPlanetGraphics.GetFont()->StringWidth(mPlanetName) / 2;
	aX = gApp->ScaleNum(0x244, 0) - aHalf;
	aY = gApp->ScaleNum(0x24, 0);
	aPlanetGraphics.DrawString(mPlanetName, aX - gApp->ScaleNum(0x1c1, 0), aY);

	mPuzzleGlow->Clear();
	Graphics aPuzzleGraphics(mPuzzleGlow);
	aPuzzleGraphics.SetColor(Color(255, 255, 255));
	aPuzzleGraphics.SetFont(FONT_CONTINUUM_MED_14_GLOW);
	aHalf = aPuzzleGraphics.GetFont()->StringWidth(mPuzzleName) / 2;
	aX = gApp->ScaleNum(0x94, 0) - aHalf;
	aY = gApp->ScaleNum(0x1e, 0);
	aPuzzleGraphics.DrawString(mPuzzleName, aX - gApp->ScaleNum(0x40, 0), aY);
}

// 0x5bb7f5
void GalaxyMap::Draw(Graphics* g)
{
	Graphics g2(*g);
	g2.ClipRect(gApp->ScaleNum(0xf4, 0), gApp->ScaleNum(0xca, 0), gApp->ScaleNumUp(0x2a2, 0), gApp->ScaleNumUp(0x1f6, 0));

	if (mSettingUp)
	{
		if (mY + mHeight <= gApp->ScaleNum(0x40, 0))
		{
			// (the original sleeps 2 milliseconds and draws nothing)
			return;
		}
		// (the original waits for its worker thread to finish the setup here)
	}

	g->DrawImage(mMapImage, gApp->ScaleNum(0xf2, 0), gApp->ScaleNum(0xc8, 0));

	int aPlanetNum = 0;
	if ((int)mPlanets.size() > 0)
	{
		do
		{
			MapPlanet& aPlanet = mPlanets[aPlanetNum];
			int aX = aPlanet.mX + 0xf2;
			int aY = aPlanet.mY + 200;
			if (aPlanet.mIsSun)
			{
				// the swirl behind the sun, added to the picture
				g2.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				float aFrac = (float)(aPlanetNum - 1);
				aFrac = (float)((double)aFrac / (double)(int)mPlanets.size());
				g2.SetColorizeImages(true);
				int anAlpha = FTol(((double)aFrac * 0.0 + 1.0) * 255.0);
				g2.SetColor(Color(255, 255, 255, anAlpha));
				g2.DrawImageRotated(IMAGE_GALAXY_SUN_SWIRL, gApp->ScaleNum(aX - 0x40, 0), gApp->ScaleNum(aY - 0x40, 0),
					(double)mUpdateCnt * -0.004, NULL);
				g2.SetColorizeImages(false);
				g2.SetDrawMode(Graphics::DRAWMODE_NORMAL);
			}
			if (aPlanet.mDone)
			{
				g2.DrawImageRotated(IMAGE_GALAXY_PLANET_COMPLETED, gApp->ScaleNum(aX - 0x4d, 0), gApp->ScaleNum(aY - 0x4d, 0),
					(double)mUpdateCnt * 0.004, NULL);
			}
			else if (aPlanet.mNearlyDone)
			{
				g2.DrawImageRotated(IMAGE_GALAXY_PLANET_SELECT, gApp->ScaleNum(aX - 0x21, 0), gApp->ScaleNum(aY - 0x21, 0),
					(double)mUpdateCnt * 0.003, NULL);
			}

			// the rings round the moons that glow
			int aMoonNum = 0;
			if ((int)aPlanet.mMoons.size() > 0)
			{
				do
				{
					MapMoon& aMoon = aPlanet.mMoons[aMoonNum];
					int aPos[2];
					GetMoonPos(aPlanet, aPos, aMoonNum);
					if (0.0f < aMoon.mGlow)
					{
						g2.SetColorizeImages(true);
						int anAlpha = FTol((double)aMoon.mGlow * 255.0);
						g2.SetColor(Color(255, 255, 255, anAlpha));
						g2.DrawImageRotated(IMAGE_GALAXY_MOON_SELECT, gApp->ScaleNum(aPos[0] - 0x1a, 0),
							gApp->ScaleNum(aPos[1] - 0x1a, 0), (double)mUpdateCnt * 0.01, NULL);
						g2.SetColorizeImages(false);
					}
					aMoonNum++;
				} while (aMoonNum < (int)aPlanet.mMoons.size());
			}

			g2.DrawImage(aPlanet.mImage, gApp->ScaleNum(aX, 0), gApp->ScaleNum(aY, 0));

			// the moons, and the question mark on those of an open planet that are not solved
			aMoonNum = 0;
			if ((int)aPlanet.mMoons.size() > 0)
			{
				do
				{
					MapMoon& aMoon = aPlanet.mMoons[aMoonNum];
					int aPos[2];
					GetMoonPos(aPlanet, aPos, aMoonNum);
					g2.DrawImage(aMoon.mImage, gApp->ScaleNum(aPos[0], 0), gApp->ScaleNum(aPos[1], 0));
					if (aPlanet.mOpen && !aMoon.mSolved)
					{
						g2.SetColorizeImages(true);
						int aHue = FTol((double)mUpdateCnt * 3.01) % 0xff;
						unsigned long aRGB = mApp->HSLToRGB(aHue, 0x5a, 0xaa);
						g2.SetColor(Color((int)((aRGB & 0xffffff) | 0xa0000000)));
						g2.DrawImage(IMAGE_GALAXY_QUESTION, gApp->ScaleNum(aPos[0] + 5, 0), gApp->ScaleNum(aPos[1] + 2, 0));
						g2.SetColorizeImages(false);
					}
					aMoonNum++;
				} while (aMoonNum < (int)aPlanet.mMoons.size());
			}
			aPlanetNum++;
		} while (aPlanetNum < (int)mPlanets.size());
	}

	// the frame of the map, with the names over it
	g->DrawImage(IMAGE_GALAXY_MAP, 0, 0);
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, 100));
	g->DrawImage(mTitleGlow, gApp->ScaleNum(0xe4, 0), gApp->ScaleNum(0x17, 0));
	g->SetColorizeImages(false);
	g->SetFont(FONT_CONTINUUM_MED_33);
	g->SetColor(Color(255, 255, 255));
	std::string aPack = mApp->mGalaxyParser->mPuzzlePack;
	int aHalf = g->GetFont()->StringWidth(aPack) / 2;
	g->DrawString(aPack, gApp->ScaleNum(0x244, 0) - aHalf, gApp->ScaleNum(0x59, 0));

	g->SetColorizeImages(true);
	g->SetColor(Color(0xb4, 0xb4, 0xff, 0x82));
	g->DrawImage(mPlanetGlow, gApp->ScaleNum(0x1c1, 0), gApp->ScaleNum(0x85, 0));
	g->SetColorizeImages(false);
	g->SetColor(Color(255, 255, 255));
	g->SetFont(FONT_CONTINUUM_MED_18);
	aHalf = g->GetFont()->StringWidth(mPlanetName) / 2;
	g->DrawString(mPlanetName, gApp->ScaleNum(0x244, 0) - aHalf, gApp->ScaleNum(0xa9, 0));

	g->SetColorizeImages(true);
	g->SetColor(Color(0xb4, 0xb4, 0xff, 100));
	g->DrawImage(mPuzzleGlow, gApp->ScaleNum(0x40, 0), gApp->ScaleNum(0x18c, 0));
	g->SetColorizeImages(false);
	g->SetColor(Color(255, 255, 255));
	g->SetFont(FONT_CONTINUUM_MED_14);
	aHalf = g->GetFont()->StringWidth(mPuzzleName) / 2;
	g->DrawString(mPuzzleName, gApp->ScaleNum(0x94, 0) - aHalf, gApp->ScaleNum(0x1aa, 0));

	// the planet's text
	g->SetFont(FONT_VAG_ROUNDED_11);
	g->SetColor(Color(255, 255, 255));
	int aHeight = gApp->ScaleNum(0xd6, 0);
	int aWidth = gApp->ScaleNum(0x88, 0);
	int aTop = gApp->ScaleNum(0x1d7, 0);
	int aLeft = gApp->ScaleNum(0x53, 0);
	Rect aTextRect(aLeft, aTop, aWidth, aHeight);
	WriteWordWrapped(g, aTextRect, mPlanetDesc, gApp->ScaleNum(0x14, 2), -1);

	// the gems of the puzzle that is picked, small
	int aRowY = 0xf3;
	int* aCell = &mPuzzleBoard[0][0];
	do
	{
		int aColX = 0x4f;
		do
		{
			unsigned int aGem = (unsigned int)*aCell;
			if (aGem < 10)
			{
				int aGemSize = gApp->ScaleNum(0x12, 0);
				int aGemY = gApp->ScaleNum((int)aGem * 0x12, 0);
				Rect aSrcRect(0, aGemY, aGemSize, aGemSize);   // the frame of the gem in the strip of small gems
				g->DrawImage(IMAGE_SMALL_GEMS, gApp->ScaleNum(aColX, 0), gApp->ScaleNum(aRowY, 0), aSrcRect);
			}
			aColX += 0x11;
			aCell++;
		} while (aColX < 0xd7);
		aRowY += 0x11;
	} while (aRowY < 0x17b);
}

// 0x5bc1c9: the gems and the names of a puzzle, for the panel
void GalaxyMap::LoadPuzzleData(int thePuzzle)
{
	GalaxyParser* aGalaxy = mApp->mGalaxyParser;
	std::string aDir = "puzzles\\" + aGalaxy->mPuzzlePack;
	Moon& aMoon = aGalaxy->GetPlanet(thePuzzle / 5).GetPuzzle(thePuzzle % 5);
	std::string aFile = PuzzlePath(aMoon.mFile, aDir);
	memset(mPuzzleBoard, 0xff, sizeof(mPuzzleBoard));
	// (the original also makes a Buffer here and never uses it)
	Board::LoadPuzzleThumb(aFile, mPuzzleBoard);
	mPuzzleName = aMoon.mName;
	int aPlanetNum = thePuzzle / 5;
	mPlanetName = aGalaxy->GetPlanet(aPlanetNum).mName;
	mPlanetDesc = aGalaxy->GetPlanet(aPlanetNum).mDesc;
	CreateFontGlowImages();
}

// 0x5bc2fe: a click on a moon picks its puzzle, on a planet the first puzzle of the planet (the puzzle of the picked planet
// stays); a click on the puzzle that is picked warps to it, and a planet whose predecessor has fewer than four puzzles
// solved is locked
void GalaxyMap::MouseDown(int theX, int theY, int theClickCount)
{
	int aPuzzle = GetMoonAt(theX, theY);
	int aPlanet = GetPlanetAt(theX, theY);
	if (aPuzzle == -1)
	{
		aPuzzle = mSelectedPuzzle;
		if (aPlanet != aPuzzle / 5)
			aPuzzle = aPlanet * 5;
	}
	// (the last planet, the sun, cannot be picked)
	if (aPlanet != (int)mPlanets.size() - 1 && aPuzzle >= 0)
	{
		if (aPuzzle == mSelectedPuzzle)
		{
			Warp();
		}
		else
		{
			if (aPuzzle / 5 != 0)
			{
				if (!mPlanets[aPuzzle / 5 - 1].mNearlyDone)
				{
					mApp->PlaySample(SOUND_BAD);
					mApp->DoDialog(DIALOG_FULL_INSTRUCTIONS, true, "Planet is Locked",
						"This planet will not be available until you solve 4 out of 5 puzzles on all the planets up to this one.",
						"OK", 3);
					return;
				}
			}
			mApp->PlaySample(SOUND_GALAXY_PLANET_SELECT);
			mSelectedPuzzle = aPuzzle;
			LoadPuzzleData(aPuzzle);
		}
	}
}

// 0x5bd2c2: frees the pictures of the planets and moons, and the planets
void GalaxyMap::Cleanup()
{
	int aPlanetNum = 0;
	if ((int)mPlanets.size() > 0)
	{
		do
		{
			MapPlanet& aPlanet = mPlanets[aPlanetNum];
			int aMoonNum = 0;
			if ((int)aPlanet.mMoons.size() > 0)
			{
				do
				{
					delete aPlanet.mMoons[aMoonNum].mImage;
					aMoonNum++;
				} while (aMoonNum < (int)aPlanet.mMoons.size());
			}
			delete aPlanet.mImage;
			aPlanetNum++;
		} while (aPlanetNum < (int)mPlanets.size());
	}
	mPlanets.clear();
}

// 0x5bd50a: loads the map of the pack and the picture of every planet and moon (a picture whose file is missing is a
// copy of the mask), and picks the puzzle the player is on. The pictures are the files of the puzzle pack that galaxy.xml
// names: the solved picture of a solved moon, the dark one while the planet is locked, the other one otherwise.
void GalaxyMap::Setup()
{
	Cleanup();
	GalaxyParser* aGalaxy = mApp->mGalaxyParser;
	CustomResourceManager* aResourceManager = (CustomResourceManager*)mApp->mResourceManager;
	std::string aDir = "puzzles\\" + aGalaxy->mPuzzlePack;
	std::string aMapPath = PuzzlePath(aGalaxy->mMapImage, aDir);
	// (the original overwrites the map of the last Setup without freeing it)
	delete mMapImage;
	mMapImage = aResourceManager->GetCachedImage(aMapPath);
	PuzzleProgress* aProgress = mApp->mProfile->GetPuzzleProgress();
	bool anOpen = true;
	int aPlanetNum = 0;
	if (aGalaxy->GetPlanetCount() > 0)
	{
		int aFirstPuzzle = 0;
		do
		{
			Planet& aPlanet = aGalaxy->GetPlanet(aPlanetNum);
			MapPlanet aMapPlanet;
			aMapPlanet.mIsSun = aPlanetNum == aGalaxy->GetPlanetCount() - 1;
			aMapPlanet.mX = aPlanet.mMapX;
			aMapPlanet.mY = aPlanet.mMapY;
			aMapPlanet.mOpen = anOpen;
			int aSolvedCount = 0;
			int aMoonNum = 0;
			if (aPlanet.GetPuzzleCount() > 0)
			{
				int aPuzzle = aFirstPuzzle;
				do
				{
					Moon& aMoon = aPlanet.GetPuzzle(aMoonNum);
					std::string aPath;
					MapMoon aMapMoon;
					if ((int)aProgress->mSolved.size() > aPuzzle && aProgress->mSolved[aPuzzle] != 0)
					{
						aSolvedCount++;
						aMapMoon.mSolved = true;
						aPath = PuzzlePath(aMoon.mSImage, aDir);
					}
					else
					{
						aMapMoon.mSolved = false;
						if (!anOpen)
							aPath = PuzzlePath(aMoon.mDImage, aDir);
						else
							aPath = PuzzlePath(aMoon.mUImage, aDir);
					}
					aMapMoon.mGlow = 0;
					Image* aImage = aResourceManager->GetCachedImage(aPath);
					aMapMoon.mImage = aImage;
					if (aImage == NULL)
					{
						aMapMoon.mImage = mApp->CopyImage(IMAGE_GALAXY_MOON_ALPHA);
					}
					else
					{
						Image* aMask = IMAGE_GALAXY_SUN_MOON_ALPHA;
						if (!aMapPlanet.mIsSun)
							aMask = IMAGE_GALAXY_MOON_ALPHA;
						ApplyAlphaMask(aMask, aImage);
					}
					aMapPlanet.mMoons.push_back(aMapMoon);
					aMoonNum++;
					aPuzzle++;
				} while (aMoonNum < aPlanet.GetPuzzleCount());
			}
			aMapPlanet.mNearlyDone = aSolvedCount >= 4;
			aMapPlanet.mDone = aSolvedCount == 5;
			std::string aPlanetPath;
			if (!anOpen)
				aPlanetPath = PuzzlePath(aPlanet.mDImage, aDir);
			else
				aPlanetPath = PuzzlePath(aPlanet.mAImage, aDir);
			Image* aPlanetImage = aResourceManager->GetCachedImage(aPlanetPath);
			aMapPlanet.mImage = aPlanetImage;
			if (aPlanetImage == NULL)
				aMapPlanet.mImage = mApp->CopyImage(IMAGE_GALAXY_PLANET_ALPHA);
			else
				ApplyAlphaMask(IMAGE_GALAXY_PLANET_ALPHA, aPlanetImage);
			anOpen = aMapPlanet.mNearlyDone;
			mPlanets.push_back(aMapPlanet);
			aPlanetNum++;
			aFirstPuzzle += 5;
		} while (aPlanetNum < aGalaxy->GetPlanetCount());
	}
	mSelectedPuzzle = aProgress->mPuzzleNum;
	LoadPuzzleData(mSelectedPuzzle);
	mSettingUp = false;
}

// 0x5bd97e
void GalaxyMap::SetupStub(void* theMap)
{
	((GalaxyMap*)theMap)->Setup();
}

// 0x5bd98a
void GalaxyMap::StartSetup()
{
	mSettingUp = true;
	mApp->mWorker->DoTask(SetupStub, this);
}

// 0x5ba705
void GalaxyMap::KeyChar(SexyChar theChar)
{
}

// 0x5ba6f2
void GalaxyMap::MouseLeave()
{
}

// 0x5ba6f3
void GalaxyMap::MouseUp(int theX, int theY, int theClickCount)
{
	Widget::MouseUp(theX, theY, theClickCount);
}

// 0x5ba6fc
void GalaxyMap::MouseDrag(int theX, int theY)
{
	Widget::MouseDrag(theX, theY);
}
