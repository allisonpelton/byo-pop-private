#include "Armory.h"
#include "ArmoryWeaponPanel.h"
#include "BracketButton.h"
#include "FlickerLight.h"
#include "HeavyWeaponApp.h"
#include "ImageFont.h"
#include "MessageDialog.h"
#include "DDImage.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "MusicInterface.h"
#include "Sprite.h"
#include "WidgetManager.h"
#include "Compat2004.h"

using namespace Sexy;

namespace
{
	const int MAX_POINTS = 0x12;			// the points earned stop at the last level's
	const int MAX_WEAPON_LEVEL = 3;
	const int GLOW_FADE = 2;
	const int ARMORY_SONG = SONG_ATOMIC_TANK_59;	// the song the armory plays, from order 0x3b
	const int ARMORY_SONG_ORDER = 0x3b;
	const int TANK_X = 0x140;				// the tank in the middle of the picture
	const int TANK_Y = 400;
	const double TANK_TURRET_FRAME = 10.0;
	const int MESSAGE_WIDTH = 300;
	const int MESSAGE_HEIGHT = 0xe6;
	const int GO_BUTTON_X = 0x1fe;
	const int MAIN_BUTTON_X = 0x14;
	const int BUTTON_Y = 0x1ae;
	const int BUTTON_WIDTH = 0x6e;
	const int BUTTON_HEIGHT = 0x1e;
	const int PANEL_ROWS = 3;
	const int PANEL_COLUMN_STEP = 0x1b8;
	const int PANEL_ROW_STEP = 100;

	// The addresses of each weapon's name, description and next-level text (GT's keys)
	const unsigned int WEAPON_NAMES[Armory::NUM_WEAPONS] = {
		GTA(0x52b190), GTA(0x52b2a8), GTA(0x52b3c0), GTA(0x52b4d8), GTA(0x52b5f0), GTA(0x52b708)};
	const unsigned int WEAPON_DESCRIPTIONS[Armory::NUM_WEAPONS] = {
		GTA(0x52b1a4), GTA(0x52b2bc), GTA(0x52b3d4), GTA(0x52b4ec), GTA(0x52b604), GTA(0x52b71c)};
	const unsigned int WEAPON_NEXT_LEVELS[Armory::NUM_WEAPONS] = {
		GTA(0x52b26c), GTA(0x52b384), GTA(0x52b49c), GTA(0x52b5b4), GTA(0x52b6cc), GTA(0x52b7e4)};
}

// 0x40f190: one point for each level passed; the tiles for the first level+2 weapons (at most all six)
Armory::Armory(HeavyWeaponApp* theApp)
{
	mApp = theApp;
	mPointsSpent = 0;
	mSelectedWeapon = -1;
	mPointsTotal = theApp->mLevel;
	mNumPanels = theApp->mLevel + 2;
	if (mNumPanels > NUM_WEAPONS)
		mNumPanels = NUM_WEAPONS;
	if (mPointsTotal > MAX_POINTS)
		mPointsTotal = MAX_POINTS;

	for (int i = 0; i < mNumPanels; i++)
	{
		mPointsSpent += mApp->mWeapons[i];
		mPanels[i] = new ArmoryWeaponPanel(this, i, i / 3);
		mGlow[i] = 0;
	}

	mGoButton = new BracketButton(mApp, this, BUTTON_GO);
	mGoButton->SetTint(Color(0x20, 0x80, 0x40, 0xff));
	mGoButton->mLabel = GT(0x52b908);
	mMainButton = new BracketButton(mApp, this, BUTTON_MAIN);
	mMainButton->SetTint(Color(0x80, 0x40, 0x20, 0xff));
	mMainButton->mLabel = GT(0x52b900);

	if (!mApp->mMusicInterface->IsPlaying(ARMORY_SONG))
	{
		mApp->mMusicInterface->FadeOutAll(true, 0.939);
		mApp->mMusicInterface->FadeIn(ARMORY_SONG, ARMORY_SONG_ORDER, 0.8765, false);
	}
	mFlickerLight = new FlickerLight(mApp, mApp->mSprites[IMAGE_ARMORYGLOW]);
	Resize(0, 0, 0x280, 0x1e0);
}

// 0x40f160 (body 0x40f0a0): the tiles and buttons, and the lamp; the app forgets the armory
Armory::~Armory()
{
	delete mGoButton;
	delete mMainButton;
	for (int i = 0; i < mNumPanels; i++)
		delete mPanels[i];
	delete mFlickerLight;
	mApp->mArmory = NULL;
}

// 0x40efd0 slot 1
void Armory::AddedToManager(WidgetManager* theManager)
{
	Widget::AddedToManager(theManager);
	theManager->AddWidget(mGoButton);
	theManager->AddWidget(mMainButton);
	for (int i = 0; i < mNumPanels; i++)
		theManager->AddWidget(mPanels[i]);
}

// 0x40f030 slot 2
void Armory::RemovedFromManager(WidgetManager* theManager)
{
	Widget::RemovedFromManager(theManager);
	theManager->RemoveWidget(mGoButton);
	theManager->RemoveWidget(mMainButton);
	for (int i = 0; i < mNumPanels; i++)
		theManager->RemoveWidget(mPanels[i]);
}

// 0x40ef20 slot 13: the buttons at the bottom corners and the tiles in two columns of three
void Armory::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mGoButton->Resize(GO_BUTTON_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
	mMainButton->Resize(MAIN_BUTTON_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
	for (int i = 0; i < mNumPanels; i++)
		mPanels[i]->Resize((i / PANEL_ROWS) * PANEL_COLUMN_STEP + 10, (i % PANEL_ROWS) * PANEL_ROW_STEP + 0xf, 0xb4, 0x5a);
}

// 0x40ee90 slot 17: the lamp flickers and the flashes fade
void Armory::Update()
{
	Widget::Update();
	mFlickerLight->Update();
	for (int i = 0; i < NUM_WEAPONS; i++)
	{
		if (mGlow[i] > 0)
			mGlow[i] = mGlow[i] - GLOW_FADE;
	}
	MarkDirty();
}

// 0x40f090 slot 26: the mouse over the armory itself, off every weapon's tile: no weapon is described
void Armory::MouseMove(int x, int y)
{
	mSelectedWeapon = -1;
}

// 0x40f440 slot 16: the picture; the weapon under the mouse (name, description and what the next level adds, or MAXED), or
// a hint when none is; the points left; the lamp; the tank, with the flash of each weapon just changed over it
void Armory::Draw(Graphics* g)
{
	mApp->mSprites[IMAGE_ARMORY]->Draw(g, 0, 0, false, 0, 0, false);
	g->SetFont(mApp->mFonts[FONT_COMPUTER]);
	if (mSelectedWeapon < 0)
	{
		g->SetColor(Color(0xff, 0xc0, 0x20));
		std::string aText = GT(0x52b928);
		WriteWordWrapped2004(g, Rect(0xf5, 0x5f, 0x96, 0xaa), aText, mApp->mFonts[FONT_COMPUTER]->GetHeight(), -1);
	}
	else
	{
		WriteCenteredLine(g, 0x69, GT(WEAPON_NAMES[mSelectedWeapon]), Color(0xff, 0xe0, 0x40), Color(0, 0, 0), Point(1, 2));
		g->SetColor(Color(0xff, 0xc0, 0x20));
		WriteWordWrapped2004(g, Rect(0xf5, 0x6e, 0x96, 0x9b), GT(WEAPON_DESCRIPTIONS[mSelectedWeapon]),
			mApp->mFonts[FONT_COMPUTER]->GetHeight(), -1);
		int aLevel = mApp->mWeapons[mSelectedWeapon];
		if (aLevel < MAX_WEAPON_LEVEL)
		{
			if (aLevel > 0)
			{
				WriteWordWrapped2004(g, Rect(0xf5, 0xd2, 0x96, 0x9b), GT(WEAPON_NEXT_LEVELS[mSelectedWeapon]),
					mApp->mFonts[FONT_COMPUTER]->GetHeight(), -1);
			}
		}
		else
			WriteCenteredLine(g, 0xfa, GT(0x52b984), Color(0xff, 0xe0, 0x40), Color(0, 0, 0), Point(1, 2));
	}

	std::string aPointsText = StrFormat(GTC(0x52b90c), mPointsTotal - mPointsSpent);
	g->SetColor(Color(0xff, 0xff, 0xff));
	g->SetFont(mApp->mFonts[FONT_NORMAL]);
	WriteCenteredLine(g, 0x1d1, aPointsText, Color(0, 0xff, 0), Color(0, 0, 0), Point(1, 2));

	mFlickerLight->Draw(g);
	mApp->DrawTank(g, TANK_X, TANK_Y, (mApp->mUpdateCount % 0x32) / 5, TANK_TURRET_FRAME);
	for (int i = 0; i < NUM_WEAPONS; i++)
	{
		if (mGlow[i] > 0)
		{
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColorizeImages(true);
			g->SetColor(Color(mGlow[i], mGlow[i], mGlow[i]));
			mApp->DrawUpgradeWeapon(g, i, TANK_X, TANK_Y, TANK_TURRET_FRAME);
			mApp->DrawUpgradeWeapon(g, i, TANK_X, TANK_Y, TANK_TURRET_FRAME);
			mApp->DrawUpgradeWeapon(g, i, TANK_X, TANK_Y, TANK_TURRET_FRAME);
			mApp->DrawUpgradeWeapon(g, i, TANK_X, TANK_Y, TANK_TURRET_FRAME);
			g->SetColorizeImages(false);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
	}
}

// 0x410670 listener slot 4
void Armory::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONDOWN, 0);
}

// 0x40fa90 listener slot 5: GO starts the campaign's next level when every point is spent (the box asks for the rest
// otherwise); MAIN goes to the main menu; both save the progress, and the armory closes
void Armory::ButtonDepress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONUP, 0);
	if (theId == BUTTON_GO)
	{
		if ((mPointsTotal != mPointsSpent) && (mPointsTotal - mPointsSpent >= 0))
		{
			mApp->AddDialog(MessageDialog::DIALOG_ID, new MessageDialog(mApp, MESSAGE_WIDTH, MESSAGE_HEIGHT, GT(0x52b990),
				GT(0x52b9a0)));
			return;
		}
		mApp->SaveUserProgress();
		mApp->SaveUsers();
		mApp->StartCampaign();
	}
	else
	{
		if (theId != BUTTON_MAIN)
			return;
		mApp->SaveUserProgress();
		mApp->SaveUsers();
		mApp->ShowMainMenu();
	}
	mApp->mWidgetManager->RemoveWidget(this);
	mApp->SafeDeleteWidget(this);
}
