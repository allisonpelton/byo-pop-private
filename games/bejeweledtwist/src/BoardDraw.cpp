// Sexy::Board's drawing (Board.h): DrawAll, Draw and the board under the gems. Only the 2D paths are ported (as with
// Piece): with Direct3D the original clears the screen first, draws the board through a transform and a 3D back.
#include "Board.h"

#include "EffectsManager.h"
#include "GameText.h"
#include "Graphics.h"
#include "Hyperspace.h"
#include "Piece.h"
#include "Res.h"
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
			// port: the geodes come with the coal that makes them
			NotPorted("Board::Draw: the geodes");
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
		// port: blitz's last seconds' warning (+0x38f0, +0x38f4) comes with blitz
		NotPorted("Board::Draw: blitz's last seconds");
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
	if (!mUnknown149C.empty()) {
		// port: the electro gems' strikes (0x71df1f) come with the electro gems
		NotPorted("Board::DrawStrikes: the electro gems' strikes");
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

void Board::DrawPieces(Graphics* g, Graphics& theLightG, bool is3D)
{
	(void)g;
	(void)theLightG;
	(void)is3D;
	NotPorted("Board::DrawPieces (0x6caa13's gems)");
}

void Board::DrawSelector(Graphics* g, bool theUnknown)
{
	(void)g;
	(void)theUnknown;
	NotPorted("Board::DrawSelector (0x6aa6d1)");
}
