// BoardDraw3D.cpp: what the 3D game draws in place of Board::Draw's usual picture while the whirlpool takes the interface
// (Draw3DWhirlpoolState 0x5a1787) and while the interface comes back after the jump (Draw3DInterfaceRestoreState
// 0x5a238c): the star field (or the mesh of the whirlpool), the black hole, and the board, the score box and the
// controls turned and shrunk about their middles by the display's transform stack.
//
// The original asks its D3DInterface (the one at the app's +0x36c, its +0x30) to PushTransform and PopTransform. The
// port's display keeps that stack in the GPU2D of a GPUDisplay, so the two helpers below reach it there; any other
// display has no stack (and never takes these paths, since Is3DAccelerated is false for it).

#include "Board.h"
#include "WinDMApp.h"
#include "Res.h"
#include "StarField.h"
#include "DDInterface.h"
#include "MemoryImage.h"
#include "GPUDisplay.h"
#include "GPU2D.h"
#include "SexyMatrix.h"
#include "TriVertex.h"
#include "Graphics.h"
#include "Color.h"
#include <math.h>
#include <vector>

using namespace Sexy;

// 0x44b8f0 D3DInterface::PushTransform(theTransform, true)
static void PushScreenTransform(WinDMApp* theApp, const SexyTransform2D& theTransform)
{
	GPUDisplay* aDisplay = dynamic_cast<GPUDisplay*>(theApp->mDDInterface);
	if (aDisplay != NULL && aDisplay->mGPU2D != NULL)
		aDisplay->mGPU2D->PushTransform(theTransform, true);
}

// 0x44b990 D3DInterface::PopTransform
static void PopScreenTransform(WinDMApp* theApp)
{
	GPUDisplay* aDisplay = dynamic_cast<GPUDisplay*>(theApp->mDDInterface);
	if (aDisplay != NULL && aDisplay->mGPU2D != NULL)
		aDisplay->mGPU2D->PopTransform();
}

// the box of the score (the same places as DrawScore's, BoardDraw.cpp): the pixel offset of its top left corner in the
// 640 by 480 screen is (aLeft, aTop)
static int ScoreBoxTop(const Board* theBoard)
{
	if (!theBoard->mIsPuzzle || theBoard->mIsSecret)
		return theBoard->mIsEndless ? 0x23 : 0x2a;
	return 0x1b;
}

static int ScoreBoxLeft(const Board* theBoard)
{
	if (!theBoard->mIsPuzzle || theBoard->mIsSecret)
		return theBoard->mIsEndless ? 0xf : 0x16;
	return 0;
}

// The matrix that the original builds for each of the three parts (the original writes these steps out in full for each
// one): the part's picture is moved so that its middle is at the origin (the original takes its width for both
// directions), scaled by theScale and turned by theAngle (the original's Rotate(-theAngle), which is RotateRad(theAngle)
// in the framework's newer names), and then moved to where its middle should be: theOffsetX/Y (already ScaleNum'ed)
// from the middle of the picture, scaled, about the middle of the screen (theCenterX/Y). The stack takes the matrix.
static void PushTurnedPart(WinDMApp* theApp, int theWidth, float theScale, float theAngle, int theOffsetX, int theOffsetY,
	int theCenterX, int theCenterY)
{
	SexyTransform2D aTransform;
	float aHalf = (float)-(theWidth / 2);
	aTransform.Translate(aHalf, aHalf);
	aTransform.Scale(theScale, theScale);
	aTransform.RotateRad(theAngle);

	float aCenterX = (float)theCenterX;
	float aCenterY = (float)theCenterY;
	int aHalfWidth = theWidth / 2;
	float aY = (float)(theOffsetY + (aHalfWidth - theCenterY)) * theScale;
	aY = aY + aCenterY;
	float aX = (float)(theOffsetX + (aHalfWidth - theCenterX)) * theScale;
	aX = aX + aCenterX;
	aTransform.Translate(aX, aY);
	PushScreenTransform(theApp, aTransform);
}

// 0x5a1787: the whirlpool state of the 3D game. The backdrop is a mesh of 47 by 47 squares (two triangles each) from the
// points of mWhirlMesh, which the whirlpool bends (the cheats' star field, when there is one, draws it instead); the black
// hole (a cover, then the five turning frames, two of them blended) grows over it; and the board, the score box and the
// controls (puzzle or action buttons) fly away as m15de0 grows from 0 to 1, each turning about its middle
void Board::Draw3DWhirlpoolState(Graphics* g)
{
	if (m165bc == NULL)
	{
		std::vector<TriVertex> aVerts(0x3600);
		TriVertex (*aTriangles)[3] = (TriVertex (*)[3])&aVerts[0];
		int aCount = 0;
		for (int aRow = 0; aRow < 0x2f; aRow++)
		{
			for (int aCol = 0; aCol < 0x2f; aCol++)
			{
				WhirlPoint* aPoint = &mWhirlMesh[aRow][aCol];
				WhirlPoint* aRight = &mWhirlMesh[aRow][aCol + 1];
				WhirlPoint* aDown = &mWhirlMesh[aRow + 1][aCol];
				WhirlPoint* aDownRight = &mWhirlMesh[aRow + 1][aCol + 1];

				TriVertex* aTriangle = aTriangles[aCount];
				aCount++;
				aTriangle[0].x = mApp->ScaleNum(aPoint->mX, 0.0f);
				aTriangle[0].y = mApp->ScaleNum(aPoint->mY, 0.0f);
				aTriangle[0].u = aPoint->mU;
				aTriangle[0].v = aPoint->mV;
				aTriangle[1].x = mApp->ScaleNum(aRight->mX, 0.0f);
				aTriangle[1].y = mApp->ScaleNum(aRight->mY, 0.0f);
				aTriangle[1].u = aRight->mU;
				aTriangle[1].v = aRight->mV;
				aTriangle[2].x = mApp->ScaleNum(aDownRight->mX, 0.0f);
				aTriangle[2].y = mApp->ScaleNum(aDownRight->mY, 0.0f);
				aTriangle[2].u = aDownRight->mU;
				aTriangle[2].v = aDownRight->mV;

				aTriangle = aTriangles[aCount];
				aCount++;
				aTriangle[0].x = mApp->ScaleNum(aPoint->mX, 0.0f);
				aTriangle[0].y = mApp->ScaleNum(aPoint->mY, 0.0f);
				aTriangle[0].u = aPoint->mU;
				aTriangle[0].v = aPoint->mV;
				aTriangle[1].x = mApp->ScaleNum(aDownRight->mX, 0.0f);
				aTriangle[1].y = mApp->ScaleNum(aDownRight->mY, 0.0f);
				aTriangle[1].u = aDownRight->mU;
				aTriangle[1].v = aDownRight->mV;
				aTriangle[2].x = mApp->ScaleNum(aDown->mX, 0.0f);
				aTriangle[2].y = mApp->ScaleNum(aDown->mY, 0.0f);
				aTriangle[2].u = aDown->mU;
				aTriangle[2].v = aDown->mV;
			}
		}
		mApp->mDDInterface->DrawTrianglesTex(mBackdropImage, aTriangles, aCount, NULL, Color(0xff, 0xff, 0xff, 0xff), 0,
			0.0f, 0.0f, true);
	}
	else
	{
		m165bc->Draw(g, m165b8);
	}

	// the cover of the black hole fades in
	if (0.0 < m15f90)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(0xff, 0xff, 0xff, FTol(m15f90 * 255.0)));
		Image* aCover = IMAGE_BLACK_HOLE_COVER;
		int aY = mApp->ScaleNum(0x180, 0) - aCover->GetHeight() / 2;
		int aX = mApp->ScaleNum(0x200, 0) - aCover->GetWidth() / 2;
		g->DrawImage(aCover, aX, aY);
		g->SetColorizeImages(false);
	}

	// the black hole turns through its five frames: this one and the next are drawn, blended by how far on it is
	if (0.0 < m15f90)
	{
		g->SetColorizeImages(true);
		g->SetDrawMode(1);
		int aFrame = FTol(m15f84);
		int aNextFrame = aFrame + 1;
		if (aNextFrame == 5)
			aNextFrame = 0;
		float aFrac = (float)((double)m15f84 - (double)aFrame);
		int aHoleX = mApp->ScaleNum(0x180, 0);
		int aHoleY = mApp->ScaleNum(0x100, 0);
		Rect aSrc(mApp->ScaleNum(aFrame << 8, 0), 0, mApp->ScaleNum(0x100, 0), mApp->ScaleNum(0x100, 0));
		Rect aNextSrc(mApp->ScaleNum(aNextFrame << 8, 0), 0, mApp->ScaleNum(0x100, 0), mApp->ScaleNum(0x100, 0));

		g->SetColor(Color(0xff, 0xff, 0xff, FTol(m15f90 * 255.0 * (1.0 - (double)aFrac))));
		if (0 < g->mColor.mAlpha && g->mColor.mAlpha <= 0xff)
			g->DrawImageRotated(IMAGE_BLACK_HOLE, aHoleX, aHoleY, m15f98, &aSrc);

		g->SetColor(Color(0xff, 0xff, 0xff, FTol(m15f90 * 255.0 * (double)aFrac)));
		if (0 < g->mColor.mAlpha && g->mColor.mAlpha <= 0xff)
			g->DrawImageRotated(IMAGE_BLACK_HOLE, aHoleX, aHoleY, m15f98, &aNextSrc);

		g->SetDrawMode(0);
		g->SetColorizeImages(false);
	}

	// the interface: aInv shrinks the three parts as the whirlpool takes them (m15de0 is how far on that is) and aSin
	// throws them off to the sides
	float aInv = (float)(1.0 - m15de0);
	float aSin = (float)sin(m15de0 * 3.14159 * 0.5);
	int aCenterX = mApp->ScaleNum(0x200, 0);
	int aCenterY = mApp->ScaleNum(0x180, 0);

	if (m15de0 == 0.0)
	{
		DrawBoard(g, false, false);
	}
	else
	{
		mFadeIn = 0.0f;
		// (the original also asks for the height of the picture and does nothing with it)
		int aBorderWidth = IMAGE_BOARD_BORDER->GetWidth();
		float aAngle = (float)(m15de0 * 5.0);
		int aOffsetY = mApp->ScaleNum(3, 0);
		int aOffsetX = mApp->ScaleNum(0x108 - FTol((double)aSin * -1000.0), 0);
		PushTurnedPart(mApp, aBorderWidth, aInv, aAngle, aOffsetX, aOffsetY, aCenterX, aCenterY);
		g->Translate(mApp->ScaleNum(-0x108, 0), mApp->ScaleNum(-3, 0));
		DrawBoard(g, false, false);
		g->Translate(mApp->ScaleNum(0x108, 0), mApp->ScaleNum(3, 0));
		PopScreenTransform(mApp);
	}

	// the score box
	int aLeft = ScoreBoxLeft(this);
	double aSideOffset = (double)aSin * 300.0;
	int aLeftX = FTol((double)aLeft - aSideOffset);
	int aTop = ScoreBoxTop(this);
	int aTopY = aTop - FTol((double)aSin * 400.0);
	int aScoreWidth;
	if (!mIsPuzzle)
		aScoreWidth = IMAGE_CONTROLS_SCORE->GetWidth();
	else
		aScoreWidth = mApp->ScaleNumUp(0x128, 0);
	float aScoreAngle = (float)(m15de0 * -6.0);
	PushTurnedPart(mApp, aScoreWidth, aInv, aScoreAngle, mApp->ScaleNum(aLeftX, 0), mApp->ScaleNum(aTopY, 0), aCenterX,
		aCenterY);
	g->Translate(mApp->ScaleNum(-aLeft, 0), mApp->ScaleNum(-aTop, 0));
	DrawScore(g, false, false);
	g->Translate(mApp->ScaleNum(aLeft, 0), mApp->ScaleNum(aTop, 0));
	PopScreenTransform(mApp);

	// the controls under the board
	int aControlsWidth = IMAGE_CONTROLS_PUZZLE->GetWidth();
	float aControlsAngle = (float)(m15de0 * -4.0);
	int aControlsY = FTol((double)aSin * 600.0 + 339.0);
	int aControlsX = FTol(27.0 - aSideOffset);
	PushTurnedPart(mApp, aControlsWidth, aInv, aControlsAngle, mApp->ScaleNum(aControlsX, 0),
		mApp->ScaleNum(aControlsY, 0), aCenterX, aCenterY);
	g->Translate(mApp->ScaleNum(-0x1b, 0), mApp->ScaleNum(-0x153, 0));
	DrawUI(g, false, false);
	g->Translate(mApp->ScaleNum(0x1b, 0), mApp->ScaleNum(0x153, 0));
	PopScreenTransform(mApp);
}

// 0x5a238c: the state after the jump: the interface comes back from the sides, turning, as m15e04 (0 to 1) grows. aInv is
// 1 - m15e04, the part of the way that is still to go, and the parts are not scaled (the whirlpool's does the scaling)
void Board::Draw3DInterfaceRestoreState(Graphics* g)
{
	int aCenterX = mApp->ScaleNum(0x200, 0);
	int aCenterY = mApp->ScaleNum(0x180, 0);
	mFadeIn = 0.0f;
	if (m165bc == NULL)
		g->DrawImage(mBackdropImage, 0, 0);
	else
		m165bc->Draw(g, m165b8);

	float aInv = (float)(1.0 - (double)m15e04);

	// the board
	// (the original also asks for the height of the picture and does nothing with it)
	int aBorderWidth = IMAGE_BOARD_BORDER->GetWidth();
	float aAngle = (float)((double)aInv * (double)0.4f);
	int aOffsetY = mApp->ScaleNum(3, 0);
	float aSquare = (float)((double)aInv * (double)aInv);
	int aOffsetX = mApp->ScaleNum(0x108 - FTol((double)aSquare * -1024.0), 0);
	PushTurnedPart(mApp, aBorderWidth, 1.0f, aAngle, aOffsetX, aOffsetY, aCenterX, aCenterY);
	g->Translate(mApp->ScaleNum(-0x108, 0), mApp->ScaleNum(-3, 0));
	DrawBoard(g, false, false);
	g->Translate(mApp->ScaleNum(0x108, 0), mApp->ScaleNum(3, 0));
	PopScreenTransform(mApp);

	// the score box
	int aLeft = ScoreBoxLeft(this);
	int aLeftX = FTol((double)aLeft - (double)aInv * 600.0);
	int aTop = ScoreBoxTop(this);
	int aScoreWidth;
	if (!mIsPuzzle)
		aScoreWidth = IMAGE_CONTROLS_SCORE->GetWidth();
	else
		aScoreWidth = mApp->ScaleNumUp(0x128, 0);
	float aScoreAngle = (float)((double)aInv * -0.5);
	PushTurnedPart(mApp, aScoreWidth, 1.0f, aScoreAngle, mApp->ScaleNum(aLeftX, 0), mApp->ScaleNum(aTop, 0), aCenterX,
		aCenterY);
	g->Translate(mApp->ScaleNum(-aLeft, 0), mApp->ScaleNum(-aTop, 0));
	DrawScore(g, false, false);
	g->Translate(mApp->ScaleNum(aLeft, 0), mApp->ScaleNum(aTop, 0));
	PopScreenTransform(mApp);

	// the controls under the board
	int aControlsWidth = IMAGE_CONTROLS_PUZZLE->GetWidth();
	float aControlsAngle = (float)((double)aInv * -0.6);
	int aControlsY = 0x153;
	int aControlsX = 0x1b - FTol((double)aInv * 400.0);
	PushTurnedPart(mApp, aControlsWidth, 1.0f, aControlsAngle, mApp->ScaleNum(aControlsX, 0),
		mApp->ScaleNum(aControlsY, 0), aCenterX, aCenterY);
	g->Translate(mApp->ScaleNum(-0x1b, 0), mApp->ScaleNum(-0x153, 0));
	DrawUI(g, false, false);
	g->Translate(mApp->ScaleNum(0x1b, 0), mApp->ScaleNum(0x153, 0));
	PopScreenTransform(mApp);
}
