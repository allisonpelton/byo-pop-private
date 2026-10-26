#include "IslandMap.h"
#include "PixelusApp.h"
#include "GameDialog.h"
#include "GameText.h"
#include "LevelData.h"
#include "UserDirectory.h"
#include "Graphics.h"
#include "Image.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "DialogButton.h"
#include "ButtonWidget.h"
#include "WidgetManager.h"
#include <math.h>
#include <stdio.h>
#include <algorithm>

using namespace Sexy;

namespace
{
// the places on the map that can be clicked: nothing to draw, the temple is part of the picture
class TempleHotspot : public ButtonWidget
{
public:
	TempleHotspot(int theId, ButtonListener* theListener) : ButtonWidget(theId, theListener)
	{
		mDoFinger = true;
	}
	virtual void Draw(Graphics* g) override {}
};
}

// 0x4176a0
IslandMap::IslandMap(PixelusApp* theApp)
{
	mApp = theApp;
	mTicks = 0;
	mSeaFrame = 0;
	mSmokeFrame = 0;
	mHover = -1;
	for (int i = 0; i < NUM_TEMPLES; i++)
	{
		mHotspots[i] = NULL;
		mState[i] = STATE_HIDDEN;
	}

	mBackButton = MakeStripButton(mApp, this, ID_BACK, IMAGE_BACKTOMAINMENU, 6, -3);

	// the sky over the island
	Image* aSky = mApp->mImages[IMAGE_ISLANDMAPSKY];
	Image* anIsland = mApp->mImages[IMAGE_ISLANDMAP];
	mBackground = new MemoryImage(mApp);
	mBackground->Create(anIsland->GetWidth(), aSky->GetHeight() + anIsland->GetHeight());
	mBackground->SetImageMode(true, true);
	{
		Graphics g(mBackground);
		g.DrawImage(aSky, 0, 0);
		g.DrawImage(anIsland, 0, aSky->GetHeight());
	}

	UserProfile* aUser = mApp->mUserDirectory->GetCurrent();
	for (int i = 0; i < NUM_TEMPLES; i++)
	{
		// a temple that is not open yet is not on the map, except the last one, which waits there for the end
		if (!IsTempleOpen(mApp, i) && i != NUM_TEMPLES - 1)
			continue;

		const char* aName = gTemples[i].mName;
		int aSolved = aUser != NULL ? aUser->CountSolved(aName) : 0;
		int aMaxed = aUser != NULL ? aUser->CountMaxed(aName) : 0;
		int aCount = mApp->mLevelData->GetPuzzleCount(aName);

		if (aSolved < aCount)
		{
			int aFlags = aUser != NULL ? aUser->mTempleFlags[aName] : 4;
			if (!(aFlags & 4))
			{
				mState[i] = STATE_NEW;
				// the first time the temple shows, its door opening is heard
				if (aUser != NULL && i >= 1 && i <= 8 && !(aFlags & 2))
				{
					aUser->mTempleFlags[aName] |= 2;
					mApp->PlaySample(SOUND_PIXELUS_TEMPLE_SCREEN_V01);
				}
			}
			else
				mState[i] = STATE_OPEN;
		}
		else
			mState[i] = aMaxed < aCount ? STATE_DONE : STATE_MAXED;

		Image* aPicture = mApp->mImages[gTemples[i].mImage];
		ButtonWidget* aHotspot = new TempleHotspot(i + 2, this);
		aHotspot->Resize(gTemples[i].mX, gTemples[i].mY, aPicture->GetWidth() / 2, aPicture->GetHeight());
		mHotspots[i] = aHotspot;
	}
}

IslandMap::~IslandMap()
{
	delete mBackButton;
	for (int i = 0; i < NUM_TEMPLES; i++)
		delete mHotspots[i];
	delete mBackground;
}

// 0x416300
void IslandMap::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mBackButton);
	for (int i = 0; i < NUM_TEMPLES; i++)
		if (mHotspots[i] != NULL)
			theWidgetManager->AddWidget(mHotspots[i]);
}

// 0x416370
void IslandMap::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mBackButton);
	for (int i = 0; i < NUM_TEMPLES; i++)
		if (mHotspots[i] != NULL)
			theWidgetManager->RemoveWidget(mHotspots[i]);
}

// 0x416280
void IslandMap::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mBackButton->Resize(theX + 6, theY - 3, 0x7d, mApp->mImages[IMAGE_BACKTOMAINMENU]->GetHeight());
}

void IslandMap::Update()
{
	Widget::Update();
	mTicks++;
	if (mTicks % SEA_TICKS == 0)
		mSeaFrame = (mSeaFrame + 1) % SEA_FRAMES;
	if (mTicks % 20 == 0)
		mSmokeFrame = (mSmokeFrame + 1) % SMOKE_FRAMES;
	MarkDirty();
}

void IslandMap::DrawTemples(Graphics* g)
{
	int aFade = std::max(0, std::min(255, (mTicks - 60) * 255 / (APPEAR_TICKS - 60)));
	for (int i = 0; i < NUM_TEMPLES; i++)
	{
		int aState = mState[i];
		if (aState == STATE_HIDDEN)
			continue;
		Image* aPicture = mApp->mImages[gTemples[i].mImage];
		int aHalf = aPicture->GetWidth() / 2;
		bool aComplete = aState >= STATE_DONE;
		Rect aSource(aComplete ? aHalf : 0, 0, aHalf, aPicture->GetHeight());

		// the temples that are simply open come in a moment after the rest
		if (aState == STATE_OPEN && aFade < 255)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, aFade));
		}
		g->DrawImage(aPicture, gTemples[i].mX, gTemples[i].mY, aSource);
		g->SetColorizeImages(false);

		if (aState == STATE_MAXED && i != 0)
			g->DrawImage(mApp->mImages[IMAGE_MEDAL], gTemples[i].mX + aHalf - 40, gTemples[i].mY - 8);
	}

	// a glow and an arrow over every open temple that has puzzles left
	double aPulse = (sin(mTicks * 0.04) + 1.0) * 0.5;
	for (int i = 0; i < NUM_TEMPLES; i++)
	{
		if ((mState[i] != STATE_OPEN && mState[i] != STATE_NEW) || IsTempleLocked(mApp, i))
			continue;
		Image* aLight = mApp->mImages[IMAGE_DIVINELIGHT];
		int anAlpha = (int) ((aPulse * 0.4 + 0.2) * 255);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, anAlpha));
		g->DrawImage(aLight, gTemples[i].mLightX - aLight->GetWidth() / 2, gTemples[i].mLightY - 0xaa);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColorizeImages(false);

		Image* anArrow = mApp->mImages[IMAGE_TEMPLEARROW];
		int aBounce = (int) (sin(mTicks * 0.09) * 5.0);
		g->DrawImage(anArrow, gTemples[i].mLightX - anArrow->GetWidth() / 2, gTemples[i].mY - anArrow->GetHeight() - 2 + aBounce);
	}
}

// the note that names the temple under the mouse
void IslandMap::DrawLabel(Graphics* g, int theTemple)
{
	UserProfile* aUser = mApp->mUserDirectory->GetCurrent();
	const char* aName = gTemples[theTemple].mName;
	std::string aTitle = aName;
	if (theTemple != 0)
		aTitle = GT(0x50dbd0) + " " + aTitle;
	char aCount[48];
	int aSolved = aUser != NULL ? aUser->CountSolved(aName) : 0;
	snprintf(aCount, sizeof(aCount), "%d / %d", aSolved, mApp->mLevelData->GetPuzzleCount(aName));

	Font* aFont = mApp->mFonts[FONT_GARAMOND14];
	int aWidth = std::max(aFont->StringWidth(aTitle), aFont->StringWidth(aCount)) + 20;
	int aX = std::max(2, std::min(mWidth - aWidth - 2, gTemples[theTemple].mLightX - aWidth / 2));
	int aY = std::max(2, gTemples[theTemple].mY - 46);
	g->SetColor(Color(0, 0, 0, 150));
	g->FillRect(aX, aY, aWidth, 36);
	g->SetFont(aFont);
	g->SetColor(Color(0xff, 0xf2, 0x5c));
	g->DrawString(aTitle, aX + (aWidth - aFont->StringWidth(aTitle)) / 2, aY + 15);
	g->SetColor(Color(255, 255, 255));
	g->DrawString(aCount, aX + (aWidth - aFont->StringWidth(aCount)) / 2, aY + 31);
}

// 0x4163e0
void IslandMap::Draw(Graphics* g)
{
	g->DrawImage(mBackground, 0, 0);

	Image* aSea = mApp->mImages[IMAGE_ISLANDMAPSEA];
	int aSeaHeight = aSea->GetHeight() / SEA_FRAMES;
	g->DrawImage(aSea, 0, 0x1e0 - aSeaHeight, Rect(0, mSeaFrame * aSeaHeight, aSea->GetWidth(), aSeaHeight));
	Image* aSmoke = mApp->mImages[IMAGE_ISLANDMAPSMOKE];
	int aSmokeHeight = aSmoke->GetHeight() / SMOKE_FRAMES;
	g->DrawImage(aSmoke, 0x1c6, 0, Rect(0, mSmokeFrame * aSmokeHeight, aSmoke->GetWidth(), aSmokeHeight));

	DrawTemples(g);
	if (mHover != -1)
		DrawLabel(g, mHover);
}

// 0x416180: a sound when the mouse comes onto a temple
void IslandMap::ButtonMouseEnter(int theId)
{
	if (theId >= 2 && theId < 2 + NUM_TEMPLES)
	{
		mHover = theId - 2;
		mApp->PlaySample(SOUND_UI_ISLAND_MOUSEOVER_02);
		MarkDirty();
	}
}

void IslandMap::ButtonMouseLeave(int theId)
{
	if (theId - 2 == mHover)
	{
		mHover = -1;
		MarkDirty();
	}
}

// 0x4170d0: the way back, or a temple: locked ones say how many puzzles they ask for, open ones are entered
void IslandMap::ButtonDepress(int theId)
{
	if (theId == ID_BACK)
	{
		mApp->BackToMainMenu();
		return;
	}
	if (theId < 2 || theId >= 2 + NUM_TEMPLES)
		return;

	int aTemple = theId - 2;
	mApp->PlaySample(SOUND_UI_ISLAND_CLICK_01);
	if (IsTempleLocked(mApp, aTemple))
	{
		char aBuffer[160];
		Dialog* aDialog;
		if (aTemple == NUM_TEMPLES - 1)
		{
			snprintf(aBuffer, sizeof(aBuffer), GTC(0x50e040), gTemples[aTemple].mUnlock);
			aDialog = mApp->DoDialog(0x1c7, true, GT(0x50e028), aBuffer, GT(0x50b558), Dialog::BUTTONS_FOOTER);
		}
		else
		{
			snprintf(aBuffer, sizeof(aBuffer), GTC(0x50dfe0), gTemples[aTemple].mUnlock);
			aDialog = mApp->DoDialog(0x1c7, true, GT(0x50dfd0), aBuffer, GT(0x50b558), Dialog::BUTTONS_FOOTER);
		}
		mApp->CenterDialog(aDialog);
		return;
	}

	if (mApp->mLevelData->GetLevel(gTemples[aTemple].mName) != NULL)
		mApp->EnterTemple(aTemple);
}
