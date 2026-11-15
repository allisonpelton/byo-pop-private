// Board.cpp: the board's life, its input and its update (the per-kind updates are in BoardUpdate.cpp, its drawing in
// BoardDraw.cpp). Board.h has the layout.

#include "Board.h"
#include "Banner.h"
#include "BossDeath.h"
#include "BossWarning.h"
#include "Bosses.h"
#include "Craft.h"
#include "Enemy.h"
#include "EnemyProjectiles.h"
#include "FloatingText.h"
#include "GameOverDialog.h"
#include "HeavyWeaponApp.h"
#include "HighScoresScreen.h"
#include "ImageFont.h"
#include "MegaLaserPart.h"
#include "MusicInterface.h"
#include "NukeBlast.h"
#include "PlayTipDialog.h"
#include "PowerUpDrops.h"
#include "Projectile.h"
#include "Res.h"
#include "ResultsScreen.h"
#include "SoundInstance.h"
#include "Sprite.h"
#include "SupplyCopter.h"
#include "TankShots.h"
#include "DDImage.h"
#include "StatusBar.h"
#include "WidgetManager.h"
#include "GameText.h"

#include <sstream>
#include <stdio.h>

using namespace Sexy;

namespace
{
	const int TANK_START_X = -380;				// a new tank comes in from off the left side
	const int TANK_Y = 443;
	const int FIRST_TANK_TIME = 300;			// the first tank comes in after this, a new one 400 after a death
	const int NEXT_TANK_TIME = 400;
	const int READY_TIME = 300;					// "get ready" or "game over" shows at this mTankDeadTime
	const int SURVIVAL_LEVEL_TIME = 12000;		// survival's difficulty goes up a level every 12000 (it counts 2 an update)
	const int MAX_DIFFICULTY_LEVEL = 8;			// mDifficulty's level, and the last of mLevels' lengths
	const int RESULTS_TIME = 500;				// the results come this long after it
	const int LAST_WAVE_TIME = 800;				// no waves start in the level's last 800 updates
	const int SUPPLY_COPTER_DELAY = 1000;
	const int SURVIVAL_SUPPLY_COPTER_DELAY = 600;
	const int SURVIVAL_NUKE_DROP_DELAY = 1200;
	const int NO_NUKES_TIME = 250;
	const int AUTO_FIRE_TIP_CLICKS = 5;			// the auto-fire tip comes after more single-shot clicks than this
	const double PAN_SCALE = 3.0;
	const double MUSIC_FADE_OUT_SPEED = 0.004;
	const double MUSIC_FADE_IN_SPEED = 0.002;
	const int GAME_OVER_SONG_ORDER = 0x4f;		// the order of SONG_ATOMIC_TANK_79 the game over plays from
	const int SCORES_SONG_ORDER = 0x3b;			// the order of SONG_ATOMIC_TANK_59 the scores play from
	const int NUKE_REACH_Y = 460;				// a nuke reaches the enemies whose top is above this
	const int NUM_BOSSES = 9;					// the bosses in turn, one a level (the final level's its own)
	const int SCREEN_WIDTH = 640;
	const double BIG_MISSILE_RATE = 0.6666666666666666;	// big missiles in a row: two thirds of the level, 2 to 5
	const int MIN_BIG_MISSILES = 2;
	const int MAX_BIG_MISSILES = 5;
}

// 0x4191d0
Board::Board(HeavyWeaponApp* theApp, bool isSurvival)
{
	mApp = theApp;
	Resize(0, 0, 640, 480);
	mIsSurvival = isSurvival;
	mTypedKeys = "";
	mHasStarted = false;
	mPaused = false;
	mSuspended = false;
	mBossDeath = NULL;
	mBossWarning = NULL;
	mMegaLaserSound = NULL;
	mBlockedLeft = false;
	mBlockedRight = false;
	mDozerOut = false;
	mSkyScroll = 0.0f;
	mGroundScroll = 0.0f;
	mBackgroundScroll = 0.0f;
	mFarBackgroundScroll = 0.0f;
	mScrollSpeed = 1.0;
	mLevelEndBlocks = 0;
	mTankDeadTime = FIRST_TANK_TIME;
	mDeathTime = 0;
	mShake = 0;
	mNukeFlash = 0.0;
	mSupplyCopterDelay = SUPPLY_COPTER_DELAY;
	mBoss = NULL;
	mBannerTime = 0;
	for (int i = 0; i < NUM_ANIM_PLANES; i++)
		mAnims[i] = NULL;
	mEnemiesDestroyed = 0;
	mEnemiesSpawned = 0;
	mProjectilesDestroyed = 0;
	mProjectilesFired = 0;
	mShotsFired = 0;
	mShotsMissed = 0;
	mFriendliesDestroyed = 0;
	mRank = NO_RANK;
	mClickCount = 0;
	mShotsThisPress = 0;
	mNoNukesTime = 0;
	mBossUntouchable = true;

	int aLevel = mApp->mLevel;
	if (aLevel > MAX_DIFFICULTY_LEVEL)
		aLevel = MAX_DIFFICULTY_LEVEL;
	mLevelLength = mApp->mLevels[aLevel].mLength;
	mLevelTime = 0;
	mNukeDropDelay = mLevelLength / 6;
	mTimePenalty = 0;
	mScrollDistance = 1000.0;
	mShieldDropDelay = mLevelLength / 8;
	if (!mIsSurvival && mApp->mLevel != 0)
		mMegaLaserPartsCaught = mApp->CurrentUser().mMegaLaserParts;
	else
		mMegaLaserPartsCaught = 0;
	if (mApp->mPowerUps[HeavyWeaponApp::POWERUP_SHIELD] == 0)
		mApp->mPowerUps[HeavyWeaponApp::POWERUP_SHIELD] = 1;
	for (int i = 0; i < NUM_ORBS; i++)
		mOrbs[i].mFlash = 0.0;
	for (int i = 0; i < NUM_WEAPON_DELAYS; i++)
		mWeaponDelays[i] = 0;
	mMegaLaserPartDelay = 0;
	mFriendlyCount = 0;
	mTankX = (double)TANK_START_X;
	mTankY = TANK_Y;
	mTankVelX = 0.0;
	mTreadFrame = 0.0;
	mTurretFrame = 0.0;
	mMegaLaserTime = 0;
	mMegaLaserFiring = 0;
	mOrbAngle = 0.0;
	mGunDelay = 0;
	mTankWidth = mApp->mSprites[IMAGE_TANK]->mCelWidth;
	mTankHeight = mApp->mSprites[IMAGE_TANK]->mCelHeight;
	mMouseDown = false;
	mShieldFrame = 0.0;
	mShieldHitGlow = 0.0;
	mLaserLength = 800.0;
	mShieldFlash = 0;
	mPowerUpDrops = new PowerUpDrops(this);
	mStatusBar = new StatusBar(mApp);
	mResultsScreen = NULL;
	mWaveDelay = 100;

	int aDifficultyLevel = (mIsSurvival ? mLevelTime / SURVIVAL_LEVEL_TIME : mApp->mLevel) + 1;
	if (aDifficultyLevel > MAX_DIFFICULTY_LEVEL)
		aDifficultyLevel = MAX_DIFFICULTY_LEVEL;
	mDifficulty = 100.0 / (double)(aDifficultyLevel + 15) * (double)(aDifficultyLevel * aDifficultyLevel);
	// The original hooks the keyboard here (SetWindowsHookEx WH_KEYBOARD_LL, 0x412d70) to swallow the Windows keys
	// while the board is up; the port leaves it out (a deviation, in the notes).
}

// 0x4198e0
Board::~Board()
{
	mWaveSpawners.Clear();
	mEnemies.Clear();
	mBullets.Clear();
	mExplosions.Clear();
	mProjectiles.Clear();
	mCraters.Clear();
	mParticles.Clear();
	mPowerUpDrops->Clear();
	mTankShots.Clear();
	mFloatingTexts.Clear();
	mTreadMarks.Clear();
	mNukeBlasts.Clear();
	mMegaLaserParts.Clear();
	mShellCasings.Clear();
	mMuzzleFlashes.Clear();
	mSparks.Clear();
	mBanners.Clear();
	mGroundFlashes.Clear();
	delete mPowerUpDrops;
	for (int i = 0; i < NUM_ANIM_PLANES; i++)
		delete mAnims[i];
	delete mBossWarning;
}

// 0x4184b0: the music starts again, "prepare for battle", and the level's banner; a board that comes up away from the
// mouse waits, paused if the window is not in front
void Board::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mStatusBar);
	mApp->mMusicInterface->FadeOutAll(true, MUSIC_FADE_OUT_SPEED);
	mApp->mMusicInterface->FadeIn(SONG_ATOMIC_TANK_0, 0, MUSIC_FADE_IN_SPEED, false);
	mApp->PlaySample(SOUND_V_PREPARE, 0);
	if (!mIsSurvival)
	{
		std::ostringstream aStream;
		aStream << GT(0x52c81c) << (mApp->mLevel + 1);
		new Banner(this, mApp->mFonts[FONT_RUBBERSTAMPLET42], aStream.str(), 220, 60);
		aStream.str("");
		aStream << mApp->mLevels[mApp->GetBossIndex(mApp->mLevel)].mName;
		new Banner(this, mApp->mFonts[FONT_RUBBERSTAMPLET20], aStream.str(), 280, 120);
	}
	else
		new Banner(this, mApp->mFonts[FONT_RUBBERSTAMPLET42], GT(0x52c828), 240, 60);
	if (!mIsOver)
	{
		mSuspended = true;
		if (!mApp->mHasFocus)
			Pause();
	}
}

// 0x40fdb0: the status bar goes with the board; the results screen is only taken off
void Board::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mStatusBar);
	mApp->SafeDeleteWidget(mStatusBar);
	mStatusBar = NULL;
	if (mResultsScreen != NULL)
		theWidgetManager->RemoveWidget(mResultsScreen);
}

// 0x4175a0: the keys typed are kept while they spell the start of the code; the whole code says it
void Board::KeyChar(SexyChar theChar)
{
	Widget::KeyChar(theChar);
	mTypedKeys += (char)theChar;
	mTypedKeys = StringToUpper(mTypedKeys);
	const std::string& aCode = GT(0x52c7d0);
	if (mTypedKeys != aCode.substr(0, mTypedKeys.size()))
		mTypedKeys = "";
	if (mTypedKeys == aCode)
	{
		mApp->PlaySample(SOUND_V_WAFFLE, 0);
		mTypedKeys = "";
	}
}

// 0x410660
void Board::KeyDown(KeyCode theKey)
{
	Widget::KeyDown(theKey);
}

// 0x413a30: the space bar pauses
void Board::KeyUp(KeyCode theKey)
{
	Widget::KeyUp(theKey);
	if (theKey == KEYCODE_SPACE)
		Pause();
}

// 0x4138c0
void Board::MouseEnter()
{
	Widget::MouseEnter();
	if (!mPaused)
		Resume();
}

// 0x413850: the mouse leaving the field stops the game, unless the button is held or the level is over
void Board::MouseLeave()
{
	Widget::MouseLeave();
	if (mMouseDown)
		return;
	if (mApp->mCustomCursorsEnabled)
		mApp->SetCursorImage(0, mApp->mSprites[IMAGE_CURSOR_POINTER]->mImage);
	if (mResultsScreen == NULL && mBossDeath == NULL && (mLevelTime <= mLevelLength || mIsSurvival))
		Suspend();
}

// 0x40feb0 (MouseMove and MouseDrag both)
void Board::MouseMove(int x, int y)
{
	mMouseX = x;
	mMouseY = y;
}

// 0x40feb0
void Board::MouseDrag(int x, int y)
{
	mMouseX = x;
	mMouseY = y;
}

// 0x41acd0: the left button fires (and starts the mega laser's sound while it has a charge), the right one a nuke
void Board::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	if ((mBossDeath != NULL || mLevelTime > mLevelLength) && !mIsSurvival)
		return;
	mMouseX = x;
	mMouseY = y;
	if (theBtnNum == 0)
	{
		mShotsThisPress = 0;
		mMouseDown = true;
		if (mMegaLaserTime != 0)
			mApp->PlaySample(SOUND_MEGALASER_START, (int)(mTankX * PAN_SCALE));
	}
	else if (theBtnNum == 1)
	{
		if (mNukeFlash == 0.0 && mTankDeadTime == 0 && mApp->mPowerUps[HeavyWeaponApp::POWERUP_NUKES] > 0)
			FireNuke();
		else
		{
			mApp->PlaySample(SOUND_DENIED, 0);
			if (mApp->mPowerUps[HeavyWeaponApp::POWERUP_NUKES] == 0 && mTankDeadTime == 0)
				mNoNukesTime = NO_NUKES_TIME;
		}
	}
}

// 0x417090: letting go of the left button; clicking shot by shot many times brings the auto-fire tip, and a release
// away from the field stops the game as MouseLeave does
void Board::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
	mMouseX = x;
	mMouseY = y;
	if (theBtnNum != 0 || theClickCount == 0)
		return;
	mMouseDown = false;
	if (mMegaLaserTime != 0)
		mApp->PlaySample(SOUND_MEGALASER_STOP, (int)(mTankX * PAN_SCALE));
	if (mShotsThisPress < 2 && mShotsThisPress != 0)
	{
		++mClickCount;
		if (mClickCount > AUTO_FIRE_TIP_CLICKS && (mApp->CurrentUser().mFlags & UserRecord::FLAG_NO_TIPS) == 0 &&
			(mApp->mTipsShown & HeavyWeaponApp::TIP_AUTO_FIRE) == 0 && mApp->mHelpScreen == NULL && !mIsSurvival)
		{
			mClickCount = 0;
			mApp->AddDialog(PlayTipDialog::DIALOG_ID, new PlayTipDialog(mApp, GT(0x52c768)));
			mApp->mTipsShown |= HeavyWeaponApp::TIP_AUTO_FIRE;
		}
	}
	else
		mClickCount = 0;
	if (!mIsOver && !mMouseDown && mBossDeath == NULL && mResultsScreen == NULL &&
		(mLevelTime <= mLevelLength || mIsSurvival))
		Suspend();
}

// 0x410670: the game-over dialog's buttons click
void Board::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONDOWN, 0);
}

// 0x410690: the game-over dialog's buttons: CONTINUE goes back to the level (survival starts again), SCORES shows the
// high scores (the main menu if the game made none)
void Board::ButtonDepress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONUP, 0);
	if (theId == BUTTON_GAME_OVER_CONTINUE)
	{
		mApp->mMusicInterface->FadeOutAll(true, MUSIC_FADE_OUT_SPEED);
		mApp->mMusicInterface->FadeIn(SONG_ATOMIC_TANK_59, SCORES_SONG_ORDER, MUSIC_FADE_IN_SPEED, false);
		mApp->KillDialog(DIALOG_GAME_OVER);
		HeavyWeaponApp* anApp = mApp;
		if (mIsSurvival)
		{
			anApp->mWidgetManager->RemoveWidget(this);
			anApp->SafeDeleteWidget(this);
			anApp->mBoard = NULL;
			anApp->StartSurvival();
			return;
		}
		anApp->mWidgetManager->RemoveWidget(this);
		anApp->SafeDeleteWidget(this);
		anApp->mBoard = NULL;
		if (anApp->mLevel > 0)
			anApp->ContinueGame(anApp->mLevel);
		else
			anApp->NewGame(false);
	}
	else if (theId == BUTTON_GAME_OVER_SCORES)
	{
		if (mRank == NO_RANK)
			mApp->ShowMainMenu();
		else
			mApp->mWidgetManager->AddWidget(new HighScoresScreen(mApp, mIsSurvival, mRank));
		HeavyWeaponApp* anApp = mApp;
		anApp->mBoard = NULL;
		anApp->KillDialog(DIALOG_GAME_OVER);
		anApp->mWidgetManager->RemoveWidget(this);
		anApp->SafeDeleteWidget(this);
	}
}

// 0x40fed0: the craft of craft.xml's index theCraft comes in (a wave's line). Trucks and enemy tanks only while nothing
// holds either side, one dozer at a time; the big missiles come as a row across the sky, more of them later in the game
void Board::SpawnCraft(int theCraft)
{
	switch (theCraft)
	{
	case CRAFT_PROPFIGHTER:
		new PropFighter(this);
		return;
	case CRAFT_SMALLJET:
		new SmallJet(this);
		return;
	case CRAFT_BOMBER:
		new Bomber(this);
		return;
	case CRAFT_JETFIGHTER:
		new JetFighter(this);
		return;
	case CRAFT_TRUCK:
		if (!mBlockedLeft && !mBlockedRight)
			new Truck(this);
		return;
	case CRAFT_BIGBOMBER:
		new BigBomber(this);
		return;
	case CRAFT_SMALLCOPTER:
		new SmallCopter(this);
		return;
	case CRAFT_MEDCOPTER:
		new MedCopter(this);
		return;
	case CRAFT_BIGCOPTER:
		new BigCopter(this);
		return;
	case CRAFT_DELTABOMBER:
		new DeltaBomber(this);
		return;
	case CRAFT_DELTAJET:
		new DeltaJet(this);
		return;
	case CRAFT_BIGMISSILE:
	{
		// two thirds of the level (a survival game's difficulty level), 2 to 5 missiles, evenly over the middle of the screen
		int aStage = mIsSurvival ? mLevelTime / SURVIVAL_LEVEL_TIME : mApp->mLevel;
		int aCount = (int)((double)aStage * BIG_MISSILE_RATE);
		if (aCount > MAX_BIG_MISSILES)
			aCount = MAX_BIG_MISSILES;
		else if (aCount < MIN_BIG_MISSILES)
			aCount = MIN_BIG_MISSILES;
		int aSpan = SCREEN_WIDTH - SCREEN_WIDTH / aCount;
		int aSpacing = aSpan / aCount;
		int aLast = aSpan / 2;
		int x = -(aSpan / 2);
		if (x <= aLast)
		{
			do
			{
				new BigMissile(this, (double)x);
				x += aSpacing;
			}
			while (x <= aLast);
		}
		return;
	}
	case CRAFT_SUPERBOMBER:
		new SuperBomber(this);
		return;
	case CRAFT_FATBOMBER:
		new FatBomber(this);
		return;
	case CRAFT_BLIMP:
		new Blimp(this);
		return;
	case CRAFT_SATELLITE:
		new Satellite(this);
		return;
	case CRAFT_STRAFER:
		new Strafer(this);
		return;
	case CRAFT_ENEMYTANK:
		if (!mBlockedLeft && !mBlockedRight)
			new EnemyTank(this);
		return;
	case CRAFT_DOZER:
		if (!mDozerOut)
			new Dozer(this);
		return;
	case CRAFT_DEFLECTOR:
		new Deflector(this);
		return;
	case CRAFT_CRUISEMISSILE:
		new CruiseMissile(this);
		return;
	}
}

// 0x4104b0
void Board::AddDumbBomb(int x, int y, double theVelX, double theVelY, bool isMirrored)
{
	if (mTankDeadTime == 0 && mBossDeath == NULL)
		new DumbBomb(this, (double)x, (double)y, theVelX, theVelY, isMirrored);
}

// 0x410540
void Board::AddIronBomb(int x, int y, double theVelX, double theVelY, bool isMirrored)
{
	if (mTankDeadTime == 0 && mBossDeath == NULL)
		new IronBomb(this, (double)x, (double)y, theVelX, theVelY, isMirrored);
}

// 0x4105d0: at full size
void Board::AddHellfire(int x, int y, double theVelX, double theVelY)
{
	if (mTankDeadTime == 0 && mBossDeath == NULL)
		new Hellfire(this, (double)x, (double)y, theVelX, theVelY, 1.0);
}

// 0x417230: a campaign's score gains thePoints (never below 0); the points float up from (x, y), red when they are a
// loss. Survival scores nothing but still shows them.
void Board::AddScore(int thePoints, int x, int y)
{
	if (thePoints != 0 && !mIsSurvival)
	{
		mApp->mScore += thePoints;
		if (mApp->mScore < 0)
			mApp->mScore = 0;
	}
	char aText[16];
	sprintf(aText, "%i", thePoints);
	new FloatingText(this, aText, x, y, false, thePoints < 1 ? Color(255, 0, 0) : Color(0));
}

// 0x41a880: the tank blows up; everything it carried that a death costs goes, and the last tank's death ends the game
// with the game-over tune and its high score
void Board::KillTank()
{
	if (mTankDeadTime != 0)
		return;
	mTankDeadTime = NEXT_TANK_TIME;
	mDeathTime++;
	mDeathX = (int)mTankX;
	AddExplosion(mTankX, (double)(mTankY - 5), 300, 200, 0.0, 0.0, 0.0);
	mParticles.Emit(mApp->mSprites[IMAGE_TANKFLAME], mTankX, (double)mTankY, 0.0, -2.0, 60, 3.0, 0.03, 3.0, 200,
		true, -1);
	mApp->PlaySample(SOUND_TANKEXPLODE, (int)(mTankX * PAN_SCALE));
	AddCrater((int)mTankX, 1);
	mTankX = (double)TANK_START_X;
	mTankY = TANK_Y;
	mTankVelX = 0.0;
	mTreadFrame = 0.0;
	mTurretFrame = 0.0;
	mMegaLaserTime = 0;
	mWaveDelay = 200;
	if (mMegaLaserSound != NULL)
		mMegaLaserSound->Release();
	mMegaLaserSound = NULL;
	for (int i = 0; i < NUM_WEAPON_DELAYS; i++)
		mWeaponDelays[i] = 0;
	mApp->mPowerUps[HeavyWeaponApp::POWERUP_NUKES] = 0;
	mApp->mPowerUps[HeavyWeaponApp::POWERUP_SHIELD] = 1;
	mMegaLaserPartsCaught = 0;
	mMegaLaserParts.Clear();
	mWaveSpawners.Clear();
	if (mApp->mLives < 1)
	{
		mApp->mMusicInterface->FadeOutAll(true, MUSIC_FADE_OUT_SPEED);
		mApp->mMusicInterface->FadeIn(SONG_ATOMIC_TANK_79, GAME_OVER_SONG_ORDER, MUSIC_FADE_IN_SPEED, false);
		mRank = mApp->AddHighScore(mIsSurvival, false);
	}
}

// 0x41aa90
int Board::ChoosePowerUp()
{
	int aLevel = mIsSurvival ? mLevelTime / SURVIVAL_LEVEL_TIME : mApp->mLevel;
	double aLevelFactor = (double)aLevel;
	std::vector<int> aChoices;
	for (int i = 0; i < HeavyWeaponApp::NUM_POWER_UPS; i++)
	{
		const PowerUpInfo& anInfo = gPowerUpInfo[i];
		int aCurrent = mApp->mPowerUps[i];
		if (((double)aCurrent <= aLevelFactor * anInfo.mLevelFactor || anInfo.mLevelFactor <= 0.0) &&
			aCurrent != anInfo.mMax)
		{
			if (i == HeavyWeaponApp::POWERUP_NUKES)
			{
				if (mNukeDropDelay == 0 && Rand() % 2 == 0)
					aChoices.push_back(i);
			}
			else if (i == HeavyWeaponApp::POWERUP_SHIELD)
			{
				if (mShieldDropDelay == 0)
					aChoices.push_back(i);
			}
			else
				aChoices.push_back(i);
		}
	}
	if (mIsSurvival)
	{
		int aWeaponTotal = 0;
		for (int j = 0; j < HeavyWeaponApp::NUM_WEAPONS; j++)
			aWeaponTotal += mApp->mWeapons[j];
		if (aWeaponTotal < aLevel)
		{
			for (int j = 0; j < HeavyWeaponApp::NUM_WEAPONS; j++)
			{
				if (mApp->mWeapons[j] < 3)
					aChoices.push_back(j + HeavyWeaponApp::NUM_POWER_UPS);
			}
		}
	}
	if (aChoices.size() == 0)
		return -1;
	return aChoices[(unsigned int)Rand() % aChoices.size()];
}

// 0x41a160: a nuke (refused while the boss warning shows or the boss cannot be hurt): the screen flashes and shakes,
// every enemy low enough is killed (with a boss on the field each instead loses 1 / (level + 5) of its full health),
// every enemy bullet and destructible projectile is shot down for its points, the waves stop and start again, and the
// background animations that can show it are nuked
void Board::FireNuke()
{
	if (mBossWarning != NULL)
	{
		mApp->PlaySample(SOUND_DENIED, 0);
		return;
	}
	if (mBoss != NULL && mBossUntouchable)
	{
		mApp->PlaySample(SOUND_DENIED, 0);
		return;
	}
	for (int i = 0; i < NUM_ANIM_PLANES; i++)
	{
		BackgroundAnim* anAnim = mAnims[i];
		if (anAnim != NULL && anAnim->mHasNukedFrame && anAnim->mX < 640.0)
			anAnim->mIsNuked = true;
	}
	mApp->mPowerUps[HeavyWeaponApp::POWERUP_NUKES]--;
	mShake = 254;
	mNukeFlash = 255.0;

	for (mEnemies.mCurrent = mEnemies.mHead; mEnemies.mCurrent->mNext != NULL; )
	{
		mEnemies.mCurrent = mEnemies.mCurrent->mNext;
		Enemy* anEnemy = mEnemies.Current();
		if (anEnemy->mIsFriendly || !anEnemy->mIsAlive)
			continue;
		if (anEnemy->mY - (double)(anEnemy->GetCelHeight() / 2) >= (double)NUKE_REACH_Y)
			continue;
		if (mBoss == NULL)
		{
			anEnemy->Killed();
			anEnemy->Remove();
		}
		else
		{
			anEnemy->mHealth = anEnemy->mHealth - anEnemy->mMaxHealth / (double)(mApp->mLevel + 5);
			if (anEnemy->mHealth < 0.0)
			{
				anEnemy->Killed();
				if (anEnemy != mBoss)
					anEnemy->Remove();
			}
		}
	}
	for (mBullets.mCurrent = mBullets.mHead; mBullets.mCurrent->mNext != NULL; )
	{
		mBullets.mCurrent = mBullets.mCurrent->mNext;
		Bullet* aBullet = mBullets.Current();
		if (!aBullet->mIsEnemy)
			continue;
		AddScore(10, (int)aBullet->mX, (int)aBullet->mY);
		mBullets.RemoveCurrent();
	}
	for (mProjectiles.mCurrent = mProjectiles.mHead; mProjectiles.mCurrent->mNext != NULL; )
	{
		mProjectiles.mCurrent = mProjectiles.mCurrent->mNext;
		Projectile* aProjectile = mProjectiles.Current();
		if (!aProjectile->mIsDestructible)
			continue;
		AddScore(aProjectile->mPoints, (int)aProjectile->mX, (int)aProjectile->mY);
		mProjectiles.RemoveCurrent();
	}
	mWaveSpawners.Clear();
	mWaveDelay = 0;
	mNukeDropDelay = mIsSurvival ? SURVIVAL_NUKE_DROP_DELAY : mLevelLength / 6;
	mApp->PlaySample(SOUND_NUKEBLAST, 0);
	new NukeBlast(this);
}

// 0x41a5b0: theWave of theLevel's list starts; past the last list the last one's waves come, each line 1/7 more
// craft a level further
void Board::AddWaveSpawner(int theLevel, int theWave)
{
	double aScale = 1.0;
	int aLevelCount = (int)mApp->mWaves.size();
	int aListIndex = theLevel;
	if (theLevel >= aLevelCount)
	{
		aScale = (double)(theLevel - aLevelCount + 1) * 0.14285714285714285 + 1.0;
		aListIndex = aLevelCount - 1;
	}
	WaveInfo& aWave = mApp->mWaves[aListIndex][theWave];
	WaveSpawner* aSpawner = new WaveSpawner();
	for (int i = 0; i < (int)aWave.mCraft.size(); i++)
	{
		aSpawner->mCraft.push_back(aWave.mCraft[i]);
		aSpawner->mQuantities.push_back((int)((double)aWave.mQuantities[i] * aScale));
	}
	aSpawner->mTimeLeft = aWave.mLength;
	aSpawner->mLength = aWave.mLength;
	mWaveSpawners.Append(aSpawner);
}

// 0x41b340: a supply copter now and then (none while one's drops are out, a friendly is on the field or the mega
// laser is charged), the waves' craft sent in evenly over each wave's length, and a new wave once none is left
void Board::UpdateWaves()
{
	if (mNukeFlash != 0.0)
		return;
	int aLevel = mIsSurvival ? mLevelTime / SURVIVAL_LEVEL_TIME : mApp->mLevel;
	if (Rand() % 150 == 0 && mSupplyCopterDelay == 0 && mFriendlyCount == 0 && mMegaLaserTime == 0 &&
		mPowerUpDrops->mDrops.mCount == 0)
	{
		mSupplyCopterDelay = mIsSurvival ? SURVIVAL_SUPPLY_COPTER_DELAY : SUPPLY_COPTER_DELAY;
		int aType = ChoosePowerUp();
		if (aType >= 0)
			new SupplyCopter(this, aType);
	}

	for (mWaveSpawners.mCurrent = mWaveSpawners.mHead; mWaveSpawners.mCurrent->mNext != NULL; )
	{
		mWaveSpawners.mCurrent = mWaveSpawners.mCurrent->mNext;
		WaveSpawner* aSpawner = mWaveSpawners.Current();
		for (int i = 0; i < (int)aSpawner->mCraft.size(); i++)
		{
			int anInterval = aSpawner->mLength / aSpawner->mQuantities[i];
			int aTime = aSpawner->mTimeLeft;
			if ((double)(aTime / anInterval) == (double)aTime / (double)anInterval && aTime > 0)
				SpawnCraft(aSpawner->mCraft[i]);
		}
		if (aSpawner->mTimeLeft == 0)
			mWaveSpawners.RemoveCurrent();
		else
		{
			aSpawner->mTimeLeft--;
			mWaveDelay = 500;
		}
	}

	if (mWaveDelay == 0)
	{
		if (mApp->mLevel == 0 && mLevelTime < 2000)
			AddWaveSpawner(0, 0);
		else
		{
			int aListIndex = aLevel;
			if (aListIndex > (int)mApp->mWaves.size() - 1)
				aListIndex = (int)mApp->mWaves.size() - 1;
			AddWaveSpawner(aLevel, (unsigned int)Rand() % mApp->mWaves[aListIndex].size());
		}
	}
	else
		mWaveDelay--;
	if (mEnemies.mCount == mFriendlyCount && mWaveDelay > 100)
		mWaveDelay = 100;
}

// 0x41add0: the bullets move (a falling one speeds up to 3 an update); one that leaves the field or hits the ground
// (where it flashes) goes and counts as a miss, the enemies' too. Then each of the tank's bullets hits the first live
// enemy it touches, and each enemy bullet the shield (while it has a level or still glows from a hit), else the tank.
// Last, the first of the tank's bullets to touch a projectile hits it: one an update.
void Board::UpdateCollisions()
{
	for (mBullets.mCurrent = mBullets.mHead; mBullets.mCurrent->mNext != NULL; )
	{
		mBullets.mCurrent = mBullets.mCurrent->mNext;
		Bullet* aBullet = mBullets.Current();
		if (aBullet->mVelY < 3.0)
			aBullet->mVelY = aBullet->mGravity + aBullet->mVelY;
		aBullet->mX = aBullet->mVelX + aBullet->mX;
		aBullet->mY = aBullet->mVelY + aBullet->mY;
		bool isOnField = aBullet->mX >= -320.0 && aBullet->mX <= 320.0 && aBullet->mY >= 0.0;
		if (isOnField && aBullet->mY <= 460.0)
			continue;
		if (aBullet->mY > 460.0)
			AddGroundFlash((int)aBullet->mX, !aBullet->mIsEnemy);
		mBullets.RemoveCurrent();
		mApp->mStats[HeavyWeaponApp::STAT_SHOTS_MISSED]++;
		mShotsMissed++;
	}

	for (mBullets.mCurrent = mBullets.mHead; mBullets.mCurrent->mNext != NULL; )
	{
		mBullets.mCurrent = mBullets.mCurrent->mNext;
		Bullet* aBullet = mBullets.Current();
		if (!aBullet->mIsEnemy)
		{
			Enemy* aTarget = NULL;
			for (mEnemies.mCurrent = mEnemies.mHead; mEnemies.mCurrent->mNext != NULL; )
			{
				mEnemies.mCurrent = mEnemies.mCurrent->mNext;
				Enemy* anEnemy = mEnemies.Current();
				if (!anEnemy->mIsAlive)
					continue;
				if (aBullet->mSprite->Collides((int)aBullet->mX, (int)aBullet->mY, aBullet->mColumn, 0, false,
						anEnemy->GetSprite(), (int)anEnemy->mX, (int)anEnemy->mY, (int)anEnemy->mFrame, 0,
						anEnemy->mMirrored, true))
				{
					aTarget = anEnemy;
					break;
				}
			}
			if (aTarget == NULL)
				continue;
			aTarget->mHitFlash = 255;
			aTarget->mHealth = aTarget->mHealth - aBullet->mDamage;
			if (aTarget->mHealth >= 0.0)
			{
				mApp->PlaySample(SOUND_BULLETHIT, (int)(aBullet->mX * PAN_SCALE));
				mParticles.Emit(mApp->mSprites[IMAGE_SPARK], aBullet->mX, aBullet->mY, aTarget->mVelX,
					aTarget->mVelY, (int)(mApp->mParticles * 10.0), 2.0, 0.05, 3.0, 30, true, -1);
			}
			else
			{
				aTarget->Killed();
				if (aTarget != mBoss)
					aTarget->Remove();
			}
			mBullets.RemoveCurrent();
			continue;
		}

		if (mApp->mPowerUps[HeavyWeaponApp::POWERUP_SHIELD] != 0 || mShieldHitGlow > 0.0)
		{
			if (aBullet->mSprite->Collides((int)aBullet->mX, (int)aBullet->mY, aBullet->mColumn, 0, false,
					mApp->mSprites[IMAGE_SHIELD], (int)mTankX, mTankY + 2, (int)mShieldFrame, 0, false, true))
			{
				mBullets.RemoveCurrent();
				if (mShieldHitGlow <= 0.0)
				{
					mApp->mPowerUps[HeavyWeaponApp::POWERUP_SHIELD]--;
					if (mApp->mPowerUps[HeavyWeaponApp::POWERUP_SHIELD] == 0)
						mApp->PlaySample(SOUND_SHIELDDOWN, (int)(mTankX * PAN_SCALE));
					mShieldHitGlow = 9.0;
					mApp->PlaySample(SOUND_SPARKING, (int)(mTankX * PAN_SCALE));
				}
				continue;
			}
		}
		if (aBullet->mSprite->Collides((int)aBullet->mX, (int)aBullet->mY, aBullet->mColumn, 0, false,
				mApp->mSprites[IMAGE_TANK], (int)mTankX, mTankY, (int)mTreadFrame, 0, false, true) &&
			mTankDeadTime == 0)
		{
			mBullets.RemoveCurrent();
			KillTank();
		}
	}

	for (mBullets.mCurrent = mBullets.mHead; mBullets.mCurrent->mNext != NULL; )
	{
		mBullets.mCurrent = mBullets.mCurrent->mNext;
		Bullet* aBullet = mBullets.Current();
		if (aBullet->mIsEnemy)
			continue;
		if (HitProjectiles(aBullet->mSprite, (int)aBullet->mX, (int)aBullet->mY, aBullet->mColumn, 0,
				aBullet->mDamage))
		{
			mBullets.RemoveCurrent();
			return;
		}
	}
}

// 0x41b690
void Board::Update()
{
	Widget::Update();
	MarkDirty();
	if (mSuspended)
		return;
	if (mApp->mCustomCursorsEnabled && mResultsScreen == NULL)
		mApp->SetCursorImage(0, mApp->mTargetImages[(mApp->mUpdateCount / 4) % HeavyWeaponApp::NUM_TARGET_IMAGES]);

	if (mTankDeadTime < 1)
	{
		mHasStarted = true;
		if (mLevelTime != mLevelLength || mIsSurvival)
			mLevelTime++;
		if (mIsSurvival)
			mLevelTime++;
		UpdateOrbs();
	}
	else
	{
		if (mTankDeadTime == READY_TIME && mHasStarted)
		{
			if (mApp->mLives < 1)
			{
				mApp->PlaySample(SOUND_V_GAMEOVER, 0);
				new Banner(this, mApp->mFonts[FONT_RUBBERSTAMPLET42], GT(0x52c978), 240, 40);
			}
			else
			{
				mApp->PlaySample(SOUND_V_GETREADY, 0);
				new Banner(this, mApp->mFonts[FONT_RUBBERSTAMPLET42], GT(0x52c984), 240, 15);
			}
		}
		if (mTankDeadTime > 1)
			mTankDeadTime--;
		if (mTankDeadTime == 1 && mLevelEndBlocks == 0 && mProjectiles.mCount == 0)
		{
			if (mApp->mLives == 0)
			{
				Suspend();
				mApp->AddDialog(DIALOG_GAME_OVER, new GameOverDialog(mApp));
				return;
			}
			mApp->mLives--;
			mTankDeadTime = 0;
		}
	}

	if (!mIsSurvival)
	{
		if (mLevelTime == mLevelLength - BOSS_WARNING_TIME)
		{
			// the warning waits until no enemy bullet or destructible projectile is left
			int aShotsLeft = 0;
			for (mBullets.mCurrent = mBullets.mHead; mBullets.mCurrent->mNext != NULL; )
			{
				mBullets.mCurrent = mBullets.mCurrent->mNext;
				if (mBullets.Current()->mIsEnemy)
					aShotsLeft++;
			}
			for (mProjectiles.mCurrent = mProjectiles.mHead; mProjectiles.mCurrent->mNext != NULL; )
			{
				mProjectiles.mCurrent = mProjectiles.mCurrent->mNext;
				if (mProjectiles.Current()->mIsDestructible)
					aShotsLeft++;
			}
			if (mLevelEndBlocks == 0 && aShotsLeft == 0)
			{
				mBossWarning = new BossWarning(this);
				mWaveSpawners.Clear();
				mApp->mMusicInterface->FadeOut(SONG_ATOMIC_TANK_0, true, MUSIC_FADE_OUT_SPEED);
			}
			else
				mLevelTime--;
		}
		if (mLevelTime == mLevelLength - 1)
		{
			switch (mApp->mLevel % NUM_BOSSES)
			{
			case 0:
				if (mApp->mLevel == HeavyWeaponApp::FINAL_LEVEL)
					mBoss = new FinalBoss(this);
				else
					mBoss = new HelicopterBoss(this);
				break;
			case 1:
				mBoss = new BattleshipBoss(this);
				break;
			case 2:
				mBoss = new RainerBoss(this);
				break;
			case 3:
				mBoss = new WreckerBoss(this);
				break;
			case 4:
				mBoss = new ApeBoss(this);
				break;
			case 5:
				mBoss = new EyeBoss(this);
				break;
			case 6:
				mBoss = new HeadBoss(this);
				break;
			case 7:
				mBoss = new WormBoss(this);
				break;
			case 8:
				mBoss = new RobotBoss(this);
				break;
			}
		}
		if (mLevelTime == mLevelLength + RESULTS_TIME)
		{
			mResultsScreen = new ResultsScreen(mApp);
			mWidgetManager->AddWidget(mResultsScreen);
			mWidgetManager->SetFocus(mResultsScreen);
			mApp->mMusicInterface->FadeOut(SONG_ATOMIC_TANK_42, true, MUSIC_FADE_OUT_SPEED);
		}
		int aTime = mLevelTime;
		if ((double)(aTime / 5000) == (double)aTime * 0.0002 && aTime > 0 && aTime < mLevelLength)
			mBannerTime--;
	}

	// the original counts the board's updates at the app's +0xb34 here; nothing reads it
	if (mShake != 0)
		UpdateShake();
	if (mNukeFlash != 0.0)
		mNukeFlash = mNukeFlash - 0.75;
	if (mNukeFlash < 0.0)
		mNukeFlash = 0.0;
	if (mSupplyCopterDelay != 0)
		mSupplyCopterDelay--;
	if (mMegaLaserPartDelay != 0)
		mMegaLaserPartDelay--;
	if (mNoNukesTime != 0)
		mNoNukesTime--;
	if (mNukeDropDelay != 0)
		mNukeDropDelay--;
	if (mShieldDropDelay != 0)
		mShieldDropDelay--;
	if (mShieldFlash != 0)
		mShieldFlash -= 4;
	if (mShieldFlash < 0)
		mShieldFlash = 0;
	if (mBossDeath != NULL)
		UpdateBossDeath();
	if (mBossWarning != NULL)
		mBossWarning->Update();
	UpdateEnemies();
	UpdateCollisions();
	UpdateTankShots();
	UpdateProjectiles();
	UpdateExplosions();
	mParticles.Update();
	mPowerUpDrops->Update();
	UpdateFloatingTexts();
	UpdateTreadMarks();
	UpdateNukeBlasts();
	UpdateLasers();

	// the mega laser fires on for 10 updates after the button is let go; while it is charged no waves wait
	if (mMegaLaserFiring != 0)
		mMegaLaserFiring--;
	if (mMegaLaserTime != 0)
	{
		if (mMouseDown)
			mMegaLaserFiring = 10;
		mWaveDelay = 0;
	}
	if (mMegaLaserFiring != 0)
		UpdateMegaLaser();

	UpdateMegaLaserParts();
	UpdateShellCasings();
	UpdateMuzzleFlashes();
	UpdateSparks();
	UpdateBanners();
	StartAnims();
	if (mDeathTime != 0)
		mDeathTime++;
	if (mDeathTime > NEXT_TANK_TIME)
		mDeathTime = 0;
	if (mShieldHitGlow > 0.0)
		mShieldHitGlow = mShieldHitGlow - 0.3;
	if (mTankDeadTime == 0)
	{
		UpdateTank();
		if (mTankDeadTime == 0 && (mLevelTime < mLevelLength - LAST_WAVE_TIME || mIsSurvival))
			UpdateWaves();
	}
	if (mBannerTime < 0)
		mBannerTime--;
	if (mBannerTime < -705)
		mBannerTime = 0;
}

// 0x419e30: the scrolling and everything that moves with the frame (background animations, ground flashes, each
// enemy's own), unless the game is stopped
void Board::UpdateF(float theFrac)
{
	if (mSuspended)
		return;
	UpdateScroll(theFrac);
	UpdateAnims(theFrac);
	UpdateGroundFlashes(theFrac);
	for (mEnemies.mCurrent = mEnemies.mHead; mEnemies.mCurrent->mNext != NULL; )
	{
		mEnemies.mCurrent = mEnemies.mCurrent->mNext;
		mEnemies.Current()->UpdateF(theFrac);
	}
}
