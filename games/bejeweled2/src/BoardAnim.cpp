// BoardAnim.cpp: animations of Sexy::Board that are not part of the play: the gems falling away when the game ends, the
// warp mesh of the hyperspace jump and the light that an explosion throws on the gems around it.
//   UpdateGameOverAnim 0x595ea2, UpdateHyperWarpPoints 0x5967d0, ComparePieceZ 0x5968f8, AddPointLight 0x596931.

#include "Board.h"
#include "WinDMApp.h"
#include "Res.h"
#include <math.h>

using namespace Sexy;

extern float gSinTable[];
extern float gCosTable[];

// 0x595ea2: the gems fly off the board; the game goes on to the high scores when they have all gone
void Board::UpdateGameOverAnim()
{
	MarkDirty();
	bool a3D = mApp->Is3DAccelerated();
	float aFade = (float)((double)mFadeIn - (double)0.02f);
	mFadeIn = aFade;
	if (aFade < 0.0f)
		mFadeIn = 0.0f;

	if (m15e58 < 1)
	{
		bool anAllGone = true;
		for (int aRow = 0; aRow < 8; aRow++)
		{
			for (int aCol = 0; aCol < 8; aCol++)
			{
				Piece* aPiece = mBoard[aRow][aCol];
				if (aPiece == NULL)
					continue;

				aPiece->mX = (float)aPiece->mVX + aPiece->mX;
				aPiece->mY = aPiece->mVY + aPiece->mY;
				float aVY = aPiece->mVY + 0.1f;
				aPiece->mVY = aVY;
				aPiece->mZ = aPiece->mVZ + aPiece->mZ;
				aPiece->mVZ = aPiece->mVZ + 0.15f;
				aPiece->mRot = aPiece->mVRot + aPiece->mRot;
				bool aGone;
				if (!a3D || m165ac)
				{
					aPiece->mVY = aVY + 0.025f;
					float aScaledY = mApp->ScaleNum(aPiece->mY, 0.0f);
					aGone = (float)(mHeight + 0x54) <= aScaledY;
				}
				else
				{
					aGone = (float)(mHeight + 0x54) <= aPiece->mY && -2048.0f <= aPiece->mZ;
				}
				if (!aGone)
					anAllGone = false;
			}
		}
		if (anAllGone)
		{
			if (mScreenState == 1)
			{
				m15f24 = false;
				mState = 7;
				mApp->DoHighScoreWidget();
				MarkDirty();
				mApp->DoGameOver();
			}
		}
	}
	else
	{
		m15e58--;
		if (m15e58 == 0)
		{
			m15f24 = true;
			m15f18 = false;
			mApp->PlaySample(SOUND_EXPLODE);
		}
		if (mUpdateCnt % 6 == 0 || m15e58 == 0)
		{
			for (int aRow = 0; aRow < 8; aRow++)
			{
				for (int aCol = 0; aCol < 8; aCol++)
				{
					Piece* aPiece = mBoard[aRow][aCol];
					if (aPiece == NULL)
						continue;

					int aX = GetColX(aPiece->mCol);
					aPiece->mX = (float)(aX - 4 + Rand() % 9);
					int aY = GetRowY(aPiece->mRow);
					aPiece->mY = (float)(aY - 4 + Rand() % 9);
					if (m15e58 == 0)
					{
						if (!a3D)
						{
							aPiece->mVY = (float)(Rand() % 0xc - 8);
							aPiece->mVX = Rand() % 0xe - 6;
						}
						else
						{
							aPiece->mVY = (float)(Rand() % 6 - 4);
							aPiece->mVX = Rand() % 7 - 3;
							aPiece->mVZ = (float)(Rand() % 0xc);
							aPiece->mVRot = (float)((double)(Rand() % 2000 - 1000) * 0.00011);
						}
					}
				}
			}
		}
	}
}

// 0x5967d0: the 7 by 7 mesh that the hyperspace jump draws the board on ripples; mScale is the size of the ripple of a
// point and mX/mY its place in the mesh
void Board::UpdateHyperWarpPoints()
{
	for (int aRow = 0; aRow < 7; aRow++)
	{
		for (int aCol = 0; aCol < 7; aCol++)
		{
			WaterPoint* aPoint = &mWaterMesh[aRow][aCol];
			double aSin1 = sin((double)(aPoint->mAngle + aPoint->mAngle) + 1.570795);
			float aAngle4 = (float)((double)aPoint->mAngle * 4.0);
			float aCos = (float)cos((double)aAngle4);
			double aSin2 = sin((double)mUpdateCnt * 0.04);
			double aBase = aSin2 * aSin1 * (double)0.1f + 1.1;
			double aSin3 = sin((double)mUpdateCnt * 0.12);
			float aScale = (float)(aSin3 * (double)aCos * 0.15 + aBase);
			aPoint->mScale = aScale;
			aScale = (float)((double)mFadeIn * (double)aScale + (1.0 - (double)mFadeIn));
			aPoint->mScale = aScale;

			int anIndex = (int)((double)aPoint->mAngle * 4096.0 / 6.28318);
			anIndex = anIndex % 4096;
			float aX = (float)((double)gCosTable[anIndex] * (double)aPoint->mRadius * (double)aPoint->mScale);
			aPoint->mX = (float)((double)aX + 42.0);
			float aY = (float)((double)gSinTable[anIndex] * (double)aPoint->mRadius * (double)aPoint->mScale);
			aPoint->mY = (float)(42.0 + (double)aY);
		}
	}
}

// 0x5968f8: the order in which the gems are drawn: by depth (the gems sort as pointers to their pieces)
int Board::ComparePieceZ(Piece* const* theA, Piece* const* theB)
{
	if ((*theA)->mZ < (*theB)->mZ)
		return -1;
	if ((*theB)->mZ < (*theA)->mZ)
		return 1;
	return 0;
}

// 0x596931: a light at (theX, theY) shines on the gems around it: the eight light levels of a gem (mGlow) each grow by
// the part of the light that comes from the direction they face. theScale stretches the distances, theRadius is taken
// off the squared distance, theGain scales the light; theSkipPiece (the gem that is the light) is left out, and so are
// the flaming gems when theSkipFlames is set
void Board::AddPointLight(float theX, float theY, float theScale, float theRadius, float theGain, Piece* theSkipPiece, bool theSkipFlames)
{
	static const float aDirections[8][2] = {
		{0.0f, -1.0f},
		{-0.707f, -0.707f},
		{-1.0f, 0.0f},
		{-0.707f, 0.707f},
		{0.0f, 1.0f},
		{0.707f, 0.707f},
		{1.0f, 0.0f},
		{0.707f, -0.707f}};

	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece == NULL || aPiece == theSkipPiece)
				continue;
			if (aPiece->m70 && theSkipFlames)
				continue;

			float aDX = (float)((double)theX - ((double)aPiece->mX + 42.0));
			aDX = aDX / theScale;
			float aDY = (float)((double)theY - ((double)aPiece->mY + 42.0));
			aDY = aDY / theScale;
			float aDist = (float)((double)aDX * (double)aDX + (double)aDY * (double)aDY);
			aDist = (float)((double)aDist - (double)theRadius);
			if (!(1.0f < aDist))
				aDist = 1.0f;
			if (aDist < 100.0)
			{
				aDX = aDX / aDist;
				aDY = aDY / aDist;
				for (int i = 0; i < 8; i++)
				{
					float aLight = (float)(((double)aDirections[i][1] * (double)aDY + (double)aDirections[i][0] * (double)aDX) * (double)theGain);
					if (0.0f < aLight)
						aPiece->mGlow[i] = aPiece->mGlow[i] + aLight;
				}
			}
		}
	}
}
