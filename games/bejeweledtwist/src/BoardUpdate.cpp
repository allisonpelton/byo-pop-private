// Board's updates (Board.h): Update (0x6d6899), which runs the game's updates at its speed, and UpdateGame
// (0x6d090f), one update of the game.
#include "Board.h"

#include "AwardTelly.h"
#include "Challenge.h"
#include "GameText.h"
#include "Compat2004.h"
#include "CustomBassMusicInterface.h"
#include "EffectsManager.h"
#include "ElectroStrike.h"
#include "FlyingText.h"
#include "GemSoul.h"
#include "GameOverShow.h"
#include "GoodTube.h"
#include "Hyperspace.h"
#include "ChallengeMenu.h"
#include "Menu.h"
#include "LevelUpShow.h"
#include "GraphicsRecorder.h"
#include "MusicInterface.h"
#include "Piece.h"
#include "Points.h"
#include "Shockwave.h"
#include "Traveller.h"
#include "WheelSpin.h"
#include "Profile.h"
#include "RankBonusWidget.h"
#include "Ranks.h"
#include "ReplayWidget.h"
#include "Res.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "TwistDialog.h"
#include "TwistUtil.h"
#include "UnderDialogWidget.h"
#include "VizManager.h"
#include "WinTwistApp.h"
#include "WidgetManager.h"
#include "X86Math.h"

#include <cmath>

using namespace Sexy;

namespace {

// 0x84e91c
const float kTwoPi = 6.2831855f;

}  // namespace

int Board::GetBoardWidth()
{
	return gBoard->mCols * gGemWidth;
}

void Board::Resize(int theX, int theY, int theWidth, int theHeight)
{
	int anOldX = GetColX(0);
	int anOldY = GetRowY(0);
	TwistWidget::Resize(theX, theY, theWidth, theHeight);
	int aNewX = GetColX(0);
	int aNewY = GetRowY(0);
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece != nullptr) {
				aPiece->mX = (float)(aNewX - anOldX) + aPiece->mX;
				aPiece->mY = (float)(aNewY - anOldY) + aPiece->mY;
			}
		}
	}
	mPointsManager->Resize(0, 0, mWidth, mHeight);
	if (mShownDialog != nullptr)
		mShownDialog->Resize(0, 0, mWidth, mHeight);
}

void Board::Update()
{
	gGraphicsRecorder->mRecording = false;
	// The original counts the updates in 0x85af00, which nothing reads
	if (mCornerWidget != nullptr) {
		mCornerWidget->Resize(Ftol((double)gApp->mScale * 770.0), Ftol((double)gApp->mScale * 720.0),
			Ftol((double)gApp->mScale * 240.0), Ftol((double)gApp->mScale * 32.0));
	}
	// 0x85c4fc: the speed the game runs at, easing to aTarget (shared by every board, set by the first)
	static bool sSpeedSet = false;
	static float sSpeed = 0;
	if (!sSpeedSet) {
		sSpeed = mSpeedTarget;
		sSpeedSet = true;
	}
	float aTarget = mSpeedTarget;
	float anEase = 0.95f;
	if (mReplayPlaying) {
		if (mRecordedFrames < 10) {
			aTarget = 0.3f;
			sSpeed = 0.3f;
		} else {
			aTarget = 0.85f;
		}
		if (mIsReplay && mRecordedFrames < 3) {
			aTarget = 0.03f;
			sSpeed = 0.03f;
		}
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece != nullptr && aPiece->mBreaking && aPiece->mFlags == 0 && 0.35f <= aTarget)
					aTarget = 0.35f;
			}
		}
		anEase = 0.98f;
	}
	if (0 < mSlowDownWait) {
		mSlowDownWait--;
		if (mSlowDownWait < 30)
			aTarget = 0.4f;
	}
	if (mGeodeFlash != 0.0f) {
		float aLeft = (float)((double)mGeodeFlash - (double)0.005f);
		mGeodeFlash = aLeft;
		if (aLeft <= 0.0f) {
			mGeodeFlash = 0;
		} else {
			double aSlow = (1.0 - (double)aLeft) * 0.6 + 0.4;
			if (aSlow <= (double)aTarget)
				aTarget = (float)aSlow;
		}
		if (!mReplayPlaying)
			anEase = 0.85f;
	}
	sSpeed = (float)((double)sSpeed * (double)anEase + (1.0 - (double)anEase) * (double)aTarget);
	if (std::fabs(sSpeed - aTarget) < 0.01)
		sSpeed = aTarget;
	if (mFruitDance != nullptr) {
		// The fruit dance (2200 updates) stops the game and the background behind it, easing in over its first
		// hundred updates and out over its last, and gives a fruit (SettleTurn) near its end
		int aDance = mFruitDanceUpdates;
		float aFromStart = (float)((double)aDance * 0.009999999776482582);
		float aFromEnd = (float)((double)(2200 - aDance) * 0.009999999776482582);
		float aNear = aFromEnd <= aFromStart ? aFromEnd : aFromStart;
		if (1.0f < aNear)
			aNear = 1.0f;
		double aSlow = 1.0 - (double)aNear;
		mApp->mVizManager->mSpeed = aSlow <= 1.0 ? aSlow : 1.0;
		sSpeed = (float)(aSlow <= 1.0 ? aSlow : 1.0);
		if (aDance == 2190)
			mFruitDue = true;
		if (!mFruitDance->mPaused)
			mFruitDanceUpdates = aDance + 1;
	}
	float anAccum = sSpeed + mSpeedAccum;
	mSpeedAccum = anAccum;
	if (mSpeedTarget != mSpeedNow) {
		float aSpeed;
		if (mSpeedTarget <= mSpeedNow) {
			aSpeed = (float)((double)mSpeedNow - (double)0.02f);
			if (aSpeed < mSpeedTarget)
				aSpeed = mSpeedTarget;
		} else {
			aSpeed = (float)((double)mSpeedNow + (double)0.02f);
			if (mSpeedTarget < aSpeed)
				aSpeed = mSpeedTarget;
		}
		mSpeedTarget = aSpeed;
	}
	int aCount = Ftol((double)anAccum);
	mSpeedAccum = anAccum - (float)aCount;
	if (mIsReplay && !gApp->Is3DAccelerated()) {
		if (!mReplayWaiting) {
			float aFade = (float)((double)mReplayStaticFade + (double)0.01f);
			if (1.0f < aFade)
				aFade = 1.0f;
			mReplayStaticFade = aFade;
			if (aFade < 1.0f)
				return;
		} else if (mReplaySlideWait < 1) {
			mGoodTube->mX -= 20;
			mBoardX += Ftol((double)gApp->mScale * 20.0);
			if (mWidth < mBoardX) {
				float aFade = (float)((double)mApp->mBoard->mReplayStaticFade - (double)0.02f);
				if (aFade < 0.0f)
					aFade = 0.0f;
				mReplayStaticFade = aFade;
			}
			SnapPiecesX();
		} else {
			mReplaySlideWait--;
		}
	}
	bool aOnce = false;
	if (mShownBeforeStart && mIsReplay) {
		aOnce = true;
		aCount = 1;
		mRecordedFrames = 0;
	}
	for (int i = aCount; 0 < i; i--)
		UpdateGame();
	if (aOnce)
		mRecordedFrames = 0;

	// The level shown rolls to the level
	unsigned int aLevel = (unsigned int)mScore;
	if ((mPendingDialogs.empty() || (mShownDialog != nullptr && !mShownDialog->mHoldsBoard)) && !IsBusy()) {
		unsigned int aShown = (unsigned int)mShownScore;
		if (aShown < aLevel) {
			unsigned int aDiv = (unsigned int)mStepLevel;
			double aSteps = (double)((aLevel - aShown) / aDiv);
			mShownScore += Ftol((aSteps / 25.0 + 1.0) * (double)(int)aDiv);
			if (aLevel <= (unsigned int)mShownScore)
				mShownScore = (int)aLevel;
			mScoreRollGlow = 1.0f;
		}
		if (aLevel < (unsigned int)mShownScore) {
			unsigned int aDiv = (unsigned int)mStepLevel;
			double aSteps = (double)(((unsigned int)mShownScore - aLevel) / aDiv);
			mShownScore -= Ftol((aSteps / 25.0 + 1.0) * (double)(int)aDiv);
			if ((unsigned int)mShownScore < aLevel)
				mShownScore = (int)aLevel;
			mScoreRollGlow = 1.0f;
		}
	}
	if (-1 < mPendingPraise && !IsBusy() && !mLevelDone) {
		// the praise waiting for the app's voice: the voice fades out first
		bool aShow = true;
		if (mApp->mUnmanagedSound != nullptr) {
			double aVolume = mApp->mUnmanagedSound->GetVolume();
			if (0 < aVolume - 0.05) {
				mApp->mUnmanagedSound->SetVolume(aVolume - 0.05);
				aShow = false;
			} else {
				mApp->mUnmanagedSound->Stop();
				mApp->mUnmanagedSound->Release();
				mApp->mUnmanagedSound = nullptr;
			}
		}
		if (aShow) {
			ShowPraise(mPendingPraise);
			mPendingPraise = -1;
		}
	}
	if (mTutorial == nullptr || mTutorial->mDialog == nullptr) {
		if (0 < mZenJumpWait) {
			mZenJumpWait--;
			if (mZenJumpWait == 0)
				ShowZenJump(1);
		}
		if (mApp->mOptionsDialog == nullptr && mApp->mDialogList.size() == 0 && mEndPending == 0 &&
			0 < mReturnDialogWait) {
			mReturnDialogWait--;
			if (mReturnDialogWait == 0) {
				// After the zen jump (Mandala sets 400): the "returning to game" dialog, which says the old game waits in
				// the main menu (0x6d6fa2)
				SexyString aFooter = GT(0x80d9a4);
				SexyString aLines = mApp->GetString(GTS(0x7df400), GT(0x7df358));
				SexyString aHeader = mApp->GetString(GTS(0x7df344), GT(0x7df320));
				Dialog* aDialog = mApp->DoDialog(-1, true, aHeader, aLines, aFooter, TwistDialog::BUTTON_MODE_YES);
				if (mApp->mUnderDialog != nullptr)
					mWidgetManager->PutBehind(mApp->mUnderDialog, aDialog);
			}
		}
	}
	if (mFruitDance != nullptr) {
		// The fruit dance and its music stop while the game is paused
		mFruitDance->mPaused = mApp->mOptionsDialog != nullptr || 0 < mPauseShownCount || mFrozen || 0 < mPauseCount;
		// port: the original has no music interface or track to check for (it reads them without a check); the
		// port has none without sound
		if (mApp->mTwistMusic != nullptr) {
			CustomBassMusicInterface::Track* aTrack = mApp->mTwistMusic->FindTrack(0x133);
			if (aTrack != nullptr) {
				if (mFruitDance->mPaused)
					mApp->mTwistMusic->PauseMusic(aTrack->mSongId);
				else
					mApp->mTwistMusic->ResumeMusic(aTrack->mSongId);
			}
		}
	}
}

void Board::SnapPiecesX()
{
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece != nullptr)
				aPiece->mX = (float)GetColX(aCol);
		}
	}
	for (unsigned int i = 0; i < mTwistPieces.size(); i++) {
		Piece* aPiece = mTwistPieces[i];
		if (aPiece != nullptr)
			aPiece->mX = (float)GetColX(aPiece->mCol);
	}
}

int Board::GetRowAt(int theY)
{
	int aRows = mRows;
	for (int aRow = 0; aRow < aRows; aRow++) {
		int aY = GetRowY(aRow);
		if (aY <= theY && theY < gGemHeight + aY)
			return aRow;
	}
	return -1;
}

int Board::GetColAt(int theX)
{
	int aCols = mCols;
	for (int aCol = 0; aCol < aCols; aCol++) {
		int aX = GetColX(aCol);
		if (aX <= theX && theX < gGemWidth + aX)
			return aCol;
	}
	return -1;
}

void Board::SetCursor(int theRow, int theCol)
{
	int aX = GetColX(theCol);
	int aY = GetRowY(theRow);
	int aSize = gGemWidth * 2;
	mCursorX = aX;
	mCursorY = aY;
	mCursorWidth = aSize;
	mCursorHeight = aSize;
}

void Board::StartTubeSlide()
{
	mTubeSlide.SetCurve(GTS(0x7dcd08));
}

void Board::NextLevelMusic()
{
	if (mApp->mNumIngameTracks != 0) {
		mMusicTrack++;
		mMusicTrack = mMusicTrack % mApp->mNumIngameTracks;
		mApp->PlayMusic(mMusicTrack + 100, true, true, 0, 0);
	}
}

void Board::AddStat(int theStat, int theDelta)
{
	mGameStats[theStat] += theDelta;
	if (!mIsReplay)
		mApp->mStats[theStat] += theDelta;
}

void Board::EaseCursor()
{
	if (mCursorEaseProgress < 1.0f) {
		float aSpeed = (float)((1.0 - (double)mCursorEaseProgress) * ((double)mCursorEaseProgress + (double)mCursorEaseProgress));
		float aDX = (float)((double)mCursorX - (double)mEasedCursorX);
		float aDY = (float)((double)mCursorY - (double)mEasedCursorY);
		float aDistSq = (float)((double)aDX * (double)aDX + (double)aDY * (double)aDY);
		float aDist = sqrtf(aDistSq);
		float aStep = (float)(((double)aDist * (double)aSpeed + (double)mCursorEaseSpeed) * 0.55);
		mCursorEaseSpeed = aStep;
		if (1.0f < aDist) {
			mEasedCursorX = (float)((double)aStep * (double)aDX / (double)aDist + (double)mEasedCursorX);
			mEasedCursorY = (float)((double)aStep * (double)aDY / (double)aDist + (double)mEasedCursorY);
			return;
		}
	}
	mEasedCursorX = (float)mCursorX;
	mEasedCursorY = (float)mCursorY;
	mCursorEaseSpeed = 0;
}

void Board::StartGo()
{
	mGoScale.SetCurve(GTS(0x7dcd28));
	if (mGameMode == MODE_CHALLENGE)
		PlayChallengeMusic();
}

void Board::PlayChallengeMusic()
{
	if (mChallenge == nullptr)
		return;
	unsigned int aFlags = mChallenge->mFlags;
	if ((aFlags & Challenge::FLAG_TIME) == 0) {
		int aTrack = 306 - ((aFlags & Challenge::FLAG_MOVES) != 0);
		mApp->PlayMusic(aTrack, true, true, 0, 0);
		if (aTrack == 305)
			UpdateTurnBasedMusic();
		return;
	}
	if (mApp->mMusicInterface != nullptr)
		mApp->mMusicInterface->StopAllMusic();
	mApp->PlayMusic((mChallenge->mTimeGoal == 180) * 8 + 300, false, false, mMusicPositionLow, mMusicPositionHigh);
	mMusicPositionLow = 0;
	mMusicPositionHigh = 0;
}

void Board::UpdateTurnBasedMusic()
{
	// (the port's music calls go to mTwistMusic, which is there whenever the music is)
	if (mApp->mMusicInterface == nullptr || mApp->mTwistMusic == nullptr)
		return;
	CustomBassMusicInterface* aMusic = mApp->mTwistMusic;
	int aMoves = mApp->mStats[0x21];  // the moves made (+0xcf8)
	int aChannel = 6;
	do {
		aMusic->SetChannelVolume(aChannel, 0, false, 305);
		aChannel++;
	} while (aChannel <= 12);
	aMusic->SetChannelVolume(18, 0, false, 305);
	// the moves each channel waits for (the first one's 7, or 5 short of a Moves goal over 10), and its number + 1
	int aSteps[10][2] = {{7, 11}, {1, 18}, {2, 6}, {3, 7}, {4, 8}, {5, 9}, {6, 10}, {8, 12}, {9, 13}, {-1, -1}};
	if (mChallenge != nullptr && (mChallenge->mFlags & Challenge::FLAG_MOVES) != 0 && 10 < mChallenge->mMoveGoal)
		aSteps[0][0] = mChallenge->mMoveGoal - 5;
	int i = 0;
	int aStep = aSteps[0][0];
	if (-1 < aStep) {
		do {
			if (aMoves < aStep)
				aMusic->SetChannelVolume(aSteps[i][1] - 1, 0, true, 305);
			else
				aMusic->SetChannelVolume(aSteps[i][1] - 1, -1, false, 305);
			i++;
			aStep = aSteps[i][0];
		} while (-1 < aStep);
	}
	int aTempo = mApp->GetInteger("MUSIC_CHALLENGE_TURNBASED_MIN_TEMPO") + aMoves;
	if (mApp->GetInteger("MUSIC_CHALLENGE_TURNBASED_MAX_TEMPO") < aTempo)
		aTempo = mApp->GetInteger("MUSIC_CHALLENGE_TURNBASED_MAX_TEMPO");
	else
		aTempo = mApp->GetInteger("MUSIC_CHALLENGE_TURNBASED_MIN_TEMPO") + aMoves;
	aMusic->SetTrackTempo(0, aTempo);
}

// part of EndTurn (0x6be49c) and UpdateGame (0x6d090f), which both count the counter gems down
bool Board::CountDownCounters()
{
	Piece* aLowest = nullptr;
	Piece* aWarn = nullptr;
	int aWarnCount = 100;
	mLowestBombCount = 100;
	unsigned int aLeft = (unsigned int)(mCols * mRows);
	Piece** aCell = mGrid;
	if (aLeft != 0) {
		do {
			Piece* aPiece = *aCell;
			aCell++;
			if (aPiece != nullptr && (aPiece->IsFlagSet(Piece::FLAG_BOMB) || aPiece->IsFlagSet(Piece::FLAG_DEVIL))) {
				if ((aPiece->IsFlagSet(Piece::FLAG_BOMB) || !mTurnStartedWithMatch) && aPiece->mBombCount < aWarnCount) {
					aWarn = aPiece;
					aWarnCount = aPiece->mBombCount;
				}
				if (aPiece->mBombCount < mLowestBombCount) {
					mLowestBombCount = aPiece->mBombCount;
					aLowest = aPiece;
				}
				if (aPiece->IsFlagSet(Piece::FLAG_DEVIL) && aPiece->mBombCount == mLowestBombCount)
					aLowest = aPiece;
				int aCount = aPiece->mBombCount;
				if (aCount < 4)
					aPiece->mJustMade = 4;
				else if (aCount < 7)
					aPiece->mJustMade = 3;
				else if (aCount < 11)
					aPiece->mJustMade = 2;
			}
			aLeft--;
		} while (aLeft != 0);
		if (aWarn != nullptr && aWarn->mBombCount < 6) {
			aWarn->mJustMade = 5;
			PlayPieceSound(aWarn, SOUND_COUNTER_WARNING, (float)((double)(5 - aWarn->mBombCount) - 3.0), 0, true);
		}
		if (aLowest != nullptr && mLowestBombCount == 0 && !mLevelDone) {
			// A counter is at zero: in a challenge, blitz or for a devil gem the game ends, else the wheel comes up
			if (!mIsReplay) {
				if (mGameMode != MODE_CHALLENGE && mGameMode != MODE_BLITZ && !aLowest->IsFlagSet(Piece::FLAG_DEVIL)) {
					SpinWheel(aLowest);
					return true;
				}
				// In a challenge whose goal is met a counter at zero changes nothing
				if (mGameMode == MODE_CHALLENGE && mChallenge != nullptr &&
					mApp->mChallengeInfo->Check(mChallenge, nullptr, false) == 1)
					return false;
				mTurnCount--;
				AddStat(0x21, -1);
				aLowest->mDevilGlow = 0;
				aLowest->mJustMade = 0;
				mEndingPiece = aLowest;
				mEndedByDevil = aLowest->IsFlagSet(Piece::FLAG_DEVIL);
				EndGame(true);
				return true;
			}
			if (mGameMode == MODE_CHALLENGE || mGameMode == MODE_BLITZ || aLowest->IsFlagSet(Piece::FLAG_DEVIL))
				mReplayMoveReady = true;
			else
				aLowest->mBusyTime = 1;
		}
	}
	return false;
}

// 0x6d090f (UpdateGame): the geodes from broken coal. Each waits its delay, then flies with the arc of its curve (slower
// near the sides), throws sparks, and when its flight ends it lands: sparks, its points to the score and its sound.
void Board::UpdateGeodes()
{
	static const float kSpinStep[8] = {0.2f, 0.12f, -0.2f, -0.12f, 1.0f, 0.3f, -1.0f, -0.3f};
	int aCount = (int)mGeodes.size();
	for (unsigned int i = 0; i < mGeodes.size(); i++) {
		Geode* aGeode = mGeodes[i];
		if (0 < aGeode->mDelay) {
			aGeode->mDelay--;
			continue;
		}
		if (((unsigned int)mSeenFlags >> 15 & 1) == 0) {
			mReplayPodDue = true;
			mApp->mProfile->SetTipSeen(0xf, true);
		}
		float aWide = (float)mWidth - aGeode->mX;
		float aSize = (float)((double)gApp->mScale * 250.0);
		float aEdge = 1.0f;
		float aNear = aWide < aGeode->mX ? aWide : aGeode->mX;
		if ((double)(aNear / aSize) <= 1.0)
			aEdge = aNear / aSize;
		if (!aGeode->mFlight.IncInVal())
			aGeode->mLife = 0;
		aGeode->mX = (float)(aGeode->mFlight.GetOutVal() * (double)aGeode->mDX * (double)aEdge + (double)aGeode->mX);
		aGeode->mY = (float)(aGeode->mFlight.GetOutVal() * (double)aGeode->mDY * (double)aEdge + (double)aGeode->mY);
		float aPull = (float)((double)gApp->mScale * 0.07500000298023224);
		aGeode->mDY = aPull + aGeode->mDY;
		double aLimit = 8.0 - (double)aCount * 0.5;
		for (int k = 0; (double)k < (1.0 <= aLimit ? aLimit : 1.0); k++) {
			Effect* aSpark = mFXManager->AllocEffect(0x28);
			float aRange = (float)((double)gApp->mScale * 10.0);
			aSpark->mX = (float)((double)RandSigned() * (double)aRange + (double)aGeode->mX);
			aRange = (float)((double)gApp->mScale * 10.0);
			aSpark->mY = (float)((double)RandSigned() * (double)aRange + (double)aGeode->mY);
			mFXManager->AddEffect(aSpark);
		}
		for (int j = 0; j < 8; j++)
			aGeode->mSpin[j] = (float)(aGeode->mArmTurn.GetOutVal() * (double)kSpinStep[j] + (double)aGeode->mSpin[j]);
		if (0.0f < aGeode->mLife)
			continue;
		// It lands: a burst of sparks of any hue
		int aBurst = (0x12 - aCount) * 5;
		if (aBurst < 10)
			aBurst = 10;
		for (int k = 0; k < aBurst; k++) {
			Effect* aDust = mFXManager->AllocEffect(0x11);
			aDust->mColor = Color(mApp->HSLToRGB((int)(Rand() % 256), 0xff, 0xdc));
			aDust->mX = (float)((double)RandSigned() * 2.0 + (double)aGeode->mX);
			aDust->mY = (float)((double)RandSigned() * 2.0 + (double)aGeode->mY);
			float aSpeed = (float)((double)RandSigned() * 15.5);
			float anAngle = (float)((double)RandSigned() * 3.1415927410125732);
			aDust->mDX = (float)((double)(aSpeed * cosf(anAngle)) + (double)aGeode->mDX * 0.0);
			aDust->mDY = (float)((double)(aSpeed * sinf(anAngle)) + (double)aGeode->mDY * 0.0);
			mFXManager->AddEffect(aDust);
		}
		float aPointsY = aGeode->mY <= 0.0f ? 0.0f : aGeode->mY;
		float aPointsX = 0.0f < aGeode->mX ? aGeode->mX : 0.0f;
		AddPoints(Ftol(aPointsX), Ftol(aPointsY), aGeode->mPoints, Color::White, -0x67, true, true, false, true);
		mGeodes.erase(mGeodes.begin() + i);
		float aPitch = (float)aGeode->mIndex;
		if (10.0f < aPitch)
			aPitch = 10.0f;
		mApp->PlaySoundAt(SOUND_GEODE, Ftol(aGeode->mX), aPitch, 0.0f);
		delete aGeode;  // the original leaves it
		i--;
	}
}

void Board::UpdateGame()
{
	mHintsReady = false;
	mCursorShown = false;
	if (mEndPending != 0) {
		if (mEndPending == 1) {
			// The game is over: space comes back over the board, and then the main menu (app +0xf20's state 12)
			mVisible = false;
			if (mApp->mHyperspace == nullptr)
				mApp->ShowHyperspace();
			Hyperspace* aSpace = mApp->mHyperspace;
			if (aSpace->mState == 12) {
				if (aSpace->mBoardBrightnessFinal.GetOutVal() == 0.0) {
					aSpace->mState = 0;
					if (mRankBonusWidget == nullptr)
						mGameOverShown = true;
				}
			} else {
				if (gApp->mLastGameMode == 2) {
					// the challenge menu comes back with the player's progress (app +0x864's +0x4a8, 0x6e2318(4, false))
					mApp->mMainMenu->mChallengeMenu->SetState(4, false);
				}
				aSpace->StartEndGame();
			}
			if (!mApp->mHyperspace->mDrawsOwnBackground)
				mVisible = true;
		}
		return;
	}
	if (mRankBonusWidget != nullptr && mRankBonusWidget->mClicked) {
		mApp->mWidgetManager->RemoveWidget(mRankBonusWidget);
		mApp->SafeDeleteWidget(mRankBonusWidget);
		mRankBonusWidget = nullptr;
		// the pod the level's end took down comes back
		if (mReplayPodDown)
			ShowReplayPod();
	}
	if (mLevelUpShow == nullptr || !mLevelUpShow->mClicked) {
		if (0.0f < mNextLevelWait) {
			float aLeft = (float)((double)mNextLevelWait - (double)0.01f);
			mNextLevelWait = aLeft;
			// The rank bar lets the level start once it waits (2) or goes (5)
			RankBonusWidget* aRankBonus = static_cast<RankBonusWidget*>(mRankBonusWidget);
			if (mShownDialog == nullptr &&
				(aRankBonus == nullptr || aRankBonus->mState == 2 || aRankBonus->mState == 5)) {
				if (aLeft <= 0.0f) {
					mApp->mVizManager->NextLook();
					mLevel++;
					NewLevel();
					mLevelSlideIn.SetCurve(GTS(0x7df2f0));
					mGoodTube->SetLevel(mLevel);
					StartTubeSlide();
					mNextLevelWait = 0;
					mSettleWait = 1.5f;
					SettleTurn();
					NextLevelMusic();
				}
			} else {
				double aNext = (double)aLeft - (double)0.01f;
				mNextLevelWait = aNext <= 2.0 ? 2.0f : (float)aNext;
			}
		}
	} else {
		mApp->mWidgetManager->RemoveWidget(mLevelUpShow);
		mApp->SafeDeleteWidget(mLevelUpShow);
		mLevelUpShow = nullptr;
	}
	mAngryBombFade.IncInVal();
	if (mGameMode == MODE_ZEN)
		mZenExtraCurve.IncInVal();
	TwistWidget* aDialog = mShownDialog;
	if (aDialog != nullptr && aDialog->mClicked) {
		if (aDialog == mTutorial) {
			CloseTip();
			ShowNextTip();
		} else if (aDialog == mGameOverShow) {
			mGameOverShow = nullptr;
		} else if (aDialog == mHeldDialog) {
			mHeldDialog = nullptr;
		}
		mApp->mWidgetManager->RemoveWidget(mShownDialog);
		mApp->SafeDeleteWidget(mShownDialog);
		mShownDialog = nullptr;
		mApp->mWidgetManager->SetFocus(this);
	}
	if ((mHeldDialog != nullptr && IsBusy()) || 0.0f < mNextLevelWait || mReplayPlaying) {
		if (0.0f < mSettleWait)
			mSettleWait = mSettleWait <= 0.1f ? 0.1f : mSettleWait;
		if (0.0f < mNextLevelWait && 0.0 < mLevelSlideIn.GetOutVal())
			mGoodTube->mVisible = false;
	}
	if (mShownDialog == nullptr) {
		if (mPendingDialogs.empty()) {
			if (mEndsWhenIdle)
				mGameOverShown = true;
		} else {
			mShownDialog = mPendingDialogs.front();
			mPendingDialogs.pop_front();
			mShownDialog->mZOrder = mZOrder + 1;
			mApp->mWidgetManager->AddWidget(mShownDialog);
		}
	} else if (mShownDialog->mHoldsBoard) {
		return;
	}
	// The first text no longer queued ends the ones before it
	bool anUpdated = false;
	for (int i = 0; i < (int)mFlyingTexts.size(); i++) {
		FlyingText* aText = mFlyingTexts[i];
		if (!aText->mQueued) {
			for (int j = 0; j < i; j++) {
				if (mFlyingTexts[j]->mSaid && mApp->mUnmanagedSound != nullptr) {
					mApp->mUnmanagedSound->Stop();
					mApp->mUnmanagedSound->Release();
					mApp->mUnmanagedSound = nullptr;
				}
				mFlyingTexts[j]->mClicked = true;
				mFlyingTexts[j]->mTextAlpha = 0;
			}
			anUpdated = true;
			aText->Update();
		}
		if (aText->mClicked) {
			delete aText;
			mFlyingTexts.erase(mFlyingTexts.begin() + i);
			i--;
		}
	}
	if (!mFlyingTexts.empty() && !anUpdated)
		mFlyingTexts[0]->mQueued = false;
	if (0 < mPauseCount || mApp->mOptionsDialog != nullptr || mGameOverShown)
		return;
	if (mMinuteWarningWait <= 0.0f) {
		if (0.0f < mMinuteWarningAlpha) {
			float aFade = (float)((double)mMinuteWarningAlpha - (double)0.02f);
			mMinuteWarningAlpha = aFade < 0.0f ? 0.0f : aFade;
		}
	} else {
		mMinuteWarningWait = (float)((double)mMinuteWarningWait - (double)0.01f);
		float aFade = (float)((double)mMinuteWarningAlpha + (double)0.02f);
		mMinuteWarningAlpha = 1.0f < aFade ? 1.0f : aFade;
	}
	if (0.0f < mMinuteWarningRed) {
		float aFade = (float)((double)mMinuteWarningRed - (double)0.01f);
		mMinuteWarningRed = aFade < 0.0f ? 0.0f : aFade;
	}
	if (!mCascadePopupScale.HasBeenTriggered()) {
		mCascadePopupScale.IncInVal();
	} else if (0.0f < mCascadePopupShadow) {
		float aFade = (float)((double)mCascadePopupShadow - (double)0.02f);
		mCascadePopupShadow = aFade < 0.0f ? 0.0f : aFade;
	}
	float aFade = (float)((double)mCascadePopupFade - (double)0.01f);
	mCascadePopupFade = aFade < 0.0f ? 0.0f : aFade;
	if (!mCountPopupScale.HasBeenTriggered()) {
		mCountPopupScale.IncInVal();
	} else if (0.0f < mCountPopupShadow) {
		float aFade2 = (float)((double)mCountPopupShadow - (double)0.02f);
		mCountPopupShadow = aFade2 < 0.0f ? 0.0f : aFade2;
	}
	aFade = (float)((double)mCountPopupFade - (double)0.01f);
	mCountPopupFade = aFade < 0.0f ? 0.0f : aFade;

	// The grid slides to its place: from the slide's start to the middle, or off to the right in a replay
	int aCentre = mIsReplay ? mWidth : mWidth / 2 - GetBoardWidth() / 2;
	int aFrom = aCentre;
	int aSlide = GetSlideX();
	int aTo = aSlide;
	if (!mLevelStarted) {
		aTo = aCentre;
		aFrom = aSlide;
	}
	mHintsReady = true;
	if (mTubeSlide.mIncRate != 0.0 && !mTubeSlide.HasBeenTriggered())
		mHintsReady = false;
	mReplayWaiting = mReplayMoveReady && mBoardSettled && mShownDialog == nullptr && mShownScore == mScore;
	if (!mReplayWaiting || gApp->Is3DAccelerated()) {
		if (mBoardX == aTo) {
			if (mShownBeforeStart) {
				MouseMove(mBoardMouseX, mBoardMouseY);
			}
			mShownBeforeStart = false;
			mGridSlide.SetInVal(0);
		} else {
			mHintsReady = false;
			mShownBeforeStart = true;
			if (!mTipShowing && (!mIsReplay || gApp->Is3DAccelerated())) {
				mBoardX = aTo;
				SnapPiecesX();
			} else {
				if (!mReplayMoveReady) {
					mHintsReady = false;
					if (mGridSlide.GetInVal() == 0.0)
						mGridSlide.SetCurve(GTS(0x7df2ac));
					mGridSlide.IncInVal();
				}
				if (mUpdateCnt == 0 && !mLevelStarted)
					mGridSlide.SetInVal(1.0);
				double anOut = mGridSlide.GetOutVal();
				double anOut2 = mGridSlide.GetOutVal();
				mBoardX = Ftol((1.0 - anOut) * (double)aFrom + (double)aTo * anOut2);
				if (!mIsReplay && !mReplayPlaying) {
					MoveCursor(GetRowAt(mBoardMouseY), GetColAt(mBoardMouseX), false, nullptr);
				} else if (mCursorPiece != nullptr) {
					SetCursor(mCursorPiece->mRow, mCursorPiece->mCol);
				}
				SnapPiecesX();
			}
		}
	} else {
		mShownBeforeStart = false;
	}
	// Whether the mouse is over the grid and the player may play
	if (mBoardMouseX < GetColX(0) || GetColX(mCols) < mBoardMouseX || mBoardMouseY < GetRowY(0) ||
		GetRowY(mRows) < mBoardMouseY || IsBusy() ||
		(mLevelDone && !mApp->mPausedByFocusLoss && mCursorOnGrid) || mApp->mOptionsDialog != nullptr ||
		mPlayState != 0 || mBonusStage == 4 || mPauseCount != 0 || mPauseShownCount != 0 || mFrozen ||
		mApp->mMandalaWidget != nullptr) {
		mCursorShown = false;
	} else {
		mCursorShown = true;
	}
	if (mPlayState == 1 || mReplayPlaying) {
		mCursorShown = true;
		mCursorAlpha = 1.0f;
	}
	if (0.0f < mMoveBlockWait) {
		float aLeft = (float)((double)mMoveBlockWait - (double)0.01f);
		mMoveBlockWait = aLeft;
		if (aLeft <= 0.0f) {
			// 0x6c425f (a puzzle's retry). Never reached: only 0x6bb145 (the puzzle's "TRY AGAIN", mode 5), which nothing
			// calls, sets +0x23d8 to anything but 0
			NotPorted("Board::UpdateGame: 0x6c425f");
			return;
		}
	}
	if (mCursorPiece == nullptr)
		mTwistMatches = false;
	MarkDirty();
	if (0.0f < mTwistGlow) {
		float aFade = (float)((double)mTwistGlow - (double)0.02f);
		mTwistGlow = aFade < 0.0f ? 0.0f : aFade;
	}
	if (mExtraWidget != nullptr)
		mExtraWidget->Update();
	if (mInstantTwist) {
		// 0x74db1b on the +0x211c object. Never reached: +0x210c is only ever written 0 (0x74d4da, which only
		// 0x74db1b calls)
		NotPorted("Board::UpdateGame: 0x74db1b");
	}
	if (0.0f < mHintFade) {
		float aFade = (float)((double)mHintFade - (double)0.01f);
		mHintFade = aFade < 0.0f ? 0.0f : aFade;
	}
	if (0.0f < mChainFlash) {
		float aFade = (float)((double)mChainFlash - (double)0.01f);
		mChainFlash = aFade < 0.0f ? 0.0f : aFade;
	}
	if (mLevelSouls.empty()) {
		// The turn ends a while after the board settles
		if (0.0f < mSettleWait) {
			if (mGameMode == MODE_BLITZ)
				mHintsReady = false;
			if (mHintShowing)
				mSettleWait = mSettleWait <= 0.1f ? 0.1f : mSettleWait;
			if (mIsReplay)
				mSettleWait = 1.0f;
			float aLeft = (float)((double)mSettleWait - (double)0.01f);
			mSettleWait = aLeft;
			if (aLeft <= 0.0f && mGameMode != MODE_BLITZ && mTutorial == nullptr) {
				if (CountDownCounters())
					return;
				if (mGameMode == MODE_ZEN && !mApp->mProfile->HasSeenTip(0x11) && 179999 < mUpdatesPlayed) {
					ShowZenJump(2);
				} else if (mGameMode == MODE_CLASSIC && !mApp->mProfile->HasSeenTip(0x11) && mIntroRoll < 3) {
					ShowZenJump(1);
				} else {
					StartGo();
					mApp->PlayVoice(SOUND_VOICE_GO, false);
				}
			}
		}
	} else {
		mSettleWait = 0.1f;
		mHintsReady = false;
		// The souls of the last level's special gems; each one back at its cell makes that gem special again
		bool aLanded = false;
		for (unsigned i = 0; i < mLevelSouls.size(); i++) {
			GemSoul* aSoul = mLevelSouls[i];
			if (mHyperspace != nullptr && (mHyperspace->mState == 6 || mHyperspace->mJumpSkipped))
				break;
			aSoul->Update();
			if (!aSoul->mIsHome)
				continue;
			int aY = Ftol((double)aSoul->mY);
			Piece* aPiece = GetPieceAtXY(Ftol((double)aSoul->mX), aY);
			if (aPiece != nullptr) {
				bool aQuiet = mQuietFill;
				mQuietFill = true;
				if (aSoul->mType & (1 << 3)) {
					MakeElectroGem(aPiece, 0.0f);
				} else if (aSoul->mType & (1 << 6)) {
					MakeIceGem(aPiece, 0.0f);
				} else if (aSoul->mType & (1 << 4)) {
					MakePowerGem(aPiece, 0.0f);
				} else if (aSoul->mType & (1 << 0)) {
					MakeFruitGem(aPiece);
					aPiece->mColor = aSoul->mColor;
				} else if (aSoul->mType & (1 << 7)) {
					// 0x6a4fc0 (Piece::MakeAngel on it). Never reached: FLAG_ANGEL (the "angel gem") is set only by MakeAngel
					// (0x729893), whose callers are this and MakeAngel's own pending kind, so no gem or soul has it
					NotPorted("Board::UpdateGame: a soul's 0x6a4fc0");
				}
				if (1 < mUpdateCnt - mLastSoulLandUpdate) {
					mApp->PlaySample(SOUND_GEMSOUL_LANDS3);
					mLastSoulLandUpdate = mUpdateCnt;
				}
				aPiece->mFlags = aSoul->mType;
				aLanded = aSoul->mFirst;
				mQuietFill = aQuiet;
			}
			delete aSoul;
			mLevelSouls.erase(mLevelSouls.begin() + i);
			i--;
		}
		if (aLanded)
			mApp->PlaySample(SOUND_GEMSOUL_LANDS3);
	}
	if (IsBusy()) {
		mHintsReady = false;
		if (mHyperspace != nullptr) {
			if (3 < mHyperspace->mState)
				mFallHeld = false;
			if (mHyperspace->mState == 10)
				mHyperspace->mClicked = true;
		}
		if (0.0f < mJumpFillWait) {
			mHintsReady = false;
			UpdateJump();
		}
	}
	if (mCursorEaseProgress < 1.0f)
		mCursorEaseProgress = (float)((double)mCursorEaseProgress + 0.25);
	EaseCursor();
	float aFadeIn = (float)((double)mCursorFadeIn + (double)0.01f);
	mCursorFadeIn = 1.0f <= aFadeIn ? 1.0f : aFadeIn;
	if (mIsReplay)
		mCursorFadeIn = 1.0f;
	if (0.0f < mWhiteFlash) {
		float aLeft = (float)((double)mWhiteFlash - (double)0.01f);
		mWhiteFlash = aLeft;
		if (aLeft < 0.0f)
			mWhiteFlash = 0;
	}
	if (mTwistAngle <= 0.0f || 90.0f <= mTwistAngle) {
		float aLeft = (float)((double)mTwistTrailFade - (double)0.01f);
		mTwistTrailFade = aLeft;
		if (aLeft <= 0.0f)
			mTwistTrailFade = 0;
	} else {
		mTwistTrailFade = 1.0f;
	}
	if (mRingBurst != 0.0f) {
		float aLeft = (float)((double)mRingBurst - (double)0.01f);
		mRingBurst = aLeft < 0.0f ? 0.0f : aLeft;
	}
	float aStep = -0.005f;
	if (mGlowGrowing)
		aStep = 0.005f;
	if (0.0f < mScoreRollGlow) {
		float aLeft = (float)((double)mScoreRollGlow - (double)0.02f);
		mScoreRollGlow = aLeft;
		if (aLeft < 0.0f)
			mScoreRollGlow = 0;
	}
	float anAngle = aStep + mCursorTurn;
	mCursorTurn = anAngle;
	if (0.0f <= anAngle) {
		if (kTwoPi < anAngle)
			mCursorTurn = anAngle - kTwoPi;
	} else {
		mCursorTurn = anAngle + kTwoPi;
	}
	if ((mStartState == 0 || mStartState == 2) && mApp->mHasFocus) {
		mStartFade.GetOutVal();
		bool aDone = !mStartFade.IncInVal(0.01);
		mStartFade.GetOutVal();
		if (aDone)
			mStartState = (mStartState != 0) * 2 + 1;
	}
	// port: no ported code opens a dialog numbered 28; the original's check of it stays
	if (mApp->GetDialog(28) != nullptr) {
		MarkDirty();
		return;
	}
	if (mPauseCount == mPauseShownCount && 0 < mPauseShownCount) {
		if (mPieceAlpha == 0.0f)
			return;
		float aFade = (float)((double)mPieceAlpha - (double)0.05f);
		mNoDialogsOpen = mApp->mDialogMap.size() == 0;
		mPieceAlpha = 0.0f < aFade ? aFade : 0.0f;
		MarkDirty();
		return;
	}
	mFXManager2->Update();
	if (!IsBusy() && mPlayState == 0) {
		if (mGameMode == MODE_CHALLENGE) {
			UpdateChallengeTimer();
		} else if (mGameMode == MODE_BLITZ && !IsBusy() && mTutorial == nullptr) {
			// Blitz's clock, in hundredths
			if (!mShownBeforeStart && -1 < mBlitzStartTime)
				mBlitzStartTime--;
			int aSeconds = mBlitzStartTime / 100;
			if ((mBlitzStartTime + 1) % 100 == 0) {
				if (aSeconds < 1) {
					if (!mIsReplay) {
						mCursorShown = false;
						aSeconds = 0;
						mBlitzStartTime = 0;
						mApp->mStats[35] = 30000 / 100; // 0x6a697d
						if (!mHintShowing && !mSettling && !mTwisting && mHintsReady && mElectroStrikes.empty() &&
							mFruitBlast == 0.0f && mShownScore == mScore && mGeodes.empty() &&
							mFruitDance == nullptr) {
							EndGame(false);
						}
					} else if (!mHintShowing && !mSettling && !mTwisting && mHintsReady) {
						mReplayMoveReady = true;
					}
				} else {
					ShowBlitzTime(aSeconds);
				}
			}
			mGoodTube->SetTime(aSeconds * 100, true);
		}
	}
	if (0.0 < mJumpActive) {
		float aLeft = (float)((double)mJumpScroll - (double)0.1f);
		mJumpScroll = aLeft;
		if (aLeft < 0.0f)
			mJumpScroll = (float)((double)mJumpScroll + 5.0);
		mJumpSpeed = mJumpSpeed + (double)0.0005f;
		mJumpDistance = mJumpDistance - mJumpSpeed;
		MarkDirty();
	}
	if (mUpdateCnt % 16 == 0) {
		mSparkleStep++;
		if (mSparkleStep == 8)
			mSparkleStep = 0;
	}
	mApp->mBoardUpdates++;
	if (mPieceAlpha < 1.0f) {
		float aFade = (float)((double)mPieceAlpha + (double)0.05f);
		mPieceAlpha = 1.0f <= aFade ? 1.0f : aFade;
		MarkDirty();
	}
	if (0 < mClickBlockUpdates) {
		mClickBlockUpdates--;
		if (mClickBlockUpdates == 0)
			mGoodTube->mWidgetFlagsMod.mRemoveFlags &= ~WIDGETFLAGS_ALLOW_MOUSE;
	}
	Widget::Update();
	mUpdatesPlayed++;
	if (mPlayState == 7)
		MarkDirty();
	if (mUpdateCnt == 2)
		mApp->ClearUpdateBacklog(false);
	if (mInputLock == 0) {
		mUpdatesSinceCascade++;
		if (mMatchCheckDue && (mMatchAtOnce || (mMatchDelay != 0 && --mMatchDelay == 0))) {
			mMatchDelay = 0;
			MatchAfterMove();
		}
	}
	if (mCursorPiece != nullptr && mUpdateCnt % 8 == 0) {
		MarkDirty();
		mCursorPieceTicks++;
	}
	int aPulsingLightCount = 0;  // the gems with +0x38 set: they send the board to 0x6c87d2 below
	int aRemoved = 0;
	if (mPlayState == 0) {
		mPowerGemCount = 0;
		mBombGemCount = 0;
		mElectroGemCount = 0;
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece != nullptr) {
					if (aPiece->IsFlagSet(Piece::FLAG_POWER))
						mPowerGemCount++;
					if (aPiece->IsFlagSet(Piece::FLAG_ELECTRO))
						mElectroGemCount++;
					if (aPiece->IsFlagSet(Piece::FLAG_BOMB))
						mBombGemCount++;
				}
			}
		}
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				Cell& aCell = mCells[mCols * aRow + aCol];
				aCell.mHoldsFixedGem = false;
				UpdateCell(aCell);
				if (aPiece == nullptr)
					continue;
				if (!aPiece->mBlasted) {
					if (aPiece->mHasPulsingLight)
						aPulsingLightCount++;
					if (aPiece->mTurnAmount == 0.0f && aPiece->mIsLive) {
						int anUnderRow = GetRowAt(Ftol(aPiece->mY));
						int anUnderCol = GetColAt(Ftol(aPiece->mX));
						if (-1 < anUnderRow && anUnderRow < mRows && -1 < anUnderCol && anUnderCol < mCols) {
							Cell& anUnder = mCells[anUnderRow * mCols + anUnderCol];
							Light aLight;
							if (aPiece->IsFlagSet(Piece::FLAG_ELECTRO) && Rand() % 2 == 0) {
								aLight.mPieceId = -1;
								anUnder.mLightGlow = 1.0f;
								aLight.mRising = true;
								anUnder.mLightPulse = 0;
								float aRadius = (float)((double)gGemWidth * (double)0.4f);
								float anAngle = (float)((double)RandSigned() * (double)3.14159265f);
								float aCos = cosf(anAngle);
								aLight.mX = (float)((double)(gGemWidth / 2) + (double)aPiece->mX + (double)aCos * (double)aRadius);
								float aSin = sinf(anAngle);
								aLight.mY = (float)((double)(gGemHeight / 2) + (double)aPiece->mY + (double)aRadius * (double)aSin);
								aLight.mLevel = 0;
								aLight.mStrength = 1.0f;
								aLight.mSize = 8.0f;
								aLight.mFalloff = 28.0f;
								aLight.mRiseRate = 0.1f;
								aLight.mFadeRate = 0.1f;
								aLight.mPiece = Rand() % 8 != 0 ? aPiece : nullptr;
								if (aLight.mPiece == nullptr)
									aLight.mStrength = 0.25f;
								mLights.push_back(aLight);
							}
							if (aPiece->IsFlagSet(Piece::FLAG_POWER)) {
								if (Rand() % 3 == 0) {
									aLight.mRising = true;
									anUnder.mLightPulse = 1.0f;
									float anAngle = (float)((double)RandSigned() * (double)3.14159265f);
									aLight.mPieceId = aPiece->mId;
									float aCos = cosf(anAngle);
									aLight.mX = (float)((double)gGemWidth * (double)aCos * 0.25 + (double)(gGemWidth / 2));
									float aSin = sinf(anAngle);
									aLight.mY = (float)(((double)aSin - (double)0.1f) * (double)gGemHeight * (double)0.35f +
														(double)(gGemHeight / 2));
									aLight.mLevel = 0;
									aLight.mStrength = 0.4f;
									aLight.mSize = 9.0f;
									aLight.mFalloff = 28.0f;
									aLight.mPiece = Rand() % 8 != 0 ? aPiece : nullptr;
									if (aLight.mPiece == nullptr)
										aLight.mStrength = 0.15f;
									aLight.mRiseRate = 0.06f;
									aLight.mFadeRate = 0.06f;
									mLights.push_back(aLight);
								}
							} else if (aPiece->IsFlagSet(Piece::FLAG_ICE)) {
								anUnder.mLightPulse = -1.0f;
							}
						}
					}
					if (!aPiece->mBlasted) {
						aPiece->Update();
						continue;
					}
				}
				// A gem that has gone off leaves the grid, or turns into what it is to become
				aPiece->mCol = aCol;
				aPiece->mRow = aRow;
				if (aPiece->mStrikeDelay != 0) {
					Piece* aNew = new Piece();
					aNew->mPendingKind = aPiece->mStrikeDelay;
					aPiece->mPendingKind = 0;
					aNew->mPendingTime = 0.0001f;
					aNew->Update();
					aNew->mHasBroken = true;
					aNew->mRow = aPiece->mRow;
					aNew->mCol = aPiece->mCol;
					aNew->mX = aPiece->mX;
					aNew->mY = aPiece->mY;
					aNew->mColor = aPiece->mColor;
					RemovePiece(aPiece, true);
					mGrid[mCols * aNew->mRow + aNew->mCol] = aNew;
				} else {
					RemovePiece(aPiece, true);
					if (mInputLock < 1 && !IsBusy())
						mSettling = true;
					aRemoved++;
				}
			}
		}
		if (!mDroppers.empty()) {
			mHintsReady = false;
			// The droppers at +0x21e4 (0x6f465f): never reached, only DropFlaggedGems makes them, for FLAG_DROPPER gems,
			// which no game has
			NotPorted("Board::UpdateGame: the objects at +0x21e4");
		}
		if (aRemoved != 0 && DropPieces())
			FillInBlanks(false);
		for (int aCol = 0; aCol < mCols; aCol++) {
			float aStep = (float)((double)gApp->mScale * 0.375);
			float aLift = aStep + mColumnLift[aCol];
			mColumnLift[aCol] = aLift <= 0.0f ? aLift : 0.0f;
		}
	}
	// The cells under the cursor's corners
	int aCursorX = mCursorX;
	int aCursorY = mCursorY;
	Piece* aCorners[4];
	aCorners[0] = GetPieceAtXY(aCursorX, aCursorY);
	aCorners[1] = GetPieceAtXY(aCursorX + gGemWidth, aCursorY);
	aCorners[2] = GetPieceAtXY(aCursorX, aCursorY + gGemWidth);
	aCorners[3] = GetPieceAtXY(aCursorX + gGemWidth, aCursorY + gGemHeight);
	for (int i = 0; i < 4; i++) {
		Piece* aPiece = aCorners[i];
		if (!CanMove(aPiece) && aPiece != nullptr)
			mCells[aPiece->mRow * mCols + aPiece->mCol].mHoldsFixedGem = true;
	}
	if (mInputLock == 0) {
		float aShine = (float)((double)mShineAmount + (double)0.025f);
		mShineAmount = aShine;
		if (1.0f < aShine)
			mShineAmount = 1.0f;
	}
	mShineAmount = mBoardFade * mShineAmount;
	float aGlow = (float)((double)mCursorAlpha - (double)0.01f);
	mCursorAlpha = aGlow;
	if (aGlow < 0.0f)
		mCursorAlpha = 0;
	LayoutTube();
	if (!IsBusy() && 0.0f < mShakeX) {
		double aStep = (double)mShakeX * (double)0.1f;
		double aMax = (double)mWidth * (double)0.015f;
		float aLeft = mShakeX - (float)(aStep <= aMax ? aStep : aMax);
		mShakeX = aLeft;
		if (aLeft < 2.0f) {
			mShakeX = 0;
			if (mLevelStarted && !mGoodTube->mVisible) {
				StartTubeSlide();
				LayoutTube();
				mGoodTube->mVisible = true;
			}
			// The souls pick their gems of the new board (as RetargetTravellers, in one pass), each starting its swirl
			// a step round from the last
			float anAngle = (float)((double)RandSigned() * 3.1415927410125732);
			float aStep = (float)((double)6.2831854820251465f / (double)(unsigned)mLevelSouls.size());
			for (unsigned i = 0; i < mLevelSouls.size(); i++) {
				GemSoul* aSoul = mLevelSouls[i];
				int aCells = mRows * mCols;
				int aCell = BoardRand() % aCells;
				for (int n = 0; n < aCells; n++) {
					Piece* aPiece = mGrid[aCell];
					if (aPiece != nullptr && aPiece->mFlags == 0) {
						bool aTaken = false;
						for (unsigned j = 0; j < mLevelSouls.size(); j++) {
							if (mLevelSouls[j]->mState == 2 && mLevelSouls[j]->mHomePiece == aPiece) {
								aTaken = true;
								break;
							}
						}
						if (!aTaken && (!(aSoul->mType & (1 << 0)) || aPiece->mColor == aSoul->mColor)) {
							aSoul->SetTarget(aPiece);
							aSoul->mAngle = anAngle;
							aSoul->mIsVisible = true;
							break;
						}
					}
					aCell++;
					if (aCell == aCells)
						aCell = 0;
					anAngle = aStep + anAngle;
				}
			}
		}
	}
	// The board's state: 0 play, 3 and 4 waits, 5 the level's start, 7 the level-up's blast, 8 the puzzle's fill
	bool aSettled = false;
	bool aFall = false;
	switch (mPlayState) {
	case 0:
		if (mTwisting) {
			UpdateTwist();
		} else {
			float aFade = (float)((double)mCursorTwistFade - (double)0.04f);
			mCursorTwistFade = 0.0f < aFade ? aFade : 0.0f;
		}
		if (mElectroStrikes.empty())
			aSettled = true;
		CheckSettled();
		if (0.0f < mBarTopAlpha) {
			float aFade = (float)((double)mBarTopAlpha - (double)0.01f);
			mBarTopAlpha = 0.0f < aFade ? aFade : 0.0f;
		}
		if (mClicksIgnored)
			mCursorAlpha = 0;
		if (!IsBusy()) {
			float aGlow = (float)((double)mCursorAlpha + (double)0.02f);
			mCursorAlpha = aGlow;
			if (1.0f < aGlow)
				mCursorAlpha = 1.0f;
			if (mInputLock == 0) {
				if (mLevelDone && !mHintShowing && !mSettling && !mTwisting && mElectroStrikes.empty() &&
					mTutorial == nullptr && mScore == mShownScore && mSmokeTravellers.empty() &&
					mGoodTube->IsIdle() && mShockwaves.empty() && !mFruitDue && mShownDialog == nullptr &&
					mPendingDialogs.empty() && mFruitBlast == 0.0f && mGeodes.empty() && mBonusStage != 4) {
					if (mTurnCount != mTurnSettled)  // 0x6aa1ef
						mTurnSettled = mTurnCount;
					LevelComplete();
					mLevelDone = false;
					mGameLoaded = false;
					if (mLevelCounts)
						mLevelsFinished++;
				} else if (mChallengeMet && !mHintShowing && !mSettling && !mTwisting && mElectroStrikes.empty() &&
						   mChallenge != 0 && mGoodTube->IsIdle()) {
					mChallengeOver = true;
					if ((mChallenge->mFlags & Challenge::FLAG_NIGHTMODE) == 0) {
						// the next challenge of the category, and the next category, open
						Profile* aProfile = mApp->mProfile;
						aProfile->SetChallengeDone(mChallenge->mCategory, mChallenge->mNumber);
						aProfile->Save();
						aProfile->OpenChallengeCategory(mChallenge->mCategory + 1);
						ShowChallengeComplete(false);
					} else {
						ShowChallengeComplete(true);
						std::vector<int>& aBests = mApp->mProfile->mChallengeBests;
						int aCategory = mChallenge->mCategory;
						if ((int)aBests.size() <= aCategory)
							aBests.resize(aCategory + 1, 0);  // 0x43c2a0
					}
					mChallengeMet = false;
				}
			}
		}
		aFall = aSettled;
		break;
	case 3:
		if (IsBusy())
			break;
		if (mGoodTube->mTimeKind == 4)
			mPlayState++;
		aFall = true;
		break;
	case 4: {
		float aLeft = (float)((double)mStateWait - (double)0.01f);
		mStateWait = aLeft;
		if (aLeft <= 0.0f)
			mPlayState++;
		aFall = true;
		break;
	}
	case 5: {
		// The level's start: once the wait is up and the board has settled, "go"
		float aLeft = (float)((double)mStateWait - (double)0.01f);
		mStateWait = aLeft;
		if (aLeft <= 0.0f && mSettleWait <= 0.0f) {
			StartGo();
			mApp->PlayVoice(SOUND_VOICE_GO, false);
			mStateWait = 1.0f;
			mGoodTube->SetShowTenths(true);
			PlayGameMusic();
			mPlayState = 0;
		}
		aFall = true;
		break;
	}
	case 7:
		// The game is over
		if (!mEndedByNuke) {
			UpdateGameOver();
		} else if (UpdateNuke()) {
			return;
		}
		break;
	case 8:
		mPuzzleFillWait--;
		if (mPuzzleFillWait < 0) {
			if (mGameMode == MODE_PUZZLE) {
				// The puzzle's fill (0x6c3515, Rand draws). Never reached: no menu starts a puzzle (mode 5; see
				// NOTES, "Never reached")
				NotPorted("Board::UpdateGame: the puzzle's fill");
				mPlayState = 0;
				mInputLock = 0;
				return;
			}
		} else if (mGameMode == MODE_PUZZLE) {
			// The puzzle's sparkles (Rand for the cell, an effect 12 over it). Never reached: no menu starts a puzzle
			NotPorted("Board::UpdateGame: the puzzle's sparkles");
		}
		break;
	default:
		break;
	}
	if (aFall)
		FallPieces();
	UpdateLevelUp();
	UpdateGlows();
	UpdateCellPoints();
	mBoardSettled = !mHintShowing && !mSettling && !mTwisting && mFruitBlast == 0.0f && mElectroStrikes.empty() &&
				   mBonusStage != 4 && mGoodTube->mBonusState == 0;
	// The progress bar's ends follow the grid's right and middle
	mTubeAboveMiddle = mBarEndY < GetGridMidY();
	if (0.0f < mProgressHold) {
		mProgressHold = (float)((double)mProgressHold - (double)0.01f);
	} else if (!mIsReplay) {
		bool aLow = (double)mBarEndY < (double)GetGridMidY() * (double)0.7f;
		float aFade = (float)((double)mBarGlow - (double)0.01f);
		mBarGlow = 0.0f <= aFade ? aFade : 0.0f;
		if (mBarWidth < GetGridRight()) {
			mBarGlow = 1.0f;
			double aTo = (double)gGemWidth * 0.5 + (double)mBarWidth;
			int aRight = GetGridRight();
			mBarWidth = (double)aRight < aTo ? GetGridRight() : Ftol(aTo);
		} else if (mTubeAboveMiddle) {
			mBarGlow = 1.0f;
			double aTo = (double)gGemHeight * (double)0.05f + (double)mBarEndY;
			float aMid = GetGridMidY();
			mBarEndY = (float)((double)aMid < aTo ? (double)GetGridMidY() : aTo);
		}
		if (aLow)
			mAlertGlow = 0.2f;
	}
	if (!mElectroStrikes.empty() || aPulsingLightCount != 0) {
		UpdateStrikes();
	}
	if (mGeodes.empty())
		mGeodesPending = false;
	UpdateGeodes();
	MarkDirty();
	// The board's shake
	if (!(0.0f < mShakeAmount)) {
		mShakeOffsetX = 0;
		mShakeOffsetY = 0;
	} else if (mUpdateCnt % 3 == 0) {
		float aFade = (float)((double)mShakeAmount - (double)0.1f);
		mShakeAmount = aFade;
		if (aFade < 0.0f)
			mShakeAmount = 0;
		int aRand = CrtRand() % 2000 - 1000;
		mShakeOffsetX = Ftol((double)mShakeAmount * (double)mShakeSize * ((double)aRand / 1000.0));
		aRand = CrtRand() % 2000 - 1000;
		mShakeOffsetY = Ftol((double)mShakeAmount * (double)mShakeSize * ((double)aRand / 1000.0));
	}
	// Now and then a glint runs over the board
	if (mGlintRunning) {
		float aNext = (float)((double)mGlintPlace + (double)0.02f);
		mGlintPlace = aNext;
		if (1.0f < aNext)
			mGlintRunning = false;
		MarkDirty();
	} else if (Rand() % 6000 == 0) {
		mGlintRunning = true;
		mGlintPlace = 0;
	}
	float aDim = (float)((double)mDimAmount - (double)0.06f);
	if (!(0.0f < aDim))
		aDim = 0;
	mDimAmount = aDim;
	if (!(0.0f < aDim) && mInputLock == 0) {
		double aNext = (double)0.06f + (double)mDimRecovery;
		mDimRecovery = (float)(aNext < 1.0 ? aNext : 1.0);
	}
	if (mRainEffect && Rand() % 2 == 0) {
		Effect* anEffect = mFXManager->AllocEffect(0x15);
		anEffect->mX = (float)(int)(Rand() % 1024);
		anEffect->mY = -2.0f;
		anEffect->mDY = (float)((double)RandSigned() * 0.25 + 0.5);
		mFXManager->AddEffect(anEffect);
	}
	if (mBurstSoundDue) {
		SoundInstance* aSound = mApp->mSoundManager->GetSoundInstance(SOUND_BOMB_EXPLODE);
		if (aSound != nullptr)
			aSound->Play(false, true);
		mBurstSoundDue = false;
	}
	if (mInputLock == 0)
		mPlaySeconds = (float)((double)mPlaySeconds + (double)0.01f);
	mClickedLastUpdate = mClickedThisUpdate;
	mClickedThisUpdate = false;
	UpdateFruitBlast();
	if (mHintShowing)
		mHintsReady = false;
	if (mSettling)
		mHintsReady = false;
	if (mTwisting)
		mHintsReady = false;
	if (mJumpFillWait != 0.0f)
		mHintsReady = false;
	if (0.0f < mMoveBlockWait)
		mHintsReady = false;
	if (mInputLock != 0)
		mHintsReady = false;
	// A fruit's blast and the bonus challenge it starts (+0x265c, BreakPiece: 1 or 2 when a fruit broke, 3 the
	// challenge, 4 the fruit dance when it is met, 5 when it is missed)
	if (mBonusStage == 2) {
		if (mGoodTube->mBonusShown == 0.0f)
			mBonusMatched = 0;
		float aT = (float)((double)mFruitBlast * 2.799999952316284 - 0.44999999999999996 - 0.5);
		aT = (float)(((double)aT - 0.30000001192092896) * 0.8500000238418579);
		mBlastRingSize.SetInVal(aT);
		float aRadius = (float)(mBlastRingSize.GetOutVal() * 4.400000095367432 * (double)gGemWidth);
		int aX = GetColX(mFruitCol) + gGemWidth / 2;
		int aY = GetRowY(mFruitRow) + gGemHeight / 2;
		int aTubeX = mGoodTube->mX - Ftol((double)gApp->mScale * -300.0);
		int aTubeY = mGoodTube->mY - Ftol((double)gApp->mScale * -900.0);
		float aDX = (float)(aX - aTubeX);
		float aDY = (float)(aY - aTubeY);
		float aDist = sqrtf((float)((double)aDX * (double)aDX + (double)aDY * (double)aDY));
		// The challenge starts when the last ring reaches a point by the tube (or the blast is over or nearly)
		if (mGoodTube->mBonusShown == 0.0f &&
			(mFruitBlast == 0.0f || fabsf((float)(aDist - aRadius)) < 10.0f || 0.8999999761581421 <= (double)mFruitBlast)) {
			mGoodTube->mBonusShownCount = 0;
			mBonusMatchedMark = -1;
			mBonusStage = 3;
			mGoodTube->mBonusGlowCurve.SetCurve(GTS(0x7dcf14));
			StartBonusChallenge(mFruitColor);
		}
	} else if (mBonusStage == 5) {
		if (mGoodTube->mBonusShown == 0.0f) {
			mBonusStage = 0;
			mBonusMatched = 0;
			mBonusColors.clear();
			mGoodTube->mBonusShownCount = 0;
		}
	}
	if (mBonusStage == 4 && mTutorial == nullptr) {
		if (mFruitDance == nullptr) {
			if (mHintsReady) {
				// The fruit dance (fruitdance.pam) over the board, with its own music
				mGoodTube->mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_ALLOW_MOUSE;
				// port: the original also writes a "FRUIT DANCE!" line (the mode, level, rank and play time) to its
				// debug log (app +0x1024)
				mGoodTube->mBonusGlowCurve.SetCurve(GTS(0x7df1c8));
				PopAnim* anAnim = new PopAnim(0, this);
				mFruitDance = anAnim;
				mFruitDanceUpdates = 0;
				mFruitDanceSmallPoints = 0;
				mFruitDanceLargePoints = 0;
				anAnim->mImageFolders.clear();
				anAnim->mImageFolders.push_back(StrFormat(GTC(0x7df1b4), gApp->mArtRes));
				anAnim->mImageScale = gApp->mScale;
				anAnim->mDrawScale = gApp->mScale;
				anAnim->LoadAnimation(GTS(0x7df190));
				mApp->ClearUpdateBacklog(false);
				int aWidth = Ftol((double)gApp->mScale * 1600.0);
				int aHeight = Ftol((double)gApp->mScale * 1200.0);
				anAnim->Resize((mWidth - aWidth) / 2, 0, aWidth, aHeight);
				AddWidget(anAnim);
				anAnim->PlayFrame(0);
				anAnim->mVisible = false;
				if (mApp->mMusicInterface != nullptr) {
					// port: the original keeps BASS's position in the music playing (0x6ee282) here (+0x3928);
					// PlayMusic drops it, so the port keeps what is there (as in RemovedFromManager)
					mApp->mMusicInterface->StopAllMusic();
					mApp->mMusicInterface->SetVolume(mApp->mSfxVolume < mApp->mMusicVolume ? mApp->mMusicVolume :
						mApp->mSfxVolume);
					mApp->PlayMusic(0x133, false, false, 0, 0);
				}
			}
		} else if (2200 <= mFruitDanceUpdates) {
			// The dance is over: the game's music again
			RemoveWidget(mFruitDance);
			delete mFruitDance;
			mFruitDance = nullptr;
			mBonusStage = 0;
			mBonusMatched = 0;
			mBonusColors.clear();
			mGoodTube->mBonusShownCount = 0;
			mSettling = true;
			if (mApp->mMusicInterface != nullptr)
				mApp->mMusicInterface->SetVolume(mApp->mMusicVolume);
			if (!mIsReplay) {
				if (mGameMode == MODE_BLITZ)
					PlayGameMusic();
				else
					mApp->PlayMusic(mMusicTrack + 100, true, true, 0, 0);
			} else if (mApp->mMainMenu != nullptr) {
				mApp->mMainMenu->PlayMusic(-1);
			}
		}
		mHintsReady = false;
	}
	if (mPlayState == 2) {
		if (mUpdateCnt % 2 == 0) {
			// State 2 puts the gems of +0x148c back on the board one by one, picked by Rand, each with a glow
			// (IMAGE_INGAMEUI_GRIDSQUAREGLOW). Never reached: nothing sets the board's +0x160 to 2 (states 3 and 4
			// step on to 4 and 5; the 2s at +0x160 in 0x6a35fd are the ReplayWidget's own state)
			NotPorted("Board::UpdateGame: state 2");
		}
	} else if (mPlayState == 1) {
		// A replay's rewind: DrawOverlay draws the recorded frames back from the last, faster and faster; at the
		// first the move is made again
		mHintsReady = false;
		mReplayRand.SRand(mUpdateCnt / 3);
		int aStep = mRewindFrame / 100 + 2;
		if (mRewindFrame / 20 <= aStep)
			aStep = mRewindFrame / 20;
		mRewindFrame -= aStep;
		if (aStep <= 0)
			mRewindFrame--;
		if (gGraphicsRecorder->GetLastFrame() < 0 || mRewindFrame < 0) {
			if (mRewindSound != nullptr) {
				mRewindSound->Release();
				mRewindSound = nullptr;
			}
			mPlayState = 0;
			mRecordedFrames = 0;
			StartTwist(false, -1, -1);
		}
		// the board itself is not drawn while the frames play back
		gGraphicsRecorder->mSkipDraw = true;
	} else if (mReplayPlaying) {
		mReplayRand.SRand(mUpdateCnt / 7);
	}
	if (mPlayState != 1 && mJumpFillWait == 0.0f && mLevelUpTilePhase == 0 && mStartState == 1) {
		// The recorder keeps the board's frames while it moves, up to 500 of them
		if (!mHintsReady && mRecordedFrames < 500) {
			mRecordedFrames++;
			int aFrame = mRecordedFrames;
			int aGap = aFrame - gGraphicsRecorder->GetLastFrame();
			bool aRecord = true;
			if (aGap < 10 && 19 < aFrame && (49 < aFrame || aGap < 5))
				aRecord = false;
			gGraphicsRecorder->mRecording = aRecord;
			gGraphicsRecorder->mFrame = mRecordedFrames;
		}
		gGraphicsRecorder->mSkipDraw = false;
	}
	if (mHintsReady && !mFrozen) {
		mSettledUpdates++;
		bool aTutorialBusy = false;
		// (the tutorial's dialog holds the hints back)
		if (mTutorial != nullptr && mTutorial->mDialog != nullptr)
			aTutorialBusy = true;
		if (!mApp->mShowHints || mHintState != 0 || mGameMode == MODE_CHALLENGE || -1 < mTipCol ||
			aTutorialBusy) {
			UpdateHint();
		} else {
			// With the hints on, a hint once the player has waited long enough
			mHintWait = (float)((double)mHintWait + (double)0.01f);
			float aWait;
			if ((unsigned int)gApp->mProfile->mGamesPlayed <= 1 && mLevel <= 1)
				aWait = 5.0f;
			else
				aWait = 15.0f;
			int aLimit = Ftol(aWait);
			if (mCounterHintDue)
				aLimit = 3;
			if ((float)aLimit <= mHintWait) {
				ShowHint();
				mHintWait = 0;
			}
		}
	}
	int anUpdateCnt = mUpdateCnt;
	if (!mGameOver && mPlayState == 0 && !IsBusy() && mTutorial == nullptr &&
		(mGameMode != MODE_CHALLENGE || !mChallengeMet)) {
		// The seconds of play, for the stats
		mSecondProgress += anUpdateCnt - mLastTimerCount;
		if (1000 / gApp->mFrameTime <= mSecondProgress) {
			if (mGameMode == MODE_CHALLENGE && mChallenge != 0 && HasChallengeFlag(2) && !mHintShowing &&
				!mSettling && !mTwisting && mHintsReady && mElectroStrikes.size() == 0 && mBoardSettled) {
				// a timed challenge runs out through the goal check (0x6d090f)
				CheckGoal(true);
			}
			AddStat(35, 1);
			mSecondProgress = mSecondProgress % (1000 / gApp->mFrameTime);
		}
	}
	mLastTimerCount = anUpdateCnt;
	if (mTipShowing) {
		// The tutorial's arrow fades in while the cursor is off its cell
		int aCol = GetColAt(mCursorX);
		int aRow = GetRowAt(mCursorY);
		if (mTipCol == aCol && mTipRow == aRow) {
			double aNext = (double)mTipArrowFade - 0.075;
			mTipArrowFade = (float)(0.0 < aNext ? aNext : 0.0);
		} else {
			double aNext = (double)mTipArrowFade + 0.075;
			mTipArrowFade = (float)(aNext < 1.0 ? aNext : 1.0);
		}
	}
	if (0 < mFillColor.mAlpha) {
		int aNext = mFillColor.mAlpha - 1;
		mFillColor.mAlpha = aNext < 0 ? 0 : aNext;
	}
	if (mShakeX == 0.0f && mTubeSlide.GetOutVal() == 0.0 && mTutorial == nullptr) {
		// The tip slides in, waits, and slides back
		mTipSlide.IncInVal();
		if (0.0f < mTipSlideBackWait) {
			float aLeft = (float)((double)mTipSlideBackWait - (double)0.01f);
			mTipSlideBackWait = aLeft;
			if (!(0.0f < aLeft))
				mTipSlide.mIncRate = mTipSlide.mIncRate * -1.0;
		}
	}
	mGoScale.IncInVal();
	// The cell under the cursor: a tick as it moves onto another gem
	int aRow = GetRowAt(mCursorY);
	int aCol = GetColAt(mCursorX);
	bool aTick = mCursorShown && !mEditorMode && !(mGameMode == MODE_PUZZLE && mPuzzleSolved) &&
				 !(mTutorial != nullptr && mTutorial->mMouseVisible) && mBonusStage != 4;
	if (aCol != mHoverCol || aRow != mHoverRow) {
		if (aCol < 0) {
			mCursorRestCount = 0;
			goto ticked;
		}
		if (aTick && mInputLock == 0) {
			mCursorRestCount = 0;
			mApp->PlaySoundEx(SOUND_SELECT, GetPan(gGemWidth / 2 + mCursorX), 0.0f, 0.0f, true, true);
		}
	}
	if (aCol < 0 || !mHintsReady)
		mCursorRestCount = 0;
ticked:
	// The cursor's glow, after 40 updates on one cell in the first level
	if (mCursorRestCount == 0)
		mCursorGlow = 0;
	mCursorRestCount++;
	if (40 < mCursorRestCount && mLevel < 1) {
		double aNext = (double)mCursorGlow + (double)0.01f;
		mCursorGlow = (float)(aNext < 0.5 ? aNext : 0.5);
	}
	if (IsTipShowing() && -1 < mTipCol)
		mCursorGlow = mTipCol == aCol && mTipRow == aRow ? 0.5f : 0.0f;
	if (mApp->mStartClassicAtOnce) {
		double aNext = (double)mBoardFade + (double)0.015f;
		float aFade = (float)(1.0 < aNext ? 1.0 : aNext);
		mBoardFade = aFade;
		if (aFade == 1.0f)
			mProgressHold = 0;
	}
	mHoverCol = aCol;
	mHoverRow = aRow;
	MarkDirty();
	UpdateToolTips();
	if (mReplayPodDue && !mIsReplay && mBoardSettled && !mTipShowing && !mLevelDone && mRankBonusWidget == nullptr &&
		mLevelUpShow == nullptr && mShownDialog == nullptr && mPendingDialogs.empty() && !mGeodesPending) {
		if (mGameMode != MODE_CHALLENGE && mGameMode != MODE_BLITZ)
			ShowReplayPod();
		if (mReplayPlaying) {
			mWidgetFlagsMod.mRemoveFlags &= ~WIDGETFLAGS_ALLOW_MOUSE;
			mReplayPlaying = false;
		}
		mReplayPodDue = false;
		mGeodesPending = false;
	}
	if (-1 < mApp->mAutoPlayMode && mBoardSettled && !mTwisting) {
		if (!mHintsReady)
			return;
		if (mPauseCount == 0 && !IsBusy() && !mReplayPlaying && !mIsReplay &&
			(mShownDialog == nullptr || !mShownDialog->mMouseVisible) && mUpdateCnt % 15 == 0) {
			// -autoplay's next move: a click at the corner the move turns about, or a random one
			int aRow;
			int aCol;
			FindMove(&aRow, &aCol, false);
			if (aRow == -1 || aCol == -1) {
				aRow = (int)Rand() % mRows;
				aCol = (int)Rand() % mCols;
			}
			mAutoClickX = GetColX(aCol) + gGemWidth;
			mAutoClickY = GetRowY(aRow) + gGemHeight;
			mAutoClickTime = mUpdateCnt;
		}
	}
	// A click queued by -autoplay
	if (mHintsReady && mAutoClickTime != 0 && mUpdateCnt - mAutoClickTime < 20) {
		mAutoClickTime = 0;
		if (-1 < mApp->mAutoPlayMode)
			mCursorShown = true;
		MouseDown(mAutoClickX, mAutoClickY, 0, 1);
		mAutoClickTime = 0;
	}
}

void Board::UpdateCell(Cell& theCell)
{
	if (0.0f < theCell.mFade) {
		float aFade = (float)((double)theCell.mFade - (double)0.01f);
		theCell.mFade = aFade;
		if (aFade < 0.0f)
			theCell.mFade = 0;
	}
	if (0.0f < theCell.mLightGlow) {
		float aFade = (float)((double)theCell.mLightGlow - (double)0.01f);
		theCell.mLightGlow = aFade < 0.0f ? 0.0f : aFade;
	}
	if (0.0f < theCell.mLightPulse) {
		float aFade = (float)((double)theCell.mLightPulse - (double)0.01f);
		theCell.mLightPulse = aFade < 0.0f ? 0.0f : aFade;
	} else if (theCell.mLightPulse < 0.0f) {
		float aFade = (float)((double)theCell.mLightPulse + (double)0.01f);
		theCell.mLightPulse = 0.0f < aFade ? 0.0f : aFade;
	}
	int aLeft = theCell.mColor.mAlpha - 3;
	theCell.mColor.mAlpha = aLeft < 0 ? 0 : aLeft;
	if (gBoard->mTutorial != nullptr) {
		int aState = gBoard->mTutorial->mState;
		if (aState != Tutorial::STATE_NONE && aState != Tutorial::STATE_MATCHES && aState != Tutorial::STATE_LEVELUP)
			theCell.mIsMarked = false;
	}
	if (!theCell.mIsMarked) {
		float aFade = (float)((double)theCell.mMarkGlow - (double)0.04f);
		theCell.mMarkGlow = aFade < 0.0f ? 0.0f : aFade;
	} else {
		float aFade = (float)((double)theCell.mMarkGlow + (double)0.02f);
		theCell.mMarkGlow = 1.0f < aFade ? 1.0f : aFade;
	}
	if (theCell.mMarkGlow == 0.0f)
		theCell.mMarkTick = 0;
	else
		theCell.mMarkTick++;
	if (theCell.mHintFlash != 0.0f) {
		float aNext = (float)((double)theCell.mHintFlash + (double)0.006f);
		theCell.mHintFlash = aNext;
		if (1.0f < aNext)
			theCell.mHintFlash = 0;
	}
}

bool Board::DropPieces()
{
	if (mFallHeld)
		return false;
	bool aGap = false;
	int aCol = 0;
	if (0 < mCols) {
		do {
			int aBelow = mRows;
			int aRow = aBelow;
			while (aRow = aRow - 1, -1 < aRow) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece == nullptr) {
					aGap = true;
				} else {
					aPiece->mHasDropped = false;
					int aTo = aRow;
					int aNext = aBelow;
					if (aBelow < mRows) {
						do {
							if (GetPieceAt(aNext, aCol) != nullptr)
								break;
							aTo++;
							aNext++;
						} while (aNext < mRows);
						if (aTo != aRow) {
							mGrid[mCols * aRow + aCol] = nullptr;
							aPiece->mRow = aTo;
							aPiece->mHasDropped = true;
							mGrid[mCols * aTo + aCol] = aPiece;
							if (mInstantTwist)
								aPiece->mY = (float)GetRowY(aPiece->mRow);
							aGap = true;
						}
					}
				}
				aBelow--;
			}
			aCol++;
		} while (aCol < mCols);
	}
	return aGap;
}

bool Board::CanMove(Piece* thePiece)
{
	if (thePiece == nullptr)
		return false;
	bool aLocked = thePiece->IsFlagSet(Piece::FLAG_DEVIL) || thePiece->IsFlagSet(Piece::FLAG_LOCKED);
	if (thePiece->IsFlagSet(Piece::FLAG_BOMB) && !mBombsCanMove)
		aLocked = true;
	if (thePiece->IsFlagSet(Piece::FLAG_DROPPER) && !mDropperGemsMove)
		aLocked = true;
	if (thePiece->IsFlagSet(Piece::FLAG_FIXED))
		aLocked = true;
	return !aLocked;
}

void Board::LayoutTube()
{
	if (mGameMode == MODE_PUZZLE) {
		if (IsBusy())
			return;
		// The puzzle's widget at +0x23d4 slides 5% of its x each update. Never reached: no menu starts a puzzle
		NotPorted("Board::LayoutTube: the puzzle's widget");
		return;
	}
	if (mLevelSlideIn.mIncRate != 0.0 && !mLevelSlideIn.HasBeenTriggered()) {
		// The new level: the board slides in (mShakeX), the tube shows once it is in
		if (!mLevelSlideIn.IncInVal() && !(0.0f < mNextLevelWait))
			mGoodTube->SetVisible(true);
		double anOut = mLevelSlideIn.GetOutVal();
		mShakeX = (float)(anOut * (double)(mWidth - GetColX(0)));
	} else if (!mTubeSlide.HasBeenTriggered() && !mTubeSlide.IncInVal()) {
		if (0.0 < mTubeSlide.GetOutVal()) {
			mGoodTube->SetVisible(false);
		} else if (mGameMode == MODE_BLITZ && mApp->mPlayPressed) {
			mGoodTube->SetTime(mBlitzStartTime, false);
		}
		if (0.0f < mNextLevelWait && mLevelSlideIn.mOutVal == 0.0) {
			mShownBeforeStart = true;
			mLevelSlideIn.SetCurve(GTS(0x7dca10));
		}
	}
	// The tube beside the board, out by the curve
	int anOldX = mGoodTube->mX;
	Board* aBoard = gBoard;
	int aRoom = (aBoard->mWidth - GetBoardWidth() - aBoard->mGoodTube->GetTubeWidth()) / 3;
	double anOut = mTubeSlide.GetOutVal();
	int aSlide = Ftol((double)gApp->mScale * 600.0);
	double aX = (double)aRoom - anOut * (double)aSlide;
	int aMargin = Ftol((double)gApp->mScale * 30.0);
	int aNewX = Ftol(aX - (double)aMargin);
	if (aNewX != anOldX && !mGameOver && (!mIsReplay || !mHintsReady)) {
		GoodTube* aTube = mGoodTube;
		int aHeight = aTube->GetTubeHeight();
		aTube->Resize(aNewX, aTube->mY, aTube->GetTubeWidth(), aHeight);
	}
}

void Board::UpdateGlows()
{
	if (0.0f < mBlueFlash)
		mFXManager3->Update();
	// port: the original reads the music's volume at +0x2c without a check for none; the port has none without sound
	CustomBassMusicInterface* aMusic = mApp->mTwistMusic;
	if (IsSparkBlocked()) {
		// The music fades out while the sparks are held back, and lightning plays over the board
		if (aMusic != nullptr && 0.0f < aMusic->mVolumeFactor) {
			float aFade = (float)((double)aMusic->mVolumeFactor - (double)0.01f);
			aMusic->mVolumeFactor = aFade < 0.0f ? 0.0f : aFade;
		}
		float aGlow = (float)((double)0.01f + (double)mBlueFlash);
		mBlueFlash = 1.0f < aGlow ? 1.0f : aGlow;
		if (!IsBusy()) {
			float aLeft = (float)((double)mGlowSoundWait - (double)0.01f);
			mGlowSoundWait = aLeft;
			if (aLeft <= 0.0f) {
				float aRand = RandSigned();
				mGlowSoundWait = (float)((double)fabsf(aRand) * 0.75 + (double)0.1f);
			}
			aLeft = (float)((double)mGlowSoundRepeat - (double)0.01f);
			mGlowSoundRepeat = aLeft;
			if (aLeft <= 0.0f) {
				mGlowSoundRepeat = 4.0f;
				if (mGlowSounds[mGlowSoundIndex] != nullptr) {
					mGlowSounds[mGlowSoundIndex]->Stop();
					mGlowSounds[mGlowSoundIndex]->Release();
				}
				mGlowSoundIndex ^= 1;
			}
			Rand();
			Effect* anEffect = mFXManager->AllocEffect(0x13);
			float anAngle = (float)((double)RandSigned() * (double)kTwoPi);
			float aSin = sinf(anAngle);
			anEffect->mX = (float)((double)gBoard->mEasedCursorX + 89.0 + (double)aSin * 96.0);
			float aCos = cosf(anAngle);
			anEffect->mY = (float)((double)gBoard->mEasedCursorY + 84.0 + (double)aCos * 96.0);
			mFXManager->AddEffect(anEffect);
			anEffect = mFXManager3->AllocEffect(0x14);
			float aRand = fabsf(RandSigned());
			anEffect->mX = (float)((double)mWidth * (double)aRand);
			aRand = RandSigned();
			aRand = (float)pow((double)aRand, (double)3.0f);
			aRand = fabsf(aRand);
			anEffect->mY = (float)((double)mHeight * (double)aRand);
			mFXManager3->AddEffect(anEffect);
		}
	} else {
		if (aMusic != nullptr && aMusic->mVolumeFactor < 1.0f) {
			float aFade = (float)((double)aMusic->mVolumeFactor + (double)0.01f);
			aMusic->mVolumeFactor = aFade < 1.0f ? aFade : 1.0f;
		}
		if (0.0f < mBlueFlash) {
			float aGlow = (float)((double)mBlueFlash - (double)0.01f);
			mBlueFlash = aGlow < 0.0f ? 0.0f : aGlow;
		}
	}
	// The 24 glows run round, faster while +0x21d9 is on
	float* aGlow = mGlowRing;
	int aLeft = 24;
	do {
		float aStep = mTwisting ? 0.05f : 0.02f;
		float aFade = *aGlow - aStep;
		*aGlow = aFade;
		if (aFade <= 0.0f)
			*aGlow = 0.0f;
		aGlow++;
		aLeft--;
	} while (aLeft != 0);
	if (mUpdateCnt % ((!mTwisting) * 8 + 2) == 0) {
		mGlowStep++;
		if (24 <= mGlowStep)
			mGlowStep = 0;
		float& aLit = mGlowRing[mGlowStep];
		aLit = 1.0f < aLit ? aLit : 1.0f;
		float& anOpposite = mGlowRing[(mGlowStep + 12) % 24];
		anOpposite = anOpposite < 1.0f ? 1.0f : anOpposite;
	}
	if (0 < mCounterFlash) {
		int aNext = mCounterFlash - 2;
		mCounterFlash = aNext < 0 ? 0 : aNext;
	}
	// The lights rise to full, then fade and go
	for (int i = 0; i < (int)mLights.size(); i++) {
		Light& aLight = mLights[i];
		if (aLight.mRising) {
			float aNext = aLight.mRiseRate + aLight.mLevel;
			aLight.mLevel = aNext;
			if (1.0f <= aNext) {
				aLight.mLevel = 1.0f;
				aLight.mRising = false;
			}
		} else {
			float aNext = aLight.mLevel - aLight.mFadeRate;
			aLight.mLevel = aNext;
			if (aNext <= 0.0f) {
				mLights.erase(mLights.begin() + i);
				i--;
			}
		}
	}
}

void Board::UpdateCellPoints()
{
	CellPoint* aPoint = mCellPoints;
	int aRows = 7;
	do {
		int aCols = 7;
		do {
			double aWave = sin((double)aPoint->mAngle * 2.0 + 1.570795);
			float aCos = cosf((float)((double)aPoint->mAngle * 4.0));
			double aPulse = sin((double)mUpdateCnt * 0.04) * aWave * (double)0.1f + 1.1;
			float aScale = (float)(sin((double)mUpdateCnt * 0.12) * (double)aCos * 0.15 + aPulse);
			aPoint->mScale = aScale;
			aScale = (float)((double)mShineAmount * (double)aScale + (1.0 - (double)mShineAmount));
			aPoint->mScale = aScale;
			int anIndex = Ftol((double)aPoint->mAngle * 4096.0 / 6.28318) % 4096;
			float anX = (float)((double)gCosTable[anIndex] * (double)aPoint->mDist * (double)aScale);
			aPoint->mX = (float)((double)anX + 42.0);
			float aY = (float)((double)gSinTable[anIndex] * (double)aPoint->mDist * (double)aPoint->mScale);
			aPoint->mY = (float)(42.0 + (double)aY);
			aPoint++;
			aCols--;
		} while (aCols != 0);
		aRows--;
	} while (aRows != 0);
}

void Board::UpdateLevelUp()
{
	if (mLevelUpTilePhase == 0)
		return;
	mSettleWait = 0.15f;
	mCursorAlpha = 0;
	float aShine = (float)((double)mShineAmount - (double)0.015f);
	mShineAmount = aShine;
	if (aShine < 0.0f)
		mShineAmount = 0;
	int aCells = mCols * mRows;
	if (mLevelUpTilePhase < 2) {
		// The tiles turn over in rings from the middle out
		float aLeft = (float)((double)mTileRingWait - (double)0.01f);
		mTileRingWait = aLeft;
		if (aLeft <= 0.0f) {
			mTileRingDistance--;
			int aCol = 0;
			int aDCol = mCols / 2;
			int aDRow0 = mRows / 2;
			if (0 < mCols) {
				do {
					int aRow = 0;
					if (0 < mRows) {
						int aDRow = aDRow0;
						do {
							Cell& aCell = mCells[mCols * aRow + aCol];
							if (!aCell.mLevelUpStarted) {
								float aDist = sqrtf((float)(aDCol * aDCol + aDRow * aDRow));
								if ((float)mTileRingDistance <= aDist) {
									aCell.mLevelUpStarted = true;
									aCell.mLevelUpTurn = 1.0f;
								}
							}
							aRow++;
							aDRow--;
						} while (aRow < mRows);
					}
					aCol++;
					mTileRingWait = 0.3f;
					aDCol--;
				} while (aCol < mCols);
			}
		}
		Cell* aCell = mCells;
		while (aCells != 0) {
			Cell* aThis = aCell;
			aCells--;
			aCell++;
			if (aThis->mLevelUpStarted && 0.0f < aThis->mLevelUpTurn)
				aThis->mLevelUpTurn = (float)((double)aThis->mLevelUpTurn - (double)0.03f);
		}
		if (0.0f < mAlertGlow) {
			float aFade = (float)((double)mAlertGlow - (double)0.015f);
			mAlertGlow = aFade;
			if (aFade <= 0.0f)
				mAlertGlow = 0;
		}
	}
	if (mLevelUpTilePhase == 2) {
		if (mHyperspace != nullptr) {
			if (mHyperspace->mClicked) {
				// The jump is over: the next level's board
				if (!mTipShowing && !mLevelStarted) {
					mLevelStarted = true;
					RemoveWidget(mGoodTube);
					AddWidget(mGoodTube);
				}
				mLevel++;
				if (mLevel == 0 && mTutorial == nullptr)
					mLevel = 1;
				SettleTurn();
				// The original writes "LEVEL++" to the debugger here
				mGoodTube->SetLevel(mLevel);
				if (mGameMode == MODE_BLITZ) {
					if (mApp->mMusicInterface != nullptr)
						mApp->mMusicInterface->FadeOutAll(true, 0.004);
				} else if (mGameMode == MODE_ZEN) {
					NextLevelMusic();
				}
				mBeforeFirstTwist = true;
				if (!mGameLoaded && IsTipShowing() && mTutorial != nullptr)
					mTutorial->Preload();
				NewLevel();
				FillInBlanks(false);
				if (mGameMode == MODE_BLITZ) {
					if (mApp->mPlayPressed) {
						mGoodTube->StartTimer(false, false);
						mPlayState = 3;
					}
					mGoodTube->SetShowTenths(mApp->mPlayPressed);
				}
				ResetLevelValues();
			}
			if (mHyperspace->mClicked) {
				mApp->mWidgetManager->RemoveWidget(mHyperspace);
				mApp->SafeDeleteWidget(mHyperspace);
				mHyperspace = nullptr;
				mLevelUpTilePhase = 3;
				mHyperspaceClicked = true;
				if (gApp->mProfile->HasSeenTip(14)) {
					mCascadePointsOn = true;
					if (mGameMode != MODE_ZEN && (mGameMode < 2 || 3 < mGameMode)) {
						// The next tip, which slides in from the right
						mTipIndex = gApp->mProfile->mReplayCount;
						gApp->mProfile->mReplayCount++;
						int aTip = mTipIndex;
						if (aTip < (int)mTips.size()) {
							mTipText = mTips[aTip];
							mTipSlide.SetCurve(GTS(0x7dd59c));
							mTipX = (float)mApp->mWidth;
						}
					}
				} else {
					mCascadePointsOn = false;
				}
				// 0x6a3e70: waits for the app's saving thread (+0x7e8) to finish; the port saves at once
			}
		}
	} else if (mLevelUpTilePhase == 3) {
		FinishLevelUp();
	}
	MarkDirty();
}

void Board::FinishLevelUp()
{
	mApp->Is3DAccelerated();
	mShineAmount = 1.0f;
	mLevelUpTilePhase = 0;
	mOverlayRedrawDue = true;
	// The original gathers each row's columns into a vector it then frees, and asks each gem for flags 4 and 3 without
	// using the answers; neither has any effect
	mJumpActive = 0;
	mClearBacklogDue = true;
	mJumpDistance = 0;
	mJumpSpeed = 0;
	if (!mGameLoaded)
		MakeCells();
	if (!IsTipShowing() && mGameMode != 5)
		mLevelStarted = true;
	mLevelPrepared = true;
	mBoardShown = true;
	MouseMove(mApp->mWidgetManager->mLastMouseX, mApp->mWidgetManager->mLastMouseY);
}

void Board::PlayGameMusic()
{
	if (mApp->mMusicInterface != nullptr)
		mApp->mMusicInterface->StopAllMusic();
	mApp->PlayMusic(300, false, false, mMusicPositionLow, mMusicPositionHigh);
	mMusicPositionLow = 0;
	mMusicPositionHigh = 0;
}

void Board::LevelComplete()
{
	if (IsBusy())
		return;
	mApp->FadeOutMusic();
	if (mLevel == 0) {
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece != nullptr)
					aPiece->ClearSpecial();
			}
		}
	}
	if (mGameMode == MODE_ZEN) {
		// Zen has no jump: the next look, and the next level after a wait
		mApp->mVizManager->NewLevel();
		if (0 < mLevel) {
			ShowLevelUp();
			mBonusLevel = GetBonusLevel(mLevel);
			ShowRankBonus(mBonusLevel);
		}
		mNextLevelWait = mApp->mVizManager->m2D ? 3.5f : 11.0f;
		mTubeSlide.SetCurve(GTS(0x7dc580));
		return;
	}
	mSoulFade = 0;
	mGemsBrokenAtTurnStart = mGemsBroken;
	mGemsClearedAtTurnStart = mGemsCleared;
	mChainLength = 0;
	mFruitBrokeThisTurn = false;
	mGameOverRow = 0;
	mFallHeld = true;
	int aSize = mRows;
	if (mRows <= mCols)
		aSize = mCols;
	mTileRingWait = 0;
	mBarTopAlpha = 1.0f;
	mTileRingDistance = aSize / 2;
	mJumpFillWait = 7.9f;
	mJumpStage = 1;
	mTipShowingAtJump = mTipShowing;
	if (0 < mLevel) {
		ShowLevelUp();
		if (!mTipShowingAtJump)
			ShowRankBonus(mBonusLevel);
	}
	TakeHyperspace(nullptr);
	// (the trial's jump, transition 14, which ends an unregistered game after level 10 of classic or 15 of zen, is
	// registration code, left out)
	mHyperspace->SetTransition(8);
	mTwistTrailFade = 1.0f;
	int aLeft = mCols * mRows;
	Piece** aCell = mGrid;
	while (aLeft != 0) {
		Piece* aPiece = *aCell;
		aLeft--;
		aCell++;
		if (aPiece != nullptr) {
			if (aPiece->mIsTwisting) {
				aPiece->mIsTwisting = false;
				aPiece->mX = (float)aPiece->mTwistStartX;
				aPiece->mY = (float)aPiece->mTwistStartY;
			}
			if (aPiece->mBreaking || (aPiece->IsFlagSet(Piece::FLAG_FRUIT) && mFruitsBlast))
				aPiece->mBlasted = true;
			if (mLockedGemsBusy && (aPiece->IsFlagSet(Piece::FLAG_DROPPER) || aPiece->IsFlagSet(Piece::FLAG_LOCKED)))
				aPiece->mBusyTime = 1;
		}
	}
}

void Board::UpdateJump()
{
	// The level's end (LevelComplete sets mJumpStage to 1): the "level complete" show (1) or none (2), then the rows
	// break up one by one as the hyperspace runs, every gem left becoming a soul; then (4, 0) the next level
	if (mJumpStage == 0) {
		if (mShownDialog == nullptr) {
			mJumpFillWait = (float)((double)mJumpFillWait - 0.01);
			for (unsigned i = 0; i < mLevelSouls.size(); i++) {
				mLevelSouls[i]->mRectY = Ftol((double)mHeight * 0.30000001192092896);
				mLevelSouls[i]->mRectHeight = Ftol((double)mHeight * 0.5);
			}
			ClearShockwaves();
			EndJump();
		}
		SettlePieces();
		return;
	}
	if (mJumpStage == 1) {
		if (mLevelUpShow == nullptr) {
			mJumpStage = 2;
		} else {
			int aShown = mLevelUpShow->mUpdateCnt;
			if ((350 <= aShown && !mTipShowingAtJump) || (200 <= aShown && mTipShowingAtJump)) {
				mApp->PlaySample(SOUND_WHIRLPOOL1);
				mJumpStage = 4;
			}
		}
	} else if (mJumpStage != 2) {
		if (mJumpStage == 4)
			mJumpStage = 0;
		SettlePieces();
		return;
	}
	// The row breaking up now
	int aRow = (mHyperspace->mUpdateCnt - 120) / 12;
	if (mTipShowingAtJump)
		aRow = (mHyperspace->mUpdateCnt - 45) / 12;
	if (aRow == mRows) {
		mJumpStage = 0;
		SettlePieces();
		return;
	}
	bool aSounded = false;
	for (int aCol = 0; aCol < mCols; aCol++) {
		Piece* aPiece = GetPieceAt(aRow, aCol);
		if (aPiece == nullptr)
			continue;
		aPiece->mStrikeDelay = 0;
		if (aPiece->IsFlagSet(Piece::FLAG_POWER) || aPiece->IsFlagSet(Piece::FLAG_FRUIT) || aPiece->IsFlagSet(Piece::FLAG_ELECTRO) || aPiece->IsFlagSet(Piece::FLAG_ICE))
			continue;
		if (aPiece->IsFlagSet(Piece::FLAG_LOCKED) || aPiece->IsFlagSet(Piece::FLAG_DROPPER) || aPiece->IsFlagSet(Piece::FLAG_DEVIL) || aPiece->IsFlagSet(Piece::FLAG_BOMB)) {
			float aY = (float)(gGemHeight / 2) + aPiece->mY;
			float aX = (float)(gGemWidth / 2) + aPiece->mX;
			BlowUpPiece(aPiece, aX, aY, true, false);
		} else if (!aPiece->mBreaking) {
			if (!aSounded) {
				aSounded = true;
				mApp->PlaySample(SOUND_ROWBLOW);
			}
			aPiece->mBreaking = true;
			if (aPiece->IsFlagSet(Piece::FLAG_COAL))
				aPiece->mBreakFrame = 40;
		}
	}
	// One gem of the row that is not breaking leaves as a soul (the row from 120 updates on, also after the show)
	int aSoulRow = (mHyperspace->mUpdateCnt - 120) / 12;
	for (int aCol = 0; aCol < mCols; aCol++) {
		Piece* aPiece = GetPieceAt(aSoulRow, aCol);
		if (aPiece == nullptr || aPiece->mBreaking)
			continue;
		GemSoul* aSoul = new GemSoul(aPiece->mFlags);
		float aX = (float)(gGemWidth / 2) + aPiece->mX;
		aSoul->mX = aX;
		aSoul->mTargetX = aX;
		float aY = (float)(gGemHeight / 2) + aPiece->mY;
		aSoul->mY = aY;
		aSoul->mTargetY = aY;
		aSoul->mColor = aPiece->mColor;
		aSoul->mPiece = aPiece;
		aSoul->mRectX = Ftol((double)mWidth * 0.44999998807907104);
		aSoul->mRectY = Ftol((double)mHeight * 0.20000000298023224);
		aSoul->mRectWidth = Ftol((double)mWidth * 0.10000000149011612);
		aSoul->mRectHeight = Ftol((double)mHeight * 0.009999999776482582);
		aSoul->mFirst = mLevelSouls.empty();
		mLevelSouls.push_back(aSoul);
		aSoul->Update();
		mGrid[mCols * aSoulRow + aCol] = nullptr;
		float aLeft = 0.33f;
		if (0.33f < mJumpFillWait)
			aLeft = mJumpFillWait;
		mJumpFillWait = aLeft;
		mSoulFade = 0.01f;
		break;
	}
	SettlePieces();
}

void Board::ClearShockwaves()
{
	for (unsigned i = 0; i < mShockwaves.size(); i++)
		delete mShockwaves[i];
	mShockwaves.clear();
}

void Board::EndJump()
{
	if (mReplayPlaying) {
		mWidgetFlagsMod.mRemoveFlags &= ~WIDGETFLAGS_ALLOW_MOUSE;
		mReplayPlaying = false;
	}
	// (the original asks the app whether it draws with Direct3D, 0x4a7900, and keeps nothing of it)
	mCountPopupOn = true;
	mCascadePopupOn = true;
	ClearSparkStarts();
	mJumpFillWait = 0;
	mApp->mProfile->mAchievementBits |= 1 << 0;  // 0x733774
	mJumpActive = 0;
	mJumpScroll = 0;
	mJumpEnded = false;
	mCursorPiece = nullptr;
	mClearSparksDue = true;
	mJumpDistance = 0;
	mJumpSpeed = 0;
	mApp->ClearUpdateBacklog(false);
	mClearedAtGameEnd = 0;
	mTurnPoints = 0;
	mLevelProgress = 0;
}

void Board::ShowLevelUp()
{
	mApp->PlayVoice(SOUND_VOICE_LEVELCOMPLETE, false);
	LevelUpShow* aShow = new LevelUpShow(this);
	mLevelUpShow = aShow;
	aShow->mZOrder = mGoodTube->mZOrder;
	mApp->mWidgetManager->AddWidget(mLevelUpShow);
}

bool Board::UpdateShockwaves()
{
	int aLeft = mCols * mRows;
	Piece** aCell = mGrid;
	while (aLeft != 0) {
		Piece* aPiece = *aCell;
		aLeft--;
		aCell++;
		if (aPiece != nullptr) {
			aPiece->mShakeY = 0;
			aPiece->mShakeX = 0;
		}
	}
	unsigned int i = 0;
	if (!mShockwaves.empty()) {
		do {
			Shockwave* aWave = mShockwaves[i];
			aWave->Update();
			if (aWave->mDone) {
				// The last takes its place, and waits an update (the original goes on to the index after)
				delete aWave;
				mShockwaves[i] = mShockwaves[mShockwaves.size() - 1];
				if (!mShockwaves.empty())
					mShockwaves.pop_back();
			}
			i++;
		} while (i < mShockwaves.size());
	}
	return !mShockwaves.empty();
}

bool Board::UpdateSmokeAndGrowth()
{
	unsigned int i = 0;
	if (!mSmokeTravellers.empty()) {
		do {
			Traveller* aTraveller = mSmokeTravellers[i];
			aTraveller->Update();
			if (aTraveller->mDone) {
				// (the next one waits an update: the original goes on to the index after)
				mSmokeTravellers.erase(mSmokeTravellers.begin() + i);
				delete aTraveller;
			}
			i++;
		} while (i < mSmokeTravellers.size());
	}
	int aLeft = mCols * mRows;
	int aGrowing = 0;
	Piece** aCell = mGrid;
	if (0 < aLeft) {
		do {
			Piece* aPiece = *aCell;
			if (aPiece != nullptr && aPiece->IsFlagSet(Piece::FLAG_DROPPER) && aPiece->mBirthScale < 1.0f)
				aGrowing++;
			aCell++;
			aLeft--;
		} while (aLeft != 0);
	}
	return 0 < (int)mSmokeTravellers.size() + aGrowing;
}

void Board::CheckSettled()
{
	bool aShaking = UpdateShockwaves();
	bool aGrowing = UpdateSmokeAndGrowth();
	bool aStill = !aGrowing && !aShaking;
	int anIndex = 0;
	int aRow = 0;
	if (0 < mRows) {
		do {
			int aCol = 0;
			if (0 < mCols) {
				do {
					Piece* aPiece = mGrid[anIndex];
					anIndex++;
					if (aPiece != nullptr) {
						if (!aPiece->mBlasted) {
							if (0.0f < aPiece->mPendingTime)
								aStill = false;
							if (aPiece->mBreaking) {
								aStill = false;
								if (mInstantTwist)
									aPiece->mBlasted = true;
							}
						} else {
							aStill = false;
						}
					}
					aCol++;
				} while (aCol < mCols);
			}
			aRow++;
		} while (aRow < mRows);
	}
	MarkDirty();
	if (aStill && mHintsReady) {
		if (!mSettling) {
			if (!IsBusy() && mElectroStrikes.empty() && !mHintShowing && mFruitDue) {
				mFruitDue = false;
				// The multiplier went past its top (SetLevelSteps) or a fruit dance ended: a fruit (AwardFruit),
				// and steps past 151 (at most 14) go on top of the steps of the levels below the board's
				AwardFruit(-1, -1);
				if (152 <= mLevelStepCount) {
					int aSteps = mLevelStepCount - 151;
					if (14 < aSteps)
						aSteps = 14;
					mLevelStepCount = aSteps;
					int aLevel = 1;
					if (1 < mStepLevel) {
						do {
							mLevelStepCount += GetLevelSteps(aLevel);
							aLevel++;
						} while (aLevel < mStepLevel);
					}
					SetLevelSteps(mLevelStepCount);
				}
			}
		} else {
			Refill();
		}
	}
}

bool Board::SettlePieces()
{
	bool aAllDone = true;
	bool anExploded = false;
	int aRow = 0;
	if (0 < mRows) {
		do {
			int aCol = 0;
			if (0 < mCols) {
				do {
					Piece* aPiece = GetPieceAt(aRow, aCol);
					if (aPiece != nullptr) {
						if (!aPiece->mIsSettled)
							aAllDone = false;
						if (0 < aPiece->mBusyTime) {
							if (aPiece->mHasBroken) {
								aPiece->mBusyTime = 0;
								aCol++;
								continue;
							}
							if (aPiece->IsFlagSet(Piece::FLAG_BOMB) && aPiece->mBombCount == 0) {
								// a spent bomb shakes as its fuse runs down
								float aShake = (float)((double)(float)(100 - aPiece->mBusyTime) / 5.0);
								if (1.0f < aShake)
									aShake = 1.0f;
								aPiece->mAura = aShake;
								aPiece->mAuraAngle = (float)((double)RandSigned() * 3.1415927410125732);
							}
							aPiece->mBusyTime--;
							if (aPiece->mBusyTime != 0) {
								aAllDone = false;
							} else {
								// the gem goes off
								float aMidX = (float)(gGemWidth / 2) + aPiece->mX;
								float aMidY = (float)(gGemHeight / 2) + aPiece->mY;
								bool aBreak = false;
								if (aPiece->IsFlagSet(Piece::FLAG_FRUIT)) {
									if (!mHintsReady || mHintShowing)
										aPiece->mBusyTime = 1;
									else
										aBreak = true;
								} else if (aPiece->IsFlagSet(Piece::FLAG_DROPPER)) {
									aBreak = true;
								} else if (aPiece->IsFlagSet(Piece::FLAG_ELECTRO)) {
									BreakPiece(aPiece);
									AddPoints(Ftol((double)(gGemWidth / 2) + (double)aPiece->mX),
										Ftol((double)(gGemHeight / 2) + (double)aPiece->mY), mBurstSize,
										gGemColors[aPiece->mColor], aPiece->mMatchSerial, true, true, false, true);
									AddBlastChain();
								} else if (aPiece->IsFlagSet(Piece::FLAG_POWER)) {
									if (!aPiece->mHasBurst) {
										aPiece->mHasBurst = true;
										AddPoints(Ftol((double)(gGemWidth / 2) + (double)aPiece->mX),
											Ftol((double)(gGemHeight / 2) + (double)aPiece->mY), mBurstSize,
											gGemColors[aPiece->mColor], aPiece->mMatchSerial, true, true, false, true);
										AddBlastChain();
										AddStat(0x40, 1);
										mGameGemCount++;
										mLevelGemCount++;
									}
									AddStat(aPiece->mColor + 0x4b, 1);
									AddStat(0x3e, 1);
									SetOffPowerGem(aPiece);
								} else if (aPiece->IsFlagSet(Piece::FLAG_BOMB)) {
									BlowUpPiece(aPiece, aMidX, aMidY, true, false);
									if (!mIsReplay)
										mFXManager->AddBombFlash(aMidX, aMidY, gGemColors[aPiece->mColor]);
									aPiece->mBlasted = true;
									aPiece->mFruitCounted = true;
									anExploded = true;
									aBreak = true;
								} else if (aPiece->IsFlagSet(Piece::FLAG_COAL)) {
									aBreak = true;
								} else {
									BlowUpPiece(aPiece, aMidX, aMidY, true, false);
									AddPoints(Ftol((double)(gGemWidth / 2) + (double)aPiece->mX),
										Ftol((double)(gGemHeight / 2) + (double)aPiece->mY), 10,
										gGemColors[aPiece->mColor], aPiece->mMatchSerial, true, true, false, true);
									anExploded = true;
									aBreak = true;
								}
								if (aBreak)
									BreakPiece(aPiece);
								if (mInputLock == 0)
									Refill();
							}
						}
						if (0.0f < aPiece->mGrowth) {
							// +0x284 grows by 0.05 and past 1 makes a 0x104-byte object (0x74b46e). Never reached: +0x284 is
							// only ever written 0 (0x72c1fa, 0x69baaf), copied (0x6bc62e) or grown here when above 0
							NotPorted("Board::SettlePieces: +0x284");
						}
					}
					aCol++;
				} while (aCol < mCols);
			}
			aRow++;
		} while (aRow < mRows);
		if (anExploded) {
			mApp->PlaySample(SOUND_BOMB_EXPLODE);
			ShowCountPopup();
		}
	}
	if (!mShockwaves.empty()) {
		UpdateShockwaves();
		aAllDone = false;
	}
	if (aAllDone)
		mSpeedNow = 1.0f;
	return aAllDone;
}

void Board::FallPieces()
{
	if (mFallHeld)
		return;
	if (0 < mFallDelay) {
		mFallDelay--;
		return;
	}
	if (2 <= mLevelUpTilePhase && mHyperspace != nullptr && mHyperspace->mTransition != 8 && mHyperspace->mTransition != 14)
		return;
	if (mStartState != 1)
		return;
	bool aStill = SettlePieces();
	if (0 < mLandingBlock)
		return;
	int aLanded = 0;
	int aLandedX = 0;
	int aRow = mRows - 1;
	if (-1 < aRow) {
		do {
			int aCol = 0;
			if (0 < mCols) {
				do {
					Piece* aPiece = GetPieceAt(aRow, aCol);
					if (aPiece == nullptr)
						goto next;
					if (aPiece->IsFlagSet(Piece::FLAG_DROPPER) && aPiece->mBirthScale < 1.0f)
						aStill = false;
					if (aPiece->mIsTwisting)
						goto next;
					// The gem moves down into the gaps under it
					while (aPiece->mRow < mRows - 1) {
						int aFrom = aPiece->mRow;
						if (GetPieceAt(aFrom + 1, aPiece->mCol) != nullptr)
							break;
						mGrid[mCols * aFrom + aPiece->mCol] = nullptr;
						aPiece->mRow++;
						mGrid[mCols * aPiece->mRow + aPiece->mCol] = aPiece;
					}
					{
						float aTarget;
						if (aPiece->mIsOff)
							aTarget = (float)(mHeight + gGemHeight);
						else
							aTarget = (float)GetRowY(aRow);
						if (!(aTarget <= aPiece->mY) || aPiece->mFallSpeed < 0.0f) {
							mFallMatchDue = true;
							bool anOff = aPiece->mIsOff;
							aStill = false;
							if (anOff) {
								aPiece->mX = aPiece->mPendingDX + aPiece->mX;
								aPiece->mY = aPiece->mY + aPiece->mFallSpeed;
							} else if (aPiece->mPendingTime != 0.0f) {
								aPiece->mFallSpeed = 0;
							} else {
								float aY = aPiece->mY + aPiece->mFallSpeed;
								aPiece->mY = aY;
								// It stops on a falling gem under it and takes its speed
								if (aPiece->mRow < mRows - 1) {
									Piece* aBelow = GetPieceAt(aPiece->mRow + 1, aPiece->mCol);
									if (aBelow != nullptr && !aBelow->mIsOff) {
										double aLimit = (double)aBelow->mY - (double)gGemHeight;
										if (!((double)aY < aLimit) && 0.0f < aPiece->mFallSpeed && aBelow->mIsCounted) {
											aPiece->mY = (float)aLimit;
											aPiece->mFallSpeed = aBelow->mFallSpeed;
										}
									}
								}
							}
							if (!anOff) {
								if (aPiece->mFallSpeed < 0.0f) {
									// Moving up: the gems over it go no faster
									int anAbove = aPiece->mRow;
									while (anAbove = anAbove - 1, -1 < anAbove) {
										Piece* anOther = GetPieceAt(anAbove, aPiece->mCol);
										if (anOther != nullptr && anOther->mY < aPiece->mY)
											anOther->mFallSpeed = aPiece->mFallSpeed <= anOther->mFallSpeed ? aPiece->mFallSpeed : anOther->mFallSpeed;
									}
								} else if (!aPiece->mBreaking) {
									// The gems over it stay a cell above it
									int anAbove = aPiece->mRow - 1;
									if (-1 < anAbove) {
										do {
											Piece* anOther = GetPieceAt(anAbove, aPiece->mCol);
											int aLimit = Ftol((double)aPiece->mY - (double)gGemHeight);
											if (anOther != nullptr && !anOther->mBreaking && (float)aLimit < anOther->mY)
												anOther->mY = (float)aLimit;
											anAbove--;
										} while (-1 < anAbove);
									}
								}
							}
							if (aTarget <= aPiece->mY) {
								// Landed
								if (aPiece->mIsOff)
									goto next;
								if (0.0f <= aPiece->mFallSpeed) {
									if (aPiece->mHasBroken)
										aPiece->mHasBroken = false;
									aPiece->mStrikeDelay = 0;
									aPiece->mHasBurst = false;
								}
								aPiece->mFallSpeed = 0;
								aPiece->mLastRow = aPiece->mRow;
								if (!aPiece->IsFlagSet(Piece::FLAG_FRUIT)) {
									aLanded++;
									if (!aPiece->mFirstDropDone && mGameMode != MODE_PUZZLE) {
										aPiece->mFirstDropDone = true;
										if (aPiece->IsFlagSet(Piece::FLAG_BOMB)) {
											aPiece->mJustMade = 4;
											mApp->PlaySample(SOUND_GEM_COUNTDOWN);
										} else if (aPiece->IsFlagSet(Piece::FLAG_DROPPER)) {
											aPiece->mJustMade = 5;
										}
									}
								}
								int anX = Ftol(aPiece->mX);
								aPiece->mY = aTarget;
								aLandedX = gGemWidth / 2 + aLandedX + anX;
								aPiece->mFallSpeed = 0;
							} else if (aPiece->mTurnAmount != 0.0f || aPiece->mBreaking) {
								aPiece->mFallSpeed = 0;
							} else {
								// Gravity
								float anAccel = (float)((double)gApp->mScale * 0.375);
								float aSpeed = anAccel + aPiece->mFallSpeed;
								aPiece->mFallSpeed = aSpeed;
								if (!((float)gGemHeight > aSpeed))
									aPiece->mFallSpeed = (float)(gGemHeight - Ftol(gApp->mScale));
							}
						}
					}
				next:
					aCol++;
				} while (aCol < mCols);
			}
			aRow--;
		} while (-1 < aRow);
		if (0 < aLanded && 9 < abs(mLastLandSound - mUpdateCnt)) {
			mLastLandSound = mUpdateCnt;
			mApp->PlaySoundEx(SOUND_GEM_HIT, GetPan(aLandedX / aLanded), 0.0f, 0.0f, true, true);
		}
	}
	if (aStill && mFallMatchDue) {
		for (int r = 0; r < mRows; r++) {
			for (int c = 0; c < mCols; c++) {
				Piece* aPiece = GetPieceAt(r, c);
				if (aPiece != nullptr)
					aPiece->mBurstRow = aPiece->mRow;
			}
		}
		mFallMatchDue = false;
		MatchAfterMove();
	}
	bool aWasMoving = mHintShowing;
	mHintShowing = !aStill;
	if (!aStill)
		mHintsReady = false;
	else if (aWasMoving)
		gSettleCount++;
	MarkDirty();
}

int Board::GetGridRight()
{
	return Ftol((double)gGemWidth * (double)0.24f + (double)(mCols * gGemWidth));
}

float Board::GetGridMidY()
{
	return (float)(mRows * gGemHeight / 2 - GetRowY(0) + GetRowY(0));
}

void Board::UpdateFruitBlast()
{
	// 0x6c4466: a broken fruit gem's blast (BreakPiece starts it). Four rings spread from its cell over 0.85 of
	// +0x248's run: the gems of its colour they cross break (another fruit becomes a power gem and goes off), bombs and
	// devil gems count up and locks break; then the board refills, and at 1 the turn settles
	if (mFruitBlast == 0.0f)
		return;
	bool aAll = false;
	float anOld = mFruitBlast;
	float aNew = (float)((double)mFruitBlast + 0.0020000000949949026);
	mFruitBlast = aNew;
	if (anOld < 0.5f && 0.5f <= aNew) {
		// Half way: the fruit's own points
		AddPoints(GetColX(mFruitCol) + gGemWidth / 2, GetRowY(mFruitRow) + gGemHeight / 2, 500,
			gGemColors[mFruitColor], -0x68, true, true, false, true);
	}
	if (0.8500000238418579 < (double)mFruitBlast) {
		if (1.0f <= mFruitBlast) {
			mFruitBlast = 0;
			SettleTurn();
		}
		int anAmnesty = mAmnestyGemCount;
		mAmnestyGemCount = 0;
		mFallHeld = false;
		if (DropPieces())
			FillInBlanks(false);
		mAmnestyGemCount = anAmnesty;
		aAll = true;
	} else {
		mHintsReady = false;
		mFallHeld = true;
		bool aSound = false;
		int aRow = 0;
		if (0 < mRows) {
			do {
				int aCol = 0;
				if (0 < mCols) {
					do {
						float aDX = (float)(aCol - mFruitCol);
						float aDY = (float)(aRow - mFruitRow);
						Piece* aPiece = GetPieceAt(aRow, aCol);
						if (aPiece != nullptr && 0.0f <= aPiece->mY) {
							aPiece->mFallSpeed = 0;
							float aDist = sqrtf((float)((double)aDX * (double)aDX + (double)aDY * (double)aDY));
							int aRing = 0;
							do {
								float aT = (float)((double)aRing * -0.15 + (double)mFruitBlast * 2.799999952316284 -
									0.5);
								if (aRing == 0) {
									if (aT < 0.0f) {
										aPiece->mJustMade = 0;
										aPiece->mDevilGlow = 0;
									}
								} else if (aRing == 3) {
									aT = (float)(((double)aT - 0.30000001192092896) * 0.8500000238418579);
								}
								mBlastRingSize.SetInVal(aT);
								mBlastRingAlpha.SetInVal(aT);
								float aRadius = (float)(mBlastRingSize.GetOutVal() * 4.400000095367432);
								if (fabsf((float)(aDist - aRadius)) < 0.3f) {
									// The ring crosses this cell
									if (aPiece->mColor == mFruitColor && !aPiece->IsFlagSet(Piece::FLAG_COAL) &&
										aPiece->mId != mFruitId && !aPiece->IsFlagSet(Piece::FLAG_DEVIL)) {
										if (aRing == 3) {
											float aX = (float)((double)(gGemWidth / 2) + (double)aPiece->mX);
											float aY = (float)((double)(gGemHeight / 2) + (double)aPiece->mY);
											if (aPiece->IsFlagSet(Piece::FLAG_FRUIT)) {
												aPiece->SetFlag(Piece::FLAG_POWER, true);
												aPiece->ClearFlag(Piece::FLAG_FRUIT);
												BlowUpPiece(aPiece, aX, aY, true, false);
												AddPoints(Ftol((double)(gGemWidth / 2) + (double)aPiece->mX),
													Ftol((double)(gGemHeight / 2) + (double)aPiece->mY), 1000,
													gGemColors[aPiece->mColor], -0x68, true, true, false, true);
												AddStat(0x45, 1);
											} else {
												bool aSparks = true;
												if (aPiece->IsSpecial()) {
													aPiece->mHasBroken = false;
													BreakPiece(aPiece);
													if (aPiece->IsFlagSet(Piece::FLAG_ELECTRO)) {
														aPiece->mBreaking = false;
														aSparks = false;
													}
												} else {
													BlowUpPiece(aPiece, aX, aY, true, false);
												}
												if (aSparks) {
													gApp->PlaySample(SOUND_ROWBLOW);
													AddPoints(Ftol((double)(gGemWidth / 2) + (double)aPiece->mX),
														Ftol((double)(gGemHeight / 2) + (double)aPiece->mY), 10,
														gGemColors[aPiece->mColor], -0x68, true, true, false, true);
													// 20 sparks fly out, pushed away from the fruit
													float anAway = atan2f(aDY, aDX);
													int aLeft = 20;
													do {
														Effect* anEffect = gBoard->mFXManager->AllocEffect(12);
														float aMin = gApp->mScale;
														float aRand = fabsf(RandSigned());
														float aRange = gApp->mScale + gApp->mScale;
														float aSpeed = (float)((double)aRange * (double)aRand + (double)aMin);
														float anAngle = (float)((double)RandSigned() * 3.1415927410125732);
														anEffect->mScale = (float)((double)RandSigned() * 0.5 + 1.0);
														float aCos = cosf(anAngle);
														float aCosAway = cosf(anAway);
														float aPush = gApp->mScale + gApp->mScale;
														anEffect->mDX = (float)((double)aPush * (double)aCosAway +
															(double)aCos * (double)aSpeed);
														float aSin = sinf(anAngle);
														float aSinAway = sinf(anAway);
														aPush = gApp->mScale + gApp->mScale;
														anEffect->mDY = (float)((double)aPush * (double)aSinAway +
															(double)aSin * (double)aSpeed);
														aCos = cosf(anAngle);
														anEffect->mX = (float)((double)gGemWidth * (double)aCos * 0.5 +
															((double)(gGemWidth / 2) + (double)aPiece->mX));
														aSin = sinf(anAngle);
														anEffect->mY = (float)((double)gGemHeight * (double)aSin * 0.5 +
															((double)(gGemHeight / 2) + (double)aPiece->mY));
														anEffect->mIsAdditive = true;
														anEffect->mAlpha = 1.0f;
														anEffect->mDAlpha = -0.005f;
														anEffect->mGravity = 0;
														gBoard->mFXManager->AddEffect(anEffect);
														aLeft--;
													} while (aLeft != 0);
												}
											}
										} else {
											aPiece->mAura = (float)((double)(aRing + 1) * 0.30000001192092896);
										}
									}
									if ((aPiece->IsFlagSet(Piece::FLAG_BOMB) || aPiece->IsFlagSet(Piece::FLAG_DEVIL)) &&
										aPiece->mDevilGlow < 0.6f) {
										// A bomb or devil gem counts up by the ring's number
										aPiece->mBombCount += aRing + 1;
										aPiece->MakeCountImage();
										aPiece->mDevilGlow = 1.0f;
										aPiece->mJustMade = 1;
										aPiece->mCountSizeCurve.SetCurve(GTS(0x7dd424));
										aSound = true;
									} else if (aRing == 3 && !aPiece->IsFlagSet(Piece::FLAG_DOOM)) {
										if (aPiece->IsFlagSet(Piece::FLAG_LOCKED))
											aPiece->BreakLock(false);
										aPiece->mLockState = 0;
									}
								}
								aRing++;
							} while (aRing < 4);
						}
						aCol++;
					} while (aCol < mCols);
				}
				aRow++;
			} while (aRow < mRows);
		}
		if (aSound)
			gApp->PlaySoundEx(SOUND_BLITZ_COUNTUP, 0, (float)((double)mFruitBlast * 1.5), 0.0f, true, true);
		// The fruit itself fades as it grows
		Piece* aFruit = GetPieceById(mFruitId);
		if (aFruit != nullptr) {
			mFruitGlowAlpha.SetInVal(mFruitBlast);
			mFruitBlastScale.SetInVal(mFruitBlast);
			double anAlpha = 1.0 - ((double)mFruitBlast - 0.4000000059604645) * 6.0;
			aFruit->mAlpha = (float)(1.0 < anAlpha ? 1.0 : anAlpha);
			aFruit->mDrawScale = (float)mFruitBlastScale.GetOutVal();
			if (!(0.0f < aFruit->mAlpha))
				aFruit->mBlasted = true;
		}
	}
	// The gems the blast can reach (its colour, bombs, locks and devil gems) are drawn over its darkening
	// (DrawOverlay) and the rest under it; once the rings are done, all of them under it again
	int aCount = mCols * mRows;
	if (0 < aCount) {
		Piece** aCell = mGrid;
		do {
			Piece* aPiece = *aCell;
			if (aPiece != nullptr) {
				if (aAll)
					aPiece->mIsDrawn = true;
				else
					aPiece->mIsDrawn = !(aPiece->mColor == mFruitColor || aPiece->IsFlagSet(Piece::FLAG_BOMB) ||
						aPiece->IsFlagSet(Piece::FLAG_LOCKED) || aPiece->IsFlagSet(Piece::FLAG_DEVIL));
			}
			aCell++;
			aCount--;
		} while (aCount != 0);
	}
}
void Board::PopAnimCommand(int theId, const std::string& theCommand, const std::string& theParam)
{
	// 0x6c3cf5: the fruit dance's points. Its "small c x y" and "large c x y" commands pop up the next points of
	// their list in gem colour c at x, y in the animation (at the art's scale)
	static const int kSmall[42] = {100, 200, 200, 200, 300, 300, 300, 300, 300, 400, 400, 400, 400, 400, 100, 100,
		100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
		100, 100, 100, 100};
	static const int kLarge[15] = {1000, 2000, 3000, 4000, 5000, 10000, 10000, 10000, 10000, 10000, 0, 0, 0, 0, 0};
	std::string aPos;
	bool isSmall = true;
	int aPoints;
	if (theCommand.substr(0, 5) == GTS(0x7dd41c)) {
		// port: past the end of its list the original reads whatever is beside it on the stack; the port gives no
		// points (fruitdance.pam sends 37 of these)
		int anIndex = mFruitDanceSmallPoints;
		mFruitDanceSmallPoints = anIndex + 1;
		aPoints = (anIndex < 42 ? kSmall[anIndex] : 0) / 2;
		aPos = theCommand.substr(6);
	} else if (theCommand.substr(0, 5) == GTS(0x7dd414)) {
		// (likewise; fruitdance.pam sends 15)
		int anIndex = mFruitDanceLargePoints;
		mFruitDanceLargePoints = anIndex + 1;
		isSmall = false;
		aPoints = (anIndex < 15 ? kLarge[anIndex] : 0) / 2;
		aPos = theCommand.substr(6);
		if (aPoints == 0)
			return;
	} else {
		return;
	}
	if (aPoints <= 0)
		return;
	size_t aFirst = aPos.find(' ', 0);
	size_t aSecond = aPos.find(' ', aFirst + 1);
	int aColor = atol(aPos.substr(0, aFirst).c_str());
	int aX = Ftol((double)gApp->mScale * (double)atol(aPos.substr(aFirst + 1, aSecond - aFirst - 1).c_str()));
	int aY = Ftol((double)gApp->mScale * (double)atol(aPos.substr(aSecond + 1).c_str()));
	Points* aFloater = AddPoints(mFruitDance->mX + aX, mFruitDance->mY + aY, aPoints, gGemColors[aColor], -0x68,
		true, false, false, false);
	// port: the original writes to the floater without a check (AddPoints gives none in a challenge that does not
	// count points)
	if (aFloater == nullptr)
		return;
	aFloater->mStopHigh = false;
	if (isSmall) {
		aFloater->mScale = (float)((double)aFloater->mScale + 0.25);
		aFloater->mTargetScale = (float)((double)aFloater->mTargetScale + 0.25);
		aFloater->mLife = (float)((double)aFloater->mLife + 0.0);
	} else {
		aFloater->mTargetScale = (float)((double)aFloater->mTargetScale * 1.5);
		aFloater->mLife = (float)((double)aFloater->mLife + 0.4000000059604645);
	}
}


void Board::UpdateHint()
{
	// 0x6aff84
	int aState = mHintState;
	if (aState == 1) {
		// the hint's gems flash one after another
		for (unsigned int i = 0; i < mHintGems.size(); i++) {
			Piece* aPiece = GetPieceById(mHintGems[i]);
			if (aPiece != nullptr)
				aPiece->mRingDelay = (float)((double)(float)i * 0.25 + 0.001);
		}
		mHintState = 0;
		mHintWait = 0;
	} else if ((unsigned int)(aState - 2) < 2) {
		// sparkles circle the hint's cell for a turn (state 3 then flashes the gems)
		float aTurn = (float)((double)kTwoPi * (double)0.008f + (double)mHintSparkleTurn);
		mHintSparkleTurn = aTurn;
		if (kTwoPi <= aTurn) {
			mHintSparkleTurn = 0;
			if (aState == 3) {
				mHintState = 1;
			} else {
				mHintState = 0;
				mHintWait = 0;
				mHintSparkleTurn = 0;
			}
		}
		float anAngle = mHintSparkleTurn;
		if (mUpdateCnt % 2 == 0) {
			EffectsManager* aFXManager = mApp->mBoard->mFXManager;
			for (int aSide = 2; aSide != 0; aSide--) {
				Effect* anEffect = gApp->mBoard->mFXManager->AllocEffect(0x11);
				RandSigned();
				float aCos = cosf(anAngle);
				float aX = (float)((double)(GetColX(mHintCol) + gGemWidth) +
					(double)gGemWidth * (double)1.25f * (double)aCos);
				float aSin = sinf(anAngle);
				float aY = (float)((double)(GetRowY(mHintRow) + gGemHeight) +
					(double)gGemHeight * (double)1.25f * (double)aSin);
				anEffect->mDAlpha = -0.01f;
				anEffect->mX = aX;
				anEffect->mY = aY;
				anEffect->mDY = (float)((double)anEffect->mDY * 0.0);
				anEffect->mScale = 1.0f;
				float aRand = RandSigned();
				anEffect->mDY = (float)((double)fabsf(aRand) * (double)0.3f);
				anEffect->mIsAdditive = true;
				anEffect->mAngle = (float)((double)RandSigned() * (double)3.1415927f);
				aFXManager->AddEffect(anEffect);
				// and two trails of 15 behind it
				float aTrail = (float)((double)anAngle - (double)1.5707964f);
				aTrail = (float)((double)aTrail + (double)0.62831855f);
				for (int aCount = 2; aCount != 0; aCount--) {
					for (int i = 0; i < 0xf; i++) {
						anEffect = gApp->mBoard->mFXManager->AllocEffect(0x11);
						RandSigned();
						anEffect->mDAlpha = -0.9f;
						float aDist = (float)i;
						anEffect->mX = (float)((double)cosf(aTrail) * (double)aDist * (double)1.95f + (double)aX);
						anEffect->mY = (float)((double)sinf(aTrail) * (double)aDist * (double)1.95f + (double)aY);
						anEffect->mScale = 1.25f;
						aRand = RandSigned();
						anEffect->mDY = (float)((double)fabsf(aRand) * (double)0.3f);
						anEffect->mIsAdditive = true;
						anEffect->mAngle = (float)((double)RandSigned() * (double)3.1415927f);
						aFXManager->AddEffect(anEffect);
					}
					aTrail = (float)((double)aTrail - (double)1.2566371f);
				}
				anAngle = (float)((double)anAngle + (double)3.1415927f);
			}
		}
	}
}

// 0x6a478b: whether a cell has its +0x34 set
bool Board::AnyCellMarked()
{
	int aCount = mCols * mRows;
	for (int i = 0; i < aCount; i++) {
		if (mCells[i].mIsMarked)
			return true;
	}
	return false;
}

// 0x6a42b4: the bomb with the lowest counter, else a locked gem (9.5); nullptr when there is neither
Piece* Board::FindHintPiece()
{
	Piece* aBest = nullptr;
	float aBestValue = 1000.0f;
	int aCount = mCols * mRows;
	for (int i = 0; i < aCount; i++) {
		Piece* aPiece = mGrid[i];
		if (aPiece != nullptr) {
			float aValue = 1000.0f;
			if (aPiece->IsFlagSet(Piece::FLAG_BOMB))
				aValue = (float)aPiece->mBombCount;
			else if (aPiece->IsFlagSet(Piece::FLAG_LOCKED))
				aValue = 9.5f;
			if (aValue < aBestValue) {
				aBestValue = aValue;
				aBest = aPiece;
			}
		}
	}
	return aBest;
}

void Board::ShowHint()
{
	// 0x6c32cf
	if (AnyCellMarked())
		return;
	FindMove(&mHintRow, &mHintCol, false);
	if (mHintRow < 0)
		return;
	// every third hint (and the first) sparkles before it flashes
	int aHints = mHintsShown + 1;
	mHintState = aHints;
	mHintsShown = aHints;
	if (3 < aHints) {
		mHintsShown = 0;
		mHintState = 1;
	}
	mHintSparkleTurn = 0;
	if (mGameMode == 1 || mGameMode == 4) {
		bool aHasBomb = false;
		for (unsigned int i = 0; i < mHintGems.size(); i++) {
			Piece* aPiece = GetPieceById(mHintGems[i]);
			if (aPiece != nullptr && aPiece->IsFlagSet(Piece::FLAG_BOMB))
				aHasBomb = true;
		}
		Piece* aPiece;
		// a new player's early levels point at the most urgent bomb or lock instead
		if (mApp->mProfile->mGamesPlayed == 0 && mLevel < 4 && !aHasBomb &&
			(aPiece = FindHintPiece()) != nullptr) {
			mHintGems.clear();
			mHintGems.push_back(aPiece->mId);
			mHintState = 1;
		}
		if ((mFirstLevelHints && mLevel < 2) || mCounterHintDue) {
			mHintState = 2;
			for (unsigned int i = 0; i < mHintGems.size(); i++) {
				Piece* aHintPiece = GetPieceById(mHintGems[i]);
				if (aHintPiece != nullptr)
					mCells[mCols * aHintPiece->mRow + aHintPiece->mCol].mHintFlash = 0.001f;
			}
		}
	}
}

// 0x6d5279 to 0x6d6695 of UpdateGame: the tooltips. While the mouse is over one of the tube's parts or a special gem the
// text for it is worked out here; once the mouse has stayed over the same text for aWait updates the tooltip comes up
// (+0x6cc) with its place, and fades in and out with the curves at +0x718 and +0x788. DrawOverlay draws it.
void Board::UpdateToolTips()
{
	// the scaled sizes (0x67647d)
	auto Sc = [](int theValue) { return Ftol((double)gApp->mScale * (double)theValue); };

	SexyString aTitle;
	SexyString aBody;
	int aSound = -1;
	int aWait = 100;
	int aX = 0;
	int aY = 0;
	int aWidth = Ftol((double)gApp->mScale * 280.0);
	if (mHintsReady) {
		int aMouseX = mWidgetManager->mLastMouseX;
		int aMouseY = mWidgetManager->mLastMouseY;
		// nothing is said while the mouse is over the tutorial's dialog
		Rect aTutorialRect(0, 0, 0, 0);
		if (mTutorial != nullptr && mTutorial->mDialog != nullptr)
			aTutorialRect = Rect(mTutorial->mDialog->mX, mTutorial->mDialog->mY, mTutorial->mDialog->mWidth,
				mTutorial->mDialog->mHeight);
		if (!aTutorialRect.Contains(aMouseX, aMouseY)) {
			GoodTube* aTube = mGoodTube;
			if (aTube != nullptr) {
				if (0 < aTube->mX) {
					if (!aTube->mLevelSoundOn) {
						// the challenge's info box
						if (mGameMode == MODE_CHALLENGE) {
							Rect aRect(Sc(0x4c) + aTube->mX, Sc(0x50) + aTube->mY, Sc(0x10e), Sc(0x190));
							if (aRect.Contains(aMouseX, aMouseY)) {
								aTitle = mApp->GetString(GTS(0x7dec1c), GT(0x7debdc));
								aBody = mApp->GetString(GTS(0x7deba0), GT(0x7deb28));
								aY = Sc(0xfa) + aTube->mY;
								aX = Sc(0x11f) + aTube->mX;
								aWidth = Sc(0x168);
							}
						}
					} else {
						// the chain meter: the arc's glow, or the box around it
						int aGlowY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(IMAGE_INGAMEUI_ARC_GLOW_ID));
						int aGlowX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(IMAGE_INGAMEUI_ARC_GLOW_ID));
						unsigned int aPixel = GetPixelAt(IMAGE_INGAMEUI_ARC_GLOW, (aMouseX - aTube->mX) - aGlowX,
							(aMouseY - aTube->mY) - aGlowY);
						Rect aRect(aTube->mX - Ftol((double)gApp->mScale * -76.0),
							aTube->mY - Ftol((double)gApp->mScale * -80.0), Ftol((double)gApp->mScale * 270.0),
							Ftol((double)gApp->mScale * 200.0));
						if (0x80000000u < aPixel || aRect.Contains(aMouseX, aMouseY)) {
							aTitle = StrFormat(mApp->GetString(GTS(0x7df138), GT(0x7df0e0)).c_str(), GetLevelFor(nullptr));
							aBody = mApp->GetString(GTS(0x7df060), GT(0x7def60));
							aY = Ftol((double)gApp->mScale * 175.0) + aTube->mY;
							aX = Ftol((double)gApp->mScale * 287.0) + aTube->mX;
							aWidth = Ftol((double)gApp->mScale * 360.0);
						}
					}
				}
				// the rank insignia
				Rect aRankRect(Sc(0x94) + aTube->mX, Sc(0x177) + aTube->mY, Sc(0x7c), Sc(0x5a));
				if (aRankRect.Contains(aMouseX, aMouseY) && aTube->mShown) {
					aTitle = mApp->GetString(GTS(0x7def3c), GT(0x7def10));
					SexyString aRank = mApp->mRanks->GetName(mApp->mProfile->mRank);
					aBody = StrFormat(mApp->GetString(GTS(0x7dee78), GT(0x7ded48)).c_str(), aRank.c_str());
					aY = Sc(0x190) + aTube->mY;
					aX = Sc(0xf4) + aTube->mX;
					aWidth = Sc(0x17c);
				}
				// the meter: the time left, or the level's progress
				int aMeterY = Ftol((double)aTube->mY + (double)aTube->mFillTopShown - (double)Sc(10));
				Rect aMeterRect(Sc(0x88) + aTube->mX, aMeterY, Sc(0x8e), Sc(0x3b));
				if (aMeterRect.Contains(aMouseX, aMouseY)) {
					if (mGameMode == MODE_BLITZ || (mGameMode == MODE_CHALLENGE && mChallenge != 0 && HasChallengeFlag(2))) {
						aTitle = mApp->GetString(GTS(0x7dea60), GT(0x7dea30));
						aBody = mApp->GetString(GTS(0x7dea00), GT(0x7de9a0));
					} else if (mGameMode != MODE_CHALLENGE) {
						aTitle = mApp->GetString(GTS(0x7deb10), GT(0x7deae4));
						int aPercent = Ftol(((double)mLevelProgress / (double)mLevelProgressGoal) * 100.0);
						aBody = StrFormat(mApp->GetString(GTS(0x7deac0), GT(0x7dea78)).c_str(), aPercent, mLevel);
					} else {
						aTitle = mApp->GetString(GTS(0x7ded2c), GT(0x7decf4));
						aBody = mApp->GetString(GTS(0x7decb8), GT(0x7dec40));
						aWidth = Sc(0x12c);
					}
					aY = Ftol((double)aTube->mY + (double)aTube->mFillTopShown + (double)Sc(5));
					aX = Sc(0xfa) + aTube->mX;
				}
				// the challenge, or the bonus challenge's four colours
				Rect aChallengeRect(Sc(0x56) + aTube->mX, Sc(0x371) + aTube->mY, Sc(0xfa), Sc(0x7d));
				if (aChallengeRect.Contains(aMouseX, aMouseY)) {
					bool aShow = true;
					if (mGameMode == MODE_CHALLENGE && mChallenge != 0) {
						aTitle = mApp->GetString(GTS(0x7de988), GT(0x7de964));
						aBody = mApp->GetString(GTS(0x7de938), GT(0x7de8e0));
					} else if (!mBonusColors.empty()) {
						aTitle = mApp->GetString(GTS(0x7de8c8), GT(0x7de898));
						aBody = mApp->GetString(GTS(0x7de868), GT(0x7de808));
					} else {
						aShow = false;
					}
					if (aShow) {
						aY = Sc(0x3bc) + aTube->mY;
						aX = Sc(0x12c) + aTube->mX;
					}
				}
			}
			// a special gem under the mouse
			Piece* aPiece = GetPieceAtXY(aMouseX, aMouseY);
			if (aPiece != nullptr) {
				int aDX = Ftol((double)aMouseX - ((double)(gGemWidth / 2) + (double)aPiece->mX));
				int aDY = Ftol((double)aMouseY - ((double)(gGemHeight / 2) + (double)aPiece->mY));
				float aDist = sqrtf((float)((double)aDX * (double)aDX + (double)(aDY * aDY)));
				if ((double)aDist <= (double)gGemWidth * 0.3) {
					bool aShown = true;
					if (!aPiece->IsFlagSet(Piece::FLAG_POWER)) {
						if (aPiece->IsFlagSet(Piece::FLAG_ELECTRO)) {
							aTitle = mApp->GetString(GTS(0x7de52c), GT(0x7de500));
							aBody = mApp->GetString(GTS(0x7de4a0), GT(0x7de3e0));
						} else if (aPiece->IsFlagSet(Piece::FLAG_LOCKED)) {
							aTitle = mApp->GetString(GTS(0x7de3cc), GT(0x7de3a4));
							aBody = mApp->GetString(GTS(0x7de360), GT(0x7de2d8));
						} else if (0 < aPiece->mLockState && !aPiece->IsFlagSet(Piece::FLAG_DOOM)) {
							aTitle = mApp->GetString(GTS(0x7de2c4), GT(0x7de29c));
							aBody = mApp->GetString(GTS(0x7de228), GT(0x7de140));
							aWidth += Sc(0);
						} else if (aPiece->IsFlagSet(Piece::FLAG_BOMB)) {
							aTitle = mApp->GetString(GTS(0x7dcce4), GT(0x7dccc0));
							aBody = mApp->GetString(GTS(0x7de0e8), GT(0x7de040));
							aWidth += Sc(0x32);
						} else if (aPiece->IsFlagSet(Piece::FLAG_COAL)) {
							if (mCoalAsSnackers) {
								aTitle = mApp->GetString(GTS(0x7de02c), GT(0x7de008));
								aBody = mApp->GetString(GTS(0x7ddf98), GT(0x7ddeb8));
								aSound = SOUND_PURR;
							} else {
								aTitle = mApp->GetString(GTS(0x7ddea4), GT(0x7dde88));
								aBody = mApp->GetString(GTS(0x7dde28), GT(0x7ddd68));
							}
						} else if (aPiece->IsFlagSet(Piece::FLAG_FRUIT)) {
							aTitle = mApp->GetString(GTS(0x7ddd50), GT(0x7ddd2c));
							aBody = mApp->GetString(GTS(0x7ddce8), GT(0x7ddc60));
						} else if (aPiece->IsFlagSet(Piece::FLAG_ANGEL)) {
							aTitle = mApp->GetString(GTS(0x7ddc4c), GT(0x7ddc28));
							aBody = mApp->GetString(GTS(0x7ddb98), GT(0x7dda78));
						} else if (aPiece->IsFlagSet(Piece::FLAG_DROPPER)) {
							aTitle = mApp->GetString(GTS(0x7dda60), GT(0x7dda3c));
							aBody = mApp->GetString(GTS(0x7dd9f0), GT(0x7dd958));
						} else if (aPiece->IsFlagSet(Piece::FLAG_DEVIL)) {
							aTitle = mApp->GetString(GTS(0x7dd944), GT(0x7dd920));
							aBody = mApp->GetString(GTS(0x7dd8c0), GT(0x7dd800));
						} else {
							aShown = false;
						}
					} else if (aPiece->IsFlagSet(Piece::FLAG_ELECTRO)) {
						aTitle = mApp->GetString(GTS(0x7de7ec), GT(0x7de7c0));
						aBody = mApp->GetString(GTS(0x7de750), GT(0x7de670));
					} else {
						aTitle = mApp->GetString(GTS(0x7de65c), GT(0x7de638));
						aBody = mApp->GetString(GTS(0x7de5e8), GT(0x7de548));
					}
					if (aShown) {
						// the wait is longer the further the mouse is from the gem's middle
						if ((double)gGemWidth * 0.25 < (double)aDist) {
							if ((double)aDist <= (double)gGemWidth * 0.35)
								aWait = 120;
						} else {
							aWait = 100;
						}
						aY = Ftol((double)(gGemHeight / 2) + (double)aPiece->mY);
						int aPieceX = Ftol((double)(gGemWidth / 2) + (double)aPiece->mX);
						// the last tooltip's right edge decides the side (both sides are the gem's middle plus 0)
						if ((double)(mTooltipWidth + mTooltipX) <= (double)mWidth * 0.8999999761581421)
							aX = aPieceX - Sc(0);
						else
							aX = aPieceX + Sc(0);
					}
				}
			}
		}
	}

	// 0x6d64e2: a new text starts the wait again; the same text for long enough brings the tooltip up
	if (aBody != mTooltipHoverText) {
		mTooltipHoverText = aBody;
		mTooltipHoverCount = 0;
		mTooltipFade.SetCurve(GTS(0x7dd7dc));
	} else {
		int aStayed = mTooltipHoverCount;
		mTooltipHoverCount = aStayed + 1;
		if (aWait <= aStayed && !aBody.empty() && aBody != mTooltipText) {
			mTooltipUp = true;
			mTooltipX = aX;
			mTooltipY = aY;
			mTooltipTitle = aTitle;
			mTooltipText = aBody;
			mTooltipWidth = aWidth;
			if (aSound != -1)
				mApp->PlaySample(aSound);
			mTooltipArrowSlide.SetCurve(GTS(0x7dd7b8));
			mTooltipArrowSlide.SetMode(CurvedVal::MODE_REPEAT);
			mTooltipFade.SetCurve(GTS(0x7dd798));
		}
	}
	mTooltipArrowSlide.IncInVal();
	if (!mTooltipFade.IncInVal() && mTooltipFade.GetOutVal() == 0.0) {
		mTooltipUp = false;
		mTooltipText = SexyString();
	}
}

void Board::ShowReplayPod()
{
	mReplayPodDown = false;
	if (mGameMode == MODE_BLITZ || 6 <= mPlayState)
		return;
	if (mReplayPod == nullptr) {
		mReplayPod = new ReplayWidget();
		mReplayPod->mX = Ftol((double)gApp->mScale * 200.0);
		mReplayPod->mY = mHeight;
		mReplayPod->mZOrder = mGoodTube->mZOrder + 1;
		AddWidget(mReplayPod);
	}
	mReplayPod->Show();
}

void Board::SetBoardFade(float theValue)
{
	float aWas = mBoardFade;
	mBoardFade = theValue;
	if (aWas == 0.0f && 0.0f < theValue) {
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece != nullptr)
					aPiece->mIsLive = true;
			}
		}
		if (mGameMode == MODE_CLASSIC)
			NextLevelMusic();
	}
}

void Board::ApplyBoardFade()
{
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece != nullptr)
				aPiece->mAlpha = mBoardFade;
		}
	}
}

void Board::RetargetTravellers()
{
	// Each soul picks a plain gem of the new board, from a random cell on, that no other soul goes to; a fruit's
	// soul looks for one of its colour first
	RandSigned();
	for (unsigned i = 0; i < mLevelSouls.size(); i++) {
		GemSoul* aSoul = mLevelSouls[i];
		int aCells = mRows * mCols;
		int aCell = BoardRand() % aCells;
		bool aFound = false;
		for (int aPass = 0; aPass < 2 && !aFound; aPass++) {
			for (int n = 0; n < aCells; n++) {
				Piece* aPiece = mGrid[aCell];
				if (aPiece != nullptr && aPiece->mFlags == 0) {
					bool aTaken = false;
					for (unsigned j = 0; j < mLevelSouls.size(); j++) {
						if (mLevelSouls[j]->mState == 2 && mLevelSouls[j]->mHomePiece == aPiece) {
							aTaken = true;
							break;
						}
					}
					if (!aTaken && (!(aSoul->mType & (1 << 0)) || aPiece->mColor == aSoul->mColor || aPass == 1)) {
						aSoul->SetTarget(aPiece);
						aSoul->mIsVisible = true;
						aSoul->mRetargetWait = (float)((double)i * 0.05);
						aFound = true;
						break;
					}
				}
				aCell++;
				if (aCell == aCells)
					aCell = 0;
			}
		}
	}
}

void Board::UpdateStrikes()
{
	// The original asks the app whether it is 3D accelerated, for the strikes' effects
	SettlePieces();
	UpdateShockwaves();
	if (mElectroStrikes.empty()) {
		mSettling = true;
		mInputLock = 0;
	}
	// Each strike lights its gems up until they break, and every tenth of a second its lightning spreads a cell
	// further along its row and column (a power electro gem's along three of each); the strikes after the n-th one
	// wait while it is less than 25 * n updates old
	int aWait = 0;
	int i = 0;
	if (0 < (int)mElectroStrikes.size()) {
		do {
			ElectroStrike* aStrike = static_cast<ElectroStrike*>(mElectroStrikes[i]);
			aStrike->Update();
			int aKind = aStrike->mKind;
			bool aRemove = false;
			if (-1 < aKind && aKind < 2) {
				int aStar = aKind == 1;
				bool aAllDone = true;
				// the gems it has lit glow brighter, and break once fully lit
				unsigned int j = 1;
				if (1 < aStrike->mIds.size()) {
					do {
						Piece* aPiece = GetPieceById(aStrike->mIds[j]);
						if (aPiece != nullptr && !aPiece->mHasBroken) {
							float aGlow = (float)((double)aPiece->mGlow + 0.014999999664723873);
							aPiece->mGlow = aGlow;
							if (!(1.0f <= aGlow)) {
								aAllDone = false;
							} else {
								aPiece->mBusyTime = 1;
								aPiece->mMatchSerial = aStrike->mPointsId;
								if (!aPiece->IsFlagSet(Piece::FLAG_COAL)) {
									aPiece->mBreakCause = -0x65;
								} else {
									aPiece->mBreakCause = -0x67;
									BreakSpecial(aPiece, 2);
									if (!aPiece->mBreaking) {
										AddStat(0x4a, 1);
										AddStat(0x3e, 1);
										mLevelGemCount++;
										mGameGemCount++;
										aPiece->mHasBurst = true;
									}
									aPiece->mBreaking = true;
									aPiece->mBreakFrame = 25;
								}
								aStrike->mIds.erase(aStrike->mIds.begin() + j);
								j--;
							}
						}
						j++;
					} while (j < aStrike->mIds.size());
				}
				float aTimer = (float)((double)aStrike->mTimer - 0.009999999776482582);
				aStrike->mTimer = aTimer;
				if (aTimer <= 0.0f) {
					aStrike->mTimer = 0.1f;
					static const int kDirs[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
					int aDir = 0;
					do {
						int aSide = -aStar;
						if (aSide <= aStar) {
							int aDX = kDirs[aDir][0];
							int aDY = kDirs[aDir][1];
							int anOffY = aDX * aSide;
							int anOffX = aDY * aSide;
							int aLeft = aStar - aSide + 1;
							do {
								int aY = (aDY * aStrike->mStep + anOffY) * gGemHeight + aStrike->mY;
								int aX = (aStrike->mStep * aDX + anOffX) * gGemWidth + aStrike->mX;
								Piece* aPiece = GetPieceAtXY(aX, aY);
								if (GetColX(0) <= aX && aX < GetColX(mCols) && GetRowY(0) <= aY &&
									aY < GetRowY(mRows) && aPiece != nullptr && !aPiece->mHasBroken) {
									// a gem a strike has lit already is left to it
									bool aTaken = false;
									unsigned int k = 0;
									if (mElectroStrikes.size() != 0) {
										do {
											std::vector<int>& anIds =
												static_cast<ElectroStrike*>(mElectroStrikes[k])->mIds;
											unsigned int m = 0;
											if (anIds.size() != 0) {
												do {
													if (aPiece->mId == anIds[m])
														aTaken = true;
													m++;
												} while (m < anIds.size());
											}
											k++;
										} while (k < mElectroStrikes.size());
									}
									if (!aTaken) {
										aAllDone = false;
										if (aPiece->mGlow == 0.0f && !aPiece->mHasBroken) {
											aStrike->mIds.push_back(aPiece->mId);
											aPiece->mGlow = 0.01f;
											float aFX = (float)aX;
											float aFY = (float)aY;
											int aSparks = 20;
											do {
												Effect* aSpark = mFXManager->AllocEffect(17);
												double anAcross = (double)std::abs(aDX);
												aSpark->mX = (float)((double)RandSigned() * anAcross *
													(double)gGemWidth / 3.0 + (double)aFX);
												double aDown = (double)std::abs(aDY);
												aSpark->mY = (float)((double)RandSigned() * aDown *
													(double)gGemHeight / 3.0 + (double)aFY);
												double aSpread = (double)std::abs(aDY) + 0.5;
												aSpark->mDX = (float)Ftol((double)RandSigned() * aSpread * 10.0);
												aSpread = (double)std::abs(aDX) + 0.5;
												aSpark->mDY = (float)Ftol((double)RandSigned() * aSpread * 10.0);
												mFXManager->AddEffect(aSpark);
												aSparks--;
											} while (aSparks != 0);
										}
									}
								}
								anOffX += aDY;
								anOffY += aDX;
								aLeft--;
							} while (aLeft != 0);
						}
						aDir++;
					} while (aDir < 4);
					if (aStrike->mStep == 0)
						mApp->PlaySample(SOUND_ELECTRO_EXPLODE);
					aStrike->mStep++;
					int aFar = mRows - 1;
					if (aFar <= mCols - 1)
						aFar = mCols - 1;
					if (aStrike->mStep < aFar)
						aAllDone = false;
					if (aAllDone) {
						// the electro gem itself breaks, and the strike fades
						Piece* aGem = GetPieceById(aStrike->mIds[0]);
						if (aGem != nullptr) {
							aGem->mHasBroken = false;
							aGem->ClearSpecial();
							aGem->mBusyTime = 1;
						}
						aStrike->mFade = (float)((double)aStrike->mFade - 0.25);
						Piece** aCell = mGrid;
						for (int aCells = mCols * mRows; 0 < aCells; aCells--) {
							if (*aCell != nullptr)
								(*aCell)->mFallSpeed = 0;
							aCell++;
						}
						if (aStrike->mFade <= 0.0f)
							aRemove = true;
					}
				}
			} else if (aKind == 2) {
				Piece* aGem = GetPieceById(aStrike->mIds[0]);
				Piece** aCell = mGrid;
				for (int aCells = mCols * mRows; 0 < aCells; aCells--) {
					Piece* aPiece = *aCell;
					if (aPiece != nullptr && aPiece != aGem && !aPiece->mBreaking &&
						(aPiece->IsFlagSet(Piece::FLAG_BOMB) || aPiece->IsFlagSet(Piece::FLAG_DROPPER) ||
							aPiece->IsFlagSet(Piece::FLAG_LOCKED))) {
						aPiece->mBusyTime = 1;
						break;
					}
					aCell++;
				}
				aRemove = true;
			}
			if (aRemove) {
				mElectroStrikes.erase(mElectroStrikes.begin() + i);
				delete aStrike;  // port: the original leaves the strike allocated
				i--;
			} else {
				if (aStrike->mCount < aWait)
					break;
				aWait += 25;
			}
			i++;
		} while (i < (int)mElectroStrikes.size());
	}
}

void Board::GetCenter(int* theX, int* theY)
{
	if (theX != nullptr) {
		if (!mLevelStarted)
			*theX = mWidth / 2;
		else
			*theX = (GetColX(mCols - 1) + gGemWidth - GetColX(0)) / 2 + GetColX(0);
	}
	if (theY != nullptr)
		*theY = (GetRowY(mRows - 1) + gGemHeight - GetRowY(0)) / 2 + GetRowY(0);
}

void Board::SetEndState(int theState)
{
	if (theState == 1 || theState == 9) {
		mEndStateWait = 0;
	} else if (theState == 3) {
		mGameOverRow = 0;
		mClearedOnHyperspaceStart = 0;
		mEndStateWait = 1.0f;
	} else if (theState == 4) {
		// blitz's FINAL SCORE and the score under it
		mFinalScoreWait = 2.0f;
		mApp->PlaySample(SOUND_TALLY_ZOOMIN);
		mFinalScoreLabel = new TextImage(FONT_INCREDIBLE, mApp->GetString(GTS(0x7dc338), GT(0x7dc320)), 0, 0, -1, 0,
			nullptr);
		mFinalScoreText = new TextImage(FONT_INCREDIBLE, CommaSeperate(mScore), 0, 0, -1, 0, nullptr);
		mNukeZoom = 8.0f;
		mFinalScoreFade = 1.0f;
	}
	mEndState = theState;
}

void Board::UpdateGameOver()
{
	if (mEndedByNuke) {
		// (the nuke's shake: a draw from Rand and a cosine nothing keeps)
		mGameOverTicks++;
		if (0x46 < mGameOverTicks)
			mGameOverShakeTicks++;
		if (mGameOverShakeTicks < 1 || 199 < mGameOverShakeTicks)
			return;
		Rand();
		cosf((float)((double)mGameOverShakeTicks * 0.031415926707925035));
		return;
	}
	mFallLimit = 0x32;
	FallPieces();
	switch (mEndState) {
	case 1:
		SetEndState(3);
		break;
	case 3: {
		// The rows fall off the board one by one, from the top
		float aDelay = (float)((double)mEndStateWait - 0.009999999776482582);
		mEndStateWait = aDelay;
		if (0.0f < aDelay)
			return;
		std::vector<Piece*> aRow;
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(mGameOverRow, aCol);
			if (aPiece != nullptr)
				aRow.push_back(aPiece);
		}
		int aMidX;
		int aMidY;
		GetCenter(&aMidX, &aMidY);
		for (unsigned int i = 0; i < aRow.size(); i++) {
			Piece* aPiece = aRow[i];
			aPiece->mIsOff = true;
			float aRand = fabsf(RandSigned());
			aPiece->mFallSpeed = (float)((double)-aRand * (double)gGemHeight * 0.019999999552965164);
			aRand = fabsf(RandSigned());
			aPiece->mPendingDX = (float)(((double)aPiece->mX - (double)aMidX) * (double)aRand * 0.02500000037252903);
		}
		mGameOverRow++;
		if (mGameOverRow < mRows + 5) {
			mEndStateWait = 0.2f;
		} else if (mGameMode == MODE_BLITZ) {
			SetEndState(4);
			unsigned int aTrack = (unsigned int)mScore / 40000;
			if (2 < aTrack)
				aTrack = 2;
			mApp->PlayMusic(301 + aTrack, true, true, 0, 0);  // 0x84ea68
		} else {
			SetEndState(9);
			mEndStateWait = 1.25f;
		}
		break;
	}
	case 4: {
		float aDelay = (float)((double)mFinalScoreWait - 0.009999999776482582);
		mFinalScoreWait = aDelay;
		if (!(0.0f < aDelay))
			mEndState = 5;
		break;
	}
	case 5: {
		float aZoom = (float)((double)mNukeZoom - 0.10000000149011612);
		mNukeZoom = aZoom;
		if (1.0f < aZoom)
			return;
		mApp->ShowHyperspace();
		mApp->mHyperspace->StartEndGame();
		mGoodTube->mVisible = false;
		mBoardFade = 0;
		mNukeZoom = 8.0f;
		mApp->PlaySample(SOUND_TALLY_ZOOMIN);
		mEndState++;
		break;
	}
	case 6: {
		float aZoom = (float)((double)mNukeZoom - 0.10000000149011612);
		mNukeZoom = aZoom;
		if (1.0f < aZoom)
			return;
		mNukeZoom = 1.0f;
		if (mApp->mProfile->mRank < (int)mApp->mRanks->mRanks.size()) {
			// The rank's bonus: the score to the power 0.6, a hundredth of it
			float aScore = (float)(unsigned int)mScore;
			int aBonus = Ftol((double)(float)std::pow((double)aScore, (double)0.6f) / 100.0);  // 0x670a49
			mBonusLevel = aBonus;
			if (aBonus != 0) {
				ShowRankBonus(aBonus);
			}
		}
		mEndStateWait = 2.0f;
		mEndState++;
		break;
	}
	case 7:
		if (0.0f < mEndStateWait)
			mEndStateWait = (float)((double)mEndStateWait - 0.009999999776482582);
		else
			mEndState++;
		break;
	case 8: {
		float aFade = (float)((double)mFinalScoreFade - 0.019999999552965164);
		mFinalScoreFade = aFade;
		bool aDone = false;
		if (!(0.0f < aFade)) {
			mFinalScoreFade = 0;
			aDone = true;
		}
		if (mRankBonusWidget == nullptr || mRankBonusWidget->mClicked) {
			// (the original removes and deletes it when there is none too)
			if (mRankBonusWidget != nullptr)
				mApp->mWidgetManager->RemoveWidget(mRankBonusWidget);
			mApp->SafeDeleteWidget(mRankBonusWidget);
			mRankBonusWidget = nullptr;
		}
		if (aDone && mRankBonusWidget == nullptr)
			SetEndState(9);
		break;
	}
	case 9: {
		float aDelay = (float)((double)mEndStateWait - 0.009999999776482582);
		mEndStateWait = aDelay;
		if (!(0.0f < aDelay) && mShownDialog == nullptr && mPendingDialogs.empty()) {
			mEndState = 0;
			if (mChallenge == 0)
				mGameOverShown = true;
			else
				mEndPending = 1;
		}
		break;
	}
	}
}

void Board::UpdateStats()
{
	if (mStatsAdded)
		return;
	unsigned int aTime = (unsigned int)(mUpdateCnt - mPlayStartCount) / 100 + (unsigned int)mSecondsBeforePause;
	mSecondsPlayed = (int)aTime;
	if ((unsigned int)mGameMode <= 2) {
		// the app's longest game of the mode
		unsigned int& aBest = (unsigned int&)mApp->mStats[5 + mGameMode];
		if (aTime < aBest)
			aTime = aBest;
		aBest = aTime;
	}
	Profile* aProfile = mApp->mProfile;
	aProfile->mTotalLevelsDelta += mSecondsPlayed - mSecondsAddedToProfile;
	mSecondsAddedToProfile = mSecondsPlayed;
	if (mGameMode != MODE_PUZZLE) {
		aProfile->mTotalFlames += mAchievementOneCount - mAchievementOneCounted;
		if (9 < aProfile->mTotalFlames)
			aProfile->mAchievementBits |= 1 << 1;  // 0x733774
		mAchievementOneCounted = mAchievementOneCount;
		aProfile->mTotalElectros += mAchievementTwoCount - mAchievementTwoCounted;
		if (4 < aProfile->mTotalElectros)
			aProfile->mAchievementBits |= 1 << 2;
		mAchievementTwoCounted = mAchievementTwoCount;
		aProfile->mTotalPowerGems += mGemsBroken - mGemsBrokenAtLastProfileSave;
		if (mGameMode == MODE_CLASSIC || mGameMode == MODE_BLITZ || mGameMode == MODE_PUZZLE) {
			unsigned int aScore = (unsigned int)mScore;
			if (aScore <= (unsigned int)aProfile->mBestScores[mGameMode])
				aScore = (unsigned int)aProfile->mBestScores[mGameMode];
			aProfile->mBestScores[mGameMode] = (int)aScore;
		} else if (mGameMode == MODE_ZEN) {
			aProfile->mBestScores[MODE_ZEN] = mScore;
		}
		mGemsBrokenAtLastProfileSave = mGemsBroken;
		int aLevel = mLevel;
		if (aLevel <= aProfile->mBestLevels[mGameMode])
			aLevel = aProfile->mBestLevels[mGameMode];
		aProfile->mBestLevels[mGameMode] = aLevel;
	}
	if (!mIsUnscoredMode) {
		unsigned int aScore = (unsigned int)aProfile->mHighestScore;
		if (aScore <= (unsigned int)mScore)
			aScore = (unsigned int)mScore;
		aProfile->mHighestScore = (int)aScore;
	}
	int aSeconds = mUpdatesPlayed / 100;
	if (mIsBlitzGame)
		mApp->mSecondsInFirstSpecial += aSeconds;
	else if (mGameMode == MODE_PUZZLE)
		mApp->mSecondsPuzzle += aSeconds;
	else if (mIsUnscoredMode)
		mApp->mSecondsInSecondSpecial += aSeconds;
	else
		mApp->mSecondsOrdinary += aSeconds;
	mStatsAdded = true;
}

bool Board::UpdateNuke()
{
	if (mNukeProgress.CheckInThreshold(0.7900000214576721) && mChallenge != 0 &&
		(mChallenge->mFlags & Challenge::FLAG_NIGHTMODE) != 0) {
		// a night challenge ends here, met
		ShowChallengeComplete(true);
		return true;
	}
	bool aDone = !mNukeProgress.IncInVal();
	if (mEndingPiece != nullptr) {
		// The gems within the blast's reach blow apart, eight bits each
		mEndingPiece->Update();
		double aGrow = mNukeNovaScale.GetOutVal();
		float aReach = (float)(aGrow * (double)(float)((double)gApp->mScale * 280.0));
		float aMidX = (float)((double)(gGemWidth / 2) + (double)mEndingPiece->mX);
		float aMidY = (float)((double)(gGemHeight / 2) + (double)mEndingPiece->mY);
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece == nullptr || aPiece == mEndingPiece)
					continue;
				float aDX = (float)((double)(gGemWidth / 2) + (double)aPiece->mX - (double)aMidX);
				float aDY = (float)((double)(gGemHeight / 2) + (double)aPiece->mY - (double)aMidY);
				float aDist = sqrtf((float)((double)aDX * (double)aDX + (double)aDY * (double)aDY));
				if (!(aDist < aReach) || aPiece->mBlasted)
					continue;
				aPiece->mBlasted = true;
				int i = 0;
				do {
					Effect* anEffect = mFXManager->AllocEffect(16);
					anEffect->mColor = gGemColors[aPiece->mColor];
					int aRand = (int)Rand() % 100;
					double anAngle = (double)aRand / 800.0 + (double)i * 0.503;
					anEffect->mX = (float)((double)(gGemWidth / 2) + (double)aPiece->mX +
										   (double)RandSigned() * (double)gGemWidth * 0.5);
					anEffect->mY = (float)((double)(gGemHeight / 2) + (double)aPiece->mY +
										   (double)RandSigned() * (double)gGemHeight * 0.5);
					anEffect->mAngle = (float)anAngle;
					anEffect->mDAngle = (float)((double)RandSigned() * 0.05000000074505806);
					anEffect->mScale = 1.0f;
					anEffect->mAlpha = 1.0f;
					anEffect->mVelocityDamping = (float)((double)RandSigned() * 0.15 + 0.8);
					float aDir = (float)atan2((double)aDY, (double)aDX);  // 0x4c1b70
					float aCos = cosf(aDir);
					anEffect->mDX = (float)((double)RandSigned() * 4.0 + (double)aCos * 16.0);
					float aSin = sinf(aDir);
					anEffect->mDY = (float)((double)RandSigned() * 4.0 + (double)aSin * 16.0);
					anEffect->mGravity = 0.05f;
					float aTurn = (float)((double)RandSigned() * (double)6.2831854820251465f);
					anEffect->mBlend = aTurn;
					anEffect->mRateFrom = (float)((double)6.2831854820251465f * 0.25 + (double)aTurn);
					anEffect->mRateTo = 0.0f < RandSigned() ? 0.0f : 1.0f;
					anEffect->mTumbleTurn = (float)(((double)fabsf(RandSigned()) * 3.0 + 1.0) * 0.04500000178813934);
					anEffect->mDAlpha = (float)(((double)fabsf(RandSigned()) * 4.0 + 2.0) * -0.005);
					mFXManager->AddEffect(anEffect);
					i++;
				} while (i < 8);
			}
		}
		if (!(mNukeProgress.GetInVal() < 0.5799999833106995) && mNukeProgress.GetInVal() < 1.649999976158142) {
			// Steam from the tube's two vents
			Effect* aSteam = mFXManager->AllocEffect(26);
			float aSpeed = (float)((double)fabsf(RandSigned()) * 2.0 + 2.0);
			aSteam->mIsAdditive = false;
			aSteam->mDScale = 0.015f;
			aSteam->mScale = (float)((double)fabsf(RandSigned()) * 0.30000001192092896 + 0.4000000059604645);
			mFXManager->AddEffect(aSteam);
			double anAngle;
			if ((Rand() & 0x80000001) == 0) {
				int anOffsetX = Ftol((double)gApp->mScale * 300.0);
				double aSlide = mNukeTubeShake.GetOutVal();
				float aSlideX = (float)((double)gApp->mScale * -60.0);
				aSteam->mX = (float)(aSlide * (double)aSlideX + (double)(mGoodTube->mX + anOffsetX));
				int anOffsetY = Ftol((double)gApp->mScale * 200.0);
				aSlide = mNukeTubeShake.GetOutVal();
				float aSlideY = (float)((double)gApp->mScale * 8.0);
				aSteam->mY = (float)(aSlide * (double)aSlideY + (double)(mGoodTube->mY + anOffsetY));
				anAngle = (double)RandSigned() * 0.20000000298023224 - 1.2000000476837158;
			} else {
				int anOffsetX = Ftol((double)gApp->mScale * 100.0);
				double aSlide = mNukeTubeShake.GetOutVal();
				float aSlideX = (float)((double)gApp->mScale * -20.0);
				aSteam->mX = (float)(aSlide * (double)aSlideX + (double)(mGoodTube->mX + anOffsetX));
				int anOffsetY = Ftol((double)gApp->mScale * 300.0);
				aSlide = mNukeTubeShake.GetOutVal();
				float aSlideY = (float)((double)gApp->mScale * 24.0);
				aSteam->mY = (float)(aSlide * (double)aSlideY + (double)(mGoodTube->mY + anOffsetY));
				aSteam->mDScale = (float)((double)aSteam->mDScale * 0.5);
				aSpeed = (float)((double)aSpeed + 3.0);
				anAngle = (double)RandSigned() * 0.10000000149011612 - 4.199999809265137;
			}
			float aDir = (float)anAngle;
			aSteam->mDX = (float)((double)cosf(aDir) * (double)aSpeed);
			aSteam->mDY = (float)((double)sinf(aDir) * (double)aSpeed);
		}
		if (mNukeProgress.CheckInThreshold(1.649999976158142)) {
			// The blast is over: the gem is plain again and space comes back
			mEndingPiece->mFlags = 0;
			mEndingPiece->mIsDrawn = true;
			mFXManager->Clear();
			if (mChallenge != 0) {
				mApp->mChallengeInfo->mResult = 2;  // 0x6d36e8: failed
				mEndPending = 1;
			} else {
				mApp->ShowHyperspace();
				mApp->mHyperspace->mLevel = 0;
			}
			mApp->mVizManager->SetVisible(false);
			mBoardShown = false;
			mLevelStarted = false;
			mGoodTube->SetVisible(false);
		}
	}
	if (mRankBonusWidget != nullptr)
		aDone = false;
	if (aDone) {
		UpdateStats();
		mLevelStarted = false;
		mOverlayRedrawDue = true;
		mGameOverShown = true;
	}
	return false;
}

void Board::ShowChallengeComplete(bool theFlag)
{
	ChallengeMenu* aMenu = mApp->mMainMenu->mChallengeMenu;
	if (aMenu->mAwardTelly != nullptr) {
		// (the original says "DUPLICATE AWARDTELLY AVOIDED" with OutputDebugString)
		return;
	}
	AwardTelly* aTelly = new AwardTelly();
	aTelly->ShowTrophy(mChallenge, theFlag);
	mWidgetManager->AddWidget(aTelly);
	mEndPending = 1;
	mApp->ShowHyperspace();
	mVisible = false;
	mApp->mHyperspace->StartEndGame();
	mApp->mHyperspace->mBoardBrightnessFinal.SetCurve(GTS(0x7dc510));
	aMenu->SetState(5, false);
	aMenu->mAwardTelly = aTelly;
	mApp->mHyperspace->mDrawsFinalFade = false;
}

void Board::Pause(bool theShow)
{
	if (mPauseCount == 0) {
		mSecondsBeforePause += (int)((unsigned int)(mUpdateCnt - mPlayStartCount) / 100);
		MarkDirty();
		mApp->mVizManager->mPaused = true;
		mGoodTube->mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_ALLOW_MOUSE;
		if (mApp->mTwistMusic != nullptr && (mApp->mTwistMusic->mCurTrack == 300 || mApp->mTwistMusic->mCurTrack == 308))
			mApp->mTwistMusic->PauseAllMusic();
		if (mGameMode == MODE_BLITZ)
			mFXManager->Clear();
	}
	if (theShow) {
		if (mPauseShownCount == 0)
			MarkDirty();
		mPauseShownCount++;
	}
	mPauseCount++;
	// 0x6a634f
	mQuickMoveCount = 0;
	mQuickMoveGap = mSettledUpdates;
	// The original says "Pause(): mPauseCount=%d" with OutputDebugString
}

void Board::Unpause(bool theShow)
{
	if (theShow && 0 < mPauseShownCount) {
		mPauseShownCount--;
		if (mPauseShownCount < 1)
			MarkDirty();
	}
	if (0 < mPauseCount) {
		mPauseCount--;
		if (mPauseCount == 0) {
			if (mApp->mTwistMusic != nullptr &&
				(mApp->mTwistMusic->mCurTrack == 300 || mApp->mTwistMusic->mCurTrack == 308))
				mApp->mTwistMusic->ResumeAllMusic();
			mPlayStartCount = mUpdateCnt;
			mClickBlockUpdates = 1;
			if (mPlayState != 7)
				mApp->mVizManager->mPaused = false;
		}
	}
	// The original says "Unpause(): mPauseCount=%d" with OutputDebugString
}

void Board::SpinWheel(Piece* thePiece)
{
	if (mBonusStage != 0) {
		// a fruit dance moves on to its end
		mBonusStage = 5;
		mGoodTube->mBonusGlowCurve.SetCurve(GTS(0x7dc558));
		// port: the tube's effects manager (+0x28c) is cleared, once it is ported
	}
	mCursorShown = false;
	if (mApp->mStats[53] == 0)  // app +0xd48: the level of the first wheel
		mApp->mStats[53] = mLevel;
	if (mReplayPod != nullptr) {
		mReplayPod->mState = ReplayWidget::STATE_GOING;
		mReplayPod->mPopped = false;
	}
	mApp->PlaySample(SOUND_SPINSEQUENCER1);
	if (((mSeenFlags >> 11) & 1) == 0) {
		QueueTip(11, thePiece);
		ShowNextTip();
	}
	mPendingDialogs.push_back(new WheelSpin(this, thePiece));
}
