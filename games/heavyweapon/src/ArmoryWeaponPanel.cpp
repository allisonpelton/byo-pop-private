#include "ArmoryWeaponPanel.h"
#include "Armory.h"
#include "ArmoryArrowButton.h"
#include "HeavyWeaponApp.h"
#include "ButtonWidget.h"
#include "Graphics.h"
#include "Sprite.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{
	const int GLOW_SPEED = 12;
	const int GLOW_MAX = 0xff;
	const int MAX_WEAPON_LEVEL = 3;
	const int PAN_RANGE = 2000;			// a tile's sounds pan to its column: -1000 on the left, 1000 on the right
	const int PAN_CENTER = 1000;
	const int PICTURE_X = 0x5f;			// the picture's x, less 10 for the second column
	const int PICTURE_COLUMN_SHIFT = 10;
	const int PICTURE_Y = 0x2d;
	const int LEVEL_X = 0x1e;			// the level's cels, 0x78 further right for the second column
	const int LEVEL_COLUMN_STEP = 0x78;
	const int LEVEL_Y = 0x2c;
	const int BUTTON_X = 0x84;			// the plus and minus buttons, 0x7b further left for the second column
	const int BUTTON_COLUMN_SHIFT = 0x7b;
	const int BUTTON_WIDTH = 0x27;
	const int BUTTON_HEIGHT = 0x1f;
	const int PAN_X_OFFSET = 0xe4;		// a button's sounds pan by the tile's x less this, three times over
}

// 0x42ff80
ArmoryWeaponPanel::ArmoryWeaponPanel(Armory* theArmory, int theWeapon, int theColumn)
{
	mArmory = theArmory;
	mApp = theArmory->mApp;
	mWeapon = theWeapon;
	mColumn = theColumn;
	mGlow = 0;
	mPlusButton = new ArmoryArrowButton(mApp, this, 0, mColumn);
	mMinusButton = new ArmoryArrowButton(mApp, this, 1, mColumn);
	mPictureButton = new ButtonWidget(0, this);
	mPictureButton->mDoFinger = true;
	mPictureButton->mBtnNoDraw = true;
}

// 0x430090 (body 0x42fa20)
ArmoryWeaponPanel::~ArmoryWeaponPanel()
{
	delete mPlusButton;
	delete mMinusButton;
	delete mPictureButton;
}

// 0x42faa0 slot 1
void ArmoryWeaponPanel::AddedToManager(WidgetManager* theManager)
{
	Widget::AddedToManager(theManager);
	theManager->AddWidget(mPlusButton);
	theManager->AddWidget(mMinusButton);
	theManager->AddWidget(mPictureButton);
}

// 0x42fae0 slot 2
void ArmoryWeaponPanel::RemovedFromManager(WidgetManager* theManager)
{
	Widget::RemovedFromManager(theManager);
	theManager->RemoveWidget(mPlusButton);
	theManager->RemoveWidget(mMinusButton);
	theManager->RemoveWidget(mPictureButton);
}

// 0x42fde0 slot 13: the buttons follow the tile
void ArmoryWeaponPanel::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mPlusButton->Resize(mX + mColumn * -BUTTON_COLUMN_SHIFT + BUTTON_X, mY + 0xe, BUTTON_WIDTH, BUTTON_HEIGHT);
	mMinusButton->Resize(mX + mColumn * -BUTTON_COLUMN_SHIFT + BUTTON_X, mY + 0x2d, BUTTON_WIDTH, BUTTON_HEIGHT);
	mPictureButton->Resize(mX + (6 - mColumn) * 10, mY + 10, 0x48, 0x48);
}

// 0x42fb20 slot 16: the weapon's picture, its glow, the bubble round it and its level
void ArmoryWeaponPanel::Draw(Graphics* g)
{
	int aPictureX = mColumn * -PICTURE_COLUMN_SHIFT + PICTURE_X;
	mApp->mSprites[IMAGE_UPGRADES]->Draw(g, aPictureX, PICTURE_Y, true, mWeapon, 0, false);
	g->SetColorizeImages(true);
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	if (mGlow > 0)
	{
		g->SetColor(Color(mGlow, mGlow, mGlow));
		mApp->mSprites[IMAGE_UPGRADEGLOW]->Draw(g, aPictureX, PICTURE_Y, true, 0, 0, false);
	}
	g->SetColorizeImages(false);
	mApp->mSprites[IMAGE_UPGRADEBUBBLE]->Draw(g, aPictureX, PICTURE_Y, true, 0, 0, false);
	mApp->mSprites[IMAGE_UPGRADELVL]->Draw(g, mColumn * LEVEL_COLUMN_STEP + LEVEL_X, LEVEL_Y, true,
		mApp->mWeapons[mWeapon], 0, false);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// 0x42fc30 slot 17: the glow rises while the mouse is over the tile and falls when it is not
void ArmoryWeaponPanel::Update()
{
	Widget::Update();
	if (!Contains(mApp->mWidgetManager->mLastMouseX, mApp->mWidgetManager->mLastMouseY))
	{
		int aGlow = mGlow;
		mGlow = aGlow - GLOW_SPEED;
		if (aGlow - GLOW_SPEED < 0)
			mGlow = 0;
	}
	else
	{
		int aGlow = mGlow;
		mGlow = aGlow + GLOW_SPEED;
		if (aGlow + GLOW_SPEED > GLOW_MAX)
			mGlow = GLOW_MAX;
	}
	MarkDirty();
}

// 0x42fc90 slot 24: the tile is selected and its sound plays, unless its glow is already full
void ArmoryWeaponPanel::MouseEnter()
{
	Widget::MouseEnter();
	if (mGlow != GLOW_MAX)
	{
		mArmory->mSelectedWeapon = mWeapon;
		mApp->PlaySample(SOUND_MAPOVER, mColumn * PAN_RANGE - PAN_CENTER);
	}
}

// 0x42fce0 slot 25
void ArmoryWeaponPanel::MouseLeave()
{
	Widget::MouseLeave();
	if (!Contains(mApp->mWidgetManager->mLastMouseX, mApp->mWidgetManager->mLastMouseY))
		mApp->PlaySample(SOUND_MAPOVER, mColumn * PAN_RANGE - PAN_CENTER);
}

// 0x42fd30 slot 26: the tile under the mouse is the selected one
void ArmoryWeaponPanel::MouseMove(int x, int y)
{
	mArmory->mSelectedWeapon = mWeapon;
}

// 0x42fd40 listener slot 0: the mouse enters one of the tile's buttons
void ArmoryWeaponPanel::ButtonMouseEnter(int theId)
{
	if (mGlow != GLOW_MAX)
	{
		mArmory->mSelectedWeapon = mWeapon;
		mApp->PlaySample(SOUND_MAPOVER, mColumn * PAN_RANGE - PAN_CENTER);
	}
}

// 0x42fd80 listener slot 1
void ArmoryWeaponPanel::ButtonMouseLeave(int theId)
{
	if (!Contains(mApp->mWidgetManager->mLastMouseX, mApp->mWidgetManager->mLastMouseY))
		mApp->PlaySample(SOUND_MAPOVER, mColumn * PAN_RANGE - PAN_CENTER);
}

// 0x42fdd0 listener slot 2
void ArmoryWeaponPanel::ButtonMouseMove(int theId, int x, int y)
{
	mArmory->mSelectedWeapon = mWeapon;
}

// 0x42fe90 listener slot 4
void ArmoryWeaponPanel::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONDOWN, (mX - PAN_X_OFFSET) * 3);
}

// 0x42feb0 listener slot 5: plus (id 0) spends an upgrade point on the weapon when it is below level 3 and a point is left;
// minus (id 1) takes one back; the weapon's glow in the armory flashes; a press that does nothing plays the denied sound
void ArmoryWeaponPanel::ButtonDepress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONUP, (mX - PAN_X_OFFSET) * 3);
	if (theId == 0)
	{
		if ((mApp->mWeapons[mWeapon] < MAX_WEAPON_LEVEL) &&
			(mArmory->mPointsTotal != mArmory->mPointsSpent) && (mArmory->mPointsTotal - mArmory->mPointsSpent >= 0))
		{
			mApp->mWeapons[mWeapon]++;
			mArmory->mPointsSpent++;
			mArmory->mGlow[mWeapon] = 0xff;
			return;
		}
	}
	else
	{
		if (theId != 1)
			return;
		if (mApp->mWeapons[mWeapon] > 0)
		{
			mApp->mWeapons[mWeapon]--;
			mArmory->mPointsSpent--;
			mArmory->mGlow[mWeapon] = 0xff;
			return;
		}
	}
	mApp->PlaySample(SOUND_DENIED, (mX - PAN_X_OFFSET) * 3);
}
