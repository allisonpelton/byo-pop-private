// Sexy::Board's drawing (Board.h): DrawAll, Draw and the board under the gems. Only the 2D paths are ported (as with
// Piece): with Direct3D the original clears the screen first, draws the board through a transform and a 3D back.
#include "Board.h"
#include "ElectroStrike.h"

#include "ColorCycle.h"
#include "EffectsManager.h"
#include "FlyingText.h"
#include "GameOverShow.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "Hyperspace.h"
#include "Piece.h"
#include "Points.h"
#include "Res.h"
#include "SexyMatrix.h"
#include "Shockwave.h"
#include "GemSoul.h"
#include "Traveller.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>

using namespace Sexy;

void Board::DrawAll(ModalFlags* theFlags, Graphics* g)
{
	// Moved or scaled (with Direct3D the scale and the second offset go through the transform at +0x29e8)
	bool aMoved = !(mShakeX == 0.0f && mShakeY == 0.0f && mUnknown268C == 1.0f && mUnknown37EC == 0.0f &&
		mUnknown37F0 == 0.0f);
	if (aMoved)
		g->Translate(Ftol(mShakeX), Ftol(mShakeY));
	if (mUnknown15DC != 0) {
		// Only with Direct3D: an image of the whole board instead
		NotPorted("Board::DrawAll: the board's image (+0x15dc)");
		return;
	}
	// (with Direct3D: the nuke's flash over the board, IMAGE_BOOM_NUKE, while +0x26cc's curve runs)
	if (mHyperspace != nullptr && mHyperspace->mTransition == 11) {
		// port: the jump's tunnel (0x711b65) comes with the jumps
		NotPorted("Board::DrawAll: the jump's tunnel (0x711b65)");
	}
	TwistWidget::DrawAll(theFlags, g);
	if (aMoved)
		g->Translate(-Ftol(mShakeX), -Ftol(mShakeY));
	if (mUnknown1F88.mAlpha != 0) {
		g->SetColor(mUnknown1F88);
		g->FillRect(0, 0, mWidth, mHeight);
	}
}

void Board::Draw(Graphics* g)
{
	bool is3D = mApp->Is3DAccelerated();
	DeferOverlay(99);
	// (with Direct3D, outside the hyperspace and the menus: the screen is cleared to black first)
	int aShakeX = Ftol((double)gSexyAppBase->mWidth * 0.0009765625 * (double)mUnknown1520);
	int aShakeY = Ftol((double)gSexyAppBase->mHeight / 768.0 * (double)mUnknown1524);
	g->Translate(aShakeX, aShakeY);
	// (the original asks for the board's middle here and drops it)
	if (!IsBusy()) {
		if (0.0f < mUnknown21B0) {
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColor(Color(32, 32, 255, Ftol((double)mUnknown21B0 * 255.0)));
			g->FillRect(0, 0, mWidth, mHeight);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			mFXManager3->Draw(g);
		}
	}
	// (when busy, 0x6a4a0d only asks whether the app is 3D accelerated)
	if (!mUnknown2043)
		return;
	if (!is3D)
		DrawBoardBack(g);
	else
		NotPorted("Board::Draw: the 3D board (0x6cc762)");
	if (!mUnknown2043)
		return;
	if (mUnknown1570 != mUnknown1574 || mUnknown1574 == 0) {
		// The geodes (+0x14ac): eight cels each, additive and then normal, in eight colours
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		if (!mUnknown14AC.empty()) {
			// 0x6cc7ed: each geode flying has its eight arms, a dark pass and a coloured one over it
			const Color aArmColors[8] = {Color(255, 128, 64), Color(128, 255, 64), Color(128, 64, 255),
				Color(255, 64, 255), Color(255, 128, 64), Color(64, 255, 128), Color(64, 128, 255), Color(255, 64, 255)};
			for (unsigned int i = 0; i < mUnknown14AC.size(); i++) {
				Geode* aGeode = mUnknown14AC[i];
				if (0 < aGeode->mDelay)
					continue;
				Image* anImage = IMAGE_PHOTONARMS;
				int aHalfWidth = anImage->GetCelWidth() / 2;
				int aHalfHeight = anImage->GetCelHeight() * 0x26 / 0xb7;
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
				for (int j = 0; j < 8; j++) {
					Rect aSrc = anImage->GetCelRect(j / 4 + 2);
					Color aColor(0, 0, 0);
					aColor.mAlpha = Ftol(aGeode->mUnknown128.GetOutVal() * 255.0);
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
					aColor.mAlpha = Ftol(aGeode->mUnknown128.GetOutVal() * 255.0);
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
	if (mUnknown1378 == 1 && !IsBusy() && mApp->mDialogMap.size() == 0 && !mUnknown137F) {
		// A colour cycling through the hues, set and then replaced before anything is drawn with it
		int aLight = Ftol(std::sin((double)mUpdateCnt * 0.03) * -40.0);
		g->SetColor(Color(mApp->HSLToRGB(Ftol((double)mUpdateCnt * 2.5) % 255, 200, 200 - aLight)));
	}
	if (0.0f < mUnknown2150) {
		double anAlpha = (double)mUnknown2150 * 255.0;
		if (255.0 < anAlpha)
			anAlpha = 255.0;
		g->SetColor(Color(255, 255, 255, Ftol(anAlpha)));
		g->FillRect(0, 0, mWidth, mHeight);
	}
	if (mUnknown1384) {
		// The level editor's labels
		if (mUnknown1382) {
			g->SetColor(Color(255, 255, 0));
			g->DrawString(GT(0x7dd728), 18, 600);
		}
		if (mGameMode == MODE_PUZZLE) {
			g->SetColor(Color(100, 100, 255));
			g->DrawString(GT(0x7dd710), 18, 630);
		}
		if (mUnknown1383) {
			g->SetColor(Color(0, 255, 0));
			g->DrawString(GT(0x7dd6fc), 18, 690);
		}
		if (mUnknown137D) {
			g->SetColor(Color(255, 100, 100));
			g->DrawString(GT(0x7dd6e4), 18, 720);
		}
	}
	if (mUnknown1398) {
		mApp->ClearUpdateBacklog(false);
		mUnknown1398 = false;
	}
	if (mUnknown1478) {
		mApp->ClearUpdateBacklog(false);
		mUnknown1478 = false;
	}
	// (with the app's debug flag at +0x4f4, which nothing sets: a readout of the moves and the board's checks in
	// FONT_DEBUG)
	g->Translate(-Ftol((double)gSexyAppBase->mWidth * 0.0009765625 * (double)mUnknown1520),
		-Ftol((double)gSexyAppBase->mHeight / 768.0 * (double)mUnknown1524));
	if (IsBusy())
		return;
	if (0.0f < mUnknown38F8) {
		// The minute mark of blitz's last seconds: the glow, bright over itself, then the front
		g->SetColorizeImages(true);
		int aMidX, aY;
		GetCenter(&aMidX, &aY);
		int aOffset = Ftol((double)gApp->mScale * 3.0);
		aY = Ftol((double)aOffset + (double)mUnknown38F4->mHeight * 0.5);
		g->SetColor(Color(Ftol(255.0 * (double)mUnknown38FC), 0, 0, Ftol((double)mUnknown38F8 * 255.0)));
		DrawImageScaledCentered(g, mUnknown38F4, (float)aMidX, (float)aY, ALIGN_CENTER, 1.0f, 1.0f);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color(Ftol((double)mUnknown38FC * 255.0), 0, 0, Ftol((double)mUnknown38F8 * 32.0)));
		float aLimit = (float)((double)mUnknown38FC + 1.0);
		float aScale = 1.0f;
		while (aScale < aLimit) {
			DrawImageScaledCentered(g, mUnknown38F4, (float)aMidX, (float)aY, aScale, aScale);
			aScale = (float)((double)aScale + 0.019999999552965164);
		}
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColor(Color(255, 255, 255, Ftol((double)mUnknown38F8 * 255.0)));
		g->DrawImage(mUnknown38F0, aMidX - mUnknown38F0->mWidth / 2, aY - mUnknown38F0->mHeight / 2);
	}
	if (mGameMode == MODE_CHALLENGE || mGameMode == MODE_ZEN)
		return;
	if (mUnknown830.GetOutVal() == 0.0 || mUnknown8A0.empty())
		return;
	// The tip, sliding in from the right
	g->SetColor(Color(255, 255, 255, Ftol(mUnknown830.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
	g->SetFont(FONT_TIP);
	int aX = Ftol((double)mWidth * 0.5);
	SexyString aTip = mUnknown8A0;
	Color aGlow = GetFontLayerColor(FONT_TIP, "GLOW");
	SetFontLayerColor(FONT_TIP, "GLOW", Color::Black);
	SetFontLayerColor(FONT_TIP, "MAIN", Color::White);
	g->WriteString(Upper(aTip), aX, Ftol((double)gApp->mScale * 50.0), -1, 0, true, 0, -1, -1);
	SetFontLayerColor(FONT_TIP, "GLOW", aGlow);
}

void Board::DrawBoardBack(Graphics* g)
{
	int anOffset = Ftol(mUnknown13B0.GetOutVal() * (double)gApp->mScale);
	if (mUnknown37C4) {
		g->PushState();
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, Ftol((double)mUnknown391C * 255.0)));
		DrawImageScaledCentered(g, IMAGE_STATIC_SPACE, (float)(mApp->mWidth / 2), (float)(mApp->mHeight / 2), 3, 1.0f,
			1.0f);
		g->PopState();
	}
	// The original makes a board-sized image here (+0x15f4) and clears it whenever +0x15fc asks; nothing draws it
	// or draws into it, so the port leaves it out
	mUnknown15FC = false;
	g->Translate(anOffset * 2, 0);
	if (!mUnknown15B0 && !mUnknown1FF5)
		DrawBoard(g, false, false);
	g->Translate(-anOffset, 0);
}

void Board::DrawStrikes(Graphics* g)
{
	mApp->Is3DAccelerated();
	for (int i = 0; i < (int)mUnknown149C.size(); i++) {
		static_cast<ElectroStrike*>(mUnknown149C[i])->Draw(g);
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
	if (!theToImage && 0.0f < mUnknown3924 && 0.001f < mUnknown20E8)
		DrawGrid(g, anArtScale);
	if (!theSkipPieces)
		DrawPieces(g, aG, is3D);
	if (mUnknown2030) {
		g->SetFont(FONT_DEBUG);
		g->SetColor(Color(255, 255, 255));
		g->DrawString(GT(0x7dd660), 0, 30);
	}
}

// 0x6caa13's first part: the grid's squares, their glow under a gem that waits, the cursor and the bars above and
// below the grid
void Board::DrawGrid(Graphics* g, float theArtScale)
{
	g->SetColorizeImages(mUnknown3924 < 1.0f);
	// (the original takes the sine of the update count's place in a 150-update cycle here and drops it)
	Color aColor(255, 255, 255, Ftol((double)mUnknown3924 * 255.0));
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
						if (aCell.mUnknown3C != 0.0f) {
							float aSin = std::sin((float)((double)aCell.mUnknown38 * 0.07500000298023224 -
								1.5707963705062866));
							aGlow = (float)(((double)aSin + 1.0) * 0.15000000596046448 + 0.30000001192092896) *
								aCell.mUnknown3C;
						}
						if (aCell.mUnknown40 != 0.0f) {
							float aT = aCell.mUnknown40;
							float aScale = 1.0f;
							float aNear = (1.0f - aT) * 4.0f <= aT * 4.0f ? 1.0f - aT : aT;
							if (aNear * 4.0f <= 1.0f)
								aScale = (1.0f - aT) * 4.0f <= aT * 4.0f ? (1.0f - aT) * 4.0f : aT * 4.0f;
							float anAngle = (float)((double)aT * 3.1415927410125732 * 2.0 * 3.0 - 1.5707963705062866);
							double aFlash = ((double)std::sin(anAngle) + 1.0) * 0.10000000149011612 + 0.2;
							if ((double)aGlow <= aFlash * (double)aScale)
								aGlow = (float)(aFlash * (double)aScale);
						}
						if (aPiece->mUnknown4FC == 1) {
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
						if (0.0f < aGlow && mUnknown20D0) {
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
	if (0 <= mUnknown2404) {
		Image* aCursor = IMAGE_CURSOR_BWCURSOR;
		g->DrawImage(aCursor, GetColX(mUnknown2400) + gGemWidth - aCursor->mWidth / 2,
			GetRowY(mUnknown2404) + gGemHeight - aCursor->mHeight / 2);
	}
	// (with a debug flag nothing sets, the board takes the mouse's place from Windows each draw and moves to it)
	if (-1 < mApp->mAutoPlayMode && mUnknown2208 == nullptr)
		mUnknown20D0 = true;
	if (mUnknown20D0 && !mUnknown2030 && (mGameMode != MODE_PUZZLE || !mUnknown1324) &&
		(mUnknown2584 == nullptr || !mUnknown2584->mMouseVisible))
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
	int aX = GetColX(0) + ((gBoard->mCols * gGemWidth) / 2 - mUnknown2590 / 2) + 2;
	g->SetColor(Color(255, 255, 255, Ftol((double)mUnknown3924 * 255.0)));
	g->DrawImage(aBar, aX, aTopY, mUnknown2590, aBarHeight);
	if (0.0f < mUnknown2598) {
		g->SetColor(Color(255, 255, 255, Ftol((double)mUnknown2598 * 255.0 * (double)mUnknown3924)));
		g->DrawImage(IMAGE_INGAMEUI_GRIDBAR_TOPGLOW, aX, aTopY, mUnknown2590, aBarHeight);
	}
	aBar = mGridBarBot;
	g->SetColor(Color(255, 255, 255, Ftol((double)mUnknown259C * 255.0 * (double)mUnknown3924)));
	g->DrawImage(aBar, aX, aBotY, mUnknown2590, aBarHeight);
	if (0.0f < mUnknown2598) {
		g->SetColor(Color(255, 255, 255, Ftol((double)mUnknown2598 * 255.0 * (double)mUnknown3924)));
		g->DrawImage(IMAGE_INGAMEUI_GRIDBAR_BOTGLOW, aX, aBotY, mUnknown2590, aBarHeight);
	}
}

namespace {

// 0x6a5c27: the gems by +0x14, the lower first
int ComparePieceDepth(const void* theA, const void* theB)
{
	Piece* aA = *(Piece* const*)theA;
	Piece* aB = *(Piece* const*)theB;
	if (aA->mUnknown14 < aB->mUnknown14)
		return -1;
	if (aB->mUnknown14 < aA->mUnknown14)
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
	if (IsTimed() && (mUnknown1574 != 0 || mApp->mOptionsDialog != nullptr || mApp->mDialogList.size() != 0)) {
		// A timed game hides its gems while it is paused
		aDrawn = false;
	} else {
		for (int i = 0; i < aCount; i++) {
			if (aPieces[i]->mUnknown2F6)
				aPieces[i]->Draw(g, false);
		}
		mFXManager2->Draw(g);
	}
	if (mGameMode != MODE_PUZZLE && mUnknown20D0 && 0.0f < mUnknown20CC &&
		(mUnknown2584 == nullptr || !mUnknown2584->mMouseVisible))
		DrawSelector(g, true);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	// The light each gem gets on its eight sides: first the glint running across the board
	for (int i = 0; i < aCount; i++) {
		Piece* aPiece = aPieces[i];
		for (int j = 0; j < 8; j++)
			aPiece->mUnknown50[j] = 0;
		aPiece->mUnknown70 = 0;
		if (mUnknown1420) {
			float aPlace = (float)(aPiece->mRow + aPiece->mCol) / (float)(mCols + mRows);
			float aDist = std::fabs(mUnknown1424 - aPlace);
			float aLight = (float)(1.0 - (double)aDist * 9.0);
			if (0.0f < aLight) {
				aPiece->mUnknown50[3] = (float)((double)aLight * 0.800000011920929 + (double)aPiece->mUnknown50[3]);
				double aSide = (double)aLight * 0.6000000238418579;
				aPiece->mUnknown50[7] = (float)((double)aPiece->mUnknown50[7] + aSide);
				aPiece->mUnknown70 = (float)(aSide + (double)aPiece->mUnknown70);
			}
		}
	}
	for (unsigned int i = 0; i < mUnknown1650.size(); i++) {
		Light& aLight = mUnknown1650[i];
		float aX = aLight.mX;
		float aY = aLight.mY;
		if (aLight.mPieceId != -1) {
			Piece* aPiece = GetPieceById(aLight.mPieceId);
			if (aPiece == nullptr)
				continue;
			aX = aPiece->mX + aX;
			aY = aPiece->mY + aY;
		}
		AddLight(aX, aY, aLight.mUnknown8, aLight.mUnknownC, aLight.mUnknown14 * aLight.mUnknown10, aLight.mPiece,
			false);
	}
	for (unsigned int i = 0; i < mUnknown14AC.size(); i++) {
		// each geode flying lights the gems (0x6caa13)
		if (mUnknown14AC[i]->mDelay == 0)
			AddLight(mUnknown14AC[i]->mX, mUnknown14AC[i]->mY, 5.5f, 320.0f, 0.75f, nullptr, false);
	}
	for (int i = 0; i < aCount; i++) {
		Piece* aPiece = aPieces[i];
		if (aPiece->mUnknown38) {
			float aSin = std::sin((float)((double)aPiece->mGlow * 15.0));
			float aGlow = (float)((double)std::fabs(aSin) * 0.6000000238418579);
			aPiece->mUnknown70 = aGlow;
			if (is3D)
				AddLight((float)(gGemWidth / 2) + aPiece->mX, (float)(gGemHeight / 2) + aPiece->mY, 15.0f, 10.0f,
					aGlow, aPiece, false);
		}
		if (aPiece->mUnknownB1 && !aPiece->mDimmed) {
			// A turning gem's light, shared between the two slots its turn lies between, and a little of each to
			// the neighbour on that side
			int aTurn = Ftol(aPiece->mUnknownC0);
			int aNext = (aTurn + 1) % 10;
			float aFrac = (float)((double)aPiece->mUnknownC0 - (double)aTurn);
			double aRest = 1.0 - (double)aFrac;
			float* aSlot = &aPiece->mUnknown50[gTurnSlots[aTurn]];
			*aSlot = (float)((double)(float)aPiece->mUnknownB8 * aRest + (double)*aSlot);
			aSlot = &aPiece->mUnknown50[gTurnSlots[aNext]];
			*aSlot = (float)aPiece->mUnknownB8 * aFrac + *aSlot;
			int k = 0;
			do {
				int aDir = gTurnSlots[k == 0 ? aTurn : aNext];
				float aShare = (float)((k == 0 ? aRest : (double)aFrac) * aPiece->mUnknownB8);
				if (aDir != 9) {
					int aCol = gSlotOffsets[aDir][0] + aPiece->mCol;
					int aRow = gSlotOffsets[aDir][1] + aPiece->mRow;
					if (0 <= aCol && aCol < mCols && 0 <= aRow && aRow < mRows) {
						Piece* aNear = GetPieceAt(aRow, aCol);
						if (aNear != nullptr) {
							float* aNearSlot = &aNear->mUnknown50[(aDir + 4) % 8];
							*aNearSlot = (float)((double)aShare * 0.30000001192092896 + (double)*aNearSlot);
						}
					}
				}
				k++;
			} while (k < 2);
		}
	}
	if (!mUnknown21E4.empty()) {
		// port: the droppers' drawing (0x6f4982) comes with the droppers
		NotPorted("Board::DrawPieces: the droppers");
	}
	// While the cursor is on the board, the gems away from its four are dimmed
	bool aCursor = 0 <= mUnknown2400;
	if (aDrawn) {
		for (int i = 0; i < aCount; i++) {
			Piece* aPiece = aPieces[i];
			if (aCursor) {
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
				g->SetColor(Color(0, 0, 0, 128));
				aPiece->mDimmed = aPiece->mCol < mUnknown2400 || mUnknown2400 + 1 < aPiece->mCol ||
					aPiece->mRow < mUnknown2404 || mUnknown2404 + 1 < aPiece->mRow;
			} else {
				aPiece->mDimmed = false;
			}
			if (aPiece->mUnknown2F6)
				aPiece->Draw(g, true);
		}
	}
	// (the original gathers rectangles over the bombs' cells here (+0x14cc) for Direct3D's heat haze; nothing else
	// uses them)
	mBombList.clear();
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	// (the hyperspace's +0x158, set only with Direct3D, would hide the travellers)
	for (GemSoul* aSoul : mUnknown26B0)
		aSoul->Draw(g);
	for (Shockwave* aWave : mUnknown1660)
		aWave->Draw(g);
	if (GetUnknown23F5())
		DrawTutorialCursor(g);
	if (mGameMode == MODE_PUZZLE && mUnknown20D0 && 0.0f < mUnknown20CC && !mUnknown1324)
		DrawSelector(g, true);
	if (mUnknown21A8 != nullptr)
		mUnknown21A8->Draw(g);
}

void Board::DrawTutorialCursor(Graphics* g)
{
	g->SetColorizeImages(true);
	if (GetUnknown23F5() && -1 < mUnknown2400) {
		// The square of the forced move: "click here" pulsing, fading to "click now"
		int aX = GetColX(mUnknown2400) + gGemWidth;
		int aY = GetRowY(mUnknown2404) + gGemHeight;
		if (0.0f < mUnknown23F8) {
			Image* anImage = IMAGE_CURSOR_CLICKHERE;
			float aPhase = (float)((double)(float)(mUpdateCnt % 150) * 3.141592653589793 / 150.0);
			float aSin = std::sin(aPhase);
			g->SetColor(Color(255, 255, 255, Ftol(((double)aSin * 100.0 + 128.0) * (double)mUnknown23F8)));
			g->DrawImage(anImage, aX - anImage->mWidth / 2, aY - anImage->mHeight / 2);
		}
		if (mUnknown23F8 < 1.0f) {
			Image* anImage = IMAGE_CURSOR_CLICKNOW;
			g->SetColor(Color(255, 255, 255, Ftol((1.0 - (double)mUnknown23F8) * 255.0)));
			g->DrawImage(anImage, aX - anImage->mWidth / 2, aY - anImage->mHeight / 2);
		}
	}
	if (mUnknown20D0 && 0.0f < mUnknown23FC) {
		// Four arrows around the cursor, pointing in and bobbing
		float aCenterX = (float)(mUnknown1C00 + gGemWidth);
		float aRadius = (float)((double)IMAGE_CURSOR_CLICKNOW->mWidth * 0.27);
		const float anAngles[4] = {0.0f, 1.5707964f, 3.1415927f, 4.712389f};
		float aCenterY = (float)(mUnknown1C04 + gGemHeight);
		for (int i = 0; i < 4; i++) {
			float anAngle = anAngles[i];
			float aBob = std::cos((float)((double)mUnknown1C18 * 0.035));
			float aBobScale = (float)((double)gApp->mScale * 36.0);
			float anOffset = -aBob * aBobScale;
			g->SetColor(Color(255, 255, 255, Ftol((double)mUnknown23FC * 255.0)));
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
	bool theSkipFlag4)
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
			if (aPiece->IsFlagSet(4) && theSkipFlag4)
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
					aPiece->mUnknown50[k] = aPiece->mUnknown50[k] + aLight;
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
	if (!((float)GetColX(0) <= mUnknown1C2C) || !((float)GetRowY(0) <= mUnknown1C30)) {
		g->PopState();
		return;
	}
	int aMidY = Ftol((double)gGemHeight + (double)mUnknown1C30);
	int aMidX = Ftol((double)gGemWidth + (double)mUnknown1C2C);
	int aRingX = aMidX - aRingWidth / 2;
	int aRingY = aMidY - aRingHeight / 2;
	if (theRing) {
		if (!mUnknown2030) {
			g->PushState();
			g->SetColorizeImages(true);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			g->SetColor(Color::White);
			g->DrawImage(aRing, aRingX, aRingY, aRingWidth, aRingHeight);
			// The 24 glows round the ring, each placed by its offset in the resources against the ring's
			int aBaseX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(IMAGE_CURSOR_RING_ID));
			int aBaseY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(IMAGE_CURSOR_RING_ID));
			for (int i = 0; i < 24; i++) {
				if (0.0f < mUnknown2058[i]) {
					int anId = IMAGE_CURSOR_YELLOW1_ID + i;
					Image* aGlow = GetImageById(anId);
					float aLevel = mUnknown2058[i] <= 1.0f ? mUnknown2058[i] : 1.0f;
					g->SetColor(Color(255, 255, 255, Ftol((double)aLevel * 255.0 * (double)mUnknown20CC)));
					int aY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(anId));
					int aX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(anId));
					g->DrawImage(aGlow, aX - aBaseX + aRingX, aY - aBaseY + aRingY);
				}
			}
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			g->SetColor(Color::White);
			if (mGameMode != MODE_CHALLENGE && !mUnknown23F5) {
				// The multiplier's bulbs above the ring, lit to the level, and their pulse
				int aCel = mUnknown21D0 - 1;
				Image* aBulbs = IMAGE_CURSOR_MULTIBULBS;
				int aDY = GetImageOffsetY(IMAGE_CURSOR_MULTIBULBS_ID) - GetImageOffsetY(GetIdByImage(aRing));
				int aBulbY = Ftol((double)aDY * (double)anArtScale + (double)(aMidY - aRing->mHeight / 2));
				g->DrawImageCel(aBulbs, aMidX - aBulbs->GetCelWidth() / 2, aBulbY, aCel);
				if (0.0f < mUnknown20B8) {
					float aSize = (float)((1.0 - (double)gBoard->mUnknown20B8) * 3.0 + 1.0);
					g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
					Image* aPulse = IMAGE_CURSOR_MULTIPULSE;
					g->SetColor(Color(255, 255, 255, Ftol((double)mUnknown20B8 * 255.0)));
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
		int aLevel = mUnknown21D0 - 1;
		if (5 < aLevel)
			aLevel = 5;
		Image* aSpiral = GetImageById(IMAGE_CURSOR_SPIRAL1_ID + aLevel);
		if (mUnknown1C41)
			g->SetColor(Color(255, 0, 0, Ftol((double)mUnknown20CC * 200.0 * (double)mUnknown1FAC)));
		else
			g->SetColor(Color(255, 255, 255, Ftol((double)mUnknown20CC * 160.0 * (double)mUnknown1FAC)));
		float anOff = (float)anInset;
		g->DrawImageRotated(aSpiral, Ftol((double)mUnknown1C2C - (double)anOff),
			Ftol((double)mUnknown1C30 - (double)anOff), (double)mUnknown20C0 + (double)mUnknown1FB0);
		if (0.0f < mUnknown20D4) {
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColor(Color(128, 255, 0,
				Ftol((double)mUnknown20CC * 255.0 * (double)mUnknown1FAC * (double)mUnknown20D4)));
			int k = 3;
			do {
				g->DrawImageRotated(aSpiral, Ftol((double)mUnknown1C2C - (double)anOff),
					Ftol((double)mUnknown1C30 - (double)anOff), (double)mUnknown20C0 + (double)mUnknown1FB0);
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
	int aX = mUnknown70C;
	// the tooltip goes to the right of its place, or to the left when the last one's right edge was far out
	bool aLeft = !((double)(mUnknown714 + mUnknown70C) <= (double)mWidth * 0.8999999761581421);
	if (aLeft)
		aX += -mUnknown714 - Ftol((double)gApp->mScale * 120.0);
	else
		aX += Ftol((double)gApp->mScale * 105.0);
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, Ftol(mUnknown788.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
	int anArrowX;
	if (aLeft) {
		anArrowX = mUnknown714 - Ftol(mUnknown718.GetOutVal() * 6.0);
	} else {
		anArrowX = Ftol((double)gApp->mScale * -80.0) - Ftol(mUnknown718.GetOutVal() * 6.0);
	}
	anArrowX += aX;
	int anArrowY;
	if (Ftol((double)gApp->mScale * 20.0) > mUnknown710 - IMAGE_INGAMEUI_TOOLTIPARROW->GetCelHeight() / 2)
		anArrowY = Ftol((double)gApp->mScale * 20.0);
	else
		anArrowY = mUnknown710 - IMAGE_INGAMEUI_TOOLTIPARROW->GetCelHeight() / 2;
	g->DrawImageMirror(IMAGE_INGAMEUI_TOOLTIPARROW, anArrowX, anArrowY, aLeft);
	int aSpacing = Ftol((double)gApp->mScale * 32.0);
	g->SetFont(FONT_TOOLTIP_BOLD);
	int aTitleHeight = g->GetWordWrappedHeight(mUnknown714, mUnknown6D4, aSpacing, nullptr);
	g->SetFont(FONT_TOOLTIP);
	int aTextHeight = g->GetWordWrappedHeight(mUnknown714, mUnknown6F0, aSpacing, nullptr);
	bool aDanger = mUnknown6D4 == mApp->GetString(GTS(0x7dcce4), GT(0x7dccc0)) && mGameMode == MODE_CLASSIC;
	int aHeight = (aTextHeight - Ftol((double)gApp->mScale * -12.0)) + aTitleHeight;
	if (aDanger)
		aHeight += Ftol((double)gApp->mScale * 106.0);
	int aPadX = Ftol((double)gApp->mScale * 36.0);
	int aPadY = Ftol((double)gApp->mScale * 32.0);
	int aTop = mUnknown710 - aHeight / 2;
	if (Ftol((double)gApp->mScale * 40.0) > aTop)
		aTop = Ftol((double)gApp->mScale * 40.0);
	Rect aTextRect(aX, aTop, mUnknown714, aHeight);
	Rect aBoxRect(aX - aPadX, aTop - aPadY, mUnknown714 + aPadX * 2, aHeight + aPadY * 2);
	g->SetColor(Color(255, 255, 255, Ftol(mUnknown788.GetOutVal() * 255.0 * 0.88)));
	g->DrawImageBox(aBoxRect, IMAGE_INGAMEUI_TOOLTIPDIALOG);
	g->SetFont(FONT_TOOLTIP_BOLD);
	g->SetColor(Color(255, 255, 255, Ftol(mUnknown788.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
	g->WriteWordWrapped(aTextRect, mUnknown6D4, aSpacing, 0, nullptr, -1, nullptr);
	if (aDanger) {
		SexyString aLevel = mApp->GetString(GTS(0x7dcc94), GT(0x7dcc68));
		g->WriteString(aLevel, mUnknown714 / 2 + aX, (aTop - Ftol((double)gApp->mScale * 64.0)) + aHeight, -1, 0, true, 0, -1, -1);
	}
	aTextRect.mY += aTitleHeight - Ftol((double)gApp->mScale * -12.0);
	g->SetFont(FONT_TOOLTIP);
	g->SetColor(Color(255, 255, 255, Ftol(mUnknown788.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
	g->WriteWordWrapped(aTextRect, mUnknown6F0, aSpacing, 0, nullptr, -1, nullptr);
	if (aDanger) {
		// the skulls: one more than the danger level, in a row under the text
		g->SetColor(Color::White);
		int aCount = mUnknown21DC + 1;
		int i = 0;
		if (0 < aCount) {
			do {
				int aSkullWidth = IMAGE_SKULLICON_SMALL->mWidth;
				g->DrawImage(IMAGE_SKULLICON_SMALL,
					mUnknown714 / 2 + (aSkullWidth * i - (aSkullWidth * aCount) / 2) + aX,
					(aTop - Ftol((double)gApp->mScale * 64.0)) + aHeight);
				i++;
				aCount = mUnknown21DC + 1;
			} while (i < aCount);
		}
	}
}

void Board::DrawOverlay(Graphics* g)
{
	if (0.0f < mUnknown248) {
		// A fruit's blast darkens the screen (to 0.6 at most: in over its first quarter, out over its last fifth)
		mUnknown340.SetInVal(mUnknown248);
		mUnknown3B0.SetInVal(mUnknown248);
		double anIn = (double)mUnknown248 * 4.0;
		double anOut = (1.0 - (double)mUnknown248) * 5.0;
		double aDark = anIn < anOut ? anIn : anOut;
		if (0.6000000238418579 < aDark)
			aDark = 0.6000000238418579;
		g->SetColor(Color(0, 0, 0, Ftol(aDark * 255.0)));
		g->FillRect(0, 0, mWidth, mHeight);
	}
	g->Translate(Ftol(mShakeX), Ftol(mShakeY));
	if (0.0f < mUnknown248) {
		// and the gems it can reach are drawn over the dark, which the board's own pass left out (UpdateUnknown248)
		int aCount = mCols * mRows;
		if (0 < aCount) {
			Piece** aCell = mGrid;
			do {
				Piece* aPiece = *aCell;
				if (aPiece != nullptr && !aPiece->mUnknown2F6) {
					aPiece->Draw(g, false);
					aPiece->Draw(g, true);
				}
				aCell++;
				aCount--;
			} while (aCount != 0);
		}
	}
	DrawCounters(g);
	g->Translate(-Ftol(mShakeX), -Ftol(mShakeY));
	if (mUnknown36B0 && mGameMode == MODE_BLITZ && 4 < mUnknown26C8) {
		// Blitz's FINAL SCORE (end state 5) and then its score (6) zoom in with a rainbow trail behind them: copies
		// from each one's lowest scale up to its current one, the one at 1 drawn plainly
		float aTo[2] = {1.0f, 1.0f};
		float aFrom[2] = {1.0f, 1.0f};
		if (mUnknown26C8 == 5) {
			aTo[0] = mUnknown3460;
			float aLow = mUnknown3460 - 2.0f;
			if (aLow < 1.0f)
				aLow = 1.0f;
			aFrom[0] = aLow;
			aFrom[1] = -1.0f;
			aTo[1] = -1.0f;
		} else if (mUnknown26C8 == 6) {
			aTo[1] = mUnknown3460;
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
						aColor.mAlpha = Ftol((double)mUnknown346C * 255.0);
					} else {
						g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
						aColor.mAlpha = 16;
					}
					g->SetColor(aColor);
					Image* anImage = i == 0 ? (Image*)mUnknown3464 : (Image*)mUnknown3468;
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
	if (mUnknown6CC && !mApp->mUnknownF85 && !mUnknown37C4 && !mUnknown2629)
		DrawToolTip(g);
	if (mUnknown160 == 1 || mUnknown2629)
		NotPorted("Board::DrawOverlay: the replay's screen");
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	for (Traveller* aTraveller : mUnknown2170)  // 0x6a9b85
		aTraveller->Draw();
	if (mUnknown26CC != nullptr) {
		if (mUnknown160 == 7) {
			// The nuke: the gem that set it off, its nova and the blast. (With Direct3D the board and the tube also
			// zoom about the gem with +0x2908, IMAGE_BOOM_BOARD and IMAGE_BOOM_FTUBE over them.)
			if (!mUnknown26CC->mUnknown2F6)
				DrawNukeGem(g, mUnknown26CC);
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, Ftol(mUnknown2748.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
			Transform aTransform;
			float aScale = (float)mUnknown26D8.GetOutVal();
			aTransform.Scale(aScale, (float)mUnknown26D8.GetOutVal());
			float aY = (float)((double)(gGemHeight / 2) + (double)mUnknown26CC->mY);
			float aX = (float)((double)(gGemWidth / 2) + (double)mUnknown26CC->mX);
			g->DrawImageTransform(IMAGE_BOOM_NOVA, aTransform, aX, aY);
			if (0.0 < mUnknown27B8.GetOutVal()) {
				g->SetColor(Color(255, 255, 255, Ftol(mUnknown2828.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
				aTransform.Reset();
				aScale = (float)mUnknown27B8.GetOutVal();
				aTransform.Scale(aScale, (float)mUnknown27B8.GetOutVal());
				aY = (float)((double)(gGemHeight / 2) + (double)mUnknown26CC->mY);
				aX = (float)((double)(gGemWidth / 2) + (double)mUnknown26CC->mX);
				g->DrawImageTransform(IMAGE_BOOM_NUKE, aTransform, aX, aY);
			}
		}
		g->SetColorizeImages(true);
		for (Shockwave* aWave : mUnknown1660)
			aWave->Draw(g);
	}
	if (mUnknown34F0.GetOutVal() != 0.0 && mUnknown1574 == 0) {
		// "GO", fading in over the curve's first half
		double aFade = mUnknown34F0.GetOutVal() * 2;
		if (aFade > 1.0)
			aFade = 1.0;
		else if (0.0 > aFade)
			aFade = 0.0;
		float anAlpha = (float)(aFade * (double)mUnknown3924);
		float aScale = (float)mUnknown34F0.GetOutVal();
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
	if (mUnknown267C != nullptr) {
		// The fruit dance darkens the screen (to 180) as it stops the game (Update)
		int aDance = mUnknown2680;
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
	// (With Direct3D a device call comes here when +0x268c is not 1.)
	if (0.0f < mUnknown248) {
		// A fruit's blast: its four rings in turning colours (IMAGE_FRUITNOVA), and the fruit's glow growing
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		int aRing = 0;
		do {
			float aT = (float)((double)aRing * -0.15 + (double)mUnknown248 * 2.799999952316284 - 0.5);
			if (aRing == 3)
				aT = (float)(((double)aT - 0.30000001192092896) * 0.8500000238418579);
			mUnknown260.SetInVal(aT);
			mUnknown2D0.SetInVal(aT);
			int aHue = Ftol(mUnknown260.GetOutVal() * 256.0) % 256;
			Color aColor = Color((int)mApp->HSLToRGB(aHue, 255, aRing == 3 ? 200 : 128));
			aColor.mAlpha = Ftol(mUnknown2D0.GetOutVal() * 255.0);
			if (aColor.mAlpha != 0) {
				g->SetColor(aColor);
				int aSize = Ftol(mUnknown260.GetOutVal() * (double)mHeight);
				int aHalf = aSize / 2;
				g->DrawImage(IMAGE_FRUITNOVA, GetColX(mUnknown250) + (gGemWidth / 2 - aHalf),
					GetRowY(mUnknown254) + (gGemHeight / 2 - aHalf), aSize, aSize);
			}
			aRing++;
		} while (aRing < 4);
		g->SetColor(Color(255, 255, 255, Ftol(mUnknown340.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
		double aScale = mUnknown3B0.GetOutVal();
		int aSize = Ftol((double)IMAGE_FRUITBLAST_FRUITGLOWS->GetCelWidth() * aScale);
		int anOffset = (gGemWidth - aSize) / 2;
		g->DrawImageCel(IMAGE_FRUITBLAST_FRUITGLOWS,
			Rect(GetColX(mUnknown250) + anOffset, GetRowY(mUnknown254) + anOffset, aSize, aSize), mUnknown420);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
	if (mUnknown267C != nullptr) {
		g->Translate(mUnknown267C->mX, mUnknown267C->mY);
		mUnknown267C->Draw(g);
		g->Translate(-mUnknown267C->mX, -mUnknown267C->mY);
	}
	mPointsManager->DrawOverlay(g, 0x62);
	// (With Direct3D a device call comes here when +0x268c is not 1.) The banner of a mode just unlocked (0x6b40b5)
	if (mUnknown498.GetOutVal() != 0.0) {
		g->SetColorizeImages(true);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		Image* aModeImage = mUnknown6C8 == MODE_CHALLENGE ? IMAGE_MODEUNLOCK_CHALLENGE : IMAGE_MODEUNLOCK_BLITZ;
		g->SetColor(Color(255, 255, 255, Ftol(mUnknown428.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
		g->DrawImage(IMAGE_MODEUNLOCK_STRIPE, mWidth / 2 - IMAGE_MODEUNLOCK_STRIPE->mWidth / 2,
			Ftol((double)gApp->mScale * 60.0));
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		float aCount = (float)mUpdateCnt;
		int aCloud1 = Ftol((double)aCount * 3.3499999046325684) % IMAGE_MODEUNLOCK_CLOUD1->mWidth;
		int aCloud2 = Ftol((double)aCount * 2.5) % IMAGE_MODEUNLOCK_CLOUD2->mWidth;
		g->SetColor(Color(255, 255, 255, Ftol(mUnknown498.GetOutVal() * 255.0)));
		g->DrawImage(IMAGE_MODEUNLOCK_CLOUD1, mWidth / 2 - IMAGE_MODEUNLOCK_CLOUD1->mWidth / 2 + aCloud1,
			Ftol((double)gApp->mScale * 60.0));
		g->DrawImage(IMAGE_MODEUNLOCK_CLOUD1, mWidth / 2 - (IMAGE_MODEUNLOCK_CLOUD1->mWidth * 3) / 2 + aCloud1,
			Ftol((double)gApp->mScale * 60.0));
		g->DrawImage(IMAGE_MODEUNLOCK_CLOUD2, mWidth / 2 - IMAGE_MODEUNLOCK_CLOUD2->mWidth / 2 - aCloud2,
			Ftol((double)gApp->mScale * 60.0));
		g->DrawImage(IMAGE_MODEUNLOCK_CLOUD2, IMAGE_MODEUNLOCK_CLOUD2->mWidth / 2 + mWidth / 2 - aCloud2,
			Ftol((double)gApp->mScale * 60.0));
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColor(Color(255, 255, 255, Ftol(mUnknown508.GetOutVal() * 255.0)));
		g->DrawImage(aModeImage, mWidth / 2 - aModeImage->mWidth / 2, Ftol((double)gApp->mScale * 100.0));
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color(255, 255, 255, Ftol(mUnknown578.GetOutVal() * 255.0)));
		g->DrawImage(aModeImage, mWidth / 2 - aModeImage->mWidth / 2, Ftol((double)gApp->mScale * 100.0));
		g->SetColor(Color(255, 255, 255, Ftol(mUnknown5E8.GetOutVal() * 255.0)));
		g->DrawImage(IMAGE_MODEUNLOCK_SHEEN, Ftol(mUnknown658.GetOutVal() * (double)mWidth),
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
	if (mUnknown37A4 != 0.0f) {
		Color aColor(96, 96, 255);
		int aY = Ftol((double)GetRowY(3) - (double)gGemHeight * 0.185);
		float aScale = (1.0f - mUnknown37AC) * 2.0f + 2.0f;
		aColor.mAlpha = Ftol((double)mUnknown37AC * 255.0);
		g->SetColor(aColor);
		g->PushState();
		SexyString aText = StrFormat(GTC(0x80da2c), mUnknown36B4);
		Font* aFont = FONT_POPUP_COUNT;
		int aWidth = aFont->StringWidth(aText);
		int anAscent = aFont->GetAscent();
		SetFontLayerColor(aFont, 1, Color(128, 128, 128));
		g->SetFont(aFont);
		g->SetScale(aScale, aScale, (float)aX, (float)aY);
		g->DrawString(aText, aX - aWidth / 2, anAscent / 2 + aY);
		float aScale2 = (float)(mUnknown3730.GetOutVal() * 0.9);
		g->SetScale(aScale2, aScale2, (float)aX, (float)aY);
		aShadow.mAlpha = Ftol((double)mUnknown37A4 * 128.0);
		g->SetColor(aShadow);
		int anOffset = Ftol((double)gApp->mScale * -8.0);
		g->DrawString(aText, aX - aWidth / 2 - anOffset, anAscent / 2 - anOffset + aY);
		int anAlpha = Ftol((double)mUnknown37A4 * 255.0);
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
		int anOffset2 = Ftol((double)gApp->mScale * -2.0);
		g->DrawString(aText, aTextX - anOffset2, aY - anOffset2);
		g->SetColor(Color(255, 255, 255, anAlpha));
		g->DrawString(aText, aTextX, aY);
	}
	if (0.0f < mUnknown37A0) {
		int aY = Ftol((double)GetRowY(4) + (double)gGemHeight * 0.6);
		Color aColor(255, 64, 64);
		float aScale = (1.0f - mUnknown37A8) * 0.9f + 1.0f;
		aColor.mAlpha = Ftol((double)mUnknown37A8 * 255.0);
		SexyString aText = StrFormat(GTC(0x80da2c), mUnknown36B8);
		Font* aFont = FONT_POPUP_COUNT;
		int aWidth = aFont->StringWidth(aText);
		int anAscent = aFont->GetAscent();
		SetFontLayerColor(aFont, 1, Color(128, 128, 128));
		g->PushState();
		g->SetFont(aFont);
		g->SetColor(aColor);
		g->SetScale(aScale, aScale, (float)aX, (float)aY);
		g->DrawString(aText, aX - aWidth / 2, anAscent / 2 + aY);
		float aScale2 = (float)mUnknown36C0.GetOutVal();
		g->SetScale(aScale2, aScale2, (float)aX, (float)aY);
		aShadow.mAlpha = Ftol((double)mUnknown37A0 * 128.0);
		g->SetColor(aShadow);
		int anOffset = Ftol((double)gApp->mScale * -8.0);
		g->DrawString(aText, aX - aWidth / 2 - anOffset, anAscent / 2 - anOffset + aY);
		int anAlpha = Ftol((double)mUnknown37A0 * 255.0);
		aColor.mAlpha = anAlpha;
		g->SetColor(aColor);
		g->DrawString(aText, aX - aWidth / 2, anAscent / 2 + aY);
		g->PopState();
		g->SetFont(FONT_RANK_DIALOG);
		SetFontLayerColor(g->GetFont(), 0, Color::White);
		aText = mApp->GetString(GTS(0x7dcc04));
		int aTextX = Ftol((double)gApp->mScale * 0.0) - g->StringWidth(aText) / 2 + aX;
		aY += Ftol((double)gApp->mScale * 130.0);
		int anOffset2 = Ftol((double)gApp->mScale * -2.0);
		// (no colour of its own for the shadow: it takes the cascades' red at full fade)
		g->DrawString(aText, aTextX - anOffset2, aY - anOffset2);
		g->SetColor(Color(255, 255, 255, anAlpha));
		g->DrawString(aText, aTextX, aY);
	}
}

void Board::DrawNukeGem(Graphics* g, Piece* thePiece)
{
	thePiece->Draw(g, false);
	thePiece->Draw(g, true);
	if (0.0 < mUnknown2898.GetOutVal()) {
		// an angry bomb over it, at half size
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, Ftol(mUnknown2898.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
		Transform aTransform;
		aTransform.Scale(0.5f, 0.5f);
		float aY = (float)((double)(gGemHeight / 2) + (double)thePiece->mY);
		float aX = (float)((double)(gGemWidth / 2) + (double)thePiece->mX);
		g->DrawImageTransform(IMAGE_ANGRYBOMB, aTransform, aX, aY);
		g->SetColorizeImages(false);
	}
}
