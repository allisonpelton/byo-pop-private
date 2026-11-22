// The Classic board's drawing (ClassicBoard::Draw 0x73b250 (bejeweled3): the panel 0x751d50, then 0x750320 with the screen's
// pictures, the gems by slot 237 (0x757df0) and the HUD). docs/NOTES.md ("The Classic screen's drawing") says what is
// translated and what is not. Every measure of the exe is in 1200ths of the art height in use: Scaled() is its
// "art size * measure / 1200".
#include "Board.h"

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
void Board::DrawPiece(Graphics* g, const Piece* thePiece)
{
	static Image** const aSheets[Piece::COLOR_COUNT] = {&IMAGE_GEMS_RED, &IMAGE_GEMS_WHITE, &IMAGE_GEMS_GREEN, &IMAGE_GEMS_YELLOW,
		&IMAGE_GEMS_PURPLE, &IMAGE_GEMS_ORANGE, &IMAGE_GEMS_BLUE};
	int aColor = thePiece->mColor;
	if (aColor < 0 || aColor >= Piece::COLOR_COUNT)
		return;
	// slot 175, 0x73bd10: the board's alpha, the cover curve (+0x2898) times 1 less +0x12c8 (0 here)
	double anAlpha = mCoverCurve.GetOutVal();
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
		if (aSheet != nullptr) {
			int aCels = aSheet->mNumCols * aSheet->mNumRows;
			float aTurn = (float)aCels * thePiece->mSpinPhase;
			int aCel = Ftol((double)aTurn);
			g->DrawImageCel(aSheet, aPixelX, aPixelY, aCel);
		}
	}
	g->PopState();
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

// The pictures of the screen round the grid, in the order the exe draws them. The left panel's two pictures are placed by their x= and y= in
// resources.xml with the panel's 400 taken off (0x741440, 0x7423e0); the frame above the grid (0x745880) and the bar's frame and back below
// it (0x745430, 0x7540c0) have their middle at the grid's middle (x + 0x200) and at y - 5, + 0x421 and + 0x41e.
// port: the level bar's fill (the exe draws PIEFFECT_LEVELBAR, 0x575650, and its look is set by 0x75a760 and 0x755620), the glows of
// the frames (while the bar's stage is on, 0x368), the pictures' draws under the board's alpha curves and the slide of the panel
void Board::DrawScreenFrame(Graphics* g)
{
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, Ftol(mCoverCurve.GetOutVal() * 255.0)));
	// 0x7423e0: the top widget at its place and the level number (the level index + 1) over it, in FONT_LEVEL
	DrawPanelPicture(g, IMAGE_INGAMEUI_TOP_WIDGET, IMAGE_INGAMEUI_TOP_WIDGET_ID);
	{
		g->SetFont(FONT_LEVEL);
		Color anOld;
		bool aSet = SetLayerColor(FONT_LEVEL, "GLOW", Color(128, 128, 128, 255), &anOld);
		g->SetColor(Color(255, 255, 255, 255));
		g->WriteString(CommaSeperate(mLevelIndex + 1), Scaled(PANEL_MIDDLE_X), Scaled(0xfa), -1, 0, true, 0, -1, -1);
		if (aSet)
			RestoreLayerColor(FONT_LEVEL, "GLOW", anOld);
	}
	// 0x746f20: the score, in the middle of x 250 at y 202
	g->SetFont(FONT_SCORE);
	g->SetColor(Color(255, 255, 255, 255));
	g->WriteString(CommaSeperate(mScore), Scaled(250), Scaled(202), 0, 0, true, 0, -1, -1);
	// 0x741440: the bottom widget of the mode (Classic)
	DrawPanelPicture(g, IMAGE_INGAMEUI_BOTTOM_WIDGET_CLASSIC, IMAGE_INGAMEUI_BOTTOM_WIDGET_CLASSIC_ID);
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
	// slot 246, 0x7568a0: the gems. port: the exe draws the plain ones, the ones held by a swap or special and the flame gems in three
	// passes, with the effects of the board's layer between them; here one pass in the grid's order
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			if (mGrid[aRow][aCol] != nullptr)
				DrawPiece(g, mGrid[aRow][aCol]);
		}
	}
}

// 0x741840 (bejeweled3), Classic's case: the Hint button (id 0, the Classic hint picture) and the Menu button (id 1, the Classic menu
// picture), each where resources.xml puts its picture with the panel's 400 off, the size of a cel (the pictures are two cels wide:
// cel 0 at rest, cel 1 over and pressed). The exe's class is Bej3Button (vtable 0x86498c, ctor 0x49c490); its hover curves (+0x150
// and +0x158) are not translated
void Board::MakeButtons()
{
	struct ButtonSpec {
		int mId;
		Image* mImage;
		int mImageId;
		ButtonWidget** mTarget;
	};
	const ButtonSpec aSpecs[] = {
		{BUTTON_HINT, IMAGE_INGAMEUI_HINT_BUTTON_CLASSIC, IMAGE_INGAMEUI_HINT_BUTTON_CLASSIC_ID, &mHintButton},
		{BUTTON_MENU, IMAGE_INGAMEUI_MENU_BUTTON_CLASSIC, IMAGE_INGAMEUI_MENU_BUTTON_CLASSIC_ID, &mMenuButton},
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
		// port: the in-game menu (dialog 0x15, made by 0x681370 with 1) is not translated yet
		break;
	default:
		break;
	}
}

}  // namespace Sexy
