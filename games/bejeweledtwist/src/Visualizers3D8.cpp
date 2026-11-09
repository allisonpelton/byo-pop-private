// VizZenWheels, its ZenWheelYinYang and its rings (Visualizers3D.h). As in Visualizers3D3.cpp the original's float code
// runs on the x87 stack (rounded to float only where it stores, and to 53 bits, the CRT's default precision), so
// expressions are written in double where the original's are. A constant the export prints short is a double (0.00333 is
// 0.003329999977722764, read from the exe), a float the compiler widened is written with its long value, and 0.3, 0.15,
// 0.667 and 1.65 where the original compares or loads them are real floats.
#include "Visualizers3D.h"

#include "Board.h"
#include "Camera3D.h"
#include "Compat2004.h"
#include "DDImage.h"
#include "GameText.h"
#include "Graphics.h"
#include "Math3D.h"
#include "Res.h"
#include "SexyMatrix.h"
#include "TwistD3D.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>
#include <cstdint>

using namespace Sexy;

namespace {

// 0x401860: a colour as ARGB
uint32_t PackArgb(const Color& theColor)
{
	return ((((uint32_t)theColor.mAlpha << 8 | (uint32_t)theColor.mRed) << 8 | (uint32_t)theColor.mGreen) << 8) |
		(uint32_t)theColor.mBlue;
}

// The 3D maths HyperspaceD3D.cpp keeps to itself (0x44f4a0, 0x44f810, 0x44f690, 0x44f710, 0x44f790 and 0x68fa70), as
// it has them
GPUMatrix Mat4Multiply(const GPUMatrix& a, const GPUMatrix& b)
{
	GPUMatrix aResult;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			float aSum = 0.0f;
			for (int k = 0; k < 4; k++)
				aSum = (float)((double)a.m[i][k] * (double)b.m[k][j] + (double)aSum);
			aResult.m[i][j] = aSum;
		}
	}
	return aResult;
}

void Mat4Scale(GPUMatrix& theMatrix, float theX, float theY, float theZ)
{
	GPUMatrix aScale = GPUMatrix::Identity();
	aScale.m[0][0] = theX;
	aScale.m[1][1] = theY;
	aScale.m[2][2] = theZ;
	theMatrix = Mat4Multiply(theMatrix, aScale);
}

void Mat4RotateX(GPUMatrix& theMatrix, float theRadians)
{
	float aSine = (float)std::sin((double)theRadians);
	float aCosine = (float)std::cos((double)theRadians);
	GPUMatrix aTurn = GPUMatrix::Identity();
	aTurn.m[1][1] = aCosine;
	aTurn.m[1][2] = aSine;
	aTurn.m[2][1] = -aSine;
	aTurn.m[2][2] = aCosine;
	theMatrix = Mat4Multiply(theMatrix, aTurn);
}

void Mat4RotateY(GPUMatrix& theMatrix, float theRadians)
{
	float aSine = (float)std::sin((double)theRadians);
	float aCosine = (float)std::cos((double)theRadians);
	GPUMatrix aTurn = GPUMatrix::Identity();
	aTurn.m[0][0] = aCosine;
	aTurn.m[0][2] = -aSine;
	aTurn.m[2][0] = aSine;
	aTurn.m[2][2] = aCosine;
	theMatrix = Mat4Multiply(theMatrix, aTurn);
}

// 0x44f790: turned about z (the same shape as the other two: the sine and cosine at m[0][0], m[0][1], m[1][0], m[1][1])
void Mat4RotateZ(GPUMatrix& theMatrix, float theRadians)
{
	float aSine = (float)std::sin((double)theRadians);
	float aCosine = (float)std::cos((double)theRadians);
	GPUMatrix aTurn = GPUMatrix::Identity();
	aTurn.m[0][0] = aCosine;
	aTurn.m[0][1] = aSine;
	aTurn.m[1][0] = -aSine;
	aTurn.m[1][1] = aCosine;
	theMatrix = Mat4Multiply(theMatrix, aTurn);
}

// 0x68b341: the scale times theScale
void ScaleBy(Transform3D& theTransform, float theX, float theY, float theZ)
{
	theTransform.mScale.x = theX * theTransform.mScale.x;
	theTransform.mScale.y = theY * theTransform.mScale.y;
	theTransform.mScale.z = theZ * theTransform.mScale.z;
}

// 0x68fa70: theThis.compose(theParam): theThis's place carried through theParam, the turns multiplied in that order
Transform3D Compose(const Transform3D& theThis, const Transform3D& theParam)
{
	Transform3D aResult;
	aResult.mPos = TransformPoint(theParam, theThis.mPos);
	aResult.mRot = theThis.mRot * theParam.mRot;
	aResult.mScale.x = theParam.mScale.x * theThis.mScale.x;
	aResult.mScale.y = theParam.mScale.y * theThis.mScale.y;
	aResult.mScale.z = theParam.mScale.z * theThis.mScale.z;
	return aResult;
}

// 0x690885: theImage over the whole of the graphics' picture, or, while the board stands still (DrawsAroundBoard), all
// of it but the board's rectangle in the VizManager (+0x1cc) as four pieces. Like Visualizer::DrawImage (0x67fdb6), but by
// the size of the picture drawn to, not the app's.
void DrawImageAroundBoard(const VizZenWheels* theViz, Graphics* g, Image* theImage)
{
	int aDestWidth = g->mDestImage->mWidth;
	int aDestHeight = g->mDestImage->mHeight;
	if (!theViz->DrawsAroundBoard()) {
		g->DrawImage(theImage, 0, 0, aDestWidth, aDestHeight);  // 0x4216f0
		return;
	}
	float aScaleX = (float)((double)theImage->mWidth / (double)aDestWidth);
	int anImageHeight = theImage->mHeight;
	float aScaleY = (float)((double)anImageHeight / (double)aDestHeight);
	const int* aBoard = theViz->mApp->mVizManager->mBoardRect;
	if (aBoard[2] == 0 || aBoard[3] == 0) {
		g->DrawImage(theImage, 0, 0, aDestWidth, aDestHeight);
		return;
	}
	int aLeft = aBoard[0];
	int aRight = aLeft + aBoard[2];
	int aBottom = aBoard[1] + aBoard[3];
	// left of the board, then right of it, above it and below it (0x421780: a destination and a source rectangle)
	g->DrawImage(theImage, Rect(0, 0, aLeft, aDestHeight),
		Rect(0, 0, Ftol((double)aLeft * (double)aScaleX), anImageHeight));
	g->DrawImage(theImage, Rect(aRight, 0, aDestWidth - aRight, aDestHeight),
		Rect(Ftol((double)aRight * (double)aScaleX), 0, Ftol((double)(aDestWidth - aRight) * (double)aScaleX), anImageHeight));
	int aMiddle = aRight - aLeft;
	g->DrawImage(theImage, Rect(aLeft, 0, aMiddle, aBoard[1]),
		Rect(Ftol((double)aLeft * (double)aScaleX), 0, Ftol((double)aMiddle * (double)aScaleX),
			Ftol((double)aBoard[1] * (double)aScaleY)));
	g->DrawImage(theImage, Rect(aLeft, aBottom, aMiddle, aDestHeight - aBottom),
		Rect(Ftol((double)aLeft * (double)aScaleX), Ftol((double)aBottom * (double)aScaleY),
			Ftol((double)aMiddle * (double)aScaleX), Ftol((double)(aDestHeight - aBottom) * (double)aScaleY)));
}

// A point of a ring as a vertex: colour 0, and a texture coordinate that is set later (the original leaves it as it was)
ZenVertex PointVertex(const Vec3& thePoint)
{
	ZenVertex aVertex;
	aVertex.x = thePoint.x;
	aVertex.y = thePoint.y;
	aVertex.z = thePoint.z;
	return aVertex;
}

// A point of a rim moved theDepth along z (up, or down), white, with the texture coordinate; the x87 adds and subtracts the
// 0.0 of the other two axes as well
ZenVertex WallVertex(const Vec3& thePoint, bool theUp, float theDepth, float theU, float theV)
{
	ZenVertex aVertex;
	if (theUp) {
		aVertex.x = (float)((double)thePoint.x + 0.0);
		aVertex.y = (float)(0.0 + (double)thePoint.y);
		aVertex.z = (float)((double)thePoint.z + (double)theDepth);
	} else {
		aVertex.x = (float)((double)thePoint.x - 0.0);
		aVertex.y = (float)((double)thePoint.y - 0.0);
		aVertex.z = (float)((double)thePoint.z - (double)theDepth);
	}
	aVertex.mColor = 0xffffffffu;
	aVertex.u = theU;
	aVertex.v = theV;
	return aVertex;
}

}  // namespace

// ---------------------------------------------------------------------------------------------------------------------
// Ring

// 0x691e55: theSegments points round the circle of radius theOuter (and of theInner, for a ring), then the face (a fan from
// the middle for a disc, two triangles a segment for a ring), the face turned over, and the rim. The face's texture
// coordinates come from where its points are, the rim's are the quad's corners.
void VizZenWheels::Ring::Build(float theInner, float theOuter, int theSegments, float theDepth, float theAspectX, float theAspectY)
{
	mFace.clear();  // 0x6911c7
	mBack.clear();
	mWalls.clear();
	std::vector<Vec3> anInner;
	std::vector<Vec3> anOuter;
	float aStep = (float)(6.2831854820251465 / (double)theSegments);
	float anAngle = 0.0f;
	if (theInner <= 0.0f) {
		Vec3 anOuterStart{0.0f, theOuter, 0.0f};
		if (0 < theSegments) {
			int aLeft = theSegments;
			do {
				Mat3 aTurn;                // 0x4bd450
				aTurn.RotateZ(anAngle);    // 0x673fd0
				anOuter.push_back(TimesMat3(anOuterStart, aTurn));  // 0x4c0c80
				aLeft--;
				anAngle = (float)((double)anAngle + (double)aStep);
			} while (aLeft != 0);
		}
		if (0 < theSegments) {
			int i = 0;
			do {
				int aNext = (i + 1) % theSegments;
				mFace.push_back(PointVertex(anOuter[i]));  // 0x674990
				mFace.push_back(PointVertex(Vec3{0.0f, 0.0f, 0.0f}));
				mFace.push_back(PointVertex(anOuter[aNext]));
				i++;
			} while (i < theSegments);
		}
	} else {
		Vec3 anInnerStart{0.0f, theInner, 0.0f};
		Vec3 anOuterStart{0.0f, theOuter, 0.0f};
		if (0 < theSegments) {
			int aLeft = theSegments;
			do {
				Mat3 aTurn;
				aTurn.RotateZ(anAngle);
				anInner.push_back(TimesMat3(anInnerStart, aTurn));
				anOuter.push_back(TimesMat3(anOuterStart, aTurn));
				aLeft--;
				anAngle = (float)((double)anAngle + (double)aStep);
			} while (aLeft != 0);
		}
		if (0 < theSegments) {
			int i = 0;
			do {
				int aNext = (i + 1) % theSegments;
				mFace.push_back(PointVertex(anOuter[i]));
				mFace.push_back(PointVertex(anInner[i]));
				mFace.push_back(PointVertex(anInner[aNext]));
				mFace.push_back(PointVertex(anOuter[i]));
				mFace.push_back(PointVertex(anInner[aNext]));
				mFace.push_back(PointVertex(anOuter[aNext]));
				i++;
			} while (i < theSegments);
		}
	}

	// the face: white, at the depth, its texture coordinates from the point's place across it
	int aCount = (int)mFace.size();
	for (int i = 0; i < aCount; i++) {
		ZenVertex& aVertex = mFace[i];
		aVertex.mColor = 0xffffffffu;
		aVertex.u = (float)(((double)aVertex.x * 0.5) * (double)theAspectX + 0.5);
		aVertex.v = (float)(((double)aVertex.y * -0.5) * (double)theAspectY + 0.5);
		aVertex.z = theDepth;
	}

	// the face turned over: each triangle's first, third and second corner, z negated and u from the other side
	int aTriangles = aCount / 3;
	for (int t = 0; t < aTriangles; t++) {
		ZenVertex aCorners[3] = {mFace[t * 3], mFace[t * 3 + 2], mFace[t * 3 + 1]};
		for (int k = 0; k < 3; k++) {
			aCorners[k].z = (float)((double)aCorners[k].z * -1.0);
			aCorners[k].u = (float)(1.0 - (double)aCorners[k].u);
			mBack.push_back(aCorners[k]);
		}
	}

	// the rim of the circle of theOuter: two quads of corners at the depth and below it for each segment
	if (0 < theSegments) {
		int i = 0;
		do {
			int aNext = (i + 1) % theSegments;
			ZenVertex aDownHere = WallVertex(anOuter[i], false, theDepth, 0.0f, 0.0f);
			ZenVertex anUpHere = WallVertex(anOuter[i], true, theDepth, 1.0f, 0.0f);
			ZenVertex anUpNext = WallVertex(anOuter[aNext], true, theDepth, 1.0f, 1.0f);
			ZenVertex aDownNext = WallVertex(anOuter[aNext], false, theDepth, 0.0f, 1.0f);
			mWalls.push_back(aDownHere);
			mWalls.push_back(anUpHere);
			mWalls.push_back(anUpNext);
			mWalls.push_back(aDownHere);
			mWalls.push_back(anUpNext);
			mWalls.push_back(aDownNext);
			i++;
		} while (i < theSegments);
	}
	// and, for a ring, the rim of the circle of theInner
	if (0.0f < theInner && 0 < theSegments) {
		int i = 0;
		do {
			int aNext = (i + 1) % theSegments;
			ZenVertex anUpHere = WallVertex(anInner[i], true, theDepth, 0.0f, 0.0f);
			ZenVertex aDownHere = WallVertex(anInner[i], false, theDepth, 1.0f, 0.0f);
			ZenVertex aDownNext = WallVertex(anInner[aNext], false, theDepth, 1.0f, 1.0f);
			ZenVertex anUpNext = WallVertex(anInner[aNext], true, theDepth, 0.0f, 1.0f);
			mWalls.push_back(anUpHere);
			mWalls.push_back(aDownHere);
			mWalls.push_back(aDownNext);
			mWalls.push_back(anUpHere);
			mWalls.push_back(aDownNext);
			mWalls.push_back(anUpNext);
			i++;
		} while (i < theSegments);
	}
}

// 0x68fad3: the face's texture coordinates from the points' places again (theFace), and the back's the other way round
void VizZenWheels::Ring::SetTexCoords(float theScaleX, float theScaleY, bool theFace, bool theBack)
{
	if (theFace && mFace.size() != 0) {
		for (size_t i = 0; i < mFace.size(); i++) {
			ZenVertex& aVertex = mFace[i];
			aVertex.u = (float)(((double)aVertex.x * 0.5) * (double)theScaleX + 0.5);
			aVertex.v = (float)(((double)aVertex.y * -0.5) * (double)theScaleY + 0.5);
		}
	}
	if (theBack && mBack.size() != 0) {
		for (size_t i = 0; i < mBack.size(); i++) {
			ZenVertex& aVertex = mBack[i];
			aVertex.u = (float)(1.0 - (((double)aVertex.x * 0.5) * (double)theScaleX + 0.5));
			aVertex.v = (float)(((double)aVertex.y * -0.5) * (double)theScaleY + 0.5);
		}
	}
}

// 0x68fb89: the rim white with theAlpha, and its texture coordinate v run on by a tenth (the original's first argument,
// the update count, is not used)
void VizZenWheels::Ring::Fade(float theAlpha)
{
	int anAlpha = Ftol((double)theAlpha * 255.0);
	uint32_t aColor = PackArgb(Color(255, 255, 255, anAlpha));  // 0x401740, 0x401860
	for (size_t i = 0; i < mWalls.size(); i++) {
		mWalls[i].mColor = aColor;
		mWalls[i].v = (float)((double)mWalls[i].v + 0.10000000149011612);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Scene

// 0x6926f2: the six rings, each a sixth of the radius wider (the first a disc), 64 segments, depth 10 and the picture's
// proportions, then the curve of the fade
void VizZenWheels::Scene::Make(int theWidth, int theHeight)
{
	float anAspect = (float)((double)theHeight / (double)theWidth);
	int i = 0;
	do {
		int aNext = i + 1;
		float anInner = (float)(0.1666666716337204 * (double)i);
		float anOuter = (float)((double)aNext * 0.1666666716337204);
		mRings[i].Build(anInner, anOuter, 0x40, 10.0f, anAspect, 1.0f);
		i = aNext;
	} while (i < 6);
	mCurve.SetCurve(GTS(0x7d84f8));
}

// 0x68fbf2: the curve's input is 0 growing in, the phase held and 1 going out; the rims' fade is its output (six times, the
// original asks for it for each ring)
void VizZenWheels::Scene::Update(int theCount)
{
	(void)theCount;  // the original passes the update count on and nothing uses it
	float anInput = 0.0f;
	int aState = mViz->mState;
	if (aState != 1) {
		if (aState == 2)
			anInput = mViz->mPhase;
		else if (aState == 3)
			anInput = 1.0f;
	}
	mCurve.SetInVal((double)anInput);
	int i = 0;
	int aLeft = 6;
	do {
		float anAlpha = (float)mCurve.GetOutVal();
		mRings[i].Fade(anAlpha);
		i++;
		aLeft--;
	} while (aLeft != 0);
}

// 0x68fc63: the three space meshes of the app turned by the update count, then the six rings in front, each turned and
// grown by how far the transition is
void VizZenWheels::Scene::Draw(Graphics* g)
{
	g->PushState();  // 0x41fd70
	WinTwistApp* anApp = mViz->mApp;
	float anAppAspect = (float)((double)anApp->mWidth / (double)anApp->mHeight);
	PerspectiveCamera aSpaceCamera((float)((double)anAppAspect * 60.0), anAppAspect, 1.0f, 10000.0f);  // 0x67417d
	{
		WorldGraphicsListener aListener(&aSpaceCamera, true);  // 0x4bd5d0
		aListener.SetCulling(false);                           // 0x4bd6c0
		aListener.SetDepth(false, false);                      // 0x4bd680
		aListener.Apply();                                     // 0x4bd700
		GPUMatrix aMatrix = GPUMatrix::Identity();             // 0x44f5f0
		Mat4Scale(aMatrix, 30.0f, 30.0f, 30.0f);               // 0x44f810
		double aCount = (double)anApp->mUpdateCount;
		Mat4RotateY(aMatrix, (float)((((double)mLevelIndex * 3.1415927410125732) + ((double)mLevelIndex * 3.1415927410125732)) / 10.0 +
			aCount * 0.00019999999494757503));
		Mat4RotateX(aMatrix, (float)((((double)mLevelIndex / 10.0) * 3.1415927410125732) / 3.0 + aCount * 9.999999747378752e-05));
		Mat4RotateZ(aMatrix, (float)(aCount / 20000.0));
		gD3DInterface->SetTextureLinearFilter(0, true);  // slot 0xc8
		gD3DInterface->DrawObject(anApp->mOuterSpaceObject, aMatrix, Color::White, false);  // slot 0x10c
		gD3DInterface->SetRenderState(GPURS_DESTBLEND, 2);                                 // slot 0xc0
		gD3DInterface->DrawObject(anApp->mSpaceObject, aMatrix, Color::White, false);
		gD3DInterface->DrawObject(anApp->mStarsObject, aMatrix, Color::White, false);
		gD3DInterface->SetRenderState(GPURS_DESTBLEND, 6);
		aListener.Remove();  // 0x4bd8a0
	}

	// the rings: the picture's width over the app's, times the art's size over 768, is the scale of their depth
	float aDestWidth = (float)g->mDestImage->mWidth;
	float aRatio = (float)((double)aDestWidth / (double)anApp->mWidth);
	float aScale = (float)((double)aRatio * ((double)anApp->mArtRes / 768.0));
	float aDestAspect = (float)((double)aDestWidth / (double)g->mDestImage->mHeight);
	PerspectiveCamera aCamera((float)((double)aDestAspect * 105.0 / 1.5989583730697632), aDestAspect, 20.0f, 10000.0f);
	WorldGraphicsListener aListener(&aCamera, true);
	aListener.SetCulling(true);
	aListener.SetDepth(true, true);
	{
		// 0x674143: the camera at (1, -1, 1209 scales) looking at (1, -1, 0)
		Transform3D aCameraPlace = aCamera.mTransform;
		aCameraPlace.mPos = Vec3{1.0f, -1.0f, (float)(0.0 + (double)(float)((double)aScale * 1209.0))};
		if (LookFrom(aCameraPlace, Vec3{1.0f, -1.0f, 0.0f}, Vec3{0.0f, 1.0f, 0.0f}))
			aCamera.mTransform = aCameraPlace;
	}
	aListener.SetDepth(false, false);
	{
		Transform3D aPlace;  // 0x4bd4c0
		ScaleBy(aPlace, (float)g->mDestImage->mWidth, (float)g->mDestImage->mHeight, (float)((double)aScale * 10.0));  // 0x68b341
		aListener.mWorld = aPlace;                                                                                     // 0x673c10
	}
	aListener.SetDepth(true, true);
	gD3DInterface->SetTextureWrap(0, true);  // slot 0xcc

	float aShift = 0.0f;
	int aState = mViz->mState;
	if (aState != 1) {
		if (aState == 2)
			aShift = (float)((double)mViz->mPhase * 220.0);
		else if (aState == 3)
			aShift = 220.0f;
	}
	PrimitiveListener aPrimitiveListener(&aListener);
	int i = 0;
	do {
		Ring& aRing = mRings[i];
		Transform3D aGrow;  // 0x4bd4c0
		float anIndex = (float)i;
		// the shift clamped to 0 to 180 degrees (less 0 a ring) gives the growth's cosine, and less 5 degrees a ring the turn
		double aFirst = (double)aShift - (double)anIndex * 0.0;
		float aFirstAngle = (float)(aFirst >= 180.0 ? 180.0 : (aFirst < 0.0 ? 0.0 : aFirst));
		double aSecond = (double)aShift - (double)anIndex * 5.0;
		float aSecondAngle = (float)(aSecond >= 180.0 ? 180.0 : (aSecond < 0.0 ? 0.0 : aSecond));
		float aHeightOverWidth = (float)((double)g->mDestImage->mHeight / (double)g->mDestImage->mWidth);
		float aTwice = (float)((double)aFirstAngle + (double)aFirstAngle);
		float aRadians = (float)(((double)aTwice * 3.1415927410125732) / 180.0);
		float aGrowth = (float)((double)cosf(aRadians) * 0.8999999761581421 + 1.0);  // 0x401b20
		float aSize = (float)((double)g->mDestImage->mHeight * (double)aGrowth);
		ScaleBy(aGrow, aSize, aSize, (float)((double)aGrowth * (double)aScale));
		aRing.SetTexCoords((float)((double)aGrowth * (double)aHeightOverWidth), aGrowth, true, true);

		Transform3D aTurn;  // 0x4bd4c0
		float aSign = ((i & 2) != 0) ? -1.0f : 1.0f;
		if ((i & 1) == 0) {
			float aTurnZ = (float)((double)(float)((double)aSign * (double)aSecondAngle) * 3.1415927410125732 / 180.0);
			aTurn.mRot.RotateZ(aTurnZ);  // 0x673fd0
			float aTurnX = (float)((double)(float)((double)aSign * (double)aSecondAngle) * 3.1415927410125732 / 180.0);
			aTurn.mRot.RotateX(aTurnX);  // 0x680589
		} else {
			float aTurnY = (float)((double)(float)((double)aSign * (double)aSecondAngle) * 3.1415927410125732 / 180.0);
			aTurn.mRot.RotateY(aTurnY);  // 0x68fa0b
		}
		aListener.mWorld = Compose(Compose(Transform3D(), aTurn), aGrow);  // 0x68fa70, 0x673c10

		gD3DInterface->SetTexture(0, mViz->mImageB);  // slot 0xd4
		gD3DInterface->DrawPrimitiveEx(0x142, GPUPT_TRIANGLELIST, &aRing.mFace[0], (int)(aRing.mFace.size() / 3), Color::White, 0,
			0.0f, 0.0f, true, &aPrimitiveListener);  // slot 0x7c
		gD3DInterface->SetTexture(0, mViz->mImageA);
		gD3DInterface->DrawPrimitiveEx(0x142, GPUPT_TRIANGLELIST, &aRing.mBack[0], (int)(aRing.mBack.size() / 3), Color::White, 0,
			0.0f, 0.0f, true, &aPrimitiveListener);
		gD3DInterface->SetTexture(0, mViz->mEdge);
		gD3DInterface->DrawPrimitiveEx(0x142, GPUPT_TRIANGLELIST, &aRing.mWalls[0], (int)(aRing.mWalls.size() / 3), Color::White, 0,
			0.0f, 0.0f, true, &aPrimitiveListener);
		i++;
	} while (i < 6);
	gD3DInterface->SetTexture(0, nullptr);
	gD3DInterface->SetTextureWrap(0, false);
	g->PopState();  // 0x41fe00
}

// ---------------------------------------------------------------------------------------------------------------------
// ZenWheelYinYang

// 0x691a77: lets the picture go (as Release does)
ZenWheelYinYang::~ZenWheelYinYang()
{
	Release();
}

// 0x68f549
void ZenWheelYinYang::Init()
{
	mImage = LoadVizImage(IMAGE_YINYANG_BLACKTOALPHAZERO_ID);
	mFillCircle = IMAGE_FRUITBLAST_FILLCIRCLE;
}

// 0x69128d
void ZenWheelYinYang::Release()
{
	DeleteVizImage(IMAGE_YINYANG_BLACKTOALPHAZERO_ID);
}

// 0x69076b: the first level picture's alpha is half at rest, grows to full as the wheels grow in and falls from three
// quarters back to half as they go out; the second is full
void ZenWheelYinYang::GetAlphas(int* theFirst, int* theSecond)
{
	*theFirst = 0x80;
	*theSecond = 0xff;
	float anAlpha;
	if (mViz->mState == 1)
		anAlpha = (float)((double)mViz->mPhase * 127.0 + 128.0);
	else if (mViz->mState == 3)
		anAlpha = (float)((double)mViz->mPhase * -64.0 + 192.0);
	else
		return;
	*theFirst = Ftol((double)anAlpha);
}

// 0x690612: the size of the wheels is 1.65 at rest, shrinks to nothing as they grow in (through the first two thirds),
// is nothing while held and, going out, comes back from nothing to 1.65 (the last 0.85 of the phase)
void ZenWheelYinYang::Draw(Graphics* g)
{
	VizZenWheels* aViz = mViz;
	WinTwistApp* anApp = aViz->mApp;
	Image* aDest = g->mDestImage;
	float aRatio = (float)((double)aDest->mWidth / (double)anApp->mWidth);
	float aScale = (float)((double)aRatio * ((double)anApp->mArtRes / 768.0));
	float aSize = 1.65f;
	if (aViz->mState == 1) {
		if (0.667f < aViz->mPhase) {
			aSize = 0.0f;
		} else {
			float aT = (float)((double)aViz->mPhase / 0.6669999957084656);
			aSize = (float)((double)aT * -1.6483499760506675 + 1.649999976158142);
		}
	}
	float aCenterX = (float)(aDest->mWidth / 2);
	float aCenterY = (float)(aDest->mHeight / 2);
	if (aViz->mState == 3) {
		if (0.15f > aViz->mPhase) {
			aSize = 0.0f;
		} else {
			float aTenth = (float)((double)aSize * 0.0010000000474974513);
			float aT = (float)(((double)aViz->mPhase - 0.15000000596046448) / 0.8499999940395355);
			float aRest = (float)(1.0 - (double)aT);
			float aSquare = (float)((double)aRest * (double)aRest);
			float aFourth = (float)((double)aSquare * (double)aSquare);
			float aCurve = (float)(1.0 - (double)aFourth);
			aSize = (float)((double)aCurve * ((double)aSize - (double)aTenth) + (double)aTenth);
		}
	}
	if (aViz->mState != 2 && 0.0f < aSize)
		DrawTree(g, aViz->mCount, aCenterX, aCenterY, (float)((double)aSize * (double)aScale), 1, true);
}

// 0x69041e: at the first three depths two more are drawn first, 248 sizes either way from the point along a line turned
// by the count and the depth a sixty-fourth of a radian (negated), each half the size (the second, at a negative size,
// turned over); then the wheel itself. With theIsRoot the wheel is a mesh of 8 by 8 (0x75727e) turned by the count and
// depth a thirty-second of a degree, else the picture drawn through a transform turned by as many thirty-seconds of a
// radian. (Every call passes true, so the second way is never taken.)
void ZenWheelYinYang::DrawTree(Graphics* g, int theCount, float theX, float theY, float theSize, int theDepth, bool theIsRoot)
{
	if (theDepth < 3) {
		float aReach = (float)((double)theSize * 248.0);
		float aRightX = aReach;
		float aRightY = 0.0f;
		float aLeftX = -aReach;
		float aLeftY = 0.0f;
		Transform aTransform;  // 0x44f9d0
		aTransform.RotateRad((float)(((double)theCount * -0.015625) * (double)theDepth));  // 0x44fab0
		const SexyTransform2D& aMatrix = aTransform.GetMatrix();                          // 0x44fc10
		float aRightTurnedX = (float)((double)aRightX * (double)aMatrix.m00 + (double)aMatrix.m01 * (double)aRightY +
			(double)aMatrix.m02);  // 0x44f3b0
		float aRightTurnedY = (float)((double)aMatrix.m11 * (double)aRightY + (double)aMatrix.m10 * (double)aRightX +
			(double)aMatrix.m12);
		float aLeftTurnedX = (float)((double)aLeftX * (double)aMatrix.m00 + (double)aMatrix.m01 * (double)aLeftY +
			(double)aMatrix.m02);
		float aLeftTurnedY = (float)((double)aMatrix.m11 * (double)aLeftY + (double)aMatrix.m10 * (double)aLeftX +
			(double)aMatrix.m12);
		aRightX = (float)((double)aRightTurnedX + (double)theX);
		aRightY = (float)((double)aRightTurnedY + (double)theY);
		aLeftX = (float)((double)aLeftTurnedX + (double)theX);
		aLeftY = (float)((double)aLeftTurnedY + (double)theY);
		DrawTree(g, theCount, aRightX, aRightY, (float)((double)theSize * 0.5), theDepth + 1, true);
		DrawTree(g, theCount, aLeftX, aLeftY, (float)((double)theSize * -0.5), theDepth + 1, true);
	}
	if (theIsRoot) {
		double anAngle = ((double)theDepth * 0.03125 * (double)theCount * 3.1415927410125732) / 180.0;
		DrawImageMeshRotated(g, mImage, theX, theY, anAngle, theSize, theSize, 8);  // 0x75727e
	} else {
		Transform aTransform;
		aTransform.Scale(theSize, theSize);                                                    // 0x44fae0
		aTransform.RotateRad((float)((double)theDepth * 0.03125 * (double)theCount));          // 0x44fab0
		g->DrawImageTransform(mImage, aTransform, theX, theY);                                 // 0x422000
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// VizZenWheels

// 0x69279c: the scene, the picture ids, the wheel and IMAGE_ZENGYRO_EDGETEXTURE; bits 4 and 5 of +0x4 are set (0x30)
VizZenWheels::VizZenWheels(WinTwistApp* theApp) : Visualizer(theApp)
{
	mScene = new Scene(this);
	mScene->Make(mApp->mWidth, mApp->mHeight);
	mFlags4 |= 0x30;
	PickImages(true, false);
	mWheelIndex = 0;
	mWheels.push_back(new ZenWheelYinYang(this));  // 0x691dab
	unsigned int i = 0;
	if (mWheels.size() != 0) {
		do {
			mWheels[i]->Init();
			i++;
		} while (i < mWheels.size());
	}
	mEdge = LoadVizImage(IMAGE_ZENGYRO_EDGETEXTURE_ID);
	mShowScene = true;
	mPending = false;
}

// 0x691d33 (0x691c52): the wheels let go and deleted, the scene deleted, the level pictures' images let go, then the edge
VizZenWheels::~VizZenWheels()
{
	unsigned int i = 0;
	if (mWheels.size() != 0) {
		do {
			mWheels[i]->Release();
			delete mWheels[i];
			i++;
		} while (i < mWheels.size());
	}
	mWheels.clear();  // 0x691094
	if (mScene != nullptr) {
		delete mScene;
		mScene = nullptr;
	}
	if (mIdA != 0)
		DeleteVizImage(mIdA);
	if (mIdB != 0)
		DeleteVizImage(mIdB);
	DeleteVizImage(IMAGE_ZENGYRO_EDGETEXTURE_ID);
}

// 0x6907c9: the draws are the C library's rand (CrtRand), from 1000 to 1046 (IMAGE_ZENDROP1 to 47)
void VizZenWheels::PickImages(bool theDo, bool theSwap)
{
	if (theDo) {
		int aKeptId = mIdA;
		if (!theSwap) {
			mNextIdA = aKeptId;
			while (mNextIdA == mIdA || mNextIdA == mIdB)
				mNextIdA = CrtRand() % 0x2f + 1000;
			mNextIdB = mIdB;
			while (mNextIdB == mIdB || mNextIdB == mIdA || mNextIdB == mNextIdA)
				mNextIdB = CrtRand() % 0x2f + 1000;
		} else {
			mIdA = mIdB;
			mIdB = aKeptId;
			Image* anImage = mImageA;
			mImageA = mImageB;
			mImageB = anImage;
			int aNextId = mNextIdA;
			mNextIdA = mNextIdB;
			mNextIdB = aNextId;
			mNextIdA = mIdA;
			while (mNextIdA == mIdA || mNextIdA == mIdB)
				mNextIdA = CrtRand() % 0x2f + 1000;
		}
	}
}

// 0x6912da (the original holds the resource manager's lock, 0x404840, for each load)
void VizZenWheels::LoadPicked()
{
	PickImages(false, false);
	if (mIdA != mNextIdA) {
		if (mIdA != 0)
			DeleteVizImage(mIdA);
		mIdA = mNextIdA;
		mImageA = LoadVizImage(mNextIdA);
	}
	if (mIdB != mNextIdB) {
		if (mIdB != 0)
			DeleteVizImage(mIdB);
		mIdB = mNextIdB;
		mImageB = LoadVizImage(mNextIdB);
	}
}

// 0x690ac9
void VizZenWheels::Init()
{
	PickImages(true, false);
}

// 0x6913fe: the rims fade, then the state: 1 grows in over 300 updates (0.00333 an update) and plays the second sound,
// 2 holds for 500 (0.002 an update; the third sound at 0.3 of it) and, if a cascade came in, goes out (its images
// swapped), 3 goes out over 8000 (0.000125 an update), then 0
void VizZenWheels::Update()
{
	mCount++;
	if (mScene != nullptr)
		mScene->Update(mCount);
	int aState = mState;
	if (aState == 1) {
		mPhase = (float)((double)mPhase + 0.003329999977722764);
		if (!(mPhase < 1.0f)) {
			mApp->PlaySample(SOUND_ZENTRANSITION2);
			mState = 2;
			mPhase = 0.0f;
		}
	} else if (aState == 2) {
		float aPrevious = mPhase;
		mPhase = (float)((double)mPhase + 0.0020000000949949026);
		if (mPhase < 1.0f) {
			if (aPrevious < 0.3f && 0.3f <= mPhase)
				mApp->PlaySample(SOUND_ZENTRANSITION3);
		} else {
			mPhase = 1.0f;
			if (mPending) {
				mPhase = 0.0f;
				mState = 3;
				PickImages(true, true);
				mPending = false;
			}
		}
	} else if (aState == 3) {
		mPhase = (float)((double)mPhase + 0.0001250000059371814);
		if (!(mPhase < 1.0f)) {
			mState = 0;
			mPhase = 0.0f;
		}
	}
	mWheels[mWheelIndex]->Update();  // 0x68f5cf
	LoadPicked();
}

// 0x691533: the two level pictures over the wheels (each with its own alpha), or, held with the scene on, the scene
void VizZenWheels::Draw(Graphics* g)
{
	g->PushState();  // 0x41fd70
	LoadPicked();
	gD3DInterface->Clear(Color::Black.mRed, Color::Black.mGreen, Color::Black.mBlue, Color::Black.mAlpha);  // slot 0x9c
	if (mImageA != nullptr && mImageB != nullptr) {
		if (mState == 2 && mShowScene) {
			mScene->Draw(g);
		} else {
			gD3DInterface->ClearDepth();                 // slot 0x124
			gD3DInterface->SetAlphaRef(0);               // slot 0x11c
			gD3DInterface->SetDepthWriteMode(2);         // slot 0x118
			g->SetColorizeImages(true);                  // 0x41ff40
			g->SetColor(Color(255, 255, 255, 0x80));     // 0x41fef0
			mWheels[mWheelIndex]->Draw(g);
			gD3DInterface->SetDepthWriteMode(0);
			gD3DInterface->SetDepthTestMode(1);          // slot 0x114
			int aFirstAlpha;
			int aSecondAlpha;
			mWheels[mWheelIndex]->GetAlphas(&aFirstAlpha, &aSecondAlpha);
			g->SetColor(Color(255, 255, 255, aFirstAlpha));
			DrawImageAroundBoard(this, g, mImageA);
			gD3DInterface->SetDepthTestMode(2);
			g->SetColor(Color(255, 255, 255, aSecondAlpha));
			DrawImageAroundBoard(this, g, mImageB);
			gD3DInterface->SetDepthTestMode(0);
		}
	}
	g->PopState();  // 0x41fe00
}

// 0x690af4: b brings in the next look, c the next pair at once, e a new level, g toggles the scene, r new pictures
bool VizZenWheels::Slot4(char theChar)
{
	if (theChar == 'b') {
		NextLook();
		return true;
	}
	if (theChar == 'c') {
		PickImages(true, true);
		return true;
	}
	if (theChar == 'e') {
		NewLevel();
		return true;
	}
	if (theChar == 'g') {
		mShowScene = !mShowScene;
		return true;
	}
	if (theChar == 'r') {
		PickImages(true, false);
		return true;
	}
	return false;
}

// 0x690ad3: a cascade: while held it is kept for the end of the hold, else the wheels go out at once and the pictures swap
void VizZenWheels::NextLook()
{
	if (mState == 2) {
		mPending = true;
		return;
	}
	mPhase = 0.0f;
	mState = 3;
	PickImages(true, true);
}

// 0x68f572: a new level: the wheels grow in (and the scene's turn starts from the board's level less one)
void VizZenWheels::NewLevel()
{
	mApp->PlaySample(SOUND_ZENTRANSITION1);
	mPhase = 0.0f;
	mState = 1;
	if (mScene != nullptr) {
		mScene->mLevelIndex = 0;
		if (mApp->mBoard != nullptr)
			mScene->mLevelIndex = mApp->mBoard->mLevel - 1;
	}
}
