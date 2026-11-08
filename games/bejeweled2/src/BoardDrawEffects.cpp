// BoardDrawEffects.cpp: the things that Board::Draw puts over the gems: the sparks, flames and explosions (DrawEffects
// 0x598525) and the flashes of lightning between two gems (DrawLightning 0x5989c7).

#include "Board.h"
#include "WinDMApp.h"
#include "Res.h"
#include "DDInterface.h"
#include "TriVertex.h"
#include "Point.h"
#include "Graphics.h"
#include "Color.h"
#include <math.h>

using namespace Sexy;

extern Color gElectColors[8];   // 0x6797d0 (BoardEffects.cpp): the colours of the lightning, by the colour of the gems

// 0x598525: the particles (mShards) and then the explosion frames (mEffects). thePass is set for the second drawing
// pass, in which the swirls (kind 3) are drawn; the others are drawn in both
void Board::DrawEffects(Graphics* g, bool thePass)
{
	int i = 0;
	int aCount = (int)mShards.size();
	if (0 < aCount)
	{
		do
		{
			Shard* aShard = mShards[i];
			int aKind = aShard->mKind;
			if (aKind == 3)
			{
				if (thePass)
				{
					g->SetDrawMode(1);
					g->SetColorizeImages(true);
					g->SetColor(Color(0xff, 0xff, 0xff, FTol((double)aShard->mAlpha * 255.0)));
					float aScaledY = mApp->ScaleNum((float)((double)aShard->mPosY - 11.0), 0.0f);
					int aY = FTol(aScaledY);
					float aScaledX = mApp->ScaleNum((float)((double)aShard->mPosX - 11.0), 0.0f);
					int aX = FTol(aScaledX);
					g->DrawImage(GetImageById(IMAGE_ENDLESS_SPARK_1_ID + aShard->mImageOffset), aX, aY);
					g->SetColorizeImages(false);
					g->SetDrawMode(0);
				}
			}
			else if (aKind == 1)
			{
				int aSize = mApp->ScaleNumUp(0x1e, 0);
				Rect aSrc(0, 0, mApp->ScaleNumUp(0x1e, 0), aSize);
				float aScaledY = mApp->ScaleNum(aShard->mY, 0.0f);
				int aY = FTol(aScaledY);
				float aScaledX = mApp->ScaleNum(aShard->mX, 0.0f);
				int aX = FTol(aScaledX);
				g->DrawImage(IMAGE_ROCK_CHUNK, aX, aY, aSrc);
			}
			else if (aKind == 2)
			{
				g->SetDrawMode(1);
				g->SetColorizeImages(true);
				g->SetColor(aShard->mColor);
				int aHeight = mApp->ScaleNum(0x28, 0);
				int aWidth = mApp->ScaleNum(0x28, 0);
				Rect aSrc(mApp->ScaleNum(aShard->mFrame * 0x28, 0), 0, aWidth, aHeight);
				float aScaledY = mApp->ScaleNum((float)((double)aShard->mY - 20.0), 0.0f);
				int aY = FTol(aScaledY);
				float aScaledX = mApp->ScaleNum((float)((double)aShard->mX - 20.0), 0.0f);
				int aX = FTol(aScaledX);
				g->DrawImage(IMAGE_SPARKLE, aX, aY, aSrc);
				g->SetColorizeImages(false);
				g->SetDrawMode(0);
			}
			else if (aKind == 5)
			{
				g->SetColor(Color(0xff, 0xff, 0xff));
				float aScaledY = mApp->ScaleNum(aShard->mY, 0.0f);
				int aY = FTol(aScaledY);
				float aScaledX = mApp->ScaleNum(aShard->mX, 0.0f);
				int aX = FTol(aScaledX);
				g->FillRect(aX, aY, 2, 2);
			}
			else
			{
				g->SetColorizeImages(true);
				g->SetColor(aShard->mColor);
				int aWidth = mApp->ScaleNumUp(0x1e, 0);
				int aHeight = mApp->ScaleNumUp(0x1e, 0);
				Rect aSrc(mApp->ScaleNum(aShard->mFrame * 0x1e, 0), 0, aHeight, aWidth);
				float aScaledY = mApp->ScaleNum(aShard->mY, 0.0f);
				int aY = FTol(aScaledY);
				float aScaledX = mApp->ScaleNum(aShard->mX, 0.0f);
				int aX = FTol(aScaledX);
				g->DrawImage(IMAGE_SHARD, aX, aY, aSrc);
				g->SetColorizeImages(false);
			}
			i++;
			aCount = (int)mShards.size();
		}
		while (i < aCount);
	}

	i = 0;
	aCount = (int)mEffects.size();
	if (0 < aCount)
	{
		do
		{
			Effect* anEffect = mEffects[i];
			if (anEffect->mKind == 0 && 0 < anEffect->mFrame)
			{
				g->SetDrawMode(1);
				int aWidth = mApp->ScaleNumUp(0x50, 0);
				int aHeight = mApp->ScaleNumUp(0x50, 0);
				Rect aSrc(mApp->ScaleNum(anEffect->mFrame * 0x50, 0), 0, aHeight, aWidth);
				int aY = mApp->ScaleNum(anEffect->mY - 0x28, 0);
				int aX = mApp->ScaleNum(anEffect->mX - 0x28, 0);
				g->DrawImage(IMAGE_EXPLOSION, aX, aY, aSrc);
				g->SetDrawMode(0);
			}
			i++;
			aCount = (int)mEffects.size();
		}
		while (i < aCount);
	}
}

// 0x5989c7: each flash of lightning is a ribbon along its eight points that fades out (mProgress, 0 to 1, and the
// ribbon is at full strength for the first eighth). The 2D game fills two triangles per piece of the ribbon in the
// colour of the gems that the lightning was made for and two thinner ones in grey on top; the 3D game draws the same
// ribbon as a textured mesh, twice (IMAGE_LIGHTNING in the gems' colour, IMAGE_LIGHTNING_CENTER in grey)
void Board::DrawLightning(Graphics* g)
{
	bool a3D = mApp->Is3DAccelerated();
	int aFlash = 0;
	int aCount = (int)mLightning.size();
	if (0 < aCount)
	{
		do
		{
			Lightning* aLightning = mLightning[aFlash];
			float aBright = (float)((1.0 - (double)aLightning->mProgress) * 8.0);
			if (1.0 <= aBright)
				aBright = 1.0f;
			int aGrey = FTol((double)aBright * 255.0);
			if (!a3D)
			{
				g->SetDrawMode(1);
				const Color& anElect = gElectColors[m15e08];
				int aBlue = FTol((double)anElect.mBlue * (double)aBright);
				int aGreen = FTol((double)anElect.mGreen * (double)aBright);
				int aRed = FTol((double)anElect.mRed * (double)aBright);
				Color aColor(aRed, aGreen, aBlue);

				// the points a, b are the first copy of a point, c, d the second; e to h belong to the next point
				const float* aPoint = &aLightning->mPoint[0][0];
				for (int aPiece = 7; aPiece != 0; aPiece--)
				{
					float a = aPoint[0], b = aPoint[1], c = aPoint[2], d = aPoint[3];
					float e = aPoint[4], f = aPoint[5], gx = aPoint[6], h = aPoint[7];

					float aInnerX1 = (float)((double)c * (double)0.7f + (double)a * (double)0.3f);
					float aInnerY1 = (float)((double)b * (double)0.3f + (double)d * (double)0.7f);
					float aInnerX2 = (float)((double)a * (double)0.7f + (double)c * (double)0.3f);
					float aInnerY2 = (float)((double)b * (double)0.7f + (double)d * (double)0.3f);
					float aInnerX3 = (float)((double)gx * (double)0.7f + (double)e * (double)0.3f);
					float aInnerY3 = (float)((double)h * (double)0.7f + (double)f * (double)0.3f);
					float aInnerX4 = (float)((double)gx * (double)0.3f + (double)e * (double)0.7f);
					float aInnerY4 = (float)((double)f * (double)0.7f + (double)h * (double)0.3f);

					Point aPoly[3];
					g->SetColor(aColor);
					aPoly[0].mX = FTol(mApp->ScaleNum(a, 0.0f));
					aPoly[0].mY = FTol(mApp->ScaleNum(b, 0.0f));
					aPoly[1].mX = FTol(mApp->ScaleNum(gx, 0.0f));
					aPoly[1].mY = FTol(mApp->ScaleNum(h, 0.0f));
					aPoly[2].mX = FTol(mApp->ScaleNum(e, 0.0f));
					aPoly[2].mY = FTol(mApp->ScaleNum(f, 0.0f));
					g->PolyFill(aPoly, 3, false);
					aPoly[0].mX = FTol(mApp->ScaleNum(a, 0.0f));
					aPoly[0].mY = FTol(mApp->ScaleNum(b, 0.0f));
					aPoly[1].mX = FTol(mApp->ScaleNum(c, 0.0f));
					aPoly[1].mY = FTol(mApp->ScaleNum(d, 0.0f));
					aPoly[2].mX = FTol(mApp->ScaleNum(gx, 0.0f));
					aPoly[2].mY = FTol(mApp->ScaleNum(h, 0.0f));
					g->PolyFill(aPoly, 3, false);

					g->SetColor(Color(aGrey, aGrey, aGrey));
					aPoly[0].mX = FTol(mApp->ScaleNum(aInnerX1, 0.0f));
					aPoly[0].mY = FTol(mApp->ScaleNum(aInnerY1, 0.0f));
					aPoly[1].mX = FTol(mApp->ScaleNum(aInnerX4, 0.0f));
					aPoly[1].mY = FTol(mApp->ScaleNum(aInnerY4, 0.0f));
					aPoly[2].mX = FTol(mApp->ScaleNum(aInnerX3, 0.0f));
					aPoly[2].mY = FTol(mApp->ScaleNum(aInnerY3, 0.0f));
					g->PolyFill(aPoly, 3, false);
					aPoly[0].mX = FTol(mApp->ScaleNum(aInnerX1, 0.0f));
					aPoly[0].mY = FTol(mApp->ScaleNum(aInnerY1, 0.0f));
					aPoly[1].mX = FTol(mApp->ScaleNum(aInnerX2, 0.0f));
					aPoly[1].mY = FTol(mApp->ScaleNum(aInnerY2, 0.0f));
					aPoly[2].mX = FTol(mApp->ScaleNum(aInnerX4, 0.0f));
					aPoly[2].mY = FTol(mApp->ScaleNum(aInnerY4, 0.0f));
					g->PolyFill(aPoly, 3, false);

					aPoint += 4;
				}
				g->SetDrawMode(0);
			}
			else
			{
				// the mesh: one triangle at each end of the ribbon and two for each piece between
				TriVertex aVerts[14][3];
				int aTriangles = 0;
				int aVert = 0;
				int aPiece = 0;
				float aV0 = 0.0f;
				float aV1 = 0.0f;
				do
				{
					const float* aPoint = &aLightning->mPoint[aPiece][0];
					aV0 = (float)((double)aPiece / 7.0);
					aV1 = (float)((double)(aPiece + 1) / 7.0);
					aTriangles++;
					TriVertex* aV = &aVerts[0][0] + aVert;
					if (aPiece == 0)
					{
						aV[0].x = mApp->ScaleNum(aPoint[0], 0.0f);
						aV[0].y = mApp->ScaleNum(aPoint[1], 0.0f);
						aV[0].u = 0.5f;
						aV[0].v = aV0;
						aV[1].x = mApp->ScaleNum(aPoint[6], 0.0f);
						aV[1].y = mApp->ScaleNum(aPoint[7], 0.0f);
						aV[1].u = 1.0f;
						aV[1].v = aV1;
						aV[2].x = mApp->ScaleNum(aPoint[4], 0.0f);
						aV[2].y = mApp->ScaleNum(aPoint[5], 0.0f);
						aV[2].u = 0.0f;
						aV[2].v = aV1;
						aVert += 3;
					}
					else if (aPiece == 6)
					{
						aV[0].x = mApp->ScaleNum(aPoint[0], 0.0f);
						aV[0].y = mApp->ScaleNum(aPoint[1], 0.0f);
						aV[0].u = 0.0f;
						aV[0].v = aV0;
						aV[1].x = mApp->ScaleNum(aPoint[2], 0.0f);
						aV[1].y = mApp->ScaleNum(aPoint[3], 0.0f);
						aV[1].u = 1.0f;
						aV[1].v = aV0;
						aV[2].x = mApp->ScaleNum(aPoint[4], 0.0f);
						aV[2].y = mApp->ScaleNum(aPoint[5], 0.0f);
						aV[2].u = 0.5f;
						aV[2].v = aV1;
						aVert += 3;
					}
					else
					{
						aV[0].x = mApp->ScaleNum(aPoint[0], 0.0f);
						aV[0].y = mApp->ScaleNum(aPoint[1], 0.0f);
						aV[0].u = 0.0f;
						aV[0].v = aV0;
						aV[1].x = mApp->ScaleNum(aPoint[6], 0.0f);
						aV[1].y = mApp->ScaleNum(aPoint[7], 0.0f);
						aV[1].u = 1.0f;
						aV[1].v = aV1;
						aV[2].x = mApp->ScaleNum(aPoint[4], 0.0f);
						aV[2].y = mApp->ScaleNum(aPoint[5], 0.0f);
						aV[2].u = 0.0f;
						aV[2].v = aV1;
						aTriangles++;
						aV[3].x = mApp->ScaleNum(aPoint[0], 0.0f);
						aV[3].y = mApp->ScaleNum(aPoint[1], 0.0f);
						aV[3].u = 0.0f;
						aV[3].v = aV0;
						aV[4].x = mApp->ScaleNum(aPoint[2], 0.0f);
						aV[4].y = mApp->ScaleNum(aPoint[3], 0.0f);
						aV[4].u = 1.0f;
						aV[4].v = aV0;
						aV[5].x = mApp->ScaleNum(aPoint[6], 0.0f);
						aV[5].y = mApp->ScaleNum(aPoint[7], 0.0f);
						aV[5].u = 1.0f;
						aV[5].v = aV1;
						aVert += 6;
					}
					aPiece++;
				}
				while (aPiece < 7);

				const Color& anElect = gElectColors[m15e08];
				int aBlue = FTol((double)anElect.mBlue * (double)aBright);
				int aGreen = FTol((double)anElect.mGreen * (double)aBright);
				int aRed = FTol((double)anElect.mRed * (double)aBright);
				Color aColor(aRed, aGreen, aBlue);
				mApp->mDDInterface->DrawTrianglesTex(IMAGE_LIGHTNING, aVerts, aTriangles, NULL, aColor, 1, 0.0f, 0.0f, true);
				mApp->mDDInterface->DrawTrianglesTex(IMAGE_LIGHTNING_CENTER, aVerts, aTriangles, NULL, Color(aGrey, aGrey, aGrey), 1, 0.0f, 0.0f, true);
			}
			aFlash++;
			aCount = (int)mLightning.size();
		}
		while (aFlash < aCount);
	}
}
