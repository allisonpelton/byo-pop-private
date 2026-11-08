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
#include "SoundManager.h"
#include "SoundInstance.h"
#include <math.h>
#include <stdio.h>
#include <ctype.h>
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

// ---- the panel -------------------------------------------------------------------------------------------------

IslandPanel::IslandPanel(PixelusApp* theApp, IslandMap* theIsland)
{
	mApp = theApp;
	mIsland = theIsland;
	mIndex = -1;
	mIndex2 = 0;
	mFadeState = 1;
	mFade = 0;
	Image* aPlate = mApp->mImages[IMAGE_TEMPLEINFOBACK];
	Widget::Resize(10, mApp->mHeight - aPlate->GetHeight() - 4, aPlate->GetWidth(), aPlate->GetHeight());
}

// 0x414750
void IslandPanel::Update()
{
	Widget::Update();
	if (mFadeState == 0)
	{
		if (mFade < 255)
		{
			mFade = std::min(255, mFade + 10);
			MarkDirty();
		}
	}
	else if (mFade > 0)
	{
		mFade = std::max(0, mFade - 10);
		MarkDirty();
	}
}

// 0x4147e0
void IslandPanel::Set(int theIndex, int theIndex2)
{
	if (theIndex != mIndex || mFadeState == 1)
	{
		mIndex = theIndex;
		mIndex2 = theIndex2;
		mFade = 0;
		mFadeState = 0;
	}
}

// 0x414710
void IslandPanel::Clear()
{
	mFadeState = 1;
}

void IslandPanel::DrawCentered(Graphics* g, int theFont, const std::string& theText, int theY)
{
	Font* aFont = mApp->mFonts[theFont];
	g->SetFont(aFont);
	g->DrawString(theText, (mWidth - aFont->StringWidth(theText)) / 2, theY);
}

// 0x414910: a bar with the text under it, centred on the plate
void IslandPanel::DrawBar(Graphics* g, int theY, int theSolved, int theCount, const char* theFormat)
{
	Image* anEmpty = mApp->mImages[IMAGE_PROGRESSEMPTY];
	Image* aFull = mApp->mImages[IMAGE_PROGRESSFULL];
	int aX = mWidth / 2 - 0x50;
	int aBarY = theY - 0x14;
	int aFilled = theCount > 0 ? (aFull->GetWidth() - 6) * theSolved / theCount : 0;
	g->SetColor(Color(255, 255, 255, mFade));
	g->DrawImage(anEmpty, aX, aBarY);
	g->DrawImage(aFull, aX, aBarY, Rect(0, 0, aFilled + 3, aFull->GetHeight()));
	// the end of the bar fades out over eight columns
	for (int i = 0; i < 8; i++)
	{
		int anAlpha = (int) (cos(i * 0.19635) * 255.0 * (mFade / 255.0));
		if (aFilled + 3 + i >= aFull->GetWidth() || anAlpha <= 0)
			continue;
		g->SetColor(Color(255, 255, 255, anAlpha));
		g->DrawImage(aFull, aX + aFilled + 3 + i, aBarY, Rect(aFilled + 3 + i, 0, 1, aFull->GetHeight()));
	}
	char aText[80];
	snprintf(aText, sizeof(aText), theFormat, theSolved, theCount);
	g->SetColor(Color(255, 255, 255, mFade));
	DrawCentered(g, FONT_GARAMOND14, aText, theY);
}

// 0x414bf0
void IslandPanel::Draw(Graphics* g)
{
	UserProfile* aUser = mApp->mUserDirectory->GetCurrent();
	g->SetColorizeImages(false);
	g->DrawImage(mApp->mImages[IMAGE_TEMPLEINFOBACK], 0, 0);
	g->SetColorizeImages(true);

	// the island's own page
	g->SetColor(Color(255, 255, 255, 255 - mFade));
	DrawCentered(g, FONT_GARAMOND14, GT(0x50de18), 0x1a);
	DrawCentered(g, FONT_GARAMOND24GOLD, GT(0x50de10), 0x3d);
	int aOutside = aUser != NULL ? aUser->CountSolved() - aUser->CountSolved(gTemples[0].mName) : 0;
	if (aUser != NULL && aUser->mFlag79)
	{
		DrawCentered(g, FONT_GARAMOND20, IsTempleLocked(mApp, 1) ? GT(0x50ddac) : GT(0x50ddc0), 0x65);
		for (int i = 0; i < NUM_TEMPLES; i++)
			if (IsTempleLocked(mApp, i))
			{
				char aBuffer[80];
				DrawCentered(g, FONT_GARAMOND20, GT(0x50dd98), 0x92);
				snprintf(aBuffer, sizeof(aBuffer), GTC(0x50dd88), gTemples[i].mUnlock);
				DrawCentered(g, FONT_GARAMOND20, aBuffer, 0xab);
				break;
			}
		int aTotal = 0;
		for (int i = 1; i < NUM_TEMPLES; i++)
			aTotal += mApp->mLevelData->GetPuzzleCount(gTemples[i].mName);
		char aBuffer[80];
		snprintf(aBuffer, sizeof(aBuffer), GTC(0x50dd6c), aOutside, aTotal);
		DrawCentered(g, FONT_GARAMOND12, aBuffer, 0xc4);
	}
	else
	{
		DrawCentered(g, FONT_GARAMOND20, GT(0x50ddfc), 0x6f);
		DrawCentered(g, FONT_GARAMOND20, GT(0x50dde8), 0x92);
		DrawCentered(g, FONT_GARAMOND20, GT(0x50ddd4), 0xab);
	}

	if (mIndex == -1 || mFade == 0)
		return;

	// the page of the temple under the mouse
	g->SetColor(Color(255, 255, 255, mFade));
	const char* aName = gTemples[mIndex2].mName;
	std::string anUpper = aName;
	for (size_t i = 0; i < anUpper.size(); i++)
		anUpper[i] = (char) toupper((unsigned char) anUpper[i]);
	if (mIndex2 == 0)
	{
		DrawCentered(g, FONT_GARAMOND24GOLD, anUpper, 0x30);
		DrawCentered(g, FONT_GARAMOND20, GT(0x50dd5c), 0x67);
		DrawCentered(g, FONT_GARAMOND20, GT(0x50dd4c), 0x80);
		DrawCentered(g, FONT_GARAMOND20, GT(0x50dd40), 0x99);
		DrawCentered(g, FONT_GARAMOND20, GT(0x50dd34), 0xb2);
		return;
	}
	DrawCentered(g, FONT_GARAMOND14, GT(0x50dbd0), 0x1a);
	DrawCentered(g, FONT_GARAMOND24GOLD, anUpper, 0x3d);
	Font* aSmall = mApp->mFonts[FONT_GARAMOND14];
	g->SetFont(aSmall);
	g->DrawString(GT(0x50dd28), 0x34, 0x6f);
	int aStarX = aSmall->StringWidth(GT(0x50dd28)) + 0x40;
	int aFull = (mIndex2 + 1) / 2;
	for (int i = 0; i < 5; i++)
		g->DrawImage(mApp->mImages[i < aFull ? IMAGE_DIFFSTARFULL : IMAGE_DIFFSTAROPEN], aStarX + i * 16, 0x5f);
	if ((mIndex2 + 1) & 1)
		g->DrawImage(mApp->mImages[IMAGE_DIFFSTARFULL], aStarX + aFull * 16, 0x5f, Rect(0, 0, 9, 15));

	int aCount = mApp->mLevelData->GetPuzzleCount(aName);
	if (IsTempleLocked(mApp, mIndex2))
	{
		char aBuffer[80];
		DrawCentered(g, FONT_GARAMOND20, GT(0x50dd20), 0x92);
		snprintf(aBuffer, sizeof(aBuffer), GTC(0x50dd88), gTemples[mIndex2].mUnlock);
		DrawCentered(g, FONT_GARAMOND20, aBuffer, 0xab);
		snprintf(aBuffer, sizeof(aBuffer), "%d out of %d", aOutside, gTemples[mIndex2].mUnlock);
		DrawCentered(g, FONT_GARAMOND12, aBuffer, 0xc4);
	}
	else
	{
		DrawBar(g, 0x92, aUser != NULL ? aUser->CountSolved(aName) : 0, aCount, GTC(0x50dd10));
		DrawBar(g, 0xb0, aUser != NULL ? aUser->CountMaxed(aName) : 0, aCount, GTC(0x50dd00));
	}
}

// ---- the island ------------------------------------------------------------------------------------------------

double IslandMap::Rand01()
{
	return mApp->AppRandNext() * 4.656612875245797e-10;
}

// 0x4176a0
IslandMap::IslandMap(PixelusApp* theApp)
{
	mApp = theApp;
	mTicks = 0;
	mSeaFrame = 0;
	mSmokeFrame = 0;
	mSeaBlend = 1.0;
	mPhase = 0;
	mCandidate = -1;
	mDwell = 0;
	mShown = -1;
	mLightning = Rand01() * 5.0;
	mLightningImage = 0;
	mSinPhase = 0;
	mActive = false;
	mWaveFlag = 0;
	mPanel = NULL;
	for (int i = 0; i < NUM_TEMPLES; i++)
	{
		mHotspots[i] = NULL;
		mState[i] = STATE_HIDDEN;
		mGlow[i] = 0;
	}

	mBackButton = MakeStripButton(mApp, this, ID_BACK, IMAGE_BACKTOMAINMENU, 6, -3);
	mPanel = new IslandPanel(mApp, this);

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
	mSolvedTotal = aUser != NULL ? aUser->CountSolved() : 0;
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
				mActive = true;
				// the first time the temple shows, its door opening is heard
				if (aUser != NULL && i >= 1 && i <= 8 && !(aFlags & 2))
				{
					aUser->mTempleFlags[aName] |= 2;
					// 0x4176a0: the bursts at its door (Rumble, PauseBloom, PauseBloomUp: not ported yet) bring it in
					// after the first 1.1 s, so until then it is state 1, which is not drawn
					mState[i] = STATE_OPEN;
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
	mNextGull1 = mApp->AppRandNext() % 100 + 350 + mApp->mUpdateCount;
	mNextGull2 = mApp->AppRandNext() % 100 + 400 + mApp->mUpdateCount;
}

IslandMap::~IslandMap()
{
	delete mBackButton;
	delete mPanel;
	for (int i = 0; i < NUM_TEMPLES; i++)
		delete mHotspots[i];
	delete mBackground;
}

// 0x416300
void IslandMap::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mBackButton);
	theWidgetManager->AddWidget(mPanel);
	for (int i = 0; i < NUM_TEMPLES; i++)
		if (mHotspots[i] != NULL)
			theWidgetManager->AddWidget(mHotspots[i]);
}

// 0x416370
void IslandMap::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mBackButton);
	theWidgetManager->RemoveWidget(mPanel);
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

// 0x4163e0
void IslandMap::Update()
{
	Widget::Update();
	mPhase += 0.01;

	int aCandidate = -1;
	for (int i = 0; i < NUM_TEMPLES; i++)
	{
		if (mHotspots[i] == NULL)
			continue;
		if (mHotspots[i]->mIsOver)
		{
			mGlow[i] = std::min(1.0, mGlow[i] + 0.02);
			if (aCandidate == -1)
				aCandidate = i;
		}
		else
			mGlow[i] = std::max(0.0, mGlow[i] - 0.02);
	}

	if (aCandidate == mCandidate)
	{
		mDwell++;
		if (mDwell == 50 && aCandidate != -1)
		{
			SoundInstance* anInstance = mApp->mSoundManager->GetSoundInstance(SOUND_UI_ISLAND_MOUSEOVER_02);
			if (anInstance != NULL)
			{
				anInstance->AdjustPitch(aCandidate - 7.0);
				anInstance->Play(false, true);
			}
		}
	}
	else
	{
		mCandidate = aCandidate;
		mDwell = 0;
	}
	if (mCandidate != mShown && (mCandidate < 0 || mDwell > 20))
	{
		mShown = mCandidate;
		if (mShown < 0)
			mPanel->Clear();
		else
			mPanel->Set(mShown, aCandidate);
	}

	if (mApp->mUpdateCount >= mNextGull1)
	{
		mNextGull1 = mApp->AppRandNext() % 5000 + 600 + mApp->mUpdateCount;
		mApp->PlaySample(SOUND_SEAGULL_05);
	}
	if (mApp->mUpdateCount >= mNextGull2)
	{
		mNextGull2 = mApp->AppRandNext() % 3000 + 600 + mApp->mUpdateCount;
		mApp->PlaySample(SOUND_SEAGULL_04B);
	}

	if (mActive)
		mSinPhase += 0.031415927;

	// the sea and the smoke cross-fade into the background
	mSeaBlend += 0.01;
	if (mSeaBlend >= 1.0)
	{
		Image* aSea = mApp->mImages[IMAGE_ISLANDMAPSEA];
		Image* aSmoke = mApp->mImages[IMAGE_ISLANDMAPSMOKE];
		int aSeaHeight = aSea->GetHeight() / SEA_FRAMES;
		int aSmokeHeight = aSmoke->GetHeight() / SMOKE_FRAMES;
		{
			Graphics g(mBackground);
			g.DrawImage(aSea, 0, 0x1e0 - aSeaHeight, Rect(0, mSeaFrame * aSeaHeight, aSea->GetWidth(), aSeaHeight));
			g.DrawImage(aSmoke, 0x1c6, 0, Rect(0, mSmokeFrame * aSmokeHeight, aSmoke->GetWidth(), aSmokeHeight));
		}
		mSeaBlend = 0;
		mSeaFrame = (mSeaFrame + 1) & 3;
		mSmokeFrame = (mSmokeFrame + 1) % SMOKE_FRAMES;
		if (mSeaFrame == 0)
		{
			mWaveFlag ^= 1;
			if (mWaveFlag == 1)
				mApp->PlaySample(SOUND_ISLAND_OCEAN_SURF_04B);
		}
	}

	// lightning
	if (mLightning > 0)
	{
		mLightning -= 0.01;
		if (mLightning <= 0)
		{
			mLightning = -0.1;
			mLightningImage = mApp->AppRandNext() % 3;
			int aRand = mApp->AppRandNext();
			SoundInstance* anInstance = mApp->mSoundManager->GetSoundInstance(0xe | (~aRand & 1));
			if (anInstance != NULL)
			{
				anInstance->SetBaseVolume(Rand01() + 1.0);
				anInstance->Play(false, true);
			}
		}
	}
	else
	{
		mLightning += 0.01;
		if (mLightning >= 0)
			mLightning = (45.0 - 10) * Rand01();
	}
	MarkDirty();
}

// 0x416950
void IslandMap::Draw(Graphics* g)
{
	g->DrawImage(mBackground, 0, 0);

	if (mSeaBlend > 0)
	{
		Image* aSea = mApp->mImages[IMAGE_ISLANDMAPSEA];
		Image* aSmoke = mApp->mImages[IMAGE_ISLANDMAPSMOKE];
		int aSeaHeight = aSea->GetHeight() / SEA_FRAMES;
		int aSmokeHeight = aSmoke->GetHeight() / SMOKE_FRAMES;
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, (int) (mSeaBlend * 255)));
		g->DrawImage(aSea, 0, 0x1e0 - aSeaHeight, Rect(0, mSeaFrame * aSeaHeight, aSea->GetWidth(), aSeaHeight));
		g->DrawImage(aSmoke, 0x1c6, 0, Rect(0, mSmokeFrame * aSmokeHeight, aSmoke->GetWidth(), aSmokeHeight));
		g->SetColorizeImages(false);
	}

	if (mLightning < 0)
	{
		static const int aLightningX[3] = { 250, 300, 300 };
		g->DrawImage(mApp->mImages[62 + mLightningImage], aLightningX[mLightningImage], 0);
	}

	for (int i = 0; i < NUM_TEMPLES; i++)
	{
		int aState = mState[i];
		if (aState == STATE_HIDDEN || (aState == STATE_OPEN && mPhase < 1.1))
			continue;
		Image* aPicture = mApp->mImages[gTemples[i].mImage];
		int aHalf = aPicture->GetWidth() / 2;
		int aSide = (aState == STATE_DONE || aState == STATE_MAXED) ? 1 : 0;
		g->DrawImage(aPicture, gTemples[i].mX, gTemples[i].mY, Rect(aHalf * aSide, 0, aHalf, aPicture->GetHeight()));
		if (aState == STATE_MAXED && i != 0)
			g->DrawImage(mApp->mImages[IMAGE_MEDAL], gTemples[i].mLightX - 3, gTemples[i].mLightY - 10);
	}

	double aPulse = (sin(mSinPhase) + 1.0) * 0.5;
	for (int i = 0; i < NUM_TEMPLES; i++)
	{
		if (mHotspots[i] == NULL)
			continue;
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		if (mGlow[i] * 0.4 > 0)
		{
			Image* aRight = mApp->mImages[IMAGE_DIVINELIGHTRIGHT];
			g->SetColor(Color(255, 255, 255, (int) (mGlow[i] * 0.4 * 255)));
			g->DrawImage(aRight, gTemples[i].mLightX - aRight->GetWidth() / 2, gTemples[i].mLightY - 0x69);
		}
		if ((mState[i] == STATE_OPEN || mState[i] == STATE_NEW) && !IsTempleLocked(mApp, i))
		{
			Image* aLight = mApp->mImages[IMAGE_DIVINELIGHT];
			g->SetColor(Color(255, 255, 255, (int) ((aPulse * 0.4 + 0.2) * 255)));
			g->DrawImage(aLight, gTemples[i].mLightX - aLight->GetWidth() / 2, gTemples[i].mLightY - 0xaa);
		}
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColorizeImages(false);
	}
	for (int i = 0; i < NUM_TEMPLES; i++)
	{
		if (mHotspots[i] == NULL || !((mState[i] == STATE_OPEN || mState[i] == STATE_NEW) && !IsTempleLocked(mApp, i)))
			continue;
		Image* anArrow = mApp->mImages[IMAGE_TEMPLEARROW];
		int aBounce = (int) (fabs(sin(mPhase * 3.0)) * 24.0);
		g->DrawImage(anArrow, gTemples[i].mLightX - aBounce - anArrow->GetWidth() / 2 - 0x14, gTemples[i].mLightY - aBounce - 0x50);
	}
}

// 0x416180: a sound when the mouse comes onto a temple
void IslandMap::ButtonMouseEnter(int theId)
{
	if (theId >= 2 && theId < 2 + NUM_TEMPLES)
		mApp->PlaySample(SOUND_UI_ISLAND_CLICK_01);
}

void IslandMap::ButtonMouseLeave(int theId)
{
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
