// Board: the constructor (0x480020), the destructor (body 0x477b90, the scalar deleting wrapper 0x47a910) and the hook
// that the event bus calls before it delivers an event (0x4328d0).

#include "Board.h"
#include "AstroButton.h"
#include "AstroPopApp.h"
#include "Brick.h"
#include "Checkbox.h"
#include "DDImage.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "Particle.h"
#include "Res.h"
#include "ResourceManager.h"
#include "SoundInstance.h"
#include "StoryScreen.h"
#include "UserProfile.h"

using namespace Sexy;

// 0x480020: the field, the scenery, the hint boxes and the buttons are made; the level the player starts at is set up
// (the rules and the stages up to it are played through by AdvanceLevel) and loaded. theFlags holds the byte at +0x1968
// (when it is not 0 and the game is not the demo, the level is loaded); theFlag is kept at +0x922; theNewGame is true
// when the game starts from the first level of a player who has not played (it marks the profile's flag 0x71);
// theDemo plays the demo's script.
//
// Port: the original's group release of the resources of the three selector screens (PlanetSelector, GameSelector,
// CharSelector, 0x48f8a0) is left out (only the memory differs). The theme checkbox gets no listener yet (the
// application's CheckboxListener part follows with the dialogs).
Board::Board(AstroPopApp* theApp, EventBus* theBus, int theGameType, unsigned int theFlags, int theLevel, bool theFlag,
			 bool theNewGame, bool theDemo)
	: EventListener(theBus)
{
	mFieldsCleared = 0xffff0000;
	mBonusRoundPoints = 0xffff0000;
	mBestChain = 0xffff0000;
	mBricksGoneCount = 0xffff0000;
	mPlayPoints = 0xffff0000;
	mLevelUpCursorInterval = 0x10000;
	mLevelUpScanInterval = 0x10000;
	mLevelUpScanMaxHeight = 0x10000;
	mLevelUpScanHeight = 0x10000;
	mLevelUpScanY = 0x10000;
	mLevelUpPanelHeight = 0x10000;
	mLevelUpPanelY = 0x10000;
	mLevelUpLetterIndex = 0x10000;
	mLevelUpLineIndex = 0x10000;
	mLevelUpTypeInterval = 0x10000;
	mLevelUpCursorShown = true;
	mWeaponHintPulse.mA = -1;
	mWeaponHintPulse.mB = -1;
	mWeaponHintPulse.mDeltaA = -1;
	mWeaponHintPulse.mDeltaB = -1;
	mWeaponHintPulse.mWait = -1;
	mWeaponHintPulse.mWaitReset = -1;

	mApp = theApp;
	m1a60 = theGameType;
	if (theGameType == 1)
		mMode = theApp->mClassicPilot;
	else
		mMode = theApp->mSurvivalPilot;

	Listen(5);
	Listen(6);
	Listen(7);
	Listen(8);
	Listen(0xc);
	Listen(0xb);
	Listen(0xf);
	Listen(0x10);
	Listen(0x11);
	Listen(0x12);
	Listen(0x13);
	Listen(0xd);
	Listen(0xe);
	Listen(0x1c);
	Listen(0x1a);
	Listen(0x1b);
	Listen(0x1f);
	Listen(0x26);
	Listen(0x29);
	Listen(0x27);
	// the hook at +0x60 is OnFiring
	mWelcomePlayed = false;

	// the hint boxes: the SUPA weapon, the meter, a third that the pause fills
	mHintBoxes.resize(3, (HintBox*) NULL);
	mHintBoxes[0] = new HintBox();
	mHintBoxes[0]->AddLine(GT(0x00569b80));
	mHintBoxes[0]->AddLine(GT(0x00569b60));
	mHintBoxes[0]->AddLine(GT(0x00569b40));
	m978 = 0;
	mDemoStartLink = NULL;
	mBonusFieldLoaded = false;
	m968 = 0xa6;
	m96c = 4;
	mHintBoxes[1] = new HintBox();
	std::string aMeterLine;
	if (m1a60 == 1)
	{
		mHintBoxes[1]->AddLine(GT(0x00569b20));
		aMeterLine = GT(0x00569b0c);
	}
	else
	{
		mHintBoxes[1]->AddLine(GT(0x00569af4));
		aMeterLine = GT(0x00569adc);
	}
	mHintBoxes[1]->AddLine(aMeterLine);
	mHintBoxes[2] = new HintBox();
	mFadingFromWhite = false;
	mLevelUpText = NULL;
	mRecordingDemo = false;
	mDemoInputActive = false;
	mDemoPlaying = false;
	m95c = 0;
	m958 = 0;
	m948 = 0;
	m949 = 0;
	m94c = 1;
	m950 = 1;
	mFirstHelpPending = mApp->mShowHelpAtStart;
	for (size_t i = 0; i < mHintBoxes.size(); i++)
	{
		mHintBoxes[i]->SetShown(false);
		mHintBoxes[i]->SetFont(GetFontById(0xb0));
		mHintBoxes[i]->mShowTime = 100;
		mHintBoxes[i]->mHideTime = 800;
	}
	mHintBoxes[2]->mShowTime = 100;
	mStartedFromCheckpoint = theFlag;
	m960 = 0;
	mStartWeaponAfterPause = false;
	m1b6c = 0;
	if (m1a60 == 1)
		m1bbc = 5;
	else
		m1bbc = (m1a60 != 0) ? 500000000 : 3;
	m1968 = (int) (theFlags & 0xff);
	m1940 = theLevel;
	mField193C = 1;
	m1c09 = false;
	m1b48 = -IMAGE_SUPA_BUTTON_ALERT->mWidth;
	mContinueButton = NULL;
	mMenuButton = NULL;
	m1be4 = 0;
	mBonusRoundStarted = false;
	m1b30 = -1;
	m1bb8 = 0;
	mStartBannerDoneInBonus = false;
	mBonusRoundOn = false;
	mClicksBlocked = false;
	m1b1c = 0;
	m1b0c = 0;
	m1b08 = 0;
	m1af4 = 0;
	m1b14 = 0;
	if (theLevel == 1 && theNewGame)
	{
		mStageTextEnabled = false;
		mApp->mUser->SetInt(0x71, 0);
	}
	else
	{
		mStageTextEnabled = true;
		m1b1c = theLevel - 1;
	}
	mStoryScreen = 0;
	mRightHeadImage = IMAGE_UPGRADE_HEAD;
	mRightArmImage = IMAGE_UPGRADE_ARM;
	mLeftHeadImage = mApp->CopyImage(IMAGE_UPGRADE_HEAD);
	mApp->MirrorImage(mLeftHeadImage);
	mLeftArmImage = mApp->CopyImage(IMAGE_UPGRADE_ARM);
	mApp->MirrorImage(mLeftArmImage);
	SetupUpgradeAnims();
	m1b28 = 0;
	m1b2c = 0;
	mUnusedClearedInCtor = false;
	mBannerWaveImage = NULL;
	mBannerTextImage = NULL;
	mBrickometerFill = 0.0f;
	mBrickometerDrain = 0.0f;
	mBricksRevealing = false;
	mBonusRoundFile = 0;
	m1abc = 0;
	m970 = 0;
	mUnusedFlagFromCtor = false;
	m1ab8 = 4;
	m1ac0 = PickSpawnDelay();
	mBonusRoundLoaded = false;
	mBonusFadingOut = false;
	m1b74 = 0;
	m1b7c = 0;
	m1b40 = 0;
	m1b50 = 0;
	mWeaponHintShown = false;
	int aHalfBoost = 1;
	mApp->mUser->GetInt(mMode + 0x2a, &aHalfBoost);
	m1c08 = (aHalfBoost == 1);
	m1b88 = 0;
	m1b8c = 0;
	m1b94 = 0;
	m1b98 = 0;
	m1b9c = 0;
	m1ba0 = 0;
	m1ba4 = 0;
	m1ba8 = 0;
	m1bb0 = 0;
	m1bac = 0;
	m1c00 = 0.0f;
	m1bc0 = 0;
	m1bc4 = 0;
	m1bc8 = 0;
	m1bd0 = 0;
	m1bcc = 0;
	mBonusRevealDone = false;

	// the theme checkbox and button of the warp screen (their pictures are made when the stage's colour is known)
	mHintOffCheckbox = new Checkbox(NULL, NULL, 0xfa3, NULL);
	mHintOffCheckbox->mUncheckedRect = Rect(0, 0, IMAGE_TH_CHECKBOX->mWidth / 2, IMAGE_TH_CHECKBOX->mHeight);
	mHintOffCheckbox->mCheckedRect = Rect(IMAGE_TH_CHECKBOX->mWidth / 2, 0, IMAGE_TH_CHECKBOX->mWidth / 2, IMAGE_TH_CHECKBOX->mHeight);
	int aThemeY = (IMAGE_TH_STRIP->mHeight - IMAGE_TH_CHECKBOX->mHeight) + 0xae;
	mHintOffCheckbox->Resize(0xa3, aThemeY, IMAGE_TH_CHECKBOX->mWidth / 2, IMAGE_TH_CHECKBOX->mHeight);
	mHintOkButton = new AstroButton(0xfa2, mApp);
	mHintOkButton->mOverImage = NULL;
	mHintOkButton->mDownImage = NULL;
	mHintOkButton->mButtonImage = NULL;
	mHintOkButton->mLabel = GT(0x00562024);
	mHintOkButton->SetFont(GetFontById(0xb1));
	mHintOkButton->SetColor(0, Color(0xff, 0xff, 0xff));
	mHintOkButton->SetColor(1, Color(0xff, 0xff, 0xff));
	mHintOkButton->Resize(0x208, aThemeY - 5, IMAGE_TH_BTN_OVER->mWidth, IMAGE_TH_BTN_OVER->mHeight);
	m1b58 = 0xf;
	m1b4c = 0;
	m1b34 = 0;
	m1b20 = 0;
	mEndPageShown = false;
	m1b84 = 0;
	m1b80 = 0;
	m944 = 0.0f;
	mClearCountRunning = false;
	m1b70 = -1;
	m1b0c = 500000000;
	m1afc = 0;
	m1af8 = 0;
	mPhasePictures.resize(14, (Image*) NULL);
	m1ac4 = 6;
	mDropTicks = 1000;
	m1af0 = 0;
	m1af4 = 0;
	mWeaponHintText = GT(0x00569a90);
	int aModeImage = 0x196;
	if (mMode == 1)
		aModeImage = 0x1a4;
	else if (mMode == 2)
		aModeImage = 0x1b2;
	else if (mMode == 3)
		aModeImage = 0x1c0;
	for (size_t i = 0; i < mPhasePictures.size(); i++)
		mPhasePictures[i] = LoadImageById(mApp->mResourceManager, aModeImage + (int) i);
	mClickRepeating = false;
	m1aa0 = 0;
	mPlanets.resize(5);
	mStreaks.resize(100);
	mSpawnRules.resize(1);
	mBackdropStars.resize(3);
	mFieldButtons.assign(1, (Widget*) NULL);
	m1b00 = 0;
	m19b8 = 0;
	m1b78 = 0;
	m1ab0 = 0;
	mMeterPaused = false;
	mInHallOfFame = false;
	m1a24 = 0;
	mTutorialStageForced = false;

	mMeterFullAnim.SetNumFrames(0x3f);
	mMeterFullAnim.SetDelay(1);
	mMeterFullAnim.SetStopped(true);
	mMeterFullAnim.mPingPong = true;
	mLevelUpPanelHeight = 0xce;
	mLevelUpPanelY = -206;
	mStageMeterAnim.SetNumFrames(0x3f);
	mStageMeterAnim.SetDelay(1);
	mStageMeterAnim.mMaxLoops = 10;
	mStageMeterAnim.mLoopCount = 0;
	mStageMeterAnim.mPingPong = true;
	mStageMeterAnim.SetStopped(true);
	m1acc = 0;
	m1adc = 0;
	m1ad8 = 0;
	m1ae8 = 0;
	m1ae4 = 0;
	m1ae0 = 0;
	m1ad4 = 0;
	m1ad0 = 0;
	m1aec = 0;
	mApp->mUser->GetInt(mMode + 0x33, &m1a8c);
	mApp->mUser->GetInt(mMode + 0x3b, &m1a94);
	if (mMode == 0)
		mApp->mUser->GetInt(0x37, &m1a90);
	else if (mMode == 3)
		mApp->mUser->GetInt(0x3a, &m1a90);
	else
		m1a90 = -1;
	m974 = 0;
	mOverlay.Resize(0, 0, 0x280, 0x1e0);
	mOverlay.mBoard = this;
	mOverlay.mHasTransparencies = true;
	mOverlay.mHasAlpha = true;
	m19d4 = 8;
	mFieldBricks.resize(20);
	for (size_t i = 0; i < mFieldBricks.size(); i++)
		mFieldBricks[i].resize(10, (Brick*) NULL);
	m1a54 = 4;
	mPauseMutedSound = NULL;
	mLevelCompleteBlocked = false;
	mWarpGateAnim.SetNumFrames(1);
	mWarpGateAnim.SetDelay(0xc);
	mWarpGateAnim.mHoldLast = false;
	mWarpGateAnim.mPingPong = false;
	if (mMode == 2)
	{
		mTurbotPowerupAnim.SetNumFrames(IMAGE_TURBOT_POWERUP->mNumCols);
		mTurbotPowerupAnim.SetDelay(6);
		mTurbotPowerupAnim.mPingPong = false;
		mTurbotPowerupAnim.SetStopped(true);
		mTurbotPowerupAnim.mHoldLast = false;
	}
	else
		mTurbotPowerupAnim.SetStopped(true);
	m1a28 = 0;
	m197c = 10000;
	m1970 = 0;
	mScore = 0;
	mSlowMotion = false;
	m1bd8 = gTestChargeAtReset < 0.0f ? 0.0f : gTestChargeAtReset;   // (a test's charge, BoardState.cpp)
	ResetGame();
	if (m1a60 == 1)
		SetupLevel1Rules();
	else if (m1a60 != 0)
		SetupClassicRules();
	mHandKindBrick = NULL;
	m198c = 0xa6;
	mHeldBricks.clear();

	// the script rows are cleared and so are the first rows of the field
	for (int aRow = 0; aRow < 100; aRow++)
	{
		for (int aCol = 0; aCol < 10; aCol++)
		{
			m984.mCode[aRow][aCol] = 0;
			if (aRow < 20)
				mFieldBricks[aRow][aCol] = NULL;
		}
	}

	// the 100 streaks of the backdrop: a kind, a speed, a place (the random numbers in this order)
	for (size_t i = 0; i < 100; i++)
	{
		StreakRec& aStreak = mStreaks[i];
		int aKind = ((int) AstroRand() % 500000000) % 3;
		if (aKind == 0)
		{
			aStreak.mColor = Color(0x50, 0x50, 0x50);
			int aRand = ((int) AstroRand() % 500000000) % 10;
			float aSpeed = (float) aRand * 0.01f + 0.25f;
			aStreak.mMinSpeed = aSpeed;
			aStreak.mSpeed = aSpeed;
			aStreak.mImage = mApp->mGradient[0];
		}
		else if (aKind == 1)
		{
			aStreak.mColor = Color(0x96, 0x96, 0x96);
			int aRand = ((int) AstroRand() % 500000000) % 10;
			float aSpeed = (float) aRand * 0.01f + 0.5f;
			aStreak.mMinSpeed = aSpeed;
			aStreak.mSpeed = aSpeed;
			aStreak.mImage = mApp->mGradient[1];
		}
		else
		{
			aStreak.mColor = Color(0xfa, 0xfa, 0xfa);
			int aRand = ((int) AstroRand() % 500000000) % 10;
			float aSpeed = (float) aRand * 0.01f + 1.0f;
			aStreak.mMinSpeed = aSpeed;
			aStreak.mSpeed = aSpeed;
			aStreak.mImage = mApp->mGradient[2];
		}
		aStreak.mKind = aKind;
		aStreak.mX = (float) (((int) AstroRand() % 500000000) % mApp->mWidth);
		aStreak.mY = (float) (((int) AstroRand() % 500000000) % (mApp->mHeight - 4) + 4);
	}

	mSupaButtonLit = false;
	m1930 = 0;
	SetupHints();
	if (1 < theLevel * 2 - 2)
	{
		int aCount = theLevel * 2 - 3;
		do
		{
			mField193C = mField193C + 1;
			AdvanceLevel(false);
			aCount--;
		} while (aCount != 0);
	}
	MarkHintsSeen(theLevel);
	if ((theFlags & 0xff) != 0 && !theDemo)
		LoadLevel(true);
	m1954 = 2;
	mStreaksFadingOut = false;

	// the scenery layer; the pictures of the three ids around it are released (the layer's own stays)
	mSceneryLayer.mImage = LoadImageById(mApp->mResourceManager, 0x18c);
	for (int anId = 0x18b; anId < 0x18e; anId++)
	{
		if (anId != 0x18c)
			mApp->mResourceManager->DeleteImage(GetStringIdById(anId));
	}
	mSceneryLayer.mI30 = 0x18c;
	mSceneryLayer.mF24 = 0.0f;
	mSceneryLayer.mF28 = -1.0f;
	mSceneryLayer.mCur[0] = 0.0f;
	mSceneryLayer.mCur[1] = (float) (mSceneryLayer.mImage->mHeight - 0x1e0);
	mSceneryLayer.mCur[2] = 640.0f;
	mSceneryLayer.mCur[3] = 480.0f;
	mSceneryLayer.mNext[0] = 0.0f;
	mSceneryLayer.mNext[1] = 0.0f;
	mSceneryLayer.mNext[2] = 640.0f;
	mSceneryLayer.mNext[3] = 0.0f;
	mSceneryLayer.mPeriod = 1;
	mSceneryLayer.mSpeed = 0.02f;
	m938 = 500000000.0f;
	m93c = 500000000.0f;
	m1948 = 0;
	m19b0 = 500000000;
	m19ac = 500000000;
	m19a8 = 500000000;
	m19a4 = 500000000;
	m19a0 = 500000000;
	m199c = 500000000;
	m19ec = 500000000;
	m19e8 = 500000000;
	m19e4 = 500000000;
	m19e0 = 500000000;
	m19f8 = 500000000;
	m19f4 = 500000000;
	m19f0 = 500000000;
	mPauseTextWanted = true;
	mFieldButtons[0] = NULL;
	mHelpButton = NULL;
	m1a58 = 0;
	m1a5c = 0;
	m1a34 = 0;
	m1bdc = 18000.0f;
	ApplyWeaponLevels();
	m1bd4 = 0;
	mRankImage = mApp->MakeRankImage(0, 0x50);

	// the text that scrolls along the top while the bonus of state 1 runs, repeated to fill the width
	std::string aBannerText;
	if (mMode == 0)
		aBannerText = GT(0x00569a80);
	else if (mMode == 1 || mMode == 2)
		aBannerText = GT(0x00569a54);
	else
		aBannerText = GT(0x00569a64);
	Font* aBannerFont = GetFontById(0xb0);
	bool aBannerDone = false;
	int aBannerX = -0x1e - aBannerFont->StringWidth(aBannerText);
	do
	{
		if (aBannerX >= aBannerFont->StringWidth(aBannerText) + 0x29e)
			aBannerDone = true;
		DrawnText aText;
		aText.mX = aBannerX;
		aText.mY = 0x1cc;
		aText.mText = aBannerText;
		aText.mFont = aBannerFont;
		mBonusBannerTexts.push_back(aText);
		aBannerX = aBannerX + 0x1e + aBannerFont->StringWidth(aBannerText);
	} while (!aBannerDone);
	m1b04 = 0;
	m1a00 = mField193C + 1;
	m1a04 = mField193C + 2;
	int aHalf = m1a04 / 2;
	int aRemainder = (aHalf + 1) % 4;
	if (aRemainder == 0)
		m1a08 = -1;
	else
		m1a08 = (aHalf - aRemainder) * 2 + 8;
	m1a0c = -1;
	mApp->mUser->GetInt(mMode + 8, &m1a0c);
	if (m1a0c * 2 - 2 < m1a08)
		m1a08 = -1;
	if (m1a08 < 1)
	{
		m1a70 = 0x17d - IMAGE_SLOW_WARP_GATE->GetCelWidth() / 2;
		m1a74 = -1;
	}
	else
	{
		m1a70 = 0xb6;
		m1a74 = 0x192;
	}

	// the 40 blinking pictures of the backdrop
	for (int i = 0; i < 0x28; i++)
	{
		BlinkRec aBlink;
		aBlink.mX = ((int) AstroRand() % 500000000) %
			(IMAGE_BEAM_RADIAL->mWidth - IMAGE_PARTICLE_RADIAL->mWidth / IMAGE_PARTICLE_RADIAL->mNumCols);
		aBlink.mCounter = ((int) AstroRand() % 500000000) % 400;
		aBlink.mAnim.SetNumFrames(IMAGE_PARTICLE_RADIAL->mNumCols);
		aBlink.mAnim.SetDelay(5);
		aBlink.mAnim.SetFrame(((int) AstroRand() % 500000000) % aBlink.mAnim.mNumFrames);
		aBlink.mAnim.mHoldLast = false;
		aBlink.mAnim.mPingPong = false;
		aBlink.mAnim.SetStopped(false);
		mBeamSparks.push_back(aBlink);
	}
	mApp->PlaySong(9);
	mWipeImageUsed = false;
	mWipeImageCaptured = false;
	mScreenWiping = false;
	m980 = -1;

	// the picture of the field that the wipe at the end of a game takes away, and the wipe
	DDImage* aFieldImage = new DDImage(mApp->mDDInterface);
	mScreenImage = aFieldImage;
	aFieldImage->Create(0x280, 0x1e0);
	aFieldImage->SetVolatile(true);
	mScreenGraphics = new Graphics(mScreenImage);
	mScreenWipe = new ScreenWipe();
	m980 = ((int) AstroRand() % 500000000) % 4;
	if (m980 == 0)
		m1a84 = 2000;
	else
		m1a84 = (m980 != 3) * 2 + 4;
	mScreenWipe->SetImage(mScreenImage, false);
	mScreenWipe->SetKind(m980);
	m1a78 = 0;
	mWipeCreated = true;
	m1a7c = -1;
	mSirenAnim.SetNumFrames(IMAGE_SIREN->mNumCols);
	mSirenAnim.SetDelay(8);
	mSirenAnim.mHoldLast = false;

	// the siren's two pictures, flipped copies that are drawn into memory images
	mSirenImage = new MemoryImage(mApp);
	mSirenFxImage = new MemoryImage(mApp);
	mSirenImage->Create(IMAGE_SIREN->mWidth, IMAGE_SIREN->mHeight);
	mSirenFxImage->Create(IMAGE_SIREN_FX->mWidth, IMAGE_SIREN_FX->mHeight);
	Graphics aSirenGraphics(mSirenImage);
	aSirenGraphics.DrawImage(IMAGE_SIREN, 0, 0);
	Graphics aSirenFxGraphics(mSirenFxImage);
	aSirenFxGraphics.DrawImage(IMAGE_SIREN_FX, 0, 0);
	mSirenImage->Palletize();
	mSirenFxImage->Palletize();
	mApp->MirrorImage(mSirenImage);
	mApp->MirrorImage(mSirenFxImage);
	mSirenImage->mNumCols = IMAGE_SIREN->mNumCols;
	mSirenFxImage->mNumCols = IMAGE_SIREN_FX->mNumCols;
	for (int i = 0; i < 3; i++)
	{
		FadeRec aFade;
		aFade.mValue = 0;
		aFade.mStep = 8;
		aFade.mRising = true;
		mEndBlastFades.push_back(aFade);
	}

	// the 40 sparks of the sun, spread by whole steps of nine degrees
	int anAngle = 0;
	for (int i = 0; i < 0x28; i++)
	{
		SunParticle* aSpark = new SunParticle();
		aSpark->mX = 357.0f;
		aSpark->mY = 215.0f;
		aSpark->mAnim.SetDelay(7);
		aSpark->mAnim.SetNumFrames(IMAGE_SUN_PARTICLE->mNumCols);
		aSpark->mAnim.mHoldLast = false;
		aSpark->mAnim.SetStopped(true);
		aSpark->mVX = gCosTable[anAngle] * 4.0f;
		aSpark->mVY = gSinTable[anAngle] * -4.0f;
		mSunSparks.push_back(aSpark);
		anAngle = (int) ((double) anAngle + 9.0);
	}
	mDemoEnding = false;
	if (theDemo)
	{
		mDemoPlaying = true;
		LoadDemo();
		BuildField();
	}
}

// 0x477b90 (the scalar deleting wrapper at 0x47a910). What the original does not delete: the pictures of the
// resources, the bricks of the lists (ClearField does that), the members it clears and frees in place. The members at
// +0x2c0 (the stories: a vector of pairs of pages) it deletes after the klaxon, and +0x754 (the story screen) before the
// buttons of the end of a game.
Board::~Board()
{
	if (mPauseMutedSound != NULL)
		mPauseMutedSound->Release();

	for (size_t i = 0; i < mEnergySparks.size(); i++)
		delete mEnergySparks[i];
	mEnergySparks.clear();

	if (mApp->mLoopSounds.size() > 7)
	{
		SoundInstance* aKlaxon = mApp->mLoopSounds[7];
		if (aKlaxon->IsPlaying())
			aKlaxon->Stop();
	}

	for (size_t i = 0; i < mPilotStories.size(); i++)
	{
		delete mPilotStories[i]->mPageA;
		delete mPilotStories[i]->mPageB;
		delete mPilotStories[i];
	}
	mPilotStories.clear();

	if (mLevelUpText != NULL)
	{
		if (m1a60 == 2)
		{
			delete mLevelUpText->mImage;
			mLevelUpText->mImage = NULL;
		}
		delete mLevelUpText;
	}
	delete mDemoStartLink;
	delete mBonusRoundFile;
	delete mBannerWaveImage;
	delete mBannerTextImage;

	if (mHintOkButton != NULL)
	{
		delete mHintOkButton->mOverImage;
		delete mHintOkButton->mDownImage;
		delete mHintOkButton->mButtonImage;
	}
	if (mHintOffCheckbox != NULL)
		delete mHintOffCheckbox->mUncheckedImage;
	delete mHintOkButton;
	delete mHintOffCheckbox;

	for (size_t i = 0; i < mHintSpots.size(); i++)
		delete mHintSpots[i];
	for (size_t i = 0; i < mHintBoxes.size(); i++)
		delete mHintBoxes[i];

	// the two buttons of the end of a game keep their pictures
	if (mMenuButton != NULL)
	{
		ButtonWidget* aButton = static_cast<ButtonWidget*>(mMenuButton);
		delete aButton->mButtonImage;
		delete aButton->mOverImage;
		delete aButton->mDownImage;
	}
	if (mContinueButton != NULL)
	{
		ButtonWidget* aButton = static_cast<ButtonWidget*>(mContinueButton);
		delete aButton->mButtonImage;
		delete aButton->mOverImage;
		delete aButton->mDownImage;
	}
	delete mStoryScreen;
	delete mMenuButton;
	delete mContinueButton;

	// the panels of the hints; the first one's chain of two more is deleted with it
	for (size_t i = 0; i < mHintPanels.size(); i++)
	{
		if (i == 0 && mHintPanels[0]->mNext != NULL)
		{
			delete mHintPanels[0]->mNext->mNext;
			delete mHintPanels[0]->mNext;
		}
		delete mHintPanels[i];
	}

	ClearField();

	if (!mFieldButtons.empty() && mFieldButtons[0] != NULL)
	{
		AstroButton* aMainButton = static_cast<AstroButton*>(mFieldButtons[0]);
		delete aMainButton->mButtonImage;
		delete aMainButton;
	}
	delete mHelpButton;

	for (size_t i = 0; i < mFloatTexts.size(); i++)
		delete mFloatTexts[i];
	for (size_t i = 0; i < mWarpStars.size(); i++)
		delete mWarpStars[i];

	StopListeningToAll();

	delete mScreenWipe;
	delete mScreenImage;
	delete mScreenGraphics;

	for (size_t i = 0; i < mSunSparks.size(); i++)
		delete mSunSparks[i];
	delete mSirenImage;
	delete mSirenFxImage;
	delete mHandKindBrick;

	if (mRightArmImagesOwned)
	{
		delete mRightArmImage;
		delete mRightHeadImage;
	}
	if (mLeftArmImagesOwned)
	{
		delete mLeftArmImage;
		delete mLeftHeadImage;
	}
}

// 0x4328d0: the hook that the event bus calls for the Board (as a source) before it delivers an event: the events of
// the play clear the flag that SaveGameState sets
void Board::OnFiring(int theEvent)
{
	switch (theEvent)
	{
	case 1:
	case 2:
	case 4:
	case 9:
	case 10:
	case 0xc:
	case 0xd:
	case 0xf:
	case 0x10:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x1f:
	case 0x26:
	case 0x27:
	case 0x29:
		gFlag5A5100 = false;
		break;
	}
}
