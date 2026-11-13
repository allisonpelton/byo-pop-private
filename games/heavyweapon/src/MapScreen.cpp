#include "MapScreen.h"
#include "HeavyWeaponApp.h"
#include "HelpScreen.h"
#include "HyperlinkWidget.h"
#include "DDImage.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Sprite.h"
#include "WidgetManager.h"
#include "Compat2004.h"
#include <cstdio>
#include <cstdlib>

using namespace Sexy;

namespace
{
	const int SCREEN_WIDTH = 0x280;
	const int SCREEN_HEIGHT = 0x1e0;
	const int START_DELAY = 0x32;
	const int FADE_SPEED = 4;
	const int HOVER_SPEED = 12;
	const int INTEL_CYCLE_UPDATES = 300;
	const int FLASH_SPEED = 2;
	const double ZOOM_START = 1.0;
	const double ZOOM_SPEED = 0.2;
	const double ZOOM_MIN = 14.0;				// the zoom's growth before the level may start
	const double ZOOM_PULL = 0.05;
	const double REGION_PULL = 0.1;
	const double SETTLED = 0.1;
	const double FADE_DARKNESS = 0.75;			// the map's darkening, in alpha for each step of mFade
	const int CLOUD_INSET = 10;					// the frame's cloud pictures sit this far inside its corners
	const int REGION_CENTER_X = 0xa0;			// where the region's picture ends up (its centre)
	const int REGION_CENTER_Y = 0x10e;
	const int BOSS_WITH_OFFSET_ZOOM = 4;		// the mission that zooms to another place
	const double ZOOM_TARGET_X = 320.0;
	const double ZOOM_TARGET_Y = 240.0;
	const double ZOOM_TARGET_X_BOSS4 = 510.0;
	const double ZOOM_TARGET_Y_BOSS4 = 410.0;
	const int INTEL_X = 0x140;					// the briefing panel's left side
	const int INTEL_CENTER_X = 0x1e0;			// the middle of the panel; the craft's picture is centred here
	const int CRAFT_Y = 0x154;
	const int INTEL_TEXT_X = 0x154;
	const int INTEL_TEXT_Y = 0x50;
	const int INTEL_TEXT_WIDTH = 0x118;
	const int INTEL_TEXT_HEIGHT = 0x172;
	const int CRAFT_TEXT_Y = 0x19a;
	const int BOSSES_PER_INTEL = 9;
	const int FINAL_LEVEL = HeavyWeaponApp::FINAL_LEVEL;
	const int FIRST_CRAFT_IMAGE = 135;			// the picture of craft n is image 135 + n (IMAGE_PROPFIGHTER is craft 1)
	const int NUM_CRAFT_IMAGES = 21;

	// Where each mission's region lies on the map (its picture's top left): the nine bosses', the last level's and the
	// survival game's
	const int REGION_POSITIONS[HeavyWeaponApp::BOSS_SURVIVAL + 1][2] = {
		{13, 23}, {10, 207}, {119, 144}, {114, 232}, {255, 115}, {247, 36}, {436, 74}, {306, 189}, {394, 247},
		{253, 347}, {0, 0}};
}

// 0x425500: the links (the briefing panel's is empty and invisible), and the list of the craft in the level's waves
MapScreen::MapScreen(HeavyWeaponApp* theApp)
{
	mApp = theApp;
	mHelpScreen = NULL;
	mFindingRegion = true;
	mMovingRegion = true;
	mTextAlpha = 0;
	mStartDelay = START_DELAY;
	mFade = 0;
	mLeft = 0.0;
	mTop = 0.0;
	mRight = (double) SCREEN_WIDTH;
	mBottom = (double) SCREEN_HEIGHT;
	mOverRegion = false;
	mHoverGlow = 0;
	mZoom = 0.0;
	mFlash = -1;
	mIntelTick = 0;
	mIntelIndex = 0;
	mAutoCycle = true;
	mInvadeLink = new HyperlinkWidget(BUTTON_INVADE, this);
	mInvadeLink->mLabel = GT(0x52dda4);
	mInvadeLink->SetFont(mApp->mFonts[FONT_NORMAL]->Duplicate());
	mInvadeLink->mColor = Color(0xff, 0xff, 0xff, 0xff);
	mInvadeLink->SetVisible(false);
	mIntelButton = new HyperlinkWidget(BUTTON_INTEL, this);
	mIntelButton->mLabel = "";
	mIntelButton->SetFont(mApp->mFonts[FONT_KEYPUNCH16]->Duplicate());
	mIntelButton->SetVisible(false);

	// the craft the level's waves send, each once, in the order of the craft table
	int aCounts[NUM_CRAFT_IDS];
	for (int i = 0; i < NUM_CRAFT_IDS; i++)
		aCounts[i] = 0;
	const std::vector<WaveInfo>& aWaves = mApp->mWaves[mApp->mLevel];
	for (size_t aWave = 0; aWave < aWaves.size(); aWave++)
	{
		for (size_t aLine = 0; aLine < aWaves[aWave].mCraft.size(); aLine++)
			aCounts[aWaves[aWave].mCraft[aLine]]++;
	}
	for (int aCraft = 1; aCraft < NUM_CRAFT_IDS; aCraft++)
	{
		if (aCounts[aCraft] > 0)
			mCrafts.push_back(aCraft);
	}
	Resize(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

// 0x4254d0 (body 0x424b30)
MapScreen::~MapScreen()
{
	delete mInvadeLink;
	delete mIntelButton;
}

// 0x423ee0 slot 1
void MapScreen::AddedToManager(WidgetManager* theManager)
{
	Widget::AddedToManager(theManager);
	theManager->AddWidget(mInvadeLink);
	theManager->AddWidget(mIntelButton);
}

// 0x423f10 slot 2
void MapScreen::RemovedFromManager(WidgetManager* theManager)
{
	Widget::RemovedFromManager(theManager);
	theManager->RemoveWidget(mInvadeLink);
	theManager->RemoveWidget(mIntelButton);
}

// 0x423f40 slot 13: the invade link under the region's picture, the panel's button over the briefing
void MapScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	int aLinkWidth = mApp->mFonts[FONT_NORMAL]->StringWidth(mInvadeLink->mLabel);
	mInvadeLink->Resize(REGION_CENTER_X - aLinkWidth / 2, 0x1b8, aLinkWidth, 0x19);
	mIntelButton->Resize(INTEL_X, 0xfa, 0x140, 0xe6);
}

// 0x423fc0: the picture of the enemy craft (its number in the craft table), with the parts the strip leaves out
void MapScreen::DrawCraft(Graphics* g, int theCraft)
{
	if ((theCraft < 1) || (theCraft > NUM_CRAFT_IMAGES))
		return;
	Sprite* aSprite = mApp->mSprites[FIRST_CRAFT_IMAGE + theCraft];
	aSprite->Draw(g, INTEL_CENTER_X, CRAFT_Y, true, 0, 0, false);
	if (theCraft == 8)
		mApp->mSprites[IMAGE_MEDROTOR]->Draw(g, INTEL_CENTER_X, 0x13e, true, 0, 0, false);
	else if (theCraft == 9)
	{
		int aY = CRAFT_Y - aSprite->mCelHeight / 2;
		int aX = INTEL_CENTER_X - aSprite->mCelWidth / 2;
		mApp->mSprites[IMAGE_ROTORS]->Draw(g, aX, aY, false, 0, 0, false);
	}
	else if (theCraft == 15)
	{
		mApp->mSprites[IMAGE_BLIMPPROP]->Draw(g, 0x1fc, 0x169, true, 0, 0, false);
		mApp->mSprites[IMAGE_BLIMPPROP]->Draw(g, 0x188, 0x168, true, 0, 0, false);
	}
	else if (theCraft == 18)
		mApp->mSprites[IMAGE_ENEMYGUN]->Draw(g, INTEL_CENTER_X, 0x143, true, 0, 0, false);
}

// 0x424510: the invasion starts: both links go, the pointer becomes an arrow and the region zooms from its place
void MapScreen::StartInvasion()
{
	mLeft = 160.0;
	mTop = 270.0;
	mZoom = ZOOM_START;
	mInvadeLink->SetVisible(false);
	mIntelButton->SetVisible(false);
	mApp->SetCursor(0);
	mApp->PlaySample(SOUND_SWOOSH, 0);
}

// 0x424410 slot 26: over the region's picture the pointer is a hand
void MapScreen::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	mMouseX = x;
	mMouseY = y;
	if ((mTextAlpha > 0xfe) && (mZoom == 0.0))
	{
		int aBoss = mApp->GetBossIndex(mApp->mLevel);
		if (mApp->mSprites[IMAGE_ROCK]->Collides(x, y, 0, 0, false, mApp->mSprites[IMAGE_MISSION1 + aBoss], REGION_CENTER_X,
			REGION_CENTER_Y, 0, 0, false, true))
		{
			mOverRegion = true;
			mApp->SetCursor(1);
			return;
		}
		if (!mIntelButton->mIsOver && !mInvadeLink->mIsOver)
		{
			mOverRegion = false;
			mApp->SetCursor(0);
		}
	}
}

// 0x4244e0 slot 32
void MapScreen::MouseDrag(int x, int y)
{
	Widget::MouseDrag(x, y);
	MouseMove(x, y);
}

// 0x424620 slot 31: a click on the region starts the invasion
void MapScreen::MouseUp(int x, int y)
{
	Widget::MouseUp(x, y);
	if (mOverRegion && (mZoom == 0.0))
		StartInvasion();
}

// 0x424580 listener slot 5: INVADE starts it, the briefing panel shows the next craft (and stops them changing by
// themselves), and the help screen's PLAY starts the level
void MapScreen::ButtonDepress(int theId)
{
	if (theId == BUTTON_INVADE)
		StartInvasion();
	else if (theId == BUTTON_INTEL)
	{
		mIntelTick = 0;
		mAutoCycle = false;
		mIntelIndex = (mIntelIndex + 1) % (int) mCrafts.size();
	}
	else if (theId == BUTTON_HELP_PLAY)
	{
		mApp->mWidgetManager->RemoveWidget(mHelpScreen);
		mApp->SafeDeleteWidget(mHelpScreen);
		mApp->StartLevel();
		mApp->mWidgetManager->RemoveWidget(this);
		mApp->SafeDeleteWidget(this);
	}
}

// 0x424660 slot 17: the frame closes in on the region, the map darkens and the briefing fades in; the invasion zooms and
// flashes red, and the screen goes
void MapScreen::Update()
{
	Widget::Update();
	MarkDirty();
	if (mFlash == 0)
		return;
	if (mFlash >= 0)
	{
		mFlash -= FLASH_SPEED;
		if (mFlash < 0)
		{
			mFlash = 0;
			if (mHelpScreen != NULL)
			{
				mApp->mWidgetManager->BringToBack(this);
				return;
			}
			mApp->mWidgetManager->RemoveWidget(this);
			mApp->SafeDeleteWidget(this);
			return;
		}
		return;
	}
	if (mStartDelay != 0)
	{
		mStartDelay--;
		return;
	}
	int aBoss = mApp->GetBossIndex(mApp->mLevel);
	if (!mFindingRegion)
	{
		if (mZoom == 0.0)
		{
			mFade += FADE_SPEED;
			if (mFade > 0xff)
			{
				mFade = 0xff;
				if (!mMovingRegion)
				{
					if (mTextAlpha < 0xff)
					{
						mTextAlpha += FADE_SPEED;
						if (mTextAlpha > 0xfe)
							mTextAlpha = 0xff;
					}
				}
				else
				{
					Sprite* aRegion = mApp->mSprites[IMAGE_MISSION1 + aBoss];
					double aDx = ((double) (REGION_CENTER_X - aRegion->mCelWidth / 2) - mLeft) * REGION_PULL;
					double aDy = ((double) (REGION_CENTER_Y - aRegion->mCelHeight / 2) - mTop) * REGION_PULL;
					mLeft = aDx + mLeft;
					mTop = aDy + mTop;
					mRight = aDx + mRight;
					mBottom = aDy + mBottom;
					if ((abs((int) aDx) < SETTLED) && (abs((int) aDy) < SETTLED))
						mMovingRegion = false;
				}
			}
			if (mTextAlpha != 0)
			{
				mInvadeLink->SetVisible(true);
				mIntelButton->SetVisible(true);
			}
		}
		else
		{
			mZoom = mZoom + ZOOM_SPEED;
			aBoss = mApp->GetBossIndex(mApp->mLevel);
			double aDx;
			double aTargetY;
			if (aBoss == BOSS_WITH_OFFSET_ZOOM)
			{
				aDx = ZOOM_TARGET_X_BOSS4 - mLeft;
				aTargetY = ZOOM_TARGET_Y_BOSS4;
			}
			else
			{
				aDx = ZOOM_TARGET_X - mLeft;
				aTargetY = ZOOM_TARGET_Y;
			}
			aDx = aDx * ZOOM_PULL;
			double aDy = (aTargetY - mTop) * ZOOM_PULL;
			mLeft = aDx + mLeft;
			mTop = aDy + mTop;
			if ((aDx < SETTLED) && (aDy < SETTLED) && (mZoom > ZOOM_MIN))
			{
				// the level starts (a first game shows the help screen on it)
				if (((mApp->CurrentUser().mFlags & UserRecord::FLAG_NO_HELP) == 0) && mApp->mHelpPending)
				{
					mHelpScreen = mApp->ShowHelp(static_cast<ButtonListener*>(this));
					mApp->mHelpPending = false;
				}
				else
					mApp->StartLevel();
				mApp->mWidgetManager->BringToFront(this);
				mMouseVisible = false;
				mFlash = 0xff;
			}
		}
	}
	else
	{
		// the frame closes in on the mission's place on the map
		double aLeft = (double) REGION_POSITIONS[aBoss][0];
		double aTop = (double) REGION_POSITIONS[aBoss][1];
		Sprite* aRegion = mApp->mSprites[IMAGE_MISSION1 + aBoss];
		double aDx1 = (aLeft - mLeft) * REGION_PULL;
		double aDy1 = (aTop - mTop) * REGION_PULL;
		double aDx2 = ((double) (aRegion->mCelWidth + REGION_POSITIONS[aBoss][0]) - mRight) * REGION_PULL;
		double aDy2 = ((double) (aRegion->mCelHeight + REGION_POSITIONS[aBoss][1]) - mBottom) * REGION_PULL;
		mLeft = aDx1 + mLeft;
		mTop = aDy1 + mTop;
		mRight = aDx2 + mRight;
		mBottom = aDy2 + mBottom;
		if ((aDx1 < SETTLED) && (aDy1 < SETTLED) && (aDx2 < SETTLED) && (aDy2 < SETTLED))
			mFindingRegion = false;
	}
	if (!mOverRegion && !mInvadeLink->mIsOver)
	{
		mHoverGlow -= HOVER_SPEED;
		if (mHoverGlow < 0)
			mHoverGlow = 0;
	}
	else
	{
		mHoverGlow += HOVER_SPEED;
		if (mHoverGlow > 0xff)
			mHoverGlow = 0xff;
	}
	mIntelTick = (mIntelTick + 1) % INTEL_CYCLE_UPDATES;
	if ((mIntelTick == 0) && mAutoCycle)
		mIntelIndex = (mIntelIndex + 1) % (int) mCrafts.size();
	MarkDirty();
}

// 0x424bf0 slot 16: the map; the cloud frame round the region; the briefing (mission, region, intel text, craft with its
// figures) at the right; the region's picture, tinted by the pointer or zoomed in red; a red flash over everything
void MapScreen::Draw(Graphics* g)
{
	if (mFlash >= 1)
	{
		g->SetColor(Color(0xff, 0, 0, mFlash));
		g->FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		return;
	}
	int aBoss = mApp->GetBossIndex(mApp->mLevel);
	int aBossIndex = aBoss;
	mApp->mSprites[IMAGE_MAP]->Draw(g, 0, 0, false, 0, 0, false);
	if (mFade > 0)
	{
		g->SetColor(Color(0, 0, 0, (int) ((double) mFade * FADE_DARKNESS)));
		g->FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	}
	g->SetColorizeImages(true);
	g->SetColor(Color(0, 0xff, 0, 0xff - mTextAlpha));
	mApp->mSprites[IMAGE_MAPRECT]->Draw(g, (int) (mLeft + CLOUD_INSET), (int) (mTop + CLOUD_INSET), true, 0, 0, false);
	mApp->mSprites[IMAGE_MAPRECT]->Draw(g, (int) (mRight - CLOUD_INSET), (int) (mTop + CLOUD_INSET), true, 1, 0, false);
	mApp->mSprites[IMAGE_MAPRECT]->Draw(g, (int) (mRight - CLOUD_INSET), (int) (mBottom - CLOUD_INSET), true, 2, 0, false);
	mApp->mSprites[IMAGE_MAPRECT]->Draw(g, (int) (mLeft + CLOUD_INSET), (int) (mBottom - CLOUD_INSET), true, 3, 0, false);
	g->SetColorizeImages(false);
	if (mTextAlpha != 0)
	{
		g->SetColor(Color(0xff, 0xff, 0xff, mTextAlpha));
		g->SetFont(mApp->mFonts[FONT_RUBBERSTAMPLET42]);
		char aBuffer[0x40];
		sprintf(aBuffer, GTC(0x52dd98), mApp->mLevel + 1);
		WriteCenteredLine(g, 0x32, aBuffer);
		g->SetFont(mApp->mFonts[FONT_RUBBERSTAMPLET20]);
		g->Translate(-REGION_CENTER_X, 0);
		WriteCenteredLine(g, 100, mApp->mLevels[aBoss].mName);
		g->Translate(INTEL_X, 0);
		WriteCenteredLine(g, 0x11d, GT(0x52dd8c));
		g->Translate(-REGION_CENTER_X, 0);
		g->SetColor(Color(0x80, 0x80, 0x80, mTextAlpha));
		g->DrawLine(INTEL_X, 0x41, INTEL_X, 0x1df);
		g->DrawLine(0, 0x41, 0x27f, 0x41);
		g->DrawLine(INTEL_X, 0xfa, 0x27f, 0xfa);
		g->SetColor(Color(0, 0xff, 0, mTextAlpha));
		g->SetFont(mApp->mFonts[FONT_COMPUTER]);
		int aIntelPart = mApp->mLevel / BOSSES_PER_INTEL;
		if (aIntelPart > 1)
			aIntelPart = 1;
		if (mApp->mLevel == FINAL_LEVEL)
			aIntelPart = 0;
		const std::string& anIntel = mApp->mLevels[aBoss].mIntel[aIntelPart];
		WriteWordWrapped2004(g, Rect(INTEL_TEXT_X, INTEL_TEXT_Y, INTEL_TEXT_WIDTH, INTEL_TEXT_HEIGHT), anIntel,
			mApp->mFonts[FONT_COMPUTER]->GetLineSpacing(), -1);
		int aCraft = mCrafts[mIntelIndex];
		DrawCraft(g, aCraft);
		const CraftInfo& aCraftInfo = mApp->mCraft[aCraft];
		WriteString2004(g, aCraftInfo.mDescription, INTEL_TEXT_X, CRAFT_TEXT_Y, -1, -1, true, 0, -1);
		int aLineY = CRAFT_TEXT_Y + mApp->mFonts[FONT_COMPUTER]->GetLineSpacing();
		if (!aCraftInfo.mArms.empty())
		{
			WriteString2004(g, GT(0x52dd80) + aCraftInfo.mArms, INTEL_TEXT_X, aLineY, -1, -1, true, 0, -1);
			aLineY += mApp->mFonts[FONT_COMPUTER]->GetLineSpacing();
		}
		char aArmor[0x40];
		sprintf(aArmor, GTC(0x52dd74), (int) aCraftInfo.mArmor);
		WriteString2004(g, aArmor, INTEL_TEXT_X, aLineY, -1, -1, true, 0, -1);
		aLineY += mApp->mFonts[FONT_COMPUTER]->GetLineSpacing();
		char aScore[0x40];
		sprintf(aScore, GTC(0x52dd68), aCraftInfo.mPoints);
		WriteString2004(g, aScore, INTEL_TEXT_X, aLineY, -1, -1, true, 0, -1);
	}
	if (mFade > 0)
	{
		g->SetColorizeImages(true);
		Sprite* aRegion = mApp->mSprites[IMAGE_MISSION1 + aBossIndex];
		if (mZoom == 0.0)
		{
			g->SetColor(Color(mHoverGlow, 0xff - mHoverGlow, 0, mFade));
			aRegion->Draw(g, (int) mLeft, (int) mTop, false, 0, 0, false);
		}
		else
		{
			g->SetColor(Color(0xff, 0, 0));
			g->SetFastStretch(!mApp->Is3DAccelerated());
			int aWidth = aRegion->mCelWidth;
			int aHeight = aRegion->mCelHeight;
			Rect aSource(0, 0, aWidth, aHeight);
			double aScaledWidth = (double) aWidth * mZoom;
			double aScaledHeight = (double) aHeight * mZoom;
			Rect aDest((int) (mLeft - aScaledWidth * 0.5), (int) (mTop - aScaledHeight * 0.5), (int) aScaledWidth,
				(int) aScaledHeight);
			g->DrawImage(aRegion->mImage, aDest, aSource);
		}
		g->SetColorizeImages(false);
	}
}
