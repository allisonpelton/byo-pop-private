// The Classic board's drawing (ClassicBoard::Draw 0x73b250 (bejeweled3): the panel 0x751d50, then 0x750320 with the screen's
// pictures, the gems by slot 237 (0x757df0) and the HUD). docs/NOTES.md ("The Classic screen's drawing") says what is
// translated and what is not. Every measure of the exe is in 1200ths of the art height in use: Scaled() is its
// "art size * measure / 1200".
#include "Board.h"
#include "Bej3Dialog.h"
#include "HintDialog.h"
#include "Profile.h"
#include "OptionsDialog.h"

#include "Bej3App.h"
#include "Color.h"
#include "Common.h"
#include "CrystalBall.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "PIEffect.h"
#include "Res.h"
#include "X86Math.h"

#include <cmath>

namespace Sexy {

namespace {

// 0x401240 (bejeweled3): art size x measure / 1200, the exe's signed division
int Scaled(int theMeasure)
{
	return gApp->mArtRes * theMeasure / 1200;
}

// slot 172's neighbour, 0x73b6c0 (bejeweled3): the middle of the left panel; the pictures' x= in resources.xml are 400 to the right of
// where the panel's pictures go (0x741840 takes 400 off the screen's x)
const int PANEL_MIDDLE_X = 0xfa;
const int PANEL_PICTURE_SHIFT = -400;

// 0x713850 (bejeweled3) at its scale of 1: the picture is drawn with its middle at the point (the point in pixels)
void DrawImageCentered(Graphics* g, Image* theImage, float theX, float theY)
{
	if (theImage == nullptr)
		return;
	int aX = Ftol((double)(theX - (float)theImage->GetWidth() * 0.5f));
	int aY = Ftol((double)(theY - (float)theImage->GetHeight() * 0.5f));
	g->DrawImage(theImage, aX, aY);
}

// A picture drawn at where resources.xml puts it (its x= and y=), the left panel's 400 off: 0x741440, 0x7423e0
void DrawPanelPicture(Graphics* g, Image* theImage, int theImageId)
{
	if (theImage == nullptr)
		return;
	g->DrawImage(theImage, Scaled(PANEL_MIDDLE_X + PANEL_PICTURE_SHIFT + GetImageOffsetX(theImageId)),
		Scaled(GetImageOffsetY(theImageId)));
}

}  // namespace

// 0x76bfc0 (bejeweled3): for each colour 15 sprites of its gem, each the colour's cel of IMAGE_GEMS_SHADOWED stretched to the size that
// frame k stands for: a scale of 2 - (k + 1) / 8, which above 1 is halved in its excess (so frame 0 is 1.4375 times the gem), in a
// square of the gem's size, or for the larger ones 1.5 times it. The exe makes only frame 7 when 0x46f830 says 3D is on; the 2D
// mode this port runs makes all 15
void Bej3App::MakeGemFrames()
{
	if (IMAGE_GEMS_SHADOWED == nullptr)
		return;
	for (int aColor = 0; aColor < 7; aColor++) {
		for (int aFrame = 0; aFrame <= 14; aFrame++) {
			double aScale = 2.0 - ((double)aFrame + 1.0) * 0.0625 * 2.0;
			if (1.0 < aScale)
				aScale = (aScale - 1.0) * 0.5 + 1.0;
			int aGemSize = (mArtRes << 7) / 1200;
			int aSize = aScale <= 1.0010000467300415 ? aGemSize : Ftol((double)aGemSize * 1.5);
			MemoryImage* anImage = new MemoryImage();
			anImage->Create(aSize, aSize);
			anImage->SetImageMode(true, true);
			int aDrawSize = Ftol((double)aGemSize * aScale) | 1;
			Graphics g(anImage);
			Rect aDest((anImage->mWidth - aDrawSize) / 2, (anImage->mHeight - aDrawSize) / 2, aDrawSize, aDrawSize);
			g.DrawImageCel(IMAGE_GEMS_SHADOWED, aDest, aColor);
			mGemFrames[aColor][aFrame] = anImage;
		}
	}
}

// slot 237, 0x757df0 (bejeweled3), the 2D path. A gem that is not held by a swap, not turning and has no flag that makes it special
// (0x73e330: flags 1, 4, 0x10, 0x800 and 0x1000 off) is the sprite of its size (frame (2 - x) * 8 - 1 of its colour's row of the table
// at app +0xb5c, x being the scale curve's value, or 2s - 1 for a scale above 1) at its place, the sprite's middle on the cell's.
// Any other gem is a cel of its colour's IMAGE_GEMS_* sheet, 20 x its turn (+0x170) of them, at its place.
// port: not translated: the 3D path (the exe takes frame 7 and the Graphics' own scale), the blend of the two cels round a turn
// (a PIEffect "Params" when the board's alpha is 1), the Flame Gem's, the Star Gem's and the Hypercube's pictures and PIEffects
// (0x745a60, 0x755030, 0x754c60, the bits 4 and 0x2000 at the end), the selection and hint glows (+0x240 and bit 0x200), and the
// piece's own alpha curve (+0x90, the constant 1)
void Board::DrawPiece(Graphics* g, const Piece* thePiece, float theScaleFactor)
{
	static Image** const aSheets[Piece::COLOR_COUNT] = {&IMAGE_GEMS_RED, &IMAGE_GEMS_WHITE, &IMAGE_GEMS_GREEN, &IMAGE_GEMS_YELLOW,
		&IMAGE_GEMS_PURPLE, &IMAGE_GEMS_ORANGE, &IMAGE_GEMS_BLUE};
	int aColor = thePiece->mColor;
	if ((aColor < 0 || aColor >= Piece::COLOR_COUNT) && (thePiece->mFlags & Piece::FLAG_HYPERCUBE) == 0)
		return;
	// slot 175, 0x73bd10: the board's alpha, the cover curve (+0x2898) times 1 less the pause's fade (+0x12c8)
	double anAlpha = GetBoardAlpha();
	if ((float)anAlpha == 0.0f)
		return;
	Piece* aPiece = const_cast<Piece*>(thePiece);
	float aScale = (float)aPiece->mScaleCurve.GetOutVal();
	int aX = Ftol((double)((float)GetGridX() + thePiece->mX));
	int aY = Ftol((double)((float)GetGridY() + thePiece->mY));
	int aPixelX = Scaled(aX);
	int aPixelY = Scaled(aY);

	g->PushState();
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, Ftol(anAlpha * 255.0)));
	int aFlags = thePiece->mFlags;
	if ((aFlags & Piece::FLAG_HYPERCUBE) != 0) {
		// 0x757df0: bit 2 is slot 235 (0x73f630), under the same scale about the cell's middle as any gem drawn from a sheet
		float aCubeScale = aScale * theScaleFactor;
		if (aCubeScale != 1.0f)
			g->SetScale(aCubeScale, aCubeScale, (float)Scaled(aX + 0x40), (float)Scaled(aY + 0x40));
		DrawHypercube(g, thePiece);
		g->PopState();
		return;
	}
	bool aPlain = !IsHeldBySwap(thePiece) && thePiece->mSpinPhase == 0.0f && (aFlags & 0x15) == 0 && (aFlags & 0x1800) == 0;
	if (aPlain) {
		float aX2 = aScale;
		if (!(1.0f >= aScale))
			aX2 = (float)(((double)aScale - 1.0) + ((double)aScale - 1.0) + 1.0);
		int aFrame = Ftol((((double)2.0 - (double)aX2) * 16.0 * 0.5) - 1.0);
		if (aFrame >= 14)
			aFrame = 14;
		else if (aFrame < 0)
			aFrame = 0;
		MemoryImage* aSprite = gApp->mGemFrames[aColor][aFrame];
		if (aSprite != nullptr) {
			int aCell = Scaled(0x80);
			g->DrawImage(aSprite, aPixelX - (aSprite->mWidth - aCell) / 2, aPixelY - (aSprite->mHeight - aCell) / 2);
		}
	}
	else {
		Image* aSheet = *aSheets[aColor];
		// 0x757df0: the whole drawing is scaled round the middle of the cell by the scale curve (+0x20) times the third argument; the
		// sprites above have the scale in their size and the exe puts it back to 1 for them (0x434370 with 1, 1, 0, 0)
		float aWholeScale = aScale * theScaleFactor;
		if (aWholeScale != 1.0f)
			g->SetScale(aWholeScale, aWholeScale, (float)Scaled(aX + 0x40), (float)Scaled(aY + 0x40));
		if (aSheet != nullptr) {
			int aCels = aSheet->mNumCols * aSheet->mNumRows;
			float aTurn = (float)aCels * thePiece->mSpinPhase;
			int aCel = Ftol((double)aTurn);
			g->DrawImageCel(aSheet, aPixelX, aPixelY, aCel);
		}
	}
	g->PopState();
}

// slot 235, 0x73f630 (bejeweled3): the Hypercube. The picture IMAGE_HYPERCUBE_FRAME (60 cels, 10 by 6) and then IMAGE_HYPERCUBE_COLORGLOW
// drawn additively, both at the cel (the app's update count / 5) mod 60, with the colour white at the board's alpha. The frame's
// corner is the cell's less 16 (in 1200ths) plus the picture's own x= and y= (23 and 20 in resources.xml); the glow, which has
// none, at the cell's less 16. port: the second cel of the exe's additive draw is read as the same cel; which of the two pictures
// the exe's globals 0x8d8b3c and the one after it are was read from the offsets' order (0x8da700 non-null: the frame)
void Board::DrawHypercube(Graphics* g, const Piece* thePiece)
{
	g->SetColor(Color(255, 255, 255, Ftol(mCoverCurve.GetOutVal() * 255.0)));
	int aCel = (gApp->mUpdateCount / 5) % 60;
	float aGridX = (float)GetGridX() + thePiece->mX - 16.0f;
	float aGridY = (float)GetGridY() + thePiece->mY - 16.0f;
	if (IMAGE_HYPERCUBE_FRAME != nullptr) {
		int aX = Ftol((double)(aGridX * (float)gApp->mArtRes / 1200.0f) + (double)(gApp->mArtRes * GetImageOffsetX(IMAGE_HYPERCUBE_FRAME_ID) / 1200));
		int aY = Ftol((double)(aGridY * (float)gApp->mArtRes / 1200.0f) + (double)(gApp->mArtRes * GetImageOffsetY(IMAGE_HYPERCUBE_FRAME_ID) / 1200));
		g->DrawImageCel(IMAGE_HYPERCUBE_FRAME, aX, aY, aCel);
	}
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	if (IMAGE_HYPERCUBE_COLORGLOW != nullptr) {
		int aX = Ftol((double)(aGridX * (float)gApp->mArtRes / 1200.0f));
		int aY = Ftol((double)(aGridY * (float)gApp->mArtRes / 1200.0f));
		g->DrawImageCel(IMAGE_HYPERCUBE_COLORGLOW, aX, aY, aCel);
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// slot 236, 0x754670 (bejeweled3): the shadow of a gem that is drawn from its sheet (held by a swap, turning, or special): the cel of
// IMAGE_GEMSSHADOW_<colour> (the sheet's 20 cels, the same as the gem's) that matches how far the gem has turned, at the gem's place
// (the scale curve scales it round the middle of the cell, as the gem). The exe draws it for a gem with a colour (0 to 6) or bit 4,
// and not for bit 0x80 or bit 0x10 (which takes another cel count; neither is in Classic). port: the piece's alpha curve (+0x90, the
// constant 1) and the shake offsets (+0x258 and +0x25c, 0) are left out
void Board::DrawPieceShadow(Graphics* g, const Piece* thePiece)
{
	static Image** const aShadows[Piece::COLOR_COUNT] = {&IMAGE_GEMSSHADOW_RED, &IMAGE_GEMSSHADOW_WHITE, &IMAGE_GEMSSHADOW_GREEN,
		&IMAGE_GEMSSHADOW_YELLOW, &IMAGE_GEMSSHADOW_PURPLE, &IMAGE_GEMSSHADOW_ORANGE, &IMAGE_GEMSSHADOW_BLUE};
	if ((thePiece->mFlags & 0x80) != 0 || thePiece->mColor < 0 || thePiece->mColor >= Piece::COLOR_COUNT)
		return;
	Image* aShadow = *aShadows[thePiece->mColor];
	if (aShadow == nullptr)
		return;
	Piece* aPiece = const_cast<Piece*>(thePiece);
	float aScale = (float)aPiece->mScaleCurve.GetOutVal();
	int aX = Ftol((double)((float)GetGridX() + thePiece->mX));
	int aY = Ftol((double)((float)GetGridY() + thePiece->mY));
	g->PushState();
	if (aScale != 1.0f)
		g->SetScale(aScale, aScale, (float)Scaled(aX + 0x40), (float)Scaled(aY + 0x40));
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, Ftol(mCoverCurve.GetOutVal() * 255.0)));
	int aCels = aShadow->mNumCols * aShadow->mNumRows;
	float aTurn = (float)aCels * thePiece->mSpinPhase;
	int aCel = Ftol((double)((double)(aCels - 1) <= (double)aTurn ? (float)(aCels - 1) : aTurn));
	g->DrawImageCel(aShadow, Scaled(aX), Scaled(aY), aCel);
	g->PopState();
}

// slot 246, 0x7568a0 (bejeweled3) with its second argument 0. Three passes: (1) the shadow of every gem that is not a Flame Gem and is
// held by a swap, turning (+0x170 not 0) or has the bits 1, 4, 0x10 (0x15) or 0x800 and 0x1000; then the effects' layer at +0x12bc
// (EffectsManager::Draw: the Star Gem's and Hypercube's effects and the flame gem's creation animation under the gems);
// (2) every gem that no swap holds, by slot 237; (3) each swap's gems: the second one (when the swap is not over and the second gem
// exists) and the first, each with a scale factor of 1 plus or minus the swap's lift, which is the constant 0 in the 2D mode (the exe's
// curve at +0x80 of the swap is set only in 3D, 0x75f8fc), and so 1; the order follows the swap's angle (+0x10), which is 0 in 2D too:
// the second gem first. port: left out: the exe's 3D-only gem shine (0x74f130) and the cursor at +0x2c44 (3D). The piece at +0x2ef8 (a piece pointer in the exe:
// a gem the fill holds, which pass 2 skips) is none in Classic
void Board::DrawPieces(Graphics* g)
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			const Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece == nullptr || (aPiece->mFlags & Piece::FLAG_FLAME) != 0)
				continue;
			if (IsHeldBySwap(aPiece) || aPiece->mSpinPhase != 0.0f || (aPiece->mFlags & 0x15) != 0 || (aPiece->mFlags & 0x1800) != 0)
				DrawPieceShadow(g, aPiece);
		}
	}
	g->PushState();
	mEffectsUnder->Draw(g, false);   // 0x7568a0: slot 26 of +0x12bc between PushState (0x432410) and PopState (0x432520)
	g->PopState();
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			const Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece != nullptr && !IsHeldBySwap(aPiece))
				DrawPiece(g, aPiece, 1.0f);
		}
	}
	for (size_t aSwap = 0; aSwap < mSwaps.size(); aSwap++) {
		const SwapRecord& aRecord = mSwaps[aSwap];
		const float aLift = 0.0f;
		if (!aRecord.mDone && aRecord.mSecond != nullptr)
			DrawPiece(g, aRecord.mSecond, 1.0f - aLift);
		DrawPiece(g, aRecord.mFirst, aLift + 1.0f);
	}
}

// 0x751d50 (bejeweled3): the 8 by 8 squares of the grid, every other one in each of two colours (the board's +0xd2c, (6, 6, 6, 160), and
// +0xd3c, (24, 24, 24, 160), made by 0x74a880), each lit towards a glow colour by the level's flash (+0x1188: the colour moves from the
// board's by 0x712c90, a plain blend, to a glow made of +0x1188 and the board's alpha, by min(1, 5 x the flash)). The even squares (column
// + row) are the first colour. port: not translated: the slide of the whole panel by the curves at +0x2980 and +0x2a60 (the board's
// intro and leaving; both at their rest value here), the picture drawn while +0x2bb8 is not 0, the y shift +0x1194 (0), the glow
// drawn while +0x27d0 is above 0 (a ring flash, drawn only) and the child drawn by slot 0x3e8 (the HUD, in DrawScreenFrame)
void Board::DrawPanel(Graphics* g)
{
	g->SetColorizeImages(true);
	double anAlpha = mCoverCurve.GetOutVal();
	float aFlash = mCheckerFlash;
	float aBlend = (float)((double)aFlash * 5.0);
	if (aBlend > 1.0f)
		aBlend = 1.0f;
	// the glow colours (0x751ef3 to 0x752073): r 180, g 14u + 100, b 8u + 48 for the first, r 160, g 12u + 90, b 7u + 40 for the second, u being
	// 60 x the flash, and 200 x the board's alpha for both
	float aGlowAmount = (float)((double)aFlash * 60.0);
	int aGlowAlpha = Ftol(anAlpha * 200.0);
	Color aGlow1(180, Ftol((double)aGlowAmount * 14.0 + 100.0), Ftol((double)aGlowAmount * 8.0 + 48.0), aGlowAlpha);
	Color aGlow2(160, Ftol((double)aGlowAmount * 12.0 + 90.0), Ftol((double)aGlowAmount * 7.0 + 40.0), aGlowAlpha);
	// the board's two colours, 0x74a880
	const Color aBoardColor1(6, 6, 6, 160);
	const Color aBoardColor2(24, 24, 24, 160);
	auto aLerp = [](const Color& theFrom, const Color& theTo, float theT) {
		// 0x712c90: from + (to - from) * t for each part, cut to a whole number
		return Color(Ftol((double)theFrom.mRed + (double)(theTo.mRed - theFrom.mRed) * (double)theT),
			Ftol((double)theFrom.mGreen + (double)(theTo.mGreen - theFrom.mGreen) * (double)theT),
			Ftol((double)theFrom.mBlue + (double)(theTo.mBlue - theFrom.mBlue) * (double)theT),
			Ftol((double)theFrom.mAlpha + (double)(theTo.mAlpha - theFrom.mAlpha) * (double)theT));
	};
	Color aColorEven = aLerp(aBoardColor1, aGlow1, aBlend);
	Color aColorOdd = aLerp(aBoardColor2, aGlow2, aBlend);

	int aColumnEdge[COLS + 1];
	for (int aEdge = 0; aEdge <= COLS; aEdge++)
		aColumnEdge[aEdge] = gApp->mArtRes * (GetGridX() + aEdge * CELL) / 1200;
	g->PushState();
	for (int aRow = 0; aRow < ROWS; aRow++) {
		int aTop = gApp->mArtRes * (GetGridY() + aRow * CELL) / 1200;
		int aHeight = gApp->mArtRes * (GetGridY() + CELL + aRow * CELL) / 1200 - aTop;
		for (int aCol = 0; aCol < COLS; aCol++) {
			g->SetColor(((aCol + aRow) & 1) == 0 ? aColorEven : aColorOdd);
			g->FillRect(aColumnEdge[aCol], aTop, aColumnEdge[aCol + 1] - aColumnEdge[aCol], aHeight);
		}
	}
	g->PopState();
}

// slot 249 (0x3e4), 0x7423e0 (bejeweled3): the top widget at its place and the level number over it
void Board::DrawTopWidget(Graphics* g)
{
	DrawPanelPicture(g, IMAGE_INGAMEUI_TOP_WIDGET, IMAGE_INGAMEUI_TOP_WIDGET_ID);
	if (ShowsLevelNumber()) {
		g->SetFont(FONT_LEVEL);
		Color anOld;
		bool aSet = SetLayerColor(FONT_LEVEL, "GLOW", Color(128, 128, 128, 255), &anOld);
		g->SetColor(Color(255, 255, 255, 255));
		g->WriteString(CommaSeperate(mLevelIndex + 1), Scaled(PANEL_MIDDLE_X), Scaled(0xfa), -1, 0, true, 0, -1, -1);
		if (aSet)
			RestoreLayerColor(FONT_LEVEL, "GLOW", anOld);
	}
}

// slot 255 (0x3fc), 0x746f20 (bejeweled3): the score
void Board::DrawScoreText(Graphics* g)
{
	g->SetFont(FONT_SCORE);
	g->SetColor(Color(255, 255, 255, 255));
	g->WriteString(CommaSeperate(mScore), Scaled(250), Scaled(202), 0, 0, true, 0, -1, -1);
}

// slot 251 (0x3ec), 0x741440 (bejeweled3): the bottom widget of the mode (Classic)
void Board::DrawBottomWidget(Graphics* g)
{
	DrawPanelPicture(g, IMAGE_INGAMEUI_BOTTOM_WIDGET_CLASSIC, IMAGE_INGAMEUI_BOTTOM_WIDGET_CLASSIC_ID);
}

// The pictures of the screen round the grid, in the order the exe draws them. The left panel's two pictures are placed by their x= and y= in
// resources.xml with the panel's 400 taken off (0x741440, 0x7423e0); the frame above the grid (0x745880) and the bar's frame and back below
// it (0x745430, 0x7540c0) have their middle at the grid's middle (x + 0x200) and at y - 5, + 0x421 and + 0x41e.
// port: the level bar's fill (the exe draws PIEFFECT_LEVELBAR, 0x575650, and its look is set by 0x75a760 and 0x755620), the glows of
// the frames (while the bar's stage is on, 0x368), the pictures' draws under the board's alpha curves and the slide of the panel
void Board::DrawScreenFrame(Graphics* g)
{
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, Ftol(mCoverCurve.GetOutVal() * 255.0)));
	DrawTopWidget(g);
	DrawScoreText(g);
	DrawBottomWidget(g);
	g->SetColor(Color(255, 255, 255, Ftol(mCoverCurve.GetOutVal() * 255.0)));
	// 0x745880: the frame over the grid; 0x7540c0 and 0x745430: the bar's back and frame under it
	DrawImageCentered(g, IMAGE_INGAMEUI_TOP_FRAME, (float)Scaled(GetGridX() + 0x200), (float)Scaled(GetGridY() - 5));
	DrawImageCentered(g, IMAGE_INGAMEUI_BOTTOM_FRAME_BACK, (float)Scaled(GetGridX() + 0x200), (float)Scaled(GetGridY() + 0x41e));
	if (mLevelBarEffect != nullptr && IMAGE_INGAMEUI_BOTTOM_FRAME_BACK != nullptr) {
		// 0x73d5e0: the bar's rectangle, the back's size with its middle on the grid's (x + 0x200, y + 0x41e)
		int aBarW = IMAGE_INGAMEUI_BOTTOM_FRAME_BACK->GetWidth();
		int aBarH = IMAGE_INGAMEUI_BOTTOM_FRAME_BACK->GetHeight();
		int aBarX = Scaled(GetGridX() + 0x200) - aBarW / 2;
		int aBarY = Scaled(GetGridY() + 0x41e) - aBarH / 2;
		float aScale = (float)gApp->mArtRes / 1200.0f;
		mLevelBarEffect->mDrawTransform.LoadIdentity();
		mLevelBarEffect->mDrawTransform.m00 = aScale;
		mLevelBarEffect->mDrawTransform.m11 = aScale;
		mLevelBarEffect->mDrawTransform.m02 = (float)aBarX;
		mLevelBarEffect->mDrawTransform.m12 = (float)aBarY;
		g->PushState();
		g->ClipRect(aBarX, aBarY, Ftol((double)((float)aBarW * mLevelBar)), aBarH);
		mLevelBarEffect->Draw(g);
		g->PopState();
	}
	DrawImageCentered(g, IMAGE_INGAMEUI_THIN_BOTTOM_FRAME, (float)Scaled(GetGridX() + 0x200), (float)Scaled(GetGridY() + 0x421));
}

// 0x73b250 (bejeweled3): the panel, then the rest of the screen (0x750320), the gems last of it
void Board::Draw(Graphics* g)
{
	DrawPanel(g);
	DrawScreenFrame(g);
	DrawBehindPieces(g);
	DrawPieces(g);   // slot 246, 0x7568a0
	DrawPausedWord(g);   // 0x756cb0, after the gems and the bars and before the overlays
	mEffectsOver->Draw(g, false);   // the +0x12c0 layer is a child of the board: its draw (slot 26) comes after the board's
	DrawModeOverlay(g);   // port: the exe's widget manager calls slot 43 in the overlay pass; here it follows the board's own draw
}

// 0x756cb0 (bejeweled3), the part of Board's draw that writes PAUSED: in a mode that can be paused (slot 107) while the word's alpha (+0x12cc) is above 0, and
// not while the screen's cover is whole and the game's end has begun (the exe's block is for a cover below 1 or no end hold, +0xd1c below 1). White at that alpha, in
// the font of resource 17 (FONT_HUGE), the text of string 0x93 centred on the board's middle (the grid's x and 0x200, at the art size over 1200), its baseline at 0x21c
void Board::DrawPausedWord(Graphics* g)
{
	if (!(mCoverCurve.GetOutVal() < 1.0 || mGameOverTicks < 1))
		return;
	if (!CanPause() || !(mPauseAlpha > 0.0f) || FONT_HUGE == nullptr)
		return;
	g->PushState();
	g->SetFont(FONT_HUGE);
	g->SetColor(Color(255, 255, 255, Ftol((double)mPauseAlpha * 255.0)));
	const SexyString aText = gApp->GetString(std::to_string(0x93), "PAUSED");
	const int aBaseline = Scaled(0x21c);
	g->DrawString(aText, Scaled(GetGridX() + 0x200) - FONT_HUGE->StringWidth(aText) / 2, aBaseline);
	g->PopState();
}

// 0x741840 (bejeweled3), Classic's case: the Hint button (id 0, the Classic hint picture) and the Menu button (id 1, the Classic menu
// picture), each where resources.xml puts its picture with the panel's 400 off, the size of a cel (the pictures are two cels wide:
// cel 0 at rest, cel 1 over and pressed). The exe's class is Bej3Button (vtable 0x86498c, ctor 0x49c490); its hover curves (+0x150
// and +0x158) are not translated
void Board::MakeClassicButtons()
{
	MakeButtons(IMAGE_INGAMEUI_HINT_BUTTON_CLASSIC, IMAGE_INGAMEUI_HINT_BUTTON_CLASSIC_ID, IMAGE_INGAMEUI_MENU_BUTTON_CLASSIC,
		IMAGE_INGAMEUI_MENU_BUTTON_CLASSIC_ID);
}

void Board::MakeButtons(Image* theHintImage, int theHintImageId, Image* theMenuImage, int theMenuImageId, Image* theResetImage,
	int theResetImageId)
{
	struct ButtonSpec {
		int mId;
		Image* mImage;
		int mImageId;
		ButtonWidget** mTarget;
	};
	const ButtonSpec aSpecs[] = {
		{BUTTON_HINT, theHintImage, theHintImageId, &mHintButton},
		{BUTTON_MENU, theMenuImage, theMenuImageId, &mMenuButton},
		{BUTTON_RESET, theResetImage, theResetImageId, &mResetButton},
	};
	for (const ButtonSpec& aSpec : aSpecs) {
		if (aSpec.mImage == nullptr)
			continue;
		ButtonWidget* aButton = new ButtonWidget(aSpec.mId, this);
		aButton->Resize(Scaled(GetImageOffsetX(aSpec.mImageId) + PANEL_MIDDLE_X + PANEL_PICTURE_SHIFT),
			Scaled(GetImageOffsetY(aSpec.mImageId)), aSpec.mImage->GetCelWidth(), aSpec.mImage->GetCelHeight());
		aButton->mButtonImage = aSpec.mImage;
		aButton->mNormalRect = aSpec.mImage->GetCelRect(0);
		aButton->mOverRect = aSpec.mImage->GetCelRect(1);
		aButton->mDownRect = aSpec.mImage->GetCelRect(1);
		aButton->mHasTransparencies = true;   // +0x94
		*aSpec.mTarget = aButton;
		AddWidget(aButton);
	}
}

// 0x74bc40 (bejeweled3): the button's id. The exe does nothing while a dialog or the game's end holds the board (slot 207, 0x73b640:
// +0xd20 and +0xd1c below 1; +0xd20 is not translated). The click sound (0x76aaf0) is left out
void Board::ButtonDepress(int theId)
{
	if (mGameOverTicks >= 1)
		return;
	switch (theId) {
	case BUTTON_HINT:
		ShowHint(true);   // slot 202 (0x75aeb0), when +0x120c (a fade) is 0
		break;
	case BUTTON_MENU:
	{
		// 0x74bc40 case 1: the options dialog (0x681370 with 1) is added as dialog 0x15 and put 512 right of the grid's middle
		// (1200ths) less half its width, 200 down
		OptionsDialog* aDialog = new OptionsDialog(true);
		gApp->AddDialog(OptionsDialog::DIALOG_ID, aDialog);
		aDialog->Move(Scaled(GetGridX() + 0x200) - aDialog->mWidth / 2, Scaled(200));
		break;
	}
	case BUTTON_RESET:
	{
		// 0x74bc40 case 2 (only while the game is being played, which the test above gives): the Bej3Dialog with id 0x16, the header
		// "RESET" (string 0x9b), the line "Abandon the current game and start over?" (0x9c), Yes and No buttons (mode 1), 850 wide (1200ths),
		// 350 down and put 512 right of the grid's middle less half its width; the board is its listener
		Dialog* aDialog = gApp->DoDialog(DIALOG_RESET, true, gApp->GetString("155", "RESET"),
			gApp->GetString("156", "Abandon the current game and start over?"), "", Dialog::BUTTONS_YES_NO);
		const int aWidth = Scaled(0x352);
		aDialog->Resize(Scaled(GetGridX() + 0x200) - aWidth / 2, Scaled(0x15e), aWidth, aDialog->GetPreferredHeight(aWidth));
		aDialog->mDialogListener = this;
		break;
	}
	default:
		break;
	}
}

// 0x7563c0 (bejeweled3). port: the Yes of the reset dialog also writes the mode's name and "Play" to the app's event log and deletes the
// saved game's file (0x743120); the port keeps no saved game
void Board::DialogButtonDepress(int theDialogId, int theButtonId)
{
	if (theDialogId == HINT_DIALOG_ID && !mHintQueue.empty() && theButtonId == Dialog::ID_YES) {
		// a tutorial box's OK: its request goes; with its "Disable Hints" box ticked every box is off from now on, the queue is emptied, and the board
		// takes the profile's flags
		mHintQueue.erase(mHintQueue.begin());
		HintDialog* aBox = dynamic_cast<HintDialog*>(gApp->GetDialog(HINT_DIALOG_ID));
		if (aBox != nullptr && aBox->mCheckbox->IsChecked()) {
			SetHintFlag(HINTS_OFF_BIT, true);
			if (gApp->mProfile != nullptr && !mDemonstration)
				mHintFlags = gApp->mProfile->mHintFlags;
			mHintQueue.clear();
		}
	}
	Bej3Dialog* aClosing = dynamic_cast<Bej3Dialog*>(gApp->GetDialog(theDialogId));
	if (aClosing != nullptr)
		aClosing->StartClose();
	if (theDialogId == DIALOG_RESET && theButtonId == Dialog::ID_YES) {
		ResetGame();   // slot 85
		NewGame();     // slot 135
	}
}

}  // namespace Sexy
