// BoardDraw.cpp: the board's Draw (0x415530) and DrawAnim (0x410c20), the drawing of the playing field from the sky down
// to the banners on top. Board.h describes the board.

#include "Board.h"
#include "Banner.h"
#include "BossWarning.h"
#include "Compat2004.h"
#include "DDImage.h"
#include "Enemy.h"
#include "FloatingText.h"
#include "GameText.h"
#include "Graphics.h"
#include "HeavyWeaponApp.h"
#include "ImageFont.h"
#include "MegaLaserPart.h"
#include "NukeBlast.h"
#include "PowerUpDrops.h"
#include "Projectile.h"
#include "Res.h"
#include "Sprite.h"
#include "TankShots.h"

#include <math.h>

using namespace Sexy;

namespace
{
	const int FIELD_CENTRE_X = 320;			// the field's own x is from the screen's centre: it is drawn translated by this
	const int SCREEN_WIDTH = 640;
	const int SCREEN_HEIGHT = 480;

	// The scrolling planes: two copies of each image side by side
	const float SKY_WIDTH = 640.0f;
	const int BACKGROUND_WIDTH = 960;
	const float BACKGROUND_WIDTH_3D = 959.0f;	// the 3D path's second copy overlaps the first by a pixel
	const int BACKGROUND_Y = 180;
	const int GROUND_Y = 420;
	const int MOON_BOSS = 5;				// GetBossIndex: the eye's levels show the moon
	const int MOON_X = 200;
	const int MOON_Y = 160;

	// The mile marker that comes by when a banner shows (mBannerTime below 0), with the miles left to the boss
	const int MILE_MARKER_X = 640;
	const int MILE_MARKER_Y = 350;
	const int MILE_TEXT_X = 650;
	const int MILE_TEXT_Y = 372;
	const int UPDATES_PER_MILE = 5000;
	const int MILE_TEXT_CELS = 5;			// 0 to 4 miles

	// The gas station a boss's level ends at, while the ground scrolls to its stop (mScrollDistance)
	const double GAS_STATION_SHOWN = 1000.0;
	const int GAS_STATION_Y = 405;
	const int GAS_SIGN_TEXT_Y = 370;
	const int GAS_SIGN_X = -135;			// from the station
	const int GAS_SIGN_Y = 350;
	const int GAS_PUMP_LEFT_X = -65;
	const int GAS_PUMP_RIGHT_X = 55;
	const int GAS_PUMP_Y = 455;
	const unsigned int SIGN_TEXTS[Board::NUM_SIGNS] =	// the station's name (mSignFrame)
	{
		GTA(0x52bc38), GTA(0x52bc4c), GTA(0x52bc60), GTA(0x52bc74)
	};

	const double TRACKS_MAX_WIDTH = 644.0;	// a stretch of tread marks this wide or wider is not drawn
	const int TRACKS_Y = 455;
	const int TRACKS_HEIGHT = 17;
	const int CRATER_Y = 460;
	const int CRATER_FADE = 512;			// a crater fades out over its last 512 updates
	const int GROUND_FLASH_Y = 460;
	const int SHIELD_ZAP_Y = 2;				// below the tank's y
	const double SHIELD_ZAP_BRIGHTNESS = 28.3;	// times mShieldHitGlow
	const int EXPLOSION_CEL = 160;

	// The lasers (WEAPON_LASERS): their beams show while the weapon's delay is between these
	const int LASER_SHOWN_MIN = 50;
	const int LASER_SHOWN_MAX = 100;
	const int LASER_BEAMS = 2;				// a wide dim one under a narrow white one
	const double QUARTER_TURN = 1.57;		// across the beam
	const int GUN_PIVOT_Y = 12;				// above the tank's y
	const double LASER_MUZZLE = 35.0;		// the beam starts this far along the gun
	const double LASER_BURN_RANGE = 800.0;	// a shorter beam ends on something, which burns
	const int LASER_GLARE_CELS = 10;
	const int LASER_BURN_PERIOD = 40;		// the burn's cels, 4 updates each
	const int LASER_BURN_CEL_TIME = 4;

	// The mega laser
	const double MEGA_LASER_MUZZLE = 40.0;
	const int BEAM_CEL_TIME = 4;			// mMegaLaserTime's updates a cel
	const int BEAM_CELS = 5;
	const double BEAM_CEL_SPACING = 0.125;	// the cels shift along the beam
	const int BEAM_FIRE_SPACING = 20;
	const int BEAM_SPACING = 15;
	const int BEAM_FRINGE_SPACING = 50;

	// The flash where the tank died (mDeathTime counts up from 1)
	const int DEATH_FLASH_TIME = 25;
	const double DEATH_FLASH_SPEED = 0.125;
	const int DEATH_FLASH_WIDTH = 200;		// the image's size
	const int DEATH_FLASH_HEIGHT = 150;
	const double DEATH_FLASH_RISE = 40.0;
	const int DEATH_FADE_STEP = 10;

	// "no nukes" and the nukes held, at the top left
	const int NO_NUKES_BLINK = 30;
	const int NO_NUKES_HIDDEN = 10;			// the line blinks: hidden the first 11 updates of each 30
	const int NO_NUKES_X = -305;
	const int NO_NUKES_Y = 55;
	const int NO_NUKES_ICON_X = -120;
	const int NUKE_ICONS_X = -300;
	const int NUKE_ICON_SPACING = 30;
	const int NUKE_ICON_Y = 50;

	// The boss's health bar
	const int BOSS_LABEL_X = 215;
	const int BOSS_LABEL_Y = 50;
	const int BOSS_BAR_X = 220;
	const int BOSS_BAR_Y = 37;
	const int BOSS_BAR_WIDTH = 200;
	const int BOSS_BAR_HEIGHT = 15;
	const int BOSS_BAR_DEAD_ALPHA = 80;		// once the boss is killed the bar fades
	const int BOSS_BAR_PULSE = 31;			// the bar pulses red on the update count
	const double BOSS_BAR_PULSE_SPEED = 0.1;
}

// 0x410c20: a background animation's frame where it is now; the planes behind the ground only with particles on
void Board::DrawAnim(Graphics* g, int thePlane)
{
	if (mAnims[thePlane] != NULL && (mApp->mParticles != 0.0 || thePlane <= PLANE_GROUND))
	{
		BackgroundAnim* anAnim = mAnims[thePlane];
		if (!mApp->Is3DAccelerated())
		{
			anAnim->mSprite->Draw(g, (int)anAnim->mX, anAnim->mY, false, (int)anAnim->mFrame, 0, false);
			return;
		}
		Sprite* aSprite = anAnim->mSprite;
		Rect aSrcRect((int)anAnim->mFrame * aSprite->mCelWidth, 0, aSprite->mCelWidth, aSprite->mCelHeight);
		g->DrawImageF(aSprite->mImage, (float)anAnim->mX, (float)anAnim->mY, aSrcRect);
	}
}

// 0x415530 slot 16: back to front: the sky, the far background, the background and the ground, each with its
// animation; then everything on the field (from the screen's centre); then the screen's flashes, and the boss's bar
// and the banners on top. The original writes out the sky's and backgrounds' DrawAnim in place.
void Board::Draw(Graphics* g)
{
	// the nuke's flash tints the planes red
	if (mNukeFlash != 0.0)
	{
		g->SetColorizeImages(true);
		int aTint = (int)(255.0 - mNukeFlash);
		g->SetColor(Color(255, aTint, aTint, 255));
	}
	Sprite* aSky = mApp->mSprites[IMAGE_LEVEL_SKY];
	if (!mApp->Is3DAccelerated())
	{
		aSky->Draw(g, (int)mSkyScroll, 0, false, 0, 0, false);
		aSky->Draw(g, (int)((double)mSkyScroll + SKY_WIDTH), 0, false, 0, 0, false);
	}
	else
	{
		g->DrawImageF(aSky->mImage, mSkyScroll, 0.0f);
		g->DrawImageF(aSky->mImage, mSkyScroll + SKY_WIDTH, 0.0f);
	}
	if (mApp->GetBossIndex(mApp->mLevel) == MOON_BOSS)
		mApp->mSprites[IMAGE_MOON]->Draw(g, MOON_X, MOON_Y, true, 0, 0, false);
	DrawAnim(g, PLANE_SKY);

	// the projectiles far off, behind the far background
	g->Translate(FIELD_CENTRE_X, 0);
	for (mProjectiles.mCurrent = mProjectiles.mHead; mProjectiles.mCurrent->mNext != NULL; )
	{
		mProjectiles.mCurrent = mProjectiles.mCurrent->mNext;
		Projectile* aProjectile = mProjectiles.Current();
		if (aProjectile->mIsInBackground)
			aProjectile->Draw(g);
	}
	g->Translate(-FIELD_CENTRE_X, 0);

	Sprite* aFarBackground = mApp->mSprites[IMAGE_LEVEL_BG2];
	if (!mApp->Is3DAccelerated())
	{
		if (0.0 < mApp->mParticles)
		{
			aFarBackground->Draw(g, (int)mFarBackgroundScroll, BACKGROUND_Y, false, 0, 0, false);
			aFarBackground->Draw(g, (int)mFarBackgroundScroll + BACKGROUND_WIDTH, BACKGROUND_Y, false, 0, 0, false);
		}
	}
	else if (0.0 < mApp->mParticles)
	{
		g->DrawImageF(aFarBackground->mImage, mFarBackgroundScroll, (float)BACKGROUND_Y);
		g->DrawImageF(aFarBackground->mImage, mFarBackgroundScroll + BACKGROUND_WIDTH_3D, (float)BACKGROUND_Y);
	}
	DrawAnim(g, PLANE_FAR_BACKGROUND);

	g->Translate(FIELD_CENTRE_X, 0);
	for (mNukeBlasts.mCurrent = mNukeBlasts.mHead; mNukeBlasts.mCurrent->mNext != NULL; )
	{
		mNukeBlasts.mCurrent = mNukeBlasts.mCurrent->mNext;
		mNukeBlasts.Current()->Draw(g);
	}
	g->Translate(-FIELD_CENTRE_X, 0);

	if (mNukeFlash != 0.0)
	{
		g->SetColorizeImages(true);
		int aTint = (int)(255.0 - mNukeFlash);
		g->SetColor(Color(255, aTint, aTint, 255));
	}
	Sprite* aBackground = mApp->mSprites[IMAGE_LEVEL_BG];
	if (!mApp->Is3DAccelerated())
	{
		aBackground->Draw(g, (int)mBackgroundScroll, BACKGROUND_Y, false, 0, 0, false);
		aBackground->Draw(g, (int)mBackgroundScroll + BACKGROUND_WIDTH, BACKGROUND_Y, false, 0, 0, false);
	}
	else
	{
		g->DrawImageF(aBackground->mImage, mBackgroundScroll, (float)BACKGROUND_Y);
		g->DrawImageF(aBackground->mImage, mBackgroundScroll + BACKGROUND_WIDTH_3D, (float)BACKGROUND_Y);
	}
	DrawAnim(g, PLANE_BACKGROUND);
	g->SetColorizeImages(false);

	if (mNukeFlash != 0.0)
	{
		g->SetColorizeImages(true);
		int aTint = (int)(255.0 - mNukeFlash);
		g->SetColor(Color(255, aTint, aTint, 255));
	}
	if (mBannerTime < 0 && !mIsSurvival)
	{
		mApp->mSprites[IMAGE_MILEMARKER]->Draw(g, mBannerTime + MILE_MARKER_X, MILE_MARKER_Y, false, 0, 0, false);
		int aMilesLeft = (mLevelLength - mLevelTime) / UPDATES_PER_MILE;
		if (aMilesLeft < MILE_TEXT_CELS && aMilesLeft >= 0)
			mApp->mSprites[IMAGE_MILETEXT]->Draw(g, mBannerTime + MILE_TEXT_X, MILE_TEXT_Y, false, aMilesLeft, 0, false);
	}
	Sprite* aGround = mApp->mSprites[IMAGE_LEVEL_GROUND];
	aGround->Draw(g, (int)mGroundScroll, GROUND_Y, false, 0, 0, false);
	aGround->Draw(g, (int)((double)mGroundScroll + SKY_WIDTH), GROUND_Y, false, 0, 0, false);
	DrawAnim(g, PLANE_GROUND);

	g->Translate(FIELD_CENTRE_X, 0);
	if (mScrollDistance < GAS_STATION_SHOWN)
	{
		int aStationX = (int)mScrollDistance;
		mApp->mSprites[IMAGE_GASSTATION]->Draw(g, aStationX, GAS_STATION_Y, true, 0, 0, false);
		g->SetFont(mApp->mFonts[FONT_STATIONFONT]);
		g->SetColor(Color(255, 255, 255, 255));
		SexyString aName = GT(SIGN_TEXTS[mSignFrame]);
		int aWidth = g->StringWidth(aName);
		g->DrawString(aName, (int)mScrollDistance - aWidth / 2, GAS_SIGN_TEXT_Y);
		mApp->mSprites[IMAGE_GASSIGN]->Draw(g, (int)mScrollDistance + GAS_SIGN_X, GAS_SIGN_Y, true, 0, 0, false);
	}

	for (mTreadMarks.mCurrent = mTreadMarks.mHead; mTreadMarks.mCurrent->mNext != NULL; )
	{
		mTreadMarks.mCurrent = mTreadMarks.mCurrent->mNext;
		TreadMark* aMark = mTreadMarks.Current();
		double aWidth = aMark->mRight - aMark->mLeft;
		if (aWidth < TRACKS_MAX_WIDTH)
		{
			Rect aSrcRect(0, 0, (int)aWidth, TRACKS_HEIGHT);
			Rect aDestRect((int)aMark->mLeft, TRACKS_Y, aSrcRect.mWidth, TRACKS_HEIGHT);
			g->DrawImage(mApp->mSprites[IMAGE_TRACKS]->mImage, aDestRect, aSrcRect);
		}
	}

	g->SetColorizeImages(true);
	for (mCraters.mCurrent = mCraters.mHead; mCraters.mCurrent->mNext != NULL; )
	{
		mCraters.mCurrent = mCraters.mCurrent->mNext;
		Crater* aCrater = mCraters.Current();
		if (aCrater->mLife < CRATER_FADE)
			g->SetColor(Color(255, 255, 255, aCrater->mLife / 2));
		else
			g->SetColor(Color(255, 255, 255));
		mApp->mSprites[IMAGE_CRATER]->Draw(g, (int)aCrater->mX, CRATER_Y, true, aCrater->mFrame, 0, false);
	}

	// the flashes on the ground: type 0 white through magenta, the others through cyan
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	for (mGroundFlashes.mCurrent = mGroundFlashes.mHead; mGroundFlashes.mCurrent->mNext != NULL; )
	{
		mGroundFlashes.mCurrent = mGroundFlashes.mCurrent->mNext;
		GroundFlash* aFlash = mGroundFlashes.Current();
		int aBright = aFlash->mAlpha;
		if (aBright > 255)
			aBright = 255;
		int aWhite = aBright * 2 - 255;
		if (aWhite < 0)
			aWhite = 0;
		if (aFlash->mType == 0)
			g->SetColor(Color(aBright, aWhite, aBright));
		else
			g->SetColor(Color(aWhite, aBright, aBright));
		mApp->mSprites[IMAGE_BULLETGLOW]->Draw(g, (int)aFlash->mX, GROUND_FLASH_Y, true, aFlash->mType, 0, false);
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColorizeImages(false);

	for (mEnemies.mCurrent = mEnemies.mHead; mEnemies.mCurrent->mNext != NULL; )
	{
		mEnemies.mCurrent = mEnemies.mCurrent->mNext;
		mEnemies.Current()->Draw(g);
	}
	if (mTankDeadTime == 0)
		mApp->DrawTank(g, (int)mTankX, mTankY, (int)mTreadFrame, mTurretFrame);
	for (mShellCasings.mCurrent = mShellCasings.mHead; mShellCasings.mCurrent->mNext != NULL; )
	{
		mShellCasings.mCurrent = mShellCasings.mCurrent->mNext;
		ShellCasing* aCasing = mShellCasings.Current();
		mApp->mSprites[IMAGE_CASING]->Draw(g, (int)aCasing->mX, (int)aCasing->mY, true, 0, 0, false);
	}
	if (mScrollDistance < GAS_STATION_SHOWN)
	{
		mApp->mSprites[IMAGE_GASPUMP]->Draw(g, (int)mScrollDistance + GAS_PUMP_LEFT_X, GAS_PUMP_Y, true, 0, 0, false);
		mApp->mSprites[IMAGE_GASPUMP]->Draw(g, (int)mScrollDistance + GAS_PUMP_RIGHT_X, GAS_PUMP_Y, true, 0, 0, false);
	}
	if (0.0 < mShieldHitGlow)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		int aBright = (int)(mShieldHitGlow * SHIELD_ZAP_BRIGHTNESS);
		g->SetColor(Color(aBright, aBright, aBright, 255));
		mApp->mSprites[IMAGE_SHIELDZAP]->Draw(g, (int)mTankX, mTankY + SHIELD_ZAP_Y, true, (int)mShieldHitGlow, 0,
			false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}

	mParticles.Draw(g);
	for (mExplosions.mCurrent = mExplosions.mHead; mExplosions.mCurrent->mNext != NULL; )
	{
		mExplosions.mCurrent = mExplosions.mCurrent->mNext;
		Explosion* anExplosion = mExplosions.Current();
		g->SetFastStretch(!mApp->Is3DAccelerated());
		Rect aSrcRect((int)anExplosion->mFrame * EXPLOSION_CEL, 0, EXPLOSION_CEL, EXPLOSION_CEL);
		Rect aDestRect((int)(anExplosion->mX - (double)(anExplosion->mWidth / 2)),
			(int)(anExplosion->mY - (double)(anExplosion->mHeight / 2)), anExplosion->mWidth, anExplosion->mHeight);
		g->DrawImage(mApp->mSprites[IMAGE_EXPLOSION]->mImage, aDestRect, aSrcRect);
	}
	for (mFloatingTexts.mCurrent = mFloatingTexts.mHead; mFloatingTexts.mCurrent->mNext != NULL; )
	{
		mFloatingTexts.mCurrent = mFloatingTexts.mCurrent->mNext;
		mFloatingTexts.Current()->Draw(g);
	}
	for (mMegaLaserParts.mCurrent = mMegaLaserParts.mHead; mMegaLaserParts.mCurrent->mNext != NULL; )
	{
		mMegaLaserParts.mCurrent = mMegaLaserParts.mCurrent->mNext;
		mMegaLaserParts.Current()->Draw(g);
	}
	mPowerUpDrops->Draw(g);

	// the lasers: a wide dim green beam and a narrow white one along the gun, the glare at the muzzle and the burn where
	// a short beam ends
	int aLaserDelay = mWeaponDelays[HeavyWeaponApp::WEAPON_LASERS];
	if (LASER_SHOWN_MIN < aLaserDelay && aLaserDelay < LASER_SHOWN_MAX)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		Point aBeam[4];
		int anEndX = 0;
		int anEndY = 0;
		int aStartY = 0;
		int aBeamIndex = LASER_BEAMS;
		do
		{
			double anAcross = mLaserAngle + QUARTER_TURN;
			double aHalfWidth = (double)((mApp->mWeapons[HeavyWeaponApp::WEAPON_LASERS] + 1) * aBeamIndex);
			double aHalfX = cos(anAcross) * aHalfWidth;
			double aHalfY = sin(anAcross) * aHalfWidth;
			double aCos = cos(mLaserAngle);
			int aStartX = (int)(LASER_MUZZLE * aCos + mTankX);
			double aSin = sin(mLaserAngle);
			double aPivotY = (double)(mTankY - GUN_PIVOT_Y);
			aStartY = (int)(aSin * LASER_MUZZLE + aPivotY);
			anEndX = (int)(aCos * mLaserLength + mTankX);
			anEndY = (int)(aSin * mLaserLength + aPivotY);
			aBeam[0] = Point((int)((double)aStartX + aHalfX), (int)((double)aStartY + aHalfY));
			aBeam[1] = Point((int)((double)aStartX - aHalfX), (int)((double)aStartY - aHalfY));
			aBeam[2] = Point((int)((double)anEndX - aHalfX), (int)((double)anEndY - aHalfY));
			aBeam[3] = Point((int)((double)anEndX + aHalfX), (int)((double)anEndY + aHalfY));
			if (aBeamIndex == LASER_BEAMS)
				g->SetColor(Color(0x20, 0x80, 0x40));
			else
				g->SetColor(Color(255, 255, 255));
			g->PolyFill(aBeam, 4, false);
			aBeamIndex--;
		}
		while (0 < aBeamIndex);
		mApp->mSprites[IMAGE_LASERGLARE]->Draw(g, (int)(cos(mLaserAngle) * LASER_MUZZLE + mTankX),
			(int)(sin(mLaserAngle) * LASER_MUZZLE + (double)(mTankY - GUN_PIVOT_Y)), true, aLaserDelay % LASER_GLARE_CELS,
			0, false);
		// the original tests the beam's start, not its end, for being below the top
		if (mLaserLength < LASER_BURN_RANGE && -FIELD_CENTRE_X < anEndX && anEndX < FIELD_CENTRE_X && 0 < aStartY)
			mApp->mSprites[IMAGE_LASERBURN]->Draw(g, anEndX, anEndY, true,
				(mApp->mUpdateCount % LASER_BURN_PERIOD) / LASER_BURN_CEL_TIME, 0, false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}

	// the mega laser: the fire at the muzzle and along the beam, the beam itself, and its fringe
	if (mMegaLaserFiring != 0)
	{
		double aPivotY = (double)(mTankY - GUN_PIVOT_Y);
		int aCel = (mMegaLaserTime / BEAM_CEL_TIME) % BEAM_CELS;
		mApp->mSprites[IMAGE_BEAMFIRE]->Draw(g, (int)(cos(mLaserAngle) * MEGA_LASER_MUZZLE + mTankX),
			(int)(sin(mLaserAngle) * MEGA_LASER_MUZZLE + aPivotY), true, aCel, 0, false);
		double aDistance = (double)(BEAM_FIRE_SPACING - (mMegaLaserTime * 2) % BEAM_FIRE_SPACING);
		while (aDistance <= mLaserLength)
		{
			double anAlong = aDistance + MEGA_LASER_MUZZLE;
			int aFireCel = (int)((double)(mMegaLaserTime / BEAM_CEL_TIME) + aDistance * BEAM_CEL_SPACING) % BEAM_CELS;
			mApp->mSprites[IMAGE_BEAMFIRE]->Draw(g, (int)(cos(mLaserAngle) * anAlong + mTankX),
				(int)(sin(mLaserAngle) * anAlong + (double)(mTankY - GUN_PIVOT_Y)), true, aFireCel, 0, false);
			aDistance += (double)BEAM_FIRE_SPACING;
		}
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		aDistance = 0.0;
		while (aDistance <= mLaserLength)
		{
			double anAlong = aDistance + MEGA_LASER_MUZZLE;
			mApp->mSprites[IMAGE_BEAM]->Draw(g, (int)(cos(mLaserAngle) * anAlong + mTankX),
				(int)(sin(mLaserAngle) * anAlong + (double)(mTankY - GUN_PIVOT_Y)), true, 0, 0, false);
			aDistance += (double)BEAM_SPACING;
		}
		aDistance = (double)(BEAM_FRINGE_SPACING - (mMegaLaserTime * 2) % BEAM_FRINGE_SPACING);
		while (aDistance <= mLaserLength)
		{
			double anAlong = aDistance + MEGA_LASER_MUZZLE;
			int aFringeCel = (int)((double)(mMegaLaserTime / BEAM_CEL_TIME) + aDistance * BEAM_CEL_SPACING) % BEAM_CELS;
			mApp->mSprites[IMAGE_BEAMFRINGE]->Draw(g, (int)(cos(mLaserAngle) * anAlong + mTankX),
				(int)(sin(mLaserAngle) * anAlong + (double)(mTankY - GUN_PIVOT_Y)), true, aFringeCel, 0, false);
			aDistance += (double)BEAM_FRINGE_SPACING;
		}
		mApp->mSprites[IMAGE_BEAMFRINGE]->Draw(g, (int)(cos(mLaserAngle) * MEGA_LASER_MUZZLE + mTankX),
			(int)(sin(mLaserAngle) * MEGA_LASER_MUZZLE + (double)(mTankY - GUN_PIVOT_Y)), true,
			(mMegaLaserTime / BEAM_CEL_TIME) % BEAM_CELS, 0, false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}

	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColorizeImages(true);
	for (mSparks.mCurrent = mSparks.mHead; mSparks.mCurrent->mNext != NULL; )
	{
		mSparks.mCurrent = mSparks.mCurrent->mNext;
		Spark* aSpark = mSparks.Current();
		g->SetColor(Color(aSpark->mBrightness, aSpark->mBrightness, aSpark->mBrightness, 255));
		mApp->mSprites[IMAGE_STATICGLOW]->Draw(g, (int)aSpark->mX, (int)aSpark->mY, true, aSpark->mFrame, 0, false);
	}
	g->SetColorizeImages(false);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);

	for (mTankShots.mCurrent = mTankShots.mHead; mTankShots.mCurrent->mNext != NULL; )
	{
		mTankShots.mCurrent = mTankShots.mCurrent->mNext;
		mTankShots.Current()->Draw(g);
	}
	for (mProjectiles.mCurrent = mProjectiles.mHead; mProjectiles.mCurrent->mNext != NULL; )
	{
		mProjectiles.mCurrent = mProjectiles.mCurrent->mNext;
		Projectile* aProjectile = mProjectiles.Current();
		if (!aProjectile->mIsInBackground)
			aProjectile->Draw(g);
	}
	for (mBullets.mCurrent = mBullets.mHead; mBullets.mCurrent->mNext != NULL; )
	{
		mBullets.mCurrent = mBullets.mCurrent->mNext;
		Bullet* aBullet = mBullets.Current();
		aBullet->mSprite->Draw(g, (int)aBullet->mX, (int)aBullet->mY, true, aBullet->mColumn, aBullet->mRow, false);
	}
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	for (mMuzzleFlashes.mCurrent = mMuzzleFlashes.mHead; mMuzzleFlashes.mCurrent->mNext != NULL; )
	{
		mMuzzleFlashes.mCurrent = mMuzzleFlashes.mCurrent->mNext;
		MuzzleFlash* aFlash = mMuzzleFlashes.Current();
		mApp->mSprites[IMAGE_MUZZLEFLASH]->Draw(g, (int)aFlash->mX, (int)aFlash->mY, true, (int)(aFlash->mLife - 1.0), 0,
			false);
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);

	// the nuke's flash, the shake's and the tank's death flash over the whole screen
	if (mNukeFlash != 0.0)
	{
		g->Translate(-FIELD_CENTRE_X, 0);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color((int)mNukeFlash, (int)(mNukeFlash * 0.25), 0, 255));
		g->FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->Translate(FIELD_CENTRE_X, 0);
	}
	if (mShake != 0)
	{
		g->Translate(-FIELD_CENTRE_X, 0);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color(mShake, mShake, mShake, 255));
		g->FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->Translate(FIELD_CENTRE_X, 0);
	}
	if (mDeathTime != 0 && mDeathTime < DEATH_FLASH_TIME)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		Rect aSrcRect(0, 0, DEATH_FLASH_WIDTH, DEATH_FLASH_HEIGHT);
		double aScale = sin((double)mDeathTime * DEATH_FLASH_SPEED) * 1.3 + 0.2;
		double aHeight = (double)DEATH_FLASH_HEIGHT * aScale;
		int aY = (int)((double)mTankY - aHeight + DEATH_FLASH_RISE);
		Rect aDestRect((int)((double)mDeathX - aScale * 100.0), aY, (int)(aScale * (double)DEATH_FLASH_WIDTH),
			(int)aHeight);
		g->DrawImage(mApp->mSprites[IMAGE_TANKFLASH]->mImage, aDestRect, aSrcRect);
		int aFade = (DEATH_FLASH_TIME - mDeathTime) * DEATH_FADE_STEP;
		g->SetColor(Color(aFade, aFade, aFade));
		g->FillRect(-FIELD_CENTRE_X, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
	if (mBossWarning != NULL)
		mBossWarning->Draw(g);

	// the nukes held, or a blinking "no nukes" after a right click without one
	if (mApp->mPowerUps[HeavyWeaponApp::POWERUP_NUKES] == 0)
	{
		if (mNoNukesTime != 0 && mNoNukesTime % NO_NUKES_BLINK > NO_NUKES_HIDDEN)
		{
			g->SetColor(Color(255, 0, 0));
			g->SetFont(mApp->mFonts[FONT_OUTLINED]);
			g->DrawString(GT(0x52c6f8), NO_NUKES_X, NO_NUKES_Y);
			mApp->mSprites[IMAGE_NUKEICON]->Draw(g, NO_NUKES_ICON_X, NUKE_ICON_Y, true, 0, 0, false);
		}
	}
	else if ((mLevelTime <= mLevelLength || mIsSurvival) && 0 < mApp->mPowerUps[HeavyWeaponApp::POWERUP_NUKES])
	{
		int aNuke = 0;
		int x = NUKE_ICONS_X;
		do
		{
			mApp->mSprites[IMAGE_NUKEICON]->Draw(g, x, NUKE_ICON_Y, true, 0, 0, false);
			aNuke++;
			x += NUKE_ICON_SPACING;
		}
		while (aNuke < mApp->mPowerUps[HeavyWeaponApp::POWERUP_NUKES]);
	}
	g->Translate(-FIELD_CENTRE_X, 0);

	// the boss's health: red, pulsing, while it lives
	if (mBoss != NULL && !mBossUntouchable)
	{
		int aHealthWidth = (int)((double)BOSS_BAR_WIDTH / mBoss->mMaxHealth * mBoss->mHealth);
		if (aHealthWidth < 0)
			aHealthWidth = 0;
		g->SetColor(Color(255, 255, 255));
		g->SetFont(mApp->mFonts[FONT_OUTLINED]);
		WriteString2004(g, GT(0x52c6f0), BOSS_LABEL_X, BOSS_LABEL_Y, -1, 1, true, 0, -1);
		if (mBoss->mIsAlive)
			g->SetColor(Color((int)(cos((double)(mUpdateCnt % BOSS_BAR_PULSE) * BOSS_BAR_PULSE_SPEED) * 64.0 + 192.0), 0, 0));
		else
			g->SetColor(Color(255, 255, 255, BOSS_BAR_DEAD_ALPHA));
		g->FillRect(BOSS_BAR_X, BOSS_BAR_Y, aHealthWidth, BOSS_BAR_HEIGHT);
		g->SetColor(Color(0, 0, 0, mBoss->mIsAlive ? 255 : BOSS_BAR_DEAD_ALPHA));
		g->FillRect(aHealthWidth + BOSS_BAR_X, BOSS_BAR_Y, BOSS_BAR_WIDTH - aHealthWidth, BOSS_BAR_HEIGHT);
	}
	g->Translate(FIELD_CENTRE_X, 0);

	for (mBanners.mCurrent = mBanners.mHead; mBanners.mCurrent->mNext != NULL; )
	{
		mBanners.mCurrent = mBanners.mCurrent->mNext;
		mBanners.Current()->Draw(g);
	}
}
