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
	if (mUnknown2208 != nullptr)
		mUnknown2208->Resize(0, 0, mWidth, mHeight);
}

void Board::Update()
{
	gGraphicsRecorder->mRecording = false;
	// The original counts the updates in 0x85af00, which nothing reads
	if (mUnknown139C != nullptr) {
		mUnknown139C->Resize(Ftol((double)gApp->mScale * 770.0), Ftol((double)gApp->mScale * 720.0),
			Ftol((double)gApp->mScale * 240.0), Ftol((double)gApp->mScale * 32.0));
	}
	// 0x85c4fc: the speed the game runs at, easing to aTarget (shared by every board, set by the first)
	static bool sSpeedSet = false;
	static float sSpeed = 0;
	if (!sSpeedSet) {
		sSpeed = mUnknown1C88;
		sSpeedSet = true;
	}
	float aTarget = mUnknown1C88;
	float anEase = 0.95f;
	if (mUnknown2629) {
		if (mUnknown3444 < 10) {
			aTarget = 0.3f;
			sSpeed = 0.3f;
		} else {
			aTarget = 0.85f;
		}
		if (mUnknown37C4 && mUnknown3444 < 3) {
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
	if (0 < mUnknown814) {
		mUnknown814--;
		if (mUnknown814 < 30)
			aTarget = 0.4f;
	}
	if (mUnknown3454 != 0.0f) {
		float aLeft = (float)((double)mUnknown3454 - (double)0.005f);
		mUnknown3454 = aLeft;
		if (aLeft <= 0.0f) {
			mUnknown3454 = 0;
		} else {
			double aSlow = (1.0 - (double)aLeft) * 0.6 + 0.4;
			if (aSlow <= (double)aTarget)
				aTarget = (float)aSlow;
		}
		if (!mUnknown2629)
			anEase = 0.85f;
	}
	sSpeed = (float)((double)sSpeed * (double)anEase + (1.0 - (double)anEase) * (double)aTarget);
	if (std::fabs(sSpeed - aTarget) < 0.01)
		sSpeed = aTarget;
	if (mUnknown267C != nullptr) {
		// The fruit dance (2200 updates) stops the game and the background behind it, easing in over its first
		// hundred updates and out over its last, and gives a fruit (SettleTurn) near its end
		int aDance = mUnknown2680;
		float aFromStart = (float)((double)aDance * 0.009999999776482582);
		float aFromEnd = (float)((double)(2200 - aDance) * 0.009999999776482582);
		float aNear = aFromEnd <= aFromStart ? aFromEnd : aFromStart;
		if (1.0f < aNear)
			aNear = 1.0f;
		double aSlow = 1.0 - (double)aNear;
		mApp->mVizManager->mSpeed = aSlow <= 1.0 ? aSlow : 1.0;
		sSpeed = (float)(aSlow <= 1.0 ? aSlow : 1.0);
		if (aDance == 2190)
			mUnknown214 = true;
		if (!mUnknown267C->mPaused)
			mUnknown2680 = aDance + 1;
	}
	float anAccum = sSpeed + mUnknown1C84;
	mUnknown1C84 = anAccum;
	if (mUnknown1C88 != mUnknown1C8C) {
		float aSpeed;
		if (mUnknown1C88 <= mUnknown1C8C) {
			aSpeed = (float)((double)mUnknown1C8C - (double)0.02f);
			if (aSpeed < mUnknown1C88)
				aSpeed = mUnknown1C88;
		} else {
			aSpeed = (float)((double)mUnknown1C8C + (double)0.02f);
			if (mUnknown1C88 < aSpeed)
				aSpeed = mUnknown1C88;
		}
		mUnknown1C88 = aSpeed;
	}
	int aCount = Ftol((double)anAccum);
	mUnknown1C84 = anAccum - (float)aCount;
	if (mUnknown37C4 && !gApp->Is3DAccelerated()) {
		if (!mUnknown3921) {
			float aFade = (float)((double)mUnknown391C + (double)0.01f);
			if (1.0f < aFade)
				aFade = 1.0f;
			mUnknown391C = aFade;
			if (aFade < 1.0f)
				return;
		} else if (mUnknown37C8 < 1) {
			mGoodTube->mX -= 20;
			mBoardX += Ftol((double)gApp->mScale * 20.0);
			if (mWidth < mBoardX) {
				float aFade = (float)((double)mApp->mBoard->mUnknown391C - (double)0.02f);
				if (aFade < 0.0f)
					aFade = 0.0f;
				mUnknown391C = aFade;
			}
			SnapPiecesX();
		} else {
			mUnknown37C8--;
		}
	}
	bool aOnce = false;
	if (mUnknown25A6 && mUnknown37C4) {
		aOnce = true;
		aCount = 1;
		mUnknown3444 = 0;
	}
	for (int i = aCount; 0 < i; i--)
		UpdateGame();
	if (aOnce)
		mUnknown3444 = 0;

	// The level shown rolls to the level
	unsigned int aLevel = (unsigned int)mUnknown168;
	if ((mUnknown23DC.empty() || (mUnknown2208 != nullptr && !mUnknown2208->mUnknownC8)) && !IsBusy()) {
		unsigned int aShown = (unsigned int)mUnknown14F8;
		if (aShown < aLevel) {
			unsigned int aDiv = (unsigned int)mUnknown21D0;
			double aSteps = (double)((aLevel - aShown) / aDiv);
			mUnknown14F8 += Ftol((aSteps / 25.0 + 1.0) * (double)(int)aDiv);
			if (aLevel <= (unsigned int)mUnknown14F8)
				mUnknown14F8 = (int)aLevel;
			mUnknown20F0 = 1.0f;
		}
		if (aLevel < (unsigned int)mUnknown14F8) {
			unsigned int aDiv = (unsigned int)mUnknown21D0;
			double aSteps = (double)(((unsigned int)mUnknown14F8 - aLevel) / aDiv);
			mUnknown14F8 -= Ftol((aSteps / 25.0 + 1.0) * (double)(int)aDiv);
			if ((unsigned int)mUnknown14F8 < aLevel)
				mUnknown14F8 = (int)aLevel;
			mUnknown20F0 = 1.0f;
		}
	}
	if (-1 < mUnknown1F84 && !IsBusy() && !mUnknown21A3) {
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
			ShowPraise(mUnknown1F84);
			mUnknown1F84 = -1;
		}
	}
	if (mUnknown2584 == nullptr || mUnknown2584->mDialog == nullptr) {
		if (0 < mUnknown257C) {
			mUnknown257C--;
			if (mUnknown257C == 0)
				ShowZenJump(1);
		}
		if (mApp->mOptionsDialog == nullptr && mApp->mDialogList.size() == 0 && mUnknown34EC == 0 &&
			0 < mUnknown2578) {
			mUnknown2578--;
			if (mUnknown2578 == 0) {
				// port: the "returning to game" dialog (a game's text, not ported yet)
				NotPorted("Board::Update: the returning to game dialog");
			}
		}
	}
	if (mUnknown267C != nullptr) {
		// The fruit dance and its music stop while the game is paused
		mUnknown267C->mPaused = mApp->mOptionsDialog != nullptr || 0 < mUnknown1574 || mFrozen || 0 < mUnknown1570;
		// port: the original has no music interface or track to check for (it reads them without a check); the
		// port has none without sound
		if (mApp->mTwistMusic != nullptr) {
			CustomBassMusicInterface::Track* aTrack = mApp->mTwistMusic->FindTrack(0x133);
			if (aTrack != nullptr) {
				if (mUnknown267C->mPaused)
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
	for (unsigned int i = 0; i < mUnknown20F4.size(); i++) {
		Piece* aPiece = mUnknown20F4[i];
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
	mUnknown1C00 = aX;
	mUnknown1C04 = aY;
	mUnknown1C08 = aSize;
	mUnknown1C0C = aSize;
}

void Board::StartUnknown3880()
{
	mUnknown3880.SetCurve(GTS(0x7dcd08));
}

void Board::NextLevelMusic()
{
	if (mApp->mNumIngameTracks != 0) {
		mUnknown2154++;
		mUnknown2154 = mUnknown2154 % mApp->mNumIngameTracks;
		mApp->PlayMusic(mUnknown2154 + 100, true, true, 0, 0);
	}
}

void Board::AddStat(int theStat, int theDelta)
{
	mUnknown2234[theStat] += theDelta;
	if (!mUnknown37C4)
		mApp->mUnknownC74[theStat] += theDelta;
}

void Board::EaseCursor()
{
	if (mUnknown1C38 < 1.0f) {
		float aSpeed = (float)((1.0 - (double)mUnknown1C38) * ((double)mUnknown1C38 + (double)mUnknown1C38));
		float aDX = (float)((double)mUnknown1C00 - (double)mUnknown1C2C);
		float aDY = (float)((double)mUnknown1C04 - (double)mUnknown1C30);
		float aDistSq = (float)((double)aDX * (double)aDX + (double)aDY * (double)aDY);
		float aDist = sqrtf(aDistSq);
		float aStep = (float)(((double)aDist * (double)aSpeed + (double)mUnknown1C34) * 0.55);
		mUnknown1C34 = aStep;
		if (1.0f < aDist) {
			mUnknown1C2C = (float)((double)aStep * (double)aDX / (double)aDist + (double)mUnknown1C2C);
			mUnknown1C30 = (float)((double)aStep * (double)aDY / (double)aDist + (double)mUnknown1C30);
			return;
		}
	}
	mUnknown1C2C = (float)mUnknown1C00;
	mUnknown1C30 = (float)mUnknown1C04;
	mUnknown1C34 = 0;
}

void Board::StartUnknown34F0()
{
	mUnknown34F0.SetCurve(GTS(0x7dcd28));
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
	mApp->PlayMusic((mChallenge->mTimeGoal == 180) * 8 + 300, false, false, mUnknown3928, mUnknown392C);
	mUnknown3928 = 0;
	mUnknown392C = 0;
}

void Board::UpdateTurnBasedMusic()
{
	// (the port's music calls go to mTwistMusic, which is there whenever the music is)
	if (mApp->mMusicInterface == nullptr || mApp->mTwistMusic == nullptr)
		return;
	CustomBassMusicInterface* aMusic = mApp->mTwistMusic;
	int aMoves = mApp->mUnknownC74[0x21];  // the moves made (+0xcf8)
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

bool Board::CountDownCounters()
{
	Piece* aLowest = nullptr;
	Piece* aWarn = nullptr;
	int aWarnCount = 100;
	mUnknown262C = 100;
	unsigned int aLeft = (unsigned int)(mCols * mRows);
	Piece** aCell = mGrid;
	if (aLeft != 0) {
		do {
			Piece* aPiece = *aCell;
			aCell++;
			if (aPiece != nullptr && (aPiece->IsFlagSet(Piece::FLAG_BOMB) || aPiece->IsFlagSet(Piece::FLAG_DEVIL))) {
				if ((aPiece->IsFlagSet(Piece::FLAG_BOMB) || !mUnknown1C42) && aPiece->mBombCount < aWarnCount) {
					aWarn = aPiece;
					aWarnCount = aPiece->mBombCount;
				}
				if (aPiece->mBombCount < mUnknown262C) {
					mUnknown262C = aPiece->mBombCount;
					aLowest = aPiece;
				}
				if (aPiece->IsFlagSet(Piece::FLAG_DEVIL) && aPiece->mBombCount == mUnknown262C)
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
		if (aLowest != nullptr && mUnknown262C == 0 && !mUnknown21A3) {
			// A counter is at zero: in a challenge, blitz or for a devil gem the game ends, else the wheel comes up
			if (!mUnknown37C4) {
				if (mGameMode != MODE_CHALLENGE && mGameMode != MODE_BLITZ && !aLowest->IsFlagSet(Piece::FLAG_DEVIL)) {
					SpinWheel(aLowest);
					return true;
				}
				// In a challenge whose goal is met a counter at zero changes nothing
				if (mGameMode == MODE_CHALLENGE && mChallenge != nullptr &&
					mApp->mChallengeInfo->Check(mChallenge, nullptr, false) == 1)
					return false;
				mUnknown1FB8--;
				AddStat(0x21, -1);
				aLowest->mDevilGlow = 0;
				aLowest->mJustMade = 0;
				mUnknown26CC = aLowest;
				mUnknown3930 = aLowest->IsFlagSet(Piece::FLAG_DEVIL);
				EndGame(true);
				return true;
			}
			if (mGameMode == MODE_CHALLENGE || mGameMode == MODE_BLITZ || aLowest->IsFlagSet(Piece::FLAG_DEVIL))
				mUnknown3920 = true;
			else
				aLowest->mUnknown88 = 1;
		}
	}
	return false;
}

// 0x6d090f (UpdateGame): the geodes from broken coal. Each waits its delay, then flies with the arc of its curve (slower
// near the sides), throws sparks, and when its flight ends it lands: sparks, its points to the score and its sound.
void Board::UpdateGeodes()
{
	static const float kSpinStep[8] = {0.2f, 0.12f, -0.2f, -0.12f, 1.0f, 0.3f, -1.0f, -0.3f};
	int aCount = (int)mUnknown14AC.size();
	for (unsigned int i = 0; i < mUnknown14AC.size(); i++) {
		Geode* aGeode = mUnknown14AC[i];
		if (0 < aGeode->mDelay) {
			aGeode->mDelay--;
			continue;
		}
		if (((unsigned int)mUnknown3914 >> 15 & 1) == 0) {
			mUnknown3904 = true;
			mApp->mProfile->SetFlag1E4(0xf, true);
		}
		float aWide = (float)mWidth - aGeode->mX;
		float aSize = (float)((double)gApp->mScale * 250.0);
		float aEdge = 1.0f;
		float aNear = aWide < aGeode->mX ? aWide : aGeode->mX;
		if ((double)(aNear / aSize) <= 1.0)
			aEdge = aNear / aSize;
		if (!aGeode->mUnknownB8.IncInVal())
			aGeode->mLife = 0;
		aGeode->mX = (float)(aGeode->mUnknownB8.GetOutVal() * (double)aGeode->mDX * (double)aEdge + (double)aGeode->mX);
		aGeode->mY = (float)(aGeode->mUnknownB8.GetOutVal() * (double)aGeode->mDY * (double)aEdge + (double)aGeode->mY);
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
			aGeode->mSpin[j] = (float)(aGeode->mUnknown38.GetOutVal() * (double)kSpinStep[j] + (double)aGeode->mSpin[j]);
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
		mUnknown14AC.erase(mUnknown14AC.begin() + i);
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
	mUnknown25A4 = false;
	mUnknown20D0 = false;
	if (mUnknown34EC != 0) {
		if (mUnknown34EC == 1) {
			// The game is over: space comes back over the board, and then the main menu (app +0xf20's state 12)
			mVisible = false;
			if (mApp->mHyperspace == nullptr)
				mApp->ShowHyperspace();
			Hyperspace* aSpace = mApp->mHyperspace;
			if (aSpace->mState == 12) {
				if (aSpace->mUnknown12B0.GetOutVal() == 0.0) {
					aSpace->mState = 0;
					if (mUnknown26A0 == nullptr)
						mUnknown1FF5 = true;
				}
			} else {
				if (gApp->mLastGameMode == 2) {
					// the challenge menu comes back with the player's progress (app +0x864's +0x4a8, 0x6e2318(4, false))
					mApp->mMainMenu->mChallengeMenu->SetState(4, false);
				}
				aSpace->StartEndGame();
			}
			if (!mApp->mHyperspace->mFlag155)
				mVisible = true;
		}
		return;
	}
	if (mUnknown26A0 != nullptr && mUnknown26A0->mClicked) {
		mApp->mWidgetManager->RemoveWidget(mUnknown26A0);
		mApp->SafeDeleteWidget(mUnknown26A0);
		mUnknown26A0 = nullptr;
		// the pod the level's end took down comes back
		if (mUnknown2628)
			ShowReplayPod();
	}
	if (mUnknown26AC == nullptr || !mUnknown26AC->mClicked) {
		if (0.0f < mUnknown380C) {
			float aLeft = (float)((double)mUnknown380C - (double)0.01f);
			mUnknown380C = aLeft;
			// The rank bar lets the level start once it waits (2) or goes (5)
			RankBonusWidget* aRankBonus = static_cast<RankBonusWidget*>(mUnknown26A0);
			if (mUnknown2208 == nullptr && mUnknown1474 == nullptr &&
				(aRankBonus == nullptr || aRankBonus->mState == 2 || aRankBonus->mState == 5)) {
				if (aLeft <= 0.0f) {
					// The trial's limit (the copy is always registered) is left out
					mApp->mVizManager->NextLook();
					mUnknown158++;
					NewLevel();
					mUnknown3810.SetCurve(GTS(0x7df2f0));
					mGoodTube->SetLevel(mUnknown158);
					StartUnknown3880();
					mUnknown380C = 0;
					mUnknown37F4 = 1.5f;
					SettleTurn();
					NextLevelMusic();
				}
			} else if (mUnknown1474 == nullptr) {
				double aNext = (double)aLeft - (double)0.01f;
				mUnknown380C = aNext <= 2.0 ? 2.0f : (float)aNext;
			} else {
				mUnknown380C = 0.01f;
			}
		}
	} else {
		mApp->mWidgetManager->RemoveWidget(mUnknown26AC);
		mApp->SafeDeleteWidget(mUnknown26AC);
		mUnknown26AC = nullptr;
	}
	// The trial's limit dialog (mUnknown1474) never comes up in a full version
	mUnknown2898.IncInVal();
	if (mGameMode == MODE_ZEN)
		mUnknown8C8.IncInVal();
	TwistWidget* aDialog = mUnknown2208;
	if (aDialog != nullptr && aDialog->mClicked) {
		if (aDialog == mUnknown2584) {
			CloseTip();
			ShowNextTip();
		} else if (aDialog == mUnknown26D0) {
			mUnknown26D0 = nullptr;
		} else if (aDialog == mUnknown3808) {
			mUnknown3808 = nullptr;
		}
		mApp->mWidgetManager->RemoveWidget(mUnknown2208);
		mApp->SafeDeleteWidget(mUnknown2208);
		mUnknown2208 = nullptr;
		mApp->mWidgetManager->SetFocus(this);
	}
	if ((mUnknown3808 != nullptr && IsBusy()) || 0.0f < mUnknown380C || mUnknown2629) {
		if (0.0f < mUnknown37F4)
			mUnknown37F4 = mUnknown37F4 <= 0.1f ? 0.1f : mUnknown37F4;
		if (0.0f < mUnknown380C && 0.0 < mUnknown3810.GetOutVal())
			mGoodTube->mVisible = false;
	}
	if (mUnknown2208 == nullptr) {
		if (mUnknown23DC.empty()) {
			if (mUnknown1FF4)
				mUnknown1FF5 = true;
		} else {
			mUnknown2208 = mUnknown23DC.front();
			mUnknown23DC.pop_front();
			mUnknown2208->mZOrder = mZOrder + 1;
			mApp->mWidgetManager->AddWidget(mUnknown2208);
		}
	} else if (mUnknown2208->mUnknownC8) {
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
	if (0 < mUnknown1570 || mApp->mOptionsDialog != nullptr || mUnknown1FF5)
		return;
	if (mUnknown3900 <= 0.0f) {
		if (0.0f < mUnknown38F8) {
			float aFade = (float)((double)mUnknown38F8 - (double)0.02f);
			mUnknown38F8 = aFade < 0.0f ? 0.0f : aFade;
		}
	} else {
		mUnknown3900 = (float)((double)mUnknown3900 - (double)0.01f);
		float aFade = (float)((double)mUnknown38F8 + (double)0.02f);
		mUnknown38F8 = 1.0f < aFade ? 1.0f : aFade;
	}
	if (0.0f < mUnknown38FC) {
		float aFade = (float)((double)mUnknown38FC - (double)0.01f);
		mUnknown38FC = aFade < 0.0f ? 0.0f : aFade;
	}
	if (!mUnknown36C0.HasBeenTriggered()) {
		mUnknown36C0.IncInVal();
	} else if (0.0f < mUnknown37A0) {
		float aFade = (float)((double)mUnknown37A0 - (double)0.02f);
		mUnknown37A0 = aFade < 0.0f ? 0.0f : aFade;
	}
	float aFade = (float)((double)mUnknown37A8 - (double)0.01f);
	mUnknown37A8 = aFade < 0.0f ? 0.0f : aFade;
	if (!mUnknown3730.HasBeenTriggered()) {
		mUnknown3730.IncInVal();
	} else if (0.0f < mUnknown37A4) {
		float aFade2 = (float)((double)mUnknown37A4 - (double)0.02f);
		mUnknown37A4 = aFade2 < 0.0f ? 0.0f : aFade2;
	}
	aFade = (float)((double)mUnknown37AC - (double)0.01f);
	mUnknown37AC = aFade < 0.0f ? 0.0f : aFade;

	// The grid slides to its place: from the slide's start to the middle, or off to the right in a replay
	int aCentre = mUnknown37C4 ? mWidth : mWidth / 2 - GetBoardWidth() / 2;
	int aFrom = aCentre;
	int aSlide = GetSlideX();
	int aTo = aSlide;
	if (!mUnknown2041) {
		aTo = aCentre;
		aFrom = aSlide;
	}
	mUnknown25A4 = true;
	if (mUnknown1474 != nullptr)
		mUnknown25A4 = false;
	if (mUnknown3880.mIncRate != 0.0 && !mUnknown3880.HasBeenTriggered())
		mUnknown25A4 = false;
	mUnknown3921 = mUnknown3920 && mUnknown25A5 && mUnknown2208 == nullptr && mUnknown14F8 == mUnknown168;
	if (!mUnknown3921 || gApp->Is3DAccelerated()) {
		if (mBoardX == aTo) {
			if (mUnknown25A6) {
				MouseMove(mUnknown15B4, mUnknown15B8);
			}
			mUnknown25A6 = false;
			mUnknown3478.SetInVal(0);
		} else {
			mUnknown25A4 = false;
			mUnknown25A6 = true;
			if (!mUnknown23F5 && (!mUnknown37C4 || gApp->Is3DAccelerated())) {
				mBoardX = aTo;
				SnapPiecesX();
			} else {
				if (!mUnknown3920) {
					mUnknown25A4 = false;
					if (mUnknown3478.GetInVal() == 0.0)
						mUnknown3478.SetCurve(GTS(0x7df2ac));
					mUnknown3478.IncInVal();
				}
				if (mUpdateCnt == 0 && !mUnknown2041)
					mUnknown3478.SetInVal(1.0);
				double anOut = mUnknown3478.GetOutVal();
				double anOut2 = mUnknown3478.GetOutVal();
				mBoardX = Ftol((1.0 - anOut) * (double)aFrom + (double)aTo * anOut2);
				if (!mUnknown37C4 && !mUnknown2629) {
					MoveCursor(GetRowAt(mUnknown15B8), GetColAt(mUnknown15B4), false, nullptr);
				} else if (mUnknown1430 != nullptr) {
					SetCursor(mUnknown1430->mRow, mUnknown1430->mCol);
				}
				SnapPiecesX();
			}
		}
	} else {
		mUnknown25A6 = false;
	}
	// Whether the mouse is over the grid and the player may play
	if (mUnknown15B4 < GetColX(0) || GetColX(mCols) < mUnknown15B4 || mUnknown15B8 < GetRowY(0) ||
		GetRowY(mRows) < mUnknown15B8 || IsBusy() ||
		(mUnknown21A3 && !mApp->mUnknownF1C && mUnknown20D8) || mApp->mOptionsDialog != nullptr ||
		mUnknown160 != 0 || mUnknown265C == 4 || mUnknown1570 != 0 || mUnknown1574 != 0 || mFrozen ||
		mApp->mUnknownF24 != nullptr) {
		mUnknown20D0 = false;
	} else {
		mUnknown20D0 = true;
	}
	if (mUnknown160 == 1 || mUnknown2629) {
		mUnknown20D0 = true;
		mUnknown20CC = 1.0f;
	}
	if (0.0f < mUnknown23D8) {
		float aLeft = (float)((double)mUnknown23D8 - (double)0.01f);
		mUnknown23D8 = aLeft;
		if (aLeft <= 0.0f) {
			// port: 0x6c425f comes with the challenges' boards
			NotPorted("Board::UpdateGame: 0x6c425f");
			return;
		}
	}
	if (mUnknown1430 == nullptr)
		mUnknown1C40 = false;
	MarkDirty();
	if (0.0f < mUnknown2038) {
		float aFade = (float)((double)mUnknown2038 - (double)0.02f);
		mUnknown2038 = aFade < 0.0f ? 0.0f : aFade;
	}
	if (mUnknown21A8 != nullptr)
		mUnknown21A8->Update();
	if (mUnknown210C) {
		// 0x74db1b on the +0x211c object: nothing in the exe sets +0x210c, so it never runs
		NotPorted("Board::UpdateGame: 0x74db1b");
	}
	if (0.0f < mUnknown1C48) {
		float aFade = (float)((double)mUnknown1C48 - (double)0.01f);
		mUnknown1C48 = aFade < 0.0f ? 0.0f : aFade;
	}
	if (0.0f < mUnknown2164) {
		float aFade = (float)((double)mUnknown2164 - (double)0.01f);
		mUnknown2164 = aFade < 0.0f ? 0.0f : aFade;
	}
	if (mUnknown26B0.empty()) {
		// The turn ends a while after the board settles
		if (0.0f < mUnknown37F4) {
			if (mGameMode == MODE_BLITZ)
				mUnknown25A4 = false;
			if (mUnknown21D8)
				mUnknown37F4 = mUnknown37F4 <= 0.1f ? 0.1f : mUnknown37F4;
			if (mUnknown37C4)
				mUnknown37F4 = 1.0f;
			float aLeft = (float)((double)mUnknown37F4 - (double)0.01f);
			mUnknown37F4 = aLeft;
			if (aLeft <= 0.0f && mGameMode != MODE_BLITZ && mUnknown2584 == nullptr) {
				if (CountDownCounters())
					return;
				if (mGameMode == MODE_ZEN && !mApp->mProfile->GetFlag1E4(0x11) && 179999 < mUnknown1470) {
					ShowZenJump(2);
				} else if (mGameMode == MODE_CLASSIC && !mApp->mProfile->GetFlag1E4(0x11) && mUnknown21E0 < 3) {
					ShowZenJump(1);
				} else {
					StartUnknown34F0();
					mApp->PlayVoice(SOUND_VOICE_GO, false);
				}
			}
		}
	} else {
		mUnknown37F4 = 0.1f;
		mUnknown25A4 = false;
		// The souls of the last level's special gems; each one back at its cell makes that gem special again
		bool aLanded = false;
		for (unsigned i = 0; i < mUnknown26B0.size(); i++) {
			GemSoul* aSoul = mUnknown26B0[i];
			if (mHyperspace != nullptr && (mHyperspace->mState == 6 || mHyperspace->mFlag15C))
				break;
			aSoul->Update();
			if (!aSoul->mUnknown73C)
				continue;
			int aY = Ftol((double)aSoul->mY);
			Piece* aPiece = GetPieceAtXY(Ftol((double)aSoul->mX), aY);
			if (aPiece != nullptr) {
				bool aQuiet = mUnknown37F8;
				mUnknown37F8 = true;
				if (aSoul->mType & (1 << 3)) {
					MakeElectroGem(aPiece, 0.0f);
				} else if (aSoul->mType & (1 << 6)) {
					Make6Gem(aPiece, 0.0f);
				} else if (aSoul->mType & (1 << 4)) {
					MakePowerGem(aPiece, 0.0f);
				} else if (aSoul->mType & (1 << 0)) {
					MakeFruitGem(aPiece);
					aPiece->mColor = aSoul->mColor;
				} else if (aSoul->mType & (1 << 7)) {
					// port: 0x6a4fc0 comes with the gems of flag 7 (no soul is made from one)
					NotPorted("Board::UpdateGame: a soul's 0x6a4fc0");
				}
				if (1 < mUpdateCnt - mUnknown37FC) {
					mApp->PlaySample(SOUND_GEMSOUL_LANDS3);
					mUnknown37FC = mUpdateCnt;
				}
				aPiece->mFlags = aSoul->mType;
				aLanded = aSoul->mFirst;
				mUnknown37F8 = aQuiet;
			}
			delete aSoul;
			mUnknown26B0.erase(mUnknown26B0.begin() + i);
			i--;
		}
		if (aLanded)
			mApp->PlaySample(SOUND_GEMSOUL_LANDS3);
	}
	if (IsBusy()) {
		mUnknown25A4 = false;
		if (mHyperspace != nullptr) {
			if (3 < mHyperspace->mState)
				mUnknown1C90 = false;
			if (mHyperspace->mState == 10)
				mHyperspace->mClicked = true;
		}
		if (0.0f < mUnknown2108) {
			mUnknown25A4 = false;
			UpdateJump();
		}
	}
	if (mUnknown1C38 < 1.0f)
		mUnknown1C38 = (float)((double)mUnknown1C38 + 0.25);
	EaseCursor();
	float aFadeIn = (float)((double)mUnknown1FAC + (double)0.01f);
	mUnknown1FAC = 1.0f <= aFadeIn ? 1.0f : aFadeIn;
	if (mUnknown37C4)
		mUnknown1FAC = 1.0f;
	if (0.0f < mUnknown2150) {
		float aLeft = (float)((double)mUnknown2150 - (double)0.01f);
		mUnknown2150 = aLeft;
		if (aLeft < 0.0f)
			mUnknown2150 = 0;
	}
	if (mUnknown1454 <= 0.0f || 90.0f <= mUnknown1454) {
		float aLeft = (float)((double)mUnknown20C8 - (double)0.01f);
		mUnknown20C8 = aLeft;
		if (aLeft <= 0.0f)
			mUnknown20C8 = 0;
	} else {
		mUnknown20C8 = 1.0f;
	}
	if (mUnknown20B8 != 0.0f) {
		float aLeft = (float)((double)mUnknown20B8 - (double)0.01f);
		mUnknown20B8 = aLeft < 0.0f ? 0.0f : aLeft;
	}
	float aStep = -0.005f;
	if (mUnknown2014)
		aStep = 0.005f;
	if (0.0f < mUnknown20F0) {
		float aLeft = (float)((double)mUnknown20F0 - (double)0.02f);
		mUnknown20F0 = aLeft;
		if (aLeft < 0.0f)
			mUnknown20F0 = 0;
	}
	float anAngle = aStep + mUnknown1FB0;
	mUnknown1FB0 = anAngle;
	if (0.0f <= anAngle) {
		if (kTwoPi < anAngle)
			mUnknown1FB0 = anAngle - kTwoPi;
	} else {
		mUnknown1FB0 = anAngle + kTwoPi;
	}
	if ((mUnknown1378 == 0 || mUnknown1378 == 2) && mApp->mHasFocus) {
		mUnknown13B0.GetOutVal();
		bool aDone = !mUnknown13B0.IncInVal(0.01);
		mUnknown13B0.GetOutVal();
		if (aDone)
			mUnknown1378 = (mUnknown1378 != 0) * 2 + 1;
	}
	// port: the dialog numbered 28 is not yet known
	if (mApp->GetDialog(28) != nullptr) {
		MarkDirty();
		return;
	}
	if (mUnknown1570 == mUnknown1574 && 0 < mUnknown1574) {
		if (mPieceAlpha == 0.0f)
			return;
		float aFade = (float)((double)mPieceAlpha - (double)0.05f);
		mUnknown1644 = mApp->mDialogMap.size() == 0;
		mPieceAlpha = 0.0f < aFade ? aFade : 0.0f;
		MarkDirty();
		return;
	}
	mFXManager2->Update();
	if (!IsBusy() && mUnknown160 == 0) {
		if (mGameMode == MODE_CHALLENGE) {
			UpdateChallengeTimer();
		} else if (mGameMode == MODE_BLITZ && !IsBusy() && mUnknown2584 == nullptr) {
			// Blitz's clock, in hundredths
			if (!mUnknown25A6 && -1 < mUnknown3458)
				mUnknown3458--;
			int aSeconds = mUnknown3458 / 100;
			if ((mUnknown3458 + 1) % 100 == 0) {
				if (aSeconds < 1) {
					if (!mUnknown37C4) {
						mUnknown20D0 = false;
						aSeconds = 0;
						mUnknown3458 = 0;
						mApp->mUnknownC74[35] = 30000 / 100; // 0x6a697d
						if (!mUnknown21D8 && !mUnknown21DA && !mUnknown21D9 && mUnknown25A4 && mUnknown149C.empty() &&
							mUnknown248 == 0.0f && mUnknown14F8 == mUnknown168 && mUnknown14AC.empty() &&
							mUnknown267C == nullptr) {
							EndGame(false);
						}
					} else if (!mUnknown21D8 && !mUnknown21DA && !mUnknown21D9 && mUnknown25A4) {
						mUnknown3920 = true;
					}
				} else {
					ShowBlitzTime(aSeconds);
				}
			}
			mGoodTube->SetTime(aSeconds * 100, true);
		}
	}
	if (0.0 < mUnknown1618) {
		float aLeft = (float)((double)mUnknown1614 - (double)0.1f);
		mUnknown1614 = aLeft;
		if (aLeft < 0.0f)
			mUnknown1614 = (float)((double)mUnknown1614 + 5.0);
		mUnknown1628 = mUnknown1628 + (double)0.0005f;
		mUnknown1620 = mUnknown1620 - mUnknown1628;
		MarkDirty();
	}
	if (mUpdateCnt % 16 == 0) {
		mUnknown1FF8++;
		if (mUnknown1FF8 == 8)
			mUnknown1FF8 = 0;
	}
	mApp->mUnknown780++;
	if (mPieceAlpha < 1.0f) {
		float aFade = (float)((double)mPieceAlpha + (double)0.05f);
		mPieceAlpha = 1.0f <= aFade ? 1.0f : aFade;
		MarkDirty();
	}
	if (0 < mUnknown15D4) {
		mUnknown15D4--;
		if (mUnknown15D4 == 0)
			mGoodTube->mWidgetFlagsMod.mRemoveFlags &= ~WIDGETFLAGS_ALLOW_MOUSE;
	}
	Widget::Update();
	mUnknown1470++;
	if (mUnknown160 == 7)
		MarkDirty();
	if (mUpdateCnt == 2)
		mApp->ClearUpdateBacklog(false);
	if (mUnknown15C == 0) {
		mUnknown14F4++;
		if (mUnknown2016 && (mUnknown1C6D || (mUnknown2008 != 0 && --mUnknown2008 == 0))) {
			mUnknown2008 = 0;
			MatchAfterMove();
		}
	}
	if (mUnknown1430 != nullptr && mUpdateCnt % 8 == 0) {
		MarkDirty();
		mUnknown1434++;
	}
	int aCount38 = 0;  // the gems with +0x38 set: they send the board to 0x6c87d2 below
	int aRemoved = 0;
	if (mUnknown160 == 0) {
		mUnknown1F74 = 0;
		mUnknown1F78 = 0;
		mUnknown1F7C = 0;
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece != nullptr) {
					if (aPiece->IsFlagSet(4))
						mUnknown1F74++;
					if (aPiece->IsFlagSet(3))
						mUnknown1F7C++;
					if (aPiece->IsFlagSet(9))
						mUnknown1F78++;
				}
			}
		}
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				Cell& aCell = mCells[mCols * aRow + aCol];
				aCell.mUnknown10 = false;
				UpdateCell(aCell);
				if (aPiece == nullptr)
					continue;
				if (!aPiece->mBlasted) {
					if (aPiece->mUnknown38)
						aCount38++;
					if (aPiece->mUnknown14 == 0.0f && aPiece->mUnknown535) {
						int anUnderRow = GetRowAt(Ftol(aPiece->mY));
						int anUnderCol = GetColAt(Ftol(aPiece->mX));
						if (-1 < anUnderRow && anUnderRow < mRows && -1 < anUnderCol && anUnderCol < mCols) {
							Cell& anUnder = mCells[anUnderRow * mCols + anUnderCol];
							Light aLight;
							if (aPiece->IsFlagSet(3) && Rand() % 2 == 0) {
								aLight.mPieceId = -1;
								anUnder.mUnknown28 = 1.0f;
								aLight.mUnknown18 = true;
								anUnder.mUnknown24 = 0;
								float aRadius = (float)((double)gGemWidth * (double)0.4f);
								float anAngle = (float)((double)RandSigned() * (double)3.14159265f);
								float aCos = cosf(anAngle);
								aLight.mX = (float)((double)(gGemWidth / 2) + (double)aPiece->mX + (double)aCos * (double)aRadius);
								float aSin = sinf(anAngle);
								aLight.mY = (float)((double)(gGemHeight / 2) + (double)aPiece->mY + (double)aRadius * (double)aSin);
								aLight.mUnknown10 = 0;
								aLight.mUnknown14 = 1.0f;
								aLight.mUnknown8 = 8.0f;
								aLight.mUnknownC = 28.0f;
								aLight.mUnknown1C = 0.1f;
								aLight.mUnknown20 = 0.1f;
								aLight.mPiece = Rand() % 8 != 0 ? aPiece : nullptr;
								if (aLight.mPiece == nullptr)
									aLight.mUnknown14 = 0.25f;
								mUnknown1650.push_back(aLight);
							}
							if (aPiece->IsFlagSet(4)) {
								if (Rand() % 3 == 0) {
									aLight.mUnknown18 = true;
									anUnder.mUnknown24 = 1.0f;
									float anAngle = (float)((double)RandSigned() * (double)3.14159265f);
									aLight.mPieceId = aPiece->mId;
									float aCos = cosf(anAngle);
									aLight.mX = (float)((double)gGemWidth * (double)aCos * 0.25 + (double)(gGemWidth / 2));
									float aSin = sinf(anAngle);
									aLight.mY = (float)(((double)aSin - (double)0.1f) * (double)gGemHeight * (double)0.35f +
														(double)(gGemHeight / 2));
									aLight.mUnknown10 = 0;
									aLight.mUnknown14 = 0.4f;
									aLight.mUnknown8 = 9.0f;
									aLight.mUnknownC = 28.0f;
									aLight.mPiece = Rand() % 8 != 0 ? aPiece : nullptr;
									if (aLight.mPiece == nullptr)
										aLight.mUnknown14 = 0.15f;
									aLight.mUnknown1C = 0.06f;
									aLight.mUnknown20 = 0.06f;
									mUnknown1650.push_back(aLight);
								}
							} else if (aPiece->IsFlagSet(6)) {
								anUnder.mUnknown24 = -1.0f;
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
				if (aPiece->mUnknown538 != 0) {
					Piece* aNew = new Piece();
					aNew->mPendingKind = aPiece->mUnknown538;
					aPiece->mPendingKind = 0;
					aNew->mPendingTime = 0.0001f;
					aNew->Update();
					aNew->mUnknown51D = true;
					aNew->mRow = aPiece->mRow;
					aNew->mCol = aPiece->mCol;
					aNew->mX = aPiece->mX;
					aNew->mY = aPiece->mY;
					aNew->mColor = aPiece->mColor;
					RemovePiece(aPiece, true);
					mGrid[mCols * aNew->mRow + aNew->mCol] = aNew;
				} else {
					RemovePiece(aPiece, true);
					if (mUnknown15C < 1 && !IsBusy())
						mUnknown21DA = true;
					aRemoved++;
				}
			}
		}
		if (!mUnknown21E4.empty()) {
			mUnknown25A4 = false;
			// port: the objects at +0x21e4 (0x6f465f) are not ported yet
			NotPorted("Board::UpdateGame: the objects at +0x21e4");
		}
		if (aRemoved != 0 && DropPieces())
			FillInBlanks(false);
		for (int aCol = 0; aCol < mCols; aCol++) {
			float aStep = (float)((double)gApp->mScale * 0.375);
			float aLift = aStep + mUnknown133C[aCol];
			mUnknown133C[aCol] = aLift <= 0.0f ? aLift : 0.0f;
		}
	}
	// The cells under the cursor's corners
	int aCursorX = mUnknown1C00;
	int aCursorY = mUnknown1C04;
	Piece* aCorners[4];
	aCorners[0] = GetPieceAtXY(aCursorX, aCursorY);
	aCorners[1] = GetPieceAtXY(aCursorX + gGemWidth, aCursorY);
	aCorners[2] = GetPieceAtXY(aCursorX, aCursorY + gGemWidth);
	aCorners[3] = GetPieceAtXY(aCursorX + gGemWidth, aCursorY + gGemHeight);
	for (int i = 0; i < 4; i++) {
		Piece* aPiece = aCorners[i];
		if (!CanMove(aPiece) && aPiece != nullptr)
			mCells[aPiece->mRow * mCols + aPiece->mCol].mUnknown10 = true;
	}
	if (mUnknown15C == 0) {
		float aShine = (float)((double)mShineAmount + (double)0.025f);
		mShineAmount = aShine;
		if (1.0f < aShine)
			mShineAmount = 1.0f;
	}
	mShineAmount = mUnknown3924 * mShineAmount;
	float aGlow = (float)((double)mUnknown20CC - (double)0.01f);
	mUnknown20CC = aGlow;
	if (aGlow < 0.0f)
		mUnknown20CC = 0;
	LayoutTube();
	if (!IsBusy() && mUnknown1474 == nullptr && 0.0f < mShakeX) {
		double aStep = (double)mShakeX * (double)0.1f;
		double aMax = (double)mWidth * (double)0.015f;
		float aLeft = mShakeX - (float)(aStep <= aMax ? aStep : aMax);
		mShakeX = aLeft;
		if (aLeft < 2.0f) {
			mShakeX = 0;
			if (mUnknown2041 && !mGoodTube->mVisible) {
				StartUnknown3880();
				LayoutTube();
				mGoodTube->mVisible = true;
			}
			// The souls pick their gems of the new board (as RetargetTravellers, in one pass), each starting its swirl
			// a step round from the last
			float anAngle = (float)((double)RandSigned() * 3.1415927410125732);
			float aStep = (float)((double)6.2831854820251465f / (double)(unsigned)mUnknown26B0.size());
			for (unsigned i = 0; i < mUnknown26B0.size(); i++) {
				GemSoul* aSoul = mUnknown26B0[i];
				int aCells = mRows * mCols;
				int aCell = Rand85B970() % aCells;
				for (int n = 0; n < aCells; n++) {
					Piece* aPiece = mGrid[aCell];
					if (aPiece != nullptr && aPiece->mFlags == 0) {
						bool aTaken = false;
						for (unsigned j = 0; j < mUnknown26B0.size(); j++) {
							if (mUnknown26B0[j]->mState == 2 && mUnknown26B0[j]->mUnknown680 == aPiece) {
								aTaken = true;
								break;
							}
						}
						if (!aTaken && (!(aSoul->mType & (1 << 0)) || aPiece->mColor == aSoul->mColor)) {
							aSoul->SetTarget(aPiece);
							aSoul->mAngle = anAngle;
							aSoul->mUnknown8F4 = true;
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
	switch (mUnknown160) {
	case 0:
		if (mUnknown21D9) {
			UpdateTwist();
		} else {
			float aFade = (float)((double)mUnknown20D4 - (double)0.04f);
			mUnknown20D4 = 0.0f < aFade ? aFade : 0.0f;
		}
		if (mUnknown149C.empty())
			aSettled = true;
		CheckSettled();
		if (0.0f < mUnknown2598) {
			float aFade = (float)((double)mUnknown2598 - (double)0.01f);
			mUnknown2598 = 0.0f < aFade ? aFade : 0.0f;
		}
		if (mUnknown21A0)
			mUnknown20CC = 0;
		if (!IsBusy()) {
			float aGlow = (float)((double)mUnknown20CC + (double)0.02f);
			mUnknown20CC = aGlow;
			if (1.0f < aGlow)
				mUnknown20CC = 1.0f;
			if (mUnknown15C == 0) {
				if (mUnknown21A3 && !mUnknown21D8 && !mUnknown21DA && !mUnknown21D9 && mUnknown149C.empty() &&
					mUnknown2584 == nullptr && mUnknown168 == mUnknown14F8 && mUnknown2170.empty() &&
					mGoodTube->IsIdle() && mUnknown1660.empty() && !mUnknown214 && mUnknown2208 == nullptr &&
					mUnknown23DC.empty() && mUnknown248 == 0.0f && mUnknown14AC.empty() && mUnknown265C != 4) {
					if (mUnknown1FB8 != mUnknown1FC4)  // 0x6aa1ef
						mUnknown1FC4 = mUnknown1FB8;
					LevelComplete();
					mUnknown21A3 = false;
					mUnknown21A2 = false;
					if (mUnknown2201)
						mUnknown2204++;
				} else if (mUnknown2669 && !mUnknown21D8 && !mUnknown21DA && !mUnknown21D9 && mUnknown149C.empty() &&
						   mChallenge != 0 && mGoodTube->IsIdle()) {
					mUnknown266A = true;
					if ((mChallenge->mFlags & Challenge::FLAG_NIGHTMODE) == 0) {
						// the next challenge of the category, and the next category, open
						Profile* aProfile = mApp->mProfile;
						aProfile->SetChallengeDone(mChallenge->mCategory, mChallenge->mNumber);
						aProfile->Save();
						aProfile->OpenChallengeCategory(mChallenge->mCategory + 1);
						ShowChallengeComplete(false);
					} else {
						ShowChallengeComplete(true);
						std::vector<int>& aBests = mApp->mProfile->mInts204;
						int aCategory = mChallenge->mCategory;
						if ((int)aBests.size() <= aCategory)
							aBests.resize(aCategory + 1, 0);  // 0x43c2a0
					}
					mUnknown2669 = false;
				}
			}
		}
		aFall = aSettled;
		break;
	case 3:
		if (IsBusy())
			break;
		if (mGoodTube->mUnknown4AC == 4)
			mUnknown160++;
		aFall = true;
		break;
	case 4: {
		float aLeft = (float)((double)mUnknown345C - (double)0.01f);
		mUnknown345C = aLeft;
		if (aLeft <= 0.0f)
			mUnknown160++;
		aFall = true;
		break;
	}
	case 5: {
		// The level's start: once the wait is up and the board has settled, "go"
		float aLeft = (float)((double)mUnknown345C - (double)0.01f);
		mUnknown345C = aLeft;
		if (aLeft <= 0.0f && mUnknown37F4 <= 0.0f) {
			StartUnknown34F0();
			mApp->PlayVoice(SOUND_VOICE_GO, false);
			mUnknown345C = 1.0f;
			mGoodTube->SetUnknown4A8(true);
			PlayGameMusic();
			mUnknown160 = 0;
		}
		aFall = true;
		break;
	}
	case 7:
		// The game is over
		if (!mUnknown36B1) {
			UpdateGameOver();
		} else if (UpdateNuke()) {
			return;
		}
		break;
	case 8:
		mUnknown203C--;
		if (mUnknown203C < 0) {
			if (mGameMode == MODE_PUZZLE) {
				// port: the puzzle's fill (0x6c3515) draws from Rand; it comes with the puzzles
				NotPorted("Board::UpdateGame: the puzzle's fill");
				mUnknown160 = 0;
				mUnknown15C = 0;
				return;
			}
		} else if (mGameMode == MODE_PUZZLE) {
			// port: the puzzle's sparkles (Rand for the cell, an effect 12 over it) come with the puzzles
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
	mUnknown25A5 = !mUnknown21D8 && !mUnknown21DA && !mUnknown21D9 && mUnknown248 == 0.0f && mUnknown149C.empty() &&
				   mUnknown265C != 4 && mGoodTube->mUnknown3B0 == 0;
	// The progress bar's ends follow the grid's right and middle
	mUnknown2698 = mUnknown2594 < GetGridMidY();
	if (0.0f < mUnknown20EC) {
		mUnknown20EC = (float)((double)mUnknown20EC - (double)0.01f);
	} else if (!mUnknown37C4) {
		bool aLow = (double)mUnknown2594 < (double)GetGridMidY() * (double)0.7f;
		float aFade = (float)((double)mUnknown25A0 - (double)0.01f);
		mUnknown25A0 = 0.0f <= aFade ? aFade : 0.0f;
		if (mUnknown2590 < GetGridRight()) {
			mUnknown25A0 = 1.0f;
			double aTo = (double)gGemWidth * 0.5 + (double)mUnknown2590;
			int aRight = GetGridRight();
			mUnknown2590 = (double)aRight < aTo ? GetGridRight() : Ftol(aTo);
		} else if (mUnknown2698) {
			mUnknown25A0 = 1.0f;
			double aTo = (double)gGemHeight * (double)0.05f + (double)mUnknown2594;
			float aMid = GetGridMidY();
			mUnknown2594 = (float)((double)aMid < aTo ? (double)GetGridMidY() : aTo);
		}
		if (aLow)
			mUnknown2224 = 0.2f;
	}
	if (!mUnknown149C.empty() || aCount38 != 0) {
		UpdateStrikes();
	}
	if (mUnknown14AC.empty())
		mUnknown3905 = false;
	UpdateGeodes();
	MarkDirty();
	// The board's shake
	if (!(0.0f < mUnknown151C)) {
		mUnknown1520 = 0;
		mUnknown1524 = 0;
	} else if (mUpdateCnt % 3 == 0) {
		float aFade = (float)((double)mUnknown151C - (double)0.1f);
		mUnknown151C = aFade;
		if (aFade < 0.0f)
			mUnknown151C = 0;
		int aRand = CrtRand() % 2000 - 1000;
		mUnknown1520 = Ftol((double)mUnknown151C * (double)mUnknown1518 * ((double)aRand / 1000.0));
		aRand = CrtRand() % 2000 - 1000;
		mUnknown1524 = Ftol((double)mUnknown151C * (double)mUnknown1518 * ((double)aRand / 1000.0));
	}
	// Now and then a glint runs over the board
	if (mUnknown1420) {
		float aNext = (float)((double)mUnknown1424 + (double)0.02f);
		mUnknown1424 = aNext;
		if (1.0f < aNext)
			mUnknown1420 = false;
		MarkDirty();
	} else if (Rand() % 6000 == 0) {
		mUnknown1420 = true;
		mUnknown1424 = 0;
	}
	float aDim = (float)((double)mUnknown1428 - (double)0.06f);
	if (!(0.0f < aDim))
		aDim = 0;
	mUnknown1428 = aDim;
	if (!(0.0f < aDim) && mUnknown15C == 0) {
		double aNext = (double)0.06f + (double)mUnknown142C;
		mUnknown142C = (float)(aNext < 1.0 ? aNext : 1.0);
	}
	if (mUnknown1BF0 && Rand() % 2 == 0) {
		Effect* anEffect = mFXManager->AllocEffect(0x15);
		anEffect->mX = (float)(int)(Rand() % 1024);
		anEffect->mY = -2.0f;
		anEffect->mDY = (float)((double)RandSigned() * 0.25 + 0.5);
		mFXManager->AddEffect(anEffect);
	}
	if (mUnknown152C) {
		SoundInstance* aSound = mApp->mSoundManager->GetSoundInstance(SOUND_BOMB_EXPLODE);
		if (aSound != nullptr)
			aSound->Play(false, true);
		mUnknown152C = false;
	}
	if (mUnknown15C == 0)
		mUnknown230 = (float)((double)mUnknown230 + (double)0.01f);
	mUnknown15D1 = mUnknown15D0;
	mUnknown15D0 = false;
	UpdateUnknown248();
	if (mUnknown21D8)
		mUnknown25A4 = false;
	if (mUnknown21DA)
		mUnknown25A4 = false;
	if (mUnknown21D9)
		mUnknown25A4 = false;
	if (mUnknown2108 != 0.0f)
		mUnknown25A4 = false;
	if (0.0f < mUnknown23D8)
		mUnknown25A4 = false;
	if (mUnknown15C != 0)
		mUnknown25A4 = false;
	// A fruit's blast and the bonus challenge it starts (+0x265c, BreakPiece: 1 or 2 when a fruit broke, 3 the
	// challenge, 4 the fruit dance when it is met, 5 when it is missed)
	if (mUnknown265C == 2) {
		if (mGoodTube->mUnknown330 == 0.0f)
			mUnknown2660 = 0;
		float aT = (float)((double)mUnknown248 * 2.799999952316284 - 0.44999999999999996 - 0.5);
		aT = (float)(((double)aT - 0.30000001192092896) * 0.8500000238418579);
		mUnknown260.SetInVal(aT);
		float aRadius = (float)(mUnknown260.GetOutVal() * 4.400000095367432 * (double)gGemWidth);
		int aX = GetColX(mUnknown250) + gGemWidth / 2;
		int aY = GetRowY(mUnknown254) + gGemHeight / 2;
		int aTubeX = mGoodTube->mX - Ftol((double)gApp->mScale * -300.0);
		int aTubeY = mGoodTube->mY - Ftol((double)gApp->mScale * -900.0);
		float aDX = (float)(aX - aTubeX);
		float aDY = (float)(aY - aTubeY);
		float aDist = sqrtf((float)((double)aDX * (double)aDX + (double)aDY * (double)aDY));
		// The challenge starts when the last ring reaches a point by the tube (or the blast is over or nearly)
		if (mGoodTube->mUnknown330 == 0.0f &&
			(mUnknown248 == 0.0f || fabsf((float)(aDist - aRadius)) < 10.0f || 0.8999999761581421 <= (double)mUnknown248)) {
			mGoodTube->mUnknown3A8 = 0;
			mUnknown2664 = -1;
			mUnknown265C = 3;
			mGoodTube->mUnknown5F0.SetCurve(GTS(0x7dcf14));
			StartBonusChallenge(mUnknown420);
		}
	} else if (mUnknown265C == 5) {
		if (mGoodTube->mUnknown330 == 0.0f) {
			mUnknown265C = 0;
			mUnknown2660 = 0;
			mBonusColors.clear();
			mGoodTube->mUnknown3A8 = 0;
		}
	}
	if (mUnknown265C == 4 && mUnknown2584 == nullptr) {
		if (mUnknown267C == nullptr) {
			if (mUnknown25A4) {
				// The fruit dance (fruitdance.pam) over the board, with its own music
				mGoodTube->mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_ALLOW_MOUSE;
				// port: the original also writes a "FRUIT DANCE!" line (the mode, level, rank and play time) to its
				// debug log (app +0x1024)
				mGoodTube->mUnknown5F0.SetCurve(GTS(0x7df1c8));
				PopAnim* anAnim = new PopAnim(0, this);
				mUnknown267C = anAnim;
				mUnknown2680 = 0;
				mUnknown2684 = 0;
				mUnknown2688 = 0;
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
		} else if (2200 <= mUnknown2680) {
			// The dance is over: the game's music again
			RemoveWidget(mUnknown267C);
			delete mUnknown267C;
			mUnknown267C = nullptr;
			mUnknown265C = 0;
			mUnknown2660 = 0;
			mBonusColors.clear();
			mGoodTube->mUnknown3A8 = 0;
			mUnknown21DA = true;
			if (mApp->mMusicInterface != nullptr)
				mApp->mMusicInterface->SetVolume(mApp->mMusicVolume);
			if (!mUnknown37C4) {
				if (mGameMode == MODE_BLITZ)
					PlayGameMusic();
				else
					mApp->PlayMusic(mUnknown2154 + 100, true, true, 0, 0);
			} else if (mApp->mMainMenu != nullptr) {
				mApp->mMainMenu->PlayMusic(-1);
			}
		}
		mUnknown25A4 = false;
	}
	if (mUnknown160 == 2) {
		if (mUpdateCnt % 2 == 0) {
			// port: state 2 puts the gems of +0x148c back on the board one by one, picked by Rand, each with a glow
			// (IMAGE_INGAMEUI_GRIDSQUAREGLOW); it comes with the puzzles' undo
			NotPorted("Board::UpdateGame: state 2");
		}
	} else if (mUnknown160 == 1) {
		// A replay's rewind: DrawOverlay draws the recorded frames back from the last, faster and faster; at the
		// first the move is made again
		mUnknown25A4 = false;
		mUnknown2A80.SRand(mUpdateCnt / 3);
		int aStep = mUnknown3448 / 100 + 2;
		if (mUnknown3448 / 20 <= aStep)
			aStep = mUnknown3448 / 20;
		mUnknown3448 -= aStep;
		if (aStep <= 0)
			mUnknown3448--;
		if (gGraphicsRecorder->GetLastFrame() < 0 || mUnknown3448 < 0) {
			if (mRewindSound != nullptr) {
				mRewindSound->Release();
				mRewindSound = nullptr;
			}
			mUnknown160 = 0;
			mUnknown3444 = 0;
			StartTwist(false, -1, -1);
		}
		// the board itself is not drawn while the frames play back
		gGraphicsRecorder->mSkipDraw = true;
	} else if (mUnknown2629) {
		mUnknown2A80.SRand(mUpdateCnt / 7);
	}
	if (mUnknown160 != 1 && mUnknown2108 == 0.0f && mUnknown20DC == 0 && mUnknown1378 == 1) {
		// The recorder keeps the board's frames while it moves, up to 500 of them
		if (!mUnknown25A4 && mUnknown3444 < 500) {
			mUnknown3444++;
			int aFrame = mUnknown3444;
			int aGap = aFrame - gGraphicsRecorder->GetLastFrame();
			bool aRecord = true;
			if (aGap < 10 && 19 < aFrame && (49 < aFrame || aGap < 5))
				aRecord = false;
			gGraphicsRecorder->mRecording = aRecord;
			gGraphicsRecorder->mFrame = mUnknown3444;
		}
		gGraphicsRecorder->mSkipDraw = false;
	}
	if (mUnknown25A4 && !mFrozen) {
		mUnknown25A8++;
		bool aTutorialBusy = false;
		// (the tutorial's dialog holds the hints back)
		if (mUnknown2584 != nullptr && mUnknown2584->mDialog != nullptr)
			aTutorialBusy = true;
		if (!mApp->mShowHints || mUnknown25C8 != 0 || mGameMode == MODE_CHALLENGE || -1 < mUnknown2400 ||
			aTutorialBusy) {
			UpdateHint();
		} else {
			// With the hints on, a hint once the player has waited long enough
			mUnknown25D0 = (float)((double)mUnknown25D0 + (double)0.01f);
			float aWait;
			if ((unsigned int)gApp->mProfile->mGamesPlayed <= 1 && mUnknown158 <= 1)
				aWait = 5.0f;
			else
				aWait = 15.0f;
			int aLimit = Ftol(aWait);
			if (mUnknown2004)
				aLimit = 3;
			if ((float)aLimit <= mUnknown25D0) {
				ShowHint();
				mUnknown25D0 = 0;
			}
		}
	}
	int anUpdateCnt = mUpdateCnt;
	if (!mUnknown36B0 && mUnknown160 == 0 && !IsBusy() && mUnknown2584 == nullptr &&
		(mGameMode != MODE_CHALLENGE || !mUnknown2669)) {
		// The seconds of play, for the stats
		mUnknown14E4 += anUpdateCnt - mUnknown14E0;
		if (1000 / gApp->mFrameTime <= mUnknown14E4) {
			if (mGameMode == MODE_CHALLENGE && mChallenge != 0 && HasChallengeFlag(2) && !mUnknown21D8 &&
				!mUnknown21DA && !mUnknown21D9 && mUnknown25A4 && mUnknown149C.size() == 0 && mUnknown25A5) {
				// a timed challenge runs out through the goal check (0x6d090f)
				CheckGoal(true);
			}
			AddStat(35, 1);
			mUnknown14E4 = mUnknown14E4 % (1000 / gApp->mFrameTime);
		}
	}
	mUnknown14E0 = anUpdateCnt;
	if (mUnknown23F5) {
		// The tutorial's arrow fades in while the cursor is off its cell
		int aCol = GetColAt(mUnknown1C00);
		int aRow = GetRowAt(mUnknown1C04);
		if (mUnknown2400 == aCol && mUnknown2404 == aRow) {
			double aNext = (double)mUnknown23F8 - 0.075;
			mUnknown23F8 = (float)(0.0 < aNext ? aNext : 0.0);
		} else {
			double aNext = (double)mUnknown23F8 + 0.075;
			mUnknown23F8 = (float)(aNext < 1.0 ? aNext : 1.0);
		}
	}
	if (0 < mUnknown1F88.mAlpha) {
		int aNext = mUnknown1F88.mAlpha - 1;
		mUnknown1F88.mAlpha = aNext < 0 ? 0 : aNext;
	}
	if (mShakeX == 0.0f && mUnknown3880.GetOutVal() == 0.0 && mUnknown2584 == nullptr) {
		// The tip slides in, waits, and slides back
		mUnknown830.IncInVal();
		if (0.0f < mUnknown8BC) {
			float aLeft = (float)((double)mUnknown8BC - (double)0.01f);
			mUnknown8BC = aLeft;
			if (!(0.0f < aLeft))
				mUnknown830.mIncRate = mUnknown830.mIncRate * -1.0;
		}
	}
	mUnknown34F0.IncInVal();
	// The cell under the cursor: a tick as it moves onto another gem
	int aRow = GetRowAt(mUnknown1C04);
	int aCol = GetColAt(mUnknown1C00);
	bool aTick = mUnknown1474 == nullptr && mUnknown20D0 && !mUnknown2030 && !(mGameMode == MODE_PUZZLE && mUnknown1324) &&
				 !(mUnknown2584 != nullptr && mUnknown2584->mMouseVisible) && mUnknown265C != 4;
	if (aCol != mUnknown1C10 || aRow != mUnknown1C14) {
		if (aCol < 0) {
			mUnknown1C18 = 0;
			goto ticked;
		}
		if (aTick && mUnknown15C == 0) {
			mUnknown1C18 = 0;
			mApp->PlaySoundEx(SOUND_SELECT, GetPan(gGemWidth / 2 + mUnknown1C00), 0.0f, 0.0f, true, true);
		}
	}
	if (aCol < 0 || !mUnknown25A4)
		mUnknown1C18 = 0;
ticked:
	// The cursor's glow, after 40 updates on one cell in the first level
	if (mUnknown1C18 == 0)
		mUnknown23FC = 0;
	mUnknown1C18++;
	if (40 < mUnknown1C18 && mUnknown158 < 1) {
		double aNext = (double)mUnknown23FC + (double)0.01f;
		mUnknown23FC = (float)(aNext < 0.5 ? aNext : 0.5);
	}
	if (GetUnknown23F5() && -1 < mUnknown2400)
		mUnknown23FC = mUnknown2400 == aCol && mUnknown2404 == aRow ? 0.5f : 0.0f;
	if (mApp->mUnknownF85) {
		double aNext = (double)mUnknown3924 + (double)0.015f;
		float aFade = (float)(1.0 < aNext ? 1.0 : aNext);
		mUnknown3924 = aFade;
		if (aFade == 1.0f)
			mUnknown20EC = 0;
	}
	mUnknown1C10 = aCol;
	mUnknown1C14 = aRow;
	MarkDirty();
	UpdateToolTips();
	if (mUnknown3904 && !mUnknown37C4 && mUnknown25A5 && !mUnknown23F5 && !mUnknown21A3 && mUnknown26A0 == nullptr &&
		mUnknown26AC == nullptr && mUnknown2208 == nullptr && mUnknown23DC.empty() && !mUnknown3905) {
		if (mGameMode != MODE_CHALLENGE && mGameMode != MODE_BLITZ)
			ShowReplayPod();
		if (mUnknown2629) {
			mWidgetFlagsMod.mRemoveFlags &= ~WIDGETFLAGS_ALLOW_MOUSE;
			mUnknown2629 = false;
		}
		mUnknown3904 = false;
		mUnknown3905 = false;
	}
	if (-1 < mApp->mAutoPlayMode && mUnknown25A5 && !mUnknown21D9) {
		if (!mUnknown25A4)
			return;
		if (mUnknown1570 == 0 && !IsBusy() && !mUnknown2629 && !mUnknown37C4 &&
			(mUnknown2208 == nullptr || !mUnknown2208->mMouseVisible) && mUpdateCnt % 15 == 0) {
			// -autoplay's next move: a click at the corner the move turns about, or a random one
			int aRow;
			int aCol;
			FindMove(&aRow, &aCol, false);
			if (aRow == -1 || aCol == -1) {
				aRow = (int)Rand() % mRows;
				aCol = (int)Rand() % mCols;
			}
			mUnknown15BC = GetColX(aCol) + gGemWidth;
			mUnknown15C0 = GetRowY(aRow) + gGemHeight;
			mUnknown15C4 = mUpdateCnt;
		}
	}
	// A click queued by -autoplay
	if (mUnknown25A4 && mUnknown15C4 != 0 && mUpdateCnt - mUnknown15C4 < 20) {
		mUnknown15C4 = 0;
		if (-1 < mApp->mAutoPlayMode)
			mUnknown20D0 = true;
		MouseDown(mUnknown15BC, mUnknown15C0, 0, 1);
		mUnknown15C4 = 0;
	}
}

void Board::UpdateCell(Cell& theCell)
{
	if (0.0f < theCell.mUnknown4) {
		float aFade = (float)((double)theCell.mUnknown4 - (double)0.01f);
		theCell.mUnknown4 = aFade;
		if (aFade < 0.0f)
			theCell.mUnknown4 = 0;
	}
	if (0.0f < theCell.mUnknown28) {
		float aFade = (float)((double)theCell.mUnknown28 - (double)0.01f);
		theCell.mUnknown28 = aFade < 0.0f ? 0.0f : aFade;
	}
	if (0.0f < theCell.mUnknown24) {
		float aFade = (float)((double)theCell.mUnknown24 - (double)0.01f);
		theCell.mUnknown24 = aFade < 0.0f ? 0.0f : aFade;
	} else if (theCell.mUnknown24 < 0.0f) {
		float aFade = (float)((double)theCell.mUnknown24 + (double)0.01f);
		theCell.mUnknown24 = 0.0f < aFade ? 0.0f : aFade;
	}
	int aLeft = theCell.mColor.mAlpha - 3;
	theCell.mColor.mAlpha = aLeft < 0 ? 0 : aLeft;
	if (gBoard->mUnknown2584 != nullptr) {
		int aState = gBoard->mUnknown2584->mState;
		if (aState != Tutorial::STATE_NONE && aState != Tutorial::STATE_MATCHES && aState != Tutorial::STATE_LEVELUP)
			theCell.mUnknown34 = false;
	}
	if (!theCell.mUnknown34) {
		float aFade = (float)((double)theCell.mUnknown3C - (double)0.04f);
		theCell.mUnknown3C = aFade < 0.0f ? 0.0f : aFade;
	} else {
		float aFade = (float)((double)theCell.mUnknown3C + (double)0.02f);
		theCell.mUnknown3C = 1.0f < aFade ? 1.0f : aFade;
	}
	if (theCell.mUnknown3C == 0.0f)
		theCell.mUnknown38 = 0;
	else
		theCell.mUnknown38++;
	if (theCell.mUnknown40 != 0.0f) {
		float aNext = (float)((double)theCell.mUnknown40 + (double)0.006f);
		theCell.mUnknown40 = aNext;
		if (1.0f < aNext)
			theCell.mUnknown40 = 0;
	}
}

bool Board::DropPieces()
{
	if (mUnknown1C90)
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
					aPiece->mUnknown114 = false;
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
							aPiece->mUnknown114 = true;
							mGrid[mCols * aTo + aCol] = aPiece;
							if (mUnknown210C)
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
	bool aLocked = thePiece->IsFlagSet(8) || thePiece->IsFlagSet(2);
	if (thePiece->IsFlagSet(9) && !mUnknown1C6C)
		aLocked = true;
	if (thePiece->IsFlagSet(1) && !mUnknown1C78)
		aLocked = true;
	if (thePiece->IsFlagSet(5))
		aLocked = true;
	return !aLocked;
}

void Board::LayoutTube()
{
	if (mGameMode == MODE_PUZZLE) {
		if (IsBusy())
			return;
		// port: the puzzle's widget at +0x23d4 slides 5% of its x each update; it comes with the puzzles
		NotPorted("Board::LayoutTube: the puzzle's widget");
		return;
	}
	if (mUnknown3810.mIncRate != 0.0 && !mUnknown3810.HasBeenTriggered()) {
		// The new level: the board slides in (mShakeX), the tube shows once it is in
		if (!mUnknown3810.IncInVal() && !(0.0f < mUnknown380C))
			mGoodTube->SetVisible(true);
		double anOut = mUnknown3810.GetOutVal();
		mShakeX = (float)(anOut * (double)(mWidth - GetColX(0)));
	} else if (!mUnknown3880.HasBeenTriggered() && !mUnknown3880.IncInVal()) {
		if (0.0 < mUnknown3880.GetOutVal()) {
			mGoodTube->SetVisible(false);
		} else if (mGameMode == MODE_BLITZ && mApp->mPlayPressed) {
			mGoodTube->SetTime(mUnknown3458, false);
		}
		if (0.0f < mUnknown380C && mUnknown3810.mOutVal == 0.0) {
			mUnknown25A6 = true;
			mUnknown3810.SetCurve(GTS(0x7dca10));
		}
	}
	// The tube beside the board, out by the curve
	int anOldX = mGoodTube->mX;
	Board* aBoard = gBoard;
	int aRoom = (aBoard->mWidth - GetBoardWidth() - aBoard->mGoodTube->GetTubeWidth()) / 3;
	double anOut = mUnknown3880.GetOutVal();
	int aSlide = Ftol((double)gApp->mScale * 600.0);
	double aX = (double)aRoom - anOut * (double)aSlide;
	int aMargin = Ftol((double)gApp->mScale * 30.0);
	int aNewX = Ftol(aX - (double)aMargin);
	if (aNewX != anOldX && !mUnknown36B0 && (!mUnknown37C4 || !mUnknown25A4)) {
		GoodTube* aTube = mGoodTube;
		int aHeight = aTube->GetTubeHeight();
		aTube->Resize(aNewX, aTube->mY, aTube->GetTubeWidth(), aHeight);
	}
}

void Board::UpdateGlows()
{
	if (0.0f < mUnknown21B0)
		mFXManager3->Update();
	// port: the original reads the music's volume at +0x2c without a check for none; the port has none without sound
	CustomBassMusicInterface* aMusic = mApp->mTwistMusic;
	if (IsSparkBlocked()) {
		// The music fades out while the sparks are held back, and lightning plays over the board
		if (aMusic != nullptr && 0.0f < aMusic->mVolumeFactor) {
			float aFade = (float)((double)aMusic->mVolumeFactor - (double)0.01f);
			aMusic->mVolumeFactor = aFade < 0.0f ? 0.0f : aFade;
		}
		float aGlow = (float)((double)0.01f + (double)mUnknown21B0);
		mUnknown21B0 = 1.0f < aGlow ? 1.0f : aGlow;
		if (!IsBusy()) {
			float aLeft = (float)((double)mUnknown21BC - (double)0.01f);
			mUnknown21BC = aLeft;
			if (aLeft <= 0.0f) {
				float aRand = RandSigned();
				mUnknown21BC = (float)((double)fabsf(aRand) * 0.75 + (double)0.1f);
			}
			aLeft = (float)((double)mUnknown21C0 - (double)0.01f);
			mUnknown21C0 = aLeft;
			if (aLeft <= 0.0f) {
				mUnknown21C0 = 4.0f;
				if (mUnknown21C4[mUnknown21CC] != nullptr) {
					mUnknown21C4[mUnknown21CC]->Stop();
					mUnknown21C4[mUnknown21CC]->Release();
				}
				mUnknown21CC ^= 1;
			}
			Rand();
			Effect* anEffect = mFXManager->AllocEffect(0x13);
			float anAngle = (float)((double)RandSigned() * (double)kTwoPi);
			float aSin = sinf(anAngle);
			anEffect->mX = (float)((double)gBoard->mUnknown1C2C + 89.0 + (double)aSin * 96.0);
			float aCos = cosf(anAngle);
			anEffect->mY = (float)((double)gBoard->mUnknown1C30 + 84.0 + (double)aCos * 96.0);
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
		if (0.0f < mUnknown21B0) {
			float aGlow = (float)((double)mUnknown21B0 - (double)0.01f);
			mUnknown21B0 = aGlow < 0.0f ? 0.0f : aGlow;
		}
	}
	// The 24 glows run round, faster while +0x21d9 is on
	float* aGlow = mUnknown2058;
	int aLeft = 24;
	do {
		float aStep = mUnknown21D9 ? 0.05f : 0.02f;
		float aFade = *aGlow - aStep;
		*aGlow = aFade;
		if (aFade <= 0.0f)
			*aGlow = 0.0f;
		aGlow++;
		aLeft--;
	} while (aLeft != 0);
	if (mUpdateCnt % ((!mUnknown21D9) * 8 + 2) == 0) {
		mUnknown20BC++;
		if (24 <= mUnknown20BC)
			mUnknown20BC = 0;
		float& aLit = mUnknown2058[mUnknown20BC];
		aLit = 1.0f < aLit ? aLit : 1.0f;
		float& anOpposite = mUnknown2058[(mUnknown20BC + 12) % 24];
		anOpposite = anOpposite < 1.0f ? 1.0f : anOpposite;
	}
	if (0 < mUnknown1C44) {
		int aNext = mUnknown1C44 - 2;
		mUnknown1C44 = aNext < 0 ? 0 : aNext;
	}
	// The lights rise to full, then fade and go
	for (int i = 0; i < (int)mUnknown1650.size(); i++) {
		Light& aLight = mUnknown1650[i];
		if (aLight.mUnknown18) {
			float aNext = aLight.mUnknown1C + aLight.mUnknown10;
			aLight.mUnknown10 = aNext;
			if (1.0f <= aNext) {
				aLight.mUnknown10 = 1.0f;
				aLight.mUnknown18 = false;
			}
		} else {
			float aNext = aLight.mUnknown10 - aLight.mUnknown20;
			aLight.mUnknown10 = aNext;
			if (aNext <= 0.0f) {
				mUnknown1650.erase(mUnknown1650.begin() + i);
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
			aPoint->mUnknown18 = aScale;
			aScale = (float)((double)mShineAmount * (double)aScale + (1.0 - (double)mShineAmount));
			aPoint->mUnknown18 = aScale;
			int anIndex = Ftol((double)aPoint->mAngle * 4096.0 / 6.28318) % 4096;
			float anX = (float)((double)gCosTable[anIndex] * (double)aPoint->mDist * (double)aScale);
			aPoint->mX = (float)((double)anX + 42.0);
			float aY = (float)((double)gSinTable[anIndex] * (double)aPoint->mDist * (double)aPoint->mUnknown18);
			aPoint->mY = (float)(42.0 + (double)aY);
			aPoint++;
			aCols--;
		} while (aCols != 0);
		aRows--;
	} while (aRows != 0);
}

void Board::UpdateLevelUp()
{
	if (mUnknown20DC == 0)
		return;
	mUnknown37F4 = 0.15f;
	mUnknown20CC = 0;
	float aShine = (float)((double)mShineAmount - (double)0.015f);
	mShineAmount = aShine;
	if (aShine < 0.0f)
		mShineAmount = 0;
	int aCells = mCols * mRows;
	if (mUnknown20DC < 2) {
		// The tiles turn over in rings from the middle out
		float aLeft = (float)((double)mUnknown1630 - (double)0.01f);
		mUnknown1630 = aLeft;
		if (aLeft <= 0.0f) {
			mUnknown1634--;
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
							if (!aCell.mUnknown2C) {
								float aDist = sqrtf((float)(aDCol * aDCol + aDRow * aDRow));
								if ((float)mUnknown1634 <= aDist) {
									aCell.mUnknown2C = true;
									aCell.mUnknown30 = 1.0f;
								}
							}
							aRow++;
							aDRow--;
						} while (aRow < mRows);
					}
					aCol++;
					mUnknown1630 = 0.3f;
					aDCol--;
				} while (aCol < mCols);
			}
		}
		Cell* aCell = mCells;
		while (aCells != 0) {
			Cell* aThis = aCell;
			aCells--;
			aCell++;
			if (aThis->mUnknown2C && 0.0f < aThis->mUnknown30)
				aThis->mUnknown30 = (float)((double)aThis->mUnknown30 - (double)0.03f);
		}
		if (0.0f < mUnknown2224) {
			float aFade = (float)((double)mUnknown2224 - (double)0.015f);
			mUnknown2224 = aFade;
			if (aFade <= 0.0f)
				mUnknown2224 = 0;
		}
	}
	if (mUnknown20DC == 2) {
		if (mHyperspace != nullptr) {
			if (mHyperspace->mClicked) {
				// The jump is over: the next level's board
				if (!mUnknown23F5 && !mUnknown2041) {
					mUnknown2041 = true;
					RemoveWidget(mGoodTube);
					AddWidget(mGoodTube);
				}
				mUnknown158++;
				if (mUnknown158 == 0 && mUnknown2584 == nullptr)
					mUnknown158 = 1;
				SettleTurn();
				// The original writes "LEVEL++" to the debugger here
				mGoodTube->SetLevel(mUnknown158);
				if (mGameMode == MODE_BLITZ) {
					if (mApp->mMusicInterface != nullptr)
						mApp->mMusicInterface->FadeOutAll(true, 0.004);
				} else if (mGameMode == MODE_ZEN) {
					NextLevelMusic();
				}
				mUnknown1390 = true;
				if (!mUnknown21A2 && GetUnknown23F5() && mUnknown2584 != nullptr)
					mUnknown2584->Preload();
				NewLevel();
				FillInBlanks(false);
				if (mGameMode == MODE_BLITZ) {
					if (mApp->mPlayPressed) {
						mGoodTube->StartUnknown4AC(false, false);
						mUnknown160 = 3;
					}
					mGoodTube->SetUnknown4A8(mApp->mPlayPressed);
				}
				ResetLevelValues();
			}
			if (mHyperspace->mClicked) {
				mApp->mWidgetManager->RemoveWidget(mHyperspace);
				mApp->SafeDeleteWidget(mHyperspace);
				mHyperspace = nullptr;
				mUnknown20DC = 3;
				mUnknown2040 = true;
				if (gApp->mProfile->GetFlag1E4(14)) {
					mUnknown150C = true;
					if (mGameMode != MODE_ZEN && (mGameMode < 2 || 3 < mGameMode)) {
						// The next tip, which slides in from the right
						mUnknown82C = gApp->mProfile->mUnknown1F0;
						gApp->mProfile->mUnknown1F0++;
						int aTip = mUnknown82C;
						if (aTip < (int)mTips.size()) {
							mUnknown8A0 = mTips[aTip];
							mUnknown830.SetCurve(GTS(0x7dd59c));
							mUnknown8C0 = (float)mApp->mWidth;
						}
					}
				} else {
					mUnknown150C = false;
				}
				// 0x6a3e70: waits for the app's saving thread (+0x7e8) to finish; the port saves at once
			}
		}
	} else if (mUnknown20DC == 3) {
		FinishLevelUp();
	}
	MarkDirty();
}

void Board::FinishLevelUp()
{
	mApp->Is3DAccelerated();
	mShineAmount = 1.0f;
	mUnknown20DC = 0;
	mUnknown15FC = true;
	// The original gathers each row's columns into a vector it then frees, and asks each gem for flags 4 and 3 without
	// using the answers; neither has any effect
	mUnknown1618 = 0;
	mUnknown1398 = true;
	mUnknown1620 = 0;
	mUnknown1628 = 0;
	if (!mUnknown21A2)
		MakeCells();
	if (!GetUnknown23F5() && mGameMode != 5)
		mUnknown2041 = true;
	mUnknown2042 = true;
	mUnknown2043 = true;
	MouseMove(mApp->mWidgetManager->mLastMouseX, mApp->mWidgetManager->mLastMouseY);
}

void Board::PlayGameMusic()
{
	if (mApp->mMusicInterface != nullptr)
		mApp->mMusicInterface->StopAllMusic();
	mApp->PlayMusic(300, false, false, mUnknown3928, mUnknown392C);
	mUnknown3928 = 0;
	mUnknown392C = 0;
}

void Board::LevelComplete()
{
	if (IsBusy())
		return;
	mApp->FadeOutMusic();
	if (mUnknown158 == 0) {
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
		if (0 < mUnknown158) {
			ShowLevelUp();
			mUnknown269C = GetBonusLevel(mUnknown158);
			ShowRankBonus(mUnknown269C);
		}
		mUnknown380C = mApp->mVizManager->m2D ? 3.5f : 11.0f;
		mUnknown3880.SetCurve(GTS(0x7dc580));
		return;
	}
	mUnknown26A4 = 0;
	mUnknown1EC = mUnknown1E8;
	mUnknown1F4 = mUnknown1F0;
	mUnknown1508 = 0;
	mUnknown262A = false;
	mUnknown26C0 = 0;
	mUnknown1C90 = true;
	int aSize = mRows;
	if (mRows <= mCols)
		aSize = mCols;
	mUnknown1630 = 0;
	mUnknown2598 = 1.0f;
	mUnknown1634 = aSize / 2;
	mUnknown2108 = 7.9f;
	mUnknown26A8 = 1;
	mUnknown23F4 = mUnknown23F5;
	if (0 < mUnknown158) {
		ShowLevelUp();
		if (!mUnknown23F4)
			ShowRankBonus(mUnknown269C);
	}
	TakeHyperspace(nullptr);
	// The trial's jump (14) ends the game after level 10 of classic or 15 of zen; the copy is registered
	int aTransition = 8;
	if (((10 <= mUnknown158 && mGameMode == MODE_CLASSIC) || (15 <= mUnknown158 && mGameMode == MODE_ZEN)) &&
		!mApp->mRegistered)
		aTransition = 14;
	mHyperspace->SetTransition(aTransition);
	mUnknown20C8 = 1.0f;
	int aLeft = mCols * mRows;
	Piece** aCell = mGrid;
	while (aLeft != 0) {
		Piece* aPiece = *aCell;
		aLeft--;
		aCell++;
		if (aPiece != nullptr) {
			if (aPiece->mUnknownB0) {
				aPiece->mUnknownB0 = false;
				aPiece->mX = (float)aPiece->mUnknownA0;
				aPiece->mY = (float)aPiece->mUnknownA4;
			}
			if (aPiece->mBreaking || (aPiece->IsFlagSet(0) && mUnknown2138))
				aPiece->mBlasted = true;
			if (mUnknown1C7B && (aPiece->IsFlagSet(1) || aPiece->IsFlagSet(2)))
				aPiece->mUnknown88 = 1;
		}
	}
}

void Board::UpdateJump()
{
	// The level's end (LevelComplete sets mUnknown26A8 to 1): the "level complete" show (1) or none (2), then the rows
	// break up one by one as the hyperspace runs, every gem left becoming a soul; then (4, 0) the next level
	if (mUnknown26A8 == 0) {
		if (mUnknown2208 == nullptr) {
			mUnknown2108 = (float)((double)mUnknown2108 - 0.01);
			for (unsigned i = 0; i < mUnknown26B0.size(); i++) {
				mUnknown26B0[i]->mRectY = Ftol((double)mHeight * 0.30000001192092896);
				mUnknown26B0[i]->mRectHeight = Ftol((double)mHeight * 0.5);
			}
			ClearShockwaves();
			EndJump(true);
		}
		SettlePieces();
		return;
	}
	if (mUnknown26A8 == 1) {
		if (mUnknown26AC == nullptr) {
			mUnknown26A8 = 2;
		} else {
			int aShown = mUnknown26AC->mUpdateCnt;
			if ((350 <= aShown && !mUnknown23F4) || (200 <= aShown && mUnknown23F4)) {
				mApp->PlaySample(SOUND_WHIRLPOOL1);
				mUnknown26A8 = 4;
			}
		}
	} else if (mUnknown26A8 != 2) {
		if (mUnknown26A8 == 4)
			mUnknown26A8 = 0;
		SettlePieces();
		return;
	}
	// The row breaking up now
	int aRow = (mHyperspace->mUpdateCnt - 120) / 12;
	if (mUnknown23F4)
		aRow = (mHyperspace->mUpdateCnt - 45) / 12;
	if (aRow == mRows) {
		mUnknown26A8 = 0;
		SettlePieces();
		return;
	}
	bool aSounded = false;
	for (int aCol = 0; aCol < mCols; aCol++) {
		Piece* aPiece = GetPieceAt(aRow, aCol);
		if (aPiece == nullptr)
			continue;
		aPiece->mUnknown538 = 0;
		if (aPiece->IsFlagSet(4) || aPiece->IsFlagSet(0) || aPiece->IsFlagSet(3) || aPiece->IsFlagSet(6))
			continue;
		if (aPiece->IsFlagSet(2) || aPiece->IsFlagSet(1) || aPiece->IsFlagSet(8) || aPiece->IsFlagSet(9)) {
			float aY = (float)(gGemHeight / 2) + aPiece->mY;
			float aX = (float)(gGemWidth / 2) + aPiece->mX;
			BlowUpPiece(aPiece, aX, aY, true, false);
		} else if (!aPiece->mBreaking) {
			if (!aSounded) {
				aSounded = true;
				mApp->PlaySample(SOUND_ROWBLOW);
			}
			aPiece->mBreaking = true;
			if (aPiece->IsFlagSet(10))
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
		aSoul->mFirst = mUnknown26B0.empty();
		mUnknown26B0.push_back(aSoul);
		aSoul->Update();
		mGrid[mCols * aSoulRow + aCol] = nullptr;
		float aLeft = 0.33f;
		if (0.33f < mUnknown2108)
			aLeft = mUnknown2108;
		mUnknown2108 = aLeft;
		mUnknown26A4 = 0.01f;
		break;
	}
	SettlePieces();
}

void Board::ClearShockwaves()
{
	for (unsigned i = 0; i < mUnknown1660.size(); i++)
		delete mUnknown1660[i];
	mUnknown1660.clear();
}

void Board::EndJump(bool theUnused)
{
	(void)theUnused;
	if (mUnknown2629) {
		mWidgetFlagsMod.mRemoveFlags &= ~WIDGETFLAGS_ALLOW_MOUSE;
		mUnknown2629 = false;
	}
	// (the original asks the app whether it draws with Direct3D, 0x4a7900, and keeps nothing of it)
	mUnknown37B0 = true;
	mUnknown37B1 = true;
	ClearSparkStarts();
	mUnknown2108 = 0;
	mApp->mProfile->mUnknown164 |= 1 << 0;  // 0x733774
	mUnknown1618 = 0;
	mUnknown1614 = 0;
	mUnknown1638 = false;
	mUnknown1430 = nullptr;
	mUnknown1478 = true;
	mUnknown1620 = 0;
	mUnknown1628 = 0;
	mApp->ClearUpdateBacklog(false);
	mUnknown1504 = 0;
	mUnknown1544 = 0;
	mUnknown200 = 0;
}

void Board::ShowLevelUp()
{
	mApp->PlayVoice(SOUND_VOICE_LEVELCOMPLETE, false);
	LevelUpShow* aShow = new LevelUpShow(this);
	mUnknown26AC = aShow;
	aShow->mZOrder = mGoodTube->mZOrder;
	mApp->mWidgetManager->AddWidget(mUnknown26AC);
}

bool Board::UpdateUnknown1660()
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
	if (!mUnknown1660.empty()) {
		do {
			Shockwave* aWave = mUnknown1660[i];
			aWave->Update();
			if (aWave->mDone) {
				// The last takes its place, and waits an update (the original goes on to the index after)
				delete aWave;
				mUnknown1660[i] = mUnknown1660[mUnknown1660.size() - 1];
				if (!mUnknown1660.empty())
					mUnknown1660.pop_back();
			}
			i++;
		} while (i < mUnknown1660.size());
	}
	return !mUnknown1660.empty();
}

bool Board::UpdateUnknown2170()
{
	unsigned int i = 0;
	if (!mUnknown2170.empty()) {
		do {
			Traveller* aTraveller = mUnknown2170[i];
			aTraveller->Update();
			if (aTraveller->mDone) {
				// (the next one waits an update: the original goes on to the index after)
				mUnknown2170.erase(mUnknown2170.begin() + i);
				delete aTraveller;
			}
			i++;
		} while (i < mUnknown2170.size());
	}
	int aLeft = mCols * mRows;
	int aGrowing = 0;
	Piece** aCell = mGrid;
	if (0 < aLeft) {
		do {
			Piece* aPiece = *aCell;
			if (aPiece != nullptr && aPiece->IsFlagSet(1) && aPiece->mScale288 < 1.0f)
				aGrowing++;
			aCell++;
			aLeft--;
		} while (aLeft != 0);
	}
	return 0 < (int)mUnknown2170.size() + aGrowing;
}

void Board::CheckSettled()
{
	bool aShaking = UpdateUnknown1660();
	bool aGrowing = UpdateUnknown2170();
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
								if (mUnknown210C)
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
	if (aStill && mUnknown25A4) {
		if (!mUnknown21DA) {
			if (!IsBusy() && mUnknown149C.empty() && !mUnknown21D8 && mUnknown214) {
				mUnknown214 = false;
				// The multiplier went past its top (SetLevelSteps) or a fruit dance ended: a fruit (AwardFruit),
				// and steps past 151 (at most 14) go on top of the steps of the levels below the board's
				AwardFruit(-1, -1);
				if (152 <= mUnknown224) {
					int aSteps = mUnknown224 - 151;
					if (14 < aSteps)
						aSteps = 14;
					mUnknown224 = aSteps;
					int aLevel = 1;
					if (1 < mUnknown21D0) {
						do {
							mUnknown224 += GetLevelSteps(aLevel);
							aLevel++;
						} while (aLevel < mUnknown21D0);
					}
					SetLevelSteps(mUnknown224);
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
						if (!aPiece->mUnknown1C)
							aAllDone = false;
						if (0 < aPiece->mUnknown88) {
							if (aPiece->mUnknown51D) {
								aPiece->mUnknown88 = 0;
								aCol++;
								continue;
							}
							if (aPiece->IsFlagSet(Piece::FLAG_BOMB) && aPiece->mBombCount == 0) {
								// a spent bomb shakes as its fuse runs down
								float aShake = (float)((double)(float)(100 - aPiece->mUnknown88) / 5.0);
								if (1.0f < aShake)
									aShake = 1.0f;
								aPiece->mUnknown29C = aShake;
								aPiece->mUnknown2A0 = (float)((double)RandSigned() * 3.1415927410125732);
							}
							aPiece->mUnknown88--;
							if (aPiece->mUnknown88 != 0) {
								aAllDone = false;
							} else {
								// the gem goes off
								float aMidX = (float)(gGemWidth / 2) + aPiece->mX;
								float aMidY = (float)(gGemHeight / 2) + aPiece->mY;
								bool aBreak = false;
								if (aPiece->IsFlagSet(Piece::FLAG_FRUIT)) {
									if (!mUnknown25A4 || mUnknown21D8)
										aPiece->mUnknown88 = 1;
									else
										aBreak = true;
								} else if (aPiece->IsFlagSet(Piece::FLAG_1)) {
									aBreak = true;
								} else if (aPiece->IsFlagSet(Piece::FLAG_ELECTRO)) {
									BreakPiece(aPiece);
									AddPoints(Ftol((double)(gGemWidth / 2) + (double)aPiece->mX),
										Ftol((double)(gGemHeight / 2) + (double)aPiece->mY), mUnknown1C58,
										gGemColors[aPiece->mColor], aPiece->mUnknown2EC, true, true, false, true);
									AddBlastChain();
								} else if (aPiece->IsFlagSet(Piece::FLAG_POWER)) {
									if (!aPiece->mUnknown537) {
										aPiece->mUnknown537 = true;
										AddPoints(Ftol((double)(gGemWidth / 2) + (double)aPiece->mX),
											Ftol((double)(gGemHeight / 2) + (double)aPiece->mY), mUnknown1C58,
											gGemColors[aPiece->mColor], aPiece->mUnknown2EC, true, true, false, true);
										AddBlastChain();
										AddStat(0x40, 1);
										mUnknown1FC++;
										mUnknown1F8++;
									}
									AddStat(aPiece->mColor + 0x4b, 1);
									AddStat(0x3e, 1);
									SetOffPowerGem(aPiece);
								} else if (aPiece->IsFlagSet(Piece::FLAG_BOMB)) {
									BlowUpPiece(aPiece, aMidX, aMidY, true, false);
									if (!mUnknown37C4)
										mFXManager->AddBombFlash(aMidX, aMidY, gGemColors[aPiece->mColor]);
									aPiece->mBlasted = true;
									aPiece->mUnknown116 = true;
									anExploded = true;
									aBreak = true;
								} else if (aPiece->IsFlagSet(Piece::FLAG_COAL)) {
									aBreak = true;
								} else {
									BlowUpPiece(aPiece, aMidX, aMidY, true, false);
									AddPoints(Ftol((double)(gGemWidth / 2) + (double)aPiece->mX),
										Ftol((double)(gGemHeight / 2) + (double)aPiece->mY), 10,
										gGemColors[aPiece->mColor], aPiece->mUnknown2EC, true, true, false, true);
									anExploded = true;
									aBreak = true;
								}
								if (aBreak)
									BreakPiece(aPiece);
								if (mUnknown15C == 0)
									Refill();
							}
						}
						if (0.0f < aPiece->mUnknown284) {
							// port: 0x6c812f's +0x284 growth (a 0x104-byte object, 0x74b46e) comes with the features
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
	if (!mUnknown1660.empty()) {
		UpdateUnknown1660();
		aAllDone = false;
	}
	if (aAllDone)
		mUnknown1C8C = 1.0f;
	return aAllDone;
}

void Board::FallPieces()
{
	if (mUnknown1C90)
		return;
	if (0 < mUnknown1394) {
		mUnknown1394--;
		return;
	}
	if (2 <= mUnknown20DC && mHyperspace != nullptr && mHyperspace->mTransition != 8 && mHyperspace->mTransition != 14)
		return;
	if (mUnknown1378 != 1)
		return;
	bool aStill = SettlePieces();
	if (0 < mUnknown219C)
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
					if (aPiece->IsFlagSet(1) && aPiece->mScale288 < 1.0f)
						aStill = false;
					if (aPiece->mUnknownB0)
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
						if (aPiece->mUnknown534)
							aTarget = (float)(mHeight + gGemHeight);
						else
							aTarget = (float)GetRowY(aRow);
						if (!(aTarget <= aPiece->mY) || aPiece->mUnknown34 < 0.0f) {
							mUnknown2199 = true;
							bool anOff = aPiece->mUnknown534;
							aStill = false;
							if (anOff) {
								aPiece->mX = aPiece->mPendingDX + aPiece->mX;
								aPiece->mY = aPiece->mY + aPiece->mUnknown34;
							} else if (aPiece->mPendingTime != 0.0f) {
								aPiece->mUnknown34 = 0;
							} else {
								float aY = aPiece->mY + aPiece->mUnknown34;
								aPiece->mY = aY;
								// It stops on a falling gem under it and takes its speed
								if (aPiece->mRow < mRows - 1) {
									Piece* aBelow = GetPieceAt(aPiece->mRow + 1, aPiece->mCol);
									if (aBelow != nullptr && !aBelow->mUnknown534) {
										double aLimit = (double)aBelow->mY - (double)gGemHeight;
										if (!((double)aY < aLimit) && 0.0f < aPiece->mUnknown34 && aBelow->mUnknown536) {
											aPiece->mY = (float)aLimit;
											aPiece->mUnknown34 = aBelow->mUnknown34;
										}
									}
								}
							}
							if (!anOff) {
								if (aPiece->mUnknown34 < 0.0f) {
									// Moving up: the gems over it go no faster
									int anAbove = aPiece->mRow;
									while (anAbove = anAbove - 1, -1 < anAbove) {
										Piece* anOther = GetPieceAt(anAbove, aPiece->mCol);
										if (anOther != nullptr && anOther->mY < aPiece->mY)
											anOther->mUnknown34 = aPiece->mUnknown34 <= anOther->mUnknown34 ? aPiece->mUnknown34 : anOther->mUnknown34;
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
								if (aPiece->mUnknown534)
									goto next;
								if (0.0f <= aPiece->mUnknown34) {
									if (aPiece->mUnknown51D)
										aPiece->mUnknown51D = false;
									aPiece->mUnknown538 = 0;
									aPiece->mUnknown537 = false;
								}
								aPiece->mUnknown34 = 0;
								aPiece->mUnknown2C = aPiece->mRow;
								if (!aPiece->IsFlagSet(0)) {
									aLanded++;
									if (!aPiece->mUnknown2E0 && mGameMode != MODE_PUZZLE) {
										aPiece->mUnknown2E0 = true;
										if (aPiece->IsFlagSet(9)) {
											aPiece->mJustMade = 4;
											mApp->PlaySample(SOUND_GEM_COUNTDOWN);
										} else if (aPiece->IsFlagSet(1)) {
											aPiece->mJustMade = 5;
										}
									}
								}
								int anX = Ftol(aPiece->mX);
								aPiece->mY = aTarget;
								aLandedX = gGemWidth / 2 + aLandedX + anX;
								aPiece->mUnknown34 = 0;
							} else if (aPiece->mUnknown14 != 0.0f || aPiece->mBreaking) {
								aPiece->mUnknown34 = 0;
							} else {
								// Gravity
								float anAccel = (float)((double)gApp->mScale * 0.375);
								float aSpeed = anAccel + aPiece->mUnknown34;
								aPiece->mUnknown34 = aSpeed;
								if (!((float)gGemHeight > aSpeed))
									aPiece->mUnknown34 = (float)(gGemHeight - Ftol(gApp->mScale));
							}
						}
					}
				next:
					aCol++;
				} while (aCol < mCols);
			}
			aRow--;
		} while (-1 < aRow);
		if (0 < aLanded && 9 < abs(mUnknown1510 - mUpdateCnt)) {
			mUnknown1510 = mUpdateCnt;
			mApp->PlaySoundEx(SOUND_GEM_HIT, GetPan(aLandedX / aLanded), 0.0f, 0.0f, true, true);
		}
	}
	if (aStill && mUnknown2199) {
		for (int r = 0; r < mRows; r++) {
			for (int c = 0; c < mCols; c++) {
				Piece* aPiece = GetPieceAt(r, c);
				if (aPiece != nullptr)
					aPiece->mUnknown30 = aPiece->mRow;
			}
		}
		mUnknown2199 = false;
		MatchAfterMove();
	}
	bool aWasMoving = mUnknown21D8;
	mUnknown21D8 = !aStill;
	if (!aStill)
		mUnknown25A4 = false;
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

void Board::UpdateUnknown248()
{
	// 0x6c4466: a broken fruit gem's blast (BreakPiece starts it). Four rings spread from its cell over 0.85 of
	// +0x248's run: the gems of its colour they cross break (another fruit becomes a power gem and goes off), bombs and
	// devil gems count up and locks break; then the board refills, and at 1 the turn settles
	if (mUnknown248 == 0.0f)
		return;
	bool aAll = false;
	float anOld = mUnknown248;
	float aNew = (float)((double)mUnknown248 + 0.0020000000949949026);
	mUnknown248 = aNew;
	if (anOld < 0.5f && 0.5f <= aNew) {
		// Half way: the fruit's own points
		AddPoints(GetColX(mUnknown250) + gGemWidth / 2, GetRowY(mUnknown254) + gGemHeight / 2, 500,
			gGemColors[mUnknown420], -0x68, true, true, false, true);
	}
	if (0.8500000238418579 < (double)mUnknown248) {
		if (1.0f <= mUnknown248) {
			mUnknown248 = 0;
			SettleTurn();
		}
		int anAmnesty = mAmnestyGemCount;
		mAmnestyGemCount = 0;
		mUnknown1C90 = false;
		if (DropPieces())
			FillInBlanks(false);
		mAmnestyGemCount = anAmnesty;
		aAll = true;
	} else {
		mUnknown25A4 = false;
		mUnknown1C90 = true;
		bool aSound = false;
		int aRow = 0;
		if (0 < mRows) {
			do {
				int aCol = 0;
				if (0 < mCols) {
					do {
						float aDX = (float)(aCol - mUnknown250);
						float aDY = (float)(aRow - mUnknown254);
						Piece* aPiece = GetPieceAt(aRow, aCol);
						if (aPiece != nullptr && 0.0f <= aPiece->mY) {
							aPiece->mUnknown34 = 0;
							float aDist = sqrtf((float)((double)aDX * (double)aDX + (double)aDY * (double)aDY));
							int aRing = 0;
							do {
								float aT = (float)((double)aRing * -0.15 + (double)mUnknown248 * 2.799999952316284 -
									0.5);
								if (aRing == 0) {
									if (aT < 0.0f) {
										aPiece->mJustMade = 0;
										aPiece->mDevilGlow = 0;
									}
								} else if (aRing == 3) {
									aT = (float)(((double)aT - 0.30000001192092896) * 0.8500000238418579);
								}
								mUnknown260.SetInVal(aT);
								mUnknown2D0.SetInVal(aT);
								float aRadius = (float)(mUnknown260.GetOutVal() * 4.400000095367432);
								if (fabsf((float)(aDist - aRadius)) < 0.3f) {
									// The ring crosses this cell
									if (aPiece->mColor == mUnknown420 && !aPiece->IsFlagSet(Piece::FLAG_COAL) &&
										aPiece->mId != mUnknown258 && !aPiece->IsFlagSet(Piece::FLAG_DEVIL)) {
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
													aPiece->mUnknown51D = false;
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
											aPiece->mUnknown29C = (float)((double)(aRing + 1) * 0.30000001192092896);
										}
									}
									if ((aPiece->IsFlagSet(Piece::FLAG_BOMB) || aPiece->IsFlagSet(Piece::FLAG_DEVIL)) &&
										aPiece->mDevilGlow < 0.6f) {
										// A bomb or devil gem counts up by the ring's number
										aPiece->mBombCount += aRing + 1;
										aPiece->MakeCountImage();
										aPiece->mDevilGlow = 1.0f;
										aPiece->mJustMade = 1;
										aPiece->mCurve1A0.SetCurve(GTS(0x7dd424));
										aSound = true;
									} else if (aRing == 3 && !aPiece->IsFlagSet(Piece::FLAG_12)) {
										if (aPiece->IsFlagSet(Piece::FLAG_LOCKED))
											aPiece->BreakLock(false);
										aPiece->mUnknown4FC = 0;
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
			gApp->PlaySoundEx(SOUND_BLITZ_COUNTUP, 0, (float)((double)mUnknown248 * 1.5), 0.0f, true, true);
		// The fruit itself fades as it grows
		Piece* aFruit = GetPieceById(mUnknown258);
		if (aFruit != nullptr) {
			mUnknown340.SetInVal(mUnknown248);
			mUnknown3B0.SetInVal(mUnknown248);
			double anAlpha = 1.0 - ((double)mUnknown248 - 0.4000000059604645) * 6.0;
			aFruit->mAlpha = (float)(1.0 < anAlpha ? 1.0 : anAlpha);
			aFruit->mDrawScale = (float)mUnknown3B0.GetOutVal();
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
					aPiece->mUnknown2F6 = true;
				else
					aPiece->mUnknown2F6 = !(aPiece->mColor == mUnknown420 || aPiece->IsFlagSet(Piece::FLAG_BOMB) ||
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
		int anIndex = mUnknown2684;
		mUnknown2684 = anIndex + 1;
		aPoints = (anIndex < 42 ? kSmall[anIndex] : 0) / 2;
		aPos = theCommand.substr(6);
	} else if (theCommand.substr(0, 5) == GTS(0x7dd414)) {
		// (likewise; fruitdance.pam sends 15)
		int anIndex = mUnknown2688;
		mUnknown2688 = anIndex + 1;
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
	Points* aFloater = AddPoints(mUnknown267C->mX + aX, mUnknown267C->mY + aY, aPoints, gGemColors[aColor], -0x68,
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
	int aState = mUnknown25C8;
	if (aState == 1) {
		// the hint's gems flash one after another
		for (unsigned int i = 0; i < mUnknown25B8.size(); i++) {
			Piece* aPiece = GetPieceById(mUnknown25B8[i]);
			if (aPiece != nullptr)
				aPiece->mUnknownE8 = (float)((double)(float)i * 0.25 + 0.001);
		}
		mUnknown25C8 = 0;
		mUnknown25D0 = 0;
	} else if ((unsigned int)(aState - 2) < 2) {
		// sparkles circle the hint's cell for a turn (state 3 then flashes the gems)
		float aTurn = (float)((double)kTwoPi * (double)0.008f + (double)mUnknown25D4);
		mUnknown25D4 = aTurn;
		if (kTwoPi <= aTurn) {
			mUnknown25D4 = 0;
			if (aState == 3) {
				mUnknown25C8 = 1;
			} else {
				mUnknown25C8 = 0;
				mUnknown25D0 = 0;
				mUnknown25D4 = 0;
			}
		}
		float anAngle = mUnknown25D4;
		if (mUpdateCnt % 2 == 0) {
			EffectsManager* aFXManager = mApp->mBoard->mFXManager;
			for (int aSide = 2; aSide != 0; aSide--) {
				Effect* anEffect = gApp->mBoard->mFXManager->AllocEffect(0x11);
				RandSigned();
				float aCos = cosf(anAngle);
				float aX = (float)((double)(GetColX(mUnknown25DC) + gGemWidth) +
					(double)gGemWidth * (double)1.25f * (double)aCos);
				float aSin = sinf(anAngle);
				float aY = (float)((double)(GetRowY(mUnknown25D8) + gGemHeight) +
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
bool Board::AnyCellUnknown34()
{
	int aCount = mCols * mRows;
	for (int i = 0; i < aCount; i++) {
		if (mCells[i].mUnknown34)
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
	if (AnyCellUnknown34())
		return;
	FindMove(&mUnknown25D8, &mUnknown25DC, false);
	if (mUnknown25D8 < 0)
		return;
	// every third hint (and the first) sparkles before it flashes
	int aHints = mUnknown25CC + 1;
	mUnknown25C8 = aHints;
	mUnknown25CC = aHints;
	if (3 < aHints) {
		mUnknown25CC = 0;
		mUnknown25C8 = 1;
	}
	mUnknown25D4 = 0;
	if (mGameMode == 1 || mGameMode == 4) {
		bool aHasBomb = false;
		for (unsigned int i = 0; i < mUnknown25B8.size(); i++) {
			Piece* aPiece = GetPieceById(mUnknown25B8[i]);
			if (aPiece != nullptr && aPiece->IsFlagSet(Piece::FLAG_BOMB))
				aHasBomb = true;
		}
		Piece* aPiece;
		// a new player's early levels point at the most urgent bomb or lock instead
		if (mApp->mProfile->mGamesPlayed == 0 && mUnknown158 < 4 && !aHasBomb &&
			(aPiece = FindHintPiece()) != nullptr) {
			mUnknown25B8.clear();
			mUnknown25B8.push_back(aPiece->mId);
			mUnknown25C8 = 1;
		}
		if ((mUnknown1559 && mUnknown158 < 2) || mUnknown2004) {
			mUnknown25C8 = 2;
			for (unsigned int i = 0; i < mUnknown25B8.size(); i++) {
				Piece* aHintPiece = GetPieceById(mUnknown25B8[i]);
				if (aHintPiece != nullptr)
					mCells[mCols * aHintPiece->mRow + aHintPiece->mCol].mUnknown40 = 0.001f;
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
	if (mUnknown25A4) {
		int aMouseX = mWidgetManager->mLastMouseX;
		int aMouseY = mWidgetManager->mLastMouseY;
		// nothing is said while the mouse is over the tutorial's dialog
		Rect aTutorialRect(0, 0, 0, 0);
		if (mUnknown2584 != nullptr && mUnknown2584->mDialog != nullptr)
			aTutorialRect = Rect(mUnknown2584->mDialog->mX, mUnknown2584->mDialog->mY, mUnknown2584->mDialog->mWidth,
				mUnknown2584->mDialog->mHeight);
		if (!aTutorialRect.Contains(aMouseX, aMouseY)) {
			GoodTube* aTube = mGoodTube;
			if (aTube != nullptr) {
				if (0 < aTube->mX) {
					if (!aTube->mUnknown14D) {
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
				int aMeterY = Ftol((double)aTube->mY + (double)aTube->mUnknown194 - (double)Sc(10));
				Rect aMeterRect(Sc(0x88) + aTube->mX, aMeterY, Sc(0x8e), Sc(0x3b));
				if (aMeterRect.Contains(aMouseX, aMouseY)) {
					if (mGameMode == MODE_BLITZ || (mGameMode == MODE_CHALLENGE && mChallenge != 0 && HasChallengeFlag(2))) {
						aTitle = mApp->GetString(GTS(0x7dea60), GT(0x7dea30));
						aBody = mApp->GetString(GTS(0x7dea00), GT(0x7de9a0));
					} else if (mGameMode != MODE_CHALLENGE) {
						aTitle = mApp->GetString(GTS(0x7deb10), GT(0x7deae4));
						int aPercent = Ftol(((double)mUnknown200 / (double)mUnknown204) * 100.0);
						aBody = StrFormat(mApp->GetString(GTS(0x7deac0), GT(0x7dea78)).c_str(), aPercent, mUnknown158);
					} else {
						aTitle = mApp->GetString(GTS(0x7ded2c), GT(0x7decf4));
						aBody = mApp->GetString(GTS(0x7decb8), GT(0x7dec40));
						aWidth = Sc(0x12c);
					}
					aY = Ftol((double)aTube->mY + (double)aTube->mUnknown194 + (double)Sc(5));
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
						} else if (0 < aPiece->mUnknown4FC && !aPiece->IsFlagSet(Piece::FLAG_12)) {
							aTitle = mApp->GetString(GTS(0x7de2c4), GT(0x7de29c));
							aBody = mApp->GetString(GTS(0x7de228), GT(0x7de140));
							aWidth += Sc(0);
						} else if (aPiece->IsFlagSet(Piece::FLAG_BOMB)) {
							aTitle = mApp->GetString(GTS(0x7dcce4), GT(0x7dccc0));
							aBody = mApp->GetString(GTS(0x7de0e8), GT(0x7de040));
							aWidth += Sc(0x32);
						} else if (aPiece->IsFlagSet(Piece::FLAG_COAL)) {
							if (mUnknown1BF2) {
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
						} else if (aPiece->IsFlagSet(Piece::FLAG_7)) {
							aTitle = mApp->GetString(GTS(0x7ddc4c), GT(0x7ddc28));
							aBody = mApp->GetString(GTS(0x7ddb98), GT(0x7dda78));
						} else if (aPiece->IsFlagSet(Piece::FLAG_1)) {
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
						if ((double)(mUnknown714 + mUnknown70C) <= (double)mWidth * 0.8999999761581421)
							aX = aPieceX - Sc(0);
						else
							aX = aPieceX + Sc(0);
					}
				}
			}
		}
	}

	// 0x6d64e2: a new text starts the wait again; the same text for long enough brings the tooltip up
	if (aBody != mUnknown7F8) {
		mUnknown7F8 = aBody;
		mUnknown6D0 = 0;
		mUnknown788.SetCurve(GTS(0x7dd7dc));
	} else {
		int aStayed = mUnknown6D0;
		mUnknown6D0 = aStayed + 1;
		if (aWait <= aStayed && !aBody.empty() && aBody != mUnknown6F0) {
			mUnknown6CC = true;
			mUnknown70C = aX;
			mUnknown710 = aY;
			mUnknown6D4 = aTitle;
			mUnknown6F0 = aBody;
			mUnknown714 = aWidth;
			if (aSound != -1)
				mApp->PlaySample(aSound);
			mUnknown718.SetCurve(GTS(0x7dd7b8));
			mUnknown718.SetMode(CurvedVal::MODE_REPEAT);
			mUnknown788.SetCurve(GTS(0x7dd798));
		}
	}
	mUnknown718.IncInVal();
	if (!mUnknown788.IncInVal() && mUnknown788.GetOutVal() == 0.0) {
		mUnknown6CC = false;
		mUnknown6F0 = SexyString();
	}
}

void Board::ShowReplayPod()
{
	mUnknown2628 = false;
	if (mGameMode == MODE_BLITZ || 6 <= mUnknown160)
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

void Board::SetUnknown3924(float theValue)
{
	float aWas = mUnknown3924;
	mUnknown3924 = theValue;
	if (aWas == 0.0f && 0.0f < theValue) {
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece != nullptr)
					aPiece->mUnknown535 = true;
			}
		}
		if (mGameMode == MODE_CLASSIC)
			NextLevelMusic();
	}
}

void Board::ApplyUnknown3924()
{
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece != nullptr)
				aPiece->mAlpha = mUnknown3924;
		}
	}
}

void Board::RetargetTravellers()
{
	// Each soul picks a plain gem of the new board, from a random cell on, that no other soul goes to; a fruit's
	// soul looks for one of its colour first
	RandSigned();
	for (unsigned i = 0; i < mUnknown26B0.size(); i++) {
		GemSoul* aSoul = mUnknown26B0[i];
		int aCells = mRows * mCols;
		int aCell = Rand85B970() % aCells;
		bool aFound = false;
		for (int aPass = 0; aPass < 2 && !aFound; aPass++) {
			for (int n = 0; n < aCells; n++) {
				Piece* aPiece = mGrid[aCell];
				if (aPiece != nullptr && aPiece->mFlags == 0) {
					bool aTaken = false;
					for (unsigned j = 0; j < mUnknown26B0.size(); j++) {
						if (mUnknown26B0[j]->mState == 2 && mUnknown26B0[j]->mUnknown680 == aPiece) {
							aTaken = true;
							break;
						}
					}
					if (!aTaken && (!(aSoul->mType & (1 << 0)) || aPiece->mColor == aSoul->mColor || aPass == 1)) {
						aSoul->SetTarget(aPiece);
						aSoul->mUnknown8F4 = true;
						aSoul->mUnknown660 = (float)((double)i * 0.05);
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
	UpdateUnknown1660();
	if (mUnknown149C.empty()) {
		mUnknown21DA = true;
		mUnknown15C = 0;
	}
	// Each strike lights its gems up until they break, and every tenth of a second its lightning spreads a cell
	// further along its row and column (a power electro gem's along three of each); the strikes after the n-th one
	// wait while it is less than 25 * n updates old
	int aWait = 0;
	int i = 0;
	if (0 < (int)mUnknown149C.size()) {
		do {
			ElectroStrike* aStrike = static_cast<ElectroStrike*>(mUnknown149C[i]);
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
						if (aPiece != nullptr && !aPiece->mUnknown51D) {
							float aGlow = (float)((double)aPiece->mGlow + 0.014999999664723873);
							aPiece->mGlow = aGlow;
							if (!(1.0f <= aGlow)) {
								aAllDone = false;
							} else {
								aPiece->mUnknown88 = 1;
								aPiece->mUnknown2EC = aStrike->mPointsId;
								if (!aPiece->IsFlagSet(Piece::FLAG_COAL)) {
									aPiece->mUnknown2F0 = -0x65;
								} else {
									aPiece->mUnknown2F0 = -0x67;
									BreakSpecial(aPiece, 2);
									if (!aPiece->mBreaking) {
										AddStat(0x4a, 1);
										AddStat(0x3e, 1);
										mUnknown1F8++;
										mUnknown1FC++;
										aPiece->mUnknown537 = true;
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
									aY < GetRowY(mRows) && aPiece != nullptr && !aPiece->mUnknown51D) {
									// a gem a strike has lit already is left to it
									bool aTaken = false;
									unsigned int k = 0;
									if (mUnknown149C.size() != 0) {
										do {
											std::vector<int>& anIds =
												static_cast<ElectroStrike*>(mUnknown149C[k])->mIds;
											unsigned int m = 0;
											if (anIds.size() != 0) {
												do {
													if (aPiece->mId == anIds[m])
														aTaken = true;
													m++;
												} while (m < anIds.size());
											}
											k++;
										} while (k < mUnknown149C.size());
									}
									if (!aTaken) {
										aAllDone = false;
										if (aPiece->mGlow == 0.0f && !aPiece->mUnknown51D) {
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
							aGem->mUnknown51D = false;
							aGem->ClearSpecial();
							aGem->mUnknown88 = 1;
						}
						aStrike->mFade = (float)((double)aStrike->mFade - 0.25);
						Piece** aCell = mGrid;
						for (int aCells = mCols * mRows; 0 < aCells; aCells--) {
							if (*aCell != nullptr)
								(*aCell)->mUnknown34 = 0;
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
						(aPiece->IsFlagSet(Piece::FLAG_BOMB) || aPiece->IsFlagSet(Piece::FLAG_1) ||
							aPiece->IsFlagSet(Piece::FLAG_LOCKED))) {
						aPiece->mUnknown88 = 1;
						break;
					}
					aCell++;
				}
				aRemove = true;
			}
			if (aRemove) {
				mUnknown149C.erase(mUnknown149C.begin() + i);
				delete aStrike;  // port: the original leaves the strike allocated
				i--;
			} else {
				if (aStrike->mCount < aWait)
					break;
				aWait += 25;
			}
			i++;
		} while (i < (int)mUnknown149C.size());
	}
}

void Board::GetCenter(int* theX, int* theY)
{
	if (theX != nullptr) {
		if (!mUnknown2041)
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
		mUnknown26C4 = 0;
	} else if (theState == 3) {
		mUnknown26C0 = 0;
		mUnknown20E4 = 0;
		mUnknown26C4 = 1.0f;
	} else if (theState == 4) {
		// blitz's FINAL SCORE and the score under it
		mUnknown3470 = 2.0f;
		mApp->PlaySample(SOUND_TALLY_ZOOMIN);
		mUnknown3464 = new TextImage(FONT_INCREDIBLE, mApp->GetString(GTS(0x7dc338), GT(0x7dc320)), 0, 0, -1, 0,
			nullptr);
		mUnknown3468 = new TextImage(FONT_INCREDIBLE, CommaSeperate(mUnknown168), 0, 0, -1, 0, nullptr);
		mUnknown3460 = 8.0f;
		mUnknown346C = 1.0f;
	}
	mUnknown26C8 = theState;
}

void Board::UpdateGameOver()
{
	if (mUnknown36B1) {
		// (the nuke's shake: a draw from Rand and a cosine nothing keeps)
		mUnknown1FFC++;
		if (0x46 < mUnknown1FFC)
			mUnknown2000++;
		if (mUnknown2000 < 1 || 199 < mUnknown2000)
			return;
		Rand();
		cosf((float)((double)mUnknown2000 * 0.031415926707925035));
		return;
	}
	mUnknown1514 = 0x32;
	FallPieces();
	switch (mUnknown26C8) {
	case 1:
		SetEndState(3);
		break;
	case 3: {
		// The rows fall off the board one by one, from the top
		float aDelay = (float)((double)mUnknown26C4 - 0.009999999776482582);
		mUnknown26C4 = aDelay;
		if (0.0f < aDelay)
			return;
		std::vector<Piece*> aRow;
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(mUnknown26C0, aCol);
			if (aPiece != nullptr)
				aRow.push_back(aPiece);
		}
		int aMidX;
		int aMidY;
		GetCenter(&aMidX, &aMidY);
		for (unsigned int i = 0; i < aRow.size(); i++) {
			Piece* aPiece = aRow[i];
			aPiece->mUnknown534 = true;
			float aRand = fabsf(RandSigned());
			aPiece->mUnknown34 = (float)((double)-aRand * (double)gGemHeight * 0.019999999552965164);
			aRand = fabsf(RandSigned());
			aPiece->mPendingDX = (float)(((double)aPiece->mX - (double)aMidX) * (double)aRand * 0.02500000037252903);
		}
		mUnknown26C0++;
		if (mUnknown26C0 < mRows + 5) {
			mUnknown26C4 = 0.2f;
		} else if (mGameMode == MODE_BLITZ) {
			SetEndState(4);
			unsigned int aTrack = (unsigned int)mUnknown168 / 40000;
			if (2 < aTrack)
				aTrack = 2;
			mApp->PlayMusic(301 + aTrack, true, true, 0, 0);  // 0x84ea68
		} else {
			SetEndState(9);
			mUnknown26C4 = 1.25f;
		}
		break;
	}
	case 4: {
		float aDelay = (float)((double)mUnknown3470 - 0.009999999776482582);
		mUnknown3470 = aDelay;
		if (!(0.0f < aDelay))
			mUnknown26C8 = 5;
		break;
	}
	case 5: {
		float aZoom = (float)((double)mUnknown3460 - 0.10000000149011612);
		mUnknown3460 = aZoom;
		if (1.0f < aZoom)
			return;
		mApp->ShowHyperspace();
		mApp->mHyperspace->StartEndGame();
		mGoodTube->mVisible = false;
		mUnknown3924 = 0;
		mUnknown3460 = 8.0f;
		mApp->PlaySample(SOUND_TALLY_ZOOMIN);
		mUnknown26C8++;
		break;
	}
	case 6: {
		float aZoom = (float)((double)mUnknown3460 - 0.10000000149011612);
		mUnknown3460 = aZoom;
		if (1.0f < aZoom)
			return;
		mUnknown3460 = 1.0f;
		if (mApp->mProfile->mRank < (int)mApp->mRanks->mRanks.size()) {
			// The rank's bonus: the score to the power 0.6, a hundredth of it
			float aScore = (float)(unsigned int)mUnknown168;
			int aBonus = Ftol((double)(float)std::pow((double)aScore, (double)0.6f) / 100.0);  // 0x670a49
			mUnknown269C = aBonus;
			if (aBonus != 0) {
				ShowRankBonus(aBonus);
			}
		}
		mUnknown26C4 = 2.0f;
		mUnknown26C8++;
		break;
	}
	case 7:
		if (0.0f < mUnknown26C4)
			mUnknown26C4 = (float)((double)mUnknown26C4 - 0.009999999776482582);
		else
			mUnknown26C8++;
		break;
	case 8: {
		float aFade = (float)((double)mUnknown346C - 0.019999999552965164);
		mUnknown346C = aFade;
		bool aDone = false;
		if (!(0.0f < aFade)) {
			mUnknown346C = 0;
			aDone = true;
		}
		if (mUnknown26A0 == nullptr || mUnknown26A0->mClicked) {
			// (the original removes and deletes it when there is none too)
			if (mUnknown26A0 != nullptr)
				mApp->mWidgetManager->RemoveWidget(mUnknown26A0);
			mApp->SafeDeleteWidget(mUnknown26A0);
			mUnknown26A0 = nullptr;
		}
		if (aDone && mUnknown26A0 == nullptr)
			SetEndState(9);
		break;
	}
	case 9: {
		float aDelay = (float)((double)mUnknown26C4 - 0.009999999776482582);
		mUnknown26C4 = aDelay;
		if (!(0.0f < aDelay) && mUnknown2208 == nullptr && mUnknown23DC.empty()) {
			mUnknown26C8 = 0;
			if (mChallenge == 0)
				mUnknown1FF5 = true;
			else
				mUnknown34EC = 1;
		}
		break;
	}
	}
}

void Board::UpdateStats()
{
	if (mUnknown137C)
		return;
	unsigned int aTime = (unsigned int)(mUpdateCnt - mUnknown14DC) / 100 + (unsigned int)mUnknown14E8;
	mUnknown14EC = (int)aTime;
	if ((unsigned int)mGameMode <= 2) {
		// the app's longest game of the mode
		unsigned int& aBest = (unsigned int&)mApp->mUnknownC74[5 + mGameMode];
		if (aTime < aBest)
			aTime = aBest;
		aBest = aTime;
	}
	Profile* aProfile = mApp->mProfile;
	aProfile->mUnknown78 += mUnknown14EC - mUnknown14F0;
	mUnknown14F0 = mUnknown14EC;
	if (mGameMode != MODE_PUZZLE) {
		aProfile->mUnknown94 += mUnknown1530 - mUnknown1534;
		if (9 < aProfile->mUnknown94)
			aProfile->mUnknown164 |= 1 << 1;  // 0x733774
		mUnknown1534 = mUnknown1530;
		aProfile->mUnknown98 += mUnknown1538 - mUnknown153C;
		if (4 < aProfile->mUnknown98)
			aProfile->mUnknown164 |= 1 << 2;
		mUnknown153C = mUnknown1538;
		aProfile->mUnknown8C += mUnknown1E8 - mUnknown1500;
		if (mGameMode == MODE_CLASSIC || mGameMode == MODE_BLITZ || mGameMode == MODE_PUZZLE) {
			unsigned int aScore = (unsigned int)mUnknown168;
			if (aScore <= (unsigned int)aProfile->mUnknown5C[mGameMode])
				aScore = (unsigned int)aProfile->mUnknown5C[mGameMode];
			aProfile->mUnknown5C[mGameMode] = (int)aScore;
		} else if (mGameMode == MODE_ZEN) {
			aProfile->mUnknown5C[MODE_ZEN] = mUnknown168;
		}
		mUnknown1500 = mUnknown1E8;
		int aLevel = mUnknown158;
		if (aLevel <= aProfile->mUnknown128[mGameMode])
			aLevel = aProfile->mUnknown128[mGameMode];
		aProfile->mUnknown128[mGameMode] = aLevel;
	}
	if (!mUnknown137E) {
		unsigned int aScore = (unsigned int)aProfile->mUnknown74;
		if (aScore <= (unsigned int)mUnknown168)
			aScore = (unsigned int)mUnknown168;
		aProfile->mUnknown74 = (int)aScore;
	}
	int aSeconds = mUnknown1470 / 100;
	if (mUnknown137D)
		mApp->mUnknown7D8 += aSeconds;
	else if (mGameMode == MODE_PUZZLE)
		mApp->mUnknown7DC += aSeconds;
	else if (mUnknown137E)
		mApp->mUnknown7E0 += aSeconds;
	else
		mApp->mUnknown7D4 += aSeconds;
	mUnknown137C = true;
}

bool Board::UpdateNuke()
{
	if (mUnknown27B8.CheckInThreshold(0.7900000214576721) && mChallenge != 0 &&
		(mChallenge->mFlags & Challenge::FLAG_NIGHTMODE) != 0) {
		// a night challenge ends here, met
		ShowChallengeComplete(true);
		return true;
	}
	bool aDone = !mUnknown27B8.IncInVal();
	if (mUnknown26CC != nullptr) {
		// The gems within the blast's reach blow apart, eight bits each
		mUnknown26CC->Update();
		double aGrow = mUnknown26D8.GetOutVal();
		float aReach = (float)(aGrow * (double)(float)((double)gApp->mScale * 280.0));
		float aMidX = (float)((double)(gGemWidth / 2) + (double)mUnknown26CC->mX);
		float aMidY = (float)((double)(gGemHeight / 2) + (double)mUnknown26CC->mY);
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece == nullptr || aPiece == mUnknown26CC)
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
					anEffect->mUnknown38 = (float)((double)RandSigned() * 0.15 + 0.8);
					float aDir = (float)atan2((double)aDY, (double)aDX);  // 0x4c1b70
					float aCos = cosf(aDir);
					anEffect->mDX = (float)((double)RandSigned() * 4.0 + (double)aCos * 16.0);
					float aSin = sinf(aDir);
					anEffect->mDY = (float)((double)RandSigned() * 4.0 + (double)aSin * 16.0);
					anEffect->mGravity = 0.05f;
					float aTurn = (float)((double)RandSigned() * (double)6.2831854820251465f);
					anEffect->mUnknown9C = aTurn;
					anEffect->mUnknownA0 = (float)((double)6.2831854820251465f * 0.25 + (double)aTurn);
					anEffect->mUnknownA4 = 0.0f < RandSigned() ? 0.0f : 1.0f;
					anEffect->mUnknownA8 = (float)(((double)fabsf(RandSigned()) * 3.0 + 1.0) * 0.04500000178813934);
					anEffect->mDAlpha = (float)(((double)fabsf(RandSigned()) * 4.0 + 2.0) * -0.005);
					mFXManager->AddEffect(anEffect);
					i++;
				} while (i < 8);
			}
		}
		if (!(mUnknown27B8.GetInVal() < 0.5799999833106995) && mUnknown27B8.GetInVal() < 1.649999976158142) {
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
				double aSlide = mUnknown2978.GetOutVal();
				float aSlideX = (float)((double)gApp->mScale * -60.0);
				aSteam->mX = (float)(aSlide * (double)aSlideX + (double)(mGoodTube->mX + anOffsetX));
				int anOffsetY = Ftol((double)gApp->mScale * 200.0);
				aSlide = mUnknown2978.GetOutVal();
				float aSlideY = (float)((double)gApp->mScale * 8.0);
				aSteam->mY = (float)(aSlide * (double)aSlideY + (double)(mGoodTube->mY + anOffsetY));
				anAngle = (double)RandSigned() * 0.20000000298023224 - 1.2000000476837158;
			} else {
				int anOffsetX = Ftol((double)gApp->mScale * 100.0);
				double aSlide = mUnknown2978.GetOutVal();
				float aSlideX = (float)((double)gApp->mScale * -20.0);
				aSteam->mX = (float)(aSlide * (double)aSlideX + (double)(mGoodTube->mX + anOffsetX));
				int anOffsetY = Ftol((double)gApp->mScale * 300.0);
				aSlide = mUnknown2978.GetOutVal();
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
		if (mUnknown27B8.CheckInThreshold(1.649999976158142)) {
			// The blast is over: the gem is plain again and space comes back
			mUnknown26CC->mFlags = 0;
			mUnknown26CC->mUnknown2F6 = true;
			mFXManager->Clear();
			if (mChallenge != 0) {
				mApp->mChallengeInfo->mResult = 2;  // 0x6d36e8: failed
				mUnknown34EC = 1;
			} else {
				mApp->ShowHyperspace();
				mApp->mHyperspace->mLevel = 0;
			}
			mApp->mVizManager->SetVisible(false);
			mUnknown2043 = false;
			mUnknown2041 = false;
			mGoodTube->SetVisible(false);
		}
	}
	if (mUnknown26A0 != nullptr)
		aDone = false;
	if (aDone) {
		UpdateStats();
		mUnknown2041 = false;
		mUnknown15FC = true;
		mUnknown1FF5 = true;
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
	mUnknown34EC = 1;
	mApp->ShowHyperspace();
	mVisible = false;
	mApp->mHyperspace->StartEndGame();
	mApp->mHyperspace->mUnknown12B0.SetCurve(GTS(0x7dc510));
	aMenu->SetState(5, false);
	aMenu->mAwardTelly = aTelly;
	mApp->mHyperspace->mFlag159 = false;
}

void Board::Pause(bool theShow)
{
	if (mUnknown1570 == 0) {
		mUnknown14E8 += (int)((unsigned int)(mUpdateCnt - mUnknown14DC) / 100);
		MarkDirty();
		mApp->mVizManager->mPaused = true;
		mGoodTube->mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_ALLOW_MOUSE;
		if (mApp->mTwistMusic != nullptr && (mApp->mTwistMusic->mCurTrack == 300 || mApp->mTwistMusic->mCurTrack == 308))
			mApp->mTwistMusic->PauseAllMusic();
		if (mGameMode == MODE_BLITZ)
			mFXManager->Clear();
	}
	if (theShow) {
		if (mUnknown1574 == 0)
			MarkDirty();
		mUnknown1574++;
	}
	mUnknown1570++;
	// 0x6a634f
	mUnknown25B4 = 0;
	mUnknown25AC = mUnknown25A8;
	// The original says "Pause(): mPauseCount=%d" with OutputDebugString
}

void Board::Unpause(bool theShow)
{
	if (theShow && 0 < mUnknown1574) {
		mUnknown1574--;
		if (mUnknown1574 < 1)
			MarkDirty();
	}
	if (0 < mUnknown1570) {
		mUnknown1570--;
		if (mUnknown1570 == 0) {
			if (mApp->mTwistMusic != nullptr &&
				(mApp->mTwistMusic->mCurTrack == 300 || mApp->mTwistMusic->mCurTrack == 308))
				mApp->mTwistMusic->ResumeAllMusic();
			mUnknown14DC = mUpdateCnt;
			mUnknown15D4 = 1;
			if (mUnknown160 != 7)
				mApp->mVizManager->mPaused = false;
		}
	}
	// The original says "Unpause(): mPauseCount=%d" with OutputDebugString
}

void Board::SpinWheel(Piece* thePiece)
{
	if (mUnknown265C != 0) {
		// a fruit dance moves on to its end
		mUnknown265C = 5;
		mGoodTube->mUnknown5F0.SetCurve(GTS(0x7dc558));
		// port: the tube's effects manager (+0x28c) is cleared, once it is ported
	}
	mUnknown20D0 = false;
	if (mApp->mUnknownC74[53] == 0)  // app +0xd48: the level of the first wheel
		mApp->mUnknownC74[53] = mUnknown158;
	if (mReplayPod != nullptr) {
		mReplayPod->mState = ReplayWidget::STATE_GOING;
		mReplayPod->mPopped = false;
	}
	mApp->PlaySample(SOUND_SPINSEQUENCER1);
	if (((mUnknown3914 >> 11) & 1) == 0) {
		QueueTip(11, thePiece);
		ShowNextTip();
	}
	mUnknown23DC.push_back(new WheelSpin(this, thePiece));
}
