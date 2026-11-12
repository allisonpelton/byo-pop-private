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
	mGameType = theGameType;
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
	mWhiteFadeAlpha = 0;
	mDemoStartLink = NULL;
	mBonusFieldLoaded = false;
	mFieldPointerX = 0xa6;
	mFieldPointerY = 4;
	mHintBoxes[1] = new HintBox();
	std::string aMeterLine;
	if (mGameType == 1)
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
	mDemoWaitLeft = 0;
	mDemoAddedRowIndex = 0;
	mDemoRecordAction = 0;
	mDemoRecordLastAction = 0;
	mDemoRecordArg = 1;
	mDemoRecordLastArg = 1;
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
	mClearBonusWorth = 0;
	mStartWeaponAfterPause = false;
	mCheckpointLevel = 0;
	if (mGameType == 1)
		mHardStage = 5;
	else
		mHardStage = (mGameType != 0) ? 500000000 : 3;
	mLevelNumber = theLevel;
	mStageNumber = 1;
	mKeepLoopSoundsMuted = false;
	mSupaButtonX = -IMAGE_SUPA_BUTTON_ALERT->mWidth;
	mContinueButton = NULL;
	mMenuButton = NULL;
	mWeaponChargeStep = 0;
	mBonusRoundStarted = false;
	mNewRecordRow = -1;
	mTallySoundCounter = 0;
	mStartBannerDoneInBonus = false;
	mBonusRoundOn = false;
	mClicksBlocked = false;
	mLevelsCompleted = 0;
	mExtraRowInterval = 0;
	mExtraRowTimer = 0;
	mResumeUpdate = 0;
	mWarpOverlayCel = 0;
	if (theLevel == 1 && theNewGame)
	{
		mStageTextEnabled = false;
		mApp->mUser->SetInt(0x71, 0);
	}
	else
	{
		mStageTextEnabled = true;
		mLevelsCompleted = theLevel - 1;
	}
	mStoryScreen = 0;
	mRightHeadImage = IMAGE_UPGRADE_HEAD;
	mRightArmImage = IMAGE_UPGRADE_ARM;
	mLeftHeadImage = mApp->CopyImage(IMAGE_UPGRADE_HEAD);
	mApp->MirrorImage(mLeftHeadImage);
	mLeftArmImage = mApp->CopyImage(IMAGE_UPGRADE_ARM);
	mApp->MirrorImage(mLeftArmImage);
	SetupUpgradeAnims();
	mNextLevelScreenKind = 0;
	mLevelEndScreenKind = 0;
	mBannerWaveImage = NULL;
	mBannerTextImage = NULL;
	mBrickometerFill = 0.0f;
	mBrickometerDrain = 0.0f;
	mBricksRevealing = false;
	mBonusRoundFile = 0;
	mFlurryTimer = 0;
	mHighestStack = 0;
	mPhase = 4;
	mPhaseDelay = PickSpawnDelay();
	mBonusFadingOut = false;
	mBonusFadeAlpha = 0;
	mBannerTimeLeft = 0;
	mSurvivalBonusTimer = 0;
	mTurbotOverlayCel = 0;
	mWeaponHintShown = false;
	int aHalfBoost = 1;
	mApp->mUser->GetInt(mMode + 0x2a, &aHalfBoost);
	mWeaponDialogDue = (aHalfBoost == 1);
	mTallyBarWidth = 0;
	mTallyTitleAlpha = 0;
	mTallyBricksLeft = 0;
	mTallyBricksCounted = 0;
	mTallyPenalty = 0;
	mTallyPenaltyShown = 0;
	mTallyCleanBonus = 0;
	mTallyCleanBonusShown = 0;
	mTallyState = 0;
	mTallyPaid = 0;
	mBonusWarpSlowdown = 0.0f;
	mBonusWarpPhase = 0;
	mBonusWarpHold = 0;
	mBonusWarpWhiteAlpha = 0;
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
	mTurbotBeamInterval = 0xf;
	mTurbotPhase = 0;
	mGameOverCounter = 0;
	mBoosterFlameY = 0;
	mEndPageShown = false;
	mBannerTimeShown = 0;
	mBannerDuration = 0;
	mBannerWavePhase = 0.0f;
	mClearCountRunning = false;
	mBonusTimeLeft = -1;
	mExtraRowInterval = 500000000;
	mComboTimeLeft = 0;
	mComboColor = 0;
	mPhasePictures.resize(14, (Image*) NULL);
	mMaxMutators = 6;
	mDropTicks = 1000;
	mPauseStartUpdate = 0;
	mResumeUpdate = 0;
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
	mTurbotPowerupAlpha = 0;
	mPlanets.resize(5);
	mStreaks.resize(100);
	mSpawnRules.resize(1);
	mBackdropStars.resize(3);
	mFieldButtons.assign(1, (Widget*) NULL);
	mComboChain = 0;
	mPlayTime = 0;
	mMeterPaused = false;
	mLongestChain = 0;
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
	mSupaTitleStep = 0;
	mSupaBracketSpread = 0;
	mSupaBannerY = 0;
	mSupaBannerTimer = 0;
	mSupaNameAlpha = 0;
	mSupaBracketFlash = 0;
	mSupaBannerStep = 0;
	mBonusBannerHue = 0;
	mScreenShakeUpdates = 0;
	mApp->mUser->GetInt(mMode + 0x33, &mWeaponPower);
	mApp->mUser->GetInt(mMode + 0x3b, &mWeaponChargeLevel);
	if (mMode == 0)
		mApp->mUser->GetInt(0x37, &mWeaponDurationLevel);
	else if (mMode == 3)
		mApp->mUser->GetInt(0x3a, &mWeaponDurationLevel);
	else
		mWeaponDurationLevel = -1;
	mButtonsHeld = 0;
	mOverlay.Resize(0, 0, 0x280, 0x1e0);
	mOverlay.mBoard = this;
	mOverlay.mHasTransparencies = true;
	mOverlay.mHasAlpha = true;
	mRowReductionPeriod = 8;
	mFieldBricks.resize(20);
	for (size_t i = 0; i < mFieldBricks.size(); i++)
		mFieldBricks[i].resize(10, (Brick*) NULL);
	mSetSize = 4;
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
	mExtraColors = 0;
	mScoreShown = 0;
	mScore = 0;
	mSlowMotion = false;
	mWeaponCharge = gTestChargeAtReset < 0.0f ? 0.0f : gTestChargeAtReset;   // (a test's charge, BoardState.cpp)
	ResetGame();
	if (mGameType == 1)
		SetupLevel1Rules();
	else if (mGameType != 0)
		SetupClassicRules();
	mHandKindBrick = NULL;
	mShipX = 0xa6;
	mHeldBricks.clear();

	// the script rows are cleared and so are the first rows of the field
	for (int aRow = 0; aRow < 100; aRow++)
	{
		for (int aCol = 0; aCol < 10; aCol++)
		{
			mScriptRows.mCode[aRow][aCol] = 0;
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
	mScriptRowIndex = 0;
	SetupHints();
	if (1 < theLevel * 2 - 2)
	{
		int aCount = theLevel * 2 - 3;
		do
		{
			mStageNumber = mStageNumber + 1;
			AdvanceLevel(false);
			aCount--;
		} while (aCount != 0);
	}
	MarkHintsSeen(theLevel);
	if ((theFlags & 0xff) != 0 && !theDemo)
		LoadLevel(true);
	mShipFrame = 2;
	mStreaksFadingOut = false;

	// the scenery layer; the pictures of the three ids around it are released (the layer's own stays)
	mSceneryLayer.mImage = LoadImageById(mApp->mResourceManager, 0x18c);
	for (int anId = 0x18b; anId < 0x18e; anId++)
	{
		if (anId != 0x18c)
			mApp->mResourceManager->DeleteImage(GetStringIdById(anId));
	}
	mSceneryLayer.mPictureId = 0x18c;
	mSceneryLayer.mCurY = 0.0f;
	mSceneryLayer.mNextY = -1.0f;
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
	mWarpShipY = 500000000.0f;
	mEndFlashAlpha = 0;
	mEffectsUpdateCount = 500000000;
	mWarpGlowStep = 500000000;
	mWarpGlowAlpha = 500000000;
	mWarpChargeAlpha = 500000000;
	mWarpPictureLift = 500000000;
	mWarpShakeX = 500000000;
	mFastGateGlow = 500000000;
	mSlowGateGlow = 500000000;
	mEasyGateGlow = 500000000;
	mWarpGateAlpha = 500000000;
	mWarpLightningY = 500000000;
	mWarpLightningPeriod = 500000000;
	mWarpLightningAlpha = 500000000;
	mPauseTextWanted = true;
	mFieldButtons[0] = NULL;
	mHelpButton = NULL;
	mTutorialPickTimer = 0;
	mTutorialPickCount = 0;
	mPowerupActive = 0;
	mWeaponChargeMax = 18000.0f;
	ApplyWeaponLevels();
	mRankNumber = 0;
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
	mSpecialBannerHue = 0;
	mSlowGateStage = mStageNumber + 2;
	int aHalf = mSlowGateStage / 2;
	int aRemainder = (aHalf + 1) % 4;
	if (aRemainder == 0)
		mFastGateStage = -1;
	else
		mFastGateStage = (aHalf - aRemainder) * 2 + 8;
	mBestLevelReached = -1;
	mApp->mUser->GetInt(mMode + 8, &mBestLevelReached);
	if (mBestLevelReached * 2 - 2 < mFastGateStage)
		mFastGateStage = -1;
	if (mFastGateStage < 1)
	{
		mSlowGateX = 0x17d - IMAGE_SLOW_WARP_GATE->GetCelWidth() / 2;
		mFastGateX = -1;
	}
	else
	{
		mSlowGateX = 0xb6;
		mFastGateX = 0x192;
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
	mScreenWiping = false;
	mScreenWipeKind = -1;

	// the picture of the field that the wipe at the end of a game takes away, and the wipe
	DDImage* aFieldImage = new DDImage(mApp->mDDInterface);
	mScreenImage = aFieldImage;
	aFieldImage->Create(0x280, 0x1e0);
	aFieldImage->SetVolatile(true);
	mScreenGraphics = new Graphics(mScreenImage);
	mScreenWipe = new ScreenWipe();
	mScreenWipeKind = ((int) AstroRand() % 500000000) % 4;
	if (mScreenWipeKind == 0)
		mScreenWipeSteps = 2000;
	else
		mScreenWipeSteps = (mScreenWipeKind != 3) * 2 + 4;
	mScreenWipe->SetImage(mScreenImage, false);
	mScreenWipe->SetKind(mScreenWipeKind);
	mSirenOffDelay = -1;
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
		if (mGameType == 2)
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
		gStateSaved = false;
		break;
	}
}
