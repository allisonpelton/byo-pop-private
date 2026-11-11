// Board: the timers and effects that step every update (UpdateStates): the pace of the incoming pieces, the fade between
// the demo and the game, the alert flashes, the start banner, the minute banners, the freeze effect, the scrolling texts
// and the countdown of the marks on bricks.

#include "Board.h"
#include "Brick.h"
#include "AstroPopApp.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "SmallObjects.h"
#include "Res.h"

using namespace Sexy;

// 0x433920: the alert flashes. While the stack is high (+0x970 from 14) the danger picture pulses (+0x1aa8 between 0 and
// 255 by +0x1aa4) and its cover fades in (+0x1aac); the warning at the top (+0x1b3c) comes up when nothing else is going
// on. The "game over" danger picture slides in from above (+0x1a98, +0x1a9c); when the timer of the klaxon (+0x1a7c)
// ends the siren animation stops, the song goes back and the pace of the pieces is chosen again.
void Board::UpdateAlertFlash()
{
	if (mHighestStack < 0xe || mBonusRoundOn)
	{
		mAlertLevel = 1;
		if (mDangerZoneAlpha > 0 && !mBonusRoundOn)
		{
			mDangerZoneAlpha -= 6;
			if (mDangerZoneAlpha < 0)
				mDangerZoneAlpha = 0;
		}
		if (m1b3c > 0)
		{
			m1b3c -= 2;
			if (m1b3c < 0)
				m1b3c = 0;
		}
	}
	else
	{
		mAlertLevel = mHighestStack - 0xc;
		if (mAlertLevel > 5)
			mAlertLevel = 5;

		mDangerPulseAlpha += mDangerPulseStep;
		if (mDangerPulseAlpha >= 0x100)
		{
			mDangerPulseAlpha = 0xff;
			mDangerPulseStep = -mDangerPulseStep;
		}
		else if (mDangerPulseAlpha < 0)
		{
			mDangerPulseAlpha = 0;
			mDangerPulseStep = -mDangerPulseStep;
		}

		if (mDangerZoneAlpha < 0xff)
		{
			mDangerZoneAlpha += 4;
			if (mDangerZoneAlpha > 0xff)
				mDangerZoneAlpha = 0xff;
		}

		if (mPowerupActive == 1 || m1bd8 < m1bdc || mWarpActive || mWarpFlight || mAtWarpGate || mWarpGatePicked || mWarpCharging || mLevelUpPhase || mGameOver || mPauseCount != 0 ||
			m1b3c > 0x7f)
		{
			if (m1b3c > 0)
			{
				m1b3c -= 2;
				if (m1b3c < 0)
					m1b3c = 0;
			}
		}
		else
			m1b3c++;

		if (m1b3c > 0 && mUpdateCnt % 0x32 == 0)
			mSupaButtonLit = !mSupaButtonLit;
	}

	if (mGameOverDangerY > -500000000)
	{
		if (mGameOverDangerY > -0x14 - IMAGE_GAMEOVER_DANGER->mHeight)
		{
			int aOther = mGameOverDangerHold;
			if (aOther < 1)
			{
				int aBefore = mGameOverDangerY;
				mGameOverDangerY = aBefore - 0xc;
				if (aBefore - 0xc < 0x15 && aOther == -500000000)
					mGameOverDangerHold = 1;
			}
			else
			{
				mGameOverDangerHold = aOther + 1;
				if (aOther + 1 > 0x95)
					mGameOverDangerHold = -250000000;
			}
		}
		else
		{
			mGameOverDangerY = -500000000;
			mGameOverDangerHold = -500000000;
		}
	}

	if (mSirenOffDelay > 0)
	{
		mSirenOffDelay--;
		if (mSirenOffDelay < 1)
		{
			mSirenOffDelay = -1;
			mSirenAnim.SetStopped(true);
			mApp->PlaySong(9);
			if (mPhase < 10 || mPhase > 0xd)
			{
				mPhase = 4;
				mFlurryTimer = 0;
			}
			mPhaseDelay = PickSpawnDelay();
		}
	}
}

// 0x433610: the start banner (+0x1ad4 is its step, 0 to 6): the brackets slide in, the lines fade in one by one, hold,
// and the lot slides out. Step 6 of a game of the second kind lets the next part (+0x911) begin. The screen shake of a
// strong hit (+0x1aec updates, +0x1a80 the shift) runs beside it.
void Board::UpdateStartBanner()
{
	if (mScreenShakeUpdates > 0 && mPowerupActive == 1)
	{
		mScreenShakeX = ((int) AstroRand() % 500000000) % 5 - 2;
		mScreenShakeUpdates--;
		if (mScreenShakeUpdates < 1)
			mScreenShakeX = 0;
	}

	if (mSupaBannerStep == 1)
	{
		mSupaBannerY += 4;
		if (mSupaBannerY >= 0)
		{
			mSupaBannerY = 0;
			mSupaBracketSpread = 0;
			mSupaBannerStep = 2;
		}
	}
	else if (mSupaBannerStep == 2)
	{
		mSupaBracketSpread += 0xc;
		if (mSupaBracketSpread > 0xe6)
		{
			mSupaBannerTimer = 0;
			mSupaTitleStep = 0;
			mSupaBracketSpread = 0xe7;
			mSupaBannerStep = 3;
		}
	}
	else if (mSupaBannerStep == 3)
	{
		mSupaBracketFlash -= 3;
		if (mSupaBracketFlash < 1)
			mSupaBracketFlash = 0;
		mSupaBannerTimer++;
		if (mSupaBannerTimer > 3)
		{
			mSupaBannerTimer = 0;
			mSupaTitleStep++;
			if (mSupaTitleStep > 10)
			{
				mSupaNameAlpha = 0;
				mSupaTitleStep = 10;
				mSupaBannerStep = 4;
			}
		}
	}
	else if (mSupaBannerStep == 4)
	{
		mSupaBracketFlash -= 3;
		if (mSupaBracketFlash < 1)
			mSupaBracketFlash = 0;
		mSupaNameAlpha += 10;
		if (mSupaNameAlpha > 0xfe)
		{
			mSupaBannerTimer = 0;
			mSupaNameAlpha = 0xff;
			mSupaBannerStep = 5;
		}
	}
	else if (mSupaBannerStep == 5)
	{
		mSupaBannerTimer++;
		if (mSupaBannerTimer > 0x95)
		{
			mSupaBracketFlash -= 3;
			if (mSupaBracketFlash < 1)
				mSupaBracketFlash = 0;
			mSupaBannerTimer = 0;
			mSupaBannerStep = 6;
			if (mBonusRoundOn)
				mStartBannerDoneInBonus = true;
		}
	}
	else if (mSupaBannerStep == 6)
	{
		mSupaBracketFlash -= 3;
		if (mSupaBracketFlash < 1)
			mSupaBracketFlash = 0;
		mSupaBannerY -= 4;
		if (mSupaBannerY <= -IMAGE_SUPA_BRACKET_LEFT->mHeight)
			mSupaBannerStep = 0;
	}
}

// 0x452a80: a time bonus. While the game is young (+0x1a2c counts the updates since it began to run, up to 700) the bonus is
// paid once for each step of the time (200, 300, 400, 500, 600, 700 updates) when the player has done well enough in the
// time (+0x1a50 at least 3 .. 8): a thousand points for each step times the fraction of the stage left (+0x940), at most
// 10000, 15000, 20000, 22000, 25000, 30000, rounded down to 50.
void Board::PayTimeBonus()
{
	int aBonus = 0;
	if (mBonusRoundOn)
		return;

	int aTime = mTimeBonusUpdates;
	if (aTime <= 0xc8 && mTimeBonusStepsPaid == 0 && mTimeBonusSets >= 3)
	{
		mTimeBonusStepsPaid = 1;
		aBonus = (int) (mScoreMultiplier * 1000.0f);
		if (aBonus > 10000)
			aBonus = 10000;
	}
	else if (aTime <= 0x12c && mTimeBonusStepsPaid <= 1 && mTimeBonusSets >= 4)
	{
		mTimeBonusStepsPaid = 2;
		aBonus = (int) (mScoreMultiplier * 2000.0f);
		if (aBonus > 15000)
			aBonus = 15000;
	}
	else if (aTime <= 0x190 && mTimeBonusStepsPaid <= 2 && mTimeBonusSets >= 5)
	{
		mTimeBonusStepsPaid = 3;
		aBonus = (int) (mScoreMultiplier * 3000.0f);
		if (aBonus > 20000)
			aBonus = 20000;
	}
	else if (aTime <= 0x1f4 && mTimeBonusStepsPaid <= 3 && mTimeBonusSets >= 6)
	{
		mTimeBonusStepsPaid = 4;
		aBonus = (int) (mScoreMultiplier * 4000.0f);
		if (aBonus > 22000)
			aBonus = 22000;
	}
	else if (aTime <= 0x258 && mTimeBonusStepsPaid <= 4 && mTimeBonusSets >= 7)
	{
		mTimeBonusStepsPaid = 5;
		aBonus = (int) (mScoreMultiplier * 5000.0f);
		if (aBonus > 25000)
			aBonus = 25000;
	}
	else if (aTime <= 0x2bc && mTimeBonusStepsPaid <= 5 && mTimeBonusSets >= 8)
	{
		mTimeBonusStepsPaid = 6;
		aBonus = (int) (mScoreMultiplier * 6000.0f);
		if (aBonus > 30000)
			aBonus = 30000;
	}

	aBonus = aBonus - aBonus % 50;
	if (aBonus > 0)
	{
		FloatText* aText = new FloatText(StrFormat(GTC(0x5652b0), aBonus), FONT_POINTS, 300.0f, (float) (mTimeBonusStepsPaid * 40 + 0xe6), false);
		mFloatTexts.push_back(aText);
		AddScore(aBonus, true);
		aBonus = 0;
	}

	if (mTimeBonusUpdates > 0x2bc)
	{
		mTimeBonusRunning = false;
		mTimeBonusStepsPaid = 0;
		mTimeBonusUpdates = 0;
		mTimeBonusSets = 0;
	}
}

// 0x474de0: the freeze effect. While the freeze lasts (+0x19fc updates, counting down once the game runs) a snowflake
// is added every 25 updates (up to 60) at one edge of the top of the screen, drifting across at one pixel an update; the
// effect ends with event 0x28 (0).
void Board::UpdateFreeze()
{
	if (mFreezeUpdatesLeft <= 0)
		return;

	if (mPlayStarted)
		mFreezeUpdatesLeft--;

	if (mUpdateCnt % 0x19 == 0)
	{
		if (mFreezeFlakes.size() < 0x3c)
		{
			FreezeFlake aFlake;
			aFlake.mAnim.SetNumFrames(IMAGE_PARTICLE_SNOWFLAKE->mNumCols);
			aFlake.mAnim.SetDelay(((int) AstroRand() % 500000000) % 5 + 7);
			aFlake.mAnim.SetStopped(false);
			aFlake.mVelY = 0.0f;
			if ((mFreezeFlakes.size() & 1) == 0)
			{
				aFlake.mVelX = -1.0f;
				aFlake.mX = 640.0f;
				aFlake.mY = (float) (((int) AstroRand() % 500000000) % 5 + 7);
			}
			else
			{
				aFlake.mVelX = 1.0f;
				aFlake.mX = 105.0f;
				aFlake.mY = (float) (((int) AstroRand() % 500000000) % 5 + 7);
			}
			mFreezeFlakes.push_back(aFlake);
		}
	}

	for (unsigned int i = 0; i < mFreezeFlakes.size(); i++)
	{
		mFreezeFlakes[i].mX = mFreezeFlakes[i].mVelX + mFreezeFlakes[i].mX;
		if (mFreezeFlakes[i].mX > 640.0f)
			mFreezeFlakes[i].mX = 105.0f;
		else if (mFreezeFlakes[i].mX < 105.0f)
			mFreezeFlakes[i].mX = 640.0f;
		mFreezeFlakes[i].mAnim.Update();
	}

	if (mFreezeUpdatesLeft == 0)
		Fire(0x28, 0);
}

// 0x475040: the per-update timers of the game (see the file's header). While the warp cover is not up (+0x1924 < 1) the
// pace timers, the level picture, the fade between the demo and the game, the choice of the next piece pace and the
// side panels step; then the alerts, the start banner and the freeze run, and while the game is running the minute
// banner, the stage, the clear count-down and the other state machines.
void Board::UpdateStates()
{
	if (mPauseCount < 1)
	{
		if (mFlurryTimer > 0)
			mFlurryTimer--;
		if (mPhaseDelay > 0)
			mPhaseDelay--;

		if (mLevelUpTextShown)
		{
			mLevelUpText->Update();
			if (mLevelUpText->mActive == 0)
				mLevelUpTextShown = false;
		}

		mBoosterAnim.Update();

		if (!mWarpActive && !mWarpFlight && !mAtWarpGate && !mWarpGatePicked && !mWarpCharging && !mLevelUpPhase && !mGameOver && mPauseCount == 0 && mFreezeUpdatesLeft < 1)
		{
			if (m1b5c > 0)
			{
				m1b5c--;
				if (m1b5c < 1)
					m1b5c = 0;
			}
		}

		if (mStartTextAlpha > 0)
		{
			mStartTextAlpha -= 4;
			if (mStartTextAlpha < 2)
				mStartTextAlpha = -1;
		}

		if (!mDemoEnding || !mDemoPlaying)
		{
			if (mFadingFromWhite)
			{
				mWhiteFadeAlpha -= 6;
				if (mWhiteFadeAlpha < 1)
				{
					mWhiteFadeAlpha = 0;
					mFadingFromWhite = false;
				}
				mOverlay.SetFill(0xff, 0xff, 0xff, mWhiteFadeAlpha);
			}
		}
		else
		{
			mWhiteFadeAlpha += 6;
			if (mWhiteFadeAlpha > 0xfe)
			{
				for (unsigned int i = 0; i < mFloatTexts.size(); i++)
					delete mFloatTexts[i];
				mFloatTexts.clear();
				mWhiteFadeAlpha = 0xff;
				mDemoPlaying = false;
				mDemoEnding = false;
				ClearField();
				ResetState();
				ResetGame();
				LoadLevel(true);
				mFadingFromWhite = true;
			}
			mOverlay.SetFill(0xff, 0xff, 0xff, mWhiteFadeAlpha);
		}

		if (mPowerupActive == 1)
		{
			if (mPhaseDelay < 1 && mPhase == 7 && (mMode == 0 || mMode == 3))
			{
				mPhaseDelay = 0;
				mPhase = 6;
			}
			else if (mPowerupActive == 1 && mPhaseDelay < 1 && (mMode == 1 || mMode == 2))
			{
				mPhase++;
				if (mPhase > 7)
					mPhase = 6;
				mPhaseDelay = PickSpawnDelay();
			}
		}
		else if (mPhaseDelay < 1)
		{
			int aPace = mPhase;
			if (aPace < 2 || aPace > 5)
			{
				if (aPace >= 8 && aPace <= 9)
					mPhase = (aPace == 8) + 8;
				else
				{
					if (aPace >= 10 && aPace <= 0xd)
					{
						aPace++;
						if (aPace > 0xd)
							aPace = 10;
						if (mFlurryTimer < 1)
						{
							mFlurryTimer = 0;
							aPace = mSirenAnim.mStopped ? 4 : 8;
						}
					}
					mPhase = aPace;
				}
			}
			else
			{
				int aNext;
				do
				{
					int aDraw = ((int) AstroRand() % 500000000) % 5;
					if (aDraw < 2)
						aNext = 2;
					else if (aDraw == 2)
						aNext = 3;
					else
						aNext = (aDraw != 3) + 4;
				} while (aNext == mPhase);
				mPhase = aNext;
			}
			mPhaseDelay = PickSpawnDelay();
		}

		if (m1bd8 == m1bdc && mPowerupActive == 0 && m1b48 < 0)
		{
			m1b48 += 2;
			if (m1b48 >= 0)
				m1b48 = 0;
		}
		else if (mPowerupActive == 1 && m1b48 > -IMAGE_SUPA_BUTTON_ALERT->mWidth)
		{
			m1b48 -= 3;
			if (m1b48 <= -IMAGE_SUPA_BUTTON_ALERT->mWidth)
				m1b48 = -IMAGE_SUPA_BUTTON_ALERT->mWidth;
		}

		if (mComboTimeLeft > 0)
		{
			mComboTimeLeft--;
			if (mComboTimeLeft <= 0)
			{
				m1b00 = 0;
				mComboTimeLeft = 0;
			}
			else if (mComboTimeLeft % 0x14 == 0)
				mComboTextVisible = !mComboTextVisible;
		}

		if (m1b18 > 0)
		{
			m1b18--;
			if (m1b18 <= 0)
			{
				m1b18 = 0;
				PayComboBonus();
			}
		}
	}

	UpdateAlertFlash();

	bool aCalm = !mWarpActive && !mWarpFlight && !mAtWarpGate && !mWarpGatePicked && !mWarpCharging && !mLevelUpPhase && !mGameOver && mPauseCount == 0;
	if (aCalm)
		UpdateFreeze();

	if (mWarpActive || mWarpFlight || mAtWarpGate || mWarpGatePicked || mWarpCharging || (!mLevelUpPhase && !mGameOver && mPauseCount == 0))
		UpdateStartBanner();

	if (mWarpActive || mWarpFlight || mAtWarpGate || mWarpGatePicked || mWarpCharging || mLevelUpPhase || mGameOver || mPauseCount != 0 || !mPlayStarted)
		goto Finish;

	if (!mBonusRoundOn && !mBonusWarpIn)
	{
		if (mGameType == 1 || (mMode != 3 && mMode != 0) || mPowerupActive != 1 || mPsychicUsed || mLaserUsed)
		{
			mRampTime++;
			if (mRampTime > 0 && mRampTime % 6000 == 0 && mGameType == 2)
			{
				mLevelUpTextShown = true;
				delete mLevelUpText;

				int aMinutes = mRampTime / 6000;
				std::string aText;
				if (aMinutes > 1)
					aText = StrFormat(GTC(0x56640c), aMinutes);
				else
					aText = GT(0x566400);

				MemoryImage* anImage = new MemoryImage(mApp);
				Font* aFont = GetFontById(0xb2);
				anImage->SetImageMode(true, true);
				anImage->Create(aFont->StringWidth(aText), aFont->GetHeight());

				Graphics g(anImage);
				g.SetColor(Color(0xff, 0xff, 0xff));
				g.SetFont(aFont);
				g.DrawString(aText, 0, aFont->GetAscent());

				mLevelUpText = new LevelUpText(anImage, 0);
				mLevelUpText->m04 = true;
				mApp->PlaySample(SOUND_WORDSWOOSH);
			}
		}
	}

	mPlayTime++;
	UpdateStageBackground();
	UpdateSpecialMode();
	UpdateIntroAndTally();

	if (m1b64 == 1)
	{
		int aBefore = m1b60;
		m1b60 = aBefore + 8;
		if (aBefore + 8 > 0xfe)
		{
			m1b64 = 2;
			m1b60 = 0xff;
		}
	}
	else if (m1b64 == 2 && (m1b68--, m1b68 < 1))
	{
		m1b68 = 0;
		m1b64 = 3;
	}
	else if (m1b64 == 3)
	{
		int aBefore = m1b60;
		m1b60 = aBefore - 3;
		if (aBefore - 3 < 1)
		{
			m1b64 = 0;
			m1b60 = 0;
		}
	}

	if (mPowerupActive == 0 && m1bbc <= mStageNumber && mPlayStarted && mFreezeUpdatesLeft < 1 && (m1b08--, m1b08 < 1))
	{
		int aHalf = m1b0c / 2;
		m1b0c = aHalf;
		if (aHalf < 300)
			m1b0c = 300;
		m1b08 = m1b0c;
		mExtraRowsDue++;
		if (mExtraRowDelay < 1)
			mExtraRowDelay = 100;
	}

	if (!mSpecialBannerTexts.empty())
	{
		m1b04 = (m1b04 + 4) & 0xff;
		for (unsigned int i = 0; i < mSpecialBannerTexts.size(); i++)
		{
			mSpecialBannerTexts[i].mX++;
			if (mSpecialBannerTexts[i].mX >= mSpecialBannerTexts[i].mFont->StringWidth(mSpecialBannerTexts[i].mText) + 0x29e)
			{
				int aMin = 500000000;
				for (unsigned int j = 0; j < mSpecialBannerTexts.size(); j++)
				{
					if (mSpecialBannerTexts[j].mX < aMin)
						aMin = mSpecialBannerTexts[j].mX;
				}
				mSpecialBannerTexts[i].mX = aMin - mSpecialBannerTexts[i].mFont->StringWidth(mSpecialBannerTexts[i].mText) - 0x1e;
			}
		}
	}

	for (unsigned int i = 0; i < mSpecialCooldowns.size(); i++)
	{
		if (mSpecialCooldowns[i] > 0)
			mSpecialCooldowns[i]--;
	}

	if (mApp->mTutorialOn)
	{
		if (mTutorialPickCount > 0)
			mTutorialPickTimer++;
		if (mTutorialPickTimer == 1000)
			mApp->MakeDialog(mMode + 2, false, 0x6c, -1, -1, 3, GT(0x56633c), GT(0x566348), GT(0x562024), std::string(), true);
	}

Finish:
	if (mPauseDarkening > 0)
	{
		MarkDirty();
		mPauseDarkening -= 0x28;
		if (mPauseDarkening < 0)
			mPauseDarkening = 0;
	}

	if (mPlayStarted)
	{
		for (unsigned int i = 0; i < mSuperchargeMarks.size(); )
		{
			if (mSuperchargeMarks[i].mCount < 1)
			{
				i++;
				continue;
			}
			mSuperchargeMarks[i].mCount--;
			if (mSuperchargeMarks[i].mCount > 0)
			{
				if (mSuperchargeMarks[i].mCount < 0xff)
					Fire(0x25, mSuperchargeMarks[i].mCount);
				i++;
			}
			else
			{
				int aKind = -mSuperchargeMarks[i].mType;
				if (aKind == 0)
					aKind = -500000000;
				Fire(0x20, aKind);
				mSuperchargeMarks.erase(mSuperchargeMarks.begin() + i);
			}
		}
	}

	if (mTimeBonusRunning)
	{
		mTimeBonusUpdates++;
		PayTimeBonus();
	}
}
