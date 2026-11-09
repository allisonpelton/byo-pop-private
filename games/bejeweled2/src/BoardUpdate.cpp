// BoardUpdate.cpp: one frame of the board (Sexy::Board::DoUpdate, 0x5ab4d7-0x5ad284, and Update, 0x5ad294).
//
// The order of the work and of every random draw is the original's. The sweep of the endless mode draws its numbers in
// the order the shards need them (see the Shard fields in Piece.h).

#include "Board.h"
#include "WinDMApp.h"
#include "Profile.h"
#include "Res.h"
#include "GalaxyParser.h"
#include "PuzzleStateOverlay.h"
#include "EffectOverlay.h"
#include "GalaxyMap.h"
#include "Points.h"
#include "StarField.h"
#include "Compat2004.h"
#include "Common.h"
#include "Dialog.h"
#include "DialogButton.h"
#include "ButtonWidget.h"
#include "PodDialog.h"
#include "WidgetManager.h"
#include "HyperlinkWidget.h"
#include "MusicInterface.h"
#include "SoundManager.h"
#include "SoundInstance.h"
#include "MemoryImage.h"
#include "Font.h"
#include <stdlib.h>
#include <algorithm>

using namespace Sexy;

// the places on the endless mode's sweep ring where sparkles start (0x6296f4 and 0x62972c)
static const int gSweepSparkleA[7][2] =
{
	{106, 66}, {144, 93}, {143, 134}, {106, 161}, {66, 135}, {66, 92}, {104, 111}
};
static const int gSweepSparkleB[7][2] =
{
	{34, 42}, {18, 100}, {33, 173}, {94, 211}, {165, 172}, {203, 100}, {164, 40}
};

// the angle and the start of each of the seven arms of the ring that the endless mode's shards swirl on
static const float gSweepAngle[7] =
{
	0.78539819f,        // 0x3f490ff9 (pi / 4)
	0.0f,
	-0.78539819f,       // 0xbf490ff9
	1.57079506f,        // 0x3fc90fd0 (pi / 2, as the original rounds it)
	0.78539819f,
	0.0f,
	-0.78539819f
};
static const int gSweepStart[7][2] =
{
	{76, 104}, {50, 170}, {76, 236}, {144, 262}, {210, 236}, {237, 170}, {207, 104}
};

// 0x5ad294: the frame of the board; the hyper mode runs two updates a frame, and the slow-motion flag runs one every other frame
static bool gUpdateToggle = false;      // DAT_006797c9

void Board::Update()
{
	gUpdateToggle = !gUpdateToggle;
	if (mBackLink != NULL)
		mBackLink->Resize(mApp->ScaleNum(0x302, 0), mApp->ScaleNum(0x2d0, 0), mApp->ScaleNum(0xf0, 0), mApp->ScaleNum(0x20, 0));
	unsigned int aCount = 1;
	if (mIsHyper)
		aCount = 2;
	if (m165af)
		aCount = ((gUpdateToggle ? 1U : 0U) + aCount) / 2;
	for (; aCount != 0; aCount--)
		DoUpdate();
}

// 0x5ab4d7
void Board::DoUpdate()
{
	// the board slides on and off the screen (the galaxy view moves with it in the puzzle modes)
	if ((mScreenState == SCREEN_SLIDING_IN || mScreenState == SCREEN_SLIDING_OUT) && mApp->mHasFocus)
	{
		bool aMore = mSlide.IncInVal(0.01);
		int anOffset = (int)mApp->ScaleNum(mSlide.GetOutVal(), 0.0);
		if (mPuzzleOverlay != NULL)
		{
			mPuzzleOverlay->Resize(-anOffset, 0, mApp->mWidth, mApp->mHeight);
			mPuzzleOverlay->mDomeWidget->Resize(mApp->ScaleNum(0x67, 0) - anOffset, mApp->ScaleNum(0x42, 0),
				mApp->ScaleNumUp(0x61, 0), mApp->ScaleNumUp(0x17, 0));
		}
		ResizeButtons();
		if (!aMore)
			mScreenState = (mScreenState != SCREEN_SLIDING_IN) * 2 + 1;
	}

	if (mApp->GetDialog(0x1a) != NULL)
	{
		// the timer bar's box is up: only its shimmer runs
		m1860 = m1860 + 0.1;
		MarkDirty();
		return;
	}
	if (mHintPiece != 0)
	{
		mUpdateCnt++;
		m1954 = (float)((double)m1954 + 0.1);
		UpdateStarRots();
		UpdateHyperWarpPoints();
		MarkDirty();
		return;
	}

	if (mState != STATE_PUZZLE_SOLVED)
		m1914++;

	if (mPauseCount == mInterfacePauseCount && mInterfacePauseCount > 0)
	{
		// only the interface is paused: the bonus bar and its fade carry on
		if (m15fb4 == 0.0f)
			return;
		UpdateBonusPenalty();
		float aFade = (float)((double)m15fb4 - 0.05);
		m15fb8 = !mApp->mPaused;
		if (aFade <= 0.0f)
			aFade = 0.0f;
		m15fb4 = aFade;
		MarkDirty();
		return;
	}
	if (mPauseCount > 0)
		return;

	// (the trial's counter of played updates, at the application's +0x738, is not ported)
	if (m15fb4 < 1.0f)
	{
		float aFade = (float)((double)m15fb4 + 0.05);
		if (1.0f <= aFade)
			aFade = 1.0f;
		m15fb4 = aFade;
		MarkDirty();
	}
	if (mUnpauseDelay > 0)
		mUnpauseDelay--;
	Widget::Update();
	m1998++;
	if (!mCoversScreenFlag)
		InitialBackdropProcessing();
	if (mPuzzleOverlay != NULL)
	{
		mPuzzleOverlay->Update();
		if (mPuzzleOverlay->mDirty)
		{
			MarkDirty();
			mPuzzleOverlay->mDirty = false;
		}
	}
	if (mState == 7)
		MarkDirty();

	// the explanation of the timer bar, 7 seconds into the first action game
	if (mScreenState == SCREEN_SHOWN && mIsAction && mNoMoveYet && mUpdateCnt == 0x1ae &&
		mApp->mProfile->WantsHint(5))
	{
		Dialog* aDialog = mApp->DoDialog(0x1a, true, "Watch the Timer Bar!",
			"Watch out for the timer bar below. It will go down faster and faster, and you will lose if it "
			"completely runs out!", "", 1);
		mApp->SetupDialog(aDialog, 500);
		aDialog->Move(mApp->ScaleNum(0x14, 0) + aDialog->mX, mApp->ScaleNum(0x118, -0x18) + aDialog->mY);
		aDialog->mYesButton->mLabel = "Don't Show Again";
		aDialog->mNoButton->mLabel = "Continue";
	}

	if (mUpdateCnt == 2)
	{
		if (m1770)
		{
			mApp->mMusicInterface->FadeIn(mApp->mCurrentSong, 1, 0.005);
			mApp->mPendingMusicOffset = mApp->GetInteger(
				(!mIsAction && !mIsPuzzle) ? "MUSICOFFSET_MAIN" : "MUSICOFFSET_ACTION");
		}
		if (mUpdateCnt == 2)
			mApp->ClearUpdateBacklog(false);
	}

	int aState = mState;
	if (aState == STATE_IDLE)
	{
		if (m15f88 > 0)
			m15f88--;
	}
	if (mIsAction && m15f88 == 0 && (aState == STATE_IDLE || aState == STATE_SWAP_BACK || aState == STATE_COMMIT_SWAP))
		UpdateBonusPenalty();

	if (0.0 < m15f90)
	{
		m15f84 = (float)((double)m15f84 - 0.1);
		if (m15f84 < 0.0f)
			m15f84 = (float)((double)m15f84 + 5.0);
		m15fa0 = m15fa0 + 0.0005000000237487257;
		m15f98 = m15f98 - m15fa0;
		MarkDirty();
	}

	if (m1960 && mUpdateCnt % 4 == 0)
	{
		m1964 += m1968;
		if (abs(m1964) > 2)
			m1968 = -m1968;
		MarkDirty();
	}

	if (m15e98 > 0)
	{
		m15e98--;
		bool aDirty = true;
		if (m15e98 < 0x32)
			m15ea0 = (double)m15e98 / 50.0;
		else if (m15ea0 >= 1.0)
			aDirty = false;
		else
		{
			double aNew = m15ea0 + 0.05;
			m15ea0 = aNew;
			if (1.0 < aNew)
				m15ea0 = 1.0;
		}
		if (aDirty)
			MarkDirty();
	}

	// where the bar of the level (or of the timer) wants to be, in 708 steps
	int aBar;
	if (!mIsAction)
	{
		if (!mIsPuzzle)
			aBar = ((m15e40 - (int)m15ef8 - m15ee8) * 0x2c4) / (m15eec - m15ee8);
		else
		{
			aBar = (m17ac * 0x2c4) / 3;
			if (aBar > 0x2c3)
				aBar = 0x2c4;
		}
	}
	else
		aBar = ((((m15e40 - (int)m15ef8) - m15ee8) * 0x2c4) / 2) / (m15eec - m15ee8) + 0x162;
	if (aBar > 0x2c4)
		aBar = 0x2c4;
	if (aBar < 0)
		aBar = 0;

	if (mState != STATE_POST_WARP)
	{
		bool aMoved = false;
		for (int i = 0; i < 2; i++)
		{
			if (m1994 < aBar)
			{
				aMoved = true;
				m1994 += (aBar - m1994) / 0x78 + 1;
				MarkDirty();
			}
			else if (aBar < m1994)
			{
				m1994 -= (m1994 - aBar) / 0x78 + 1;
				MarkDirty();
			}
		}

		int aFrame;
		if (!mIsAction)
			aFrame = 0x17;
		else
		{
			aFrame = (m1994 * 0x18 - 1) / 0x2c4;
			double aFraction = (double)m1994 / 708.0;
			if (mState != STATE_GAME_OVER_FALL && mState != 7)
			{
				if (!aMoved)
				{
					// the warning beep of the timer: faster the lower the bar is
					int aInterval = (int)(aFraction * 1600.0);
					if (aInterval < 0x50)
						aInterval = 0x50;
					if (aInterval < 0xf1 && aInterval <= mUpdateCnt - m15f20)
					{
						mApp->PlaySample(SOUND_WARNING);
						m15f20 = mUpdateCnt;
					}
				}
				if (0.15 <= aFraction)
					m15f18 = false;
				else
					m15f18 = ((mUpdateCnt / 8) % 2) == 0;
			}
		}
		if (mState == STATE_WHIRLPOOL && ((mUpdateCnt / 8) % 2) == 0)
			aFrame = 0x18;
		if (aFrame != m15f1c)
		{
			m15f1c = aFrame;
			MarkDirty();
		}
	}

	// (the registration nag of the trial, 0x95 updates in, is not ported)

	if (--m1768 == 0)
		mEffectOverlay->DoStrechyText(StrFormat("LEVEL %d", mNextLevel), 0x50);
	if (--m1764 == 0)
	{
		if (mScreenState != SCREEN_SLIDING_OUT)
			mApp->PlaySample(SOUND_GO);
		if (m190c == 0)
		{
			if (mNextLevel == 1)
				mEffectOverlay->DoStrechyText("GO!", 0);
		}
		else
			ShowPlanetCount();
	}

	// a hint after 10 seconds without a move (20 in the timed games)
	if (mState == STATE_IDLE)
		mHintCountdown++;
	if (((mHintCountdown > 1000 && mIsAction) || mHintCountdown > 0x5dc) && mState == STATE_IDLE && !mIsPuzzle)
	{
		m15ea8 = false;
		TryShowHint();
		mHintCountdown = 0;
	}

	// the score shown runs after the score
	{
		int aScore = mScore;
		if (m15e40 < aScore)
		{
			int aStep = (int)((((double)(aScore - m15e40) / mLevelValue) / 25.0 + 1.0) * mLevelValue);
			m15e40 += aStep;
			if (aScore < m15e40)
				m15e40 = aScore;
		}
		if (aScore < m15e40)
		{
			int aStep = (int)((((double)(m15e40 - aScore) / mLevelValue) / 25.0 + 1.0) * mLevelValue);
			m15e40 -= aStep;
			if (m15e40 < aScore)
				m15e40 = aScore;
		}
	}

	if (mSelectedPiece != NULL && mUpdateCnt % 8 == 0)
	{
		MarkDirty();
		m195c++;
	}

	// the gems
	bool anyFlame = false;
	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece == NULL)
				continue;
			bool aRemove = false;
			if (aPiece->mBC < 1)
			{
				if (aPiece->mB8 == 1)
				{
					// a gem that appears: its fade-in
					aPiece->mC0 = (float)((double)aPiece->mC0 + 0.0175);
					if (1.0 < aPiece->mC0)
						aRemove = true;
				}
				else if (aPiece->mB8 == 2)
				{
					m17d8 = (int)aPiece->mX;
					m17dc = (int)aPiece->mY;
				}
			}
			else
				aPiece->mBC--;

			if (aPiece->m70)
				anyFlame = true;

			if (aPiece->mType == 8)
			{
				// the bomb gem shakes, wildly in its last move
				if (aPiece->m58 == 1)
				{
					aPiece->m68 = CrtRand() % 5 - 2;
					aPiece->m6C = CrtRand() % 3 - 1;
					MarkDirty();
				}
				else if (aPiece->m58 == 0)
				{
					aPiece->m68 = CrtRand() % 9 - 4;
					aPiece->m6C = CrtRand() % 9 - 4;
					MarkDirty();
				}
			}
			else if (aPiece->mType == 9)
				MarkDirty();

			if (aPiece->mHintTime > 0)
			{
				aPiece->mHintTime--;
				if (aPiece->mHintTime % 5 == 0)
				{
					aPiece->mHintFrame++;
					MarkDirty();
				}
			}

			if (mUpdateCnt % 3 == 0 && (aPiece->mA4 != 0 || (aPiece == mSelectedPiece && aPiece->mType != 8)))
			{
				aPiece->mA4 = (aPiece->mA4 + 1) % 0x14;
				MarkDirty();
			}
			else
			{
				// the glow of the gem under the mouse
				if (aPiece->m8C)
				{
					aPiece->m90 = aPiece->m90 - 0.012;
					if (aPiece->m90 < 0.0)
					{
						aPiece->m8C = false;
						MarkDirty();
					}
				}
				if (mState == STATE_IDLE && aPiece != mSelectedPiece &&
					mApp->ScaleNum(aPiece->mX, 0.0f) <= (double)mMouseX &&
					mApp->ScaleNum(aPiece->mY, 0.0f) <= (double)mMouseY &&
					(double)mMouseX < mApp->ScaleNum((float)(aPiece->mX + 84.0), 0.0f) &&
					(double)mMouseY < mApp->ScaleNum((float)(aPiece->mY + 84.0), 0.0f))
				{
					double aGlow = aPiece->m90 + 0.045;
					aPiece->m90 = aGlow;
					if (1.0 < aGlow)
						aPiece->m90 = 1.0;
					if (!aPiece->m8C)
					{
						aPiece->m8C = true;
						aPiece->m98 = 0.0f;
					}
				}
				if (aPiece->m8C)
				{
					// the frame of the glow's animation, 0 to 10
					aPiece->m98 = (float)((double)aPiece->m98 + 0.0625);
					if (10.0f <= aPiece->m98)
						aPiece->m98 = (float)((double)aPiece->m98 - 10.0);
					MarkDirty();
				}
			}

			if (aRemove)
			{
				if (mSelectedPiece == aPiece)
					mSelectedPiece = NULL;
				delete aPiece;
				mBoard[aRow][aCol] = NULL;
			}
		}
	}
	if (anyFlame)
		UpdateStarRots();

	// the score texts rise, grow and fade
	for (size_t i = 0; i < mPoints.size(); )
	{
		Points* aPoints = mPoints[i];
		if (mUpdateCnt % 3 == 0)
		{
			MarkDirty();
			int aY = aPoints->mY;
			if (FONT_DIGIT->GetAscent() / 2 + 2 < aY)
				aPoints->mY = aY - 1;
		}
		float aDelta = (float)((double)aPoints->mTargetScale - (double)aPoints->mScale);
		aDelta = (float)((double)aPoints->mRise * (double)aDelta + (double)aPoints->mVelocity);
		aDelta = (float)((double)aDelta * (double)aPoints->mFade);
		aPoints->mVelocity = aDelta;
		aPoints->mScale = (float)((double)aPoints->mScale + (double)aDelta);
		MarkDirty();
		aPoints->mLife--;
		if (aPoints->mLife < 1)
		{
			delete aPoints;
			mPoints.erase(mPoints.begin() + i);
			MarkDirty();
		}
		else
			i++;
	}

	// the level is done
	if (!mIsPuzzle && m15eec - m15ee8 <= (m15e40 - (int)m15ef8) - m15ee8)
	{
		if (mState == STATE_IDLE)
			mState = STATE_LEVEL_COMPLETE_WAIT;
		else if (mState == STATE_LEVEL_COMPLETE_WAIT && m1602c == 0)
		{
			mApp->PlaySample(SOUND_LEVEL_COMPELTE);
			mState = 9;
		}
	}
	if (mState == 9 && mScore <= m15e40)
		DoWhirlpool();

	// the timed games end when the bar is empty
	if (mIsAction && mState == STATE_IDLE && (m15e40 - (int)m15ef8) - m15ee8 <= m15ee8 - m15eec)
	{
		m15f18 = true;
		mEffectOverlay->DoStrechyText("TIME UP!", 0x50);
		mApp->PlaySample(SOUND_TIME_UP);
		GameOver(200);
	}

	if (mState == STATE_NEW || mState == STATE_IDLE)
	{
		mFadeIn = (float)((double)mFadeIn + 0.025);
		if (1.0 < mFadeIn)
			mFadeIn = 1.0f;
	}

	// the work of the state
	int aStateNow = mState;
	if (aStateNow < 0xc)
	{
		if (aStateNow == 0xb)
			UpdateLevelTransition();
		else if (aStateNow == STATE_NEW)
			UpdateFalling();
		else if (aStateNow > 1)
		{
			if (aStateNow < 5)
				UpdateSwapping();
			else if (aStateNow == 5)
				UpdateClearing();
			else if (aStateNow == 6)
				UpdateGameOverAnim();
			else if (aStateNow == 10)
				UpdateWhirlpool();
		}
	}
	else if (aStateNow == 0xc)
		UpdateBoardDropping();
	else if (aStateNow == 0xd)
	{
		m190c = 0;
		m15e04 = (float)((double)m15e04 + 0.01);
		if (1.0 < m15e04)
		{
			if (!mIsPuzzle || mIsSecret)
				mEffectOverlay->DoStrechyText(StrFormat("LEVEL %d", mNextLevel), 0x50);
			else if (mPuzzleOverlay->mSolvedCount == 0)
			{
				if (mPuzzleOverlay->mSelectedPod == -1)
					m190c = 1;
				else
					ShowPlanetCount();
			}
			EndInterfaceRestore();
		}
		MarkDirty();
	}
	else if (aStateNow == 0x13)
	{
		float anAlpha = (float)((double)mEffectOverlay->mFade - 0.025);
		if (anAlpha <= 0.0f)
			anAlpha = 0.0f;
		mEffectOverlay->mFade = anAlpha;
	}
	else if (aStateNow == 0x15 || aStateNow == 0x17)
	{
		// the map of the galaxies slides in or out
		if (!mGalaxySlide.IncInVal(0.01))
		{
			if (mState == 0x17)
			{
				PuzzleProgress* aProgress = mApp->mProfile->GetPuzzleProgress();
				int aSelected = mGalaxyMap->mSelectedPuzzle;
				int aCurrent = aProgress->mPuzzleNum;
				if (aSelected == aCurrent)
				{
					mState = STATE_IDLE;
					mWidgetManager->RemoveWidget(mGalaxyMap);
				}
				else if (aSelected / 5 == aCurrent / 5)
					SelectPuzzlePod(aSelected % 5);
				else
				{
					DoWhirlpool();
					aProgress->mPuzzleNum = mGalaxyMap->mSelectedPuzzle;
					m15f6c = mGalaxyMap->mSelectedPuzzle / 5;
					StartPrepareLevelImages();
				}
			}
			else
				mState = 0x16;
		}
		mGalaxyMap->Resize(0, (int)mApp->ScaleNum(mGalaxySlide.GetOutVal(), 0.0), mWidth, mHeight);
	}

	if (mState == 0xe || !mLightning.empty())
		UpdateLightning();
	UpdateEffects();
	UpdateHyperWarpPoints();
	MarkDirty();

	if (m17d4 > 0)
	{
		m17d4--;
		if (m17d4 == 0)
			m17d0 = true;
	}
	else if (m17d0)
	{
		m17e0 = (float)((double)m17e0 + 0.012);
		if (1.0 < m17e0)
			m17d0 = false;
	}

	// the shaking of the board after a blast
	if (mShakeTime <= 0.0f)
	{
		mShakeX = 0;
		mShakeY = 0;
	}
	else if (mUpdateCnt % 3 == 0)
	{
		mShakeTime = (float)((double)mShakeTime - 0.1);
		if (mShakeTime < 0.0f)
			mShakeTime = 0.0f;
		int aDraw = CrtRand() % 2000 - 1000;
		mShakeX = (int)((double)mShakeSize * (double)mShakeTime * ((double)aDraw / 1000.0));
		aDraw = CrtRand() % 2000 - 1000;
		mShakeY = (int)((double)mShakeSize * (double)mShakeTime * ((double)aDraw / 1000.0));
	}

	if (m1850)
		m1858 = m1858 + 0.1;

	// the shooting star behind the board
	if (!m1848)
	{
		if (Rand() % 6000 == 0)
		{
			m1848 = true;
			m184c = 0.0f;
		}
	}
	else
	{
		m184c = (float)((double)m184c + 0.02);
		if (1.0 < m184c)
			m1848 = false;
		MarkDirty();
	}

	// the cheers for a run of chains in one move
	if (m1602c < 1)
	{
		int aChain = m16028;
		if (m16024 < aChain)
		{
			m16024 = aChain;
			if (aChain == 1)
			{
				if (mUpdateCnt - m16030 > 11999)
				{
					mApp->PlaySample(SOUND_GOOD);
					m1602c = 100;
					m16030 = mUpdateCnt;
				}
			}
			else if (aChain == 2)
			{
				if (!mIsAction)
					mEffectOverlay->DoStrechyText("EXCELLENT", 0x50);
				mApp->PlaySample(SOUND_EXCELLENT);
				m1602c = 0x78;
			}
			else
			{
				if (!mIsAction)
					mEffectOverlay->DoStrechyText("INCREDIBLE", 0x50);
				mApp->PlaySample(SOUND_INCREDIBLE);
				m1602c = 0x96;
			}
		}
	}
	else
		m1602c--;
	if (m16028 == m16024 && mState == STATE_IDLE)
	{
		m16028 = 0;
		m16024 = 0;
	}

	// the buttons of the puzzle fade out while a pod is being picked, and in again afterwards
	if (mPendingPod == -1)
	{
		if (mTutorialButton1 != NULL)
		{
			float anAlpha = (float)((double)mTutorialButton1->mAlpha + 0.05);
			if (1.0f <= anAlpha)
				anAlpha = 1.0f;
			mTutorialButton1->mAlpha = anAlpha;
		}
		if (mTutorialButton2 != NULL)
		{
			float anAlpha = (float)((double)mTutorialButton2->mAlpha + 0.05);
			if (1.0f <= anAlpha)
				anAlpha = 1.0f;
			mTutorialButton2->mAlpha = anAlpha;
		}
		float aFade = (float)((double)m15fac + 0.05);
		if (1.0f <= aFade)
			aFade = 1.0f;
		m15fac = aFade;
	}
	else
	{
		float aFade = (float)((double)m15fac - 0.05);
		m15fac = aFade;
		if (aFade <= 0.0f)
		{
			m15fac = 0.0f;
			SelectPuzzlePod(mPendingPod);
		}
		if (mTutorialButton1 != NULL)
			mTutorialButton1->mAlpha = m15fac;
		if (mTutorialButton2 != NULL)
			mTutorialButton2->mAlpha = m15fac;
	}

	// the box that explains the hints after the puzzle was set back
	if (m1910 > 0)
	{
		m1910--;
		if (m1910 == 0)
		{
			Dialog* aDialog = mApp->DoDialog(0x19, true, "More Hints?",
				"One or more moves have been undone to make the puzzle solvable again.\n\n"
				"Press HINT again if you want help on a move to make from here.", "", 1);
			mApp->SetupDialog(aDialog, 500);
			aDialog->Move(mApp->ScaleNum(0x20, 0) + aDialog->mX, aDialog->mY);
			aDialog->mYesButton->mLabel = "Don't Show Again";
			aDialog->mNoButton->mLabel = "Continue";
		}
	}

	// the pointer of the tutorial fades out when a move was made and in again when the board is quiet
	{
		float aFade = (float)((double)m1944 - 0.06);
		if (aFade <= 0.0f)
			aFade = 0.0f;
		m1944 = aFade;
		if (m1944 <= 0.0f && mState == STATE_IDLE)
		{
			float aNext = 1.0f;
			if ((double)m1948 + 0.06 < 1.0)
				aNext = (float)((double)m1948 + 0.06);
			m1948 = aNext;
		}
	}

	// the sweep of the endless mode: sparkles on the ring that shows the level, shards swirling around the arms
	if (mIsEndless && mState != STATE_POST_WARP && mScreenState == SCREEN_SHOWN)
	{
		if (mSweepTimer > 0)
		{
			if (Rand() % 500 - 0x14 < mSweepTimer)
			{
				int aOffX = (mIsPuzzle && !mIsSecret) ? 0 : (0x16 - (mIsEndless ? 7 : 0));
				int aOffY = (mIsPuzzle && !mIsSecret) ? 0x1b : (0x2a - (mIsEndless ? 7 : 0));
				if (mSweepB != -1)
				{
					MemoryImage* anImage = (MemoryImage*)GetImageById(mSweepB + 0xa2);
					AddAlphaSparkle(gSweepSparkleA[mSweepB][0] + aOffX, gSweepSparkleA[mSweepB][1] + aOffY, anImage, 0, 0);
				}
				if (mSweepA != -1)
				{
					MemoryImage* anImage = (MemoryImage*)GetImageById(mSweepA + 0xa9);
					AddAlphaSparkle(gSweepSparkleB[mSweepA][0] + aOffX, gSweepSparkleB[mSweepA][1] + aOffY, anImage,
						anImage->mWidth / 9, 0);
				}
			}
			mSweepTimer--;
		}

		for (int i = 0; i < mSweep0; i++)
		{
			if (Rand() % 100 < 13)
			{
				int aStartX = gSweepStart[i][0];
				int aStartY = gSweepStart[i][1];
				Shard* aShard = new Shard();
				aShard->mImageOffset = Rand() % 3;
				aShard->mKind = 3;
				aShard->mX = (float)((double)GetRandFloat() * 7.0 + (double)(float)aStartX);
				int aSign = (Rand() % 2 != 0) ? -1 : 1;
				aShard->mY = (float)((double)aStartY - (double)aSign * 12.0);
				float anOffset = (float)((Rand() % 6) * aSign);
				aShard->mY = anOffset + aShard->mY;
				aShard->mY2 = anOffset + aShard->mY2;
				aShard->mCenterX = (float)aStartX;
				aShard->mCenterY = (float)aStartY;
				aShard->mAngle = gSweepAngle[i];
				aShard->mBaseX = aShard->mX;
				aShard->mVY = (float)((((double)(Rand() % 1000) / 1000.0) * 0.02 + 0.05) * (double)aSign);
				aShard->mGravity = (float)((((double)(Rand() % 1000) / 1000.0) * 0.002 + 0.001) * (double)aSign);
				aShard->mFrequency = (float)(((double)(Rand() % 1000) / 1000.0) * 0.1 + 0.1);
				double aFirst = (double)(Rand() % 1000) / 1000.0;
				float aSecond = (float)(Rand() % 1000) / 1000.0f;
				aShard->mAmplitude = (float)((double)aSecond * (double)(float)aFirst * 3.2);
				aShard->mLimit = (float)(((double)(Rand() % 1000) / 1000.0) * 10.0 + 25.0);
				mShards.push_back(aShard);
			}
		}
	}

	// the stars of the hyperspace fly down the screen
	if (m165ad && Rand() % 2 == 0)
	{
		Shard* aShard = new Shard();
		aShard->mKind = 5;
		aShard->mX = (float)(Rand() % 1024);
		aShard->mY = -2.0f;
		aShard->mGravity = 0.0f;
		aShard->mVY = (float)((double)GetRandFloat() * 0.25 + 0.5);
		mShards.push_back(aShard);
	}
	if (m165bc != NULL)
		m165bc->Update();

	// the end screens (the galaxy's end, the secret mode's unlock)
	if (mState == 0x12 || mState == 0x13)
	{
		if (mEffectOverlay->mEndTimer == 0x6a4)
		{
			mBackLink = new HyperlinkWidget(500, this);
			mBackLink->SetFont(FONT_CONTINUUM_MED_14);
			mBackLink->mLabel = "Back to Main Menu";
			mBackLink->mUnderlineOffset = 0;
			mWidgetManager->AddWidget(mBackLink);
		}
		if (mEffectOverlay->mEndTimer == 0x2134)
			UpdateSecretUnlocked();
	}

	// the sound of a bomb gem, once per update
	if (m15e78)
	{
		SoundInstance* anInstance = mApp->mSoundManager->GetSoundInstance(SOUND_BOMB_EXPLODE);
		if (anInstance != NULL)
			anInstance->Play(false, true);
		m15e78 = false;
	}
}
