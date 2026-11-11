// The full-screen widgets of Atomica Deluxe (see Screens.h).

#include "Screens.h"
#include "MusicInterface.h"
#include "AtomicaApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"
#include "Support.h"

#include <cmath>
#include <cstdio>

using namespace Sexy;

// ---------------------------------------------------------------------------------------------------------
// TitleScreen

// 0x42fe90
TitleScreen::TitleScreen(AtomicaApp* theApp) :
	mApp(theApp),
	mProgress(0.0),
	mLinkShown(false),
	mSlide(-800.0f),
	mBlink(0)
{
	mTitle = mApp->mSprites->Load("titlescreen");
	mBar = mApp->mSprites->Load("progressbar");
	mLogo = mApp->mSprites->Load("atomicatitle");
	mDeluxe = mApp->mSprites->Load("deluxe");
	mPopCapLogo = mApp->mSprites->Load("popcaplogo");
	mPopCapUrl = mApp->mSprites->Load("popcapurl");
	mSponsor = mApp->mSprites->Load("sponsor");

	// port: the shareware branches (the free plays, the register link) are not ported; the screen always takes the
	// registered path (docs/GAME_NOTES.md section 3)
	mPlayLink = new HyperlinkWidget(1, this);
	mPlayLink->SetFont(mApp->mAtomicFont);
	mPlayLink->mColor = Color(0xff, 0xff, 0);
	mPlayLink->mLabel = "";
	mPlayLink->SetVisible(false);
	mHasTransparencies = false;
	// 0x42fe90: the title's own tune (order 6 of the song), until the menu starts its
	mApp->mMusicInterface->PlayMusic(0, 6, false);
}

// 0x42fd10: the title's tune stops with it
TitleScreen::~TitleScreen()
{
	if (mApp->mMusicInterface != NULL)
		mApp->mMusicInterface->StopMusic(0);
	delete mPlayLink;
}

// 0x42fb30 / 0x42fb70
void TitleScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mPlayLink);
}

// 0x42fb70
void TitleScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mPlayLink);
}

// 0x42fbb0
void TitleScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	int aWidth = mPlayLink->mFont->StringWidth(mPlayLink->mLabel);
	mPlayLink->Resize((mWidth - aWidth) / 2, 0x1b1, aWidth, 0x18);
}

// 0x430bd0
void TitleScreen::Update()
{
	Widget::Update();

	float aSlide = mSlide;
	mSlide = aSlide * 0.9f;
	if (-3.0f < aSlide * 0.9f)
		mSlide = 0;

	// the bar's fill follows the loading
	if (mUpdateCnt % 4 == 0)
	{
		mProgress = mApp->GetLoadingThreadProgress();
		if ((mProgress == 1.0) && mApp->mLoaded && !mLinkShown)
		{
			mPlayLink->mLabel = GT(0x004b2cc0);
			mPlayLink->SetVisible(true);
			mLinkShown = true;
			mProgress = 1.0;
			int aWidth = mPlayLink->mFont->StringWidth(mPlayLink->mLabel);
			mPlayLink->Resize((mWidth - aWidth) / 2, 0x1b1, aWidth, 0x18);
		}
		MarkDirty();
	}

	// 0x430bd0: the logo's letters are a white sprite that the update repaints every 20th update with three triangle
	// waves between 150 and 255, one per colour channel, that run down the rows (steps of 2, 3 and 4 a row), and that
	// move on by 5 steps of 1, 0.5 and 1/3 each time. The values are the original's globals (0x4cbe18-0x4cbe28, 0x4d40e0).
	static float sB = 0.0f, sBDir = 1.0f;			// 0x4d40e0, 0x4cbe1c
	static float sG = 255.0f, sGDir = -1.0f;		// 0x4cbe28, 0x4cbe20
	static float sR = 255.0f, sRDir = -1.0f;		// 0x4cbe24, 0x4cbe18
	if (mBlink == 0)
	{
		float aB = sB, aG = sG, aBDir = sBDir, aGDir = sGDir, aRDir = sRDir;
		double aR = sR;
		MemoryImage* anImage = (MemoryImage*) mLogo->mImage;
		unsigned char* aBits = (unsigned char*) anImage->GetBits();
		for (int aRow = 0; aRow < anImage->mHeight; aRow++)
		{
			unsigned char aFirst = (unsigned char) (int) aB, aSecond = (unsigned char) (int) aG, aThird = (unsigned char) (int) aR;
			for (int aCol = 0; aCol < anImage->mWidth; aCol++)
			{
				// the original's buffer holds them in the order red, green, blue (the first wave is the red one)
				aBits[2] = aFirst;
				aBits[1] = aSecond;
				aBits[0] = aThird;
				aBits += 4;
			}
			aB = aB + aBDir * 2.0f;
			aG = aG + aGDir * 3.0f;
			aR = aR + 4.0 * aRDir;
			if (aB >= 255.0f) { aB = 255.0f; aBDir = -1.0f; }
			else if (aB <= 150.0f) { aB = 150.0f; aBDir = 1.0f; }
			if (aG >= 255.0f) { aG = 255.0f; aGDir = -1.0f; }
			else if (aG <= 150.0f) { aG = 150.0f; aGDir = 1.0f; }
			if (aR >= 255.0) { aR = 255.0; aRDir = -1.0f; }
			else if (aR <= 150.0) { aR = 150.0; aRDir = 1.0f; }
		}
		anImage->BitsChanged();
		for (int i = 0; i < 5; i++)
		{
			sB = sBDir + sB;
			sG = sGDir * 0.5f + sG;
			sR = sRDir * 0.33333334f + sR;
			if (sB >= 255.0f) { sB = 255.0f; sBDir = -1.0f; }
			else if (sB <= 150.0f) { sB = 150.0f; sBDir = 1.0f; }
			if (sG >= 255.0f) { sG = 255.0f; sGDir = -1.0f; }
			else if (sG <= 150.0f) { sG = 150.0f; sGDir = 1.0f; }
			if (sR >= 255.0f) { sR = 255.0f; sRDir = -1.0f; }
			else if (sR <= 150.0f) { sR = 150.0f; sRDir = 1.0f; }
		}
	}
	if (++mBlink > 19)
		mBlink = 0;
	MarkDirty();
}

// 0x430450
void TitleScreen::Draw(Graphics* g)
{
	mTitle->Draw(g, 0, 0);
	mLogo->Draw(g, 0x10, 8);
	mDeluxe->Draw(g, (int) (mSlide + 370.0f), 0x46);
	mPopCapLogo->Draw(g, 0x280 - mPopCapLogo->mWidth, 0x1d6 - mPopCapLogo->mHeight);
	mPopCapUrl->Draw(g, 0x282 - mPopCapLogo->mWidth, 0x1db - mPopCapUrl->mHeight);
	mSponsor->Draw(g, 0, 0x1db - mSponsor->mHeight);

	// the version, in a dark box at the right
	std::string aVersion = GT(0x004b2e08) + mApp->mProductVersion;
	g->SetFont(mApp->mAtomicFont);
	g->SetColor(Color(0, 0, 0, 200));
	int aWidth = mApp->mAtomicFont->StringWidth(aVersion);
	g->FillRect(0x218 - aWidth, 0x78, aWidth + 6, 0x13);
	g->SetColor(Color(0, 0, 0, 255));
	g->DrawString(aVersion, 0x21f - aWidth, 0x8a);
	g->SetColor(Color(0, 255, 0, 255));
	g->DrawString(aVersion, 0x21c - aWidth, 0x87);

	// port: the original draws "thanks for registering" here (0x4b2df0, a green line over a dark box under the logo) on its registered
	// path; a game with nothing to register has no use for it, so it is left out (docs/GAME_NOTES.md section 3)

	// the bar
	mBar->DrawPartial(g, 0x9a, 0x17f, (float) mProgress, 1, 0);

	// the line under it: what is being loaded
	if (mPlayLink->mVisible)
	{
		g->SetColor(Color(0, 0, 0, 0xaf));
		g->FillRect(mPlayLink->mX - 5, mPlayLink->mY - 5, mPlayLink->mWidth + 10, mPlayLink->mHeight + 10);
	}
	else
	{
		g->SetColor(Color(0, 0, 0, 255));
		int aTextWidth = mApp->mAtomicFont->StringWidth(mApp->mLoadingText);
		g->DrawString(mApp->mLoadingText, 0x142 - aTextWidth / 2, 0x1ce);
		g->SetColor(Color(255, 255, 0, 255));
		g->DrawString(mApp->mLoadingText, 0x140 - aTextWidth / 2, 0x1cc);
	}
}

// 0x42fc40: the play link (id 1) starts the game's menus. The original's link with id 0 opens the registration box; the port
// has no such link.
void TitleScreen::ButtonDepress(int theId)
{
	if (theId != 0)
		mApp->TitleScreenDone();
}
