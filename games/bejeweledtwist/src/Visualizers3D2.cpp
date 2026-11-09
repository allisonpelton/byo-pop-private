// FluffyStars, PlasmaClouds, SpaceThreads and VizWarpBars (Visualizers3D.h), and the mesh draw 0x75727e they share.
// As in Visualizers3D.cpp the original's float code runs on the x87 stack (rounded to float only where it stores), so
// expressions are written in double where the original's are; a constant printed short in the export is a double, and a
// long one (0.800000011920929) is a float.
#include "Visualizers3D.h"

#include "Board.h"
#include "Common.h"
#include "Compat2004.h"
#include "DDImage.h"
#include "Graphics.h"
#include "Profile.h"
#include "Res.h"
#include "SexyMatrix.h"
#include "TriVertex.h"
#include "TwistD3D.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <ctime>

using namespace Sexy;

// 0x401860: a colour as ARGB
static uint32_t PackColor(const Color& theColor)
{
	return ((((uint32_t)theColor.mAlpha << 8 | (uint32_t)theColor.mRed) << 8 | (uint32_t)theColor.mGreen) << 8) |
		(uint32_t)theColor.mBlue;
}

// ---------------------------------------------------------------------------------------------------------------------
// 0x75727e

void Sexy::DrawImageMeshRotated(Graphics* g, Image* theImage, float theX, float theY, double theRot, float theScaleX,
	float theScaleY, int theGrid)
{
	g->SetLinearBlend(true);  // 0x41ff80
	float aWidth = (float)((double)theImage->mWidth * (double)theScaleX);
	float aHeight = (float)((double)theImage->mHeight * (double)theScaleY);
	if (9 < theGrid)
		theGrid = 10;
	// port: TriVertex is 20 bytes, the original's 32 (x, y, z 0, rhw 1, diffuse, specular 0, u, v)
	static TriVertex aTriangles[200][3];
	TriVertex* aVertex = &aTriangles[0][0];
	uint32_t aColor = PackColor(g->mColor);
	float aGrid = (float)theGrid;
	int aRow = 0;
	if (0 < theGrid) {
		do {
			float aV0 = (float)aRow / aGrid;
			aRow++;
			float aV1 = (float)aRow / aGrid;
			double aY0 = (double)aV0 - 0.5;
			double aY1 = (double)aV1 - 0.5;
			int aColumn = 0;
			do {
				float aU0 = (float)aColumn / aGrid;
				aColumn++;
				float aU1 = (float)aColumn / aGrid;
				double aX0 = (double)aU0 - 0.5;
				double aX1 = (double)aU1 - 0.5;
				const double aCos = cos(theRot);
				const double aSin = sin(theRot);
				// (column, row), (column + 1, row), (column, row + 1), (column + 1, row + 1)
				const double aXs[4] = {aX0, aX1, aX0, aX1};
				const double aYs[4] = {aY0, aY0, aY1, aY1};
				const float aUs[4] = {aU0, aU1, aU0, aU1};
				const float aVs[4] = {aV0, aV0, aV1, aV1};
				TriVertex aCorner[4];
				for (int i = 0; i < 4; i++) {
					double aAcross = aCos * aXs[i] * (double)aWidth + (double)theX;
					float aPX = (float)(aSin * aYs[i] * (double)aHeight + aAcross);
					double aDown = aCos * aYs[i] * (double)aHeight + (double)theY;
					float aPY = (float)(aDown - aSin * aXs[i] * (double)aWidth);
					aCorner[i] = TriVertex(aPX, aPY, aUs[i], aVs[i], aColor);
				}
				aVertex[0] = aCorner[0];
				aVertex[1] = aCorner[1];
				aVertex[2] = aCorner[2];
				aVertex[3] = aCorner[3];
				aVertex[4] = aCorner[2];
				aVertex[5] = aCorner[1];
				aVertex += 6;
			} while (aColumn < theGrid);
		} while (aRow < theGrid);
	}
	g->DrawTrianglesTex(theImage, aTriangles, theGrid * theGrid * 2);  // 0x4221c0
}

// ---------------------------------------------------------------------------------------------------------------------
// FluffyStars

FluffyStars::FluffyStars(WinTwistApp* theApp) : Visualizer(theApp)
{
	mImage = LoadVizImage(IMAGE_FLUFFYSTARS_ID);
	mTimer = 0.0f;
	mCount = 0x32;
	for (int i = 0; i < 50; i++)
		mStars[i] = nullptr;
	mBrightness = 0.2f;  // 0x3e4ccccd
	mFlags4 |= 2;       // drawn at half size
}

FluffyStars::~FluffyStars()
{
	DeleteVizImage(IMAGE_FLUFFYSTARS_ID);
	int anIndex = 0;
	if (0 < mCount) {
		do {
			if (mStars[anIndex] != nullptr)
				delete mStars[anIndex];
			anIndex++;
		} while (anIndex < mCount);
	}
}

void FluffyStars::Init()
{
	int aCount = 1000;
	do {
		Update();
		aCount--;
	} while (aCount != 0);
}

void FluffyStars::Update()
{
	mTimer = (float)((double)mTimer - 0.01);
	if (mTimer <= 0.0f && 0 < mCount) {
		int anIndex = 0;
		Star** aSlot = mStars;
		do {
			if (*aSlot == nullptr) {
				Star* aStar = new Star();  // 0x2c bytes; its colour starts as black (0x401690)
				aStar->mColor = Color(0, 0, 0, 0xff);
				aStar->mAlpha = 0.0f;
				// Rand draws (RandSigned): the angle, x, y
				aStar->mAngle = (float)((double)RandSigned() * (double)3.1415927f);
				int aWidth = mApp->mWidth;
				aStar->mX = (float)((double)RandSigned() * (double)(aWidth / 4) + (double)(aWidth / 2));
				int aHeight = mApp->mHeight;
				aStar->mY = (float)((double)RandSigned() * (double)(aHeight / 4) + (double)(aHeight / 2));
				aStar->mScale = 0.35f;
				aStar->mGrowth = 0.0001f;
				float aSine = sinf((float)(((double)(float)(mApp->mUpdateCount % 200) / 200.0) * (double)3.1415927f));
				float aWave = std::fabs(aSine);
				int aKind = Ftol((double)aWave + (double)aWave);
				Color aColor;
				if (aKind == 0) {
					aStar->mMult = 2.5f;
					aStar->mGrowth = (float)((1.0 - (double)aWave) * 0.0008 + 5e-05 + (double)aStar->mGrowth);
					aStar->mScale = 0.35f;
					aColor = Color(0xff, 0, 0);
					mTimer = 0.12f;  // 0x3df5c28f
				} else {
					aStar->mMult = 3.0f;
					// the fourth Rand draw
					aStar->mGrowth = (float)((double)RandSigned() * 0.0005000000237487257 + (double)aStar->mGrowth);
					aColor = Color(0, 0, 0x80);
					mTimer = 0.08f;  // 0x3da3d70a
				}
				aStar->mColor = aColor;
				mStars[anIndex] = aStar;
				break;
			}
			anIndex++;
			aSlot++;
		} while (anIndex < mCount);
	}
	int anIndex = 0;
	if (0 < mCount) {
		do {
			Star* aStar = mStars[anIndex];
			if (aStar != nullptr) {
				float aScale = (float)((double)aStar->mGrowth + (double)aStar->mScale);
				aStar->mScale = aScale;
				if ((0.0f < aStar->mGrowth && 0.95 < (double)aScale) || (aStar->mGrowth < 0.0f && (double)aScale < 0.35)) {
					float anAlpha = (float)((double)aStar->mAlpha - 0.01);
					aStar->mAlpha = anAlpha;
					if (anAlpha <= 0.0f) {
						mStars[anIndex] = nullptr;
						delete aStar;
					}
				} else if (aStar->mAlpha < 1.0f) {
					float anAlpha = (float)((double)aStar->mAlpha + 0.01);
					if (1.0 < (double)anAlpha)
						anAlpha = 1.0f;
					aStar->mAlpha = anAlpha;
				}
			}
			anIndex++;
		} while (anIndex < mCount);
	}
}

void FluffyStars::Draw(Graphics* g)
{
	bool aIs3D = mApp->Is3DAccelerated();  // 0x4a7900
	g->SetColor(Color(0, 0, 0, 0xff));
	g->FillRect(0, 0, mApp->mWidth, mApp->mHeight);  // 0x420020
	if (!aIs3D) {
		g->DrawImage(mImage, 0, 0);  // 0x4211b0
		return;
	}
	float aCenterX = (float)(mApp->mWidth / 2);
	float aCenterY = (float)(mApp->mHeight / 2);
	g->SetDrawMode(1);
	g->SetColorizeImages(true);
	int anIndex = 0;
	if (0 < mCount) {
		do {
			const Star* aStar = mStars[anIndex];
			if (aStar != nullptr) {
				float aScale = (float)((double)aStar->mMult * (double)aStar->mScale);
				float anAlpha = (float)((double)aStar->mAlpha * 0.8);
				float aX = (float)(((double)aStar->mX - (double)aCenterX) * (double)aScale + (double)aCenterX);
				float aY = (float)(((double)aStar->mY - (double)aCenterY) * (double)aScale + (double)aCenterY);
				g->SetColor(Color(aStar->mColor.mRed, aStar->mColor.mGreen, aStar->mColor.mBlue,
					Ftol((double)anAlpha * 255.0)));
				DrawImageMeshRotated(g, mImage, aX, aY, (double)aStar->mAngle, aScale, aScale, 5);  // 0x75727e
			}
			anIndex++;
		} while (anIndex < mCount);
	}
	g->SetDrawMode(0);
}

// ---------------------------------------------------------------------------------------------------------------------
// PlasmaClouds

PlasmaClouds::PlasmaClouds(WinTwistApp* theApp) : Visualizer(theApp)
{
	mAngle = 0.0f;
	mNextImage = 0;
	mFlags4 |= 2;  // drawn at half size
	mCloud1 = LoadVizImage(IMAGE_CLOUD1_ID);
	mCloud2 = LoadVizImage(IMAGE_CLOUD2_ID);
	mCloud3 = LoadVizImage(IMAGE_CLOUD3_ID);
}

PlasmaClouds::~PlasmaClouds()
{
	DeleteVizImage(IMAGE_CLOUD1_ID);
	DeleteVizImage(IMAGE_CLOUD2_ID);
	DeleteVizImage(IMAGE_CLOUD3_ID);
	for (size_t i = 0; i < mClouds.size(); i++)
		delete mClouds[i];
}

void PlasmaClouds::Init()
{
	int aCount = 0x1e;
	do {
		Update();
		aCount--;
	} while (aCount != 0);
}

void PlasmaClouds::Update()
{
	// the eight colours the clouds take by the direction they are in (made once)
	static Color aColors[9];
	static bool aMade = false;
	if (!aMade) {
		aMade = true;
		aColors[0] = Color(0, 0xff, 0xff);
		aColors[1] = Color(0, 0, 0xff);
		aColors[2] = Color(0, 0, 0xff);
		aColors[3] = Color(0xff, 0, 0xff);
		aColors[4] = Color(0xff, 0, 0);
		aColors[5] = Color(0xff, 0x80, 0);
		aColors[6] = Color(0xff, 0xff, 0);
		aColors[7] = Color(0, 0xff, 0);
		// port: the original's index can reach 8 only where the direction is exactly the last one, and then reads the
		// word after the table; here those are zeros
		aColors[8] = Color(0, 0, 0, 0);
	}
	const float kTwoPi = 6.2831855f;  // 0x84e1e4
	while (true) {
		if (19 < (unsigned int)mClouds.size()) {
			unsigned int anIndex = 0;
			if (mClouds.size() != 0) {
				do {
					Cloud* aCloud = mClouds[anIndex];
					// the colour by the direction from the middle, mixed between two neighbours
					float aDirectionY = (float)aCloud->mY - (float)(mApp->mHeight / 2);
					float aDirectionX = (float)aCloud->mX - (float)(mApp->mWidth / 2);
					float aAngle = (float)atan2((double)aDirectionY, (double)aDirectionX);  // 0x4c1b70
					float aPosition = (float)(-(double)(float)((double)aAngle - 3.1415927410125732) /
						((double)kTwoPi * 0.125));
					float aFloor = (float)floor((double)aPosition);  // 0x401b40
					float aFraction = (float)((double)aPosition - (double)aFloor);
					int aFirst = Ftol((double)aPosition);
					int aSecond = (aFirst + 1) % 8;
					const Color& aA = aColors[aFirst];
					const Color& aB = aColors[aSecond];
					aCloud->mColor.mRed = Ftol((double)(aB.mRed - aA.mRed) * (double)aFraction + (double)aA.mRed);
					aCloud->mColor.mGreen = Ftol((double)(aB.mGreen - aA.mGreen) * (double)aFraction + (double)aA.mGreen);
					aCloud->mColor.mBlue = Ftol((double)(aB.mBlue - aA.mBlue) * (double)aFraction + (double)aA.mBlue);
					if (aCloud->mFade < 1) {
						if (0.0f < aCloud->mAlpha) {
							float anOld = aCloud->mAlpha;
							aCloud->mAlpha = (float)((double)anOld - 0.003);
							if ((double)anOld - 0.003 <= 0.0) {
								aCloud->mAlpha = 0.0f;
								aCloud->mDone = true;
							}
						}
					} else {
						aCloud->mFade--;
						if (aCloud->mAlpha < 1.0f) {
							float anOld = aCloud->mAlpha;
							aCloud->mAlpha = (float)((double)anOld + 0.01);
							if (1.0 < (double)anOld + 0.01)
								aCloud->mAlpha = 1.0f;
						}
					}
					aCloud->mColor.mAlpha = Ftol((double)aCloud->mAlpha * 255.0);
					Image* anImage = aCloud->mImage;
					float aX = (float)((double)aCloud->mX + (double)aCloud->mVX * 0.1);
					aCloud->mX = aX;
					float aY = (float)((double)aCloud->mVY * 0.1 + (double)aCloud->mY);
					aCloud->mY = aY;
					aCloud->mAngle = (float)((double)aCloud->mSpin * 0.05 + (double)aCloud->mAngle);
					float anOldScaleX = aCloud->mScaleX;
					aCloud->mScaleX = (float)((double)anOldScaleX * 1.0001);
					float anOldScaleY = aCloud->mScaleY;
					aCloud->mScaleY = (float)((double)anOldScaleY * 1.0001);
					int aWidth = Ftol((double)anImage->mWidth * ((double)anOldScaleX * 1.0001));
					int aHeight = Ftol((double)anImage->mHeight * ((double)anOldScaleY * 1.0001));
					if ((double)(float)(mApp->mWidth + aWidth / 2) < (double)aX || (double)aX < (double)(float)-aWidth * 0.5 ||
						(double)(float)(mApp->mHeight + aHeight / 2) < (double)aY ||
						(double)aY < (double)(float)-aHeight * 0.5)
						aCloud->mDone = true;
					if (aCloud->mDone) {
						// the last cloud takes its place (0x676c44), and the list's end moves in by one
						mClouds[anIndex] = mClouds[mClouds.size() - 1];
						if (mClouds.size() != 0)
							mClouds.pop_back();
						anIndex--;
						delete aCloud;
					}
					anIndex++;
				} while (anIndex < (unsigned int)mClouds.size());
			}
			return;
		}
		Cloud* aCloud = new Cloud();  // 0x676be5: its colour black, its alpha 0, not done
		aCloud->mColor = Color(0, 0, 0, 0xff);
		// the draws, in order: Rand() (the distance), Rand() % 40 twice (the scales), Rand() % 20 twice (the speeds),
		// Rand() (the fade), Rand() (the angle), Rand() (the spin)
		float aDistance = (float)(Rand() % mApp->mWidth);
		if (mNextImage == 3)
			mNextImage = 0;
		int aKind = mNextImage++;
		// port: as in the original, the cases run on into each other, so the third cloud's image is every cloud's
		if (aKind == 0) {
			aCloud->mImage = mCloud1;
			aKind = 1;
		}
		if (aKind == 1) {
			aCloud->mImage = mCloud2;
			aKind = 2;
		}
		if (aKind == 2)
			aCloud->mImage = mCloud3;
		float aCos = cosf(mAngle);
		aCloud->mX = (float)((double)aCos * (double)aDistance + (double)(mApp->mWidth / 2));
		float aSin = sinf(mAngle);
		aCloud->mY = (float)((double)aSin * (double)aDistance + (double)(mApp->mHeight / 2));
		aCloud->mScaleY = (float)((double)(float)(Rand() % 0x28) / 10.0 + 1.0);
		aCloud->mScaleX = (float)((double)(float)(Rand() % 0x28) / 10.0 + 1.0);
		aCloud->mVX = (float)((double)(float)(Rand() % 0x14) / 10.0 - 1.0);
		aCloud->mVY = (float)((double)(float)(Rand() % 0x14) / 10.0 - 1.0);
		aCloud->mAlpha = 0.0f;
		aCloud->mFade = Rand() % 900 + 100;
		aCloud->mAngle = (float)((double)(float)(Rand() % 0x275) * 0.01);
		aCloud->mSpin = (float)((double)(float)(Rand() % 100) * 0.0002 - 0.01);
		mClouds.push_back(aCloud);  // 0x677622
		float aNext = (float)((double)kTwoPi / 20.0 + (double)mAngle);
		mAngle = aNext;
		if (kTwoPi < aNext)
			mAngle = (float)((double)aNext - (double)kTwoPi);
	}
}

void PlasmaClouds::Draw(Graphics* g)
{
	bool aIs3D = mApp->Is3DAccelerated();  // 0x4a7900
	g->SetColor(Color(0, 0, 0, 0xff));
	g->FillRect(0, 0, mApp->mWidth, mApp->mHeight);  // 0x420020
	if (aIs3D) {
		g->SetDrawMode(1);
		g->SetColorizeImages(true);
		float aScaleX = (float)g->mDestImage->mWidth / (float)mApp->mWidth;
		float aScaleY = (float)g->mDestImage->mHeight / (float)mApp->mHeight;
		unsigned int aCount = (unsigned int)mClouds.size();
		unsigned int anIndex = 0;
		if (aCount != 0) {
			do {
				const Cloud* aCloud = mClouds[anIndex];
				g->SetColor(Color(aCloud->mColor.mRed, aCloud->mColor.mGreen, aCloud->mColor.mBlue,
					Ftol((double)aCloud->mColor.mAlpha * 0.8999999761581421)));
				float aY = (float)((double)aCloud->mY * (double)aScaleY * (double)gApp->mScale);
				float aX = (float)((double)aCloud->mX * (double)aScaleX * (double)gApp->mScale);
				float aSizeX = aCloud->mScaleX * aScaleX;
				DrawImageMeshRotated(g, aCloud->mImage, aX, aY, (double)aCloud->mAngle, aSizeX, aCloud->mScaleY * aScaleY,
					4);  // 0x75727e
				anIndex++;
			} while (anIndex < aCount);
		}
		g->SetColorizeImages(false);
		g->SetDrawMode(0);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// SpaceThreads

SpaceThreads::SpaceThreads(WinTwistApp* theApp) : Visualizer(theApp)
{
	mCount = 0;
	mCursor = 0;
	mTriCount = 0;
	mThread = LoadVizImage(IMAGE_SPACE_THREAD_ID);
	mBackground = LoadVizImage(IMAGE_SPACETHREADS_BG_ID);
	mBrightness = 0.2f;  // 0x3e4ccccd
}

SpaceThreads::~SpaceThreads()
{
	DeleteVizImage(IMAGE_SPACE_THREAD_ID);
	DeleteVizImage(IMAGE_SPACETHREADS_BG_ID);
}

void SpaceThreads::Init()
{
	// the first draw is a plain Rand(); then four RandSigned for each point, in the order A, B, (C from the column), D, E
	mCount = Rand() % 1000 + 360000;
	int aRow = 0x14;
	Point* aPoint = &mPoints[0][0];
	do {
		int aColumn = 0;
		do {
			aPoint->mA = RandSigned();
			aPoint->mB = RandSigned();
			aPoint->mC = (float)((double)(float)aColumn / 15.0 - 1.0);
			aPoint->mD = RandSigned();
			aPoint->mE = RandSigned();
			aColumn++;
			aPoint++;
		} while (aColumn < 0x2d);
		aRow--;
	} while (aRow != 0);
	int aCount = 0x4b0;
	do {
		Update();
		aCount--;
	} while (aCount != 0);
}

void SpaceThreads::Update()
{
	mCount++;
	if (mCount % 0x1e == 0) {
		// the thread's next column (the cursor's) is set from the count, and the speeds of the last one are smoothed
		int anOld = mCursor;
		mCursor = (anOld + 1) % 0x2d;
		int aRow = 0;
		int aStep = 0;
		do {
			aRow++;
			Point& aNew = mPoints[aStep / 0x2d][mCursor];
			Point& anOldPoint = mPoints[aStep / 0x2d][anOld];
			aNew.mC = 1.1f;  // 0x3f8ccccd
			float aT = (float)((double)(float)mCount * 0.00175 + (double)((((float)aRow + (float)aRow) * 3.141592) / 20.0));
			float aCos1 = cosf((float)((double)aT * 1.1));
			float aSin1 = sinf((float)((double)aT * 4.1));
			aNew.mA = (float)((double)aCos1 * 0.75 + (double)aSin1 * 3.0);
			float aCos2 = cosf((float)((double)aT * 3.0));
			float aSin2 = sinf((float)((double)aT * 1.7));
			aNew.mB = (float)((double)aCos2 * 3.0 + (double)aSin2 * 0.75);
			aNew.mD = aNew.mA - anOldPoint.mA;
			aNew.mE = aNew.mB - anOldPoint.mB;
			aStep += 0x2d;
		} while (aStep < 900);
		aStep = 0;
		int aPrevious = (anOld + 0x2c) % 0x2d;
		do {
			Point& aNew = mPoints[aStep / 0x2d][mCursor];
			Point& anOldPoint = mPoints[aStep / 0x2d][anOld];
			Point& aPrev = mPoints[aStep / 0x2d][aPrevious];
			anOldPoint.mD = (float)(((double)aNew.mD + (double)aPrev.mD) * 0.5 + (double)anOldPoint.mD);
			anOldPoint.mE = (float)(((double)aNew.mE + (double)aPrev.mE) * 0.5 + (double)anOldPoint.mE);
			anOldPoint.mD = (float)((double)anOldPoint.mD * 0.5);
			anOldPoint.mE = (float)((double)anOldPoint.mE * 0.5);
			aStep += 0x2d;
		} while (aStep < 900);
	}
	// every point sinks a little
	int aRow = 0x14;
	Point* aPoint = &mPoints[0][0];
	do {
		int aColumn = 0x2d;
		do {
			aPoint->mC = (float)((double)aPoint->mC - 0.0025);
			aPoint++;
			aColumn--;
		} while (aColumn != 0);
		aRow--;
	} while (aRow != 0);
}

void SpaceThreads::AddQuad(const Color& theColor, int theThread, int thePoint)
{
	const Point& aPoint = mPoints[theThread][thePoint];
	const Point& aNextPoint = mPoints[theThread][(thePoint + 1) % 0x2d];
	float aWidth = (float)mApp->mWidth;
	float aHeight = (float)mApp->mHeight;
	float aGap = (float)((double)aPoint.mC - (double)aNextPoint.mC);
	if (!((double)std::fabs(aGap) <= 0.800000011920929 && -2.0f <= aPoint.mC))
		return;
	uint32_t aColor = PackColor(theColor);
	float aWeight = (float)(((double)aPoint.mC + 1.0) * (1.0 - (double)aPoint.mC) * 0.800000011920929);
	if (aWeight < 0.0f)
		aWeight = 0.0f;
	// the point's two sides, A - E w, B + D w and A + E w, B - D w, in perspective by its depth
	float aTX = (float)((double)aPoint.mA - (double)aPoint.mE * (double)aWeight);
	float aSideD = (float)((double)aPoint.mD * (double)aWeight);
	float aTY = (float)((double)aPoint.mB + (double)aSideD);
	double aDepth = (double)aPoint.mC + 3.700000047683716;
	float aV1X = (float)((((double)aPoint.mE * (double)aWeight + (double)aPoint.mA) / aDepth + 1.0) * (double)aWidth * 0.5);
	float aV1Y = (float)((double)aHeight * (((double)aPoint.mB - (double)aSideD) / aDepth + 1.0) * 0.5);
	float aDepth2 = (float)((double)aPoint.mC + 3.7);
	float aV2X = (float)((double)aWidth * ((double)aTX / (double)aDepth2 + 1.0) * 0.5);
	float aV2Y = (float)((double)aHeight * ((double)aTY / (double)aDepth2 + 1.0) * 0.5);
	// the next point
	float aWeight2 = (float)(((double)aNextPoint.mC + 1.0) * (1.0 - (double)aNextPoint.mC) * 0.8);
	if (aWeight2 < 0.0f)
		aWeight2 = 0.0f;
	float aQX = (float)((double)aNextPoint.mA - (double)aNextPoint.mE * (double)aWeight2);
	float aSideD2 = (float)((double)aNextPoint.mD * (double)aWeight2);
	float aQY = (float)((double)aNextPoint.mB + (double)aSideD2);
	float aDepth3 = (float)((double)aNextPoint.mC + 3.7);
	float aV3X = (float)((((double)aNextPoint.mE * (double)aWeight2 + (double)aNextPoint.mA) / (double)aDepth3 + 1.0) * (double)aWidth * 0.5);
	float aV3Y = (float)((double)aHeight * (((double)aNextPoint.mB - (double)aSideD2) / (double)aDepth3 + 1.0) * 0.5);
	float aV0X = (float)((double)aWidth * ((double)aQX / (double)aDepth3 + 1.0) * 0.5);
	float aV0Y = (float)((double)aHeight * ((double)aQY / (double)aDepth3 + 1.0) * 0.5);
	TriVertex aV0(aV0X, aV0Y, 1.0f, 0.5f, aColor);
	TriVertex aV1(aV1X, aV1Y, 0.0f, 0.5f, aColor);
	TriVertex aV2(aV2X, aV2Y, 1.0f, 0.5f, aColor);
	TriVertex aV3(aV3X, aV3Y, 0.0f, 0.5f, aColor);
	// the original's array has room for 1800 triangles, what 20 threads of 45 points make at most
	mTriangles[mTriCount][0] = aV1;
	mTriangles[mTriCount][1] = aV3;
	mTriangles[mTriCount][2] = aV2;
	mTriCount++;
	mTriangles[mTriCount][0] = aV3;
	mTriangles[mTriCount][1] = aV2;
	mTriangles[mTriCount][2] = aV0;
	mTriCount++;
}

void SpaceThreads::Draw(Graphics* g)
{
	mTriCount = 0;
	g->PushState();  // 0x41fd70
	g->SetClipRect(0, 0, gApp->mWidth, gApp->mHeight);  // 0x4222c0
	g->SetColorizeImages(false);
	g->SetColor(Color(0));
	g->FillRect(0, 0, gApp->mWidth, gApp->mHeight);  // 0x420020
	DrawImage(g, mBackground);  // 0x67fdb6
	g->SetColorizeImages(true);
	// the seven colours the threads cycle through (made once)
	static Color aColors[7];
	static bool aMade = false;
	if (!aMade) {
		aMade = true;
		aColors[0] = Color(0x291fc0);
		aColors[1] = Color(0x2996ff);
		aColors[2] = Color(0xa0a0ff);
		aColors[3] = Color(0xdddddd);
		aColors[4] = Color(0xffa0a0);
		aColors[5] = Color(0xffa0ff);
		aColors[6] = Color(0xff3b32);
	}
	// port: the original pushes the state again here and pops it only once at the end, leaving one on g's stack
	g->PushState();
	g->SetColorizeImages(true);
	g->SetDrawMode(1);
	int aThread = 0;
	do {
		float aT = (float)((double)(float)aThread * 0.25 + (double)(float)mCount * 0.01);
		float aFloor = (float)floor((double)aT);  // 0x401b40
		float aFraction = (float)((double)aT - (double)aFloor);
		int aFirst = Ftol((double)aT);
		const Color& aA = aColors[aFirst % 7];
		const Color& aB = aColors[(aFirst + 1) % 7];
		int aRed = Ftol((double)(aB.mRed - aA.mRed) * (double)aFraction + (double)aA.mRed);
		int aGreen = Ftol((double)(aB.mGreen - aA.mGreen) * (double)aFraction + (double)aA.mGreen);
		int aBlue = Ftol((double)(aB.mBlue - aA.mBlue) * (double)aFraction + (double)aA.mBlue);
		Color aColor(aRed, aGreen, aBlue, 200);
		int aPoint = 0;
		do {
			AddQuad(aColor, aThread, aPoint);  // 0x67be27
			aPoint++;
		} while (aPoint < 0x2d);
		aThread++;
	} while (aThread < 0x14);
	g->PopState();  // 0x41fe00
	gD3DInterface->DrawTrianglesTex(mThread, mTriangles, mTriCount, Color::White, 1, 0.0f, 0.0f, true);  // slot 0x74
}

// ---------------------------------------------------------------------------------------------------------------------
// VizWarpBars

// 0x68ed45: a bar's start, drawn again until it is 80 or more from the middle (CrtRand: four draws a try)
void VizWarpBars::Bar::Start(WinTwistApp* theApp)
{
	while (true) {
		int aWidth = theApp->mWidth;
		mX = (float)(CrtRand() % aWidth - aWidth / 2);
		int aHeight = theApp->mHeight;
		mY = (float)(CrtRand() % aHeight - aHeight / 2);
		mZ = (float)(CrtRand() % 1024 - 0x200);
		int aSpeed = CrtRand();
		mSpeed = (float)((double)(float)(aSpeed % 1000) * 0.001);
		float aDX = std::fabs(mX);
		float aDY = std::fabs(mY);
		if (80.0 <= (double)aDY + (double)aDX)
			break;
	}
}

VizWarpBars::VizWarpBars(WinTwistApp* theApp) : Visualizer(theApp)
{
	mCount = 0;
	// the flag 14 of the profile (0x733897): set once the tutorial has been taken
	mTutorial = !mApp->mProfile->HasSeenTip(0xe);
	mBlur = LoadVizImage(IMAGE_COREWARP_BLUR_ID);
	mBackground = LoadVizImage(mTutorial ? IMAGE_VIRTUTORIAL_ID : IMAGE_COREWARP_BG_ID);
	mBrightness = 0.2f;  // 0x3e4ccccd
}

VizWarpBars::~VizWarpBars()
{
	DeleteVizImage(IMAGE_COREWARP_BLUR_ID);
	DeleteVizImage(mTutorial ? IMAGE_VIRTUTORIAL_ID : IMAGE_COREWARP_BG_ID);
	for (size_t i = 0; i < mBars.size(); i++)
		delete mBars[i];
	mBars.clear();
}

void VizWarpBars::Init()
{
	int aCount = 400;
	do {
		Bar* aBar = new Bar();
		aBar->Start(mApp);      // 0x68eea5
		mBars.push_back(aBar);  // 0x673a96
		aCount--;
	} while (aCount != 0);
}

void VizWarpBars::Update()
{
	mCount++;
	for (size_t anIndex = 0; anIndex < mBars.size(); anIndex++) {
		Bar* aBar = mBars[anIndex];
		float aZ = (float)((double)aBar->mZ + 0.4);
		aBar->mZ = aZ;
		float aScale = (float)((0.01 / (1.0 - ((double)aZ * 0.0009765625 + 0.5) * 0.99)) * 0.95 + 0.05);
		float aX = (float)((double)aBar->mX * (double)aScale + 512.0);
		float aY = (float)((double)aBar->mY * (double)aScale + 512.0);
		if (aX < -50.0 || (float)(mApp->mWidth + 0x32) < aX || aY < -50.0 || (float)(mApp->mHeight + 0x32) < aY ||
			512.0 < aZ) {
			aBar->Start(mApp);  // 0x68ed45
			aBar->mZ = -512.0f;
		}
	}
}

void VizWarpBars::Draw(Graphics* g)
{
	g->PushState();
	g->SetColorizeImages(true);
	g->SetColor(Color(0xe1, 0xe1, 0xe1));
	DrawImage(g, mBackground);  // 0x67fdb6
	g->SetDrawMode(1);
	unsigned int aNumber = 0;
	for (size_t anIndex = 0; anIndex < mBars.size(); anIndex++) {
		const Bar* aBar = mBars[anIndex];
		aNumber++;
		float aDepth = (float)((double)aBar->mZ * 0.0009765625 + 0.5);
		if (0.0f <= aDepth) {
			if (1.0f < aDepth)
				aDepth = 1.0f;
		} else {
			aDepth = 0.0f;
		}
		float aStep = (float)((double)(float)(int)(aNumber % 4 + 1) * 0.25);
		float aFade = (float)(((double)aBar->mZ + 512.0) * 0.005);
		if (1.0f < aFade)
			aFade = 1.0f;
		int anAlpha = Ftol((double)aStep * 255.0 * (double)aFade);
		if (!mTutorial) {
			int aBlue = Ftol((double)aBar->mSpeed * 30.0);
			int aGreen = Ftol((double)aBar->mSpeed * 0.0 + 71.0);
			g->SetColor(Color(0x7f, aGreen, aBlue, anAlpha));
		} else {
			int aRed = Ftol((double)aBar->mSpeed * 30.0 + 80.0);
			int aGreen = Ftol((double)aBar->mSpeed * 0.0 + 71.0);
			g->SetColor(Color(aRed, aGreen, 0x7f, anAlpha));
		}
		float aSizeY = (float)((0.6 / (double)aStep) / (1.0 - (double)aDepth * 0.99));
		Transform aTransform;  // 0x44f9d0
		float aSizeX = (float)((double)aSizeY * 0.25 * (double)aStep);
		aTransform.Scale(aSizeX, aSizeY);  // 0x44fae0
		float anAngle = (float)atan2((double)aBar->mX, (double)aBar->mY);  // 0x4c1b70
		aTransform.RotateRad((float)((double)anAngle + 0.0));  // 0x44fa70
		Rect aSource(0, 0, mBlur->mWidth, mBlur->mHeight);
		float aReach = (float)((double)aSizeY * 0.95 + 0.25);
		float aDrawX = (float)((double)aReach * (double)aBar->mX + (double)(float)(mApp->mWidth / 2));
		float aDrawY = (float)((double)aBar->mY * (double)aReach + (double)(float)(mApp->mHeight / 2));
		g->DrawImageTransformF(mBlur, aTransform, aSource, aDrawX, aDrawY);  // 0x4220f0
	}
	g->PopState();
}

// ---------------------------------------------------------------------------------------------------------------------
// VizKaleidoCat

// 0x689959: the seed of 0x689985's generator, shared by every instance
static uint32_t gKaleidoSeed = 0;

// 0x689985: the C runtime's rand's formula on gKaleidoSeed
static unsigned int KaleidoRand()
{
	gKaleidoSeed = gKaleidoSeed * 0x343fd + 0x269ec3;
	return (gKaleidoSeed >> 16) & 0x7fff;
}

// 0x67fac2 (a Visualizer's): theA, or by how far a blitz game has come theB: theB at its start and after its end
// (0x160 of the board is 0, 6 or 7), theA otherwise
static float KaleidoFactor(WinTwistApp* theApp, float theA, float theB)
{
	float aResult = theA;
	if (theApp != nullptr && theApp->mBoard != nullptr && theApp->mBoard->mGameMode == Board::MODE_BLITZ) {
		Board* aBoard = theApp->mBoard;
		int aState = aBoard->mPlayState;
		if (aState == 0) {
			float aProgress = (float)((double)(float)(aBoard->mBlitzStartTime - 100) / 29900.0);
			float aLimited = aProgress;
			if (1.0f <= aProgress)
				aLimited = 1.0f;
			float aClamped = 0.0f;
			if (0.0f < aLimited) {
				aClamped = 1.0f;
				if (aProgress < 1.0f)
					aClamped = aProgress;
			}
			aClamped = (float)(1.0 - (double)aClamped);
			theB = (float)(((double)theB - (double)theA) * (double)aClamped * (double)aClamped * (double)aClamped +
				(double)theA);
			aResult = theB;
		} else if (aState == 6 || aState == 7) {
			aResult = theB;
		}
	}
	return aResult;
}

// 0x6899a5: the cat's direction turned theDegrees
static void TurnNode(VizKaleidoCat::Node* theNode, float theDegrees)
{
	Transform aTransform;  // 0x44f9d0
	aTransform.RotateRad((float)(((double)theDegrees * 3.1415927410125732) / 180.0));  // 0x44fab0
	const SexyTransform2D& aMatrix = aTransform.GetMatrix();  // 0x44fc10
	float aX = (float)((double)theNode->mDX * (double)aMatrix.m00 + (double)aMatrix.m01 * (double)theNode->mDY +
		(double)aMatrix.m02);  // 0x44f3b0
	float aY = (float)((double)aMatrix.m11 * (double)theNode->mDY + (double)aMatrix.m10 * (double)theNode->mDX +
		(double)aMatrix.m12);
	theNode->mDX = aX;
	theNode->mDY = aY;
}

// 0x689a64: a new cat at the middle of theCats' picture, going somewhere, with a program of its own (the generator's
// draws: the hue, 64 for the program, one for where it ends, the turn, the step, the direction)
static void InitNode(VizKaleidoCat::Node* theNode, const VizKaleidoCat::Cats* theCats)
{
	theNode->mX = (float)(theCats->mWidth / 2);
	theNode->mY = (float)(theCats->mHeight / 2);
	theNode->mDX = 1.0f;
	theNode->mDY = 0.0f;
	theNode->mScale = 1.0f;
	theNode->mColor = (int)(KaleidoRand() & 0x1f) + 0x70;
	// 0x6899eb
	int anIndex = 0;
	do {
		unsigned int aDraw = KaleidoRand();
		theNode->mProgram[anIndex] = (unsigned char)(aDraw % 3 + 1);
		anIndex++;
	} while (anIndex < 0x40);
	unsigned int anEnd = KaleidoRand();
	theNode->mProgram[(anEnd & 0x1f) + 0x20] = 0;
	unsigned int aTurn = KaleidoRand();
	theNode->mTurn = (float)(((double)(float)(aTurn & 0x7fff) / 32767.0) * 25.0);
	unsigned int aStep = KaleidoRand();
	theNode->mStep = (float)((aStep & 1) + 2);
	unsigned int aDirection = KaleidoRand();
	TurnNode(theNode, (float)(((double)(float)(aDirection & 0x7fff) / 32767.0) * 360.0));
}

// 0x68a4df: theNode in the ring just after theAfter
static void InsertNodeAfter(VizKaleidoCat::Node* theNode, VizKaleidoCat::Node* theAfter)
{
	theNode->mPrev->mNext = theNode->mNext;
	theNode->mNext->mPrev = theNode->mPrev;
	theNode->mNext = theNode;
	theNode->mPrev = theAfter;
	VizKaleidoCat::Node* aNext = theAfter->mNext;
	theNode->mNext = aNext;
	aNext->mPrev = theNode;
	theNode->mPrev->mNext = theNode;
}

// 0x68a7c4: a copy of the cat, in a ring of its own, at the start of its program
static VizKaleidoCat::Node* CloneNode(const VizKaleidoCat::Node* theNode)
{
	VizKaleidoCat::Node* aNode = new VizKaleidoCat::Node();
	aNode->mX = theNode->mX;
	aNode->mY = theNode->mY;
	aNode->mDX = theNode->mDX;
	aNode->mDY = theNode->mDY;
	aNode->mScale = theNode->mScale;
	aNode->mColor = theNode->mColor;
	memcpy(aNode->mProgram, theNode->mProgram, sizeof(aNode->mProgram));
	aNode->mTurn = theNode->mTurn;
	aNode->mStep = theNode->mStep;
	return aNode;
}

// 0x68a9b0: the cat out of its ring and freed
static void DeleteNode(VizKaleidoCat::Node* theNode)
{
	theNode->mPrev->mNext = theNode->mNext;
	theNode->mNext->mPrev = theNode->mPrev;
	theNode->mNext = theNode;
	theNode->mPrev = theNode;
	delete theNode;
}

// 0x68a828: a cat's step; false when its program is over (it is then taken out, and a copy of it goes on while it
// is small)
static bool StepNode(VizKaleidoCat::Node* theNode)
{
	while (true) {
		if (0.0f < theNode->mTimer) {
			float aStepX = (float)((double)theNode->mDX * 0.25);
			float aStepY = (float)((double)theNode->mDY * 0.25);
			theNode->mX = theNode->mX + aStepX;
			theNode->mY = theNode->mY + aStepY;
			theNode->mScale = (float)((double)theNode->mScale * 1.0125);
			theNode->mTimer = (float)((double)theNode->mTimer - 0.25);
			return true;
		}
		unsigned char anOp = theNode->mProgram[theNode->mPC];
		if (anOp == 0)
			break;
		if (anOp == 1) {
			theNode->mTimer = theNode->mStep;
		} else if ((unsigned char)(anOp - 2) < 2) {
			float aTurn = theNode->mTurn;
			if (anOp == 3)
				aTurn = -aTurn;
			TurnNode(theNode, aTurn);
		}
		theNode->mPC++;
	}
	if ((double)theNode->mScale < 15.0) {
		VizKaleidoCat::Node* aClone = CloneNode(theNode);
		aClone->mTurn = (float)((double)aClone->mTurn * 1.25);
		InsertNodeAfter(aClone, theNode);
	}
	return false;
}

VizKaleidoCat::Cats::~Cats()
{
	// 0x68ab53: the ring emptied
	while (mHead.mPrev != &mHead)
		DeleteNode(mHead.mPrev);
}

void VizKaleidoCat::Cats::SetSize(int theWidth, int theHeight)
{
	mWidth = theWidth;
	mHeight = theHeight;
	InitNode(&mHead, this);  // the ring's start is made like a cat, and never steps
	Spawn();
	Spawn();
	Spawn();
}

void VizKaleidoCat::Cats::Spawn()
{
	Node* aNode = new Node();  // 0x68a67f
	InitNode(aNode, this);
	InsertNodeAfter(aNode, &mHead);
}

void VizKaleidoCat::Cats::StepAll()
{
	// the ring from its end backwards to its start, the next one taken before this one may be freed
	Node* aCurrent = mHead.mPrev;
	Node* aNext = aCurrent->mPrev;
	while (aCurrent != &mHead) {
		if (!StepNode(aCurrent) && aCurrent != nullptr)
			DeleteNode(aCurrent);
		aCurrent = aNext;
		aNext = aNext->mPrev;
	}
}

VizKaleidoCat::VizKaleidoCat(WinTwistApp* theApp) : Visualizer(theApp)
{
	mCanvas = nullptr;
	mKept = nullptr;
	mCount = 0;
	mCountBase = 0;
	mSnackers = IMAGE_SNACKERS;
	mFlags4 = (mFlags4 & ~1) | 8;
	gKaleidoSeed = 0x12345678;
	mSeeded = false;
	mCats = new Cats();
	mCats->SetSize(mApp->mWidth / 4, mApp->mHeight / 4);
}

VizKaleidoCat::~VizKaleidoCat()
{
	// 0x68abe2
	if (mCats != nullptr) {
		delete mCats;
		mCats = nullptr;
	}
	if (mKept != nullptr) {
		delete mKept;
		mKept = nullptr;
	}
	if (mCanvas != nullptr) {
		delete mCanvas;
		mCanvas = nullptr;
	}
}

bool VizKaleidoCat::EnsureImage(DDImage*& theImage)
{
	bool aMade = false;
	if (theImage == nullptr) {
		theImage = new DDImage(gSexyAppBase->mDDInterface);
		theImage->mD3DFlags = 0x18;
		theImage->SetImageMode(false, false);
		theImage->Create(mCats->mWidth, mCats->mHeight);
		aMade = true;
	}
	if (theImage != nullptr && gD3DInterface->SetRenderTarget(theImage)) {
		if (aMade)
			gD3DInterface->Clear(0, 0, 0, 0xff);
		return true;
	}
	return false;
}

void VizKaleidoCat::Update()
{
	mCount++;
	if (mCats == nullptr)
		return;
	if (999 < mCount && !mSeeded) {
		gKaleidoSeed = (uint32_t)time(nullptr);  // port: the original's __time64 (its low 32 bits)
		mSeeded = true;
	}
	if (mCats->mHead.mPrev == &mCats->mHead)
		mCats->Spawn();  // 0x68a8f6: none left
	mCats->StepAll();
	float aFactor = KaleidoFactor(mApp, 1.0f, 2.0f);  // 0x67fac2
	Cats* aCats = mCats;
	Node* aCurrent = aCats->mHead.mPrev;
	Node* aNext = aCurrent->mPrev;
	if (aCurrent == &aCats->mHead)
		return;
	int aHueShift = Ftol(((double)aFactor - 1.0) * -152.0);
	do {
		float aSize = (float)((double)aCurrent->mScale * 0.025);
		// (a Transform scaled by aSize is made here (0x44f9d0, 0x44fae0) and never used)
		float aFade = (float)((double)aCurrent->mScale / 44.0);
		if (aFade <= 0.0f)
			aFade = 0.0f;
		else if (1.0f <= aFade)
			aFade = 1.0f;
		unsigned int aRGB = mApp->HSLToRGB((aCurrent->mColor - aHueShift) & 0xff, 0xff, 0x80);  // 0x4a7220
		int anAlpha = Ftol((double)aFade * 255.0);  // 0x77e016 cuts toward zero
		Particle aParticle;
		aParticle.mX = aCurrent->mX;
		aParticle.mY = aCurrent->mY;
		aParticle.mSize = aSize;
		aParticle.mColor = Color((int)((aRGB & 0xffffff) | ((unsigned int)anAlpha << 24)));  // 0x4016b0: no alpha is opaque
		aParticle.mAge = 0.0f;
		if (mUpdateCnt % 10 == 0)
			aCats->mParticles.push_back(aParticle);  // 0x68af9f
		EnsureImage(mKept);  // 0x689b0d
		Graphics aG(mKept);  // 0x41fc60
		int anIndex = 0;
		while (anIndex < (int)aCats->mParticles.size()) {
			Particle& aStamp = aCats->mParticles[anIndex];
			float anAge = (float)((double)aStamp.mAge + 0.02);
			aStamp.mAge = anAge;
			if (anAge < 1.0f) {
				anIndex++;
			} else {
				// it lands: stamped into the kept picture, and gone
				aG.SetColorizeImages(true);
				aG.SetColor(aStamp.mColor);
				DrawImageScaledCentered(&aG, mSnackers, aStamp.mX, aStamp.mY, aStamp.mSize, aStamp.mSize);  // 0x757751
				aCats->mParticles.erase(aCats->mParticles.begin() + anIndex);
			}
		}
		Node* aFollowing = aNext;
		aNext = aNext->mPrev;
		aCurrent = aFollowing;
	} while (aCurrent != &aCats->mHead);
}

void VizKaleidoCat::Draw(Graphics* g)
{
	g->PushState();
	float aFactor = KaleidoFactor(mApp, 1.0f, 2.0f);  // 0x67fac2
	EnsureImage(mCanvas);  // 0x689be3
	Cats* aCats = mCats;
	{
		Graphics aG(mCanvas);
		if (mKept == nullptr) {
			aG.SetColor(Color(0, 0, 0));
			aG.FillRect(0, 0, mWidth, mHeight);  // 0x420020
		} else {
			aG.DrawImage(mKept, 0, 0);  // 0x4211b0
		}
		aG.SetColorizeImages(true);
		for (size_t i = 0; i < aCats->mParticles.size(); i++) {
			const Particle& aStamp = aCats->mParticles[i];
			aG.SetColorizeImages(true);
			aG.SetColor(Color(aStamp.mColor.mRed, aStamp.mColor.mGreen, aStamp.mColor.mBlue,
				Ftol((double)aStamp.mColor.mAlpha * (double)aStamp.mAge)));
			DrawImageScaledCentered(&aG, mSnackers, aStamp.mX, aStamp.mY, aStamp.mSize, aStamp.mSize);  // 0x757751
		}
	}
	gD3DInterface->SetRenderTarget(g->mDestImage);  // slot 0x98: the screen again
	// the wedges: the sixteen points round the edge of the picture, and a pair of texture points for each, turning
	static const float kPerimeter[32] = {0.0f, 0.0f, 0.25f, 0.0f, 0.5f, 0.0f, 0.75f, 0.0f, 1.0f, 0.0f, 1.0f, 0.25f, 1.0f,
		0.5f, 1.0f, 0.75f, 1.0f, 1.0f, 0.75f, 1.0f, 0.5f, 1.0f, 0.25f, 1.0f, 0.0f, 1.0f, 0.0f, 0.75f, 0.0f, 0.5f,
		0.0f, 0.25f};
	TriVertex aTriangles[16][3];
	float aDestWidth = (float)g->mDestImage->mWidth;
	float aDestHeight = (float)g->mDestImage->mHeight;
	int anIndex = 0;
	do {
		float aPoints[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		float aCos = cosf(1.1781f);  // 0x3f96cbe4
		float aSin = sinf(1.1781f);
		float aNegCos = -aCos;
		aPoints[0] = aCos;
		aPoints[2] = aNegCos;
		if ((anIndex & 1) != 0) {
			aPoints[2] = aCos;
			aPoints[0] = aNegCos;
		}
		aPoints[1] = aSin;
		aPoints[3] = aSin;
		for (int aPair = 0; aPair < 2; aPair++) {
			float* aPoint = aPoints + aPair * 2;
			Transform aTransform;  // 0x44f9d0
			float anAngle = (float)((double)(float)(mCount - mCountBase) * 0.125 * (double)aFactor);
			aTransform.RotateRad((float)(((double)anAngle * 3.1415927410125732) / 180.0));  // 0x44fab0
			const SexyTransform2D& aMatrix = aTransform.GetMatrix();
			float aTurnedX = (float)((double)aPoint[0] * (double)aMatrix.m00 + (double)aMatrix.m01 * (double)aPoint[1] +
				(double)aMatrix.m02);  // 0x44f3b0
			float aTurnedY = (float)((double)aMatrix.m11 * (double)aPoint[1] + (double)aMatrix.m10 * (double)aPoint[0] +
				(double)aMatrix.m12);
			aPoint[0] = aTurnedX;
			aPoint[1] = aTurnedY;
			// 0x673b6f: made one long, then a quarter of that round the picture's middle
			float aLength = (float)sqrt((double)(aPoint[0] * aPoint[0] + aPoint[1] * aPoint[1]));  // 0x618620
			float aUnitX = aPoint[0];
			float aUnitY = aPoint[1];
			if (aLength != 0.0f) {
				aUnitX = aPoint[0] / aLength;
				aUnitY = aPoint[1] / aLength;
			}
			aPoint[0] = (float)((double)aUnitX * 0.25);
			aPoint[1] = (float)((double)aUnitY * 0.25);
			aPoint[0] = (float)((double)aPoint[0] + 0.5);
			aPoint[1] = (float)((double)aPoint[1] + 0.5);
		}
		int aNextIndex = (anIndex + 1) % 16;
		// port: TriVertex is 20 bytes, the original's 32 (z 0, rhw 1, specular 0 in between)
		aTriangles[anIndex][0] = TriVertex(aDestWidth * kPerimeter[anIndex * 2], aDestHeight * kPerimeter[anIndex * 2 + 1],
			aPoints[0], aPoints[1], 0xffffffff);
		aTriangles[anIndex][1] = TriVertex(kPerimeter[aNextIndex * 2] * aDestWidth,
			kPerimeter[aNextIndex * 2 + 1] * aDestHeight, aPoints[2], aPoints[3], 0xffffffff);
		aTriangles[anIndex][2] = TriVertex((float)(g->mDestImage->mWidth / 2), (float)(g->mDestImage->mHeight / 2), 0.5f,
			0.5f, 0xffffffff);
		anIndex++;
	} while (anIndex < 16);
	gD3DInterface->DrawTrianglesTex(mCanvas, aTriangles, 16, Color(0xff, 0xff, 0xff, 0xff), 0, 0.0f, 0.0f, true);  // slot 0x74
	g->PopState();
}
