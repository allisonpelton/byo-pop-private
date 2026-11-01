// Board's updates (Board.h): Update (0x6d6899), which runs the game's updates at its speed, and UpdateGame
// (0x6d090f), one update of the game.
#include "Board.h"

#include "GameText.h"
#include "GoodTube.h"
#include "GraphicsRecorder.h"
#include "Piece.h"
#include "SoundInstance.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "WidgetManager.h"
#include "X86Math.h"

#include <cmath>

using namespace Sexy;

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
					// port: 0x6c2f69, the new level's intro, draws from Rand through the tutorial
					NotPorted("Board::UpdateGame: the new level's intro (0x6c2f69)");
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
	// port: the rest of 0x6d090f comes next
	NotPorted("Board::UpdateGame: the rest");
}
