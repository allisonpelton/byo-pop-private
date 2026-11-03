// GravityField, VizBuckyShards and VizMoleculeRedux (Visualizers3D.h). As in Visualizers3D3.cpp the original's float code
// runs on the x87 stack (rounded to float only where it stores, and to 53 bits, the CRT's default precision), so
// expressions are written in double where the original's are. The constants are written as the exe holds them: a float
// the compiler widened to a double is written with its long value (0.012500000186264515), a real double as it is.
#include "Visualizers3D.h"

#include "Board.h"
#include "Camera3D.h"
#include "Compat2004.h"
#include "DDImage.h"
#include "Graphics.h"
#include "Math3D.h"
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

// 0x401860: a colour as ARGB
static uint32_t PackArgb(const Color& theColor)
{
	return ((((uint32_t)theColor.mAlpha << 8 | (uint32_t)theColor.mRed) << 8 | (uint32_t)theColor.mGreen) << 8) |
		(uint32_t)theColor.mBlue;
}

// ---------------------------------------------------------------------------------------------------------------------
// GravityField

// 0x673d0f: IMAGE_GRAVITY_SPIRALTEXTURE and IMAGE_GRAVITY_EDGETEXTURE; drawn anywhere on the screen, not only round the
// board (bit 0 of +0x4 cleared)
GravityField::GravityField(WinTwistApp* theApp) : Visualizer(theApp)
{
	mCount = 0;
	mSpiral = LoadVizImage(IMAGE_GRAVITY_SPIRALTEXTURE_ID);
	mUnknown10 = 0.2f;  // 0x3e4ccccd
	mFlags4 &= ~1;
	mEdge = LoadVizImage(IMAGE_GRAVITY_EDGETEXTURE_ID);
}

// 0x674963 (0x67475d)
GravityField::~GravityField()
{
	DeleteVizImage(IMAGE_GRAVITY_SPIRALTEXTURE_ID);
	DeleteVizImage(IMAGE_GRAVITY_EDGETEXTURE_ID);
}

// 0x673d7e: a hundred updates
void GravityField::Init()
{
	int aCount = 100;
	do {
		Update();
		aCount--;
	} while (aCount != 0);
}

// 0x673d92
void GravityField::Update()
{
	mCount++;
}

static GravityField::Vertex MakeGravityVertex(float theX, float theY, float theZ, unsigned int theColor, float theU, float theV)
{
	GravityField::Vertex aVertex;
	aVertex.x = theX;
	aVertex.y = theY;
	aVertex.z = theZ;
	aVertex.mColor = theColor;
	aVertex.u = theU;
	aVertex.v = theV;
	return aVertex;
}

// 0x6749e5
void GravityField::MakeRibbon(std::vector<Vertex>& theVertices, const float* theFrom, const float* theTo,
	const float* theAcross, float theU, float theUStep, int theCount, float theFraction, float theAlpha)
{
	float aFromX = theFrom[0];
	float aFromY = theFrom[1];
	float aToX = theTo[0];
	float aToY = theTo[1];
	int aGrey = Ftol(((double)1 - (double)theAlpha) * 255.0);
	unsigned int aColor = PackArgb(Color(aGrey, aGrey, aGrey, 0xff));  // 0x401740, 0x401860
	if (-1 < theCount) {
		double aRest = (double)(1.0 - (double)theFraction);
		do {
			float aDeltaToX = theAcross[0] - aToX;
			float aDeltaToY = theAcross[1] - aToY;
			float aStepToX = theFraction * aDeltaToX;
			float aStepToY = theFraction * aDeltaToY;
			float aNextToX = aStepToX + aToX;
			float aNextToY = aStepToY + aToY;
			float aDeltaFromX = theAcross[0] - aFromX;
			float aDeltaFromY = theAcross[1] - aFromY;
			float aStepFromX = theFraction * aDeltaFromX;
			float aStepFromY = theFraction * aDeltaFromY;
			float aNextFromX = aStepFromX + aFromX;
			float aNextFromY = aStepFromY + aFromY;
			Vertex aFirst = MakeGravityVertex(aToX, aToY, 0.0f, aColor, theU, 0.0f);
			float aNextU = (float)((double)theU + (double)theUStep * (double)theFraction);
			Vertex aSecond = MakeGravityVertex(aNextToX, aNextToY, 0.0f, aColor, aNextU, 0.0f);
			Vertex aThird = MakeGravityVertex(aNextFromX, aNextFromY, 0.0f, aColor, aNextU, 1.0f);
			Vertex aFourth = MakeGravityVertex(aFromX, aFromY, 0.0f, aColor, theU, 1.0f);
			theVertices.push_back(aFirst);  // 0x674990
			theVertices.push_back(aSecond);
			theVertices.push_back(aThird);
			theVertices.push_back(aFirst);
			theVertices.push_back(aThird);
			theVertices.push_back(aFourth);
			theCount--;
			theUStep = (float)((double)theUStep * aRest);
			aFromX = aNextFromX;
			aFromY = aNextFromY;
			aToX = aNextToX;
			aToY = aNextToY;
			theU = aNextU;
		} while (-1 < theCount);
	}
}

// 0x6755a5
void GravityField::Draw(Graphics* g)
{
	g->PushState();                // 0x41fd70
	g->SetColor(Color(0xffffff));  // 0x4016b0, 0x41fef0
	g->SetColorizeImages(true);    // 0x41ff40
	DrawField(g);
	g->PopState();  // 0x41fe00
}

// 0x674c02: a chain of 125 segments worked out afresh from the update count. A segment starts at a point heading in a way
// (a unit vector); the side point is across the way from the start (by the length times 0.33 and a wobble) and the end
// point is along it by the length. The ribbon runs from the start and the side point towards the end (MakeRibbon, into
// aRibbons), and a thin quad (aEdges) lies along the step from the side point to the end. The next segment starts at the
// side point, heads along the step and is as long as the step less a little (the decay).
void GravityField::DrawField(Graphics* g)
{
	gD3DInterface->Clear(Color::Black.mRed, Color::Black.mGreen, Color::Black.mBlue, Color::Black.mAlpha);  // slot 0x9c
	std::vector<Vertex> aRibbons;  // -0x110
	std::vector<Vertex> aEdges;    // -0x80
	float aScale = (float)((double)g->mDestImage->mWidth / (double)mApp->mWidth);
	float aHalf = (float)((double)aScale * 0.5);
	float aTime = (float)((double)mCount * 0.03125);
	float aLength = (float)((double)aHalf * 5000.0);
	float aStartX = (float)((double)aHalf * -5000.0);
	float aStartY = (float)((double)aHalf * -1500.0);
	// 0x4bd450, 0x673fd0, 0x673fa4: the start point and the way, turned by the time (the original also makes a second turn
	// matrix, of the time times 0.25 degrees, and does nothing with it)
	Mat3 aTurn;
	float aDegrees = (float)((double)aTime * -0.15000000596046448);
	aTurn.RotateZ((float)((double)aDegrees * 3.1415927410125732 / 180.0));
	Vec3 aStart = TimesMat3(Vec3{aStartX, aStartY, 0.0f}, aTurn);
	Vec3 aWay = TimesMat3(Vec3{1.0f, 0.0f, 0.0f}, aTurn);
	float aFromX = aStart.x;
	float aFromY = aStart.y;
	float aWayX = aWay.x;
	float aWayY = aWay.y;
	float aShift = (float)((double)aTime * 0.0024999999441206455);
	float anAngle = (float)((double)aTime * 0.009999999776482582);
	float aDecay = (float)((double)sinf(anAngle) * 0.012500000186264515 + 0.9100000262260437);  // 0x401b70
	int aSegment = 0;
	do {
		float aPoints[8];  // the start, the side point and the end (and a fourth that is never used)
		int aPair = 3;
		float* aPointer = aPoints;
		do {
			*aPointer = 0.0f;
			aPair--;
			aPointer[1] = 0.0f;
			aPointer += 2;
		} while (-1 < aPair);
		float aNegWayY = -aWayY;
		float aAcrossX = aNegWayY * aLength;
		float aAcrossY = aWayX * aLength;
		aPoints[0] = aFromX;
		aPoints[1] = aFromY;
		float aFactor = (float)((double)cosf(anAngle) * 0.029999999329447746 + 0.33000001311302185);  // 0x401b20
		float aSideOffsetX = aAcrossX * aFactor;
		float aSideOffsetY = aAcrossY * aFactor;
		float aSideX = aSideOffsetX + aFromX;
		float aSideY = aSideOffsetY + aFromY;
		aPoints[2] = aSideX;
		aPoints[3] = aSideY;
		float anAlongX = aWayX * aLength;
		float anAlongY = aWayY * aLength;
		float anEndX = anAlongX + aFromX;
		float anEndY = anAlongY + aFromY;
		aPoints[4] = anEndX;
		aPoints[5] = anEndY;
		float aStepX = anEndX - aSideX;
		float aStepY = anEndY - aSideY;
		if (aSegment < 0x7d) {
			float aT = (float)((double)aSegment / 125.0);
			float aRunsF = (float)((double)aT * -12.0 + 14.0);
			int aRuns = Ftol((double)aRunsF);
			float aFraction = (float)((double)aT * 0.6499999761581421 + 0.25);
			MakeRibbon(aRibbons, &aPoints[0], &aPoints[2], &aPoints[4], 0.0f, 1.0f, aRuns, aFraction, aT);
			// the quad along the step, from a decay of the way along it to its end, 0.0125 of the step wide each side
			float aNegStepY = -aStepY;
			float aPerpX = (float)(0.012500000186264515 * (double)aNegStepY);
			float aPerpY = (float)(0.012500000186264515 * (double)aStepX);
			float aReachX = aStepX * aDecay;
			float aReachY = aStepY * aDecay;
			float aMidX = aReachX + aSideX;
			float aMidY = aReachY + aSideY;
			float aMidLeftX = aPerpX + aMidX;
			float aMidLeftY = aPerpY + aMidY;
			float anEndLeftX = aPerpX + anEndX;
			float anEndLeftY = aPerpY + anEndY;
			float anEndRightX = anEndX - aPerpX;
			float anEndRightY = anEndY - aPerpY;
			float aMidRightX = aMidX - aPerpX;
			float aMidRightY = aMidY - aPerpY;
			int aFade = Ftol(((double)1 - (double)aT) * 255.0);
			float aFadeF = (float)aFade;
			unsigned int aPacked = PackArgb(Color(Ftol((double)aFadeF), Ftol((double)aFadeF), Ftol((double)aFadeF * 0.5), 0xff));
			Vertex aFirst = MakeGravityVertex(aMidLeftX, aMidLeftY, 0.0f, aPacked, 1.0f, 0.0f);
			Vertex aSecond = MakeGravityVertex(anEndLeftX, anEndLeftY, 0.0f, aPacked, 1.0f, 1.0f);
			Vertex aThird = MakeGravityVertex(anEndRightX, anEndRightY, 0.0f, aPacked, 0.0f, 1.0f);
			Vertex aFourth = MakeGravityVertex(aMidRightX, aMidRightY, 0.0f, aPacked, 0.0f, 0.0f);
			aEdges.push_back(aFirst);
			aEdges.push_back(aSecond);
			aEdges.push_back(aThird);
			aEdges.push_back(aFirst);
			aEdges.push_back(aThird);
			aEdges.push_back(aFourth);
		}
		aFromX = aSideX;
		aFromY = aSideY;
		// 0x673b6f: the step made unit (it is as it was when it has no length), by 0x618620, its length
		float aStepSquare = (float)((double)aStepX * (double)aStepX + (double)aStepY * (double)aStepY);
		float aStepLength = (float)std::sqrt((double)aStepSquare);  // 0x4c0650
		if (aStepLength != 0.0f) {
			aWayX = aStepX / aStepLength;
			aWayY = aStepY / aStepLength;
		} else {
			aWayX = aStepX;
			aWayY = aStepY;
		}
		float aLengthSquare = (float)((double)aStepX * (double)aStepX + (double)aStepY * (double)aStepY);
		float aStepLength2 = (float)std::sqrt((double)aLengthSquare);
		aSegment++;
		aLength = (float)((double)aStepLength2 * (double)aDecay);
	} while (aSegment < 0x7d);

	// each quad of the edges joins on to the next: its second, third and fifth vertices come from the next quad's first and
	// fourth (the last quad is left as it is)
	size_t aQuads = aEdges.size() / 6;
	Vertex* aQuad = &aEdges[0];
	int aLeft = (int)aQuads - 1;
	if (aLeft > 0) {
		aQuad = aQuad + 1;
		do {
			aQuad[0] = aQuad[5];      // +0x78 is 5 vertices on
			aQuad[1] = aQuad[10];     // +0xf0
			aQuad[3] = aQuad[10];
			aQuad += 6;
			aLeft--;
		} while (aLeft != 0);
	}

	// the ribbons again, last triangle first, with the texture coordinate run on by the shift and made four times as quick
	std::vector<Vertex> aRuns;
	aRuns.reserve(aRibbons.size());  // 0x6747f9
	int aTriangle = (int)(aRibbons.size() / 3) - 1;
	if (-1 < aTriangle) {
		int aBase = aTriangle * 3;
		do {
			int aCorner = 0;
			do {
				Vertex aVertex = aRibbons[aBase + aCorner];
				aVertex.u = (float)(((double)aVertex.u + (double)aShift) * 4.0);
				aRuns.push_back(aVertex);
				aCorner++;
			} while (aCorner < 3);
			aTriangle--;
			aBase -= 3;
		} while (-1 < aTriangle);
	}

	// 0x67417d: the camera at (0, 0, 555) looking at the middle, the aspect of the screen's picture
	float anAspect = (float)((double)g->mDestImage->mWidth / (double)g->mDestImage->mHeight);
	PerspectiveCamera aCamera(90.0f, anAspect, 1.0f, 10000.0f);
	WorldGraphicsListener aListener(&aCamera, true);  // 0x4bd5d0
	aListener.SetCulling(false);                      // 0x4bd6c0
	aListener.SetDepth(false, false);                 // 0x4bd680
	{
		// 0x674143: the camera's transform put at the place and turned to look at the target (0x67411e)
		Transform3D aPlace = aCamera.mTransform;
		aPlace.mPos = Vec3{0.0f, 0.0f, 555.0f};
		if (LookFrom(aPlace, Vec3{0.0f, 0.0f, 0.0f}, Vec3{0.0f, 1.0f, 0.0f}))
			aCamera.mTransform = aPlace;
	}
	PrimitiveListener aPrimitiveListener(&aListener);
	gD3DInterface->SetTextureWrap(0, true);  // slot 0xcc
	if (aRuns.size() != 0) {
		gD3DInterface->SetTexture(0, mSpiral);  // slot 0xd4
		gD3DInterface->DrawPrimitiveEx(0x142, GPUPT_TRIANGLELIST, &aRuns[0], (int)(aRuns.size() / 3), Color(0, 0, 0, 0), 0,
			0.0f, 0.0f, true, &aPrimitiveListener);  // slot 0x7c
		gD3DInterface->SetTexture(0, nullptr);
	}
	gD3DInterface->SetTextureWrap(0, false);
	if (aEdges.size() != 0) {
		gD3DInterface->SetTexture(0, mEdge);
		gD3DInterface->DrawPrimitiveEx(0x142, GPUPT_TRIANGLELIST, &aEdges[0], (int)(aEdges.size() / 3), Color(0, 0, 0, 0), 0,
			0.0f, 0.0f, true, &aPrimitiveListener);
		gD3DInterface->SetTexture(0, nullptr);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// The meshes of VizBuckyShards and VizMoleculeRedux

namespace {

// The vertex of both visualizers' meshes (0x24 bytes; FVF 0x152: position, normal, diffuse, one texture coordinate; made by
// 0x680276 and 0x68024d, the same as the menu's planets have)
struct MeshVertex {
	float x, y, z;                     // +0x00
	float nx, ny, nz;                  // +0x0c
	unsigned int mColor = 0xffffffff;  // +0x18
	float u, v;                        // +0x1c
};

// 0x4c0840: a vector made unit, as its x87 code does it (the squares summed on the stack and rounded to a float once, the
// float's square root, one over that, each part times it); left as it is when it has no length. (Camera3D.h's
// NormalizedVec rounds partway differently; the menu keeps that.)
Vec3 NormalizedX87(const Vec3& v)
{
	float aSquare = (float)(((double)v.y * (double)v.y + (double)v.x * (double)v.x) + (double)v.z * (double)v.z);
	float aLength = (float)std::sqrt((double)aSquare);
	if (aLength == 0.0f)
		return v;
	float anInverse = (float)(1.0 / (double)aLength);
	return Vec3{v.x * anInverse, v.y * anInverse, v.z * anInverse};
}

// 0x4c06d0: a crossed with b, each part's products taken on the x87 stack (not rounded before the difference)
Vec3 CrossX87(const Vec3& a, const Vec3& b)
{
	return Vec3{(float)((double)b.z * (double)a.y - (double)b.y * (double)a.z),
		(float)((double)a.z * (double)b.x - (double)a.x * (double)b.z),
		(float)((double)b.y * (double)a.x - (double)b.x * (double)a.y)};
}

// 0x680276 (with 0x6801fe): the vertex at thePosition with the normal of the position made unit, the colour and the texture
// coordinate; 0x68024d is the same with colour 0 and no texture coordinate
MeshVertex MakeMeshVertex(const float* thePosition, float theU, float theV, unsigned int theColor)
{
	MeshVertex aVertex;
	aVertex.x = thePosition[0];
	aVertex.y = thePosition[1];
	aVertex.z = thePosition[2];
	Vec3 aNormal = NormalizedX87(Vec3{thePosition[0], thePosition[1], thePosition[2]});
	aVertex.nx = aNormal.x;
	aVertex.ny = aNormal.y;
	aVertex.nz = aNormal.z;
	aVertex.mColor = theColor;
	aVertex.u = theU;
	aVertex.v = theV;
	return aVertex;
}

// VizBuckyShards's mesh (0x681e4b, made in Draw): a radius and a list of triangles (a std::vector at +0x04). It is a
// 20-sided shape (0x68108f) cut up into 180 triangles on a sphere of the radius (0x68146d), each of which then grows a prism
// of flat triangles (0x681a99).
struct BuckyMesh {
	float mRadius;                      // +0x00
	std::vector<MeshVertex> mVertices;  // +0x04
	bool mMolecule = false;             // the VizMoleculeRedux's mesh (0x68d605), whose triangles are 0x68ca63's

	// 0x68ca63: the corners smooth (each its position made unit as the normal), pale and with the texture coordinates of
	// a triangle drawn in the middle of the texture; the same icosahedron and cutting as the shards' (0x68cbfb, 0x68cfd9)
	void AddMoleculeTriangle(const MeshVertex& a, const MeshVertex& b, const MeshVertex& c)
	{
		static float sUV[6];
		static bool sMade = false;
		if (!sMade) {
			sMade = true;
			sUV[0] = 0.5f;
			sUV[1] = 0.95f;
			sUV[2] = (float)((double)cosf(2.6179938f) * 0.44999998807907104 + 0.5);
			sUV[3] = (float)(0.5 - (double)sinf(2.6179938f) * 0.44999998807907104);
			sUV[4] = (float)((double)cosf(0.5235988f) * 0.44999998807907104 + 0.5);
			sUV[5] = (float)(0.5 - (double)sinf(0.5235988f) * 0.44999998807907104);
			for (int i = 0; i < 6; i++)
				sUV[i] = (float)((double)sUV[i] + 0.001953125);
		}
		const MeshVertex* aCorners[3] = {&a, &b, &c};
		for (int i = 0; i < 3; i++) {
			MeshVertex aVertex = *aCorners[i];
			Vec3 aNormal = NormalizedX87(Vec3{aVertex.x, aVertex.y, aVertex.z});  // 0x4c0840
			aVertex.nx = aNormal.x;
			aVertex.ny = aNormal.y;
			aVertex.nz = aNormal.z;
			aVertex.mColor = 0xc0ffffff;
			aVertex.u = sUV[i * 2];
			aVertex.v = sUV[i * 2 + 1];
			mVertices.push_back(aVertex);
		}
	}

	// 0x680e72: the triangle a, b, c with the normal of its face (b to a crossed with b to c) on each of its three vertices
	void AddTriangle(const MeshVertex& a, const MeshVertex& b, const MeshVertex& c)
	{
		if (mMolecule) {
			AddMoleculeTriangle(a, b, c);
			return;
		}
		Vec3 aToA{a.x - b.x, a.y - b.y, a.z - b.z};
		Vec3 aToC{c.x - b.x, c.y - b.y, c.z - b.z};
		Vec3 aNormal = NormalizedX87(CrossX87(aToA, aToC));
		const MeshVertex* aCorners[3] = {&a, &b, &c};
		for (int i = 0; i < 3; i++) {
			MeshVertex aVertex = *aCorners[i];
			aVertex.nx = aNormal.x;
			aVertex.ny = aNormal.y;
			aVertex.nz = aNormal.z;
			mVertices.push_back(aVertex);  // 0x680e1d
		}
	}
	// 0x680fd6: the quad a, b, c, d as two triangles
	void AddQuad(const MeshVertex& a, const MeshVertex& b, const MeshVertex& c, const MeshVertex& d)
	{
		AddTriangle(a, b, c);
		AddTriangle(a, c, d);
	}
	// 0x680f67 and 0x680fff: a triangle and a quad of positions, each vertex given the colour and (0.5, 0.5)
	void AddColoredTriangle(const float* a, const float* b, const float* c, unsigned int theColor)
	{
		MeshVertex aThird = MakeMeshVertex(c, 0.5f, 0.5f, theColor);
		MeshVertex aSecond = MakeMeshVertex(b, 0.5f, 0.5f, theColor);
		MeshVertex aFirst = MakeMeshVertex(a, 0.5f, 0.5f, theColor);
		AddTriangle(aFirst, aSecond, aThird);
	}
	void AddColoredQuad(const float* a, const float* b, const float* c, const float* d, unsigned int theColor)
	{
		MeshVertex aFourth = MakeMeshVertex(d, 0.5f, 0.5f, theColor);
		MeshVertex aThird = MakeMeshVertex(c, 0.5f, 0.5f, theColor);
		MeshVertex aSecond = MakeMeshVertex(b, 0.5f, 0.5f, theColor);
		MeshVertex aFirst = MakeMeshVertex(a, 0.5f, 0.5f, theColor);
		AddQuad(aFirst, aSecond, aThird, aFourth);
	}

	// 0x68d605: the molecule's sphere (the radius 23)
	explicit BuckyMesh(float theRadius)
	{
		mRadius = theRadius;
		mMolecule = true;
		BuildIcosahedron();
		Subdivide();
	}
	// 0x681e4b
	BuckyMesh(float theRadius, int theCount, float theFactor)
	{
		mRadius = theRadius;
		BuildIcosahedron();
		Subdivide();
		MakeShards(theCount, theFactor);
	}

	// 0x68108f: twelve points (one at each pole and two rings of five) and the twenty triangles between them
	void BuildIcosahedron()
	{
		mVertices.clear();  // 0x680a3a
		float aRadius = mRadius;
		float aTwiceSin = (float)(((double)aRadius + (double)aRadius) * (double)sinf(0.62831855f));  // 0x401b70, 36 degrees
		float aCos36 = (float)((double)cosf(0.62831855f) * (double)aRadius);                          // 0x401b20
		float aCos18 = (float)((double)cosf(0.31415927f) * (double)aRadius);
		float aSin18 = (float)((double)sinf(0.31415927f) * (double)aRadius);
		float aSquare = (float)((double)aTwiceSin * (double)aTwiceSin - (double)aRadius * (double)aRadius);
		float aRootA = (float)std::sqrt((double)aSquare);  // 0x4c0650
		aSquare = (float)(((double)aCos36 + (double)aRadius) * ((double)aCos36 + (double)aRadius) -
			(double)aCos36 * (double)aCos36);
		float aRootB = (float)std::sqrt((double)aSquare);
		float aRingHeight = (float)(((double)aRootB - (double)aRootA) * 0.5);
		float aTop = aRingHeight + aRootA;
		float aHalfSin = (float)(0.5 * (double)aTwiceSin);
		MeshVertex aPoints[12];
		float aPosition[3];
		aPosition[0] = 0.0f;
		aPosition[1] = aTop;
		aPosition[2] = 0.0f;
		aPoints[0] = MakeMeshVertex(aPosition, 0.0f, 0.0f, 0);
		aPosition[0] = aRadius;
		aPosition[1] = aRingHeight;
		aPosition[2] = 0.0f;
		aPoints[1] = MakeMeshVertex(aPosition, 0.0f, 0.0f, 0);
		aPosition[0] = aSin18;
		aPosition[2] = aCos18;
		aPoints[2] = MakeMeshVertex(aPosition, 0.0f, 0.0f, 0);
		float aNegCos36 = -aCos36;
		aPosition[0] = aNegCos36;
		aPosition[2] = aHalfSin;
		aPoints[3] = MakeMeshVertex(aPosition, 0.0f, 0.0f, 0);
		float aNegHalfSin = -aHalfSin;
		aPosition[2] = aNegHalfSin;
		aPoints[4] = MakeMeshVertex(aPosition, 0.0f, 0.0f, 0);
		float aNegCos18 = -aCos18;
		aPosition[0] = aSin18;
		aPosition[2] = aNegCos18;
		aPoints[5] = MakeMeshVertex(aPosition, 0.0f, 0.0f, 0);
		float aNegHeight = -aRingHeight;
		aPosition[0] = -aRadius;
		aPosition[1] = aNegHeight;
		aPosition[2] = 0.0f;
		aPoints[6] = MakeMeshVertex(aPosition, 0.0f, 0.0f, 0);
		float aNegSin18 = -aSin18;
		aPosition[0] = aNegSin18;
		aPosition[2] = aNegCos18;
		aPoints[7] = MakeMeshVertex(aPosition, 0.0f, 0.0f, 0);
		aPosition[0] = aCos36;
		aPosition[2] = aNegHalfSin;
		aPoints[8] = MakeMeshVertex(aPosition, 0.0f, 0.0f, 0);
		aPosition[2] = aHalfSin;
		aPoints[9] = MakeMeshVertex(aPosition, 0.0f, 0.0f, 0);
		aPosition[0] = aNegSin18;
		aPosition[2] = aCos18;
		aPoints[10] = MakeMeshVertex(aPosition, 0.0f, 0.0f, 0);
		aPosition[0] = 0.0f;
		aPosition[1] = -aTop;
		aPosition[2] = 0.0f;
		aPoints[11] = MakeMeshVertex(aPosition, 0.0f, 0.0f, 0);
		// white, with the texture coordinate in the middle
		MeshVertex* aPoint = aPoints;
		int aLeft = 0xc;
		do {
			aPoint->mColor = 0xffffffff;
			aPoint->u = 0.5f;
			aPoint->v = 0.5f;
			aPoint++;
			aLeft--;
		} while (aLeft != 0);
		// the top and the upper ring (points 1 to 5), the bottom and the lower ring (6 to 10)
		int i = 0;
		MeshVertex* aLower = &aPoints[6];
		do {
			i++;
			AddTriangle(aPoints[0], aLower[-5], aPoints[1 + (i % 5)]);
			AddTriangle(aPoints[11], aPoints[6 + (i % 5)], *aLower);
			aLower++;
		} while (i < 5);
		// the band between the rings
		int aThird = 3;
		int aCount = 5;
		aLower = &aPoints[6];
		do {
			int aBack = (aThird - 2) % 5;
			int aNext = aThird % 5;
			AddTriangle(aLower[-5], aPoints[6 + aNext], aPoints[1 + aBack]);
			AddTriangle(*aLower, aPoints[6 + aBack], aPoints[1 + aNext]);
			aThird++;
			aLower++;
			aCount--;
		} while (aCount != 0);
	}

	// 0x68146d: each triangle cut in nine: its sides in thirds and a point in the middle, all put on the sphere of the radius
	void Subdivide()
	{
		std::vector<MeshVertex> anOld = mVertices;  // 0x680d42
		mVertices.clear();
		unsigned int aTriangles = (unsigned int)(anOld.size() / 3);
		size_t aSource = 0;
		if (aTriangles != 0) {
			do {
				float aCorners[3][3];
				for (int aCorner = 0; aCorner < 3; aCorner++) {
					aCorners[aCorner][0] = anOld[aSource + aCorner].x;
					aCorners[aCorner][1] = anOld[aSource + aCorner].y;
					aCorners[aCorner][2] = anOld[aSource + aCorner].z;
				}
				aSource += 3;
				const float* P0 = aCorners[0];
				const float* P1 = aCorners[1];
				const float* P2 = aCorners[2];
				float aSide01[3] = {P1[0] - P0[0], P1[1] - P0[1], P1[2] - P0[2]};
				float aSide12[3] = {P2[0] - P1[0], P2[1] - P1[1], P2[2] - P1[2]};  // 0x673bb6
				float aSide20[3] = {P0[0] - P2[0], P0[1] - P2[1], P0[2] - P2[2]};
				float aPoints[10][3];  // the corners, the thirds of each side and the middle
				for (int j = 0; j < 3; j++) {
					aPoints[0][j] = P0[j];
					float aThird1 = aSide01[j] * 0.33333334f;
					aPoints[1][j] = P0[j] + aThird1;
					float aThird2 = aSide01[j] * 0.6666667f;
					aPoints[2][j] = aThird2 + P0[j];
					aPoints[3][j] = P1[j];
					float aThird3 = aSide12[j] * 0.33333334f;
					aPoints[4][j] = P1[j] + aThird3;
					float aThird4 = aSide12[j] * 0.6666667f;
					aPoints[5][j] = P1[j] + aThird4;
					aPoints[6][j] = P2[j];
					float aThird5 = aSide20[j] * 0.33333334f;
					aPoints[7][j] = P2[j] + aThird5;
					float aThird6 = aSide20[j] * 0.6666667f;
					aPoints[8][j] = P2[j] + aThird6;
				}
				// 0x4c0760: the middle is half of the sum of the thirds nearest P0 on its two sides
				float aSum[3];
				for (int j = 0; j < 3; j++)
					aSum[j] = aPoints[7][j] + aPoints[2][j];
				for (int j = 0; j < 3; j++)
					aPoints[9][j] = (float)((double)aSum[j] * 0.5);
				MeshVertex aVertices[10];
				int aNumber = 10;
				for (int k = 0; k < aNumber; k++) {
					float aRadius = mRadius;
					Vec3 aUnit = NormalizedX87(Vec3{aPoints[k][0], aPoints[k][1], aPoints[k][2]});  // 0x4c0840
					aPoints[k][0] = aUnit.x * aRadius;
					aPoints[k][1] = aUnit.y * aRadius;
					aPoints[k][2] = aUnit.z * aRadius;
					aVertices[k] = MakeMeshVertex(aPoints[k], 0.5f, 0.5f, 0xffffffff);
				}
				AddTriangle(aVertices[0], aVertices[1], aVertices[8]);
				AddTriangle(aVertices[8], aVertices[9], aVertices[7]);
				AddTriangle(aVertices[8], aVertices[1], aVertices[9]);
				AddTriangle(aVertices[1], aVertices[2], aVertices[9]);
				AddTriangle(aVertices[7], aVertices[5], aVertices[6]);
				AddTriangle(aVertices[7], aVertices[9], aVertices[5]);
				AddTriangle(aVertices[9], aVertices[4], aVertices[5]);
				AddTriangle(aVertices[9], aVertices[2], aVertices[4]);
				AddTriangle(aVertices[2], aVertices[3], aVertices[4]);
				aTriangles--;
			} while (aTriangles != 0);
		}
	}

	// 0x681a99: each triangle stays, and a smaller one (0.65 of the way out from its middle) is pushed out along its first
	// vertex's normal, joined to it by a quad on each side. The size of the push and the colour of the sides come from the
	// normal and theCount (the draws since the visualizer's last Slot7), theFactor being the blitz game's progress.
	void MakeShards(int theCount, float theFactor)
	{
		std::vector<MeshVertex> anOld = mVertices;  // 0x680d42
		mVertices.clear();
		unsigned int aTriangles = (unsigned int)(anOld.size() / 3);
		size_t aSource = 0;
		if ((int)aTriangles > 0) {
			double aPhase = ((double)theFactor * 0.0024999999441206455 + 0.0024999999441206455) * (double)theCount;
			double aHueScale = 255.0 - 0.0;
			double aGrow = (double)theFactor * 0.125 + 1.0;
			do {
				float aCorners[3][3];
				for (int aCorner = 0; aCorner < 3; aCorner++) {
					aCorners[aCorner][0] = anOld[aSource + aCorner].x;
					aCorners[aCorner][1] = anOld[aSource + aCorner].y;
					aCorners[aCorner][2] = anOld[aSource + aCorner].z;
				}
				float aSum[3] = {aCorners[1][0] + aCorners[0][0], aCorners[1][1] + aCorners[0][1],
					aCorners[1][2] + aCorners[0][2]};
				float aTriple[3] = {aCorners[2][0] + aSum[0], aCorners[2][1] + aSum[1], aCorners[2][2] + aSum[2]};
				// 0x4c07c0: the middle, the sum times one over 3
				float anInverse = (float)(1.0 / (double)3.0f);
				float aMiddle[3] = {anInverse * aTriple[0], anInverse * aTriple[1], anInverse * aTriple[2]};
				const MeshVertex& aFirst = anOld[aSource];
				float aNormalX = aFirst.nx;
				float aNormalY = aFirst.ny;
				float aNormalZ = aFirst.nz;
				float aMix = (float)(((double)aNormalX * 5.0 + (double)aNormalY) + (double)aNormalZ * 3.0);
				float anArgument = (float)((double)aMix + aPhase);
				float aSine = sinf(anArgument);  // 0x401b70
				float anAbs = fabsf(aSine);      // 0x401ad0
				float aHueF = (float)((double)anAbs * aHueScale + 0.0);
				int aHue = Ftol((double)aHueF);  // 0x77e016 cuts toward zero
				unsigned int aSideColor = (unsigned int)gApp->HSLToRGB(aHue, 0xff, 0x80) | 0xff000000;  // 0x4a7220
				float aSize = (float)((double)anAbs * aGrow + 0.25);
				aSize = aSize * mRadius;
				float aPush[3] = {aSize * aNormalX, aSize * aNormalY, aSize * aNormalZ};
				float aShrunk[3][3];  // the corners drawn in towards the middle
				float aRaised[3][3];  // and pushed out
				int aCorner = 0;
				do {
					float aFromMiddle[3] = {aCorners[aCorner][0] - aMiddle[0], aCorners[aCorner][1] - aMiddle[1],
						aCorners[aCorner][2] - aMiddle[2]};
					for (int j = 0; j < 3; j++) {
						float aPart = (float)((double)aFromMiddle[j] * 0.6499999761581421);
						aShrunk[aCorner][j] = aPart + aMiddle[j];
						aRaised[aCorner][j] = aShrunk[aCorner][j] + aPush[j];
					}
					aCorner++;
				} while (aCorner < 3);
				AddTriangle(anOld[aSource], anOld[aSource + 1], anOld[aSource + 2]);
				AddColoredTriangle(aRaised[0], aRaised[1], aRaised[2], 0xffffffff);
				int aSide = 0;
				int aNumber = 0;
				do {
					aNumber++;
					int aNext = aNumber % 3;
					AddColoredQuad(aShrunk[aSide], aShrunk[aNext], aRaised[aNext], aRaised[aSide], aSideColor);
					aSide++;
				} while (aSide < 3);
				aSource += 3;
				aTriangles--;
			} while (aTriangles != 0);
		}
	}
};

}  // namespace

// ---------------------------------------------------------------------------------------------------------------------
// VizBuckyShards

// 0x67fac2 (a Visualizer's, as Visualizers3D2.cpp's): theA, or by how far a blitz game has come theB: theB at its start and
// after its end (0x160 of the board is 0, 6 or 7), theA otherwise
static float BlitzFactor(WinTwistApp* theApp, float theA, float theB)
{
	float aResult = theA;
	if (theApp != nullptr && theApp->mBoard != nullptr && theApp->mBoard->mGameMode == Board::MODE_BLITZ) {
		Board* aBoard = theApp->mBoard;
		int aState = aBoard->mUnknown160;
		if (aState == 0) {
			float aProgress = (float)((double)(float)(aBoard->mUnknown3458 - 100) / 29900.0);
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

// 0x6802e8: IMAGE_CUBEMAP_6X1_VIZBUCKYSHARDS_CUBE; bit 0 of +0x4 cleared and bit 3 set
VizBuckyShards::VizBuckyShards(WinTwistApp* theApp) : Visualizer(theApp)
{
	mCount = 0;
	mCountBase = 0;
	mCube = LoadVizImage(IMAGE_CUBEMAP_6X1_VIZBUCKYSHARDS_CUBE_ID);
	mFlags4 = (mFlags4 & ~1) | 8;
}

// 0x680d15 (0x680aad)
VizBuckyShards::~VizBuckyShards()
{
	DeleteVizImage(IMAGE_CUBEMAP_6X1_VIZBUCKYSHARDS_CUBE_ID);
}

// 0x68034f
void VizBuckyShards::Update()
{
	mCount++;
}

// 0x681e98: the mesh of BuckyMesh (the radius 11) turned about z and x by the updates since Slot7 and lit by a point light
// whose colour moves round with them, drawn with the cube map as an environment map (stage 0's texture coordinates from the
// camera space reflection vector)
void VizBuckyShards::Draw(Graphics* g)
{
	g->PushState();  // 0x41fd70
	float aFactor = BlitzFactor(mApp, 0.0f, 1.0f);  // 0x67fac2
	gD3DInterface->Clear(Color::Black.mRed, Color::Black.mGreen, Color::Black.mBlue, Color::Black.mAlpha);  // slot 0x9c
	float anAspect = (float)((double)g->mDestImage->mWidth / (double)g->mDestImage->mHeight);
	PerspectiveCamera aCamera(90.0f, anAspect, 1.0f, 10000.0f);  // 0x67417d
	WorldGraphicsListener aListener(&aCamera, true);             // 0x4bd5d0
	aListener.SetCulling(true);                                  // 0x4bd6c0
	aListener.SetDepth(true, true);                              // 0x4bd680
	{
		// 0x674143: the camera at (0, 0, 30) looking at the middle
		Transform3D aPlace = aCamera.mTransform;
		aPlace.mPos = Vec3{0.0f, 0.0f, 30.0f};
		if (LookFrom(aPlace, Vec3{0.0f, 0.0f, 0.0f}, Vec3{0.0f, 1.0f, 0.0f}))
			aCamera.mTransform = aPlace;
	}
	Transform3D aTransform;  // 0x4bd4c0
	int aCount = mCount - mCountBase;
	float aCountF = (float)aCount;
	aTransform.mRot.RotateZ((float)((double)aCountF * 0.0));                    // 0x673fd0
	aTransform.mRot.RotateX((float)((double)aCountF * -0.0005000000237487257));  // 0x680589
	aListener.mWorld = aTransform;                                               // 0x673c10
	BuckyMesh aMesh(11.0f, aCount, aFactor);  // 0x681e4b
	aListener.SetLighting(true);              // 0x4bd6e0
	gD3DInterface->SetMaterial(Color::White, Color::White, Color::White, Color::Black, 0.0f);  // slot 0xf0
	float aTime = (float)((double)aCountF * 0.0002500000118743628);
	float aSine = fabsf(sinf(aTime));  // 0x401b70, 0x401ad0
	float aRaw = (float)((double)aSine * 32.0 + 112.0);
	int aHueA = Ftol((double)aRaw);  // 0x77e016, which cuts toward zero
	int aHueB = Ftol(((double)aFactor - 0.0) * -122.0);
	int aHue = (aHueA - aHueB) & 0xff;
	Color aTint((int)((unsigned int)mApp->HSLToRGB(aHue, 0xff, 0x90) | 0xff000000));  // 0x4a7220, 0x4016b0
	D3DLightColors aColors;  // 0x6802ad: white, black and black, 1.0; the third is the tint
	aColors.mDiffuse = Color::White;
	aColors.mSpecular = Color::Black;
	aColors.mAmbient = aTint;
	aColors.mIntensity = 1.0f;
	Vec3 anAttenuation{1.0f, 0.0f, 0.0f};
	Vec3 aLightPlace{0.0f, 0.0f, 100.0f};
	gD3DInterface->SetPointLight(0, aLightPlace, aColors, 1000.0f, anAttenuation);  // slot 0xe8
	gD3DInterface->LightEnable(0, true);                                            // slot 0xe4
	gD3DInterface->SetTexture(0, mCube);                                            // slot 0xd4
	gD3DInterface->SetTextureWrap(0, true);                                         // slot 0xcc
	gD3DInterface->SetTextureStageState(0, 0xb, 0x30000);                           // slot 0xbc
	PrimitiveListener aPrimitiveListener(&aListener);
	gD3DInterface->DrawPrimitiveEx(0x152, GPUPT_TRIANGLELIST, aMesh.mVertices.data(), (int)(aMesh.mVertices.size() / 3),
		Color(0xff, 0xff, 0xff, 0xff), 0, 0.0f, 0.0f, true, &aPrimitiveListener);  // slot 0x7c
	gD3DInterface->SetTextureStageState(0, 0xb, 0);
	gD3DInterface->SetTextureWrap(0, false);
	gD3DInterface->SetTexture(0, nullptr);
	g->PopState();  // 0x41fe00
}

// ---------------------------------------------------------------------------------------------------------------------
// VizMoleculeRedux

namespace {

// 0x4c1710 (through 0x68c71e, which turns the matrix at +0xc of a Transform3D): theMatrix times the turn of theRadians about
// theAxis (through the quaternion 0x4c1070 and its matrix 0x4c0f00)
void TurnAbout(Mat3& theMatrix, float theRadians, const Vec3& theAxis)
{
	// 0x4c0840: the axis made unit (left alone when it has no length)
	float aLength = (float)std::sqrt((double)(float)((double)theAxis.y * (double)theAxis.y + (double)theAxis.x * (double)theAxis.x +
		(double)theAxis.z * (double)theAxis.z));
	Vec3 anAxis = theAxis;
	if (aLength != 0.0f) {
		float anInverse = (float)(1.0 / (double)aLength);
		anAxis.x = anInverse * theAxis.x;
		anAxis.y = anInverse * theAxis.y;
		anAxis.z = anInverse * theAxis.z;
	}
	float aHalf = (float)((double)theRadians * 0.5);
	float aSine = (float)std::sin((double)aHalf);
	float x = aSine * anAxis.x;
	float y = anAxis.y * aSine;
	float z = aSine * anAxis.z;
	float w = (float)std::cos((double)aHalf);
	// 0x4c0f00
	float z2 = z * 2.0f;
	float w2 = w * 2.0f;
	float xx = x * 2.0f * x;
	float yy = y * 2.0f * y;
	float xy = x * y * 2.0f;
	Mat3 aTurn;
	aTurn.m[0] = 1.0f - (yy + z2 * z);
	aTurn.m[1] = w2 * z + xy;
	aTurn.m[3] = xy - w2 * z;
	aTurn.m[2] = z2 * x - w2 * y;
	aTurn.m[4] = 1.0f - (z2 * z + xx);
	aTurn.m[6] = z2 * x + w2 * y;
	aTurn.m[5] = z2 * y + w2 * x;
	aTurn.m[7] = z2 * y - w2 * x;
	aTurn.m[8] = 1.0f - (yy + xx);
	theMatrix = theMatrix * aTurn;
}

}  // namespace

// 0x68c928: IMAGE_BUCKYTRIFACE and IMAGE_MOLECULE_BG; bit 0 of +0x4 cleared and bit 1 set
VizMoleculeRedux::VizMoleculeRedux(WinTwistApp* theApp) : Visualizer(theApp)
{
	mCount = 0;
	mImage = LoadVizImage(IMAGE_BUCKYTRIFACE_ID);
	mBackground = LoadVizImage(IMAGE_MOLECULE_BG_ID);
	mFlags4 = (mFlags4 & ~1) | 2;
}

// 0x68ca44 (0x68c9a0)
VizMoleculeRedux::~VizMoleculeRedux()
{
	DeleteVizImage(IMAGE_BUCKYTRIFACE_ID);
	DeleteVizImage(IMAGE_MOLECULE_BG_ID);
}

// 0x68c99c
void VizMoleculeRedux::Update()
{
	mCount++;
}

// 0x68d641: the background, then the molecule's sphere (the radius 23, its triangles smooth and textured with the
// triangle-face image) drawn three times, additively, each turned about its own axis by the updates and a little smaller than
// the one before, lit by a point light whose colour moves round with them
void VizMoleculeRedux::Draw(Graphics* g)
{
	g->PushState();  // 0x41fd70
	float aFactor = BlitzFactor(mApp, 0.0f, 1.0f);  // 0x67fac2
	g->SetColorizeImages(true);                     // 0x41ff40
	g->SetColor(Color(0xff, 0xff, 0xff));
	DrawImage(g, mBackground);  // 0x67fdb6
	float anAspect = (float)((double)g->mDestImage->mWidth / (double)g->mDestImage->mHeight);
	PerspectiveCamera aCamera(90.0f, anAspect, 1.0f, 10000.0f);  // 0x67417d
	WorldGraphicsListener aListener(&aCamera, true);             // 0x4bd5d0
	aListener.SetCulling(false);                                 // 0x4bd6c0
	aListener.SetDepth(false, false);                            // 0x4bd680
	{
		// 0x674143: the camera at (0, 0, 30) looking at the middle
		Transform3D aPlace = aCamera.mTransform;
		aPlace.mPos = Vec3{0.0f, 0.0f, 30.0f};
		if (LookFrom(aPlace, Vec3{0.0f, 0.0f, 0.0f}, Vec3{0.0f, 1.0f, 0.0f}))
			aCamera.mTransform = aPlace;
	}
	BuckyMesh aMesh(23.0f);       // 0x68d605
	aListener.SetLighting(true);  // 0x4bd6e0
	gD3DInterface->SetMaterial(Color::White, Color::White, Color::White, Color::Black, 0.0f);  // slot 0xf0
	float aTime = (float)((double)mCount * 0.0024999999441206455);
	float aSine = fabsf(sinf(aTime));  // 0x401b70, 0x401ad0
	float aRaw = (float)((double)aSine * 28.0 + 144.0);
	int aHueA = Ftol((double)aRaw);  // 0x77e016, which cuts toward zero
	int aHueB = Ftol(((double)aFactor - 0.0) * -122.0);
	Color aTint((int)((unsigned int)mApp->HSLToRGB((aHueA - aHueB) & 0xff, 0xff, 0x4a) | 0xff000000));  // 0x4a7220, 0x4016b0
	D3DLightColors aColors;
	aColors.mDiffuse = aTint;
	aColors.mSpecular = Color::Black;
	aColors.mAmbient = Color(0x12, 0x3a, 0x3a);
	aColors.mIntensity = 1.0f;
	Vec3 anAttenuation{1.0f, 0.0f, 0.0f};
	Vec3 aLightPlace{0.0f, 0.0f, 100.0f};
	gD3DInterface->SetPointLight(0, aLightPlace, aColors, 1000.0f, anAttenuation);  // slot 0xe8
	gD3DInterface->LightEnable(0, true);                                            // slot 0xe4
	gD3DInterface->SetTexture(0, mImage);                                           // slot 0xd4
	Transform3D aTransforms[3];  // 0x4bd4c0
	aTransforms[0].mRot.RotateX((float)((double)mCount * -0.00025499999173916876));  // 0x680589
	TurnAbout(aTransforms[1].mRot, (float)((double)mCount * -0.0004250000056345016),
		NormalizedX87(Vec3{1.0f, 2.0f, 0.0f}));  // 0x68c71e
	TurnAbout(aTransforms[2].mRot, (float)((double)mCount * -0.0003150000120513141),
		NormalizedX87(Vec3{0.0f, -2.0f, 1.0f}));
	PrimitiveListener aPrimitiveListener(&aListener);
	for (int i = 2; i >= 0; i--) {
		Transform3D aPlace = aTransforms[i % 3];  // 0x4bd500
		float aScale = (float)(1.0 - (double)i * 0.02500000037252903);
		aPlace.mScale.x = aScale * aPlace.mScale.x;  // 0x68b341
		aPlace.mScale.y = aScale * aPlace.mScale.y;
		aPlace.mScale.z = aScale * aPlace.mScale.z;
		aListener.mWorld = aPlace;  // 0x673c10
		gD3DInterface->DrawPrimitiveEx(0x152, GPUPT_TRIANGLELIST, aMesh.mVertices.data(),
			(int)(aMesh.mVertices.size() / 3), Color(0xff, 0xff, 0xff, 0xff), 1, 0.0f, 0.0f, true,
			&aPrimitiveListener);  // slot 0x7c
	}
	gD3DInterface->SetTextureWrap(0, false);
	gD3DInterface->SetTexture(0, nullptr);
	g->PopState();  // 0x41fe00
}
