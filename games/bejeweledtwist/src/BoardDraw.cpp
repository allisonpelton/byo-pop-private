// Sexy::Board's drawing (Board.h): DrawAll, Draw and the board under the gems. Only the 2D paths are ported (as with
// Piece): with Direct3D the original clears the screen first, draws the board through a transform and a 3D back.
#include "Board.h"
#include "ElectroStrike.h"

#include "ColorCycle.h"
#include "DDImage.h"
#include "EffectsManager.h"
#include "FlyingText.h"
#include "GameOverShow.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "GraphicsRecorder.h"
#include "Hyperspace.h"
#include "Piece.h"
#include "Points.h"
#include "Res.h"
#include "SexyMatrix.h"
#include "Shockwave.h"
#include "GemSoul.h"
#include "Traveller.h"
#include "TwistUtil.h"
#include "GoodTube.h"
#include "TwistD3D.h"
#include "VizManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>

using namespace Sexy;

void Board::DrawAll(ModalFlags* theFlags, Graphics* g)
{
	// Moved or scaled
	bool aMoved = !(mShakeX == 0.0f && mShakeY == 0.0f && mBoardZoom == 1.0f && mBoardOffsetX == 0.0f &&
		mBoardOffsetY == 0.0f);
	Rect aSavedClip = g->mClipRect;  // 0x439450
	mTransform.LoadIdentity();       // 0x44f320
	if (aMoved) {
		if (gD3DInterface == nullptr)
			g->Translate(Ftol(mShakeX), Ftol(mShakeY));
		else {
			// With Direct3D the scale and the offsets go through the transform at +0x29e8
			int aHalfHeight = mHeight / 2;
			int aHalfWidth = mWidth / 2;
			float aMoveY = (float)((double)-aHalfHeight + (double)mShakeY + (double)mBoardOffsetY);
			float aMoveX = (float)((double)-aHalfWidth + (double)mShakeX + (double)mBoardOffsetX);
			mTransform.Translate(aMoveX, aMoveY);  // 0x44f8a0
			mTransform.Scale(mBoardZoom, mBoardZoom);  // 0x44f990
			mTransform.Translate((float)(mWidth / 2), (float)(mHeight / 2));
			gD3DInterface->PushTransform(mTransform, true);  // slot 0x14
			g->mClipRect.mY = 1;  // +0x24
		}
	}
	if (mBoardImage != 0) {
		// An image of the whole board instead (drawn at 0, 0, also in 2D). Never reached: only the constructor
		// (0x6c5d1f) writes +0x15dc, a 0
		NotPorted("Board::DrawAll: the board's image (+0x15dc)");
		return;
	}
	if (mPlayState == 7 && gD3DInterface != nullptr)
		gD3DInterface->ClearDepth();  // slot 0x124
	if (mNukeNovaScale.GetOutVal() != 0.0 && mBoardShown && gD3DInterface != nullptr) {
		// With Direct3D: the nuke's flash over the board, IMAGE_BOOM_NUKE, while +0x26cc's curve runs
		gD3DInterface->SetDepthWriteMode(1);  // slot 0x118
		gD3DInterface->SetAlphaRef(0);        // slot 0x11c
		Transform aTransform;
		float aScaleY = (float)(mNukeNovaScale.GetOutVal() * 0.9300000071525574);
		float aScaleX = (float)(mNukeNovaScale.GetOutVal() * 0.9300000071525574);
		aTransform.Scale(aScaleX, aScaleY);
		g->SetColor(Color::White);
		float aY = (float)((double)(gGemHeight / 2) + (double)mEndingPiece->mY);
		float aX = (float)((double)(gGemWidth / 2) + (double)mEndingPiece->mX);
		g->DrawImageTransform(IMAGE_BOOM_NUKE, aTransform, aX, aY);  // 0x422000
		gD3DInterface->SetDepthWriteMode(0);
		gD3DInterface->SetDepthTestMode(2);  // slot 0x114
	}
	if (mHyperspace != nullptr && mHyperspace->mTransition == 11) {
		// A challenge's start: the jump's tunnel under the board (in 2D the level's picture while the VizManager is
		// hidden; with Direct3D the depth buffer is cleared first)
		if (gD3DInterface != nullptr)
			gD3DInterface->ClearDepth();
		mHyperspace->DrawTunnel(g);
	}
	TwistWidget::DrawAll(theFlags, g);
	if (aMoved) {
		if (gD3DInterface == nullptr)
			g->Translate(-Ftol(mShakeX), -Ftol(mShakeY));
		else {
			gD3DInterface->PopTransform();  // slot 0x18
			g->mClipRect = aSavedClip;
		}
	}
	if (mFillColor.mAlpha != 0) {
		g->SetColor(mFillColor);
		g->FillRect(0, 0, mWidth, mHeight);
	}
}

void Board::Draw(Graphics* g)
{
	bool is3D = mApp->Is3DAccelerated();
	DeferOverlay(99);
	if (!mApp->mVizManager->mVisible && mHyperspace == nullptr && !mApp->mStartClassicAtOnce && !mIsReplay &&
		mPlayState != 7 && gD3DInterface != nullptr) {
		// (the original has no null test: 2D never gets here) With Direct3D, outside the hyperspace and the menus: the screen is cleared to black first (slot 0x9c)
		gD3DInterface->Clear(Color::Black.mRed, Color::Black.mGreen, Color::Black.mBlue, Color::Black.mAlpha);
	}
	int aShakeX = Ftol((double)gSexyAppBase->mWidth * 0.0009765625 * (double)mShakeOffsetX);
	int aShakeY = Ftol((double)gSexyAppBase->mHeight / 768.0 * (double)mShakeOffsetY);
	g->Translate(aShakeX, aShakeY);
	// (the original asks for the board's middle here and drops it)
	if (!IsBusy()) {
		if (0.0f < mBlueFlash) {
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColor(Color(32, 32, 255, Ftol((double)mBlueFlash * 255.0)));
			g->FillRect(0, 0, mWidth, mHeight);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			mFXManager3->Draw(g);
		}
	}
	// (when busy, 0x6a4a0d only asks whether the app is 3D accelerated)
	if (!mBoardShown)
		return;
	if (!is3D)
		DrawBoardBack(g);
	else
		DrawBoardBack3D(g);
	if (!mBoardShown)
		return;
	if (mPauseCount != mPauseShownCount || mPauseShownCount == 0) {
		// The geodes (+0x14ac): eight cels each, additive and then normal, in eight colours
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		if (!mGeodes.empty()) {
			// 0x6cc7ed: each geode flying has its eight arms, a dark pass and a coloured one over it
			const Color aArmColors[8] = {Color(255, 128, 64), Color(128, 255, 64), Color(128, 64, 255),
				Color(255, 64, 255), Color(255, 128, 64), Color(64, 255, 128), Color(64, 128, 255), Color(255, 64, 255)};
			for (unsigned int i = 0; i < mGeodes.size(); i++) {
				Geode* aGeode = mGeodes[i];
				if (0 < aGeode->mDelay)
					continue;
				Image* anImage = IMAGE_PHOTONARMS;
				int aHalfWidth = anImage->GetCelWidth() / 2;
				int aHalfHeight = anImage->GetCelHeight() * 0x26 / 0xb7;
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
				for (int j = 0; j < 8; j++) {
					Rect aSrc = anImage->GetCelRect(j / 4 + 2);
					Color aColor(0, 0, 0);
					aColor.mAlpha = Ftol(aGeode->mBrightness.GetOutVal() * 255.0);
					if (aColor.mAlpha != 0) {
						g->SetColor(aColor);
						g->DrawImageRotated(anImage, Ftol((double)aGeode->mX - (double)aHalfWidth),
							Ftol((double)aGeode->mY - (double)aHalfHeight), (double)aGeode->mSpin[j], aHalfWidth,
							aHalfHeight, &aSrc);
					}
				}
				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				for (int j = 0; j < 8; j++) {
					Rect aSrc = anImage->GetCelRect(j / 4);
					Color aColor = aArmColors[j];
					aColor.mAlpha = Ftol(aGeode->mBrightness.GetOutVal() * 255.0);
					if (aColor.mAlpha != 0) {
						g->SetColor(aColor);
						g->DrawImageRotated(anImage, Ftol((double)aGeode->mX - (double)aHalfWidth),
							Ftol((double)aGeode->mY - (double)aHalfHeight), (double)aGeode->mSpin[j], aHalfWidth,
							aHalfHeight, &aSrc);
					}
				}
			}
		}
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColor(Color::White);
		DrawStrikes(g);
	}
	// (the board's middle again, unused, and FONT_DEBUG's scaled 654)
	g->SetFont(FONT_DEBUG);
	if (mStartState == 1 && !IsBusy() && mApp->mDialogMap.size() == 0 && !mDebugColorOff) {
		// A colour cycling through the hues, set and then replaced before anything is drawn with it
		int aLight = Ftol(std::sin((double)mUpdateCnt * 0.03) * -40.0);
		g->SetColor(Color(mApp->HSLToRGB(Ftol((double)mUpdateCnt * 2.5) % 255, 200, 200 - aLight)));
	}
	if (0.0f < mWhiteFlash) {
		double anAlpha = (double)mWhiteFlash * 255.0;
		if (255.0 < anAlpha)
			anAlpha = 255.0;
		g->SetColor(Color(255, 255, 255, Ftol(anAlpha)));
		g->FillRect(0, 0, mWidth, mHeight);
	}
	if (mShowEditorLabels) {
		// The level editor's labels
		if (mEditorNoMatches) {
			g->SetColor(Color(255, 255, 0));
			g->DrawString(GT(0x7dd728), 18, 600);
		}
		if (mGameMode == MODE_PUZZLE) {
			g->SetColor(Color(100, 100, 255));
			g->DrawString(GT(0x7dd710), 18, 630);
		}
		if (mEditorGreenLabel) {
			g->SetColor(Color(0, 255, 0));
			g->DrawString(GT(0x7dd6fc), 18, 690);
		}
		if (mIsBlitzGame) {
			g->SetColor(Color(255, 100, 100));
			g->DrawString(GT(0x7dd6e4), 18, 720);
		}
	}
	if (mClearBacklogDue) {
		mApp->ClearUpdateBacklog(false);
		mClearBacklogDue = false;
	}
	if (mClearSparksDue) {
		mApp->ClearUpdateBacklog(false);
		mClearSparksDue = false;
	}
	// (with the app's debug flag at +0x4f4, which nothing sets: a readout of the moves and the board's checks in
	// FONT_DEBUG)
	g->Translate(-Ftol((double)gSexyAppBase->mWidth * 0.0009765625 * (double)mShakeOffsetX),
		-Ftol((double)gSexyAppBase->mHeight / 768.0 * (double)mShakeOffsetY));
	if (IsBusy())
		return;
	if (0.0f < mMinuteWarningAlpha) {
		// The minute mark of blitz's last seconds: the glow, bright over itself, then the front
		g->SetColorizeImages(true);
		int aMidX, aY;
		GetCenter(&aMidX, &aY);
		int aOffset = Ftol((double)gApp->mScale * 3.0);
		aY = Ftol((double)aOffset + (double)mMinuteWarningGlow->mHeight * 0.5);
		g->SetColor(Color(Ftol(255.0 * (double)mMinuteWarningRed), 0, 0, Ftol((double)mMinuteWarningAlpha * 255.0)));
		DrawImageScaledCentered(g, mMinuteWarningGlow, (float)aMidX, (float)aY, ALIGN_CENTER, 1.0f, 1.0f);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color(Ftol((double)mMinuteWarningRed * 255.0), 0, 0, Ftol((double)mMinuteWarningAlpha * 32.0)));
		float aLimit = (float)((double)mMinuteWarningRed + 1.0);
		float aScale = 1.0f;
		while (aScale < aLimit) {
			DrawImageScaledCentered(g, mMinuteWarningGlow, (float)aMidX, (float)aY, aScale, aScale);
			aScale = (float)((double)aScale + 0.019999999552965164);
		}
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColor(Color(255, 255, 255, Ftol((double)mMinuteWarningAlpha * 255.0)));
		g->DrawImage(mMinuteWarningFront, aMidX - mMinuteWarningFront->mWidth / 2, aY - mMinuteWarningFront->mHeight / 2);
	}
	if (mGameMode == MODE_CHALLENGE || mGameMode == MODE_ZEN)
		return;
	if (mTipSlide.GetOutVal() == 0.0 || mTipText.empty())
		return;
	// The tip, sliding in from the right
	g->SetColor(Color(255, 255, 255, Ftol(mTipSlide.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
	g->SetFont(FONT_TIP);
	int aX = Ftol((double)mWidth * 0.5);
	SexyString aTip = mTipText;
	Color aGlow = GetFontLayerColor(FONT_TIP, "GLOW");
	SetFontLayerColor(FONT_TIP, "GLOW", Color::Black);
	SetFontLayerColor(FONT_TIP, "MAIN", Color::White);
	g->WriteString(Upper(aTip), aX, Ftol((double)gApp->mScale * 50.0), -1, 0, true, 0, -1, -1);
	SetFontLayerColor(FONT_TIP, "GLOW", aGlow);
}

void Board::DrawBoardBack(Graphics* g)
{
	int anOffset = Ftol(mStartFade.GetOutVal() * (double)gApp->mScale);
	if (mIsReplay) {
		g->PushState();
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, Ftol((double)mReplayStaticFade * 255.0)));
		DrawImageScaledCentered(g, IMAGE_STATIC_SPACE, (float)(mApp->mWidth / 2), (float)(mApp->mHeight / 2), 3, 1.0f,
			1.0f);
		g->PopState();
	}
	// The original makes a board-sized image here (+0x15f4) and clears it whenever +0x15fc asks; nothing draws it
	// or draws into it, so the port leaves it out
	mOverlayRedrawDue = false;
	g->Translate(anOffset * 2, 0);
	if (!mBoardHidden && !mGameOverShown)
		DrawBoard(g, false, false);
	g->Translate(-anOffset, 0);
}

// 0x6cc762: Direct3D's board, slid by the same curve in the screen's own pixels
void Board::DrawBoardBack3D(Graphics* g)
{
	float aSlide = (float)mStartFade.GetOutVal();
	int anOffset = Ftol((double)gSexyAppBase->mWidth * 0.0009765625 * (double)aSlide);
	g->Translate(-anOffset, 0);
	if (!mBoardHidden) {
		g->Translate(anOffset * 2, 0);
		DrawBoard(g, false, false);
		g->Translate(-anOffset, 0);
	} else {
		DrawBoard(g, false, true);
	}
}

void Board::DrawStrikes(Graphics* g)
{
	mApp->Is3DAccelerated();
	for (int i = 0; i < (int)mElectroStrikes.size(); i++) {
		static_cast<ElectroStrike*>(mElectroStrikes[i])->Draw(g);
	}
}

void Board::DrawBoard(Graphics* g, bool theToImage, bool theSkipPieces)
{
	float anArtScale = (float)((double)mApp->mArtRes / 1200.0);
	bool is3D = mApp->Is3DAccelerated();
	// A copy of the graphics, additive and colourised, for the gems' light (the grid draws on g itself)
	Graphics aG(*g);
	aG.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	aG.SetColorizeImages(true);
	if (!theToImage && 0.0f < mBoardFade && 0.001f < mBoardGlowSize)
		DrawGrid(g, anArtScale);
	if (!theSkipPieces)
		DrawPieces(g, aG, is3D);
	if (mEditorMode) {
		g->SetFont(FONT_DEBUG);
		g->SetColor(Color(255, 255, 255));
		g->DrawString(GT(0x7dd660), 0, 30);
	}
}

// 0x6caa13's first part: the grid's squares, their glow under a gem that waits, the cursor and the bars above and
// below the grid
void Board::DrawGrid(Graphics* g, float theArtScale)
{
	g->SetColorizeImages(mBoardFade < 1.0f);
	// (the original takes the sine of the update count's place in a 150-update cycle here and drops it)
	Color aColor(255, 255, 255, Ftol((double)mBoardFade * 255.0));
	g->SetColor(aColor);
	int aRow = 0;
	if (0 < mRows) {
		do {
			int aY = GetRowY(aRow);
			int aCol = 0;
			if (0 < mCols) {
				do {
					int aX = GetColX(aCol);
					Cell& aCell = mCells[mCols * aRow + aCol];
					Piece* aPiece = GetPieceAt(aRow, aCol);
					g->DrawImage((aCol + aRow) & 1 ? IMAGE_INGAMEUI_GRIDSQUARE2 : IMAGE_INGAMEUI_GRIDSQUARE, aX, aY);
					if (!IsBusy() && aPiece != nullptr && (float)aY == aPiece->mY) {
						// The glow: the cell's own pulse, the cell's flash, and a locked gem's red
						float aGlow = 0;
						aColor = Color(255, 255, 255);
						if (aCell.mMarkGlow != 0.0f) {
							float aSin = std::sin((float)((double)aCell.mMarkTick * 0.07500000298023224 -
								1.5707963705062866));
							aGlow = (float)(((double)aSin + 1.0) * 0.15000000596046448 + 0.30000001192092896) *
								aCell.mMarkGlow;
						}
						if (aCell.mHintFlash != 0.0f) {
							float aT = aCell.mHintFlash;
							float aScale = 1.0f;
							float aNear = (1.0f - aT) * 4.0f <= aT * 4.0f ? 1.0f - aT : aT;
							if (aNear * 4.0f <= 1.0f)
								aScale = (1.0f - aT) * 4.0f <= aT * 4.0f ? (1.0f - aT) * 4.0f : aT * 4.0f;
							float anAngle = (float)((double)aT * 3.1415927410125732 * 2.0 * 3.0 - 1.5707963705062866);
							double aFlash = ((double)std::sin(anAngle) + 1.0) * 0.10000000149011612 + 0.2;
							if ((double)aGlow <= aFlash * (double)aScale)
								aGlow = (float)(aFlash * (double)aScale);
						}
						if (aPiece->mLockState == 1) {
							int aTick = aPiece->mLockTick;
							float anAngle = (float)((double)aTick * 0.11999999731779099);
							float aRed = 0;
							if (anAngle < 18.849556f) {
								float aSin = std::sin(anAngle);
								aRed = (float)((double)(0.0 <= (double)aSin ? std::sin(anAngle) : 0.0f) * 0.5);
							} else if (200 < aTick) {
								float aBeat = (float)((double)((aTick - 200) % 100) * 0.10000000149011612);
								if ((double)aBeat < 3.1415927410125732)
									aRed = (float)((double)std::sin(aBeat) * 0.25);
							}
							if (aGlow < aRed) {
								aGlow = aRed;
								aColor = Color(255, 40, 40);
							}
						}
						if (0.0f < aGlow && mCursorShown) {
							aColor.mAlpha = Ftol((double)aGlow * 255.0);
							g->SetColorizeImages(true);
							g->SetColor(aColor);
							g->DrawImage(IMAGE_INGAMEUI_GRIDSQUAREGLOW, aX, aY);
							g->SetColorizeImages(false);
						}
					}
					aCol++;
				} while (aCol < mCols);
			}
			aRow++;
		} while (aRow < mRows);
	}
	g->SetColorizeImages(false);
	if (0 <= mTipRow) {
		Image* aCursor = IMAGE_CURSOR_BWCURSOR;
		g->DrawImage(aCursor, GetColX(mTipCol) + gGemWidth - aCursor->mWidth / 2,
			GetRowY(mTipRow) + gGemHeight - aCursor->mHeight / 2);
	}
	// (with a debug flag nothing sets, the board takes the mouse's place from Windows each draw and moves to it)
	if (-1 < mApp->mAutoPlayMode && mShownDialog == nullptr)
		mCursorShown = true;
	if (mCursorShown && !mEditorMode && (mGameMode != MODE_PUZZLE || !mPuzzleSolved) &&
		(mTutorial == nullptr || !mTutorial->mMouseVisible))
		DrawSelector(g, false);
	// The bars above and below the grid, stretched to its width, each with its glow
	g->SetColorizeImages(true);
	int aHalfHeight = (gBoard->mRows * gGemHeight) / 2;
	Image* aBar = mGridBarTop;
	int aBarHeight = aBar->mHeight;
	float aHalfBar = (float)(aBarHeight / 2);
	int aTopY = Ftol((float)(GetRowY(0) + aHalfHeight) - GetGridMidY() - aHalfBar);
	int aBotY = Ftol((double)((float)(GetRowY(0) + aHalfHeight) + GetGridMidY() - aHalfBar) +
		(double)theArtScale * 10.0);
	int aX = GetColX(0) + ((gBoard->mCols * gGemWidth) / 2 - mBarWidth / 2) + 2;
	g->SetColor(Color(255, 255, 255, Ftol((double)mBoardFade * 255.0)));
	g->DrawImage(aBar, aX, aTopY, mBarWidth, aBarHeight);
	if (0.0f < mBarTopAlpha) {
		g->SetColor(Color(255, 255, 255, Ftol((double)mBarTopAlpha * 255.0 * (double)mBoardFade)));
		g->DrawImage(IMAGE_INGAMEUI_GRIDBAR_TOPGLOW, aX, aTopY, mBarWidth, aBarHeight);
	}
	aBar = mGridBarBot;
	g->SetColor(Color(255, 255, 255, Ftol((double)mBarBottomAlpha * 255.0 * (double)mBoardFade)));
	g->DrawImage(aBar, aX, aBotY, mBarWidth, aBarHeight);
	if (0.0f < mBarTopAlpha) {
		g->SetColor(Color(255, 255, 255, Ftol((double)mBarTopAlpha * 255.0 * (double)mBoardFade)));
		g->DrawImage(IMAGE_INGAMEUI_GRIDBAR_BOTGLOW, aX, aBotY, mBarWidth, aBarHeight);
	}
}

namespace {

// 0x6a5c27: the gems by +0x14, the lower first
int ComparePieceDepth(const void* theA, const void* theB)
{
	Piece* aA = *(Piece* const*)theA;
	Piece* aB = *(Piece* const*)theB;
	if (aA->mTurnAmount < aB->mTurnAmount)
		return -1;
	if (aB->mTurnAmount < aA->mTurnAmount)
		return 1;
	return 0;
}

// 0x84ea40: the light's slot for each tenth of a turn; 0x84e9f8: each slot's neighbour (column, row), slot 8 the gem
// itself
const int gTurnSlots[10] = {0, 4, 8, 2, 6, 3, 7, 8, 1, 5};
const int gSlotOffsets[9][2] = {{0, -1}, {-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, 0}};

}  // namespace

// 0x6caa13's gems: sorted by depth, drawn behind and in front, lit by the board's lights
void Board::DrawPieces(Graphics* g, Graphics& theLightG, bool is3D)
{
	(void)theLightG;
	int aCount = 0;
	(void)g->GetColorizeImages();
	g->SetColor(Color(255, 255, 255));
	Piece* aPieces[435];
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece != nullptr && !aPiece->mBlasted)
				aPieces[aCount++] = aPiece;
		}
	}
	CrtQsort(aPieces, (size_t)aCount, sizeof(Piece*), ComparePieceDepth);
	bool aDrawn = true;
	if (IsTimed() && (mPauseShownCount != 0 || mApp->mOptionsDialog != nullptr || mApp->mDialogList.size() != 0)) {
		// A timed game hides its gems while it is paused
		aDrawn = false;
	} else {
		for (int i = 0; i < aCount; i++) {
			if (aPieces[i]->mIsDrawn)
				aPieces[i]->Draw(g, false);
		}
		mFXManager2->Draw(g);
	}
	if (mGameMode != MODE_PUZZLE && mCursorShown && 0.0f < mCursorAlpha &&
		(mTutorial == nullptr || !mTutorial->mMouseVisible))
		DrawSelector(g, true);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	// The light each gem gets on its eight sides: first the glint running across the board
	for (int i = 0; i < aCount; i++) {
		Piece* aPiece = aPieces[i];
		for (int j = 0; j < 8; j++)
			aPiece->mShineSlots[j] = 0;
		aPiece->mShineGlow = 0;
		if (mGlintRunning) {
			float aPlace = (float)(aPiece->mRow + aPiece->mCol) / (float)(mCols + mRows);
			float aDist = std::fabs(mGlintPlace - aPlace);
			float aLight = (float)(1.0 - (double)aDist * 9.0);
			if (0.0f < aLight) {
				aPiece->mShineSlots[3] = (float)((double)aLight * 0.800000011920929 + (double)aPiece->mShineSlots[3]);
				double aSide = (double)aLight * 0.6000000238418579;
				aPiece->mShineSlots[7] = (float)((double)aPiece->mShineSlots[7] + aSide);
				aPiece->mShineGlow = (float)(aSide + (double)aPiece->mShineGlow);
			}
		}
	}
	for (unsigned int i = 0; i < mLights.size(); i++) {
		Light& aLight = mLights[i];
		float aX = aLight.mX;
		float aY = aLight.mY;
		if (aLight.mPieceId != -1) {
			Piece* aPiece = GetPieceById(aLight.mPieceId);
			if (aPiece == nullptr)
				continue;
			aX = aPiece->mX + aX;
			aY = aPiece->mY + aY;
		}
		AddLight(aX, aY, aLight.mSize, aLight.mFalloff, aLight.mStrength * aLight.mLevel, aLight.mPiece,
			false);
	}
	for (unsigned int i = 0; i < mGeodes.size(); i++) {
		// each geode flying lights the gems (0x6caa13)
		if (mGeodes[i]->mDelay == 0)
			AddLight(mGeodes[i]->mX, mGeodes[i]->mY, 5.5f, 320.0f, 0.75f, nullptr, false);
	}
	for (int i = 0; i < aCount; i++) {
		Piece* aPiece = aPieces[i];
		if (aPiece->mHasPulsingLight) {
			float aSin = std::sin((float)((double)aPiece->mGlow * 15.0));
			float aGlow = (float)((double)std::fabs(aSin) * 0.6000000238418579);
			aPiece->mShineGlow = aGlow;
			if (is3D)
				AddLight((float)(gGemWidth / 2) + aPiece->mX, (float)(gGemHeight / 2) + aPiece->mY, 15.0f, 10.0f,
					aGlow, aPiece, false);
		}
		if (aPiece->mIsShining && !aPiece->mDimmed) {
			// A turning gem's light, shared between the two slots its turn lies between, and a little of each to
			// the neighbour on that side
			int aTurn = Ftol(aPiece->mShinePos);
			int aNext = (aTurn + 1) % 10;
			float aFrac = (float)((double)aPiece->mShinePos - (double)aTurn);
			double aRest = 1.0 - (double)aFrac;
			float* aSlot = &aPiece->mShineSlots[gTurnSlots[aTurn]];
			*aSlot = (float)((double)(float)aPiece->mShineFade * aRest + (double)*aSlot);
			aSlot = &aPiece->mShineSlots[gTurnSlots[aNext]];
			*aSlot = (float)aPiece->mShineFade * aFrac + *aSlot;
			int k = 0;
			do {
				int aDir = gTurnSlots[k == 0 ? aTurn : aNext];
				float aShare = (float)((k == 0 ? aRest : (double)aFrac) * aPiece->mShineFade);
				if (aDir != 9) {
					int aCol = gSlotOffsets[aDir][0] + aPiece->mCol;
					int aRow = gSlotOffsets[aDir][1] + aPiece->mRow;
					if (0 <= aCol && aCol < mCols && 0 <= aRow && aRow < mRows) {
						Piece* aNear = GetPieceAt(aRow, aCol);
						if (aNear != nullptr) {
							float* aNearSlot = &aNear->mShineSlots[(aDir + 4) % 8];
							*aNearSlot = (float)((double)aShare * 0.30000001192092896 + (double)*aNearSlot);
						}
					}
				}
				k++;
			} while (k < 2);
		}
	}
	if (!mDroppers.empty()) {
		// The droppers' drawing (0x6f4982): never reached, only DropFlaggedGems makes droppers, for FLAG_DROPPER gems,
		// which no game has
		NotPorted("Board::DrawPieces: the droppers");
	}
	// While the cursor is on the board, the gems away from its four are dimmed
	bool aCursor = 0 <= mTipCol;
	if (aDrawn) {
		for (int i = 0; i < aCount; i++) {
			Piece* aPiece = aPieces[i];
			if (aCursor) {
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
				g->SetColor(Color(0, 0, 0, 128));
				aPiece->mDimmed = aPiece->mCol < mTipCol || mTipCol + 1 < aPiece->mCol ||
					aPiece->mRow < mTipRow || mTipRow + 1 < aPiece->mRow;
			} else {
				aPiece->mDimmed = false;
			}
			if (aPiece->mIsDrawn)
				aPiece->Draw(g, true);
		}
	}
	// The bombs' heat haze: the mask shader's pass into the first manager's height image and the rectangles over the
	// bombs' cells (+0x14cc) that its distortion drawing (0x6f7ab4) takes from here
	mHazeRects.clear();  // 0x6b1746
	{
		float aScaleOff = (float)((double)mBoardZoom - 1.0);
		float aHazeFactor = 1.0f;
		if (mHyperspace != nullptr)
			aHazeFactor = 0.0f;
		aHazeFactor = (float)((1.0 - (std::fabs((double)aScaleOff) + std::fabs((double)aScaleOff))) * (double)aHazeFactor);
		bool aHaze = !mIsReplay && mBoardFade != 0.0f;
		// port: the original gathers the rectangles in 2D too, where nothing reads them
		if (gD3DInterface != nullptr && !mBombList.empty() && aHaze && 0.0f < aHazeFactor) {
			DrawBombHaze(g, aHazeFactor);
			GatherHazeRects();
		}
	}
	mBombList.clear();
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	// The hyperspace's +0x158 (set only with Direct3D) hides the souls
	if (mHyperspace == nullptr || !mHyperspace->mDrawsTravellers) {
		for (GemSoul* aSoul : mLevelSouls)
			aSoul->Draw(g);
	}
	for (Shockwave* aWave : mShockwaves)
		aWave->Draw(g);
	if (IsTipShowing())
		DrawTutorialCursor(g);
	if (mGameMode == MODE_PUZZLE && mCursorShown && 0.0f < mCursorAlpha && !mPuzzleSolved)
		DrawSelector(g, true);
	if (mExtraWidget != nullptr)
		mExtraWidget->Draw(g);
}

// 0x6a333f (with 0x684796): a vertex of FVF 0x2c4, pre-transformed, diffuse as given, no specular, two texture sets
struct HazeVertex {
	float x, y, z, rhw;
	unsigned int mDiffuse, mSpecular;
	float u0, v0, u1, v1;
};

// part of DrawBombHaze (0x6caa13): one vertex of the haze, whose fields the original sets in place
static HazeVertex MakeHazeVertex(float theX, float theY, unsigned int theColor, float theU0, float theV0, float theU1,
	float theV1)
{
	HazeVertex aVertex;
	aVertex.x = theX;
	aVertex.mSpecular = 0;
	aVertex.y = theY;
	aVertex.mDiffuse = theColor;
	aVertex.u0 = theU0;
	aVertex.v0 = theV0;
	aVertex.z = 0;
	aVertex.rhw = 1.0f;
	aVertex.u1 = theU1;
	aVertex.v1 = theV1;
	return aVertex;
}

// 0x6caa13's mask pass (the part under the Direct3D test): each bomb draws four fans of five vertices (IMAGE_HEAT_DISTORTION
// on stage 0, IMAGE_SHADER_TEST on stage 1) through the mask shader into the first manager's height image, in the
// screen's size scaled to the image's, under the board's own scale and offsets
void Board::DrawBombHaze(Graphics* g, float theHazeFactor)
{
	// The shader (slot 0xd8) answers a failure when the device cannot run pixel shaders
	if ((gD3DInterface->GetCaps(0) & 2) == 0)
		return;
	gD3DInterface->SetPixelShader(gTwistShaders[TWISTSHADER_MASK]);
	DDImage* aHeight = mFXManager->GetHeightImage();  // 0x6f5efc
	if (aHeight == nullptr)  // port: the original does not look
		return;
	gD3DInterface->SetRenderTarget(aHeight);
	mFXManager->mHazeMarked = true;
	float aScaleX = (float)aHeight->mWidth / (float)gApp->mWidth;
	float aScaleY = (float)aHeight->mHeight / (float)gApp->mHeight;
	// With the board moved or scaled the transform changes to one that also divides by the image's scale, so the
	// vertices below stand in the screen's pixels of the board
	bool aMoved = !(mShakeX == 0.0f && mShakeY == 0.0f && mBoardZoom == 1.0f && mBoardOffsetX == 0.0f &&
		mBoardOffsetY == 0.0f);
	if (aMoved) {
		SexyTransform2D aTransform;
		gD3DInterface->PopTransform();  // slot 0x18
		aTransform.LoadIdentity();      // 0x44f860
		float aInverse = (float)(1.0 / (double)aScaleX);
		aTransform.Scale(aInverse, aInverse);  // 0x44f990
		float aMoveX = (float)((double)-(mWidth / 2) + (double)mShakeX + (double)mBoardOffsetX);
		float aMoveY = (float)((double)-(mHeight / 2) + (double)mShakeY + (double)mBoardOffsetY);
		aTransform.Translate(aMoveX, aMoveY);  // 0x44f8a0
		aTransform.Scale(mBoardZoom, mBoardZoom);
		aTransform.Translate((float)(mWidth / 2), (float)(mHeight / 2));
		aTransform.Scale(aScaleX, aScaleX);
		gD3DInterface->PushTransform(aTransform, true);  // slot 0x14
	}
	int aBombCount = (int)mBombList.size();
	int aBomb = 0;
	if (0 < aBombCount) {
		do {
			Piece* aPiece = mBombList[aBomb];
			((MemoryImage*)IMAGE_HEAT_DISTORTION)->mD3DFlags = 8;  // +0x44
			float aT = (float)((double)aPiece->mBombCount / 9.0);
			float aFade = (float)(1.0 - (double)aT);
			float aStrength = aFade * theHazeFactor;
			// 0x77e016 is a floor
			int aAlpha = (int)std::floor((double)aStrength * 28.0 + 10.0);
			unsigned int aColor = ((unsigned int)aAlpha << 24) | 0xffffff;
			double aPeriodF = (double)aT * 85.0;
			if (aPeriodF < 55.0)
				aPeriodF = 55.0;
			int aPeriod = Ftol(aPeriodF);
			float aPhase = (float)(1.0 - (double)(gApp->mUpdateCount % aPeriod) / (double)aPeriod);
			float aPhase2 = (float)(1.0 + (double)aPhase);
			float aCenterX = (float)(gGemWidth / 2) + aPiece->mX;
			float aCenterY = (float)(gGemHeight / 2) + aPiece->mY;
			float aGrow1 = (float)((double)aStrength * 2.0 + 2.0);
			if (3.15f < aGrow1)
				aGrow1 = 3.15f;
			float aGrow2 = (float)((double)aStrength * 2.0 + 2.5);
			if (4.4f < aGrow2)
				aGrow2 = 4.4f;
			gD3DInterface->SetSamplerState(0, 1, 1);
			gD3DInterface->SetSamplerState(0, 2, 1);
			gD3DInterface->SetTexture(0, IMAGE_HEAT_DISTORTION);  // slot 0xd4
			gD3DInterface->SetTexture(1, IMAGE_SHADER_TEST);
			int aQuad = 0;
			do {
				float aAngle = (float)((double)aQuad * 3.1415927410125732 * 0.5 + 0.7853981852531433);
				float aAngleB = aAngle - 0.25132743f;
				float aAngleC = aAngle - 0.47123894f;
				float aAngleD = aAngle + 0.25132743f;
				float aAngleE = aAngle + 0.47123894f;
				float aRadiusA = (float)IMAGE_SHADER_TEST->mHeight * aGrow1;
				float aRadiusC = (float)IMAGE_SHADER_TEST->mHeight * aGrow2;
				float aRadiusB = gApp->mScale * 100.0f;
				HazeVertex aVertices[5];
				// Each vertex: the point on its radius at its angle, then the board's place, in the image's scale
				float aSin = sinf(aAngleE);
				float aY = (float)(((double)aRadiusA * (double)aSin + aCenterY) * aScaleY - 0.5);
				float aX = (float)(((double)cosf(aAngleE) * aRadiusA + aCenterX) * aScaleX - 0.5);
				aVertices[0] = MakeHazeVertex(aX, aY, aColor, 1.0f, aPhase2, 1.0f, 1.0f);  // 0x6a333f
				aSin = sinf(aAngleD);
				aY = (float)(((double)aRadiusB * (double)aSin + aCenterY) * aScaleY - 0.5);
				aX = (float)(((double)cosf(aAngleD) * aRadiusB + aCenterX) * aScaleX - 0.5);
				aVertices[1] = MakeHazeVertex(aX, aY, aColor, 0.7f, aPhase, 1.0f, 0.0f);
				aSin = sinf(aAngle);
				aY = (float)(((double)aRadiusC * (double)aSin + aCenterY) * aScaleY - 0.5);
				aX = (float)(((double)cosf(aAngle) * aRadiusC + aCenterX) * aScaleX - 0.5);
				aVertices[2] = MakeHazeVertex(aX, aY, aColor, 0.5f, aPhase2, 0.5f, 1.0f);
				aSin = sinf(aAngleB);
				aY = (float)(((double)aRadiusB * (double)aSin + aCenterY) * aScaleY - 0.5);
				aX = (float)(((double)cosf(aAngleB) * aRadiusB + aCenterX) * aScaleX - 0.5);
				aVertices[3] = MakeHazeVertex(aX, aY, aColor, 0.3f, aPhase, 0.0f, 0.0f);
				aSin = sinf(aAngleC);
				aY = (float)(((double)aRadiusA * (double)aSin + aCenterY) * aScaleY - 0.5);
				aX = (float)(((double)cosf(aAngleC) * aRadiusA + aCenterX) * aScaleX - 0.5);
				aVertices[4] = MakeHazeVertex(aX, aY, aColor, 0.0f, aPhase2, 0.0f, 1.0f);
				// The middle vertex's place again, its sum rounded to a float before the scale
				float aMidX = (float)((double)cosf(aAngle) * aRadiusC + aCenterX);
				aVertices[2].x = aMidX * aScaleX - 0.5f;
				float aMidY = (float)((double)sinf(aAngle) * aRadiusC + aCenterY);
				aVertices[2].y = aMidY * aScaleY - 0.5f;
				gD3DInterface->DrawPrimitiveEx(0x2c4, GPUPT_TRIANGLESTRIP, aVertices, 3, Color::White, 0, 0.0f, 0.0f, true,
					nullptr);  // slot 0x7c
				aQuad++;
			} while (aQuad < 4);
			aBomb++;
		} while (aBomb < aBombCount);
	}
	if (aMoved) {
		gD3DInterface->PopTransform();  // slot 0x18
		gD3DInterface->PushTransform(mTransform, true);  // slot 0x14
	}
	gD3DInterface->SetRenderTarget(g != nullptr ? g->mDestImage : nullptr);  // slot 0x98
	gD3DInterface->SetPixelShader(nullptr);
}

// 0x6caa13's rectangles: the cells of the pieces at the four corners of each bomb's cell, made into runs along each row
// (0x6b9e8d), runs of the same columns in rows below one another merged, and each run given its rectangle in the
// screen's pixels (0x6b9ace into +0x14cc)
void Board::GatherHazeRects()
{
	struct Run { int mLeft, mRight, mTop, mBottom; };  // 0x10 bytes
	std::vector<unsigned char> aMask((size_t)mRows * (size_t)mCols, 0);
	std::vector<Piece*> aCorners;  // 0x6b99c3
	int aCount = (int)mBombList.size();
	int aBomb = 0;
	if (0 < aCount) {
		do {
			Piece* aBombPiece = mBombList[aBomb];
			aCorners.push_back(GetPieceAt(aBombPiece->mRow - 1, aBombPiece->mCol - 1));
			aCorners.push_back(GetPieceAt(aBombPiece->mRow - 1, aBombPiece->mCol + 1));
			aCorners.push_back(GetPieceAt(aBombPiece->mRow + 1, aBombPiece->mCol - 1));
			aCorners.push_back(GetPieceAt(aBombPiece->mRow + 1, aBombPiece->mCol + 1));
			aBomb++;
		} while (aBomb < aCount);
	}
	int aCornerCount = (int)aCorners.size();
	int i = 0;
	if (0 < aCornerCount) {
		do {
			Piece* aPiece = aCorners[i];
			if (aPiece != nullptr)
				aMask[aPiece->mRow * mCols + aPiece->mCol] = 1;
			i++;
		} while (i < aCornerCount);
	}
	std::vector<std::vector<Run> > aRows((size_t)mRows);
	const unsigned char* aCell = aMask.data();
	int aRow = 0;
	if (0 < mRows) {
		do {
			int aStart = -1;
			int aEnd = -1;
			int aCol = 0;
			int aLimit = mCols + 1;
			if (0 < aLimit) {
				do {
					if (aCol < mCols && aCell[aCol] != 0) {
						aEnd = aCol;
						if (aStart < 0)
							aStart = aCol;
					} else if (-1 < aStart) {
						Run aRun = {aStart, aEnd, aRow, aRow};
						aRows[aRow].push_back(aRun);  // 0x6b9e8d
						aStart = -1;
						aEnd = -1;
					}
					aCol++;
					aLimit = mCols + 1;
				} while (aCol < aLimit);
			}
			aCell += mCols;
			aRow++;
		} while (aRow < mRows);
	}
	// From the bottom up, a run takes the bottom of the run of the same columns in the row below, which is dropped
	for (int r = mRows - 2; -1 < r; r--) {
		std::vector<Run>& anUpper = aRows[r];
		std::vector<Run>& aLower = aRows[r + 1];
		size_t j = 0;
		if (!anUpper.empty()) {
			do {
				size_t k = 0;
				if (!aLower.empty()) {
					do {
						if (anUpper[j].mLeft == aLower[k].mLeft && anUpper[j].mRight == aLower[k].mRight) {
							anUpper[j].mBottom = aLower[k].mBottom;
							aLower[k].mLeft = -1;
						}
						k++;
					} while (k < aLower.size());
				}
				j++;
			} while (j < anUpper.size());
		}
	}
	int aHalfWidth = mWidth / 2;
	int aHalfHeight = mHeight / 2;
	aRow = 0;
	if (0 < mRows) {
		do {
			size_t k = 0;
			if (!aRows[aRow].empty()) {
				do {
					const Run& aRun = aRows[aRow][k];
					if (-1 < aRun.mLeft) {
						HazeRect aRect;
						aRect.mLeft = (float)aHalfWidth + mBoardZoom *
							((float)((double)mBoardOffsetX + (double)mShakeX + ((double)GetColX(aRun.mLeft) - (double)aHalfWidth)));
						aRect.mTop = (float)aHalfHeight + mBoardZoom *
							((float)((double)mBoardOffsetY + (double)mShakeY + ((double)GetRowY(aRun.mTop) - (double)aHalfHeight)));
						aRect.mRight = (float)((((double)GetColX(aRun.mRight + 1) - (double)aHalfWidth) + (double)mShakeX +
							(double)mBoardOffsetX) * (double)mBoardZoom + (double)aHalfWidth);
						aRect.mBottom = (float)((((double)GetRowY(aRun.mBottom + 1) - (double)aHalfHeight) + (double)mShakeY +
							(double)mBoardOffsetY) * (double)mBoardZoom + (double)aHalfHeight);
						mHazeRects.push_back(aRect);  // 0x6b9ace
					}
					k++;
				} while (k < aRows[aRow].size());
			}
			aRow++;
		} while (aRow < mRows);
	}
}

void Board::DrawTutorialCursor(Graphics* g)
{
	g->SetColorizeImages(true);
	if (IsTipShowing() && -1 < mTipCol) {
		// The square of the forced move: "click here" pulsing, fading to "click now"
		int aX = GetColX(mTipCol) + gGemWidth;
		int aY = GetRowY(mTipRow) + gGemHeight;
		if (0.0f < mTipArrowFade) {
			Image* anImage = IMAGE_CURSOR_CLICKHERE;
			float aPhase = (float)((double)(float)(mUpdateCnt % 150) * 3.141592653589793 / 150.0);
			float aSin = std::sin(aPhase);
			g->SetColor(Color(255, 255, 255, Ftol(((double)aSin * 100.0 + 128.0) * (double)mTipArrowFade)));
			g->DrawImage(anImage, aX - anImage->mWidth / 2, aY - anImage->mHeight / 2);
		}
		if (mTipArrowFade < 1.0f) {
			Image* anImage = IMAGE_CURSOR_CLICKNOW;
			g->SetColor(Color(255, 255, 255, Ftol((1.0 - (double)mTipArrowFade) * 255.0)));
			g->DrawImage(anImage, aX - anImage->mWidth / 2, aY - anImage->mHeight / 2);
		}
	}
	if (mCursorShown && 0.0f < mCursorGlow) {
		// Four arrows around the cursor, pointing in and bobbing
		float aCenterX = (float)(mCursorX + gGemWidth);
		float aRadius = (float)((double)IMAGE_CURSOR_CLICKNOW->mWidth * 0.27);
		const float anAngles[4] = {0.0f, 1.5707964f, 3.1415927f, 4.712389f};
		float aCenterY = (float)(mCursorY + gGemHeight);
		for (int i = 0; i < 4; i++) {
			float anAngle = anAngles[i];
			float aBob = std::cos((float)((double)mCursorRestCount * 0.035));
			float aBobScale = (float)((double)gApp->mScale * 36.0);
			float anOffset = -aBob * aBobScale;
			g->SetColor(Color(255, 255, 255, Ftol((double)mCursorGlow * 255.0)));
			float aSin = std::sin(anAngle);
			float aSide = (float)((double)anAngle + 1.5707963705062866);
			float aSideSin = std::sin(aSide);
			float aCos = std::cos(anAngle);
			float aSideCos = std::cos(aSide);
			float aDrawY = (float)((double)aSin * (double)aRadius + (double)aCenterY + (double)aSideSin * (double)anOffset);
			float aDrawX = (float)((double)aCos * (double)aRadius + (double)aCenterX + (double)aSideCos * (double)anOffset);
			DrawImageCentered(g, IMAGE_TUTORIAL_ARROW, aDrawX, aDrawY,
				(double)anAngle * -1.0 - (double)6.2831855f * 0.25, 0.45f, 0.45f);
		}
	}
}

// 0x6a5c60: a light at (theX, theY) of size theSize and fall-off theFalloff adds to the gems near it (to all of them
// when wide), on each of their eight sides that faces it, at most theStrength
void Board::AddLight(float theX, float theY, float theSize, float theFalloff, float theStrength, Piece* theSkip,
	bool theSkipPower)
{
	float aFalloff = std::sqrt(theFalloff * theFalloff * gApp->mScale);
	float aSize = std::sqrt(theSize * theSize * gApp->mScale);
	static const float aDirs[16] = {0.0f, -1.0f, -0.707f, -0.707f, -1.0f, 0.0f, -0.707f, 0.707f, 0.0f, 1.0f, 0.707f,
		0.707f, 1.0f, 0.0f, 0.707f, -0.707f};
	float aGemW = (float)gGemWidth;
	int aCol0 = GetColAt(Ftol((double)theX - (double)aGemW));
	int aCol1 = GetColAt(Ftol((double)aGemW + (double)theX));
	if (aCol1 == -1)
		aCol1 = 6;
	float aGemH = (float)gGemHeight;
	int aRow0 = GetRowAt(Ftol((double)theY - (double)aGemH));
	int aRow1 = GetRowAt(Ftol((double)aGemH + (double)theY));
	if (aRow1 == -1)
		aRow1 = 8;
	if (256.0f < aFalloff) {
		aCol0 = 0;
		aCol1 = mCols - 1;
		aRow1 = mRows - 1;
		aRow0 = 0;
	}
	for (int aRow = aRow0; aRow <= aRow1; aRow++) {
		for (int aCol = aCol0; aCol <= aCol1; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece == nullptr || aPiece == theSkip || aPiece->mDimmed)
				continue;
			if (aPiece->IsFlagSet(Piece::FLAG_POWER) && theSkipPower)
				continue;
			float aDX = (float)((double)theX - ((double)(gGemWidth / 2) + (double)aPiece->mX));
			aDX = aDX / aSize;
			float aDY = (float)((double)theY - ((double)(gGemHeight / 2) + (double)aPiece->mY));
			aDY = aDY / aSize;
			float aDist = (float)((double)aDX * (double)aDX + (double)aDY * (double)aDY);
			aDist = aDist - aFalloff;
			if (!(1.0f < aDist))
				aDist = 1.0f;
			if (!((double)aDist < 100.0))
				continue;
			aDX = aDX / aDist;
			aDY = aDY / aDist;
			for (int k = 0; k < 8; k++) {
				float aLight = (float)(((double)aDirs[k * 2 + 1] * (double)aDY + (double)aDirs[k * 2] * (double)aDX) *
					(double)theStrength);
				if (!(theStrength > aLight))
					aLight = theStrength;
				if (0.0f < aLight)
					aPiece->mShineSlots[k] = aPiece->mShineSlots[k] + aLight;
			}
		}
	}
}

// 0x6aa6d1: the twist's cursor at +0x1c2c/+0x1c30, while it lies on the grid. With theRing the ring, its 24 glows,
// the multiplier's bulbs and their pulse; without, the spiral under the gems, red where the turn is blocked
void Board::DrawSelector(Graphics* g, bool theRing)
{
	mApp->Is3DAccelerated();
	g->PushState();
	float anArtScale = (float)((double)mApp->mArtRes / 1200.0);
	int anInset = Ftol((double)anArtScale * 10.0);
	Image* aRing = IMAGE_CURSOR_RING;
	int aRingWidth = aRing->mWidth;
	int aRingHeight = aRing->mHeight;
	if (!((float)GetColX(0) <= mEasedCursorX) || !((float)GetRowY(0) <= mEasedCursorY)) {
		g->PopState();
		return;
	}
	int aMidY = Ftol((double)gGemHeight + (double)mEasedCursorY);
	int aMidX = Ftol((double)gGemWidth + (double)mEasedCursorX);
	int aRingX = aMidX - aRingWidth / 2;
	int aRingY = aMidY - aRingHeight / 2;
	if (theRing) {
		if (!mEditorMode) {
			g->PushState();
			g->SetColorizeImages(true);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			g->SetColor(Color::White);
			g->DrawImage(aRing, aRingX, aRingY, aRingWidth, aRingHeight);
			// The 24 glows round the ring, each placed by its offset in the resources against the ring's
			int aBaseX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(IMAGE_CURSOR_RING_ID));
			int aBaseY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(IMAGE_CURSOR_RING_ID));
			for (int i = 0; i < 24; i++) {
				if (0.0f < mGlowRing[i]) {
					int anId = IMAGE_CURSOR_YELLOW1_ID + i;
					Image* aGlow = GetImageById(anId);
					float aLevel = mGlowRing[i] <= 1.0f ? mGlowRing[i] : 1.0f;
					g->SetColor(Color(255, 255, 255, Ftol((double)aLevel * 255.0 * (double)mCursorAlpha)));
					int aY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(anId));
					int aX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(anId));
					g->DrawImage(aGlow, aX - aBaseX + aRingX, aY - aBaseY + aRingY);
				}
			}
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			g->SetColor(Color::White);
			if (mGameMode != MODE_CHALLENGE && !mTipShowing) {
				// The multiplier's bulbs above the ring, lit to the level, and their pulse
				int aCel = mStepLevel - 1;
				Image* aBulbs = IMAGE_CURSOR_MULTIBULBS;
				int aDY = GetImageOffsetY(IMAGE_CURSOR_MULTIBULBS_ID) - GetImageOffsetY(GetIdByImage(aRing));
				int aBulbY = Ftol((double)aDY * (double)anArtScale + (double)(aMidY - aRing->mHeight / 2));
				g->DrawImageCel(aBulbs, aMidX - aBulbs->GetCelWidth() / 2, aBulbY, aCel);
				if (0.0f < mRingBurst) {
					float aSize = (float)((1.0 - (double)gBoard->mRingBurst) * 3.0 + 1.0);
					g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
					Image* aPulse = IMAGE_CURSOR_MULTIPULSE;
					g->SetColor(Color(255, 255, 255, Ftol((double)mRingBurst * 255.0)));
					while (1.0f <= aSize) {
						int aWidth = Ftol((double)aPulse->mWidth * (double)aSize);
						int aHeight = Ftol((double)aPulse->mHeight * (double)aSize);
						int aX = aMidX - aWidth / 2;
						int k = 2;
						do {
							g->DrawImage(aPulse, aX, aPulse->GetCelHeight() / 2 - aHeight / 2 + aBulbY, aWidth,
								aHeight);
							k--;
						} while (k != 0);
						aSize = (float)((double)aSize - 1.0);
					}
				}
			}
			g->SetColorizeImages(false);
			g->PopState();
		}
	} else {
		// The spiral for the level (its first five, then the sixth), turning with the cursor
		g->SetColorizeImages(true);
		int aLevel = mStepLevel - 1;
		if (5 < aLevel)
			aLevel = 5;
		Image* aSpiral = GetImageById(IMAGE_CURSOR_SPIRAL1_ID + aLevel);
		if (mTwistBlocked)
			g->SetColor(Color(255, 0, 0, Ftol((double)mCursorAlpha * 200.0 * (double)mCursorFadeIn)));
		else
			g->SetColor(Color(255, 255, 255, Ftol((double)mCursorAlpha * 160.0 * (double)mCursorFadeIn)));
		float anOff = (float)anInset;
		g->DrawImageRotated(aSpiral, Ftol((double)mEasedCursorX - (double)anOff),
			Ftol((double)mEasedCursorY - (double)anOff), (double)mCursorTurnNow + (double)mCursorTurn);
		if (0.0f < mCursorTwistFade) {
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColor(Color(128, 255, 0,
				Ftol((double)mCursorAlpha * 255.0 * (double)mCursorFadeIn * (double)mCursorTwistFade)));
			int k = 3;
			do {
				g->DrawImageRotated(aSpiral, Ftol((double)mEasedCursorX - (double)anOff),
					Ftol((double)mEasedCursorY - (double)anOff), (double)mCursorTurnNow + (double)mCursorTurn);
				k--;
			} while (k != 0);
		}
	}
	g->PopState();
}

// 0x6b4e82: the tooltip UpdateToolTips brought up: a box at (+0x70c, +0x710) with the title and the text in it and an
// arrow at its side, all fading with +0x788 (the arrow slides with +0x718). A bomb gem's in the classic game adds the
// danger level's skulls.
void Board::DrawToolTip(Graphics* g)
{
	int aX = mTooltipX;
	// the tooltip goes to the right of its place, or to the left when the last one's right edge was far out
	bool aLeft = !((double)(mTooltipWidth + mTooltipX) <= (double)mWidth * 0.8999999761581421);
	if (aLeft)
		aX += -mTooltipWidth - Ftol((double)gApp->mScale * 120.0);
	else
		aX += Ftol((double)gApp->mScale * 105.0);
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, Ftol(mTooltipFade.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
	int anArrowX;
	if (aLeft) {
		anArrowX = mTooltipWidth - Ftol(mTooltipArrowSlide.GetOutVal() * 6.0);
	} else {
		anArrowX = Ftol((double)gApp->mScale * -80.0) - Ftol(mTooltipArrowSlide.GetOutVal() * 6.0);
	}
	anArrowX += aX;
	int anArrowY;
	if (Ftol((double)gApp->mScale * 20.0) > mTooltipY - IMAGE_INGAMEUI_TOOLTIPARROW->GetCelHeight() / 2)
		anArrowY = Ftol((double)gApp->mScale * 20.0);
	else
		anArrowY = mTooltipY - IMAGE_INGAMEUI_TOOLTIPARROW->GetCelHeight() / 2;
	g->DrawImageMirror(IMAGE_INGAMEUI_TOOLTIPARROW, anArrowX, anArrowY, aLeft);
	int aSpacing = Ftol((double)gApp->mScale * 32.0);
	g->SetFont(FONT_TOOLTIP_BOLD);
	int aTitleHeight = g->GetWordWrappedHeight(mTooltipWidth, mTooltipTitle, aSpacing, nullptr);
	g->SetFont(FONT_TOOLTIP);
	int aTextHeight = g->GetWordWrappedHeight(mTooltipWidth, mTooltipText, aSpacing, nullptr);
	bool aDanger = mTooltipTitle == mApp->GetString(GTS(0x7dcce4), GT(0x7dccc0)) && mGameMode == MODE_CLASSIC;
	int aHeight = (aTextHeight - Ftol((double)gApp->mScale * -12.0)) + aTitleHeight;
	if (aDanger)
		aHeight += Ftol((double)gApp->mScale * 106.0);
	int aPadX = Ftol((double)gApp->mScale * 36.0);
	int aPadY = Ftol((double)gApp->mScale * 32.0);
	int aTop = mTooltipY - aHeight / 2;
	if (Ftol((double)gApp->mScale * 40.0) > aTop)
		aTop = Ftol((double)gApp->mScale * 40.0);
	Rect aTextRect(aX, aTop, mTooltipWidth, aHeight);
	Rect aBoxRect(aX - aPadX, aTop - aPadY, mTooltipWidth + aPadX * 2, aHeight + aPadY * 2);
	g->SetColor(Color(255, 255, 255, Ftol(mTooltipFade.GetOutVal() * 255.0 * 0.88)));
	g->DrawImageBox(aBoxRect, IMAGE_INGAMEUI_TOOLTIPDIALOG);
	g->SetFont(FONT_TOOLTIP_BOLD);
	g->SetColor(Color(255, 255, 255, Ftol(mTooltipFade.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
	g->WriteWordWrapped(aTextRect, mTooltipTitle, aSpacing, 0, nullptr, -1, nullptr);
	if (aDanger) {
		SexyString aLevel = mApp->GetString(GTS(0x7dcc94), GT(0x7dcc68));
		g->WriteString(aLevel, mTooltipWidth / 2 + aX, (aTop - Ftol((double)gApp->mScale * 64.0)) + aHeight, -1, 0, true, 0, -1, -1);
	}
	aTextRect.mY += aTitleHeight - Ftol((double)gApp->mScale * -12.0);
	g->SetFont(FONT_TOOLTIP);
	g->SetColor(Color(255, 255, 255, Ftol(mTooltipFade.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
	g->WriteWordWrapped(aTextRect, mTooltipText, aSpacing, 0, nullptr, -1, nullptr);
	if (aDanger) {
		// the skulls: one more than the danger level, in a row under the text
		g->SetColor(Color::White);
		int aCount = mWheelSpinCount + 1;
		int i = 0;
		if (0 < aCount) {
			do {
				int aSkullWidth = IMAGE_SKULLICON_SMALL->mWidth;
				g->DrawImage(IMAGE_SKULLICON_SMALL,
					mTooltipWidth / 2 + (aSkullWidth * i - (aSkullWidth * aCount) / 2) + aX,
					(aTop - Ftol((double)gApp->mScale * 64.0)) + aHeight);
				i++;
				aCount = mWheelSpinCount + 1;
			} while (i < aCount);
		}
	}
}

void Board::DrawOverlay(Graphics* g)
{
	if (0.0f < mFruitBlast) {
		// A fruit's blast darkens the screen (to 0.6 at most: in over its first quarter, out over its last fifth)
		mFruitGlowAlpha.SetInVal(mFruitBlast);
		mFruitBlastScale.SetInVal(mFruitBlast);
		double anIn = (double)mFruitBlast * 4.0;
		double anOut = (1.0 - (double)mFruitBlast) * 5.0;
		double aDark = anIn < anOut ? anIn : anOut;
		if (0.6000000238418579 < aDark)
			aDark = 0.6000000238418579;
		g->SetColor(Color(0, 0, 0, Ftol(aDark * 255.0)));
		g->FillRect(0, 0, mWidth, mHeight);
	}
	if (gD3DInterface == nullptr)
		g->Translate(Ftol(mShakeX), Ftol(mShakeY));
	else if (mBoardZoom != 1.0f)
		gD3DInterface->PushTransform(mTransform, true);  // slot 0x14
	if (0.0f < mFruitBlast) {
		// and the gems it can reach are drawn over the dark, which the board's own pass left out (UpdateFruitBlast)
		int aCount = mCols * mRows;
		if (0 < aCount) {
			Piece** aCell = mGrid;
			do {
				Piece* aPiece = *aCell;
				if (aPiece != nullptr && !aPiece->mIsDrawn) {
					aPiece->Draw(g, false);
					aPiece->Draw(g, true);
				}
				aCell++;
				aCount--;
			} while (aCount != 0);
		}
	}
	DrawCounters(g);
	if (gD3DInterface == nullptr)
		g->Translate(-Ftol(mShakeX), -Ftol(mShakeY));
	else if (mBoardZoom != 1.0f)
		gD3DInterface->PopTransform();  // slot 0x18
	if (mGameOver && mGameMode == MODE_BLITZ && 4 < mEndState) {
		// Blitz's FINAL SCORE (end state 5) and then its score (6) zoom in with a rainbow trail behind them: copies
		// from each one's lowest scale up to its current one, the one at 1 drawn plainly
		float aTo[2] = {1.0f, 1.0f};
		float aFrom[2] = {1.0f, 1.0f};
		if (mEndState == 5) {
			aTo[0] = mNukeZoom;
			float aLow = mNukeZoom - 2.0f;
			if (aLow < 1.0f)
				aLow = 1.0f;
			aFrom[0] = aLow;
			aFrom[1] = -1.0f;
			aTo[1] = -1.0f;
		} else if (mEndState == 6) {
			aTo[1] = mNukeZoom;
		}
		g->SetColorizeImages(true);
		ColorCycle aCycle;
		int i = 0;
		do {
			aCycle.SetPosition((float)((double)mUpdateCnt * 0.001));
			float aScale = aFrom[i];
			if (0.0f <= aScale) {
				while (aScale <= aTo[i]) {
					Color aColor = aCycle.mColor;
					if (aScale == 1.0f) {
						g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
						aColor.mAlpha = Ftol((double)mFinalScoreFade * 255.0);
					} else {
						g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
						aColor.mAlpha = 16;
					}
					g->SetColor(aColor);
					Image* anImage = i == 0 ? (Image*)mFinalScoreLabel : (Image*)mFinalScoreText;
					int aY = Ftol((double)mHeight * 0.4000000059604645);
					float aRise = (float)((double)aScale * (double)(float)mHeight * 0.009999999776482582);
					if (i == 0)
						aRise = -aRise;
					else
						aY = Ftol((double)mHeight * 0.20000000298023224 + (double)aY);
					DrawImageScaledCentered(g, anImage, (float)(mWidth / 2),
						(float)(((double)aScale - 1.0) * (double)aRise + (double)aY), aScale, aScale);
					aScale = (float)((double)(mApp->Is3DAccelerated() ? 0.1f : 0.5f) + (double)aScale);
					aCycle.Update();
				}
			}
			i++;
		} while (i < 2);
	}
	if (mTooltipUp && !mApp->mStartClassicAtOnce && !mIsReplay && !mReplayPlaying)
		DrawToolTip(g);
	// A replay: the rewind draws the last recorded frame and drops it (and the frames down to +0x3448), dimmed and
	// streaked like a tape running back; then REPLAY blinks in the corner while the move is made again
	bool aReplay = false;
	if (mPlayState == 1) {
		gGraphicsRecorder->DrawFrame(g, gGraphicsRecorder->GetLastFrame());
		gGraphicsRecorder->PopFramesFrom(mRewindFrame);
		aReplay = true;
	}
	if (mReplayPlaying)
		aReplay = true;
	if (aReplay && !mIsReplay) {
		// (the exe brackets this with MTRand::SetRandAllowed, 0x456310, a debug count)
		if (gD3DInterface != nullptr) {
			// With Direct3D the streaks are lines one pixel wide down the left edge: a rewind's red and green, else
			// green. (Its draws come from +0x2a80's generator too.)
			if (mPlayState == 1) {
				g->SetColor(Color(0x201000));
				g->FillRect(0, 0, 1, mHeight);
				int i = 0;
				while (true) {
					int aLimit = mRewindFrame * 5;
					if (0x31 < aLimit)
						aLimit = 0x32;
					if (aLimit <= i)
						break;
					g->SetColor(Color(255, 0, 0, 0x28));
					int aHeight = (int)(mReplayRand.Next() % 0x1e) + 0x1e;
					int aY = (int)(mReplayRand.Next() % (unsigned long)(unsigned int)mHeight);
					g->FillRect(0, aY, 1, aHeight);
					i++;
				}
				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				i = 0;
				while (true) {
					int aLimit = mRewindFrame / 3;
					if (4 < aLimit)
						aLimit = 5;
					if (aLimit <= i)
						break;
					g->SetColor(Color(0, 255, 0, 0x40));
					int aHeight = (int)(mReplayRand.Next() % 10) + 2;
					int aY = (int)(mReplayRand.Next() % (unsigned long)(unsigned int)mHeight);
					g->FillRect(0, aY, 1, aHeight);
					i++;
				}
			} else {
				g->SetColor(Color(0));
				g->FillRect(0, 0, 1, mHeight);
				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				int i = 0;
				if (0 < (int)mReplayRand.Next() % 3 + 2) {
					do {
						g->SetColor(Color(0, 255, 0, 4));
						int aHeight = (int)(mReplayRand.Next() % 6) + 4;
						int aY = (int)(mReplayRand.Next() % (unsigned long)(unsigned int)mHeight);
						g->FillRect(0, aY, 1, aHeight);
						i++;
					} while (i < (int)mReplayRand.Next() % 3 + 2);
				}
			}
			if ((gD3DInterface->GetCaps(0) & 2) != 0) {
				// The whole screen is taken (0x4f6670 "Rewind": slot 0xa0 hands the screen's picture to a pooled target
				// and leaves a cleared one) and drawn back through the rewind shader (0x85db10)
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);  // 0x41ff20
				TargetHold aHold;
				DDImage* aScreen = gD3DInterface->HoldScreen(&aHold, "Rewind");
				gD3DInterface->SetPixelShader(gTwistShaders[TWISTSHADER_REWIND]);  // slot 0xd8, 0x85db10
				g->DrawImage(aScreen, 0, 0);  // 0x4211b0
				gD3DInterface->SetPixelShader(nullptr);
				gD3DInterface->ReleaseTarget(&aHold);
			}
		} else if (mPlayState == 1) {
			g->SetColor(Color(64, 64, 64, 128));
			g->FillRect(0, 0, mWidth, mHeight);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			for (int i = 0; i < (int)mReplayRand.Next() % 10 + 6; i++) {
				g->SetColor(Color(255, 255, 255, 64));
				int aHeight = (int)(mReplayRand.Next() & 1) + 1;
				int aY = (int)(mReplayRand.Next() % (unsigned long)(unsigned int)mHeight);
				g->FillRect(0, aY, mWidth, aHeight);
			}
		}
	}
	bool aShowReplay = mIsReplay ? !mApp->Is3DAccelerated() : mReplayPlaying && mPlayState != 1;
	if (aShowReplay && ((mApp->mUpdateCount / 10) & 1) == 0) {
		g->SetColorizeImages(false);
		Image* aReplayImage = IMAGE_REPLAY;
		g->DrawImage(aReplayImage, Ftol((double)mWidth - (double)aReplayImage->mWidth * (double)1.1f),
			Ftol((double)mHeight - (double)aReplayImage->mHeight * (double)1.1f));
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	for (Traveller* aTraveller : mSmokeTravellers)  // 0x6a9b85
		aTraveller->Draw();
	if (mEndingPiece != nullptr) {
		if (mPlayState == 7) {
			// The nuke: the gem that set it off, its nova and the blast
			if (gD3DInterface != nullptr) {
				// With Direct3D the board and the tube are drawn again over the game's, the tube turned and moved by
				// +0x2978 and the board's gems cleared by IMAGE_BOOM_BOARD, the crater and the bars
				gD3DInterface->PushTransform(mTransform, true);  // slot 0x14
				gD3DInterface->SetDepthTestMode(1);              // slot 0x114
				g->SetColor(Color::White);
				if (mLevelStarted) {
					double aTurn = mNukeTubeShake.GetOutVal();
					int aTubeDownI = Ftol((double)gApp->mScale * 64.0);
					double aMoveY = mNukeTubeShake.GetOutVal();
					float aRiseY = (float)((double)gApp->mScale * 42.0);
					int aTubeRightI = Ftol((double)gApp->mScale * 32.0);
					double aMoveX = mNukeTubeShake.GetOutVal();
					int aY = Ftol((double)(mGoodTube->mY + aTubeDownI) + (double)aRiseY * aMoveY);
					float aRiseX = (float)((double)gApp->mScale * -8.0);
					int aX = Ftol(aMoveX * (double)aRiseX + (double)(mGoodTube->mX + aTubeRightI));
					g->DrawImageRotated(IMAGE_BOOM_FTUBE, aX, aY, aTurn * 0.10000000149011612, nullptr);  // 0x4219d0
				}
				if (mBoardShown) {
					g->DrawImage(IMAGE_BOOM_BOARD, GetColX(0), GetRowY(0));
					Graphics aClipG(*g);  // 0x41fbc0
					aClipG.SetClipRect(GetColX(0), GetRowY(0), mCols * gGemWidth, mRows * gGemHeight);  // 0x4222e0
					Image* aCrater = IMAGE_BOOM_CRATER;
					int aCraterY = Ftol(((double)(gGemHeight / 2) + (double)mEndingPiece->mY) - (double)(aCrater->mHeight / 2));
					int aCraterX = Ftol(((double)(gGemWidth / 2) + (double)mEndingPiece->mX) - (double)(aCrater->mWidth / 2));
					aClipG.DrawImage(aCrater, aCraterX, aCraterY);
					Board* aBoard = gBoard;
					int aHalfGrid = (aBoard->mRows * gGemHeight) / 2;
					Image* aBarTop = IMAGE_BOOM_FGRIDBAR_TOP;
					int aBarHeight = aBarTop->mHeight;
					float aBarHalf = (float)(aBarHeight / 2);
					int aBarTopY = Ftol(((double)(aBoard->GetRowY(0) + aHalfGrid) - (double)GetGridMidY()) - (double)aBarHalf);
					int aBarBotY = Ftol((double)((double)(aBoard->GetRowY(0) + aHalfGrid) + (double)GetGridMidY() - (double)aBarHalf) +
						(double)Ftol((double)gApp->mScale * 10.0));
					int aBarX = aBoard->GetColX(0) + ((aBoard->mCols * gGemWidth) / 2 - mBarWidth / 2) + 2;
					g->DrawImage(aBarTop, aBarX, aBarTopY);
					Image* aBarBot = IMAGE_BOOM_FGRIDBAR_BOT;
					g->SetColor(Color(255, 255, 255, Ftol((double)mBarBottomAlpha * 255.0 * (double)mBoardFade)));
					g->DrawImage(aBarBot, aBarX, aBarBotY, mBarWidth, aBarHeight);  // 0x4216f0
				}
				gD3DInterface->SetDepthTestMode(0);
			}
			// The gem is drawn zoomed about its middle by +0x2908 in 3D (the matrix, 0x44f860 and on, is also made in 2D)
			SexyTransform2D aNukeMatrix;
			float aNukeBackY = (float)((double)-mEndingPiece->mY - (double)(gGemHeight / 2));
			aNukeMatrix.Translate((float)((double)-mEndingPiece->mX - (double)(gGemWidth / 2)), aNukeBackY);
			float aNukeZoom = (float)mNukeGemZoom.GetOutVal();
			aNukeMatrix.Scale((float)mNukeGemZoom.GetOutVal(), aNukeZoom);
			aNukeMatrix.Translate((float)((double)(gGemWidth / 2) + (double)mEndingPiece->mX),
				(float)((double)(gGemHeight / 2) + (double)mEndingPiece->mY));
			if (gD3DInterface != nullptr)
				gD3DInterface->PushTransform(aNukeMatrix, true);
			if (!mEndingPiece->mIsDrawn)
				DrawNukeGem(g, mEndingPiece);
			if (gD3DInterface != nullptr)
				gD3DInterface->PopTransform();
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, Ftol(mNukeNovaAlpha.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
			Transform aTransform;
			float aScale = (float)mNukeNovaScale.GetOutVal();
			aTransform.Scale(aScale, (float)mNukeNovaScale.GetOutVal());
			float aY = (float)((double)(gGemHeight / 2) + (double)mEndingPiece->mY);
			float aX = (float)((double)(gGemWidth / 2) + (double)mEndingPiece->mX);
			g->DrawImageTransform(IMAGE_BOOM_NOVA, aTransform, aX, aY);
			if (0.0 < mNukeProgress.GetOutVal()) {
				g->SetColor(Color(255, 255, 255, Ftol(mNukeCloudAlpha.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
				aTransform.Reset();
				aScale = (float)mNukeProgress.GetOutVal();
				aTransform.Scale(aScale, (float)mNukeProgress.GetOutVal());
				aY = (float)((double)(gGemHeight / 2) + (double)mEndingPiece->mY);
				aX = (float)((double)(gGemWidth / 2) + (double)mEndingPiece->mX);
				g->DrawImageTransform(IMAGE_BOOM_NUKE, aTransform, aX, aY);
			}
			if (gD3DInterface != nullptr)
				gD3DInterface->PopTransform();  // the board's, pushed above
		}
		g->SetColorizeImages(true);
		for (Shockwave* aWave : mShockwaves)
			aWave->Draw(g);
	}
	if (mGoScale.GetOutVal() != 0.0 && mPauseShownCount == 0) {
		// "GO", fading in over the curve's first half
		double aFade = mGoScale.GetOutVal() * 2;
		if (aFade > 1.0)
			aFade = 1.0;
		else if (0.0 > aFade)
			aFade = 0.0;
		float anAlpha = (float)(aFade * (double)mBoardFade);
		float aScale = (float)mGoScale.GetOutVal();
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, Ftol((double)anAlpha * 255.0)));
		if (IMAGE_GO != nullptr) {
			float aY = (float)((double)mHeight * 0.44999998807907104);
			float aX = (float)((double)gGemWidth * 4.0 + (double)mBoardX);
			DrawImageScaledCentered(g, IMAGE_GO, aX, aY, aScale, aScale);
		}
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	for (FlyingText* aText : mFlyingTexts)
		aText->Draw(g);
	if (mFruitDance != nullptr) {
		// The fruit dance darkens the screen (to 180) as it stops the game (Update)
		int aDance = mFruitDanceUpdates;
		float aFromStart = (float)((double)aDance * 0.009999999776482582);
		float aFromEnd = (float)((double)(2200 - aDance) * 0.009999999776482582);
		float aNear = aFromEnd <= aFromStart ? aFromEnd : aFromStart;
		if (1.0f < aNear)
			aNear = 1.0f;
		if (0.0f < aNear) {
			g->SetColor(Color(0, 0, 0, Ftol((double)aNear * 180.0)));
			g->FillRect(0, 0, mWidth, mHeight);
		}
	}
	if (mBoardZoom != 1.0f && gD3DInterface != nullptr)
		gD3DInterface->PushTransform(mTransform, true);  // slot 0x14
	if (0.0f < mFruitBlast) {
		// A fruit's blast: its four rings in turning colours (IMAGE_FRUITNOVA), and the fruit's glow growing
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		int aRing = 0;
		do {
			float aT = (float)((double)aRing * -0.15 + (double)mFruitBlast * 2.799999952316284 - 0.5);
			if (aRing == 3)
				aT = (float)(((double)aT - 0.30000001192092896) * 0.8500000238418579);
			mBlastRingSize.SetInVal(aT);
			mBlastRingAlpha.SetInVal(aT);
			int aHue = Ftol(mBlastRingSize.GetOutVal() * 256.0) % 256;
			Color aColor = Color((int)mApp->HSLToRGB(aHue, 255, aRing == 3 ? 200 : 128));
			aColor.mAlpha = Ftol(mBlastRingAlpha.GetOutVal() * 255.0);
			if (aColor.mAlpha != 0) {
				g->SetColor(aColor);
				int aSize = Ftol(mBlastRingSize.GetOutVal() * (double)mHeight);
				int aHalf = aSize / 2;
				g->DrawImage(IMAGE_FRUITNOVA, GetColX(mFruitCol) + (gGemWidth / 2 - aHalf),
					GetRowY(mFruitRow) + (gGemHeight / 2 - aHalf), aSize, aSize);
			}
			aRing++;
		} while (aRing < 4);
		g->SetColor(Color(255, 255, 255, Ftol(mFruitGlowAlpha.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
		double aScale = mFruitBlastScale.GetOutVal();
		int aSize = Ftol((double)IMAGE_FRUITBLAST_FRUITGLOWS->GetCelWidth() * aScale);
		int anOffset = (gGemWidth - aSize) / 2;
		g->DrawImageCel(IMAGE_FRUITBLAST_FRUITGLOWS,
			Rect(GetColX(mFruitCol) + anOffset, GetRowY(mFruitRow) + anOffset, aSize, aSize), mFruitColor);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
	if (mFruitDance != nullptr) {
		g->Translate(mFruitDance->mX, mFruitDance->mY);
		mFruitDance->Draw(g);
		g->Translate(-mFruitDance->mX, -mFruitDance->mY);
	}
	mPointsManager->DrawOverlay(g, 0x62);
	if (mBoardZoom != 1.0f && gD3DInterface != nullptr)
		gD3DInterface->PopTransform();  // slot 0x18
	// The banner of a mode just unlocked (0x6b40b5)
	if (mBannerShow.GetOutVal() != 0.0) {
		g->SetColorizeImages(true);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		Image* aModeImage = mUnlockedMode == MODE_CHALLENGE ? IMAGE_MODEUNLOCK_CHALLENGE : IMAGE_MODEUNLOCK_BLITZ;
		g->SetColor(Color(255, 255, 255, Ftol(mBannerStripe.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
		g->DrawImage(IMAGE_MODEUNLOCK_STRIPE, mWidth / 2 - IMAGE_MODEUNLOCK_STRIPE->mWidth / 2,
			Ftol((double)gApp->mScale * 60.0));
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		float aCount = (float)mUpdateCnt;
		int aCloud1 = Ftol((double)aCount * 3.3499999046325684) % IMAGE_MODEUNLOCK_CLOUD1->mWidth;
		int aCloud2 = Ftol((double)aCount * 2.5) % IMAGE_MODEUNLOCK_CLOUD2->mWidth;
		g->SetColor(Color(255, 255, 255, Ftol(mBannerShow.GetOutVal() * 255.0)));
		g->DrawImage(IMAGE_MODEUNLOCK_CLOUD1, mWidth / 2 - IMAGE_MODEUNLOCK_CLOUD1->mWidth / 2 + aCloud1,
			Ftol((double)gApp->mScale * 60.0));
		g->DrawImage(IMAGE_MODEUNLOCK_CLOUD1, mWidth / 2 - (IMAGE_MODEUNLOCK_CLOUD1->mWidth * 3) / 2 + aCloud1,
			Ftol((double)gApp->mScale * 60.0));
		g->DrawImage(IMAGE_MODEUNLOCK_CLOUD2, mWidth / 2 - IMAGE_MODEUNLOCK_CLOUD2->mWidth / 2 - aCloud2,
			Ftol((double)gApp->mScale * 60.0));
		g->DrawImage(IMAGE_MODEUNLOCK_CLOUD2, IMAGE_MODEUNLOCK_CLOUD2->mWidth / 2 + mWidth / 2 - aCloud2,
			Ftol((double)gApp->mScale * 60.0));
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColor(Color(255, 255, 255, Ftol(mBannerModePicture.GetOutVal() * 255.0)));
		g->DrawImage(aModeImage, mWidth / 2 - aModeImage->mWidth / 2, Ftol((double)gApp->mScale * 100.0));
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color(255, 255, 255, Ftol(mBannerGlow.GetOutVal() * 255.0)));
		g->DrawImage(aModeImage, mWidth / 2 - aModeImage->mWidth / 2, Ftol((double)gApp->mScale * 100.0));
		g->SetColor(Color(255, 255, 255, Ftol(mBannerSheenColor.GetOutVal() * 255.0)));
		g->DrawImage(IMAGE_MODEUNLOCK_SHEEN, Ftol(mBannerSheenPos.GetOutVal() * (double)mWidth),
			Ftol((double)gApp->mScale * 80.0));
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
}

// 0x6b42e1: the gems and cascades of the last move, each a number with its name under it, left of the board
void Board::DrawCounters(Graphics* g)
{
	int aX = GetColX(0) - Ftol((double)gApp->mScale * 95.0);
	g->SetColorizeImages(true);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	Color aShadow(0, 0, 0, 128);
	if (mCountPopupShadow != 0.0f) {
		Color aColor(96, 96, 255);
		int aY = Ftol((double)GetRowY(3) - (double)gGemHeight * 0.185);
		float aScale = (1.0f - mCountPopupFade) * 2.0f + 2.0f;
		aColor.mAlpha = Ftol((double)mCountPopupFade * 255.0);
		g->SetColor(aColor);
		g->PushState();
		SexyString aText = StrFormat(GTC(0x80da2c), mCountPopupCount);
		Font* aFont = FONT_POPUP_COUNT;
		int aWidth = aFont->StringWidth(aText);
		int anAscent = aFont->GetAscent();
		SetFontLayerColor(aFont, 1, Color(128, 128, 128));
		g->SetFont(aFont);
		g->SetScale(aScale, aScale, (float)aX, (float)aY);
		g->DrawString(aText, aX - aWidth / 2, anAscent / 2 + aY);
		float aScale2 = (float)(mCountPopupScale.GetOutVal() * 0.9);
		g->SetScale(aScale2, aScale2, (float)aX, (float)aY);
		aShadow.mAlpha = Ftol((double)mCountPopupShadow * 128.0);
		g->SetColor(aShadow);
		int anOffset = Ftol((double)gApp->mScale * -8.0);
		g->DrawString(aText, aX - aWidth / 2 - anOffset, anAscent / 2 - anOffset + aY);
		int anAlpha = Ftol((double)mCountPopupShadow * 255.0);
		aColor.mAlpha = anAlpha;
		g->SetColor(aColor);
		g->DrawString(aText, aX - aWidth / 2, anAscent / 2 + aY);
		g->PopState();
		g->SetFont(FONT_RANK_DIALOG);
		SetFontLayerColor(g->GetFont(), 0, Color::White);
		aText = mApp->GetString(GTS(0x7dcc20));
		g->SetColor(Color(64, 64, 64, anAlpha));
		int aTextX = Ftol((double)gApp->mScale * 10.0) - g->StringWidth(aText) / 2 + aX;
		aY += Ftol((double)gApp->mScale * 130.0);
		int aShadowOffset = Ftol((double)gApp->mScale * -2.0);
		g->DrawString(aText, aTextX - aShadowOffset, aY - aShadowOffset);
		g->SetColor(Color(255, 255, 255, anAlpha));
		g->DrawString(aText, aTextX, aY);
	}
	if (0.0f < mCascadePopupShadow) {
		int aY = Ftol((double)GetRowY(4) + (double)gGemHeight * 0.6);
		Color aColor(255, 64, 64);
		float aScale = (1.0f - mCascadePopupFade) * 0.9f + 1.0f;
		aColor.mAlpha = Ftol((double)mCascadePopupFade * 255.0);
		SexyString aText = StrFormat(GTC(0x80da2c), mCascadePopupCount);
		Font* aFont = FONT_POPUP_COUNT;
		int aWidth = aFont->StringWidth(aText);
		int anAscent = aFont->GetAscent();
		SetFontLayerColor(aFont, 1, Color(128, 128, 128));
		g->PushState();
		g->SetFont(aFont);
		g->SetColor(aColor);
		g->SetScale(aScale, aScale, (float)aX, (float)aY);
		g->DrawString(aText, aX - aWidth / 2, anAscent / 2 + aY);
		float aScale2 = (float)mCascadePopupScale.GetOutVal();
		g->SetScale(aScale2, aScale2, (float)aX, (float)aY);
		aShadow.mAlpha = Ftol((double)mCascadePopupShadow * 128.0);
		g->SetColor(aShadow);
		int anOffset = Ftol((double)gApp->mScale * -8.0);
		g->DrawString(aText, aX - aWidth / 2 - anOffset, anAscent / 2 - anOffset + aY);
		int anAlpha = Ftol((double)mCascadePopupShadow * 255.0);
		aColor.mAlpha = anAlpha;
		g->SetColor(aColor);
		g->DrawString(aText, aX - aWidth / 2, anAscent / 2 + aY);
		g->PopState();
		g->SetFont(FONT_RANK_DIALOG);
		SetFontLayerColor(g->GetFont(), 0, Color::White);
		aText = mApp->GetString(GTS(0x7dcc04));
		int aTextX = Ftol((double)gApp->mScale * 0.0) - g->StringWidth(aText) / 2 + aX;
		aY += Ftol((double)gApp->mScale * 130.0);
		int aShadowOffset = Ftol((double)gApp->mScale * -2.0);
		// (no colour of its own for the shadow: it takes the cascades' red at full fade)
		g->DrawString(aText, aTextX - aShadowOffset, aY - aShadowOffset);
		g->SetColor(Color(255, 255, 255, anAlpha));
		g->DrawString(aText, aTextX, aY);
	}
}

void Board::DrawNukeGem(Graphics* g, Piece* thePiece)
{
	thePiece->Draw(g, false);
	thePiece->Draw(g, true);
	if (0.0 < mAngryBombFade.GetOutVal()) {
		// an angry bomb over it, at half size
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, Ftol(mAngryBombFade.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
		Transform aTransform;
		aTransform.Scale(0.5f, 0.5f);
		float aY = (float)((double)(gGemHeight / 2) + (double)thePiece->mY);
		float aX = (float)((double)(gGemWidth / 2) + (double)thePiece->mX);
		g->DrawImageTransform(IMAGE_ANGRYBOMB, aTransform, aX, aY);
		g->SetColorizeImages(false);
	}
}
