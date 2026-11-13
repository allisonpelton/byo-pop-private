#include "HelpScreen.h"
#include "Banner.h"
#include "BracketButton.h"
#include "Craft.h"
#include "Enemy.h"
#include "FlickerLight.h"
#include "FloatingText.h"
#include "HeavyWeaponApp.h"
#include "MainMenu.h"
#include "NukeBlast.h"
#include "OptionsDialog.h"
#include "Projectile.h"
#include "PowerUpDrops.h"
#include "SupplyCopter.h"
#include "Checkbox.h"
#include "DDImage.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Sprite.h"
#include "WidgetManager.h"
#include "Compat2004.h"
#include <cmath>
#include <cstdlib>

using namespace Sexy;

namespace
{
	const int SCREEN_WIDTH = 0x280;
	const int SCREEN_HEIGHT = 0x1e0;
	const int OK_X = 500;
	const int OK_Y = 0x1bf;
	const int OK_WIDTH = 0x78;
	const int OK_HEIGHT = 0x1e;
	const int CHECKBOX_X = 0x14;
	const int CHECKBOX_Y = 0x1c0;
	const int CHECKBOX_SIZE = 0x1e;
	const int START_MOUSE_Y = 0x104;
	const int START_TANK_Y = 400;
	const int MOUSE_CENTER_X = 0x140;
	const int HEADING_Y = 0x19;
	const int LABEL_X = 0x37;
	const int LABEL_Y = 0x1d3;
	const int FIELD_X = 0x14;					// the playing field's rectangle, which clips the drawing
	const int FIELD_Y = 0x28;
	const int FIELD_WIDTH = 600;
	const int FIELD_HEIGHT = 0x168;
	const int TRACKS_HEIGHT = 400;				// the field's clip while the tread marks are drawn
	const int TRACK_Y = 0x1c7;
	const int TRACK_HEIGHT = 0x11;
	const double TRACK_MAX_LENGTH = 644.0;
	const int CRATER_Y = 0x1cc;
	const int CRATER_FADE_LIFE = 0x200;
	const int EXPLOSION_CELL = 0xa0;
	const int NUKE_ICON_X = -0x118;
	const int NUKE_ICON_Y = 0x46;
	const int POINTER_OFFSET = 0x1c;
	const int POINTER_FRAMES = 8;
	const int MOUSE_PICTURE_X = 0x21c;
	const int MOUSE_PICTURE_Y = 0x15e;
	const int MOUSE_PICTURE_NUKE_FIRST_TICK = 0xb5;	// the nuke turn's mouse picture shows the right button from here...
	const int MOUSE_PICTURE_NUKE_LAST_TICK = 0xef;		// ...to here
	const int MOUSE_PICTURE_RIGHT_BUTTON = 2;
	const int TEXT_X = 0x78;
	const int TEXT_Y = 0x32;
	const int TEXT_WIDTH = 400;
	const int TEXT_HEIGHT = 100;
	const int SWING_RADIUS = 100;				// of the pointer in the first turn
	const int SWING_RADIUS_LAST = 200;			// and in the second turn's end
	const double SWING_SPEED = 0.03;
	const double SWING_SPEED_LAST = 0.03976696558169129;
	const int FIRE_TURN_MIN_TICKS = 500;
	const int FIRE_TURN_END_TICK = 399;			// the second turn starts 1 tick on, at 400
	const int FIRE_BURST_PERIOD = 0x96;
	const int FIRE_BURST_LENGTH = 0x4b;
	const int FIRST_ENEMY_TICK = 400;
	const int SECOND_ENEMY_TICK = 600;
	const int ENEMY_START_X = -300;
	const double ENEMY_Y = 140.0;
	const double ENEMY_SPEED = 2.0;
	const double DEBRIS_MARK_X = -120.0;		// where the plane is when it is shot down
	const double SHOT_ANGLE_OFFSET = 1.5705;
	const int SHOT_RELOAD = 5;
	const int FIRST_FIRE_TICK_A = 0x226;		// the second turn's two shots of the tank
	const int FIRST_FIRE_TICK_B = 0x30c;
	const int SWING_START_TICK = 800;
	const int SWING_END_TICK = 0x370;
	const int DODGE_END_TICK = 1000;
	const double POWER_UP_START_Y = 140.0;
	const double POWER_UP_START_X = -520.0;
	const double POWER_UP_EXIT_X = 520.0;
	const int PLANES_UNTIL_TICK = 100;
	const int PLANE_PERIOD = 10;
	const int NUKE_TICK = 200;
	const int NUKE_TURN_END_TICK = 700;
	const double NUKE_FLASH_DECAY = 0.75;
	const double FLASH_GREEN_SHARE = 0.25;
}

// 0x421410: the tank is on the field's middle with the pointer, and the OK button and the checkbox are at the foot
HelpScreen::HelpScreen(HeavyWeaponApp* theApp, ButtonListener* theListener) :
	Board(theApp, true)
{
	mApp = theApp;
	mOkButton = new BracketButton(mApp, (theListener != NULL) ? theListener : static_cast<ButtonListener*>(this), BUTTON_OK);
	mOkButton->SetTint(Color(0x20, 0x80, 0x40));
	mOkButton->mLabel = GT(0x52d4f8);
	mCheckbox = new Checkbox(mApp->mSprites[IMAGE_UNCHECKED]->mImage, mApp->mSprites[IMAGE_CHECKED]->mImage,
		CHECKBOX_DONT_SHOW, this);
	mCheckbox->mChecked = (mApp->CurrentUser().mFlags & UserRecord::FLAG_NO_HELP) != 0;
	mTankX = 0;
	mTankY = START_TANK_Y;
	mScrollSpeed = 0;
	mMegaLaserPartDelay = 1;
	mMouseX = MOUSE_CENTER_X;
	mMouseY = START_MOUSE_Y;
	mPhase = PHASE_FIRE;
	mTick = 0;
	mAimAngle = 0;
	mTankDeadTime = 0;
	mPropFighter = NULL;
	mSmallCopter = NULL;
	mSupplyCopter = NULL;
	mNukeIcon = 0;
	Widget::Resize(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	mOkButton->Resize(OK_X, OK_Y, OK_WIDTH, OK_HEIGHT);
	mCheckbox->Resize(CHECKBOX_X, CHECKBOX_Y, CHECKBOX_SIZE, CHECKBOX_SIZE);
}

// 0x420d80 (body 0x4209b0): the main menu returns (with a new lamp), and an options dialog open under the help is made again
HelpScreen::~HelpScreen()
{
	delete mOkButton;
	delete mCheckbox;
	if (mApp->mMainMenu != NULL)
	{
		mApp->mWidgetManager->AddWidget(mApp->mMainMenu);
		mApp->mMainMenu->mFlickerLight = new FlickerLight(mApp, mApp->mSprites[IMAGE_MAINGLOW]);
		if (mApp->GetDialog(HeavyWeaponApp::DIALOG_OPTIONS) != NULL)
		{
			mApp->KillDialog(HeavyWeaponApp::DIALOG_OPTIONS);
			mApp->AddDialog(HeavyWeaponApp::DIALOG_OPTIONS, new OptionsDialog(mApp));
		}
	}
	if (mApp->mHelpScreen != NULL)
		mApp->mHelpScreen = NULL;
}

// 0x420b10 slot 1: the Widget's, not the board's
void HelpScreen::AddedToManager(WidgetManager* theManager)
{
	Widget::AddedToManager(theManager);
	theManager->AddWidget(mOkButton);
	theManager->AddWidget(mCheckbox);
}

// 0x420b50 slot 2
void HelpScreen::RemovedFromManager(WidgetManager* theManager)
{
	Widget::RemovedFromManager(theManager);
	theManager->RemoveWidget(mOkButton);
	theManager->RemoveWidget(mCheckbox);
}

// 0x420b90 slot 13
void HelpScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mOkButton->Resize(OK_X, OK_Y, OK_WIDTH, OK_HEIGHT);
	mCheckbox->Resize(CHECKBOX_X, CHECKBOX_Y, CHECKBOX_SIZE, CHECKBOX_SIZE);
}

// 0x471be0 slot 21: empty (the framework's one-argument stub), so the board's keys do nothing here
void HelpScreen::KeyChar(SexyChar theChar)
{
}

// 0x471be0 slot 22: empty
void HelpScreen::KeyDown(KeyCode theKey)
{
}

// 0x471be0 slot 23: empty
void HelpScreen::KeyUp(KeyCode theKey)
{
}

// 0x420b00 slot 24
void HelpScreen::MouseEnter()
{
}

// 0x420af0 slot 25
void HelpScreen::MouseLeave()
{
}

// 0x45fa60 slot 26: empty (the framework's two-argument stub), so the player's pointer does not steer the demonstration
void HelpScreen::MouseMove(int x, int y)
{
}

// 0x45fa60 slot 32: empty
void HelpScreen::MouseDrag(int x, int y)
{
}

// 0x45fa50 slot 27: empty (the framework's four-argument stub), so a click does not fire the demonstration's gun
void HelpScreen::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
}

// 0x45fa50 slot 29: empty
void HelpScreen::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
}

// 0x420bf0 listener slot 5: the OK button, when the screen is its own listener, closes it
void HelpScreen::ButtonDepress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONUP, 0);
	mApp->mWidgetManager->RemoveWidget(this);
	mApp->SafeDeleteWidget(this);
}

// 0x4213c0 checkbox listener slot 0
void HelpScreen::CheckboxChecked(int theId, bool checked)
{
	mApp->PlaySample(checked ? SOUND_BUTTONDOWN : SOUND_BUTTONUP, 0);
	mApp->CurrentUser().mFlags ^= UserRecord::FLAG_NO_HELP;
}

// 0x420c30: the pointer swings over the field and the gun fires in bursts; when the swing comes back to the middle after
// 500 ticks the second turn starts
void HelpScreen::UpdateFirePhase()
{
	mMouseX = (int) (sin(mAimAngle) * (double) SWING_RADIUS + (double) MOUSE_CENTER_X);
	mAimAngle = mAimAngle + SWING_SPEED;
	mMouseDown = (mTick % FIRE_BURST_PERIOD) < FIRE_BURST_LENGTH;
	if ((abs(mMouseX - MOUSE_CENTER_X) < 4) && (mTick > FIRE_TURN_MIN_TICKS))
	{
		mTick = FIRE_TURN_END_TICK;
		mAimAngle = 0;
		mMouseX = MOUSE_CENTER_X;
		mMouseDown = false;
		mPhase = PHASE_DODGE;
	}
}

// 0x420e10: a plane flies in at tick 400, and the tank shoots it as it passes -120; a helicopter at 600 shoots at the
// tank as it passes the middle; the tank fires at 550 and 780 and the pointer swings between 800 and 880
void HelpScreen::UpdateDodgePhase()
{
	if (mTick == FIRST_ENEMY_TICK)
	{
		mPropFighter = new PropFighter(this);
		mPropFighter->mY = ENEMY_Y;
		mPropFighter->mMirrored = false;
		mPropFighter->mPoints = 0;
		mPropFighter->mVelX = ENEMY_SPEED;
		mPropFighter->mX = ENEMY_START_X - mPropFighter->GetCelWidth() / 2;
	}
	if (mTick == SECOND_ENEMY_TICK)
	{
		mSmallCopter = new SmallCopter(this);
		mSmallCopter->mY = ENEMY_Y;
		mSmallCopter->mMirrored = false;
		mSmallCopter->mPoints = 0;
		mSmallCopter->mVelX = ENEMY_SPEED;
		mSmallCopter->mX = ENEMY_START_X - mSmallCopter->GetCelWidth() / 2;
	}
	if (mPropFighter != NULL)
	{
		if ((abs((int) (mPropFighter->mX - DEBRIS_MARK_X)) < 2) && (mProjectiles.mCount == 0))
		{
			AddDumbBomb((int) mPropFighter->mX, (int) (mPropFighter->GetCelHeight() / 2 + mPropFighter->mY),
				mPropFighter->mVelX, mPropFighter->mVelY, mPropFighter->mMirrored);
		}
		if ((double) (mPropFighter->GetCelWidth() / 2 + 300) < mPropFighter->mX)
		{
			mPropFighter->Remove();
			mPropFighter = NULL;
		}
	}
	if (mSmallCopter != NULL)
	{
		if (((double) abs((int) mSmallCopter->mX) < 1.1) && (mBullets.mCount == 0))
		{
			double anAngle = atan2(mTankX - mSmallCopter->mX, (double) mTankY - mSmallCopter->mY) + SHOT_ANGLE_OFFSET;
			AddEnemyBullet((int) mSmallCopter->mX, (int) (mSmallCopter->GetCelHeight() / 2 + mSmallCopter->mY), anAngle, 0.0);
			mSmallCopter->mShotDelay = SHOT_RELOAD;
		}
		if ((double) (mSmallCopter->GetCelWidth() / 2 + 300) < mSmallCopter->mX)
		{
			mSmallCopter->Remove();
			mSmallCopter = NULL;
		}
	}
	mMouseDown = (mTick == FIRST_FIRE_TICK_A) || (mTick == FIRST_FIRE_TICK_B);
	if ((mTick >= SWING_START_TICK) && (mTick < SWING_END_TICK))
		mMouseX = (int) (sin((mTick - SWING_START_TICK) * SWING_SPEED_LAST) * (double) SWING_RADIUS_LAST + (double) MOUSE_CENTER_X);
	if (mTick > DODGE_END_TICK)
	{
		mTick = 0;
		mPhase = PHASE_POWER_UP;
		mMouseX = MOUSE_CENTER_X;
		mSmallCopter = NULL;
		mPropFighter = NULL;
	}
}

// 0x421160: at tick 1 the friendly helicopter comes in from the left; at the middle it drops its power-up; when it has crossed
// it goes, and the nuke turn starts. The nuke icon shows once the power-up's text is up
void HelpScreen::UpdatePowerUpPhase()
{
	if (mTick == 1)
	{
		mSupplyCopter = new SupplyCopter(this, 0);
		mSupplyCopter->mY = POWER_UP_START_Y;
		mSupplyCopter->mMirrored = false;
		mSupplyCopter->mVelX = ENEMY_SPEED;
		mSupplyCopter->mX = POWER_UP_START_X;
	}
	if (mSupplyCopter != NULL)
	{
		if ((0.0 <= mSupplyCopter->mX) && !mSupplyCopter->mHasDropped)
		{
			mPowerUpDrops->Add(mSupplyCopter->mX, mSupplyCopter->mY, mSupplyCopter->mVelX, mSupplyCopter->mVelY, 0);
			mSupplyCopter->mHasDropped = true;
		}
		if (POWER_UP_EXIT_X < mSupplyCopter->mX)
		{
			mSupplyCopter->Remove();
			mSupplyCopter = NULL;
			mTick = 0;
			mPhase = PHASE_NUKE;
		}
	}
	if (mFloatingTexts.mCount > 0)
		mNukeIcon = 1;
}

// 0x420cd0: a plane every 10 ticks for the first 100, a nuke at 200 (the icon goes), and back to the first turn after 700
void HelpScreen::UpdateNukePhase()
{
	if ((mTick < PLANES_UNTIL_TICK) && ((mTick % PLANE_PERIOD) == 0))
		new PropFighter(this);
	if (mTick == NUKE_TICK)
	{
		FireNuke();
		if (mNukeIcon != 0)
			mNukeIcon--;
	}
	if (mTick > NUKE_TURN_END_TICK)
	{
		mTick = 0;
		mPhase = PHASE_FIRE;
	}
}

// 0x4212a0 slot 17: the board's updates, less the ones that need a game (no waves, no scroll, no power-ups of its own)
void HelpScreen::Update()
{
	Widget::Update();
	switch (mPhase)
	{
	case PHASE_FIRE:
		UpdateFirePhase();
		break;
	case PHASE_DODGE:
		UpdateDodgePhase();
		break;
	case PHASE_POWER_UP:
		UpdatePowerUpPhase();
		break;
	case PHASE_NUKE:
		UpdateNukePhase();
		break;
	}
	if (mShake != 0)
		UpdateShake();
	if (mNukeFlash != 0.0)
		mNukeFlash = mNukeFlash - NUKE_FLASH_DECAY;
	if (mNukeFlash < 0.0)
		mNukeFlash = 0;
	UpdateEnemies();
	UpdateCollisions();
	UpdateProjectiles();
	UpdateExplosions();
	mParticles.Update();
	mPowerUpDrops->Update();
	UpdateFloatingTexts();
	UpdateTreadMarks();
	UpdateNukeBlasts();
	UpdateShellCasings();
	UpdateMuzzleFlashes();
	UpdateTank();
	mTick++;
	MarkDirty();
}

// 0x420db0 slot 18: only the enemies move between updates
void HelpScreen::UpdateF(float theFrac)
{
	for (mEnemies.mCurrent = mEnemies.mHead; mEnemies.mCurrent->mNext != NULL; )
	{
		mEnemies.mCurrent = mEnemies.mCurrent->mNext;
		mEnemies.Current()->UpdateF(theFrac);
	}
}

// 0x421620 slot 16: the picture, the heading, then the field as the board draws it (without the ground or the sky), the pointer
// and the line of text for the turn
void HelpScreen::Draw(Graphics* g)
{
	int aSavedX = (int) g->mTransX;
	int aSavedY = (int) g->mTransY;
	g->mTransX = 0;
	g->mTransY = 0;
	g->mClipRect = Rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	mApp->mSprites[IMAGE_HELP]->Draw(g, 0, 0, false, 0, 0, false);
	g->SetColor(Color(0xff, 0xff, 0xff));
	g->SetFont(mApp->mFonts[FONT_RUBBERSTAMPLET20]);
	WriteCenteredLine(g, HEADING_Y, GT(0x52d5f0));
	g->SetColor(Color(0xff, 0xff, 0xff));
	g->SetFont(mApp->mFonts[FONT_OUTLINED]);
	WriteString2004(g, GT(0x52d5cc), LABEL_X, LABEL_Y, -1, -1, true, 0, -1);
	g->mTransX = (float) aSavedX;
	g->mTransY = (float) aSavedY;
	g->mClipRect = Rect(FIELD_X, FIELD_Y, FIELD_WIDTH, FIELD_HEIGHT);
	g->Translate(MOUSE_CENTER_X, 0);
	for (mNukeBlasts.mCurrent = mNukeBlasts.mHead; mNukeBlasts.mCurrent->mNext != NULL; )
	{
		mNukeBlasts.mCurrent = mNukeBlasts.mCurrent->mNext;
		mNukeBlasts.Current()->Draw(g);
	}
	g->Translate(-MOUSE_CENTER_X, 0);
	g->mClipRect = Rect(FIELD_X, FIELD_Y, FIELD_WIDTH, TRACKS_HEIGHT);
	for (mTreadMarks.mCurrent = mTreadMarks.mHead; mTreadMarks.mCurrent->mNext != NULL; )
	{
		mTreadMarks.mCurrent = mTreadMarks.mCurrent->mNext;
		TreadMark* aMark = mTreadMarks.Current();
		if (aMark->mRight - aMark->mLeft < TRACK_MAX_LENGTH)
		{
			int aWidth = (int) (aMark->mRight - aMark->mLeft);
			int aLeft = (int) aMark->mLeft;
			g->DrawImage(mApp->mSprites[IMAGE_TRACKS]->mImage, Rect(aLeft, TRACK_Y, aWidth, TRACK_HEIGHT),
				Rect(0, 0, aWidth, TRACK_HEIGHT));
		}
	}
	g->Translate(MOUSE_CENTER_X, 0);
	g->SetColorizeImages(true);
	for (mCraters.mCurrent = mCraters.mHead; mCraters.mCurrent->mNext != NULL; )
	{
		mCraters.mCurrent = mCraters.mCurrent->mNext;
		Crater* aCrater = mCraters.Current();
		if (aCrater->mLife < CRATER_FADE_LIFE)
			g->SetColor(Color(0xff, 0xff, 0xff, aCrater->mLife / 2));
		else
			g->SetColor(Color(0xff, 0xff, 0xff));
		mApp->mSprites[IMAGE_CRATER]->Draw(g, (int) aCrater->mX, CRATER_Y, true, aCrater->mFrame, 0, false);
	}
	g->SetColorizeImages(false);
	for (mEnemies.mCurrent = mEnemies.mHead; mEnemies.mCurrent->mNext != NULL; )
	{
		mEnemies.mCurrent = mEnemies.mCurrent->mNext;
		mEnemies.Current()->Draw(g);
	}
	mApp->DrawTank(g, (int) mTankX, mTankY, (int) mTreadFrame, mTurretFrame);
	for (mShellCasings.mCurrent = mShellCasings.mHead; mShellCasings.mCurrent->mNext != NULL; )
	{
		mShellCasings.mCurrent = mShellCasings.mCurrent->mNext;
		ShellCasing* aCasing = mShellCasings.mCurrent->mItem;
		mApp->mSprites[IMAGE_CASING]->Draw(g, (int) aCasing->mX, (int) aCasing->mY, true, 0, 0, false);
	}
	mParticles.Draw(g);
	for (mExplosions.mCurrent = mExplosions.mHead; mExplosions.mCurrent->mNext != NULL; )
	{
		mExplosions.mCurrent = mExplosions.mCurrent->mNext;
		Explosion* anExplosion = mExplosions.Current();
		g->SetFastStretch(!mApp->Is3DAccelerated());
		int aFrame = (int) anExplosion->mFrame;
		Rect aSrc(aFrame * EXPLOSION_CELL, 0, EXPLOSION_CELL, EXPLOSION_CELL);
		Rect aDest((int) (anExplosion->mX - anExplosion->mWidth / 2), (int) (anExplosion->mY - anExplosion->mHeight / 2),
			anExplosion->mWidth, anExplosion->mHeight);
		g->DrawImage(mApp->mSprites[IMAGE_EXPLOSION]->mImage, aDest, aSrc);
	}
	for (mFloatingTexts.mCurrent = mFloatingTexts.mHead; mFloatingTexts.mCurrent->mNext != NULL; )
	{
		mFloatingTexts.mCurrent = mFloatingTexts.mCurrent->mNext;
		mFloatingTexts.Current()->Draw(g);
	}
	mPowerUpDrops->Draw(g);
	for (mProjectiles.mCurrent = mProjectiles.mHead; mProjectiles.mCurrent->mNext != NULL; )
	{
		mProjectiles.mCurrent = mProjectiles.mCurrent->mNext;
		Projectile* aProjectile = mProjectiles.Current();
		if (!aProjectile->mIsInBackground)
			mProjectiles.Current()->Draw(g);
	}
	for (mBullets.mCurrent = mBullets.mHead; mBullets.mCurrent->mNext != NULL; )
	{
		mBullets.mCurrent = mBullets.mCurrent->mNext;
		Bullet* aBullet = mBullets.Current();
		aBullet->mSprite->Draw(g, (int) aBullet->mX, (int) aBullet->mY, true, aBullet->mColumn, aBullet->mRow, false);
	}
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	for (mMuzzleFlashes.mCurrent = mMuzzleFlashes.mHead; mMuzzleFlashes.mCurrent->mNext != NULL; )
	{
		mMuzzleFlashes.mCurrent = mMuzzleFlashes.mCurrent->mNext;
		MuzzleFlash* aFlash = mMuzzleFlashes.mCurrent->mItem;
		mApp->mSprites[IMAGE_MUZZLEFLASH]->Draw(g, (int) aFlash->mX, (int) aFlash->mY, true, (int) (aFlash->mLife - 1.0), 0,
			false);
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	if (mNukeFlash != 0.0)
	{
		g->Translate(-MOUSE_CENTER_X, 0);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color((int) mNukeFlash, (int) (mNukeFlash * FLASH_GREEN_SHARE), 0, 0xff));
		g->FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->Translate(MOUSE_CENTER_X, 0);
	}
	if (mShake != 0)
	{
		g->Translate(-MOUSE_CENTER_X, 0);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color(mShake, mShake, mShake, 0xff));
		g->FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->Translate(MOUSE_CENTER_X, 0);
	}
	if (mNukeIcon != 0)
		mApp->mSprites[IMAGE_NUKEICON]->Draw(g, NUKE_ICON_X, NUKE_ICON_Y, true, 0, 0, false);
	for (mBanners.mCurrent = mBanners.mHead; mBanners.mCurrent->mNext != NULL; )
	{
		mBanners.mCurrent = mBanners.mCurrent->mNext;
		mBanners.Current()->Draw(g);
	}
	g->Translate(-MOUSE_CENTER_X, 0);
	g->DrawImage(mApp->mTargetImages[(mApp->mUpdateCount / 4) % POINTER_FRAMES], mMouseX - POINTER_OFFSET,
		mMouseY - POINTER_OFFSET);
	g->SetColor(Color(0xff, 0xff, 0xff));
	g->SetFont(mApp->mFonts[FONT_OUTLINED]);
	Rect aTextRect(TEXT_X, TEXT_Y, TEXT_WIDTH, TEXT_HEIGHT);
	int aLineSpacing = mApp->mFonts[FONT_OUTLINE]->GetHeight();
	switch (mPhase)
	{
	case PHASE_FIRE:
		mApp->mSprites[IMAGE_MOUSE]->Draw(g, MOUSE_PICTURE_X, MOUSE_PICTURE_Y, true, mMouseDown ? 1 : 0, 0, false);
		WriteWordWrapped2004(g, aTextRect, GT(0x52d588), aLineSpacing, 0);
		break;
	case PHASE_DODGE:
		WriteWordWrapped2004(g, aTextRect, GT(0x52d564), aLineSpacing, 0);
		break;
	case PHASE_POWER_UP:
		WriteWordWrapped2004(g, aTextRect, GT(0x52d524), aLineSpacing, 0);
		break;
	case PHASE_NUKE:
		{
			int aColumn = ((mTick < MOUSE_PICTURE_NUKE_FIRST_TICK) || (mTick > MOUSE_PICTURE_NUKE_LAST_TICK)) ? 0 :
				MOUSE_PICTURE_RIGHT_BUTTON;
			mApp->mSprites[IMAGE_MOUSE]->Draw(g, MOUSE_PICTURE_X, MOUSE_PICTURE_Y, true, aColumn, 0, false);
			WriteWordWrapped2004(g, aTextRect, GT(0x52d4fc), aLineSpacing, 0);
		}
		break;
	}
}
