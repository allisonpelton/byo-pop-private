// Board's updates (Board.h): Update (0x6d6899), which runs the game's updates at its speed, and UpdateGame
// (0x6d090f), one update of the game.
#include "Board.h"

#include "GameText.h"
#include "Compat2004.h"
#include "CustomBassMusicInterface.h"
#include "EffectsManager.h"
#include "GoodTube.h"
#include "Hyperspace.h"
#include "GraphicsRecorder.h"
#include "MusicInterface.h"
#include "Piece.h"
#include "Profile.h"
#include "Res.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "TwistUtil.h"
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
	// port: the PointsManager is not ported yet (the original always has one)
	if (mPointsManager != nullptr)
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
		// port: the object at +0x267c (with a flag at +0x170) slows the game down and dims the VizManager over
		// 2200 updates; neither is ported yet
		NotPorted("Board::Update: the +0x267c object");
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
			// port: 0x6ab551 comes with the replays
			NotPorted("Board::Update: 0x6ab551");
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
		// port: the sound at app +0x938 fades out first, then 0x6ba73a(+0x1f84) with the replays' sounds
		NotPorted("Board::Update: 0x6ba73a");
		mUnknown1F84 = -1;
	}
	// port: the dialog at +0x2584 is not ported; while there is one, its +0x174 is taken as set
	if (mUnknown2584 != nullptr)
		NotPorted("Board::Update: the +0x2584 dialog");
	if (mUnknown2584 == nullptr) {
		if (0 < mUnknown257C) {
			mUnknown257C--;
			if (mUnknown257C == 0)
				NotPorted("Board::Update: 0x6a615b");
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
	if (mUnknown267C != nullptr)
		NotPorted("Board::Update: the +0x267c object's music");
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

void Board::MoveCursor(int theRow, int theCol, bool theQuiet, void* theUnknown)
{
	// port: 0x6bd9ac and the moves it looks for (0x6bd872) come with the matching; its trail draws from Rand
	NotPorted("Board::MoveCursor");
	SetCursor(theRow, theCol);
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
	if (mGameMode == MODE_CHALLENGE) {
		// port: 0x6b66d2 comes with the challenges
		NotPorted("Board::StartUnknown34F0: 0x6b66d2");
	}
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
			// A counter is at zero: in a challenge, blitz or for a devil gem the game ends (0x6b2c50), else the bomb
			// goes off (0x6bd0d9)
			// port: both come with the end of the game
			NotPorted("Board::CountDownCounters: a counter at zero");
			if (!mUnknown37C4)
				return true;
			if (mGameMode == MODE_CHALLENGE || mGameMode == MODE_BLITZ || aLowest->IsFlagSet(Piece::FLAG_DEVIL))
				mUnknown3920 = true;
			else
				aLowest->mUnknown88 = 1;
		}
	}
	return false;
}

void Board::UpdateGame()
{
	mUnknown25A4 = false;
	mUnknown20D0 = false;
	if (mUnknown34EC != 0) {
		if (mUnknown34EC == 1) {
			// port: the game is over and space comes back (ShowHyperspace, 0x714883 and the menu's 0x6e2318)
			NotPorted("Board::UpdateGame: the end of the game");
		}
		return;
	}
	if (mUnknown26A0 != nullptr && mUnknown26A0->mClicked) {
		mApp->mWidgetManager->RemoveWidget(mUnknown26A0);
		mApp->SafeDeleteWidget(mUnknown26A0);
		mUnknown26A0 = nullptr;
		if (mUnknown2628)
			NotPorted("Board::UpdateGame: 0x6ab48d");
	}
	if (mUnknown26AC == nullptr || !mUnknown26AC->mClicked) {
		if (0.0f < mUnknown380C) {
			float aLeft = (float)((double)mUnknown380C - (double)0.01f);
			mUnknown380C = aLeft;
			// port: the RankBonusWidget (mUnknown26A0) lets the level start in its states 2 and 5; it is not ported,
			// so while there is one the level waits
			if (mUnknown26A0 != nullptr)
				NotPorted("Board::UpdateGame: the RankBonusWidget's state");
			if (mUnknown2208 == nullptr && mUnknown1474 == nullptr && mUnknown26A0 == nullptr) {
				if (aLeft <= 0.0f) {
					// The trial's limit (the copy is always registered) is left out
					// port: the VizManager's next look (0x757ff5) once it is ported
					mUnknown158++;
					NewLevel();
					mUnknown3810.SetCurve(GTS(0x7df2f0));
					mGoodTube->SetLevel(mUnknown158);
					StartUnknown3880();
					mUnknown380C = 0;
					mUnknown37F4 = 1.5f;
					StartLevelIntro();
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
			// port: 0x6c78a5 and the tutorial's dialog 0x6bbc58 come with the tutorial
			NotPorted("Board::UpdateGame: the tutorial's dialog closing");
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
	if (!mUnknown1F98.empty()) {
		// port: the hints' widgets (+0x1f98) come with the tutorial
		NotPorted("Board::UpdateGame: the hints");
	}
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
				// port: the board's MouseMove (slot 52, 0x6a47ba) comes with the input
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
		// port: the 0x211c object (0x74c8c8) is not ported yet
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
				if ((mGameMode == MODE_ZEN && !mApp->mProfile->GetFlag1E4(0x11) && 179999 < mUnknown1470) ||
					(mGameMode == MODE_CLASSIC && !mApp->mProfile->GetFlag1E4(0x11) && mUnknown21E0 < 3)) {
					// port: 0x6a615b comes with the tutorial's tips
					NotPorted("Board::UpdateGame: 0x6a615b");
				} else {
					StartUnknown34F0();
					mApp->PlayVoice(SOUND_VOICE_GO, false);
				}
			}
		}
	} else {
		mUnknown37F4 = 0.1f;
		mUnknown25A4 = false;
		// port: the travellers (+0x26b0: fruit, angels, smoke and the like, 0x70188f) come with the features
		NotPorted("Board::UpdateGame: the travellers");
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
			// port: the jump between levels (+0x26a8's states, the travellers and 0x6c812f) comes with the jumps
			NotPorted("Board::UpdateGame: the jump between levels");
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
			// port: the challenge's timer (0x6bedd8) comes with the challenges
			NotPorted("Board::UpdateGame: the challenge's timer (0x6bedd8)");
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
							// port: the end of the game (0x6b2c50) comes with it
							NotPorted("Board::UpdateGame: blitz's end (0x6b2c50)");
						}
					} else if (!mUnknown21D8 && !mUnknown21DA && !mUnknown21D9 && mUnknown25A4) {
						mUnknown3920 = true;
					}
				} else {
					// port: blitz's last seconds (0x6bea4b) come with blitz
					NotPorted("Board::UpdateGame: blitz's last seconds (0x6bea4b)");
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
			mGoodTube->mWidgetFlagsMod.mAddFlags &= ~WIDGETFLAGS_ALLOW_MOUSE;
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
			// port: 0x6cf3fd, the matches after a move, comes with the matching; it draws from Rand
			NotPorted("Board::UpdateGame: the matches after a move (0x6cf3fd)");
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
			RandSigned();
			if (!mUnknown26B0.empty()) {
				// port: the travellers' new targets draw from Rand once per traveller (0x6a3447)
				NotPorted("Board::UpdateGame: the travellers' targets");
			}
		}
	}
	// The board's state: 0 play, 3 and 4 waits, 5 the level's start, 7 the level-up's blast, 8 the puzzle's fill
	bool aSettled = false;
	bool aFall = false;
	switch (mUnknown160) {
	case 0:
		if (mUnknown21D9) {
			// port: 0x6cfd85 comes with the hyper cascades; it draws from Rand
			NotPorted("Board::UpdateGame: 0x6cfd85");
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
						   mUnknown2630 != 0 && mGoodTube->IsIdle()) {
					mUnknown266A = true;
					// port: the challenge's goal met (0x6b330d and the profile's challenges) comes with the challenges
					NotPorted("Board::UpdateGame: the challenge's goal");
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
		// port: the level-up's blast (+0x27b8's curve, the gems blown off from +0x26cc, 0x6a3b4c) and 0x6d045b come
		// with the jumps; both draw from Rand
		NotPorted("Board::UpdateGame: the level-up's blast");
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
		// port: 0x6c87d2 clears the matched gems (their points, sparks and the cascades); it comes with the matching
		// and draws from Rand
		NotPorted("Board::UpdateGame: the matches (0x6c87d2)");
	}
	if (mUnknown14AC.empty())
		mUnknown3905 = false;
	if (!mUnknown14AC.empty()) {
		// port: the geodes that fly from broken coal (+0x14ac) come with the coal; they draw from Rand
		NotPorted("Board::UpdateGame: the geodes");
	}
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
	if (mUnknown265C == 2 || mUnknown265C == 5 || (mUnknown265C == 4 && mUnknown2584 == nullptr)) {
		// port: the fruit gem's dance (+0x265c: 2 the fruit flies to the tube, 4 the dance, 5 its end; 0x6bbfe4 picks
		// the bonus colours) comes with the fruit gem; it draws from Rand
		NotPorted("Board::UpdateGame: the fruit dance");
		if (mUnknown265C == 4)
			mUnknown25A4 = false;
	}
	if (mUnknown160 == 2) {
		if (mUpdateCnt % 2 == 0) {
			// port: state 2 puts the gems of +0x148c back on the board one by one, picked by Rand, each with a glow
			// (IMAGE_INGAMEUI_GRIDSQUAREGLOW); it comes with the puzzles' undo
			NotPorted("Board::UpdateGame: state 2");
		}
	} else if (mUnknown160 == 1) {
		// port: state 1 plays the recorded board back (gGraphicsRecorder's frames, +0x3448 counting down, the
		// generator at +0x2a80 seeded from the update count); it comes with the recorder's frames
		mUnknown25A4 = false;
		NotPorted("Board::UpdateGame: state 1, the playback");
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
		if (mUnknown2584 != nullptr) {
			// port: the tutorial's dialog holds the hints back while its +0x174 is set; it comes with the tutorial
			NotPorted("Board::UpdateGame: the tutorial's +0x174");
		}
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
			if (mGameMode == MODE_CHALLENGE && mUnknown2630 != 0) {
				// port: a timed challenge (its +0x80 flag 2) runs out through 0x6b3432; it comes with the challenges
				NotPorted("Board::UpdateGame: the challenge's time");
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
	if (0 < mUnknown1F94) {
		int aNext = mUnknown1F94 - 1;
		mUnknown1F94 = aNext < 0 ? 0 : aNext;
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
			ShowStatsBar();
		if (mUnknown2629) {
			mWidgetFlagsMod.mAddFlags &= ~WIDGETFLAGS_ALLOW_MOUSE;
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
			// port: -autoplay's next move (0x6c14fc finds one, else Rand picks a cell) comes with the move search
			NotPorted("Board::UpdateGame: -autoplay's move");
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
		// port: the tutorial's dialog clears +0x34 unless its state (+0x16c) is 0, 2 or 12; it comes with the tutorial
		NotPorted("Board::UpdateCell: the tutorial's state");
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

bool Board::FillInBlanks(bool theUnknown)
{
	// port: 0x6c42e3 fills the gaps with new gems (0x6c25a4, which draws from Rand) and saves the game; it comes with
	// the matching
	NotPorted("Board::FillInBlanks");
	return false;
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
	// port: the tube's layout (0x6b3da8) comes with the tube; it draws nothing from Rand
	NotPorted("Board::LayoutTube");
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
				StartLevelIntro();
				// The original writes "LEVEL++" to the debugger here
				mGoodTube->SetLevel(mUnknown158);
				if (mGameMode == MODE_BLITZ) {
					if (mApp->mMusicInterface != nullptr)
						mApp->mMusicInterface->FadeOutAll(true, 0.004);
				} else if (mGameMode == MODE_ZEN) {
					NextLevelMusic();
				}
				mUnknown1390 = true;
				if (!mUnknown21A2 && GetUnknown23F5() && mUnknown2584 != nullptr) {
					// port: 0x7540ad, the tutorial's next board, comes with the tutorial; it draws from Rand
					NotPorted("Board::UpdateLevelUp: the tutorial (0x7540ad)");
				}
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
	// port: 0x6b3634, the level's end and the jump into space (+0x2108, the hyperspace's 0x7181b3), comes with the
	// jumps; it draws nothing from Rand itself
	NotPorted("Board::LevelComplete (0x6b3634)");
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
	if (!mUnknown1660.empty()) {
		// port: the objects at +0x1660 (they shake the gems) are not ported yet
		NotPorted("Board::UpdateUnknown1660: the objects");
	}
	return !mUnknown1660.empty();
}

bool Board::UpdateUnknown2170()
{
	if (!mUnknown2170.empty()) {
		// port: the objects at +0x2170 are not ported yet
		NotPorted("Board::UpdateUnknown2170: the objects");
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
				// port: 0x6ba5b4 sends a traveller to a gem (Rand picks it) and the points it brings (0x70205f,
				// 0x6a43e8) come with the travellers
				NotPorted("Board::CheckSettled: a traveller (0x6ba5b4)");
			}
		} else {
			// port: 0x6c755e, the board's refill once the gems have gone, comes with the matching; it draws from Rand
			NotPorted("Board::CheckSettled: the refill (0x6c755e)");
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
							// port: a gem's countdown to going off (its sparks, points and the effects, 0x6c1857,
							// 0x6bf983, 0x6c0ccf) comes with the matching; it draws from Rand
							NotPorted("Board::SettlePieces: a gem going off");
							aAllDone = false;
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
			NotPorted("Board::SettlePieces: 0x6b3ffc");
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
		// port: 0x6cf3fd, the matches once the board has settled, comes with the matching; it draws from Rand
		NotPorted("Board::FallPieces: the matches (0x6cf3fd)");
	}
	bool aWasMoving = mUnknown21D8;
	mUnknown21D8 = !aStill;
	if (!aStill)
		mUnknown25A4 = false;
	else if (aWasMoving)
		gSettleCount++;
	MarkDirty();
}

void Board::StartLevelIntro()
{
	// port: 0x6c2f69, the new level's intro (the first move's search 0x6c14fc, the challenge's text and the counts of
	// locked gems), comes next; it draws from Rand through the tutorial
	NotPorted("Board::StartLevelIntro (0x6c2f69)");
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
	if (mUnknown248 != 0.0f) {
		// port: 0x6c4466 runs the fruit gem's flight (+0x248, its sparks and points); it comes with the fruit gem and
		// draws from Rand
		NotPorted("Board::UpdateUnknown248 (0x6c4466)");
	}
}

void Board::UpdateHint()
{
	if (mUnknown25C8 != 0) {
		// port: 0x6aff84, the hint's sparkles round the gems (+0x25c8 1 to 3), comes with the hints; it draws from Rand
		NotPorted("Board::UpdateHint (0x6aff84)");
	}
}

void Board::ShowHint()
{
	// port: 0x6c32cf finds a move (0x6c14fc) and shows it; it comes with the hints
	NotPorted("Board::ShowHint (0x6c32cf)");
}

void Board::UpdateToolTips()
{
	if (mUnknown25A4 || mUnknown6CC || mUnknown6D0 != 0) {
		// port: the tooltips over the tube and the special gems (0x6d5279 to 0x6d6695 of UpdateGame: their texts, the
		// wait in +0x6d0 and the curves at +0x718 and +0x788) come with the board's drawing; SOUND_PURR plays over the
		// cat
		NotPorted("Board::UpdateGame: the tooltips");
	}
}

void Board::ShowStatsBar()
{
	// port: 0x6ab48d puts up the bar of the level's stats (a 0x170-byte widget, 0x6a99fe) and runs it (0x6a9aeb);
	// it comes with the level's end
	NotPorted("Board::ShowStatsBar (0x6ab48d)");
}
