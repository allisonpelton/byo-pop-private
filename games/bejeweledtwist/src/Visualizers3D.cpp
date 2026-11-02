#include "Visualizers3D.h"

#include "Compat2004.h"
#include "DDImage.h"
#include "Graphics.h"
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

using namespace Sexy;

// The original's float code runs on the x87 stack, so its intermediate results are not rounded to float until they are
// stored; these are written in double where the original's expression is, and rounded where it stores.
// port: the original's trigonometry on floats is the CRT's sin and cos of a double (0x77e8c0, 0x77e790) in WindCloud and
// sinf and cosf (0x401b70, 0x401b20) elsewhere; the port's std functions answer alike except in the last bit.

Visualizer* Sexy::MakeVisualizer3D(const char* theName, WinTwistApp* theApp)
{
	if (sexy_stricmp(theName, "RedBars") == 0)
		return new RedBars(theApp);
	if (sexy_stricmp(theName, "BlobDrift") == 0)
		return new BlobDrift(theApp);
	if (sexy_stricmp(theName, "UVWarp") == 0)
		return new UVWarp(theApp);
	if (sexy_stricmp(theName, "ContainmentField") == 0)
		return new ContainmentField(theApp);
	if (sexy_stricmp(theName, "ParametricFractal") == 0)
		return new ParametricFractal(theApp);
	if (sexy_stricmp(theName, "WindCloud") == 0)
		return new WindCloud(theApp);
	if (sexy_stricmp(theName, "Raptis_Pulse") == 0)
		return new Raptis_Pulse(theApp);
	if (sexy_stricmp(theName, "PlasmaClouds") == 0)
		return new PlasmaClouds(theApp);
	if (sexy_stricmp(theName, "SpaceThreads") == 0)
		return new SpaceThreads(theApp);
	if (sexy_stricmp(theName, "FluffyStars") == 0)
		return new FluffyStars(theApp);
	if (sexy_stricmp(theName, "VizWarpBars") == 0)
		return new VizWarpBars(theApp);
	if (sexy_stricmp(theName, "VizKaleidoCat") == 0)
		return new VizKaleidoCat(theApp);
	if (sexy_stricmp(theName, "VizRainbowGlyphs") == 0)
		return new VizRainbowGlyphs(theApp);
	if (sexy_stricmp(theName, "SubAtomic") == 0)
		return new SubAtomic(theApp);
	if (sexy_stricmp(theName, "Cubes") == 0)
		return new Cubes(theApp);
	if (sexy_stricmp(theName, "RainDrops") == 0)
		return new RainDrops(theApp);
	if (sexy_stricmp(theName, "Gears") == 0)
		return new Gears(theApp);
	if (sexy_stricmp(theName, "Slipstream") == 0)
		return new Slipstream(theApp);
	return nullptr;
}

// A vertex of the original's TriVertex and of FVF 0x1c4 (32 bytes: x, y, z, rhw, diffuse, specular, u, v)
struct VizVertex {
	float mX, mY, mZ, mRHW;
	uint32_t mDiffuse, mSpecular;
	float mU, mV;
};

// 0x67f3cd and 0x69299a: theValue held between theLow and theHigh
static float Clamp01(float theValue)
{
	if (0.0f <= theValue) {
		if (theValue <= 1.0f)
			return theValue;
		return 1.0f;
	}
	return 0.0f;
}

// 0x6929cc (UVWarp's 0x67f3ff): the u of the mesh's texture coordinate at (a, b) in 0 to 1, swimming with theCount
static float WarpU(int theCount, double theSpeed, float a, float b)
{
	a = a - 0.5f;
	float aT = (float)((float)theCount * theSpeed);
	float aWave = cosf(a * 100.0f * (b - 0.5f) * aT);
	float aCos = cosf(aT);
	float aSin = sinf(aT);
	float aBreath = sinf((float)((double)aT * 3.0));
	return Clamp01((float)((((double)aSin * (double)(b - 0.5f) + (double)aWave * 0.05 + (double)aCos * (double)a) * 0.1 *
		((double)aBreath * 0.5 + 1.0)) + (double)a * 0.8 + 0.5));
}

// 0x692aab (UVWarp's 0x67f4de): the v
static float WarpV(int theCount, double theSpeed, float a, float b)
{
	b = b - 0.5f;
	float aT = (float)((float)theCount * theSpeed);
	float aWave = sinf((a - 0.5f) * 100.0f * b * aT);
	float aCos = cosf(aT);
	float aSin = sinf(aT);
	float aBreath = cosf((float)((double)aT * 4.0));
	return Clamp01((float)((((double)aWave * 0.05 + (double)aCos * (double)b) - (double)aSin * (double)(a - 0.5f)) * 0.1 *
		((double)aBreath * 0.5 + 1.0) + (double)b * 0.8 + 0.5));
}

// 0x692cba and 0x67f5bd's first pass: a 16 by 16 mesh over the screen, six vertices a cell (two triangles), each with
// the warped texture coordinates of its corner; the cells are in columns, the column the outer index
static void BuildWarpMesh(TriVertex (*theTriangles)[3], WinTwistApp* theApp, int theCount, double theSpeed)
{
	float aCellH;
	for (int aCol = 0; aCol < 16;) {
		int aNextCol = aCol + 1;
		float aLeftA = (float)aCol * 0.0625f;
		float aRightA = (float)aNextCol * 0.0625f;
		float aLeftX = (float)aCol * ((float)theApp->mWidth * 0.0625f);
		float aRightX = (float)aNextCol * ((float)theApp->mWidth * 0.0625f);
		int aRow = 0;
		do {
			aCellH = (float)theApp->mHeight * 0.0625f;
			float aTopY = (float)aRow * aCellH;
			float aTopB = (float)aRow * 0.0625f;
			int aNextRow = aRow + 1;
			float aBottomY = (float)aNextRow * aCellH;
			float aBottomB = (float)aNextRow * 0.0625f;
			TriVertex* aCell = &theTriangles[(aCol * 16 + aRow) * 2][0];
			aCell[0] = TriVertex(aLeftX, aTopY, WarpU(theCount, theSpeed, aLeftA, aTopB), WarpV(theCount, theSpeed, aLeftA, aTopB));
			aCell[1] = TriVertex(aRightX, aTopY, WarpU(theCount, theSpeed, aRightA, aTopB), WarpV(theCount, theSpeed, aRightA, aTopB));
			aCell[2] = TriVertex(aLeftX, aBottomY, WarpU(theCount, theSpeed, aLeftA, aBottomB), WarpV(theCount, theSpeed, aLeftA, aBottomB));
			aCell[3] = TriVertex(aRightX, aBottomY, WarpU(theCount, theSpeed, aRightA, aBottomB), WarpV(theCount, theSpeed, aRightA, aBottomB));
			aCell[4] = TriVertex(aRightX, aTopY, WarpU(theCount, theSpeed, aRightA, aTopB), WarpV(theCount, theSpeed, aRightA, aTopB));
			aCell[5] = TriVertex(aLeftX, aBottomY, WarpU(theCount, theSpeed, aLeftA, aBottomB), WarpV(theCount, theSpeed, aLeftA, aBottomB));
			if (aCol == 15) {
				// the last column ends at the screen's edge
				aCell[1].x = (float)theApp->mWidth;
				aCell[3].x = (float)theApp->mWidth;
				aCell[4].x = (float)theApp->mWidth;
			}
			aRow = aNextRow;
		} while (aRow < 16);
		aCol = aNextCol;
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// RedBars

RedBars::RedBars(WinTwistApp* theApp) : Visualizer(theApp)
{
	mImage = LoadVizImage(IMAGE_FIRESTAR_ID);
}

RedBars::~RedBars()
{
	DeleteVizImage(IMAGE_FIRESTAR_ID);
}

void RedBars::Draw(Graphics* g)
{
	(void)g;
	// one quad over the screen (a half pixel out each way), its texture coordinates sliding with the update count
	VizVertex aVertices[4];
	float aU = (float)((float)mUpdateCnt * 1.5e-05);
	float aV = (float)((float)mUpdateCnt * 1.1e-05);
	float aV2 = (float)((double)aV + 0.2);
	float aRight = (float)mWidth + 0.5f;
	float aBottom = (float)mHeight + 0.5f;
	const float aXs[4] = {-0.5f, aRight, -0.5f, aRight};
	const float aYs[4] = {-0.5f, -0.5f, aBottom, aBottom};
	const float aVs[4] = {aV, aV, aV2, aV2};
	for (int i = 0; i < 4; i++) {
		aVertices[i].mX = aXs[i];  // 0x67a154
		aVertices[i].mY = aYs[i];
		aVertices[i].mZ = 0.0f;
		aVertices[i].mRHW = 1.0f;
		aVertices[i].mDiffuse = 0;
		aVertices[i].mSpecular = 0;
		aVertices[i].mU = aU;
		aVertices[i].mV = aVs[i];
	}
	gD3DInterface->SetTextureWrap(0, true);       // slot 0xcc
	gD3DInterface->SetTexture(0, IMAGE_FIRESTAR);  // slot 0xd4
	gD3DInterface->DrawPrimitiveEx(0x1c4, GPUPT_TRIANGLESTRIP, aVertices, 2, Color::White, 0, 0.0f, 0.0f, true, nullptr);  // slot 0x7c
}

// ---------------------------------------------------------------------------------------------------------------------
// BlobDrift

BlobDrift::BlobDrift(WinTwistApp* theApp) : Visualizer(theApp)
{
	mBlobImage = LoadVizImage(IMAGE_BLOB_BLUR_ID);
	mBackground = LoadVizImage(IMAGE_BLOBDRIFT_BG_ID);
	mUnknown10 = 0.2f;  // 0x3e4ccccd
}

BlobDrift::~BlobDrift()
{
	DeleteVizImage(IMAGE_BLOB_BLUR_ID);
	DeleteVizImage(IMAGE_BLOBDRIFT_BG_ID);
}

void BlobDrift::Init()
{
	// the blobs' seven colours (made once)
	static Color aColors[7];
	static bool aMade = false;
	if (!aMade) {
		aMade = true;
		aColors[0] = Color(0x11, 0x11, 0x44);
		aColors[1] = Color(0, 0, 0x22);
		aColors[2] = Color(0x33, 0x33, 0x33);
		aColors[3] = Color(0x44, 0x44, 0x44);
		aColors[4] = Color(0x33, 0x11, 0x66);
		aColors[5] = Color(0x66, 0x66, 0x66);
		aColors[6] = Color(0x77, 0x77, 0x77);
	}
	int aCount = 0xfa;
	Blob* aBlob = mBlobs;
	do {
		// Rand draws, in this order: Rand(width), Rand(height), Rand(2.0f), Rand(2.0f), Rand(7), Rand(0.1f)
		aBlob->mX = (float)Rand(mApp->mWidth);
		aBlob->mY = (float)Rand(mApp->mHeight);
		aBlob->mVX = (float)(((double)Rand(2.0f) - 1.0) * 0.30000001192092896);
		aBlob->mVY = (float)(((double)Rand(2.0f) - 1.0) * 0.30000001192092896);
		int aIndex = Rand(7);
		aBlob->mColor = aColors[aIndex];
		// the colour twice as bright
		aBlob->mColor.mRed = aBlob->mColor.mRed << 1;
		aBlob->mColor.mGreen = aBlob->mColor.mGreen << 1;
		aBlob->mColor.mBlue = aBlob->mColor.mBlue << 1;
		float aSize = (float)((double)Rand(0.1f) + 0.20000000298023224);
		aBlob->mSize = aSize;
		aBlob->mColor.mAlpha = Ftol((double)aSize * 255.0);
		aBlob++;
		aCount--;
	} while (aCount != 0);
}

void BlobDrift::Update()
{
	mFactor = (float)((double)mFactor + 0.01);
	// every pair of blobs pushes apart and drifts
	int aFirst = 1;
	int aOuter = 0xfa;
	Blob* aI = mBlobs;
	do {
		if (aFirst < 0xfa) {
			int aInner = 0xfa - aFirst;
			Blob* aJ = aI + 1;
			do {
				float aDX = aI->mX - aJ->mX;
				float aDY = aI->mY - aJ->mY;
				float aDist = aDX * aDX + aDY * aDY;
				if (10.0 < aDist && aDist < 25000.0) {
					float aPushX = (float)(((double)aDX * 0.5) / (double)aDist);
					aI->mVX = aI->mVX + aPushX;
					float aPushY = (float)(((double)aDY * 0.5) / (double)aDist);
					aI->mVY = aI->mVY + aPushY;
					aJ->mVX = aJ->mVX - aPushX;
					aJ->mVY = aJ->mVY - aPushY;
					aI->mX = (float)((double)aDX / (double)aDist + (double)aI->mX);
					aI->mY = (float)((double)aDY / (double)aDist + (double)aI->mY);
					aJ->mX = (float)((double)aJ->mX - (double)aDX / (double)aDist);
					aJ->mY = (float)((double)aJ->mY - (double)aDY / (double)aDist);
				}
				aJ++;
				aInner--;
			} while (aInner != 0);
		}
		aI++;
		aFirst++;
		aOuter--;
	} while (aOuter != 0);

	int aCount = 0xfa;
	Blob* aBlob = mBlobs;
	do {
		float aOldVY = aBlob->mVY;
		float aY = aBlob->mY;
		float aGravity;
		if (mFactor <= 0.1)
			aGravity = 0.0016f;
		else
			aGravity = (float)(0.0016 / (double)mFactor);
		aBlob->mVY = aGravity + aBlob->mVY;
		float aVX;
		if (10.0 <= (double)(aBlob->mVX + aBlob->mX)) {
			if ((double)(aBlob->mVX + aBlob->mX) <= (double)((float)mApp->mWidth) - 10.0)
				aVX = (float)((double)aBlob->mVX * 0.9990000128746033);
			else
				aVX = (float)((double)std::fabs(aBlob->mVX) * -1.0099999904632568);
		} else {
			aVX = (float)((double)std::fabs(aBlob->mVX) * 1.0099999904632568);
		}
		aBlob->mVX = aVX;
		if (-100.0 <= (double)(aOldVY + aY)) {
			if ((float)mApp->mHeight < aOldVY + aY)
				aBlob->mVY = (float)((double)std::fabs(aBlob->mVY) * -1.015 - (double)mFactor * 0.012);
		} else {
			// off the top: back in at the bottom (Rand(2.0f), then Rand(0.1f))
			aBlob->mVX = (float)(((double)Rand(2.0f) - 1.0) * 0.30000001192092896);
			aBlob->mVY = (float)(((double)Rand(0.1f) - 1.0) * 0.20000000298023224);
			aBlob->mY = (float)mApp->mHeight + 90.0f;
			mFactor = (float)((double)mFactor * 0.85);
		}
		aBlob->mX = aBlob->mVX + aBlob->mX;
		aBlob->mY = aBlob->mVY + aBlob->mY;
		aBlob++;
		aCount--;
	} while (aCount != 0);
}

void BlobDrift::Draw(Graphics* g)
{
	if (!mApp->Is3DAccelerated())  // 0x4a7900
		return;
	g->PushState();
	g->SetColor(Color(0x666666));  // 0x4016b0
	g->SetColorizeImages(true);
	DrawImage(g, mBackground);  // 0x67fdb6
	int aWidth = Ftol((double)mBlobImage->mWidth * 0.6499999761581421);
	int aHeight = Ftol((double)mBlobImage->mHeight * 0.6499999761581421);
	g->SetDrawMode(1);
	int aHalfHeight = aHeight / 2;
	int aHalfWidth = aWidth / 2;
	const Blob* aBlob = mBlobs;
	int aCount = 0xfa;
	do {
		g->SetColor(aBlob->mColor);
		int aY = Ftol((double)aBlob->mY) - aHalfHeight;
		int aX = Ftol((double)aBlob->mX) - aHalfWidth;
		g->DrawImage(mBlobImage, aX, aY, aWidth, aHeight);
		aBlob++;
		aCount--;
	} while (aCount != 0);
	g->PopState();
}

// ---------------------------------------------------------------------------------------------------------------------
// UVWarp

UVWarp::UVWarp(WinTwistApp* theApp) : Visualizer(theApp)
{
	mImage = LoadVizImage(IMAGE_CRAZY_BG_ID);
	mUnknown10 = 0.75f;  // 0x3f400000
}

UVWarp::~UVWarp()
{
	DeleteVizImage(IMAGE_CRAZY_BG_ID);
}

void UVWarp::Draw(Graphics* g)
{
	if (!g->Is3D())
		return;
	static TriVertex aTriangles[512][3];
	g->PushState();
	g->SetColorizeImages(true);
	g->SetColor(Color(0xffffff));
	g->SetDrawMode(0);
	BuildWarpMesh(aTriangles, mApp, mCount, 0.00075);
	g->DrawTrianglesTex(mImage, aTriangles, 0x200);  // 0x4221c0
	// again, its columns mirrored and added dimly
	g->SetDrawMode(1);
	g->SetColor(Color(0x1f1f1f));
	int aWidth = mApp->mWidth;
	int aColumn = 1;
	do {
		int aCellW = (aWidth + (aWidth >> 31 & 0xf)) >> 4;
		float aLeft = (float)((aColumn - 1) * aCellW);
		float aRight = (float)(aCellW * aColumn);
		TriVertex* aCell = &aTriangles[(16 - aColumn) * 16 * 2][0];
		int aRow = 0x10;
		do {
			aCell[0].x = aRight;
			aCell[1].x = aLeft;
			aCell[2].x = aRight;
			aCell[3].x = aLeft;
			aCell[4].x = aLeft;
			aCell[5].x = aRight;
			if (aColumn == 0x10) {
				aCell[0].x = (float)aWidth;
				aCell[2].x = (float)aWidth;
				aCell[5].x = (float)aWidth;
			}
			aCell += 6;
			aRow--;
		} while (aRow != 0);
		aColumn++;
	} while (aColumn < 0x11);
	g->DrawTrianglesTex(mImage, aTriangles, 0x200);
	g->PopState();
}

// ---------------------------------------------------------------------------------------------------------------------
// ContainmentField

ContainmentField::ContainmentField(WinTwistApp* theApp) : Visualizer(theApp)
{
	mImage = LoadVizImage(IMAGE_CRAZY_BG2_ID);
	mUnknown10 = 0.2f;  // 0x3e4ccccd
}

ContainmentField::~ContainmentField()
{
	DeleteVizImage(IMAGE_CRAZY_BG2_ID);
}

void ContainmentField::StartRing(Ring& theRing)
{
	// three of the C runtime's rand: the place and a negative age (a delay)
	theRing.mX = CrtRand() % mApp->mWidth;
	theRing.mY = CrtRand() % mApp->mHeight;
	theRing.mAge = -CrtRand() % 100;
}

void ContainmentField::Init()
{
	int aCount = 8;
	Ring* aRing = mRings;
	do {
		StartRing(*aRing);
		aRing++;
		aCount--;
	} while (aCount != 0);
	aCount = 1000;
	do {
		Update();
		aCount--;
	} while (aCount != 0);
}

void ContainmentField::Update()
{
	mCount++;
	Ring* aRing = mRings;
	int aCount = 8;
	do {
		aRing->mAge++;
		if (400 < aRing->mAge)
			StartRing(*aRing);
		aRing++;
		aCount--;
	} while (aCount != 0);
}

void ContainmentField::Draw(Graphics* g)
{
	// the colours the field passes through (made once)
	static Color aColors[8];
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
	}
	g->PushState();
	g->SetColorizeImages(true);
	Image* anImage = mImage;
	float aTime = (float)((double)mCount * 0.0024);
	double aTimeD = (double)aTime;
	float aFraction = (float)(aTimeD - (double)(float)std::floor((double)aTime));  // 0x401b40
	unsigned int aFrom = (unsigned int)Ftol((double)aTime);
	unsigned int aFromIndex = aFrom & 0x80000007;
	if ((int)aFromIndex < 0)
		aFromIndex = (aFromIndex - 1 | 0xfffffff8) + 1;
	unsigned int aToIndex = aFrom + 1 & 0x80000007;
	if ((int)aToIndex < 0)
		aToIndex = (aToIndex - 1 | 0xfffffff8) + 1;
	const Color& aFromColor = aColors[aFromIndex];
	const Color& aToColor = aColors[aToIndex];
	int aBlue = Ftol((double)(aToColor.mBlue - aFromColor.mBlue) * (double)aFraction + (double)aFromColor.mBlue);
	int aGreen = Ftol((double)(aToColor.mGreen - aFromColor.mGreen) * (double)aFraction + (double)aFromColor.mGreen);
	int aRed = Ftol((double)(aToColor.mRed - aFromColor.mRed) * (double)aFraction + (double)aFromColor.mRed);
	g->SetColor(Color(aRed, aGreen, aBlue));
	// the picture scrolls sideways, wrapping
	int aWidth = mApp->mWidth;
	int aShift = Ftol((double)mCount * 0.6000000238418579) % aWidth;
	int aSourceShift = (anImage->mWidth * aShift) / aWidth;
	g->DrawImage(anImage, Rect(0, 0, aShift, mApp->mHeight),
		Rect(anImage->mWidth - aSourceShift, 0, aSourceShift, anImage->mHeight));
	g->DrawImage(anImage, Rect(aShift, 0, mApp->mWidth - aShift, mApp->mHeight),
		Rect(0, 0, anImage->mWidth - aSourceShift, anImage->mHeight));
	// squares of 26 pixels, darkened where the rings are not
	int aColumns = mApp->mWidth / 0x1a + 2;
	int aRows = mApp->mHeight / 0x1a + 2;
	if (0 < aColumns) {
		int aX = 0;
		int aColumnsLeft = aColumns;
		do {
			if (0 < aRows) {
				int aY = 0;
				int aRowsLeft = aRows;
				do {
					float aSum = 0.0f;
					const Ring* aRing = mRings;
					int aRingsLeft = 8;
					do {
						if (-1 < aRing->mAge) {
							int aDX = Ftol((double)aX - (double)(float)aRing->mX);
							int aDY = Ftol((double)aY - (double)(float)aRing->mY);
							int aDistance = (aRing->mAge * aRing->mAge - aDY * aDY) - aDX * aDX;
							if (aDistance < 0)
								aDistance = -aDistance;  // 0x77e769
							if (aDistance < 0xdac)
								aDistance = 0xdac;
							int aLimit = 0x32;
							if (aRing->mAge < 0x32)
								aLimit = aRing->mAge;
							if (400 - aLimit < aRing->mAge)
								aLimit = 400 - aRing->mAge;
							double aHalf = (double)aLimit * 0.5;
							float aPower = (float)std::pow((double)(float)aDistance, (double)0.65f);  // 0x670a49
							aSum = (float)(aHalf / ((double)aPower + 50.0) + (double)aSum);
						}
						aRing++;
						aRingsLeft--;
					} while (aRingsLeft != 0);
					int anAlpha = Ftol(255.0 - (double)aSum * 1000.0);
					if (0xff < anAlpha)
						anAlpha = 0xff;
					if (0 < anAlpha) {
						g->SetColor(Color(0, anAlpha));  // 0x4016f0
						g->FillRect(aX - 8, aY - 6, 0x1a, 0x1a);
					}
					aY += 0x1a;
					aRowsLeft--;
				} while (aRowsLeft != 0);
			}
			aX += 0x1a;
			aColumnsLeft--;
		} while (aColumnsLeft != 0);
	}
	g->PopState();
}

// ---------------------------------------------------------------------------------------------------------------------
// ParametricFractal

ParametricFractal::ParametricFractal(WinTwistApp* theApp) : Visualizer(theApp)
{
	mBlur = LoadVizImage(IMAGE_FRACTAL_BLUR_ID);
	mBackground = LoadVizImage(IMAGE_FRACTAL_BG_ID);
	mUnknown10 = 0.2f;  // 0x3e4ccccd
}

ParametricFractal::~ParametricFractal()
{
	DeleteVizImage(IMAGE_FRACTAL_BLUR_ID);
	DeleteVizImage(IMAGE_FRACTAL_BG_ID);
}

void ParametricFractal::Draw(Graphics* g)
{
	g->PushState();
	g->SetColorizeImages(true);
	g->SetColor(Color(0xffffff));
	DrawImage(g, mBackground);  // 0x67fdb6
	float anAngle = (float)((float)mCount * 0.0003);
	float aX = (float)mApp->mWidth * 0.4f;
	float aY = (float)(mApp->mHeight / 2);
	g->SetDrawMode(1);
	float aSine = sinf((float)((float)mCount * 0.0001));
	DrawBranch(g, aX, aY, anAngle, 1.0f, (float)((double)aSine * 15.710000038146973));
	g->PopState();
}

void ParametricFractal::SetBranchColor(Graphics* g, float theDepth)
{
	static Color aColors[8];
	static bool aMade = false;
	if (!aMade) {
		aMade = true;
		aColors[0] = Color(0, 0, 0xff);
		aColors[1] = Color(0, 0x40, 0xff);
		aColors[2] = Color(0, 0x80, 0xff);
		aColors[3] = Color(0, 0xc4, 0xff);
		aColors[4] = Color(0, 0xff, 0xff);
		aColors[5] = Color(0x80, 0xff, 0xff);
		aColors[6] = Color(0xff, 0xff, 0xff);
		aColors[7] = Color(0x80, 0x80, 0xff);
	}
	if (theDepth < 0.0f)
		theDepth = (float)((double)theDepth * -1.0);
	float aFraction = (float)((double)theDepth - (double)(float)std::floor((double)theDepth));  // 0x401b40
	unsigned int aFrom = (unsigned int)Ftol((double)theDepth);
	unsigned int aFromIndex = aFrom & 0x80000007;
	if ((int)aFromIndex < 0)
		aFromIndex = (aFromIndex - 1 | 0xfffffff8) + 1;
	unsigned int aToIndex = aFrom + 1 & 0x80000007;
	if ((int)aToIndex < 0)
		aToIndex = (aToIndex - 1 | 0xfffffff8) + 1;
	const Color& aFromColor = aColors[aFromIndex];
	const Color& aToColor = aColors[aToIndex];
	int aBlue = Ftol((double)(aToColor.mBlue - aFromColor.mBlue) * (double)aFraction + (double)aFromColor.mBlue);
	int aGreen = Ftol((double)(aToColor.mGreen - aFromColor.mGreen) * (double)aFraction + (double)aFromColor.mGreen);
	int aRed = Ftol((double)(aToColor.mRed - aFromColor.mRed) * (double)aFraction + (double)aFromColor.mRed);
	g->SetColor(Color(aRed, aGreen, aBlue, 0x19));
}

void ParametricFractal::DrawBranch(Graphics* g, float theX, float theY, float theAngle, float theScale, float theDepth)
{
	float aSway = (float)((float)mCount * 0.00065);
	aSway = sinf(aSway);
	float aDrawX = (float)((double)theX - (double)mApp->mWidth * (double)aSway * 0.12);
	float aReach = (float)((double)mApp->mWidth * 0.45 * (double)theScale);
	float aCos = cosf(theAngle);
	float aSin = sinf(theAngle);
	float aStepX = (float)(((double)aCos * (double)aReach + (double)aSin * (double)aReach) * 0.45);
	float aStepY = (float)((double)aReach * 0.75 * (double)aSin * (double)aCos);
	if (0.15 < (double)theScale) {
		float aSmaller = (float)((double)theScale * 0.85 - 0.025);
		float aRightX = aStepX + theX;
		DrawBranch(g, aRightX, aStepY + theY, (float)((((double)aCos - 2.0) - (double)aSin) * 0.1 + (double)theAngle), aSmaller,
			(float)((double)theDepth + 0.5));
		DrawBranch(g, theX - aStepX, theY - aStepY, (float)(((double)aSin - ((double)aCos + 1.6)) * 0.1 + (double)theAngle),
			aSmaller, (float)((double)theDepth + 0.5));
	}
	SetBranchColor(g, theDepth);
	if ((double)theScale < 0.75) {
		MatrixTransform aTransform;  // 0x44f9d0
		float aLength = (float)std::sqrt((double)(aStepX * aStepX + aStepY * aStepY));  // 0x618620
		float aDirX;
		float aDirY;
		if (aLength == 0.0f) {
			aDirX = aStepX;
			aDirY = aStepY;
		} else {
			aDirX = aStepX / aLength;
			aDirY = aStepY / aLength;
		}
		float aA = (float)((double)aDirX * (double)theScale * 2.5);
		float aB = (float)((double)aDirY * (double)theScale * 2.5);
		float aC = (float)((double)aDirX * (double)aLength * 0.02);
		float aD = (float)((double)aDirY * (double)aLength * 0.02);
		SexyMatrix3 aMatrix;  // 0x44f860: the identity
		aMatrix.LoadIdentity();
		aMatrix.m00 = aB;
		aMatrix.m01 = aC;
		aMatrix.m10 = -aA;
		aMatrix.m11 = aD;
		aTransform.SetMatrix(aMatrix);
		int anEdge = Ftol((double)mApp->mScale * -200.0);
		if ((float)anEdge < theY && theY < (float)(mApp->mHeight - anEdge)) {
			// the sprite at the point and a screen's width either way, as far as it reaches
			aDrawX = aDrawX - (float)mApp->mWidth;
			if ((double)mApp->mScale * -700.0 * (double)theScale < (double)aDrawX)
				g->DrawImageTransformF(mBlur, aTransform, aDrawX, theY);  // 0x422050
			aDrawX = (float)mApp->mWidth + aDrawX;
			if ((double)mApp->mScale * -700.0 * (double)theScale < (double)aDrawX)
				g->DrawImageTransformF(mBlur, aTransform, aDrawX, theY);
			if ((double)aDrawX < (double)mApp->mScale * 700.0 * (double)theScale) {
				aDrawX = (float)mApp->mWidth + aDrawX;
				g->DrawImageTransformF(mBlur, aTransform, aDrawX, theY);
			}
			if ((double)aDrawX < (double)mApp->mScale * 700.0 * (double)theScale)
				g->DrawImageTransformF(mBlur, aTransform, aDrawX, theY);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// WindCloud

WindCloud::WindCloud(WinTwistApp* theApp) : Visualizer(theApp)
{
	mSprite = LoadVizImage(IMAGE_AIR_STREAM_ID);
	mBackground = LoadVizImage(IMAGE_TADPOLE_BG_ID);
	mUnknown10 = 0.2f;  // 0x3e4ccccd
}

WindCloud::~WindCloud()
{
	DeleteVizImage(IMAGE_AIR_STREAM_ID);
	DeleteVizImage(IMAGE_TADPOLE_BG_ID);
	for (Vortex* aVortex : mVortices)
		delete aVortex;
}

void WindCloud::Init()
{
	// each sprite: 0x6928e8, five of the C runtime's rand
	Sprite* aSprite = mSprites;
	int aCount = 500;
	do {
		int aWidth = mApp->mWidth;
		int aRand = CrtRand();
		aSprite->mX = (float)(aRand % aWidth - aWidth / 2);
		int aHeight = mApp->mHeight;
		aRand = CrtRand();
		aSprite->mY = (float)(aRand % aHeight - aHeight / 2);
		aRand = CrtRand();
		aSprite->mVX = (float)((1.0 - (double)(float)(aRand % 2000) / 1000.0) * 1.2);
		aRand = CrtRand();
		aSprite->mVY = (float)((1.0 - (double)(float)(aRand % 2000) / 1000.0) * 1.2);
		aRand = CrtRand();
		aSprite->mUnknown = (float)(aRand % 800);
		aSprite++;
		aCount--;
	} while (aCount != 0);
	// two vortices, one each side
	int aSide = -1;
	do {
		int aX = Ftol((double)mApp->mWidth * 0.36000001430511475);
		Vortex* aVortex = new Vortex();
		aVortex->mX = (float)(aSide * aX);
		int aRand = CrtRand();
		aVortex->mY = (float)(aRand % 0x3c + -0x1e);
		aRand = CrtRand();
		aVortex->mAngle = (float)((double)(float)(aRand % 1000) * 0.00628);
		mVortices.push_back(aVortex);  // 0x693ad2
		aSide += 2;
	} while (aSide < 3);
	aCount = 0x1e;
	do {
		Update();
		aCount--;
	} while (aCount != 0);
}

void WindCloud::Update()
{
	mCount++;
	int aCount = mCount;
	if (aCount % 3 != 0)
		return;
	if (aCount % 100 == 0x31) {
		Vortex* aVortex = mVortices[0];  // 0x693308
		int aRand = CrtRand();
		aVortex->mAngle = (float)((double)(float)(0x96 - aRand % 300) * 0.00628 + (double)aVortex->mAngle);
	}
	if (mCount % 100 == 99) {
		Vortex* aVortex = mVortices[1];  // 0x69331f
		int aRand = CrtRand();
		aVortex->mAngle = (float)((double)(float)(0x96 - aRand % 300) * 0.00628 + (double)aVortex->mAngle);
	}
	for (size_t aVortexIndex = 0; aVortexIndex < mVortices.size(); aVortexIndex++) {
		Vortex* aVortex = mVortices[aVortexIndex];
		// the two points 50 off the vortex, either side of its angle
		double aSin1 = std::sin((double)(float)((double)aVortex->mAngle + 1.57));
		double aCos1 = std::cos((double)(float)((double)aVortex->mAngle + 1.57));
		double aSin2 = std::sin((double)(float)((double)aVortex->mAngle - 1.57));
		double aCos2 = std::cos((double)(float)((double)aVortex->mAngle - 1.57));
		float aX = aVortex->mX;
		float aY = aVortex->mY;
		float aPoint1X = (float)(aSin1 * 50.0 + (double)aX);
		float aPoint1Y = (float)(aCos1 * 50.0 + (double)aY);
		float aPoint2X = (float)(aSin2 * 50.0 + (double)aX);
		float aPoint2Y = (float)(aCos2 * 50.0 + (double)aY);
		Sprite* aSprite = mSprites;
		int aSprites = 500;
		do {
			float aDistX = aX - aSprite->mX;
			float aDistY = aY - aSprite->mY;
			float aSquare = aDistX * aDistX + aDistY * aDistY;
			float aDistance = (float)std::sqrt((double)aSquare);  // 0x4c0650
			float aScale = (float)((double)mApp->mWidth * 0.0009765625);
			if ((double)aDistance <= (double)aScale * (double)aScale * 70.0 * 70.0 && aDistance != 0.0f) {
				int aRandX = CrtRand();
				int aRandY = CrtRand();
				float aDX1 = (float)((double)(float)(aRandX % 100) / 1000.0 + ((double)aSprite->mX - (double)aPoint1X));
				float aDY1 = (float)((double)(float)(aRandY % 100) / 1000.0 + ((double)aSprite->mY - (double)aPoint1Y));
				float aLength1 = (float)std::sqrt((double)(aDX1 * aDX1 + aDY1 * aDY1));
				if (0.0f < aLength1) {
					float aPull = 1.0f;
					if (20.0 < (double)aLength1)
						aPull = (float)(20.0 / (double)aLength1);
					if (40.0 < (double)aDistance)
						aPull = (float)(40.0 / (double)aDistance * (double)aPull);
					aPull = (float)((double)aScale * 0.4000000059604645 * (double)aPull);
					double aQuotient = (double)aLength1 / (double)aScale;
					float aEX = (float)((double)aDX1 / aQuotient);
					float aEY = (float)((double)aDY1 / aQuotient);
					aSprite->mVX = (float)((double)aEY * (double)aPull + (double)aSprite->mVX);
					aSprite->mVY = (float)((double)aPull * (double)(-aEX) + (double)aSprite->mVY);
				}
				aRandX = CrtRand();
				aRandY = CrtRand();
				float aDX2 = (float)((double)(float)(aRandX % 100) / 1000.0 + ((double)aSprite->mX - (double)aPoint2X));
				float aDY2 = (float)((double)(float)(aRandY % 100) / 1000.0 + ((double)aSprite->mY - (double)aPoint2Y));
				float aLength2 = (float)std::sqrt((double)(aDX2 * aDX2 + aDY2 * aDY2));
				if (0.0f < aLength2) {
					float aPull = 1.0f;
					if (20.0 < (double)aLength2)
						aPull = (float)(20.0 / (double)aLength2);
					if (40.0 < (double)aDistance)
						aPull = (float)(40.0 / (double)aDistance * (double)aPull);
					aPull = (float)((double)aScale * 0.4000000059604645 * (double)aPull);
					double aQuotient = (double)aLength2 / (double)aScale;
					float aEX = (float)((double)aDX2 / aQuotient);
					float aEY = (float)((double)aDY2 / aQuotient);
					aSprite->mVX = (float)(-(double)aEY * (double)aPull + (double)aSprite->mVX);
					aSprite->mVY = (float)((double)aPull * (double)aEX + (double)aSprite->mVY);
				}
				if (9.0f < aSprite->mVX * aSprite->mVX + aSprite->mVY * aSprite->mVY) {
					float aSpeed = (float)std::sqrt((double)(aSprite->mVX * aSprite->mVX + aSprite->mVY * aSprite->mVY));
					aSprite->mVX = (float)((double)aSprite->mVX * (3.0 / (double)aSpeed));
					aSprite->mVY = (float)(3.0 / (double)aSpeed * (double)aSprite->mVY);
				}
			}
			aSprite++;
			aSprites--;
		} while (aSprites != 0);
	}
	// every sprite moves, slowing when it is fast, and wraps round the screen (a little wider)
	double aSlow = (double)0.996f;
	Sprite* aSprite = mSprites;
	int aSprites = 500;
	do {
		aSprite->mX = aSprite->mX + aSprite->mVX;
		aSprite->mY = aSprite->mVY + aSprite->mY;
		if (1.0f < aSprite->mVX * aSprite->mVX + aSprite->mVY * aSprite->mVY) {
			aSprite->mVX = (float)((double)aSprite->mVX * aSlow);
			aSprite->mVY = (float)((double)aSprite->mVY * aSlow);
		}
		int aMargin = Ftol((double)mApp->mScale * 20.0);
		int aWidth = mApp->mWidth;
		if ((float)(-aMargin - aWidth / 2) <= aSprite->mX) {
			if ((float)(aWidth / 2 + aMargin) < aSprite->mX) {
				aSprite->mX = aSprite->mX - (float)(aWidth + aMargin * 2);
			} else {
				int aHeight = mApp->mHeight;
				if ((float)(-aMargin - aHeight / 2) <= aSprite->mY) {
					if ((float)(aHeight / 2 + aMargin) < aSprite->mY)
						aSprite->mY = aSprite->mY - (float)(aHeight + aMargin * 2);
				} else {
					aSprite->mY = (float)(aHeight + aMargin * 2) + aSprite->mY;
				}
			}
		} else {
			aSprite->mX = (float)(aWidth + aMargin * 2) + aSprite->mX;
		}
		aSprite++;
		aSprites--;
	} while (aSprites != 0);
}

void WindCloud::Draw(Graphics* g)
{
	g->PushState();
	g->SetColorizeImages(true);
	g->SetColor(Color(0xffffff));
	if (!g->Is3D()) {
		DrawImage(g, mBackground);  // 0x67fdb6
	} else {
		// 0x692cba: the background on a 16 by 16 mesh whose texture coordinates swim
		static TriVertex aTriangles[512][3];
		g->SetDrawMode(0);
		BuildWarpMesh(aTriangles, mApp, mCount, 0.001);
		g->DrawTrianglesTex(mBackground, aTriangles, 0x200);  // 0x4221c0
	}
	g->SetDrawMode(1);
	g->SetColor(Color(0xbfff20));
	const Sprite* aSprite = mSprites;
	int aCount = 500;
	do {
		float aSpeedSquare = (aSprite->mVX * aSprite->mVX + aSprite->mVY * aSprite->mVY) * 0.05f;
		Transform aTransform;  // 0x44f9d0
		aTransform.Scale((float)((double)aSpeedSquare * 0.75 + 0.05000000074505806), (float)((double)aSpeedSquare * 1.75 + 0.05000000074505806));  // 0x44fae0
		float anAngle = (float)atan2((double)aSprite->mVX, (double)aSprite->mVY);  // 0x4c1b70
		aTransform.RotateRad((float)((double)anAngle + 0.0));  // 0x44fa70
		Rect aSource(0, 0, mSprite->mWidth, mSprite->mHeight);
		g->DrawImageTransformF(mSprite, aTransform, aSource, (float)(mApp->mWidth / 2) + aSprite->mX,
			(float)(mApp->mHeight / 2) + aSprite->mY);  // 0x4220f0
		aSprite++;
		aCount--;
	} while (aCount != 0);
	g->PopState();
}

// ---------------------------------------------------------------------------------------------------------------------
// Raptis_Pulse

Raptis_Pulse::Raptis_Pulse(WinTwistApp* theApp) : Visualizer(theApp)
{
	mPulseImage = LoadVizImage(IMAGE_PULSERING_ID);
	mStarry = LoadVizImage(IMAGE_STARRY_ID);
	mFlags4 |= 2;  // drawn at half size
}

Raptis_Pulse::~Raptis_Pulse()
{
	DeleteVizImage(IMAGE_PULSERING_ID);
	DeleteVizImage(IMAGE_STARRY_ID);
	for (Pulse* aPulse : mPulses)
		delete aPulse;
	for (Ring* aRing : mRings)
		delete aRing;
}

void Raptis_Pulse::Init()
{
	int aCount = 3000;
	do {
		Update();
		aCount--;
	} while (aCount != 0);
}

void Raptis_Pulse::Update()
{
	mCountdown--;
	if (mCountdown < 1) {
		// a new pulse: Rand(50), Rand(30) first
		mCountdown = Rand(0x32) + 0x96;
		if (Rand(0x1e) == 2)
			mCountdown = mCountdown * 5;
		Pulse* aPulse = new Pulse();
		aPulse->mScale = 0.0f;
		aPulse->mAngle1 = 1.0f;
		aPulse->mAngle2 = 2.0f;
		int aRandY = Rand(0x3c);
		int aRandX = Rand(0x3c);
		aPulse->mX = aPulse->mX + aRandX;
		aPulse->mY = aPulse->mY + aRandY;
		if (Rand(2) != 0)
			aPulse->mX = -aPulse->mX;
		if (Rand(2) != 0)
			aPulse->mY = -aPulse->mY;
		aPulse->mColor.mRed = Rand(0xff);
		aPulse->mColor.mGreen = Rand(0xff);
		int aBlue = Rand(0xff);
		aPulse->mColor.mAlpha = 0;
		aPulse->mSize = 0.0f;
		aPulse->mMaxSize = 64.0f;  // 0x42800000
		aPulse->mFlipped = false;
		aPulse->mColor.mBlue = aBlue;
		aPulse->mDegrees = mAngle;
		int aTurn = Rand(0x14);
		mAngle = (float)aTurn + mAngle;
		mPulses.push_back(aPulse);  // 0x693fbe: at the end
	}
	if (mLatestRing == nullptr || mLatestRing->mB < 1.75f) {
		Ring* aRing = new Ring();  // 0x679710
		aRing->mB = 2.0f;
		aRing->mC = (float)Rand(0x168);
		aRing->mA = 0.0f;
		mRings.push_back(aRing);
		mLatestRing = aRing;
	}
	// the pulses grow and fade (a pulse that is over goes: 0x694040, the loop goes on with the next one)
	size_t aPulseIndex = 0;
	while (aPulseIndex < mPulses.size()) {
		Pulse* aPulse = mPulses[aPulseIndex++];
		aPulse->mScale = (float)(((double)aPulse->mScale + 1.0) * 0.0005 * ((double)mUnknown74 + 1.0) + (double)aPulse->mScale);
		aPulse->mAngle1 = (float)((double)aPulse->mAngle1 + 0.01);
		aPulse->mAngle2 = (float)((double)aPulse->mAngle2 + 0.01);
		aPulse->mDegrees = (float)((double)aPulse->mDegrees + 0.1);
		if (Rand(1000) == 2)
			aPulse->mMaxSize = 128.0f;  // 0x43000000
		if (aPulse->mScale <= 4.0f) {
			float aOne = 1.0f;
			float aLimit;
			if (1.0f <= aPulse->mScale)
				aLimit = 1.0f;
			else
				aLimit = aPulse->mScale;
			float aSize = (float)((double)aPulse->mSize + 0.1);
			if ((double)aLimit * (double)aPulse->mMaxSize <= (double)aSize) {
				if (aPulse->mScale < 1.0f)
					aOne = aPulse->mScale;
				aSize = aOne * aPulse->mMaxSize;
			}
			aPulse->mSize = aSize;
		} else {
			float aSize = (float)((double)aPulse->mSize - 0.1);
			aPulse->mSize = aSize;
			if (aSize <= 0.0f) {
				mPulses.erase(mPulses.begin() + (aPulseIndex - 1));
				aPulseIndex--;
				delete aPulse;
				continue;
			}
		}
		int anAlpha;
		if (!aPulse->mFlipped) {
			anAlpha = Ftol((double)aPulse->mSize);
		} else {
			aPulse->mSize = (float)((double)aPulse->mSize - (double)aPulse->mMaxSize * 0.5);
			double aHalf = (double)aPulse->mMaxSize * 0.5;
			anAlpha = Ftol(aHalf - (double)(float)std::fabs((double)aPulse->mSize));
			anAlpha = anAlpha * 2;
			aPulse->mSize = (float)(aHalf + (double)aPulse->mSize);
		}
		aPulse->mColor.mAlpha = anAlpha;
	}
	size_t aRingIndex = 0;
	while (aRingIndex < mRings.size()) {
		Ring* aRing = mRings[aRingIndex++];
		float aB = (float)((double)aRing->mB - 0.0002);
		aRing->mB = aB;
		float aA = aRing->mA;
		aRing->mA = (float)((double)aA + 0.01);
		aRing->mC = (float)((double)aRing->mC - (2.0 - (double)aB) * 0.02 * 0.25);
		if (1.0 < (double)aA + 0.01)
			aRing->mA = 1.0f;
		if (aB <= 0.0f) {
			mRings.erase(mRings.begin() + (aRingIndex - 1));
			aRingIndex--;
			delete aRing;
		}
	}
	mPhase = mPhaseStep + mPhase;
}

void Raptis_Pulse::DrawPulse(Graphics* g, float theX, float theY, float theRotation, float theScale)
{
	// 34 vertices of a strip of 32 triangles: two rings of the picture's middle, the inner at 0.15 of its size and the
	// outer at 0.35, 17 points round
	TriVertex aVertices[34];
	int aPoint = 0;
	do {
		float aTurn = (float)((double)((float)aPoint * 0.0625f) * 3.1415927);
		aTurn = aTurn + aTurn;
		float aAround = aTurn + theRotation;
		float aCos = cosf(aAround);
		float aSin = sinf(aAround);
		TriVertex& anInner = aVertices[aPoint * 2];
		TriVertex& anOuter = aVertices[aPoint * 2 + 1];
		anInner.x = (float)((double)aCos * 0.15 * (double)mPulseImage->mWidth * 3.125 * (double)theScale);
		anInner.y = (float)((double)aSin * 0.15 * (double)mPulseImage->mHeight * 3.125 * (double)theScale);
		anInner.u = (float)((double)cosf(aTurn) * 0.15000000596046448 + 0.5);
		anInner.v = (float)((double)sinf(aTurn) * 0.15000000596046448 + 0.5);
		anInner.color = 0;
		anOuter.x = (float)((double)aCos * 0.35 * (double)mPulseImage->mWidth * 3.125 * (double)theScale);
		anOuter.y = (float)((double)aSin * 0.35 * (double)mPulseImage->mHeight * 3.125 * (double)theScale);
		anOuter.u = (float)((double)cosf(aTurn) * 0.3499999940395355 + 0.5);
		anOuter.v = (float)((double)sinf(aTurn) * 0.3499999940395355 + 0.5);
		anOuter.color = 0;
		aPoint++;
	} while (aPoint < 0x11);
	gD3DInterface->DrawTrianglesTexStrip(mPulseImage, aVertices, 0x20, g->mColor, 1, theX, theY, true);  // slot 0x78
}

void Raptis_Pulse::Draw(Graphics* g)
{
	if (mPulseImage == nullptr)
		return;
	int aCenterX = mApp->mWidth / 2;
	int aCenterYInt = mApp->mHeight / 2;
	if (!mApp->Is3DAccelerated())  // 0x4a7900
		return;
	g->PushState();
	g->SetColorizeImages(true);
	g->SetDrawMode(0);
	g->SetColor(Color(0, 0, 0, 0xff));
	// (+0x3c of the app's display is true on the Direct3D 9 path: the screen is cleared with slot 0x9c)
	gD3DInterface->Clear(Color::Black.mRed, Color::Black.mGreen, Color::Black.mBlue, Color::Black.mAlpha);
	g->SetDrawMode(1);
	// the rings once set the colour here, and nothing draws them
	size_t aRingIndex = 0;
	while (aRingIndex < mRings.size()) {
		Ring* aRing = mRings[aRingIndex++];
		double aLimit = 64.0;
		double aValue = (double)aRing->mB * aLimit;
		if (aLimit < aValue)
			aValue = aLimit;
		int anAlpha = Ftol(aValue * (double)aRing->mA);
		g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
	}
	g->SetColor(Color(0xff));
	float aSine = sinf(mPhase);
	float aFactor = mUnknown78;
	float aCenterX2 = (float)aCenterX;
	float aCenterY = (float)aCenterYInt;
	size_t aPulseIndex = 0;
	while (aPulseIndex < mPulses.size()) {
		Pulse* aPulse = mPulses[aPulseIndex++];
		g->SetColor(aPulse->mColor);
		float aSinAngle = aPulse->mAngle1;
		float aScale;
		float aX;
		float aY;
		if (!aPulse->mFlipped) {
			aScale = (float)((double)sinf(aSinAngle) * 0.01 * (double)aPulse->mScale + (double)aPulse->mScale);
			aX = (float)((double)aPulse->mX * (double)aPulse->mScale + (double)aCenterX2);
			aY = (float)((double)aPulse->mY * (double)aPulse->mScale);
		} else {
			float aInverse = (float)(4.0 - (double)aPulse->mScale);
			aScale = (float)((double)sinf(aSinAngle) * 0.01 * (double)aPulse->mScale + (double)aInverse);
			aX = (float)((double)aPulse->mX * (double)aInverse + (double)aCenterX2);
			aY = aInverse * (float)aPulse->mY;
		}
		aScale = (float)((double)aScale * ((double)aFactor * (double)aSine + 1.0));
		float aScaleX = (float)g->mDestImage->mWidth / (float)mApp->mWidth;
		float aSizeScale = aScaleX * aScale;
		float aScaleY = (float)g->mDestImage->mHeight / (float)mApp->mHeight;
		float aDrawY = aScaleY * (aY + aCenterY);
		float aDrawX = aScaleX * aX;
		DrawPulse(g, aDrawX, aDrawY, (float)(((double)aPulse->mDegrees * 3.1415927) / 180.0), aSizeScale);
	}
	g->PopState();
}
