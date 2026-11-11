// Board: the update of a frame (original 0x479480, slot 17). One big function in the original; the port keeps its order
// of work and splits some blocks out into members named after what they do.

#include "Board.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "AstroPopApp.h"
#include "UserProfile.h"
#include "SoundInstance.h"
#include "Particle.h"
#include "Image.h"
#include "Res.h"

using namespace Sexy;

// 0x479480
void Board::Update()
{
	Widget::Update();

	if (mRecordingDemo)
	{
		if (!mDemoRecordAction)
			mDemoRecordArg++;
		else
		{
			mDemoRecordAction = 0;
			mDemoRecordArg = 1;
		}
	}

	// a warp to the next stage is on the screen (and is not fading out already)
	bool aWarpActive = true;
	if (mHintSpots.empty() || !mHintSpots[0]->mActive || -mLevelUpPanelHeight < mLevelUpPanelY)
		aWarpActive = false;

	if (mSlowMotion && (mUpdateCnt % 8) != 0)
	{
		if (mRecordingDemo)
			RecordStep(false);
		return;
	}

	if (mScreenWiping)
	{
		if (mScreenWipe->Check(mScreenWipeSteps))
			mApp->CloseBoard();
		MarkDirty();
		return;
	}

	if (mPauseCount >= 1)
	{
		// the stage's picture is covered by the warp: the field's cover slides down
		mPausedUpdates++;
		if (mPauseDarkening < mHeight)
		{
			MarkDirty();
			int aCover = mPauseDarkening;
			mPauseDarkening = aCover + 0x14;
			if (mHeight < aCover + 0x14)
				mPauseDarkening = mHeight;
			mOverlay.MarkDirtyFull();
		}

		if (mHintSpots.empty() || !mHintSpots[0]->mActive || mPauseCount != 1)
		{
			if (mWeaponHintShown)
			{
				mWeaponHintPulse.Update(8);
				MarkDirtyFull();
			}
		}
		else if (!mWeaponHintShown)
			UpdateStageBackground();
		else
		{
			mWeaponHintPulse.Update(8);
			MarkDirtyFull();
		}

		if (aWarpActive)
			UpdateEffects();
	}
	else
	{
		mApp->mGameUpdates++;
		if (mApp->mHelpScreen != NULL)
			return;

		if (mFirstHelpPending && mApp->mShowHelpAtStart && !mApp->mHelpShown)
		{
			mFirstHelpPending = false;
			mApp->ShowFullScreenHelp();
			return;
		}

		// the demo (the title's game plays by itself): one step of its script
		if (mDemoPlaying)
		{
			unsigned int aSize = (unsigned int) mDemoSteps.size();
			unsigned int aStep = (unsigned int) mDemoStepIndex;
			if (aSize <= aStep)
			{
				ResetState();
				ResetGame();
				BuildField();
				return;
			}

			int anAction = mDemoSteps[aStep].mAction;
			if (anAction == 0)
			{
				int aWait = mDemoWaitLeft;
				mDemoWaitLeft = aWait - 1;
				if (aWait - 1 < 1)
					mDemoStepIndex = aStep + 1;
			}
			else
			{
				mDemoInputActive = true;
				if (anAction == 2)
					MouseDown(mLastPointerX, mLastPointerY, 1);
				else if (anAction == 1)
					MouseDown(mLastPointerX, mLastPointerY, -1);
				else
				{
					int aX = 0;
					switch (anAction)
					{
					case 3:		aX = 0x232;	break;
					case 4:		aX = 0x206;	break;
					case 5:		aX = 0x1da;	break;
					case 6:		aX = 0x1ae;	break;
					case 7:		aX = 0x182;	break;
					case 8:		aX = 0x156;	break;
					case 9:		aX = 0x12a;	break;
					case 10:	aX = 0xfe;	break;
					case 0xb:	aX = 0xd2;	break;
					case 0xc:	aX = 0xa6;	break;
					}
					MouseMove(aX, 400);
				}
				unsigned int aNext = (unsigned int) mDemoStepIndex + 1;
				mDemoInputActive = false;
				mDemoStepIndex = (int) aNext;
				if (aNext < (unsigned int) mDemoSteps.size() && mDemoSteps[aNext].mAction == 0)
					mDemoWaitLeft = mDemoSteps[aNext].mArg;
			}
		}

		if (!mWelcomePlayed)
		{
			if (!mDemoPlaying)
			{
				mWelcomePlayed = true;
				mApp->PlaySample(SOUND_VWELCOME);
			}
			if (!mApp->mEngineLoop->IsPlaying())
			{
				mApp->mEngineLoop->SetVolume(1.0);
				mApp->mHyperspaceLoop->SetVolume(1.0);
				mApp->mEngineLoop->Play(true, false);
			}
		}

		UpdateStates();
		if (!gStateSaved && mGameType != 2 && !mGameOver && mLevelNumber > 1)
			SaveGameState();
		ShowThemeButtons();
		if (mAtomicFlashAlpha < 0)
			UpdateScenery();
		if (mGameOver)
			UpdateGameOver();

		if (mBonusRoundOn && mBricksRevealing && (mUpdateCnt % 2) == 0)
			RevealBricks();

		SweepSprockets();
		SweepTurbots();

		if (IsSettled(false))
			LevelComplete();
		if (UpdateWarp())
			return;

		if (mStreaksFadingOut && !aWarpActive)
			UpdateEffects();
		UpdateAnimCounters();
		if (mBackdropTintChanging)
			FadeBackdrop();
		UpdateBricks();
		if (mGameType == 2)
			UpdateRamp();

		if ((mUpdateCnt % mRowReductionPeriod) == 0 && !mApp->mTutorialOn && mHardStage <= mStageNumber && mPlayStarted && mRowIntervalReduction < mRowInterval - 200 && mFreezeUpdatesLeft == 0)
			mRowIntervalReduction++;

		if (mStackDanger < mStackDangerPeak)
		{
			float aNew = mStackDangerPeak - 0.2f;
			mStackDangerPeak = aNew;
			if (aNew < mStackDanger)
				mStackDangerPeak = mStackDanger;
		}

		if (SpawnControl())
			return;

		// the floating texts
		for (int i = 0; i < (int) mFloatTexts.size(); i++)
		{
			mFloatTexts[i]->Update();
			if (mFloatTexts[i]->mDone)
			{
				delete mFloatTexts[i];
				mFloatTexts.erase(mFloatTexts.begin() + i);
				i--;
			}
		}

		mSirenAnim.Update();

		// the score on the display catches up with the score, faster the further behind it is
		int aShown = mScoreShown;
		int aScore = mScore;
		if (aShown < aScore)
		{
			int aDiff = aScore - aShown;
			if (aDiff < 1000)
				aShown += 0xf;
			else if (aDiff < 2000)
				aShown += 0x32;
			else if (aDiff < 5000)
				aShown += 0xfa;
			else if (aDiff < 10000)
				aShown += 700;
			else if (aDiff < 20000)
				aShown += 0x5dc;
			else if (aDiff < 50000)
				aShown += 4000;
			else if (aDiff < 200000)
				aShown += 10000;
			else if (aDiff < 400000)
				aShown += 20000;
			else if (aDiff < 600000)
				aShown += 50000;
			else if (aDiff < 800000)
				aShown += 70000;
			else if (aDiff < 1000000)
				aShown += 100000;
			else
				aShown += 150000;
			mScoreShown = aShown;
			if (aScore <= aShown)
				mScoreShown = aScore;
		}

		mLaserAnim.Update();
		if (mFreezeUpdatesLeft == 0 && mPlayStarted)
			mRowTimer++;

		UpdateExplosion();

		// the blinking pictures
		for (int i = 0; i < 40; i++)
		{
			mBeamSparks[i].mCounter--;
			if (mBeamSparks[i].mCounter < -0x14)
				mBeamSparks[i].mCounter = 400;
			mBeamSparks[i].mAnim.Update();
		}

		if (mGameType == 2)
			UpdateSparks();

		if (mGameOver && !mHighScoreSoundDone)
		{
			mHighScoreSoundDelay--;
			if (mHighScoreSoundDelay < 1)
			{
				mHighScoreSoundDone = true;
				if (mNewRecordRow == 0)
					mApp->PlaySample(SOUND_NEW_HIGH_SCORE);
			}
		}
		if (!mNewRankDelayOver && mNewRankDelay > 0)
		{
			mNewRankDelay--;
			if (mNewRankDelay < 1)
				mNewRankDelayOver = true;
		}

		// with the left button held in the field of a classic game the click is repeated every update
		if (mMode == 0 && mLastPointerX > 0xa5 && mLastPointerX < 0x25f && mPowerupActive == 1 && mApp->IsLeftButtonDown())
		{
			bool aFlag = mClickRepeating;
			mClickRepeating = true;
			MouseDown(mLastPointerX, mLastPointerY, 1);
			mClickRepeating = aFlag;
		}

		for (unsigned int i = 0; i < mHintBoxes.size(); i++)
			mHintBoxes[i]->Update();

		if ((mUpdateCnt % 300) == 0 && mGameType == 2)
			DeleteDoomedBricks();
		MarkDirty();
	}

	if (mRecordingDemo)
		RecordStep(false);
}

// the field's bricks appear one at a time (every second update) while a new field is built from the top: the first
// brick that is not visible yet shows (and turns into a rainbow brick); when none is left the targets stop flashing and
// the field is ready
void Board::RevealBricks()
{
	for (int aRow = 1; aRow <= 0xe; aRow++)
	{
		for (int aCol = 0; aCol < 10; aCol++)
		{
			Brick* aBrick = mFieldBricks[aRow][aCol];
			if (aBrick != NULL && !aBrick->mVisible)
			{
				aBrick->SetVisible(true);
				aBrick->MakeRainbow();
				return;
			}
		}
	}

	for (unsigned int i = 0; i < mTargeted.size(); i++)
		mTargeted[i]->StopFlash();
	if (!mBonusRevealDone)
	{
		mBonusRevealDone = true;
		mBonusRevealDelay = 300;
	}
	mTargeted.clear();
	mBricksRevealing = false;
	mClicksBlocked = false;
	mBonusTimeLeft = 6000;
	HighlightTargets();
}

// every second update, in mode 1 of a running game: the first brick that has no sprocket yet is told it hits a marked
// cell (event 0x18) or not (0x16)
void Board::SweepSprockets()
{
	if (mPowerupActive != 1)
		return;
	if ((mUpdateCnt % 2) != 0 || mMode != 1 || mAtomicMarkedBricks.empty())
		return;

	for (unsigned int aRow = 1; aRow < mFieldBricks.size(); aRow++)
	{
		for (int aCol = 0; aCol < 10; aCol++)
		{
			Brick* aBrick = mFieldBricks[aRow][aCol];
			if (aBrick != NULL && !aBrick->mSprocketOn && !aBrick->mSprocketFading)
			{
				FireTo(aBrick, HasCell3(aCol, aRow, mAtomicMarkedBricks) ? 0x18 : 0x16);
				return;
			}
		}
	}
}

// the same in mode 2 with the turbot bricks (events 0x19 and 0x17); the phase of mTurbotPhase moves on once the list of the
// marked cells is empty (or was)
void Board::SweepTurbots()
{
	if (mPowerupActive != 1 || mTurbotPhase == 1 || mTurbotPhase == 3)
		return;
	if ((mUpdateCnt % 2) != 0 || mMode != 2)
		return;

	if (!mTurbotMarkedCells.empty())
	{
		for (unsigned int aRow = 1; aRow < mFieldBricks.size(); aRow++)
		{
			for (int aCol = 0; aCol < 10; aCol++)
			{
				Brick* aBrick = mFieldBricks[aRow][aCol];
				if (aBrick != NULL && !aBrick->mTurbotOn && !aBrick->mTurbotFading)
				{
					FireTo(aBrick, HasCell(aCol, aRow, mTurbotMarkedCells) ? 0x19 : 0x17);
					if (!mTurbotMarkedCells.empty())
						return;
					goto Phase;
				}
			}
		}
	}
Phase:
	if (mTurbotPhase == 0)
		mTurbotPhase = 1;
	else if (mTurbotPhase == 2)
		mTurbotPhase = 3;
}

// when the field is quiet and the time has come, the next piece drops; when there is no room left the game ends (if the
// profile asks for it). Returns true when the update ends here.
bool Board::SpawnControl()
{
	if (!(mTopRowsAllowed && !mNewRowBouncing && !mBonusRoundOn && !mWarpCharging && !mWarpActive && !mWarpFlight && !mAtWarpGate && !mWarpGatePicked))
		return false;

	int aTime;
	if (mGameType == 2)
		aTime = mRowInterval;
	else
	{
		float aStage = (mStageNumber > 5) ? 0.0f : (float) (8 - mStageNumber);
		aTime = (int) ((float) (mRowInterval - mRowIntervalReduction) + (aStage + aStage + 20.0f) * mStackDanger);
	}

	int aPhase = mPowerupActive;
	if (aPhase == 0)
		mExtraRowDelay--;

	if (mSupportedBrickCount == 0 && CountMovingBricks(false) == 0 && mTakenBrickCount == 0 && !mStreaksFadingOut && (aPhase != 1 || mFieldCleared) &&
		((mMode == 0 && (aPhase != 1 || mLaserUsed)) || (mMode == 3 && (aPhase != 1 || mPsychicUsed)) || aPhase == 0 || mFieldCleared) &&
		!mApp->mTutorialOn && mPlayStarted && mKillAllTypes.empty() && mFreezeUpdatesLeft < 1 && (aTime <= mRowTimer || mExtraRowsDue > 0))
	{
		int aWaited = mRowTimer;
		int aSpare = mExtraRowsDue;
		if (aSpare >= 1 && !mFieldCleared && mExtraRowDelay > 0 && aWaited < aTime)
			return false;

		if (mExtraRowDelay < 0)
			mExtraRowDelay = 0;

		if (mStackWarningActive)
		{
			mStackWarningTimer--;
			if (mStackWarningTimer < 1)
				mStackWarningActive = false;
			if (mStackWarningActive)
				return false;
		}

		mSetMadeSinceTopRow = false;
		if (aWaited < aTime)
		{
			if (mExtraRowDelay < 1)
			{
				mExtraRowsDue = aSpare - 1;
				if (mExtraRowsDue >= 0)
					mExtraRowDelay = 100;
				else
					mExtraRowsDue = 0;
			}
		}
		else
		{
			mRowTimer = 0;
			if (aSpare > 0)
			{
				mExtraRowsDue = aSpare - 1;
				mExtraRowDelay = 100;
			}
		}

		if (NoRoomLeft())
		{
			int aValue = 1;
			mApp->mUser->GetInt(0x2e, &aValue);
			if (aValue == 1 && mWeaponChargeMax <= mWeaponCharge)
			{
				Pause(true, false);
				mOverlay.SetEmergencyPause(true);
				return true;
			}
		}
		AddTopRow();
		mHintBoxes[2]->SetShown(false);
		UpdateKlaxon();
	}
	return false;
}

// the 8 explosion animations of the game's end: they count their loops; when all 8 are through the explosion is over
// (the animations are freed); the white flash of the overlay fades in as the blast grows and out again
void Board::UpdateExplosion()
{
	int aNumDone = 0;
	for (int i = 0; i < (int) mAtomicExplosions.size(); i++)
	{
		FrameAnim* anAnim = mAtomicExplosions[i];
		anAnim->Update();
		if (anAnim->mDone || (0 < anAnim->mMaxLoops && anAnim->mMaxLoops <= anAnim->mLoopCount))
			aNumDone++;
		if (i == 7 && anAnim->mNumFrames != 1 && anAnim->mFrame == 7 && !anAnim->mStopped && mAtomicSymbolTimer == -1)
		{
			mAtomicSymbolTimer = 0;
			anAnim->SetStopped(true);
			anAnim->SetDelay(3);
		}
	}

	if (aNumDone == 8)
	{
		mAtomicBlastOver = true;
		mAtomicSymbolTimer = -1;
		mAtomicFlashAlpha = -1;
		SetOverlayHidden(true);
		for (unsigned int i = 0; i < mAtomicExplosions.size(); i++)
			delete mAtomicExplosions[i];
		mAtomicExplosions.clear();
	}

	int aCount = (int) mAtomicExplosions.size();
	int aFade = mAtomicFlashAlpha;
	if (aFade >= 0)
	{
		bool aSetFill = true;
		if (aCount >= 1)
		{
			FrameAnim* aLast = mAtomicExplosions[aCount - 1];
			int aCel = (aLast->mNumFrames == 1) ? 0 : aLast->mFrame;
			if (aCel > IMAGE_ATOMIC_EXPLOSION->mNumCols / 2)
			{
				aFade += 10;
				mAtomicFlashAlpha = aFade;
				if (aFade > 0xfe)
				{
					mAtomicFlashAlpha = -1;
					SetOverlayHidden(true);
				}
				mOverlay.SetFill(255, 255, 255, aFade);
				return;
			}
		}
		if (aFade < 0 || aCount < 1 || mAtomicExplosions[aCount - 1]->mStopped)
			aSetFill = false;
		if (aSetFill)
		{
			mAtomicFlashAlpha = aFade + 1;
			mOverlay.SetFill(255, 255, 255, aFade + 1);
		}
	}
}

// the energy mode's sparks: every 10 updates a row of white sparks starts at the top of the field; they fall and are
// freed when done
void Board::UpdateSparks()
{
	if (mSurvivalBonusTimer > 0)
		mSurvivalBonusTimer--;
	if (mSurvivalBonusTimer > 0 && (mUpdateCnt % 10) == 0)
	{
		int aCol = 0;
		int aSparkWidth = IMAGE_PARTICLE_WHITE->GetCelWidth();
		if (0x1b8 / (aSparkWidth + 3) > 0)
		{
			do
			{
				float aSpeed = (float) (((int) AstroRand() % 500000000) % 300) * 0.001f + 0.1f;
				Particle* aParticle = new Particle(0.0f, 0.0f, 0.0f, 0, aSpeed, (float) mHeight, gSinTable, gCosTable);
				aParticle->mLife = 0x32;

				int aRandom = (int) AstroRand() % 500000000;
				int aWidth = IMAGE_PARTICLE_WHITE->GetCelWidth();
				aParticle->SetX((float) ((aWidth + aRandom % 8) * aCol + 0xa6));
				aParticle->SetY((float) (((int) AstroRand() % 500000000) % 10 + 0x11));
				aParticle->SetImage(GetImageById(((int) AstroRand() % 500000000) % 5 + 0x23f), 5);
				mEnergySparks.push_back(aParticle);

				aCol += mApp->Is3DAccelerated() ? 2 : 4;
			}
			while (aCol < 0x1b8 / (IMAGE_PARTICLE_WHITE->GetCelWidth() + 3));
		}
	}

	for (unsigned int i = 0; i < mEnergySparks.size(); i++)
	{
		mEnergySparks[i]->Update();
		if (mEnergySparks[i]->mDone)
		{
			delete mEnergySparks[i];
			mEnergySparks.erase(mEnergySparks.begin() + i);
			i--;
		}
	}

	if (!mSuperchargeBlastAnim.mStopped)
		mSuperchargeBlastAnim.Update();
}
