#include "HeavyWeaponApp.h"
#include "FlickerLight.h"
#include "Armory.h"
#include "Board.h"
#include "StatusBar.h"
#include "Compat2004.h"
#include "CreditsScreen.h"
#include "GameText.h"
#include "HelpScreen.h"
#include "MainMenu.h"
#include "MapScreen.h"
#include "OptionsDialog.h"
#include "Sprite.h"
#include "TitleScreen.h"
#include "UserNameDialog.h"

#include "Buffer.h"
#include "Checkbox.h"
#include "DDImage.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MusicInterface.h"
#include "SDLMusicInterface.h"
#include "SoundManager.h"
#include "WidgetManager.h"
#include "XMLParser.h"

#include <SDL.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>

using namespace Sexy;

HeavyWeaponApp* Sexy::gApp = NULL;

// The players' file: a version byte (0, or below 0x15 for the old format) and one UserRecord a player
static const char* const USERS_FILE = "userdata\\users.dat";
static const char* const HIGH_SCORES_FILE = "userdata\\hiscores.dat";
// The size of hiscores.dat: the ten lines of each list, a campaign line and a survival line in turn
static const int HIGH_SCORES_FILE_SIZE = HeavyWeaponApp::NUM_HIGH_SCORES * (sizeof(HighScoreRecord) + sizeof(SurvivalScoreRecord));

// The high-score lists of a new installation (0x5275c8 and 0x527708): the names are game texts, at these addresses
struct DefaultHighScore
{
	unsigned int	mNameText;
	int				mScore;
	int				mLevel;
};

static const DefaultHighScore DEFAULT_HIGH_SCORES[HeavyWeaponApp::NUM_HIGH_SCORES] = {
	{GTA(0x5275c8), 60000, 3}, {GTA(0x5275e8), 50000, 2}, {GTA(0x527608), 40000, 2}, {GTA(0x527628), 30000, 2}, {GTA(0x527648), 20000, 1},
	{GTA(0x527668), 15000, 1}, {GTA(0x527688), 10000, 1}, {GTA(0x5276a8), 5000, 0}, {GTA(0x5276c8), 2500, 0}, {GTA(0x5276e8), 1000, 0}};

struct DefaultSurvivalScore
{
	unsigned int	mNameText;
	int				mTime;
};

static const DefaultSurvivalScore DEFAULT_SURVIVAL_SCORES[HeavyWeaponApp::NUM_HIGH_SCORES] = {
	{GTA(0x527708), 30000}, {GTA(0x527724), 27000}, {GTA(0x527740), 24000}, {GTA(0x52775c), 21000}, {GTA(0x527778), 18000},
	{GTA(0x527794), 15000}, {GTA(0x5277b0), 12000}, {GTA(0x5277cc), 9000}, {GTA(0x5277e8), 6000}, {GTA(0x527804), 3000}};

// 0x40b750
HeavyWeaponApp::HeavyWeaponApp()
{
	gApp = this; // port: the original is the static global 0x56b1a0

	mProdName = "HeavyWeapon";
	mProductVersion = "1.0";
	// port: the original reads the game's name from 0x52a548; the texts are read from the exe only in Init, after the
	// window is made, so the name is written here as the other ports write theirs
	mTitle = "Heavy Weapon Deluxe " + mProductVersion;
	mRegKey = "PopCap\\" + mProdName;
	mCustomCursorsEnabled = true;
	mAutoEnable3D = true;
	mTest3D = true;
	mCurrentUser = 0;
	mMapScreen = NULL;
	mIsNewUser = false;
	mHelpScreen = NULL;
	mBoard = NULL;
	mArmory = NULL;
	mTitleScreen = NULL;
	mLoadingCompleteShown = false;
	mTraceFile = NULL;
	mMainMenu = NULL;
	mMusicVolume = 0.3;
	mSfxVolume = 0.9;

	// port: the original leaves these to the static global's zeroed memory
	for (int i = 0; i < NUM_IMAGES; i++)
		mSprites[i] = NULL;
	for (int i = 0; i < NUM_TARGET_IMAGES; i++)
		mTargetImages[i] = NULL;
	for (int i = 0; i < NUM_FONTS; i++)
		mFonts[i] = NULL;
	mScore = 0;
	mLevel = 0;
	mLives = 0;
	for (int i = 0; i < NUM_POWER_UPS; i++)
		mPowerUps[i] = 0;
	for (int i = 0; i < NUM_WEAPONS; i++)
		mWeapons[i] = 0;
	for (int i = 0; i < NUM_SOUNDS; i++)
		mSoundDelays[i] = 0;
	for (int i = 0; i < NUM_STATS; i++)
		mStats[i] = 0;
	mParticles = 1.0;
	mHelpPending = false;
	mTipsShown = 0;
	for (int i = 0; i < NUM_CRAFT; i++)
	{
		mCraft[i].mPoints = 0;
		mCraft[i].mIndex = 0;
		mCraft[i].mArmor = 0.0;
	}
}

// 0x40b980. port: the original frees only its vectors and leaves the images, fonts and screens to the process's end;
// the port frees them
HeavyWeaponApp::~HeavyWeaponApp()
{
	if (mTraceFile != NULL)
		fclose(mTraceFile);
	for (int i = 0; i < NUM_IMAGES; i++)
		delete mSprites[i];
	for (int i = 0; i < NUM_TARGET_IMAGES; i++)
		delete mTargetImages[i];
	for (int i = 0; i < NUM_FONTS; i++)
		delete mFonts[i];
	for (size_t aLevel = 0; aLevel < mAnims.size(); aLevel++)
		for (size_t anAnim = 0; anAnim < mAnims[aLevel].size(); anAnim++)
			delete mAnims[aLevel][anAnim].mSprite;
	if (gApp == this)
		gApp = NULL;
}

// 0x401040 slot 70: back in the window, a paused game is redrawn and the sound stays off until the game is resumed
void HeavyWeaponApp::GotFocus()
{
	if ((mBoard != NULL) && (mBoard->mPaused))
	{
		mBoard->MarkDirty();
		mMusicInterface->SetVolume(0.0);
		mSoundManager->SetVolume(0.0);
	}
}

// 0x401020 slot 71: a game in play pauses when the window loses the focus
void HeavyWeaponApp::LostFocus()
{
	if (mWidgetManager->mFocusWidget == mBoard)
		mBoard->Pause();
}

// 0x401080 slot 4
void HeavyWeaponApp::ButtonPress(int theId)
{
	PlaySample(SOUND_BUTTONDOWN, 0);
}

// 0x401090: the armory before a level, in place of the game that ended
void HeavyWeaponApp::ShowArmory()
{
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = NULL;
	}
	mArmory = new Armory(this);
	mWidgetManager->AddWidget(mArmory);
	mWidgetManager->SetFocus(mArmory);
	mArmory->MarkDirty();
}

// 0x401150 slot 46: a sound plays only once its last play is 10 updates old (20 for the two explosions, 6 and 7; 5
// for the stats screen's beep)
void HeavyWeaponApp::PlaySample(int theSoundNum, int thePan)
{
	if (mSoundDelays[theSoundNum] < 1)
	{
		mSoundDelays[theSoundNum] = 10;
		if ((theSoundNum == SOUND_BIGEXPLODE) || (theSoundNum == SOUND_SMALLEXPLODE))
			mSoundDelays[theSoundNum] = 20;
		if (theSoundNum == SOUND_STATBEEP)
			mSoundDelays[SOUND_STATBEEP] = 5;
		SexyApp::PlaySample(theSoundNum, thePan);
	}
}

// 0x4011b0: one of the tank's six special weapons at its level, for the armory and the game. theTurretFrame is the gun's
// angle as a frame of its strip (the lasers and flak guns turn with it, the rocket pods aim along it).
void HeavyWeaponApp::DrawUpgradeWeapon(Graphics* g, int theWeapon, int x, int y, double theTurretFrame)
{
	switch (theWeapon)
	{
	case WEAPON_ORBS:
		if (mWeapons[WEAPON_ORBS] > 0)
		{
			if (mBoard == NULL)
			{
				// The armory: the orbs circle the tank, 120 degrees apart (180 for two)
				int anOrbCount = mWeapons[WEAPON_ORBS];
				int anAngle = (mUpdateCount % 360) * (4 - anOrbCount) * 3;
				if (anOrbCount > 0)
				{
					double aY = (double) y;
					double aX = (double) x;
					int anAngle3 = anAngle;		// three orbs: 120 degrees apart
					int anAngle2 = anAngle;		// two: 180
					for (int anOrb = 0; anOrb < mWeapons[WEAPON_ORBS]; anOrb++)
					{
						double aRadians = (double) ((anOrbCount == 2) ? anAngle2 : anAngle3) * 0.017453292520882225;
						int anOrbY = (int) (sin(aRadians) * 40.0 + aY);
						int anOrbX = (int) (cos(aRadians) * 50.0 + aX);
						mSprites[IMAGE_ORB]->Draw(g, anOrbX, anOrbY, true, 0, 0, false);
						anAngle3 += 120;
						anAngle2 += 180;
						anOrbCount = mWeapons[WEAPON_ORBS];
					}
				}
			}
			else if ((mBoard->mTankDeadTime == 0) && (mBoard->mResultsScreen == NULL))
			{
				// In a game, while the tank is on the field: each orb where the board moved it, flashed white as it is hit
				for (int anOrb = 0; anOrb < mWeapons[WEAPON_ORBS]; anOrb++)
				{
					DefenseOrb& anOrbState = mBoard->mOrbs[anOrb];
					mSprites[IMAGE_ORB]->Draw(g, (int) anOrbState.mX, (int) anOrbState.mY, true, 0, 0, false);
					if (anOrbState.mFlash > 0.0)
					{
						g->SetColorizeImages(true);
						g->SetColor(Color(0xff, 0xff, 0xff));
						g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
						int aBrightness = (int) (mBoard->mOrbs[anOrb].mFlash * 255.0);
						g->SetColor(Color(aBrightness, aBrightness, aBrightness));
						mSprites[IMAGE_ORB]->Draw(g, (int) mBoard->mOrbs[anOrb].mX, (int) mBoard->mOrbs[anOrb].mY, true, 1, 0,
							false);
						g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
						g->SetColorizeImages(false);
					}
				}
			}
		}
		break;
	case WEAPON_HOMING:
		if (mWeapons[WEAPON_HOMING] > 0)
			mSprites[IMAGE_HOMING]->Draw(g, x, y - 2, true, mWeapons[WEAPON_HOMING] - 1, 0, false);
		break;
	case WEAPON_LASERS:
		if (mWeapons[WEAPON_LASERS] > 0)
			mSprites[IMAGE_LASERS]->Draw(g, x - 0x28, y - 0x2a, false, (int) theTurretFrame,
				mWeapons[WEAPON_LASERS] - 1, false);
		break;
	case WEAPON_ROCKETS:
		if (mWeapons[WEAPON_ROCKETS] > 0)
		{
			// The pods stand in a row across the gun's line, 90 pixels between the outer ones
			double anAngle = theTurretFrame * 9.0 * 0.017453292520882225;
			int aFrame = (int) theTurretFrame;
			y -= 0xc;
			double aBaseY = (double) y - sin(anAngle) * 12.0;
			double aSin = sin(1.5707963268794 + anAngle);
			double aBaseX = (double) x - cos(anAngle) * 12.0;
			double aCos = cos(1.5707963268794 + anAngle);
			for (int aPod = 0; aPod < mWeapons[WEAPON_ROCKETS]; aPod++)
			{
				int anOffset = (0x5a / mWeapons[WEAPON_ROCKETS]) * aPod;
				if (mWeapons[WEAPON_ROCKETS] > 1)
					anOffset += -0x16 - ((mWeapons[WEAPON_ROCKETS] == 3) ? 8 : 0);
				int aPodY = (int) (aBaseY - aSin * (double) anOffset);
				int aPodX = (int) (aBaseX - aCos * (double) anOffset);
				mSprites[IMAGE_ROCKETPOD]->Draw(g, aPodX, aPodY, true, aFrame, 0, false);
			}
		}
		break;
	case WEAPON_FLAK:
		if (mWeapons[WEAPON_FLAK] > 0)
			mSprites[IMAGE_FLAKGUNS]->Draw(g, x - 0x28, y - 0x34, false, (int) theTurretFrame,
				mWeapons[WEAPON_FLAK] - 1, false);
		break;
	case WEAPON_STATIC:
		if (mWeapons[WEAPON_STATIC] > 0)
			mSprites[IMAGE_STATICSTRIKE]->Draw(g, x, y + 4, true, 0, mWeapons[WEAPON_STATIC] - 1, false);
		break;
	}
}

// 0x4015f0: the boss of a level: the survival game's own, the last level's, or one of the nine in turn
int HeavyWeaponApp::GetBossIndex(int theLevel)
{
	if ((mBoard != NULL) && (mBoard->mIsSurvival))
		return BOSS_SURVIVAL;
	if (theLevel == FINAL_LEVEL)
		return BOSS_FINAL;
	return theLevel % 9;
}

// 0x401630: the credits, over the main menu
void HeavyWeaponApp::ShowCredits()
{
	CreditsScreen* aCredits = new CreditsScreen(this);
	mWidgetManager->AddWidget(aCredits);
	mWidgetManager->SetFocus(aCredits);
	if (mMainMenu != NULL)
	{
		mWidgetManager->RemoveWidget(mMainMenu);
		delete mMainMenu->mFlickerLight;
		mMainMenu->mFlickerLight = NULL;
	}
}

// 0x4016f0 slot 60: the game pauses, and an open options dialog's full-screen box follows the new mode
void HeavyWeaponApp::SwitchScreenMode(bool wantWindowed)
{
	if (mBoard != NULL)
		mBoard->Pause();
	OptionsDialog* anOptions = (OptionsDialog*) GetDialog(DIALOG_OPTIONS);
	if (anOptions != NULL)
		anOptions->mWindowedCheckbox->SetChecked(mIsWindowed, false);
	SexyApp::SwitchScreenMode(wantWindowed);
}

// 0x401740 slot 59
void HeavyWeaponApp::SwitchScreenMode(bool wantWindowed, bool is3d, bool force)
{
	SexyApp::SwitchScreenMode(wantWindowed, is3d, force);
}

// 0x401a80: the main menu; with no player yet, the dialog that asks for a name over it
void HeavyWeaponApp::ShowMainMenu()
{
	mMainMenu = new MainMenu(this);
	mWidgetManager->AddWidget(mMainMenu);
	mWidgetManager->SetFocus(mMainMenu);
	mMainMenu->MarkDirty();
	mHelpPending = true;
	if (mUsers.size() != 0)
		return;
	AddDialog(DIALOG_USER_NAME, new UserNameDialog(this, false, 0));
}

// 0x401b80: the tank at x, y (its centre): its shadow, the body at its tread frame, the gun at its angle and level, and
// the upgrades on it. In a game, the shield's glow (its colour by the shield's level) and its flash as it is hit.
void HeavyWeaponApp::DrawTank(Graphics* g, int x, int y, int theTreadFrame, double theTurretFrame)
{
	mSprites[IMAGE_TANKSHADOW]->Draw(g, x, y + 0x16, true, 0, 0, false);
	mSprites[IMAGE_TANK]->Draw(g, x, y, true, theTreadFrame, 0, false);
	int aTurretFrame = (int) theTurretFrame;
	mSprites[IMAGE_GUN]->Draw(g, x - 0x28, y - 0x34, false, aTurretFrame, mPowerUps[POWERUP_SPREAD], false);
	if (mHelpScreen != NULL)
		return;

	if (mWeapons[WEAPON_LASERS] > 0)
		mSprites[IMAGE_LASERS]->Draw(g, x - 0x28, y - 0x2a, false, aTurretFrame, mWeapons[WEAPON_LASERS] - 1, false);
	if (mWeapons[WEAPON_FLAK] > 0)
		mSprites[IMAGE_FLAKGUNS]->Draw(g, x - 0x28, y - 0x34, false, aTurretFrame, mWeapons[WEAPON_FLAK] - 1, false);
	if (mWeapons[WEAPON_HOMING] > 0)
		mSprites[IMAGE_HOMING]->Draw(g, x, y - 2, true, mWeapons[WEAPON_HOMING] - 1, 0, false);
	if (mWeapons[WEAPON_STATIC] > 0)
		mSprites[IMAGE_STATICSTRIKE]->Draw(g, x, y + 4, true, 0, mWeapons[WEAPON_STATIC] - 1, false);
	DrawUpgradeWeapon(g, WEAPON_ROCKETS, x, y, theTurretFrame);
	DrawUpgradeWeapon(g, WEAPON_ORBS, x, y, theTurretFrame);

	int aShield = mPowerUps[POWERUP_SHIELD];
	if ((aShield == 0) || (mBoard == NULL))
		return;
	if (aShield == 1)
		g->SetColor(Color(0xff, 0x40, 0, 0x80));
	else if (aShield == 2)
		g->SetColor(Color(0xff, 0xff, 0, 0x80));
	else if (aShield == 3)
		g->SetColor(Color(0, 0xff, 0, 0x80));
	g->SetColorizeImages(true);
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	mSprites[IMAGE_SHIELD]->Draw(g, x, y + 2, true, (mUpdateCount % 100) / 10, 0, false);
	int aFlash = mBoard->mShieldFlash;
	if (aFlash != 0)
	{
		// The hit: the shield's frame again, brighter and larger by the flash
		g->SetColor(Color(aFlash, aFlash, aFlash));
		double aScale = (double) mBoard->mShieldFlash * 0.00392156862745098 + 1.0;
		double aWidth = (double) mSprites[IMAGE_SHIELD]->mCelWidth;
		int aHeight = mSprites[IMAGE_SHIELD]->mCelHeight;
		Rect aSrcRect(((mUpdateCount % 100) / 10) * 0x50, 0, 0x50, 0x46);
		Rect aDestRect;
		aDestRect.mX = (int) ((double) x - aWidth * 0.5 * aScale);
		aDestRect.mY = (int) ((double) y - (double) aHeight * 0.5 * aScale);
		aDestRect.mWidth = (int) (aWidth * aScale);
		aDestRect.mHeight = (int) ((double) aHeight * aScale);
		g->DrawImage(mSprites[IMAGE_SHIELD]->mImage, aDestRect, aSrcRect);
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColorizeImages(false);
}

// 0x401ec0: how to play, in place of the main menu. theListener is who the OK button reports to (id 0x3f); with none
// the help screen closes itself
HelpScreen* HeavyWeaponApp::ShowHelp(ButtonListener* theListener)
{
	mHelpScreen = new HelpScreen(this, theListener);
	mWidgetManager->AddWidget(mHelpScreen);
	mWidgetManager->SetFocus(mHelpScreen);
	if (mMainMenu != NULL)
	{
		mWidgetManager->RemoveWidget(mMainMenu);
		delete mMainMenu->mFlickerLight;
		mMainMenu->mFlickerLight = NULL;
	}
	return mHelpScreen;
}

// 0x402860: the campaign so far into the player's record (the furthest level, with its score, only when it is passed)
void HeavyWeaponApp::SaveUserProgress()
{
	UserRecord& aUser = mUsers[mCurrentUser];
	if ((int) aUser.mMaxLevel < mLevel)
	{
		aUser.mMaxLevel = (unsigned short) mLevel;
		mUsers[mCurrentUser].mBestScore = mScore;
	}
	mUsers[mCurrentUser].mScore = mScore;
	for (int i = 0; i < NUM_STATS; i++)
		mUsers[mCurrentUser].mStats[i] = mStats[i];
	mUsers[mCurrentUser].mLevel = (unsigned short) mLevel;
	for (int i = 0; i < NUM_POWER_UPS; i++)
		mUsers[mCurrentUser].mPowerUps[i] = (unsigned short) mPowerUps[i];
	for (int i = 0; i < NUM_WEAPONS; i++)
		mUsers[mCurrentUser].mWeapons[i] = (unsigned short) mWeapons[i];
}

// The framework's image flag (MemoryImage::mD3DFlags) that keeps a texture 32-bit in 3D (GPUInterface.h, UseA8R8G8B8)
static const DWORD IMAGE_FLAG_USE_A8R8G8B8 = 8;

// The level's five background images (sky, far and near background, map, ground) for the set of levelName
// (StartLevel 0x403e20 inline)
static void LoadLevelBackgrounds(HeavyWeaponApp* theApp, const std::string& theSetName, bool hasMap)
{
	std::string aPath = std::string("images\\backgrounds\\") + theSetName;
	theApp->mSprites[IMAGE_LEVEL_SKY]->Load(aPath + "_sky", 1, 1);
	theApp->mSprites[IMAGE_LEVEL_SKY]->mImage->mD3DFlags |= IMAGE_FLAG_USE_A8R8G8B8;
	theApp->mSprites[IMAGE_LEVEL_BG]->Load(aPath + "_bg", 1, 1);
	theApp->mSprites[IMAGE_LEVEL_BG2]->Load(aPath + "_bg2", 1, 1);
	if (hasMap)
		theApp->mSprites[IMAGE_LEVEL_MAP]->Load(aPath + "_map", 1, 1);
	theApp->mSprites[IMAGE_LEVEL_GROUND]->Load(aPath + "_ground", 1, 1);
}

// 0x403e20: a campaign level: the board behind the dialogs, then the level's backgrounds (its set by the boss: the nine
// in turn, the last level's own)
void HeavyWeaponApp::StartLevel()
{
	mLives = CAMPAIGN_LIVES;
	mBoard = new Board(this, false);
	mWidgetManager->AddWidget(mBoard);
	mWidgetManager->BringToBack(mBoard);
	mWidgetManager->SetFocus(mBoard);
	mBoard->MarkDirty();
	LoadLevelBackgrounds(this, mLevels[GetBossIndex(mLevel)].mName, true);
}

// 0x4041b0 slot 20 (runs in one go in the port): the images after the first eleven, then the sounds after the first.
// A file that does not load ends the loading with a message.
void HeavyWeaponApp::LoadingThreadProc()
{
	mNumLoadingThreadTasks = NUM_LOADING_TASKS;
	mCompletedLoadingThreadTasks = 0;
	for (int anImage = NUM_INIT_IMAGES; anImage < NUM_IMAGES; anImage++)
	{
		const ImageInfo& anInfo = gImageInfo[anImage];
		mSprites[anImage] = new Sprite(this);
		if (!mSprites[anImage]->Load(std::string("images\\") + anInfo.mFile, anInfo.mColumns, anInfo.mRows))
		{
			Popup(GT(0x52a180) + anInfo.mFile);
			mLoadingFailed = true;
			return;
		}
		// The armory's, the main menu's and the help screen's backgrounds are 32-bit textures in 3D
		if ((anImage == IMAGE_ARMORY) || (anImage == IMAGE_MAINMENU) || (anImage == IMAGE_HELP))
			mSprites[anImage]->mImage->mD3DFlags |= IMAGE_FLAG_USE_A8R8G8B8;
		if (anInfo.mAlphaMask)
			mSprites[anImage]->GetAlphaMask();
		mCompletedLoadingThreadTasks++;
	}

	if (!mSoundManager->Initialized())
	{
		mCompletedLoadingThreadTasks += NUM_SOUNDS;
		return;
	}
	for (int aSound = 1; aSound < NUM_SOUNDS; aSound++)
	{
		const SoundInfo& anInfo = gSoundInfo[aSound];
		if (!mSoundManager->LoadSound(aSound, std::string("sounds\\") + anInfo.mFile))
		{
			Popup(GT(0x52a168) + anInfo.mFile);
			mLoadingFailed = true;
			return;
		}
		mSoundManager->SetBaseVolume(aSound, anInfo.mBaseVolume);
		mCompletedLoadingThreadTasks++;
	}
}

// 0x404c90: the players to userdata\users.dat (the file is removed when there is none)
void HeavyWeaponApp::SaveUsers()
{
	if (mUsers.size() == 0)
	{
		EraseFile(USERS_FILE);
		return;
	}
	Buffer aBuffer;
	aBuffer.WriteByte(0);
	for (size_t i = 0; i < mUsers.size(); i++)
		aBuffer.WriteBytes((const uchar*) &mUsers[i], sizeof(UserRecord));
	WriteBufferToFile(USERS_FILE, &aBuffer);
}

// 0x404e80: both high-score lists to userdata\hiscores.dat, a campaign line and a survival line in turn
void HeavyWeaponApp::SaveHighScores()
{
	Buffer aBuffer;
	for (int i = 0; i < NUM_HIGH_SCORES; i++)
	{
		aBuffer.WriteBytes((const uchar*) &mHighScores[i], sizeof(HighScoreRecord));
		aBuffer.WriteBytes((const uchar*) &mSurvivalScores[i], sizeof(SurvivalScoreRecord));
	}
	WriteBufferToFile(HIGH_SCORES_FILE, &aBuffer);
}

// 0x405080: a message, then the game closes
void HeavyWeaponApp::FatalError(const std::string& theMessage)
{
	MsgBox(theMessage, GT(0x52a3dc), MB_OK);
	Shutdown();
}

// 0x406ab0 slot 8: once the loading is done the title screen offers to play; the sounds' delays count down
// (PlaySample)
void HeavyWeaponApp::UpdateFrames()
{
	// port: a test's trace (testhooks) first writes the state the last update left, before this update's script input
	if (mTraceFile != NULL)
		TraceFrame();
	SexyApp::UpdateFrames();
	if (mLoadingThreadCompleted && !mLoadingCompleteShown)
	{
		mLoadingCompleteShown = true;
		// 0x4045d0: the original first shows the trial's boxes (expired, time or plays left, register now) and the
		// update check's; with registration and the update check left out, only its last step remains
		mTitleScreen->LoadingComplete();
	}
	for (int aSound = 0; aSound < NUM_SOUNDS; aSound++)
	{
		if (mSoundDelays[aSound] > 0)
			mSoundDelays[aSound]--;
	}
}

// 0x406b20 slot 38: the last player and the particle setting to the registry, the players and high scores to their files
void HeavyWeaponApp::Shutdown()
{
	if (!mShutdown)
	{
		RegistryWriteInteger("LastUser", mCurrentUser);
		RegistryWriteInteger("Particles", (int) (mParticles * 10000.0));
		if (mUsers.size() != 0)
			SaveUsers();
		SaveHighScores();
		SexyApp::Shutdown();
	}
}

// 0x406d70: the craft table's index of a name in waves.xml; a name it does not have ends the game with a message
int HeavyWeaponApp::GetCraftIndex(const std::string& theName)
{
	int anIndex = 0;
	while (mCraft[anIndex].mName != theName)
	{
		anIndex++;
		if (anIndex > NUM_CRAFT - 1)
		{
			FatalError(GT(0x52a468) + theName);
			return 0;
		}
	}
	return anIndex;
}

// 0x407540: the game that ended into its high-score list, under the scores it does not beat; the last line drops off.
// A finished campaign shows as level NUM_LEVELS_PLUS_ONE and one level more. Returns the line, NUM_HIGH_SCORES for none.
int HeavyWeaponApp::AddHighScore(bool isSurvival, bool hasFinished)
{
	unsigned int aRank = 0;
	if (!isSurvival)
	{
		while ((aRank < mHighScores.size()) && (mScore <= mHighScores[aRank].mScore))
			aRank++;
		if (aRank < mHighScores.size())
		{
			// port: the original leaves the record on its stack as it was, so the name's bytes after its end are whatever lay
			// there and go into hiscores.dat; the port clears the record, so the file is the same on every run
			HighScoreRecord aRecord;
			memset(&aRecord, 0, sizeof(aRecord));
			strcpy(aRecord.mName, mUsers[mCurrentUser].mName);
			aRecord.mLevel = mLevel;
			aRecord.mScore = mScore;
			if (!hasFinished)
				aRecord.mMaxLevel = mUsers[mCurrentUser].mMaxLevel;
			else
			{
				aRecord.mMaxLevel = NUM_LEVELS_PLUS_ONE;
				aRecord.mLevel++;
			}
			mHighScores.insert(mHighScores.begin() + aRank, aRecord);
			if (mHighScores.size() != 0)
				mHighScores.pop_back();
		}
	}
	else
	{
		while ((aRank < mSurvivalScores.size()) &&
			(mBoard->mLevelTime - mBoard->mTimePenalty <= mSurvivalScores[aRank].mTime))
			aRank++;
		if (aRank < mSurvivalScores.size())
		{
			SurvivalScoreRecord aRecord;	// port: cleared as the campaign's line is
			memset(&aRecord, 0, sizeof(aRecord));
			strcpy(aRecord.mName, mUsers[mCurrentUser].mName);
			aRecord.mTime = mBoard->mLevelTime - mBoard->mTimePenalty;
			mSurvivalScores.insert(mSurvivalScores.begin() + aRank, aRecord);
			if (mSurvivalScores.size() != 0)
				mSurvivalScores.pop_back();
		}
		// The player's best: the original compares it with the clock and keeps the time less the penalties, so a game
		// whose clock passed the best but whose time did not still replaces it, with a lower one
		if (mUsers[mCurrentUser].mBestSurvivalTime < mBoard->mLevelTime)
			mUsers[mCurrentUser].mBestSurvivalTime = mBoard->mLevelTime - mBoard->mTimePenalty;
	}
	SaveHighScores();
	return (int) aRank;
}

// The value of an attribute of an XML element as the original reads it (the attribute map's operator[], 0x4085b0)
static std::string Attribute(XMLElement& theElement, const char* theName)
{
	return theElement.mAttributes[theName];
}

// 0x4094e0: data\craft.xml into the craft table, from entry 1
void HeavyWeaponApp::LoadCraft()
{
	XMLParser* aParser = new XMLParser();
	aParser->OpenFile("data\\craft.xml");
	XMLElement anElement;
	int anIndex = 1;
	if (!aParser->HasFailed())
	{
		CraftInfo* aCraft = &mCraft[1];
		do
		{
			if (!aParser->NextElement(&anElement))
				break;
			if ((anElement.mType == XMLElement::TYPE_START) && (anElement.mValue == "Craft"))
			{
				aCraft->mName = Attribute(anElement, "name");
				aCraft->mDescription = Attribute(anElement, "desc");
				aCraft->mArms = Attribute(anElement, "arms");
				aCraft->mPoints = atol(Attribute(anElement, "points").c_str());
				aCraft->mArmor = (double) (int) atol(Attribute(anElement, "armor").c_str());
				aCraft->mIndex = anIndex;
				anIndex++;
				aCraft++;
			}
		} while (!aParser->HasFailed());
	}
	delete aParser;
}

// 0x40a8b0: data\levels.xml: each Level's name and length, and its Intel lines
void HeavyWeaponApp::LoadLevels()
{
	XMLParser* aParser = new XMLParser();
	aParser->OpenFile("data\\levels.xml");
	XMLElement anElement;
	LevelInfo aLevel;
	aLevel.mLength = 0;
	bool hasElement = aParser->NextElement(&anElement);
	while (hasElement)
	{
		if ((anElement.mType == XMLElement::TYPE_START) && (anElement.mValue == "Level"))
		{
			aLevel.mIntel.clear();
			aLevel.mName = Attribute(anElement, "name");
			aLevel.mLength = atol(Attribute(anElement, "length").c_str());
			hasElement = aParser->NextElement(&anElement);
			while (hasElement)
			{
				if (anElement.mType == XMLElement::TYPE_START)
				{
					if (anElement.mValue == "Intel")
						aLevel.mIntel.push_back(Attribute(anElement, "text"));
				}
				else if ((anElement.mType == XMLElement::TYPE_END) && (anElement.mValue == "Level"))
					break;
				hasElement = aParser->NextElement(&anElement);
			}
			mLevels.push_back(aLevel);
		}
		hasElement = aParser->NextElement(&anElement);
	}
	delete aParser;
}

// 0x40ba90: the waves of every level: data\waves.xml for the campaign, one of data\survival0..9.xml at random for a
// survival game
void HeavyWeaponApp::LoadWaves(bool isSurvival)
{
	mWaves.clear();
	XMLParser* aParser = new XMLParser();
	char aFileName[0x100];
	if (!isSurvival)
		strcpy(aFileName, "data\\waves.xml");
	else
		sprintf(aFileName, "data\\survival%i.xml", Rand() % 10);
	aParser->OpenFile(aFileName);
	XMLElement anElement;
	std::vector<WaveInfo> aLevelWaves;
	WaveInfo aWave;
	aWave.mLength = 0;
	bool hasElement = aParser->NextElement(&anElement);
	while (hasElement)
	{
		if ((anElement.mType == XMLElement::TYPE_START) && (anElement.mValue == "Level"))
		{
			hasElement = aParser->NextElement(&anElement);
			while (hasElement)
			{
				if (anElement.mType == XMLElement::TYPE_START)
				{
					if (anElement.mValue == "Wave")
					{
						aWave.mLength = atol(Attribute(anElement, "length").c_str());
						hasElement = aParser->NextElement(&anElement);
						while (hasElement)
						{
							if (anElement.mType == XMLElement::TYPE_START)
							{
								if (anElement.mValue == "Craft")
								{
									aWave.mCraft.push_back(GetCraftIndex(Attribute(anElement, "id")));
									aWave.mQuantities.push_back(atol(Attribute(anElement, "qty").c_str()));
								}
							}
							else if ((anElement.mType == XMLElement::TYPE_END) && (anElement.mValue == "Wave"))
								break;
							hasElement = aParser->NextElement(&anElement);
						}
						aLevelWaves.push_back(aWave);
						aWave.mCraft.clear();
						aWave.mQuantities.clear();
					}
				}
				else if ((anElement.mType == XMLElement::TYPE_END) && (anElement.mValue == "Level"))
					break;
				hasElement = aParser->NextElement(&anElement);
			}
			mWaves.push_back(aLevelWaves);
			aLevelWaves.clear();
		}
		hasElement = aParser->NextElement(&anElement);
	}
	delete aParser;
}

// 0x40c1d0: images\anims\anims.xml: each level's background animations and their strips. A strip that does not load
// ends the game with a message.
void HeavyWeaponApp::LoadAnims()
{
	XMLParser* aParser = new XMLParser();
	aParser->OpenFile("Images\\Anims\\anims.xml");
	XMLElement anElement;
	std::vector<BackgroundAnimInfo> aLevelAnims;
	std::vector<double> aDelays;
	bool hasElement = aParser->NextElement(&anElement);
	while (hasElement)
	{
		if ((anElement.mType == XMLElement::TYPE_START) && (anElement.mValue == "Level"))
		{
			hasElement = aParser->NextElement(&anElement);
			while (hasElement)
			{
				if (anElement.mType == XMLElement::TYPE_START)
				{
					if (anElement.mValue == "Anim")
					{
						BackgroundAnimInfo anAnim;
						double aSpeed = atof(Attribute(anElement, "speed").c_str());
						anAnim.mPlane = atol(Attribute(anElement, "plane").c_str()) - 1;
						anAnim.mOffset = (double) (int) atol(Attribute(anElement, "offset").c_str());
						anAnim.mY = atol(Attribute(anElement, "y").c_str());
						anAnim.mMoveX = atof(Attribute(anElement, "mx").c_str());
						anAnim.mSprite = new Sprite(this);
						int aFrameCount = atol(Attribute(anElement, "frames").c_str());
						if (!anAnim.mSprite->Load("Images\\Anims\\" + Attribute(anElement, "name"), aFrameCount, 1))
						{
							std::string anImageName = "Images\\Anims\\" + Attribute(anElement, "name");
							MsgBox(GT(0x52a6fc) + anImageName, GT(0x52a3dc), MB_OK);
							Shutdown();
							// port: the original leaves its parser and this level's list (and the strip) behind
							delete anAnim.mSprite;
							for (size_t i = 0; i < aLevelAnims.size(); i++)
								delete aLevelAnims[i].mSprite;
							delete aParser;
							return;
						}
						anAnim.mLoadFailed = false;
						anAnim.mHasNukedFrame = Attribute(anElement, "nuke") == "yes";
						anAnim.mRare = Attribute(anElement, "rare") == "yes";
						anAnim.mPlaysOnce = Attribute(anElement, "type") != "looping";
						for (int aFrame = aFrameCount; aFrame > 0; aFrame--)
							aDelays.push_back(aSpeed);

						hasElement = aParser->NextElement(&anElement);
						while (hasElement)
						{
							if (anElement.mType == XMLElement::TYPE_START)
							{
								if (anElement.mValue == "Delay")
								{
									std::string aDelaySpeed = Attribute(anElement, "speed");
									int aDelayFrame = atol(Attribute(anElement, "frame").c_str());
									aDelays[aDelayFrame] = atof(aDelaySpeed.c_str());
								}
							}
							else if ((anElement.mType == XMLElement::TYPE_END) && (anElement.mValue == "Anim"))
								break;
							hasElement = aParser->NextElement(&anElement);
						}
						anAnim.mFrameDelays = aDelays;
						aLevelAnims.push_back(anAnim);
						aDelays.clear();
					}
				}
				else if ((anElement.mType == XMLElement::TYPE_END) && (anElement.mValue == "Level"))
					break;
				hasElement = aParser->NextElement(&anElement);
			}
			mAnims.push_back(aLevelAnims);
			aLevelAnims.clear();
		}
		hasElement = aParser->NextElement(&anElement);
	}
	delete aParser;
}

// 0x40d020: a survival game: one of the ten wave files at random, no upgrades but the first gun level, and the survival
// backgrounds (they have no map)
void HeavyWeaponApp::StartSurvival()
{
	for (int i = 0; i < NUM_STATS; i++)
		mStats[i] = 0;
	mLives = SURVIVAL_LIVES;
	mScore = 0;
	LoadWaves(true);
	for (int i = 0; i < NUM_POWER_UPS; i++)
		mPowerUps[i] = 0;
	for (int i = 0; i < NUM_WEAPONS; i++)
		mWeapons[i] = 0;
	mPowerUps[POWERUP_NUKES] = 1;
	mBoard = new Board(this, true);
	mWidgetManager->AddWidget(mBoard);
	mWidgetManager->BringToBack(mBoard);
	mWidgetManager->SetFocus(mBoard);
	mBoard->MarkDirty();
	LoadLevelBackgrounds(this, "survival", false);
}

// 0x40e170: the campaign's waves, and the map of its first mission
void HeavyWeaponApp::StartCampaign()
{
	LoadWaves(false);
	mMapScreen = new MapScreen(this);
	mWidgetManager->AddWidget(mMapScreen);
	mWidgetManager->SetFocus(mMapScreen);
	mMapScreen->MarkDirty();
}

// 0x40e250: a new game from the first level, with no score, stats or upgrades but one nuke. A survival game
// marks every play tip as shown, so none comes up.
void HeavyWeaponApp::NewGame(bool isSurvival)
{
	mTipsShown = isSurvival ? 0xffff : 0;
	mScore = 0;
	mLevel = 0;
	for (int i = 0; i < NUM_STATS; i++)
		mStats[i] = 0;
	for (int i = 0; i < NUM_POWER_UPS; i++)
		mPowerUps[i] = 0;
	for (int i = 0; i < NUM_WEAPONS; i++)
		mWeapons[i] = 0;
	mPowerUps[POWERUP_NUKES] = 1;
	if (!isSurvival)
		StartCampaign();
	else
		StartSurvival();
}

// 0x40e2f0: the player's campaign so far: its score, stats and upgrades, at theLevel; the armory, or the map before the
// first level
void HeavyWeaponApp::ContinueGame(int theLevel)
{
	UserRecord& aUser = mUsers[mCurrentUser];
	mScore = aUser.mScore;
	for (int i = 0; i < NUM_STATS; i++)
		mStats[i] = aUser.mStats[i];
	mLevel = theLevel;
	mTipsShown = 0;
	for (int i = 0; i < NUM_POWER_UPS; i++)
		mPowerUps[i] = mUsers[mCurrentUser].mPowerUps[i];
	for (int i = 0; i < NUM_WEAPONS; i++)
		mWeapons[i] = mUsers[mCurrentUser].mWeapons[i];
	if (mLevel > 0)
		ShowArmory();
	else
		StartCampaign();
}

// 0x40e4f0 slot 5: the buttons of the dialogs, which report to the app
void HeavyWeaponApp::ButtonDepress(int theId)
{
	PlaySample(SOUND_BUTTONUP, 0);
	switch (theId)
	{
	case BUTTON_QUIT_CANCEL:
		KillDialog(DIALOG_QUIT);
		mBoard->mStatusBar->mQuitDialog = NULL;
		mWidgetManager->SetFocus(mBoard);
		break;
	case BUTTON_QUIT_TO_MENU:
		KillDialog(DIALOG_QUIT);
		mBoard->mStatusBar->mQuitDialog = NULL;
		ShowMainMenu();
		break;
	case BUTTON_OPTIONS:
		AddDialog(DIALOG_OPTIONS, new OptionsDialog(this));
		break;
	case BUTTON_CLOSE_USER_LIST:
		KillDialog(DIALOG_USER_LIST);
		break;
	case BUTTON_CLOSE_MESSAGE:
		KillDialog(DIALOG_MESSAGE);
		break;
	case BUTTON_HELP_PLAY:
		mWidgetManager->RemoveWidget(mHelpScreen);
		SafeDeleteWidget(mHelpScreen);
		mHelpScreen = NULL;
		NewGame(true);
		break;
	}
}

// port: the original's SexyApp makes a BASS music interface; the port's plays the MO3 module through libopenmpt
MusicInterface* HeavyWeaponApp::CreateMusicInterface()
{
	if (mNoSoundNeeded)
		return new MusicInterface;
	return new SDLMusicInterface();
}

// 0x40d2c0 slot 18
void HeavyWeaponApp::Init()
{
	SexyApp::Init();

	// port: the early-2005 SexyAppBase::Init (0x45c970) seeds Rand()'s generator with the 2004 MTRand seeding, which the
	// port's SRand does not (docs/FRAMEWORK_2004.md); -seed=N gives a test run its seed
	CrtSRand((unsigned int) mRandSeed);
	{
		MTRand aRand;
		SeedMTRand2004(aRand, (unsigned long) mRandSeed);
		uint32_t aState[MTRAND_N];
		int anIndex;
		aRand.GetState(aState, &anIndex);
		SetRandMTState(aState, anIndex);
	}

	if (!LoadGameTexts())
		SDL_Log("Heavy Weapon: no game texts read from the game's executable (texts without an override stay empty)");

	for (int i = 0; i < NUM_SOUNDS; i++)
		mSoundDelays[i] = 0;

	// The players
	Buffer* aBuffer = new Buffer();
	if (ReadBufferFromFile(USERS_FILE, aBuffer))
	{
		bool isOldFormat = false;
		unsigned char aVersion = aBuffer->ReadByte();
		if (aVersion != 0)
		{
			if (aVersion < 0x15)
			{
				// port: CopyFile then DeleteFile, through the user folder
				WriteBufferToFile("userdata\\users.bak", aBuffer);
				EraseFile(USERS_FILE);
				MsgBox(GT(0x52a8b8), GT(0x52a3dc), MB_OK);
				isOldFormat = true;
			}
			else
				aBuffer->SeekFront();
		}
		if (!isOldFormat)
		{
			for (unsigned int aRecord = 0; aRecord < (unsigned int) (aBuffer->GetDataLenBits() / 8) / sizeof(UserRecord);
				aRecord++)
			{
				UserRecord aUser;
				memset(&aUser, 0, sizeof(aUser));
				aBuffer->ReadBytes((uchar*) &aUser, sizeof(UserRecord));
				if (strlen(aUser.mName) == 0)
					break;
				mUsers.push_back(aUser);
			}
		}
	}

	// The high scores
	aBuffer->Clear();
	bool hasHighScores = false;
	if (ReadBufferFromFile(HIGH_SCORES_FILE, aBuffer))
	{
		if (aBuffer->GetDataLenBits() / 8 == HIGH_SCORES_FILE_SIZE)
		{
			for (int i = NUM_HIGH_SCORES; i != 0; i--)
			{
				HighScoreRecord aHighScore;
				SurvivalScoreRecord aSurvivalScore;
				aBuffer->ReadBytes((uchar*) &aHighScore, sizeof(HighScoreRecord));
				aBuffer->ReadBytes((uchar*) &aSurvivalScore, sizeof(SurvivalScoreRecord));
				mHighScores.push_back(aHighScore);
				mSurvivalScores.push_back(aSurvivalScore);
			}
			hasHighScores = true;
		}
		else
		{
			// The original deletes "userdata\hiscore.dat" (sic), so a file of the wrong size stays and the message comes
			// at every start
			EraseFile("userdata\\hiscore.dat");
			MsgBox(GT(0x52a858), GT(0x52a3dc), MB_OK);
		}
	}
	if (!hasHighScores)
	{
		for (int i = 0; i < NUM_HIGH_SCORES; i++)
		{
			HighScoreRecord aHighScore;
			memset(&aHighScore, 0, sizeof(aHighScore));
			strncpy(aHighScore.mName, GTC(DEFAULT_HIGH_SCORES[i].mNameText), sizeof(aHighScore.mName) - 1);
			aHighScore.mScore = DEFAULT_HIGH_SCORES[i].mScore;
			aHighScore.mLevel = DEFAULT_HIGH_SCORES[i].mLevel;
			aHighScore.mMaxLevel = DEFAULT_HIGH_SCORES[i].mLevel;
			SurvivalScoreRecord aSurvivalScore;
			memset(&aSurvivalScore, 0, sizeof(aSurvivalScore));
			strncpy(aSurvivalScore.mName, GTC(DEFAULT_SURVIVAL_SCORES[i].mNameText), sizeof(aSurvivalScore.mName) - 1);
			aSurvivalScore.mTime = DEFAULT_SURVIVAL_SCORES[i].mTime;
			mHighScores.push_back(aHighScore);
			mSurvivalScores.push_back(aSurvivalScore);
		}
	}
	delete aBuffer;

	for (int aFont = 0; aFont < NUM_FONTS; aFont++)
		mFonts[aFont] = new ImageFont(this, std::string("Fonts\\") + gFontNames[aFont] + ".txt");

	for (int aSong = 0; aSong < NUM_SONGS; aSong++)
		mMusicInterface->LoadMusic(aSong, std::string("music\\") + gSongInfo[aSong].mFile);

	for (int anImage = 0; anImage < NUM_INIT_IMAGES; anImage++)
	{
		mSprites[anImage] = new Sprite(this);
		mSprites[anImage]->Load(std::string("images\\") + gImageInfo[anImage].mFile, gImageInfo[anImage].mColumns,
			gImageInfo[anImage].mRows);
	}

	mSoundManager->LoadSound(SOUND_GUNPOWERUP, std::string("sounds\\") + gSoundInfo[SOUND_GUNPOWERUP].mFile);
	mSoundManager->SetBaseVolume(SOUND_GUNPOWERUP, 0.5);

	for (int aTarget = 1; aTarget <= NUM_TARGET_IMAGES; aTarget++)
		mTargetImages[aTarget - 1] = GetImage(StrFormat("images\\target%i", aTarget));

	if (!RegistryReadInteger("LastUser", &mCurrentUser))
	{
		RegistryWriteInteger("LastUser", 0);
		mCurrentUser = 0;
	}
	int aParticles;
	if (RegistryReadInteger("Particles", &aParticles))
		mParticles = (double) aParticles * 0.0001;
	else
	{
		RegistryWriteInteger("Particles", 10000);
		mParticles = 1.0;
	}

	mWaitForVSync = true;
	// Every game object moves in UpdateF, once a drawn frame (docs/FRAMEWORK_2005.md, "Updates")
	mVSyncUpdates = true;
	// The loading starts after the intro (TitleScreen)
	mAutoStartLoadingThread = false;

	SetCursorImage(CURSOR_POINTER, mSprites[IMAGE_CURSOR_POINTER]->mImage);
	SetCursorImage(CURSOR_HAND, mSprites[IMAGE_CURSOR_HAND]->mImage);
	SetCursorImage(CURSOR_DRAGGING, mSprites[IMAGE_CURSOR_DRAGGING]->mImage);
	SetCursorImage(CURSOR_TEXT, mSprites[IMAGE_CURSOR_TEXT]->mImage);

	LoadCraft();
	LoadLevels();
	LoadAnims();

	mTitleScreen = new TitleScreen(this);
	mWidgetManager->AddWidget(mTitleScreen);
	mWidgetManager->SetFocus(mTitleScreen);
	mTitleScreen->MarkDirty();

	// port: WinMain (0x40e210) seeds the app's own generator from Rand() after Init
	SeedMTRand2004(mRand, (unsigned long) (unsigned int) Rand());
}

int Sexy::KillPercentage(int theDestroyed, int theSpawned)
{
	double aPercentage = (double)theDestroyed / (double)theSpawned * 100.0;
	if (!(aPercentage > (double)INT_MIN - 1.0 && aPercentage < (double)INT_MAX + 1.0))
		return INT_MIN;
	return (int)aPercentage;
}
