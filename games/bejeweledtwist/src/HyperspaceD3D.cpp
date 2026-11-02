// Sexy::Hyperspace's 3D mode (HyperspaceD3D.cpp): what the original's Draw (0x713e2d) does when gD3DInterface is set
// (space, outer space and the stars through a WorldGraphicsListener, the level's world, the saucer or the planet, the
// tunnel of a jump, the sidebar's tube, the warp cone, the ship's thrust and the souls), the world's making (0x717dd6,
// 0x717fcc) and the fixed-function shader class the sidebar's engine uses. In 2D mode (gD3DInterface null) none of it
// runs: Hyperspace.cpp's 2D paths are unchanged.
#include "Hyperspace.h"

#include "Board.h"
#include "SexyMatrix.h"
#include "CurvedVal.h"
#include "GameText.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "GemSoul.h"
#include "GoodTube.h"
#include "Menu.h"
#include "ChallengeMenu.h"
#include "Graphics.h"
#include "Res.h"
#include "TwistD3D.h"
#include "TwistUtil.h"
#include "VizManager.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>
#include <cstring>

// gApp+0xf98: the picture the saucer's "Image" mesh wears (the port keeps it here)
static int gSaucerCameo = 0;

using namespace Sexy;

namespace {

// The 3D maths the game uses on the way (0x44f460-0x44f810 for the 4 by 4 matrix, 0x68b2a8-0x68fa70 for transforms);
// the generic ones for Math3D, noted in the report.

// 0x44f4a0: a times b; each term is added to the sum rounded to a float, as the original's loop stores it
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

// 0x44f810: the matrix scaled (this = this * scale)
void Mat4Scale(GPUMatrix& theMatrix, float theX, float theY, float theZ)
{
	GPUMatrix aScale = GPUMatrix::Identity();
	aScale.m[0][0] = theX;
	aScale.m[1][1] = theY;
	aScale.m[2][2] = theZ;
	theMatrix = Mat4Multiply(theMatrix, aScale);
}

// 0x44f690: turned about x
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

// 0x44f710: turned about y
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

// 0x68b2a8: the place moved by theDelta
void Translate(Transform3D& theTransform, float theX, float theY, float theZ)
{
	theTransform.mPos.x = theX + theTransform.mPos.x;
	theTransform.mPos.y = theY + theTransform.mPos.y;
	theTransform.mPos.z = theZ + theTransform.mPos.z;
}

// 0x68b2cc (called 0x68b341): the scale times theScale
void ScaleBy(Transform3D& theTransform, float theX, float theY, float theZ)
{
	theTransform.mScale.x = theX * theTransform.mScale.x;
	theTransform.mScale.y = theY * theTransform.mScale.y;
	theTransform.mScale.z = theZ * theTransform.mScale.z;
}

// 0x68fa70 (0x71008e puts the result into the first): theThis.compose(theParam): theThis's place carried through
// theParam, the turns multiplied in that order
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

// 0x70e2fe: a transform that only turns
Transform3D TransformOfMat3(const Mat3& theRotation)
{
	Transform3D aResult;
	aResult.mRot = theRotation;
	return aResult;
}

// 0x4c1710: theMatrix times the turn of theRadians about theAxis (through the quaternion 0x4c1070 and its matrix
// 0x4c0f00)
void RotateAbout(Mat3& theMatrix, float theRadians, const Vec3& theAxis)
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

// The colours the lights are given: white lit and unlit, as the original's callers fill them
D3DLightColors LightColors(const Color& theDiffuse, const Color& theSpecular, const Color& theAmbient)
{
	D3DLightColors aColors;
	aColors.mDiffuse = theDiffuse;
	aColors.mSpecular = theSpecular;
	aColors.mAmbient = theAmbient;
	aColors.mIntensity = 1.0f;
	return aColors;
}

// The material every 3D draw of the hyperspace sets first (white, lit, dull): 0xf0 with 0xe14068 (white), 0xe14078 (black)
void SetPlainMaterial()
{
	gD3DInterface->SetMaterial(Color::White, Color::White, Color::White, Color::Black, 0.0f);
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// The off-center camera and its listener

void OffCenterCamera::SetFrustum(float theFovTimesAspect, float theAspect, float theCenterX, float theCenterY,
	float theNear, float theFar)
{
	// 0x70e381
	float anAngle = (float)((double)theFovTimesAspect * 0.5);
	anAngle = (float)((double)anAngle * 3.1415927410125732 / 180.0);
	anAngle = (float)((double)anAngle / (double)theAspect);
	double aCosine = (double)std::cos(anAngle);
	double aSine = (double)std::sin(anAngle);
	float aCotangent = (float)(aCosine / aSine);
	float aHalfWidth = (float)((double)theNear / (double)(float)((double)aCotangent / (double)theAspect));
	mLeft = (float)((double)theCenterX - (double)aHalfWidth);
	mRight = (float)((double)aHalfWidth + (double)theCenterX);
	mTop = (float)((double)theNear / (double)aCotangent + (double)theCenterY);
	mBottom = (float)((double)theCenterY - (double)theNear / (double)aCotangent);
	mScaleY = aCotangent;
	mScaleX = (float)((double)aCotangent / (double)theAspect);
	float aDepthScale = (float)((double)theFar / ((double)theFar - (double)theNear));
	mDepthScale = aDepthScale;
	mDepthOffset = (float)((double)theNear * -(double)aDepthScale);
	mNear = theNear;
	mFar = theFar;
}

namespace {

// WorldGraphicsListener (0x4bd5d0) around the off-center camera: its Apply (0x4bd700) takes the camera's own projection
// (slot 0, 0x70e44b) where the base takes the plain one (0x6741c5); the states are the base's
class OffCenterListener : public WorldGraphicsListener {
public:
	OffCenterListener(const OffCenterCamera* theCamera, bool theClearDepth) : WorldGraphicsListener(theCamera, theClearDepth)
	{
		mOffCenter = theCamera;
	}

	void Apply() override
	{
		gD3DInterface->SetRenderState(GPURS_ZWRITEENABLE, mZWrite);
		gD3DInterface->SetRenderState(GPURS_ZENABLE, mZEnable);
		gD3DInterface->SetRenderState(GPURS_ZFUNC, mZFunc);
		gD3DInterface->SetRenderState(GPURS_CULLMODE, mCullMode);
		GPUMatrix aWorld = WorldMatrixOf(mWorld);
		Transform3D aCamera = mOffCenter->mTransform;
		aCamera.mScale.z = -aCamera.mScale.z;
		GPUMatrix aView = ViewMatrixOf(aCamera);
		// 0x70e44b
		GPUMatrix aProjection;
		std::memset(&aProjection, 0, sizeof(aProjection));
		aProjection.m[0][0] = mOffCenter->mScaleX;
		aProjection.m[1][1] = mOffCenter->mScaleY;
		aProjection.m[2][0] = (mOffCenter->mRight + mOffCenter->mLeft) / (mOffCenter->mLeft - mOffCenter->mRight);
		aProjection.m[2][1] = (mOffCenter->mBottom + mOffCenter->mTop) / (mOffCenter->mBottom - mOffCenter->mTop);
		aProjection.m[2][2] = mOffCenter->mDepthScale;
		aProjection.m[2][3] = 1.0f;
		aProjection.m[3][2] = mOffCenter->mDepthOffset;
		gD3DInterface->SetTransform(GPUTS_WORLD, aWorld);
		gD3DInterface->SetTransform(GPUTS_VIEW, aView);
		gD3DInterface->SetTransform(GPUTS_PROJECTION, aProjection);
		if (mLighting) {
			gD3DInterface->SetRenderState(GPURS_LIGHTING, 1);
			gD3DInterface->SetRenderState(GPURS_AMBIENTMATERIALSOURCE, 0);
			gD3DInterface->SetRenderState(GPURS_DIFFUSEMATERIALSOURCE, 1);
			gD3DInterface->SetRenderState(GPURS_SPECULARMATERIALSOURCE, 0);
			gD3DInterface->SetRenderState(GPURS_EMISSIVEMATERIALSOURCE, 0);
		} else {
			gD3DInterface->SetRenderState(GPURS_LIGHTING, 0);
		}
		if (mInner != nullptr)
			mInner->Apply();
	}

	const OffCenterCamera* mOffCenter;
};

///////////////////////////////////////////////////////////////////////////////
// The models' listeners (the draws' LocalListener classes)

// DrawSaucer's LocalListener (vtable 0x7e42c4, the object at 0x85d708): +4 the hyperspace, +8 the cameo
class SaucerListener : public D3DObjectListener {
public:
	Hyperspace* mHyperspace = nullptr;  // +0x04
	int mCameo = 0;                     // +0x08: gApp+0xf98, the picture the saucer's "Image" mesh wears

	// 0x7134ed
	void BeginMesh(const D3DMesh& theMesh, bool hasBump) override
	{
		(void)hasBump;
		Image* aCameos[5] = {IMAGE_CAMEOS_SPLORK, IMAGE_CAMEOS_LEX, IMAGE_CAMEOS_ZUMA, IMAGE_CAMEOS_CHUZZLE, IMAGE_CAMEOS_SNACKERS};
		TwistShader* aShader = nullptr;
		if (theMesh.mName == "Window") {
			gD3DInterface->SetRenderState(GPURS_DESTBLEND, 2);
			aShader = gTwistShaders[TWISTSHADER_SAUCER_WINDOW_P];
		} else if (theMesh.mName == "Image") {
			gD3DInterface->SetRenderState(GPURS_ALPHABLENDENABLE, 1);
			gD3DInterface->SetTexture(0, aCameos[mCameo]);
		}
		gD3DInterface->SetPixelShader(aShader);
		gD3DInterface->SetTextureStageState(1, 0xb, 0x10000);
		gD3DInterface->SetRenderState(GPURS_NORMALIZENORMALS, 1);
	}

	// 0x70f1ca
	void EndMesh(D3DObject* theObject, const D3DMesh& theMesh) override
	{
		(void)theObject;
		(void)theMesh;
		gD3DInterface->SetRenderState(GPURS_ZENABLE, 1);
		gD3DInterface->SetRenderState(GPURS_DESTBLEND, 6);
		gD3DInterface->SetTextureStageState(1, 0xb, 1);
		gD3DInterface->SetRenderState(GPURS_NORMALIZENORMALS, 0);
	}
};

// DrawPlanet's LocalListener (vtable 0x7e42a0, the object at 0x85d6fc): +4 the hyperspace
class PlanetListener : public D3DObjectListener {
public:
	Hyperspace* mHyperspace = nullptr;  // +0x04

	// 0x713437
	void BeginMesh(const D3DMesh& theMesh, bool hasBump) override
	{
		(void)hasBump;
		if (theMesh.mName == "SurfaceShader") {
			gD3DInterface->SetRenderState(GPURS_DESTBLEND, 6);
			gD3DInterface->SetTexture(0, mHyperspace->mPlanetImage);
		} else {
			gD3DInterface->SetRenderState(GPURS_DESTBLEND, 2);
			gD3DInterface->SetRenderState(GPURS_ZWRITEENABLE, 0);
			gD3DInterface->SetRenderState(GPURS_LIGHTING, 0);
		}
		gD3DInterface->SetPixelShader(nullptr);
		gD3DInterface->SetTextureStageState(1, 0xb, 0x10000);
		gD3DInterface->SetRenderState(GPURS_NORMALIZENORMALS, 1);
	}

	// 0x70f15c
	void EndMesh(D3DObject* theObject, const D3DMesh& theMesh) override
	{
		(void)theObject;
		(void)theMesh;
		gD3DInterface->SetRenderState(GPURS_DESTBLEND, 6);
		gD3DInterface->SetRenderState(GPURS_ZWRITEENABLE, 1);
		gD3DInterface->SetRenderState(GPURS_LIGHTING, 1);
		gD3DInterface->SetTextureStageState(1, 0xb, 1);
		gD3DInterface->SetRenderState(GPURS_NORMALIZENORMALS, 0);
	}
};

// ISexyFixedFunctionShader (0x501610; vtable 0x8183ec) and its ladder (0x501630 applies, 0x501890 removes): the shader
// holds the level it was applied at. The device's ValidateDevice (slot 0x30) is taken to succeed for every level
// Twist asks for (the device can do each of them), so the ladder ends at the first level the class has, and the
// "_fallback_forced" and "_fallback" settings it reads (0x4a3f30) are not.
class FixedFunctionShader {
public:
	virtual ~FixedFunctionShader() {}
	virtual bool Apply(int theLevel) = 0;   // +0x04: sets the states of the level; false when the class has no such level
	virtual void Remove(int theLevel) = 0;  // +0x08
	virtual bool UsesLights() { return false; }  // +0x0c
	int mLevel = 0;                         // +0x04 in the original: the level that was applied

	// 0x501630
	void ApplyLadder()
	{
		int aLevel = 1;
		if (mLevel < 1 || !Apply(mLevel)) {
			while (Apply(aLevel)) {
				mLevel = aLevel;
				break;
			}
		}
	}
	// 0x501890
	bool RemoveLadder()
	{
		if (0 < mLevel) {
			Remove(mLevel);
			return true;
		}
		return false;
	}
};

// Draw3DSidebar's SidebarEngineFixedFunctionShader (vtable 0x7e4324, the object at 0x85d788; the name
// "shader_sidebar_engine_p"): the engine mesh is always drawn by it, with or without pixel shaders
class SidebarEngineShader : public FixedFunctionShader {
public:
	float mUnknown0C[6] = {0, 0, 0, 0, 0, 0};  // +0x0c: the two lights' directions (0x7100b2's, which this class does not use)
	float mUnknown24 = 0;                      // +0x24
	float mUnknown28 = 0;                      // +0x28
	float mUnknown2C = 0;                      // +0x2c
	float mGlow = 0;                           // +0x30: how bright the engine burns

	// 0x70eef8
	bool Apply(int theLevel) override
	{
		if (theLevel == 1) {
			int aGlow = Ftol((double)mGlow * 255.0);
			gD3DInterface->SetRenderState(GPURS_TEXTUREFACTOR, Color(aGlow, aGlow, aGlow, 255).ToInt());
			gD3DInterface->SetTextureStageState(0, 1, 4);
			gD3DInterface->SetTextureStageState(0, 2, 2);
			gD3DInterface->SetTextureStageState(0, 3, 3);
			gD3DInterface->SetTextureStageState(0, 4, 4);
			gD3DInterface->SetTextureStageState(0, 5, 2);
			gD3DInterface->SetTextureStageState(0, 6, 3);
		}
		return theLevel == 1;
	}

	// 0x70efbd
	void Remove(int theLevel) override
	{
		(void)theLevel;
		gD3DInterface->SetTextureStageState(0, 1, 4);
		gD3DInterface->SetTextureStageState(0, 2, 2);
		gD3DInterface->SetTextureStageState(0, 3, 0);
		gD3DInterface->SetTextureStageState(0, 4, 4);
		gD3DInterface->SetTextureStageState(0, 5, 2);
		gD3DInterface->SetTextureStageState(0, 6, 0);
	}

	// 0x70eef5 (slot 3): no lights; the engine draws with lighting off
	bool UsesLights() override { return false; }
};

// Draw3DSidebar's LocalListener (vtable 0x7e433c, the object at 0x85d718)
class SidebarListener : public D3DObjectListener {
public:
	Hyperspace* mHyperspace = nullptr;   // +0x04
	bool mFixedFunction = false;         // +0x08: the card cannot run the pixel shaders (caps bit 2)
	float mConstants0[4] = {0, 0, 0, 0}; // +0x0c: the first constant register of the sidebar's shaders
	float mConstants1[4] = {0, 0, 0, 0}; // +0x1c: the second
	GPUMatrix mTubeTexture = GPUMatrix::Identity();  // +0x2c: the tube texture's transform
	FixedFunctionShader* mShader = nullptr;          // +0x6c: the fixed-function class applied for this mesh

	SidebarEngineShader mEngine;  // the function's static object (0x85d788)

	// 0x7131a4
	void BeginMesh(const D3DMesh& theMesh, bool hasBump) override
	{
		float aDirection[3] = {-0.70710677f, -0.70710677f, 0.0f};  // 0xbf34fdf4 twice, the first light's y and z
		float aSecond[3] = {0.0f, 0.0f, 0.0f};
		float aIntensity = 1.0f;
		mShader = nullptr;
		gD3DInterface->SetTextureStageState(1, 0xb, 0x10000);
		gD3DInterface->SetRenderState(GPURS_NORMALIZENORMALS, 1);
		TwistShader* aShader = nullptr;
		bool anIsFixed = false;  // the class path runs (the fixed-function case) for the sidebar and tube meshes only
		if (theMesh.mName == "EngineShader") {
			mShader = &mEngine;
			aShader = nullptr;
			anIsFixed = true;
		} else if (theMesh.mName == "TubeShader") {
			gD3DInterface->SetTextureStageState(0, 0x18, 2);
			gD3DInterface->SetTransform(GPUTS_TEXTURE0, mTubeTexture);
			aShader = gTwistShaders[TWISTSHADER_SIDEBAR_TUBE_P];
			if (!mFixedFunction) {
				gD3DInterface->SetTexture(1, IMAGE_TUBESPARKLES);
				gD3DInterface->SetTextureWrap(1, true);
			} else {
				NotPorted("SidebarTubeFixedFunctionShader 0x70ecc2 (a card without ps_2_0)");
			}
		} else {
			aShader = hasBump ? gTwistShaders[TWISTSHADER_SIDEBAR_BUMP_P] : gTwistShaders[TWISTSHADER_SIDEBAR_P];
			if (mFixedFunction)
				NotPorted("SidebarFixedFunctionShader 0x70e8b0 (a card without ps_2_0)");
		}
		(void)anIsFixed;
		if (mShader == nullptr && mFixedFunction) {
			gD3DInterface->SetPixelShader(nullptr);
		} else if (mShader != nullptr && (mFixedFunction || theMesh.mName == "EngineShader")) {
			gD3DInterface->SetPixelShader(nullptr);
			if (!mFixedFunction) {
				gD3DInterface->SetShaderConstant(nullptr, 0, mConstants0);
				gD3DInterface->SetShaderConstant(nullptr, 1, mConstants1);
			}
		} else {
			gD3DInterface->SetPixelShader(aShader);
			gD3DInterface->SetShaderConstant(aShader, 0, mConstants0);
			gD3DInterface->SetShaderConstant(aShader, 1, mConstants1);
		}
		if (mShader != nullptr) {
			// the shader's fields as the original sets them (0x7131a4 on: its lights at +0x0c to +0x24, the glow's two
			// parts, and the glow)
			SidebarEngineShader* anEngine = static_cast<SidebarEngineShader*>(mShader);
			anEngine->mUnknown0C[0] = 0.0f;
			anEngine->mUnknown0C[1] = aDirection[0];
			anEngine->mUnknown0C[2] = aDirection[1];
			anEngine->mUnknown0C[3] = aSecond[0];
			anEngine->mUnknown0C[4] = aSecond[1];
			anEngine->mUnknown0C[5] = aSecond[2];
			anEngine->mUnknown24 = aIntensity;
			anEngine->mUnknown28 = mConstants0[1];
			anEngine->mUnknown2C = mConstants0[2];
			anEngine->mGlow = (float)mHyperspace->mUnknown1160.GetOutVal();
			if (anEngine->UsesLights()) {
				gD3DInterface->SetRenderState(GPURS_LIGHTING, 1);
				NotPorted("SetLights of the fixed-function sidebar classes 0x7100b2 (only a class with lights calls it)");
			} else {
				gD3DInterface->SetRenderState(GPURS_LIGHTING, 0);
			}
			anEngine->ApplyLadder();
		}
	}

	// 0x70f036
	void EndMesh(D3DObject* theObject, const D3DMesh& theMesh) override
	{
		(void)theObject;
		(void)theMesh;
		if (mShader != nullptr) {
			mShader->RemoveLadder();
			mShader = nullptr;
		}
		gD3DInterface->SetTextureStageState(1, 0xb, 1);
		gD3DInterface->SetRenderState(GPURS_NORMALIZENORMALS, 0);
		gD3DInterface->SetTextureStageState(0, 0x18, 0);
		gD3DInterface->SetTextureWrap(1, false);
		gD3DInterface->SetTexture(1, nullptr);
		gD3DInterface->SetRenderState(GPURS_LIGHTING, 1);
		gD3DInterface->SetRenderState(GPURS_DESTBLEND, 6);
	}
};

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// Draw (0x713e2d), the part for gD3DInterface

void Hyperspace::Draw3D(Graphics* g)
{
	if (((mFlag155 && !gApp->mVizManager->mVisible) || mState == 0) || mTransition == 9 || mTransition == 10) {
		float aFovA = (float)(mUnknownF28.GetOutVal() * 4.0 * mWarmUp + 75.0);
		float aFovB = (float)(mUnknownF98.GetOutVal() * 4.0 * mWarmUp + 75.0);
		float aFovC = (float)(mUnknown1008.GetOutVal() * 4.0 * mWarmUp + 75.0);
		double anAspect = (double)mWidth / (double)mHeight;
		// the three backdrops, each through the camera at +0x13f4 with its own field of view
		mCamera13F4.SetFov((float)(anAspect * (double)aFovB), (float)anAspect, 1.0f, 10000.0f);
		WorldGraphicsListener aListener(&mCamera13F4, true);
		aListener.SetCulling(false);
		aListener.SetDepth(false, false);
		aListener.Apply();
		GPUMatrix aMatrix = GPUMatrix::Identity();
		Mat4Scale(aMatrix, 30.0f, 30.0f, 30.0f);
		Mat4RotateX(aMatrix, -1.5707963705062866f);
		float anAngle = (float)((((double)mLevel * 3.1415927410125732) + ((double)mLevel * 3.1415927410125732)) / 10.0);
		Mat4RotateY(aMatrix, anAngle);
		anAngle = (float)(((double)(mLevel / 10) * 3.1415927410125732) / 3.0);
		Mat4RotateX(aMatrix, anAngle);
		gD3DInterface->SetTextureLinearFilter(0, true);
		gD3DInterface->SetRenderState(GPURS_DESTBLEND, 6);
		gD3DInterface->DrawObject(gApp->mOuterSpaceObject, aMatrix, Color::White, false);
		aListener.Remove();
		mCamera13F4.SetFov((float)(anAspect * (double)aFovA), (float)anAspect, 1.0f, 10000.0f);
		aListener.Apply();
		gD3DInterface->SetRenderState(GPURS_DESTBLEND, 2);
		gD3DInterface->DrawObject(gApp->mSpaceObject, aMatrix, Color::White, false);
		aListener.Remove();
		mCamera13F4.SetFov((float)(anAspect * (double)aFovC), (float)anAspect, 1.0f, 10000.0f);
		aListener.Apply();
		gD3DInterface->DrawObject(gApp->mStarsObject, aMatrix, Color::White, false);
		gD3DInterface->SetRenderState(GPURS_DESTBLEND, 6);
		aListener.Remove();
		mCamera13F4.SetFov((float)(anAspect * 60.0), (float)anAspect, 1.0f, 10000.0f);
		if (mIs3D) {
			DrawCrystals();
			if (mWorld == 0)
				DrawSaucer();
			else if (mWorld == 1)
				DrawPlanet(gApp->mPlanetObjects[mPlanetObject]);
		}
		if (((mState != 0 && mState != 5) && (mTransition != 0xb && (mState != 0xc && mState != 0xb))) &&
			(mUnknown5D8.GetOutVal() < 1.0 || 2 < mState))
			DrawTunnel(g);
		if (mIs3D && !mFlag157) {
			Draw3DSidebar();
			DrawCone();
			DrawParticles();
		}
		if (mFlag158)
			DrawTravellers(g);
	}
	if ((mState == 0xc || mState == 0xb) && mFlag159)
		DrawOldPicture(g);
	if (mFlash.GetOutVal() == 0.0)
		return;
	mWidgetManager->FlushDeferredOverlayWidgets(0x7fffffff);
	g->SetColor(Color(255, 255, 255, Ftol(mFlash.GetOutVal() * 255.0)));  // CurvedVal::GetColor 0x6a1984
	g->FillRect(0, 0, mWidth, mHeight);
}

///////////////////////////////////////////////////////////////////////////////
// The level's world

void Hyperspace::DrawCrystals()
{
	// 0x71042e
	if (mCrystals.empty())
		return;
	gD3DInterface->SetPixelShader(nullptr);
	WorldGraphicsListener aListener(&mCamera139C, !mIs3D);
	aListener.SetCulling(true);
	aListener.SetDepth(true, true);
	aListener.SetLighting(true);
	SetPlainMaterial();
	D3DLightColors aColors = LightColors(Color::White, Color::Black, Color(0, 0, 0));
	Vec3 aDirection{0.0f, 0.0f, -1.0f};
	gD3DInterface->SetDirectionalLight(0, aDirection, aColors);
	gD3DInterface->LightEnable(0, true);
	gD3DInterface->SetTextureStageState(1, 0xb, 0x10000);
	gD3DInterface->SetRenderState(GPURS_NORMALIZENORMALS, 1);
	unsigned int i = 0;
	if (!mCrystals.empty()) {
		do {
			aListener.SetLighting(true);
			aListener.SetDepth(true, true);
			gD3DInterface->SetRenderState(GPURS_DESTBLEND, 6);
			SetPlainMaterial();
			aListener.Apply();
			Transform3D aTransform = mCrystals[i].mTransform;
			GPUMatrix aMatrix = WorldMatrixOf(aTransform);
			gD3DInterface->SetRenderState(GPURS_DESTBLEND, 2);
			gD3DInterface->SetPixelShader(gTwistShaders[TWISTSHADER_SAUCER_WINDOW_P]);
			gD3DInterface->DrawObject(gApp->mCrystalObjects[mCrystals[i].mType], aMatrix, Color::White, false);
			gD3DInterface->SetRenderState(GPURS_DESTBLEND, 6);
			gD3DInterface->SetPixelShader(nullptr);
			aListener.Remove();
			i++;
		} while (i < mCrystals.size());
	}
	gD3DInterface->SetTextureStageState(1, 0xb, 1);
	gD3DInterface->SetRenderState(GPURS_NORMALIZENORMALS, 0);
	gD3DInterface->SetVertexShader(nullptr);
	gD3DInterface->SetPixelShader(nullptr);
}

void Hyperspace::DrawSaucer()
{
	// 0x710c13
	SaucerListener aSaucerListener;
	aSaucerListener.mHyperspace = this;
	aSaucerListener.mCameo = gSaucerCameo;  // gApp+0xf98 (SetWorldCurves(0) steps it)
	gD3DInterface->SetVertexShader(nullptr);
	D3DObjectListener* anOldListener = gApp->mSaucerObject->mListener;
	gApp->mSaucerObject->mListener = &aSaucerListener;
	WorldGraphicsListener aListener(&mCamera139C, !mIs3D);
	aListener.SetCulling(true);
	aListener.SetDepth(true, true);
	aListener.SetLighting(true);
	SetPlainMaterial();
	D3DLightColors aColors = LightColors(Color::White, Color::Black, Color(0, 0, 0));
	Vec3 aDirection{0.0f, -1.0f, -1.0f};
	gD3DInterface->SetDirectionalLight(0, aDirection, aColors);
	aColors.mDiffuse = Color(0, 128, 255, 255);
	aDirection = Vec3{0.0f, 1.0f, 1.0f};
	gD3DInterface->SetDirectionalLight(1, aDirection, aColors);
	gD3DInterface->LightEnable(0, true);
	gD3DInterface->LightEnable(1, true);
	aListener.Apply();
	Transform3D aTransform = mSaucer;
	GPUMatrix aMatrix = WorldMatrixOf(aTransform);
	gD3DInterface->DrawObject(gApp->mSaucerObject, aMatrix, Color::White, false);
	aListener.Remove();
	gApp->mSaucerObject->mListener = anOldListener;
	gD3DInterface->SetPixelShader(nullptr);
	aListener.Apply();
	Mat3 aTurn;
	float anAngle = (float)((double)(mUpdateCnt % 0x7c) / 20.0);
	aTurn.RotateY(anAngle);
	aTurn = aTurn * aTransform.mRot;
	aTransform.mRot = aTurn;
	aMatrix = WorldMatrixOf(aTransform);
	gD3DInterface->SetRenderState(GPURS_DESTBLEND, 2);
	gD3DInterface->DrawObject(gApp->mSaucerLightsObject, aMatrix, Color::White, false);
	gD3DInterface->SetRenderState(GPURS_DESTBLEND, 6);
	aListener.Remove();
}

void Hyperspace::DrawPlanet(D3DObject* thePlanet)
{
	// 0x7106f6
	PlanetListener aPlanetListener;
	aPlanetListener.mHyperspace = this;
	D3DObjectListener* anOldListener = thePlanet->mListener;
	thePlanet->mListener = &aPlanetListener;
	WorldGraphicsListener aListener(&mCamera139C, !mIs3D);
	aListener.SetCulling(true);
	aListener.SetDepth(true, true);
	aListener.SetLighting(true);
	SetPlainMaterial();
	D3DLightColors aColors = LightColors(Color::White, Color::Black, Color(0, 0, 0));
	Vec3 aDirection{0.0f, -1.0f, -1.0f};
	gD3DInterface->SetDirectionalLight(0, aDirection, aColors);
	aColors.mDiffuse = Color(0, 128, 255, 255);
	aDirection = Vec3{0.0f, 1.0f, 1.0f};
	gD3DInterface->SetDirectionalLight(1, aDirection, aColors);
	gD3DInterface->LightEnable(0, true);
	gD3DInterface->LightEnable(1, true);
	aListener.Apply();
	Transform3D aTransform;
	aTransform.mRot.RotateX(0.30000001192092896f);
	float anAngle = (float)((double)(mUpdateCnt % 0x274) / 100.0);
	aTransform.mRot.RotateY(anAngle);
	Translate(aTransform, 0.0f, 0.0f, -25.0f);
	GPUMatrix aMatrix = WorldMatrixOf(aTransform);
	gD3DInterface->DrawObject(thePlanet, aMatrix, Color::White, false);
	gD3DInterface->SetRenderState(GPURS_DESTBLEND, 2);
	gD3DInterface->SetRenderState(GPURS_ZWRITEENABLE, 0);
	aTransform = Transform3D();
	aTransform.mRot.RotateX(0.30000001192092896f);
	aTransform.mRot.RotateY(0.10000000149011612f);
	Translate(aTransform, 0.0f, 0.0f, -25.0f);
	if (mRingCrystals.empty()) {
		aMatrix = WorldMatrixOf(aTransform);
		gD3DInterface->DrawObject(gApp->mPlanetRingsObject, aMatrix, Color::White, false);
	} else {
		gD3DInterface->SetRenderState(GPURS_LIGHTING, 0);
		gD3DInterface->SetPixelShader(gTwistShaders[TWISTSHADER_SAUCER_WINDOW_P]);
		gD3DInterface->SetTextureStageState(1, 0xb, 0x10000);
		gD3DInterface->SetRenderState(GPURS_NORMALIZENORMALS, 1);
		unsigned int i = 0;
		if (!mRingCrystals.empty()) {
			do {
				aTransform = mRingCrystals[i].mTransform;
				aTransform.mRot.RotateX(0.30000001192092896f);
				aTransform.mRot.RotateY(0.10000000149011612f);
				Mat3 aTurn = mRingCrystals[i].mUnknown40;
				aTurn = aTurn * mRingCrystals[i].mUnknown64;
				Transform3D aSpin = TransformOfMat3(aTurn);
				aTransform = Compose(aTransform, aSpin);
				Translate(aTransform, 0.0f, 0.0f, -25.0f);
				aMatrix = WorldMatrixOf(aTransform);
				gD3DInterface->DrawObject(gApp->mCrystalObjects[mRingCrystals[i].mType], aMatrix, Color::White, false);
				i++;
			} while (i < mRingCrystals.size());
		}
		gD3DInterface->SetPixelShader(nullptr);
		gD3DInterface->SetRenderState(GPURS_LIGHTING, 1);
		gD3DInterface->SetTextureStageState(1, 0xb, 1);
		gD3DInterface->SetRenderState(GPURS_NORMALIZENORMALS, 0);
	}
	gD3DInterface->SetRenderState(GPURS_DESTBLEND, 6);
	gD3DInterface->SetRenderState(GPURS_ZWRITEENABLE, 1);
	aListener.Remove();
	thePlanet->mListener = anOldListener;
	gD3DInterface->SetVertexShader(nullptr);
	gD3DInterface->SetPixelShader(nullptr);
}

void Hyperspace::MakeCrystals()
{
	// 0x717dd6: four crystals at the points 0.2, 0.35, 0.5 and 0.65 along the orbit's curves, each turned and spun at
	// random. The draws run in the original's order: every Rand below is taken as 0x45c120 (the int range) or 0x45c130
	// (the float range) takes it.
	float aT = 0.20000000298023224f;  // 0x7d7708
	do {
		WorldCrystal aCrystal;
		float aDistance = (float)((double)Rand(5.0f) + 3.0);
		float anAngle = Rand(6.2831854820251465f);
		aCrystal.mType = Rand(7);
		int aSign = Rand(1) * 2 - 1;
		aCrystal.mUnknown40 = (float)(((double)Rand(0.019999999552965164f) + 0.009999999776482582) * (double)aSign);
		aSign = Rand(1) * 2 - 1;
		aCrystal.mUnknown44 = (float)(((double)Rand(0.019999999552965164f) + 0.009999999776482582) * (double)aSign);
		ScaleBy(aCrystal.mTransform, 0.20000000298023224f, 0.20000000298023224f, 0.20000000298023224f);
		aCrystal.mTransform.mRot.RotateX(Rand(6.2831854820251465f));
		aCrystal.mTransform.mRot.RotateY(Rand(6.2831854820251465f));
		aCrystal.mTransform.mRot.RotateZ(Rand(6.2831854820251465f));
		float aSine = (float)std::sin((double)anAngle);
		float aCosine = (float)std::cos((double)anAngle);
		float aZ = (float)mUnknownA58.GetOutVal((double)aT);
		float aY = (float)((double)aSine * (double)aDistance + mUnknown9E8.GetOutVal((double)aT));
		float aX = (float)((double)aCosine * (double)aDistance + mUnknown978.GetOutVal((double)aT));
		Translate(aCrystal.mTransform, aX, aY, aZ);
		mCrystals.push_back(aCrystal);
		aT = (float)((double)aT + 0.15000000596046448);
	} while (aT < 0.699999988079071f);
}

void Hyperspace::MakeRingCrystals(int theCount)
{
	// 0x717fcc: theCount crystals in rings round the planet, and the planet's model and picture
	int aRing = 1;
	if (0 < theCount) {
		do {
			RingCrystal aCrystal;
			aCrystal.mType = Rand(7);
			int aSign = Rand(2) * 2 - 1;
			aCrystal.mUnknown88.x = (float)(((double)Rand(0.019999999552965164f) + 0.009999999776482582) * (double)aSign);
			aSign = Rand(2) * 2 - 1;
			aCrystal.mUnknown88.y = (float)(((double)Rand(0.019999999552965164f) + 0.009999999776482582) * (double)aSign);
			aCrystal.mUnknown88.z = (float)((double)Rand(0.019999999552965164f) + 0.009999999776482582);
			float aSize = (float)((double)Rand(0.10000000149011612f) + 0.10000000149011612);
			ScaleBy(aCrystal.mTransform, aSize, aSize, aSize);
			Translate(aCrystal.mTransform, 0.0f, 0.0f, (float)aRing + (float)aRing + 3.5f);
			aSign = Rand(2) * 2 - 1;
			float aTurn = (float)(((double)Rand(0.60000002384185791f) + 0.40000000596046448) * (double)aSign);
			aCrystal.mTransform.mRot.RotateZ(aTurn);
			aCrystal.mTransform.mRot.RotateY(Rand(6.2831854820251465f));
			mRingCrystals.push_back(aCrystal);
			aRing++;
		} while (aRing <= theCount);
	}
	mPlanetObject = Rand(2);
	Image* aPlanets[7] = {IMAGE_PLANET1, IMAGE_PLANET2, IMAGE_PLANET3, IMAGE_PLANET4, IMAGE_PLANET5, IMAGE_PLANET6, IMAGE_PLANET7};
	mPlanetImage = aPlanets[Rand(7)];
}

///////////////////////////////////////////////////////////////////////////////
// The sidebar, the cone and the souls

void Hyperspace::Draw3DSidebar()
{
	// 0x711529
	gD3DInterface->SetRenderState(GPURS_DESTBLEND, 6);
	static SidebarListener aSidebarListener;  // the function's static objects (0x85d7f0, 0x85d7bc, 0x85d788, 0x85d718)
	aSidebarListener.mHyperspace = this;
	gD3DInterface->SetVertexShader(nullptr);
	aSidebarListener.mFixedFunction = true;
	if (gApp != nullptr && gApp->mDDInterface != nullptr && gD3DInterface != nullptr) {
		unsigned long aCaps = gD3DInterface->GetCaps(0);
		aSidebarListener.mFixedFunction = true;
		if ((aCaps & 2) != 0)
			aSidebarListener.mFixedFunction = false;
	}
	aSidebarListener.mTubeTexture = GPUMatrix::Identity();
	aSidebarListener.mTubeTexture.m[2][0] = (float)(mUnknown6B8.GetOutVal() * 0.5);
	float aConstants[4];
	aConstants[0] = (float)mUnknown2C8.GetOutVal();
	aConstants[1] = (float)mUnknown338.GetOutVal();
	double aValue = mUnknown338.GetOutVal() + (1.0 - mUnknown3A8.GetOutVal());
	aConstants[2] = (float)(aValue > 0.0 ? (aValue < 1.0 ? aValue : 1.0) : 0.0);
	aConstants[3] = (float)((double)(mUpdateCnt % 0x15e) / 350.0);
	std::memcpy(aSidebarListener.mConstants0, aConstants, 0x10);
	Transform3D aSidebarCamera = mCamera144C.mTransform;
	float aShift[3];
	aShift[0] = aSidebarCamera.mPos.x - mUnknown14C0.mPos.x;
	aShift[1] = aSidebarCamera.mPos.y - mUnknown14C0.mPos.y;
	aShift[2] = aSidebarCamera.mPos.z - mUnknown14C0.mPos.z;
	float aSidebarShift[3] = {-aShift[0], aShift[1], aShift[2]};
	std::memcpy(aSidebarListener.mConstants1, aSidebarShift, 0xc);
	D3DObjectListener* anOldListener = gApp->mSidebarObject->mListener;
	gApp->mSidebarObject->mListener = &aSidebarListener;
	double anAspect = (double)mWidth / (double)mHeight;
	float aFov = (float)((double)(float)mUnknown2C8.GetOutVal() * 45.0 + 15.0);
	float aFovTimesAspect = (float)((double)aFov * (double)(float)anAspect);
	float aNear = (float)((double)(float)mUnknown2C8.GetOutVal() * -0.7999999970197678 + 1.0);
	float aFar = (float)((double)(float)mUnknown2C8.GetOutVal() * -1500.0 + 2000.0);
	float aCenterY = (float)mUnknown2C8.GetOutVal();
	aCenterY = (float)(0.008999999612569809 + -0.008999999612569809 * (double)aCenterY);
	mCamera144C.SetFrustum(aFovTimesAspect, (float)anAspect, 0.0f, aCenterY, aNear, aFar);
	Transform3D aTransform;
	float aBack = (float)(((1.0 - mUnknown2C8.GetOutVal()) / (double)aFov) * 73.5);
	float aGrow = (float)mUnknown2C8.GetOutVal();
	float aZ = (float)((double)mUnknown1390[2] + (double)aBack);
	float aX = (float)((double)(float)(0.017000000923871994 * (double)aGrow - 0.017000000923871994) + (double)mUnknown1390[0]);
	Translate(aTransform, aX, mUnknown1390[1], aZ);
	aTransform.mRot.RotateZ((float)mUnknownE48.GetOutVal());
	aTransform.mRot.RotateY((float)mUnknownEB8.GetOutVal());
	aTransform.mRot.RotateX((float)mUnknownDD8.GetOutVal());
	mCamera144C.mTransform = aTransform;
	OffCenterListener aListener(&mCamera144C, !mIs3D);
	aListener.SetCulling(true);
	aListener.SetDepth(true, true);
	aListener.SetLighting(true);
	SetPlainMaterial();
	D3DLightColors aColors = LightColors(Color::White, Color::White, Color(0, 0, 0));
	Vec3 aDirection{0.0f, 0.0f, -1.0f};
	gD3DInterface->SetDirectionalLight(0, aDirection, aColors);
	gD3DInterface->LightEnable(0, true);
	aListener.Apply();
	Transform3D aPlace = mUnknown14C0;
	Mat3 aTurn;
	Vec3 anAxis{0.0f, 1.0f, 0.0f};
	float aGlance = (float)mUnknown2C8.GetOutVal();
	aGlance = (float)(8.0 + -8.0 * (double)aGlance);
	RotateAbout(aTurn, (float)((double)aGlance * 3.1415927410125732 / 180.0), anAxis);
	aPlace.mRot = aPlace.mRot * aTurn;
	GPUMatrix aMatrix = WorldMatrixOf(aPlace);
	gD3DInterface->DrawObject(gApp->mSidebarObject, aMatrix, Color::White, false);
	aListener.Remove();
	gApp->mSidebarObject->mListener = anOldListener;
	gD3DInterface->SetVertexShader(nullptr);
	gD3DInterface->SetPixelShader(nullptr);
}

void Hyperspace::DrawCone()
{
	// 0x71019e
	OffCenterListener aListener(&mCamera144C, !mIs3D);
	aListener.SetCulling(false);
	aListener.SetDepth(true, false);
	aListener.Apply();
	int anAlpha = Ftol(mUnknown1080.GetOutVal() * 255.0);
	Color aColor(0, 0, 0, anAlpha);
	gD3DInterface->SetRenderState(GPURS_LIGHTING, 0);
	gD3DInterface->SetRenderState(GPURS_TEXTUREFACTOR, aColor.ToInt());
	gD3DInterface->SetTextureStageState(0, 4, 4);
	gD3DInterface->SetTextureStageState(0, 5, 3);
	gD3DInterface->SetRenderState(GPURS_DESTBLEND, 2);
	gD3DInterface->SetTextureLinearFilter(0, true);
	gD3DInterface->SetTextureWrap(0, true);
	GPUMatrix aTexture = GPUMatrix::Identity();
	aTexture.m[2][0] = (float)((double)(mUpdateCnt % 0x28) / 40.0);
	gD3DInterface->SetTextureStageState(0, 0x18, 2);
	gD3DInterface->SetTransform(GPUTS_TEXTURE0, aTexture);
	Transform3D aPlace = mUnknown14C0;
	Mat3 aTurn;
	Vec3 anAxis{0.0f, 1.0f, 0.0f};
	float aGlance = (float)mUnknown2C8.GetOutVal();
	aGlance = (float)(8.0 + -8.0 * (double)aGlance);
	RotateAbout(aTurn, (float)((double)aGlance * 3.1415927410125732 / 180.0), anAxis);
	aPlace.mRot = aPlace.mRot * aTurn;
	GPUMatrix aMatrix = WorldMatrixOf(aPlace);
	gD3DInterface->DrawObject(gApp->mWarpConeObject, aMatrix, Color::White, false);
	gD3DInterface->SetTextureStageState(0, 0x18, 0);
	gD3DInterface->SetTextureStageState(0, 4, 4);
	gD3DInterface->SetTextureStageState(0, 5, 2);
	gD3DInterface->SetRenderState(GPURS_DESTBLEND, 6);
	gD3DInterface->SetTextureWrap(0, false);
	aListener.Remove();
	aListener.SetCulling(true);
	aListener.SetDepth(true, true);
}

void Hyperspace::DrawTravellers(Graphics* g)
{
	// 0x710f5b: each soul of the last level's special gems drawn where the sidebar's camera puts its place, scaled by
	// how near it is
	if (gBoard == nullptr)
		return;
	unsigned int i = 0;
	if (!gBoard->mUnknown26B0.empty()) {
		do {
			GemSoul* aSoul = gBoard->mUnknown26B0[i];
			Vec3 aScreen = mCamera144C.ProjectWorld(Vec3{aSoul->mUnknown7BC[0], aSoul->mUnknown7BC[1], aSoul->mUnknown7BC[2]});
			float aSize = (float)(0.0018 / (double)aScreen.z);
			double aX = aSoul->mUnknown748.GetOutVal();
			double aX2 = aSoul->mUnknown748.GetOutVal();
			float aPlaceX = (float)((double)mWidth * (double)aScreen.x * aX2 + (double)aSoul->mDrawX * (1.0 - aX));
			double aY = aSoul->mUnknown748.GetOutVal();
			double aY2 = aSoul->mUnknown748.GetOutVal();
			float aPlaceY = (float)((double)mHeight * (double)aScreen.y * aY2 + (double)aSoul->mDrawY * (1.0 - aY));
			double aScale1 = aSoul->mUnknown748.GetOutVal();
			double aScale2 = aSoul->mUnknown748.GetOutVal();
			aSize = (float)((double)aSize * aScale2 + (1.0 - aScale1));
			SexyTransform2D aMatrix;
			aMatrix.LoadIdentity();
			aMatrix.Translate((float)-aSoul->mDrawX, (float)-aSoul->mDrawY);
			aMatrix.Scale(aSize, aSize);
			aMatrix.Translate(aPlaceX, aPlaceY);
			if (0.009999999776482582f < aSize && aSize < 8.0f) {
				gD3DInterface->PushTransform(aMatrix, true);
				aSoul->Draw(g);
				gD3DInterface->PopTransform();
			}
			i++;
		} while (i < gBoard->mUnknown26B0.size());
	}
}

void Hyperspace::DrawTunnel3D(Graphics* g)
{
	(void)g;
	NotPorted("Hyperspace::DrawTunnel 3D half 0x711b65 (cube tunnel, CubeFixedFunctionShader/RainbowCube)");
}

void Hyperspace::DrawParticles()
{
	NotPorted("Hyperspace::DrawParticles 0x71378b (the ship's thrust)");
}


///////////////////////////////////////////////////////////////////////////////
// SetTransition (0x7181b3) with gD3DInterface set


// 0x718d8e: the curves each world sets (the jump table at 0x71c153, one block of SetCurve calls per world; GTS takes the
// address of the exe's curve string; the path curves follow the jump's clock, mUnknown6B8)
void Hyperspace::SetWorldCurves(int theWorld)
{
	switch (theWorld) {
	case 0:  // 0x718d95
		gSaucerCameo = (gSaucerCameo + 1) % 5;  // gApp+0xf98 (0x718da2): the picture the saucer's "Image" mesh wears
		mUnknownB08.SetCurve(GTS(0x7e5d58), &mUnknown6B8);
		mUnknownB78.SetCurve(GTS(0x7e5d2c), &mUnknown6B8);
		mUnknownBE8.SetCurve(GTS(0x7e5d04), &mUnknown6B8);
		mUnknown978.SetCurve(GTS(0x7e5cd0), &mUnknown6B8);
		mUnknown9E8.SetCurve(GTS(0x7e5c98), &mUnknown6B8);
		mUnknownA58.SetCurve(GTS(0x7e5c74), &mUnknown6B8);
		mUnknownC88.SetCurve(GTS(0x7e5ec0), &mUnknown6B8);
		mUnknownCF8.SetCurve(GTS(0x80d3c8), &mUnknown6B8);
		mUnknownD68.SetCurve(GTS(0x7e5e94), &mUnknown6B8);
		mUnknownEB8.SetCurve(GTS(0x7e5c40), &mUnknown6B8);
		mUnknownE48.SetCurve(GTS(0x7e5c18), &mUnknown6B8);
		mUnknownDD8.SetCurve(GTS(0x7e5bec), &mUnknown6B8);
		break;
	case 1:  // 0x7190ad
		mUnknown978.SetCurve(GTS(0x7e5bc0), &mUnknown6B8);
		mUnknown9E8.SetCurve(GTS(0x7e5b94), &mUnknown6B8);
		mUnknownA58.SetCurve(GTS(0x7e5b70), &mUnknown6B8);
		mUnknownC88.SetCurve(GTS(0x7e5b44), &mUnknown6B8);
		mUnknownCF8.SetCurve(GTS(0x7e5b18), &mUnknown6B8);
		mUnknownD68.SetCurve(GTS(0x7e5af0), &mUnknown6B8);
		mUnknownEB8.SetCurve(GTS(0x7e5ac0), &mUnknown6B8);
		mUnknownE48.SetCurve(GTS(0x7e5c18), &mUnknown6B8);
		mUnknownDD8.SetCurve(GTS(0x7e5a94), &mUnknown6B8);
		mUnknown6B8.SetCurve(GTS(0x7e5a64));
		mUnknown10F0.SetCurve(GTS(0x7e5a18));
		mUnknown1160.SetCurve(GTS(0x7e59d8));
		break;
	case 2:  // 0x7193a7
		mUnknown978.SetCurve(GTS(0x7e59b0), &mUnknown6B8);
		mUnknown9E8.SetCurve(GTS(0x7e5b94), &mUnknown6B8);
		mUnknownA58.SetCurve(GTS(0x7e5b70), &mUnknown6B8);
		mUnknownC88.SetCurve(GTS(0x7e5990), &mUnknown6B8);
		mUnknownCF8.SetCurve(GTS(0x7e5964), &mUnknown6B8);
		mUnknownD68.SetCurve(GTS(0x7e5934), &mUnknown6B8);
		mUnknownEB8.SetCurve(GTS(0x7e5910), &mUnknown6B8);
		mUnknownE48.SetCurve(GTS(0x7e58e0), &mUnknown6B8);
		mUnknownDD8.SetCurve(GTS(0x7e58b0), &mUnknown6B8);
		mUnknown6B8.SetCurve(GTS(0x7e5a64));
		mUnknown10F0.SetCurve(GTS(0x7e5a18));
		mUnknown1160.SetCurve(GTS(0x7e59d8));
		break;
	case 3:  // 0x7196a1
		mUnknown978.SetCurve(GTS(0x7e588c), &mUnknown6B8);
		mUnknown9E8.SetCurve(GTS(0x7e5854), &mUnknown6B8);
		mUnknownA58.SetCurve(GTS(0x7e5b70), &mUnknown6B8);
		mUnknownC88.SetCurve(GTS(0x7e5990), &mUnknown6B8);
		mUnknownCF8.SetCurve(GTS(0x7e5834), &mUnknown6B8);
		mUnknownD68.SetCurve(GTS(0x7e580c), &mUnknown6B8);
		mUnknownEB8.SetCurve(GTS(0x7e57e4), &mUnknown6B8);
		mUnknownE48.SetCurve(GTS(0x7e57b8), &mUnknown6B8);
		mUnknownDD8.SetCurve(GTS(0x7e5790), &mUnknown6B8);
		mUnknown6B8.SetCurve(GTS(0x7e5a64));
		mUnknown10F0.SetCurve(GTS(0x7e5a18));
		mUnknown1160.SetCurve(GTS(0x7e59d8));
		break;
	case 4:  // 0x71999b
		mUnknown978.SetCurve(GTS(0x7e5cd0), &mUnknown6B8);
		mUnknown9E8.SetCurve(GTS(0x7e5c98), &mUnknown6B8);
		mUnknownA58.SetCurve(GTS(0x7e5c74), &mUnknown6B8);
		mUnknownC88.SetCurve(GTS(0x7e5ec0), &mUnknown6B8);
		mUnknownCF8.SetCurve(GTS(0x80d3c8), &mUnknown6B8);
		mUnknownD68.SetCurve(GTS(0x7e5e94), &mUnknown6B8);
		mUnknownEB8.SetCurve(GTS(0x7e5c40), &mUnknown6B8);
		mUnknownE48.SetCurve(GTS(0x7e5c18), &mUnknown6B8);
		mUnknownDD8.SetCurve(GTS(0x7e5bec), &mUnknown6B8);
		break;
	case 5:  // 0x719bde
		mUnknown978.SetCurve(GTS(0x7e5768), &mUnknown6B8);
		mUnknown9E8.SetCurve(GTS(0x7e5b94), &mUnknown6B8);
		mUnknownA58.SetCurve(GTS(0x7e5b70), &mUnknown6B8);
		mUnknownC88.SetCurve(GTS(0x7e5748), &mUnknown6B8);
		mUnknownCF8.SetCurve(GTS(0x7e5724), &mUnknown6B8);
		mUnknownD68.SetCurve(GTS(0x7e56f8), &mUnknown6B8);
		mUnknownEB8.SetCurve(GTS(0x7e56d8), &mUnknown6B8);
		mUnknownE48.SetCurve(GTS(0x7e56b0), &mUnknown6B8);
		mUnknownDD8.SetCurve(GTS(0x7e5684), &mUnknown6B8);
		mUnknown6B8.SetCurve(GTS(0x7e5a64));
		mUnknown10F0.SetCurve(GTS(0x7e5a18));
		mUnknown1160.SetCurve(GTS(0x7e59d8));
		break;
	case 6:  // 0x719ed8
		mUnknown978.SetCurve(GTS(0x7e5654), &mUnknown6B8);
		mUnknown9E8.SetCurve(GTS(0x7e5614), &mUnknown6B8);
		mUnknownA58.SetCurve(GTS(0x7e5c74), &mUnknown6B8);
		mUnknownC88.SetCurve(GTS(0x7e55f4), &mUnknown6B8);
		mUnknownCF8.SetCurve(GTS(0x80d3c8), &mUnknown6B8);
		mUnknownD68.SetCurve(GTS(0x7e55d0), &mUnknown6B8);
		mUnknownEB8.SetCurve(GTS(0x7e5c40), &mUnknown6B8);
		mUnknownE48.SetCurve(GTS(0x7e5c18), &mUnknown6B8);
		mUnknownDD8.SetCurve(GTS(0x7e55a8), &mUnknown6B8);
		break;
	case 7:  // 0x71a11b
		mUnknown978.SetCurve(GTS(0x7e557c), &mUnknown6B8);
		mUnknown9E8.SetCurve(GTS(0x7e5b94), &mUnknown6B8);
		mUnknownA58.SetCurve(GTS(0x7e5b70), &mUnknown6B8);
		mUnknownC88.SetCurve(GTS(0x7e5554), &mUnknown6B8);
		mUnknownCF8.SetCurve(GTS(0x7e552c), &mUnknown6B8);
		mUnknownD68.SetCurve(GTS(0x7e5500), &mUnknown6B8);
		mUnknownEB8.SetCurve(GTS(0x7e54d8), &mUnknown6B8);
		mUnknownE48.SetCurve(GTS(0x7e54b0), &mUnknown6B8);
		mUnknownDD8.SetCurve(GTS(0x7e5478), &mUnknown6B8);
		mUnknown6B8.SetCurve(GTS(0x7e5a64));
		mUnknown10F0.SetCurve(GTS(0x7e5a18));
		mUnknown1160.SetCurve(GTS(0x7e59d8));
		break;
	case 8:  // 0x71a415
		mUnknown978.SetCurve(GTS(0x7e544c), &mUnknown6B8);
		mUnknown9E8.SetCurve(GTS(0x7e5414), &mUnknown6B8);
		mUnknownA58.SetCurve(GTS(0x7e5c74), &mUnknown6B8);
		mUnknownC88.SetCurve(GTS(0x7e56d8), &mUnknown6B8);
		mUnknownCF8.SetCurve(GTS(0x7e5834), &mUnknown6B8);
		mUnknownD68.SetCurve(GTS(0x7e53ec), &mUnknown6B8);
		mUnknownEB8.SetCurve(GTS(0x7e53c4), &mUnknown6B8);
		mUnknownE48.SetCurve(GTS(0x7e53a4), &mUnknown6B8);
		mUnknownDD8.SetCurve(GTS(0x7e5370), &mUnknown6B8);
		break;
	case 9:  // 0x71a658
		mUnknown978.SetCurve(GTS(0x7e5350), &mUnknown6B8);
		mUnknown9E8.SetCurve(GTS(0x7e5310), &mUnknown6B8);
		mUnknownA58.SetCurve(GTS(0x7e5c74), &mUnknown6B8);
		mUnknownC88.SetCurve(GTS(0x7e52e0), &mUnknown6B8);
		mUnknownCF8.SetCurve(GTS(0x7e5834), &mUnknown6B8);
		mUnknownD68.SetCurve(GTS(0x7e52b8), &mUnknown6B8);
		mUnknownEB8.SetCurve(GTS(0x7e5290), &mUnknown6B8);
		mUnknownE48.SetCurve(GTS(0x7e53a4), &mUnknown6B8);
		mUnknownDD8.SetCurve(GTS(0x7e526c), &mUnknown6B8);
		break;
	case 10:  // 0x71a89b
		mUnknown978.SetCurve(GTS(0x7e5240), &mUnknown6B8);
		mUnknown9E8.SetCurve(GTS(0x7e5208), &mUnknown6B8);
		mUnknownA58.SetCurve(GTS(0x7e5c74), &mUnknown6B8);
		mUnknownC88.SetCurve(GTS(0x7e51e4), &mUnknown6B8);
		mUnknownCF8.SetCurve(GTS(0x7e51c0), &mUnknown6B8);
		mUnknownD68.SetCurve(GTS(0x7e5190), &mUnknown6B8);
		mUnknownEB8.SetCurve(GTS(0x7e5168), &mUnknown6B8);
		mUnknownE48.SetCurve(GTS(0x7e5140), &mUnknown6B8);
		mUnknownDD8.SetCurve(GTS(0x7e5834), &mUnknown6B8);
		break;
	case 11:  // 0x71aade
		mUnknown978.SetCurve(GTS(0x7e5118), &mUnknown6B8);
		mUnknown9E8.SetCurve(GTS(0x7e5208), &mUnknown6B8);
		mUnknownA58.SetCurve(GTS(0x7e5c74), &mUnknown6B8);
		mUnknownC88.SetCurve(GTS(0x7e50f0), &mUnknown6B8);
		mUnknownCF8.SetCurve(GTS(0x7e50c8), &mUnknown6B8);
		mUnknownD68.SetCurve(GTS(0x7e50a4), &mUnknown6B8);
		mUnknownEB8.SetCurve(GTS(0x7e5834), &mUnknown6B8);
		mUnknownE48.SetCurve(GTS(0x7e5834), &mUnknown6B8);
		mUnknownDD8.SetCurve(GTS(0x7e5080), &mUnknown6B8);
		break;
	case 12:  // 0x71ad21
		mUnknown978.SetCurve(GTS(0x7e5044), &mUnknown6B8);
		mUnknown9E8.SetCurve(GTS(0x7e5000), &mUnknown6B8);
		mUnknownA58.SetCurve(GTS(0x7e5b70), &mUnknown6B8);
		mUnknownC88.SetCurve(GTS(0x7e4fc8), &mUnknown6B8);
		mUnknownCF8.SetCurve(GTS(0x7e5834), &mUnknown6B8);
		mUnknownD68.SetCurve(GTS(0x7e4fa0), &mUnknown6B8);
		mUnknownEB8.SetCurve(GTS(0x7e4f6c), &mUnknown6B8);
		mUnknownE48.SetCurve(GTS(0x7e53a4), &mUnknown6B8);
		mUnknownDD8.SetCurve(GTS(0x7e4f30), &mUnknown6B8);
		mUnknown6B8.SetCurve(GTS(0x7e5a64));
		mUnknown10F0.SetCurve(GTS(0x7e5a18));
		mUnknown1160.SetCurve(GTS(0x7e59d8));
		break;
	case 13:  // 0x71b01b
		mUnknown978.SetCurve(GTS(0x7e4f08), &mUnknown6B8);
		mUnknown9E8.SetCurve(GTS(0x7e5b94), &mUnknown6B8);
		mUnknownA58.SetCurve(GTS(0x7e5b70), &mUnknown6B8);
		mUnknownC88.SetCurve(GTS(0x7e4ee0), &mUnknown6B8);
		mUnknownCF8.SetCurve(GTS(0x7e4eb8), &mUnknown6B8);
		mUnknownD68.SetCurve(GTS(0x7e4e84), &mUnknown6B8);
		mUnknownEB8.SetCurve(GTS(0x7e4e58), &mUnknown6B8);
		mUnknownE48.SetCurve(GTS(0x7e53a4), &mUnknown6B8);
		mUnknownDD8.SetCurve(GTS(0x7e4e24), &mUnknown6B8);
		mUnknown6B8.SetCurve(GTS(0x7e5a64));
		mUnknown10F0.SetCurve(GTS(0x7e5a18));
		mUnknown1160.SetCurve(GTS(0x7e59d8));
		break;
	case 14:  // 0x71b315
		mUnknown978.SetCurve(GTS(0x7e4df4), &mUnknown6B8);
		mUnknown9E8.SetCurve(GTS(0x7e4dbc), &mUnknown6B8);
		mUnknownA58.SetCurve(GTS(0x7e4d98), &mUnknown6B8);
		mUnknownC88.SetCurve(GTS(0x7e55f4), &mUnknown6B8);
		mUnknownCF8.SetCurve(GTS(0x80d3c8), &mUnknown6B8);
		mUnknownD68.SetCurve(GTS(0x7e55d0), &mUnknown6B8);
		mUnknownEB8.SetCurve(GTS(0x7e5c40), &mUnknown6B8);
		mUnknownE48.SetCurve(GTS(0x7e5c18), &mUnknown6B8);
		mUnknownDD8.SetCurve(GTS(0x7e55a8), &mUnknown6B8);
		break;
	}
}

// 0x70e83c: five curves to 1 and the board's brightness to 0, as a jump into the camera's path begins
void ResetJumpCurves(Hyperspace* theSpace)
{
	theSpace->mUnknown418.SetConstant(1.0);
	theSpace->mUnknown568.SetConstant(1.0);
	theSpace->mUnknown2C8.SetConstant(1.0);
	theSpace->mUnknown648.SetConstant(1.0);
	theSpace->mUnknown11D0.SetConstant(1.0);
	theSpace->mUnknown258.SetConstant(0.0);
}

void Hyperspace::SetTransition3D(int theTransition)
{
	// 0x7181b3 (the caller stored mTransition, +0x150); the gD3DInterface == 0 branches are SetTransition's own
	if (theTransition == 14) {
		// the trial's last jump: the curves of a fresh world, and the limit dialog (+0x152c) over the space
		mUnknown6B8.SetCurve(GTS(0x7e61fc));
		mUnknown10F0.SetCurve(GTS(0x7e61b0));
		mUnknown1160.SetCurve(GTS(0x7e6174));
		mUnknown418.SetCurve(GTS(0x7e6150));
		mUnknown11D0.SetCurve(GTS(0x7e612c));
		mUnknown1E8.SetCurve(GTS(0x7e6108));
		mUnknown258.SetCurve(GTS(0x7e60e4));
		mUnknown338.SetCurve(GTS(0x7e60bc));
		mUnknown3A8.SetCurve(GTS(0x7e6098));
		mUnknown2C8.SetCurve(GTS(0x7e6068));
		mUnknown488.SetCurve(GTS(0x7e6048));
		mUnknown568.SetCurve(GTS(0x7e6028));
		mUnknown5D8.SetCurve(GTS(0x7e6008));
		mUnknown4F8.SetCurve(GTS(0x7e5fd8));
		mUnknown648.SetCurve(GTS(0x7e5fb4));
		mUnknown1240.SetCurve(GTS(0x7e5f90));
		mUnknown1080.SetCurve(GTS(0x7e5f60), &mUnknown6B8);
		mUnknown978.SetCurve(GTS(0x7e5f2c), &mUnknown6B8);
		mUnknown9E8.SetCurve(GTS(0x7e5f08), &mUnknown6B8);
		mUnknownA58.SetCurve(GTS(0x7e5ee4), &mUnknown6B8);
		mUnknownC88.SetCurve(GTS(0x7e5ec0), &mUnknown6B8);
		mUnknownCF8.SetCurve(GTS(0x80d3c8), &mUnknown6B8);
		mUnknownD68.SetCurve(GTS(0x7e5e94), &mUnknown6B8);
		mUnknownEB8.SetCurve(GTS(0x7e5e60), &mUnknown6B8);
		mUnknownE48.SetCurve(GTS(0x7e5e38), &mUnknown6B8);
		mUnknownDD8.SetCurve(GTS(0x7e5e10), &mUnknown6B8);
		NotPorted("SetTransition(14)'s trial-limit dialog 0x73a458 (+0x152c): the copy is registered");
	} else if (theTransition == 8) {
		bool aMakeCrystals = false;  // [ebp + 0xb]
		switch ((mLevel + 1) % 20) {
		case 0:
		case 1:
			mWorld = 4;
			break;
		case 2:
			mWorld = 9;
			break;
		case 3:
			mWorld = 14;
			break;
		case 4:
		case 19:
			mWorld = 8;
			aMakeCrystals = true;
			break;
		case 5:
			mWorld = 1;
			MakeRingCrystals(0);
			break;
		case 6:
			mWorld = 11;
			break;
		case 7:
			mWorld = 10;
			break;
		case 8:
			mWorld = 6;
			aMakeCrystals = true;
			break;
		case 9:
			mWorld = 7;
			break;
		case 10:
		case 18:
			mWorld = 0;
			break;
		case 11:
			mWorld = 12;
			aMakeCrystals = true;
			break;
		case 12:
			mWorld = 13;
			break;
		case 13:
		case 17:
			mWorld = 1;
			MakeRingCrystals(4);
			break;
		case 14:
			mWorld = 5;
			break;
		case 15:
			mWorld = 3;
			break;
		case 16:
			mWorld = 2;
			break;
		}
		mUnknown6B8.SetCurve(GTS(0x7e61fc));
		mUnknown10F0.SetCurve(GTS(0x7e61b0));
		mUnknown1160.SetCurve(GTS(0x7e6174));
		mUnknown418.SetCurve(GTS(0x7e6150));
		mUnknown11D0.SetCurve(GTS(0x7e612c));
		mUnknown1E8.SetCurve(GTS(0x7e6108));
		mUnknown258.SetCurve(GTS(0x7e60e4));
		mUnknown338.SetCurve(GTS(0x7e60bc));
		mUnknown3A8.SetCurve(GTS(0x7e6098));
		mUnknown2C8.SetCurve(GTS(0x7e6068));
		mUnknown488.SetCurve(GTS(0x7e6048));
		mUnknown568.SetCurve(GTS(0x7e6028));
		mUnknown5D8.SetCurve(GTS(0x7e6008));
		mUnknown4F8.SetCurve(GTS(0x7e5fd8));
		mUnknown648.SetCurve(GTS(0x7e5fb4));
		mUnknown1240.SetCurve(GTS(0x7e5f90));
		mUnknown1080.SetCurve(GTS(0x7e5f60), &mUnknown6B8);
		SetWorldCurves(mWorld);
		if (aMakeCrystals)
			MakeCrystals();
	} else if (theTransition == 9) {
		mFlag155 = true;
		ChooseViz(true);
		mState = 3;
		gApp->PlaySoundEx(SOUND_TRANS_SPACENOISE, 0, 0.0f, 2.0f, true, true);
		gApp->PlaySample(SOUND_TRANS_ENGINEIGNITION);
		mSpaceSound = gApp->mSoundManager->GetSoundInstance(SOUND_TRANS_ENGINELOOP1);  // 0x75fa0b
		if (mSpaceSound != nullptr) {
			mSpaceSound->Play(true, false);
			mSpaceSound->SetVolume(2.0);
		}
		// the camera path's first point, taken while the curves still hold the last world's
		double aZ = mUnknownD68.GetOutVal();
		double aY = mUnknownCF8.GetOutVal();
		float aX = (float)mUnknownC88.GetOutVal();
		mUnknown153C = Vec3{aX, (float)aY, (float)aZ};
		ResetJumpCurves(this);
		mIs3D = true;
		mUnknown488.SetCurve(GTS(0x7e4d74));
		mUnknown4F8.SetCurve(GTS(0x7e4924));
		mUnknown10F0.SetCurve(GTS(0x7e4d48));
		mUnknown1160.SetCurve(GTS(0x7e4d20));
		mUnknown6B8.SetCurve(GTS(0x7e4cf4));
		mUnknown978.SetCurve(GTS(0x7e4cc8), &mUnknown6B8);
		mUnknown9E8.SetCurve(GTS(0x7e4c94), &mUnknown6B8);
		mUnknownA58.SetCurve(GTS(0x7e4c6c), &mUnknown6B8);
		mUnknownC88.SetCurve(GTS(0x7e56d8), &mUnknown6B8);
		mUnknownCF8.SetCurve(GTS(0x80d3c8), &mUnknown6B8);
		mUnknownD68.SetCurve(GTS(0x7e4c40), &mUnknown6B8);
		mUnknownEB8.SetCurve(GTS(0x7e4c1c), &mUnknown6B8);
		mUnknownE48.SetCurve(GTS(0x7e53a4), &mUnknown6B8);
		mUnknownDD8.SetCurve(GTS(0x7e4be8), &mUnknown6B8);
		mUnknown1080.SetCurve(GTS(0x7e4bb4), &mUnknown6B8);
		mFlag15B = false;
	} else if (theTransition == 10) {
		mFlag155 = true;
		ChooseViz(true);
		mState = 3;
		gApp->PlaySoundEx(SOUND_TRANS_SPACENOISE, 0, 0.0f, 2.0f, true, true);
		ResetJumpCurves(this);
		mIs3D = false;
		mUnknown488.SetCurve(GTS(0x7e4b08));
		mUnknown4F8.SetCurve(GTS(0x7e4924));
		mUnknown10F0.SetConstant(0.0);
		mUnknown1160.SetConstant(0.0);
		mUnknown6B8.SetCurve(GTS(0x7e4cf4));
		mUnknown978.SetCurve(GTS(0x7e4ae0), &mUnknown6B8);
		mUnknown9E8.SetCurve(GTS(0x7e4aac), &mUnknown6B8);
		mUnknownA58.SetCurve(GTS(0x7e5c74), &mUnknown6B8);
		mUnknownC88.SetCurve(GTS(0x7e56d8), &mUnknown6B8);
		mUnknownCF8.SetCurve(GTS(0x80d3c8), &mUnknown6B8);
		mUnknownD68.SetCurve(GTS(0x7e4c40), &mUnknown6B8);
		mUnknownDD8.SetConstant(0.0);
		mUnknownE48.SetConstant(0.0);
		mUnknownEB8.SetConstant(0.0);
		mFlag15B = false;
	} else if (theTransition == 12) {
		mUnknown258.SetConstant(1.0);
		mUnknown1E8.SetConstant(1.0);
		mUnknown2C8.SetConstant(0.0);
		mUnknown338.SetConstant(0.0);
		mUnknown3A8.SetConstant(0.0);
		mUnknownDD8.SetConstant(0.0);
		mUnknownE48.SetConstant(0.0);
		mUnknownEB8.SetConstant(0.0);
		mUnknownC88.SetConstant(0.0);
		mUnknownCF8.SetConstant(0.0);
		mUnknownD68.SetConstant(0.0);
		mIs3D = false;
		ChooseViz(true);
		mTransition = 12;
		mState = 11;
		mFlag159 = true;
		mLevel = 0;
		mUnknown12B0.SetCurve(GTS(0x7e4a7c));
		mFlash.SetCurve(GTS(0x7e4a48));
		mUnknown12B0.SetCurve(GTS(0x7e4a1c));
		gBoard->SetUnknown3924(0.0f);
		mFlag155 = true;
		gApp->mBoard->SetVisible(false);
		gApp->mVizManager->SetVisible(false);
	}
	Update();
}


///////////////////////////////////////////////////////////////////////////////
// Update (0x715c21) with gD3DInterface set

void Hyperspace::Update3D()
{
	// 0x716336 and on: the common code of every transition, which the 2D chain (0x715d68) leaves to them only in 3D
	if (!mUnknown1E8.IncInVal() && mState == 1) {
		// the jump's first step is over: the VizManager's picture goes into +0x160 and space takes the screen
		Graphics aG(mUnknown160);
		gApp->mVizManager->Draw(&aG);
		gApp->mVizManager->SetVisible(false);
		ChooseViz(false);
		mFlag155 = true;
		mIs3D = true;
		gApp->mBoard->mDisabled = false;  // +0x55
		mState++;
		gApp->PlaySoundEx(SOUND_TRANS_SPACENOISE, 0, 0.0f, 2.0f, true, true);
	}
	gApp->mVizManager->mSpeed = mUnknown1E8.GetOutVal();
	NotPorted("Hyperspace::Update: the VizManager's rectangle +0x1bc to +0x1c8 (-1, -1, 1, 1; its 3D render target)");
	if (gApp->mBoard != nullptr && !gApp->mBoard->mUnknown37C4) {
		mUnknown258.IncInVal();
		gApp->mBoard->SetUnknown3924((float)mUnknown258.GetOutVal());
		if (mTransition == 11)
			gApp->mBoard->ApplyUnknown3924();
	}
	mUnknown900.IncInVal();
	mUnknown10F0.IncInVal();
	mUnknown2C8.IncInVal();
	mUnknown338.IncInVal();
	bool aWentTo5 = false;  // [ebp - 0x19]
	if (1 < mState && mState < 10) {
		mUnknown488.IncInVal();
		if (mTransition == 8 || mTransition == 14 || mTransition == 9 || mTransition == 10) {
			mUnknown418.IncInVal();
			mUnknown11D0.IncInVal();
		}
		mUnknown568.IncInVal();
		mUnknown648.IncInVal();
		mUnknown1240.IncInVal();
		mUnknown178.IncInVal();
		if (mUnknown5D8.CheckUpdatesFromEndThreshold(0x7d))
			gApp->PlaySample(SOUND_TRANS_PREIGNITION);
		if (!mUnknown5D8.IncInVal() && mState == 2) {
			mState = 3;
			gApp->PlaySample(SOUND_TRANS_ENGINEIGNITION);
			mSpaceSound = gApp->mSoundManager->GetSoundInstance(SOUND_TRANS_ENGINELOOP1);  // 0x75fa0b
			if (mSpaceSound != nullptr) {
				mSpaceSound->Play(true, false);
				mSpaceSound->SetVolume(2.0);
			}
		}
		if (!mUnknown4F8.IncInVal()) {
			if (mState == 7) {
				NotPorted("Hyperspace::Update state 7: the trial-limit dialog (+0x152c) taken off, 0x716632");
				if (gBoard->mGameMode == 1)
					gBoard->EndGame(false);
				else
					gBoard->mUnknown1FF5 = true;
			} else if (mState == 8) {
				NotPorted("Hyperspace::Update state 8: the trial-limit dialog (+0x152c) taken off, 0x716681");
				mTransition = 8;
				mState = 9;
				mUnknown4F8.SetCurve(GTS(0x7d8630));
			} else if (mState == 9) {
				if (gApp->mMainMenu->mChallengeMenu != nullptr)
					gApp->mMainMenu->mChallengeMenu->mVisible = false;
				gApp->mVizManager->SetVisible(true);
				mState = 5;
				mUnknown1E8.SetCurve(GTS(0x7e496c));
				aWentTo5 = true;
			}
		}
	}
	if (2 < mState && mState < 10) {
		if (mTransition != 13 && !mUnknown6B8.IncInVal()) {
			if (mTransition == 14) {
				NotPorted("Hyperspace::Update: the replay pod's and trial dialog's end of a transition-14 jump, 0x716811");
			} else {
				if (gApp->mMainMenu->mChallengeMenu != nullptr)
					gApp->mMainMenu->mChallengeMenu->mVisible = false;
				gApp->mVizManager->SetVisible(true);
				mState++;
				mUnknown1E8.SetCurve(GTS(0x7e496c));
			}
		}
		if (mUnknown6B8.CheckUpdatesFromEndThreshold(100) && mTransition == 14)
			gApp->PlaySample(SOUND_TRIALLIMIT);
		if (mUnknown6B8.CheckUpdatesFromEndThreshold(0x17c) && mFlag15B) {
			mFlag15B = false;
			mUnknown488.SetCurve(GTS(0x7e4948));
			mUnknown4F8.SetCurve(GTS(0x7e4924));
			NotPorted("Hyperspace::Update: VizManager 0x7581e5 (the visualizer its thread made, +0x188)");
			Graphics aG(mUnknown160);
			gApp->mVizManager->Draw(&aG);
		}
		if (mUnknown6B8.CheckUpdatesFromEndThreshold(0xfa) && mTransition != 11) {
			mUnknown648.SetCurve(GTS(0x7e48fc));
			mUnknown568.SetCurve(GTS(0x7e48dc));
			mUnknown11D0.SetCurve(GTS(0x7e48bc));
			mUnknown418.SetCurve(GTS(0x7e48bc));
		}
		if (mUnknown6B8.CheckUpdatesFromEndThreshold(0x8c))
			gApp->PlaySample(SOUND_TRANS_ARRIVAL);
		if (mState == 6 && gApp->mRegistered) {  // the copy is registered
			NotPorted("Hyperspace::Update state 6 with the registered copy 0x716a6a (the dialog hidden, state 8)");
			mState = 8;
			mUnknownA58.SetCurve(GTS(0x7e4894));
			mUnknown4F8.SetCurve(GTS(0x7df2d0));
		}
		if (mUnknown6B8.CheckUpdatesFromEndThreshold(0x3c)) {
			for (GemSoul* aSoul : gBoard->mUnknown26B0)
				aSoul->mUnknown8F4 = false;
			mFlag158 = false;
		}
		if (mUnknown6B8.CheckUpdatesFromEndThreshold(0x23) || aWentTo5) {
			if (mTransition == 14 && !aWentTo5) {
				mState = 6;
			} else {
				mIs3D = false;
				if (mState != 5)
					mState = 4;
				mUnknown258.SetCurve(GTS(0x7e4874));
				if (mTransition == 11) {
					mUnknown338.SetCurve(GTS(0x7e4840));
					mUnknown3A8.SetCurve(GTS(0x7e481c));
					mUnknown2C8.SetCurve(GTS(0x7e47e8));
				} else {
					mUnknown338.SetCurve(GTS(0x7e47b4));
					mUnknown3A8.SetCurve(GTS(0x7e478c));
					mUnknown2C8.SetCurve(GTS(0x7e4758));
				}
				if (mTransition != 10 && mTransition != 11) {
					gApp->mBoard->NewLevel();
					gApp->mBoard->ClearBoard();
					gApp->mBoard->FillInBlanks(false);
				}
			}
		}
	}
	if (mSpaceSound != nullptr && (mState == 4 || mState == 6)) {
		// the space noise fades out by 0.2 an update and goes
		double aVolume = mSpaceSound->GetVolume() - (double)0.20000000298023224f;
		if (0.0 < aVolume) {
			mSpaceSound->SetVolume(aVolume);
		} else {
			mSpaceSound->Release();
			mSpaceSound = nullptr;
		}
	}
	if (mState == 5) {
		if (mUnknown1E8.CheckInThreshold(0.3))  // 0x81d278
			gApp->mBoard->RetargetTravellers();
		if (mUnknown1E8.HasBeenTriggered()) {
			gApp->mBoard->mDisabled = true;  // +0x55 = 1
			mState = 10;
		}
	}
	UpdateSpace();
}


///////////////////////////////////////////////////////////////////////////////
// The world's motion (0x716e31 to 0x717dbe), the ship (0x715480) and the saucer (0x7158f8)

namespace {

Vec3 NormalizedV(const Vec3& v)  // 0x4c0840
{
	float aLength = (float)std::sqrt((double)(v.y * v.y + v.x * v.x) + (double)(v.z * v.z));
	if (aLength == 0.0f)
		return v;
	float anInverse = (float)(1.0 / (double)aLength);
	return Vec3{v.x * anInverse, v.y * anInverse, v.z * anInverse};
}

Vec3 CrossV(const Vec3& a, const Vec3& b)  // 0x4c06d0: a (this) crossed with b
{
	return Vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

float DotV(const Vec3& a, const Vec3& b)  // 0x4c06a0
{
	return (float)((double)(b.z * a.z) + (double)(b.x * a.x) + (double)(b.y * a.y));
}

// 0x70ffbd: theMatrix turned to look along theDirection, with theUp above it (left alone when they are all but
// parallel): its rows are the right, the up and the direction
void LookAlong(Mat3& theMatrix, const Vec3& theDirection, const Vec3& theUp)
{
	Vec3 aDirection = NormalizedV(theDirection);
	float aDot = DotV(theUp, aDirection);
	if (std::fabs(aDot) <= 0.999) {  // 0x7d7a60
		Mat3 aTurn;
		aTurn.m[6] = aDirection.x;
		aTurn.m[7] = aDirection.y;
		aTurn.m[8] = aDirection.z;
		Vec3 aRight = NormalizedV(CrossV(theUp, aDirection));
		aTurn.m[0] = aRight.x;
		aTurn.m[1] = aRight.y;
		aTurn.m[2] = aRight.z;
		Vec3 aTop = NormalizedV(CrossV(aDirection, aRight));
		aTurn.m[3] = aTop.x;
		aTurn.m[4] = aTop.y;
		aTurn.m[5] = aTop.z;
		theMatrix = theMatrix * aTurn;
	}
}

// 0x44f560: a point through a 4 by 4 matrix, its translation row added
Vec3 Mat4TransformPoint(const GPUMatrix& theMatrix, const Vec3& thePoint)
{
	const float* p = &theMatrix.m[0][0];
	Vec3 aResult;
	aResult.x = p[8] * thePoint.z + thePoint.x * p[0] + p[4] * thePoint.y + p[12];
	aResult.y = p[9] * thePoint.z + p[5] * thePoint.y + p[1] * thePoint.x + p[13];
	aResult.z = p[10] * thePoint.z + p[6] * thePoint.y + p[2] * thePoint.x + p[14];
	return aResult;
}

}  // namespace

void Hyperspace::UpdateWorld3D()
{
	// the crystals' spin (0x716e31)
	unsigned int i = 0;
	if (!mCrystals.empty()) {
		do {
			Mat3 aRot = mCrystals[i].mTransform.mRot;
			aRot.RotateX(mCrystals[i].mUnknown40);
			aRot.RotateY(mCrystals[i].mUnknown44);
			mCrystals[i].mTransform.mRot = aRot;
			i++;
		} while (i < mCrystals.size());
	}
	i = 0;
	if (!mRingCrystals.empty()) {
		do {
			RingCrystal& aCrystal = mRingCrystals[i];
			Mat3 aRot = aCrystal.mTransform.mRot;
			aRot.RotateX(aCrystal.mUnknown88.x);
			aRot.RotateY(aCrystal.mUnknown88.y);
			aCrystal.mTransform.mRot = aRot;
			Mat3 aSpin = aCrystal.mUnknown40;
			aSpin.RotateY(aCrystal.mUnknown88.z);
			aCrystal.mUnknown40 = aSpin;
			i++;
		} while (i < mRingCrystals.size());
	}
	// the world's camera (0x716f83): a camera at the origin looking down -z (0x674035 builds its rows by the same
	// cross products as 0x70ffbd), the sidebar's offset behind it
	Transform3D aLook;
	LookAlong(aLook.mRot, Vec3{0.0f, 0.0f, -1.0f}, Vec3{0.0f, 1.0f, 0.0f});
	GPUMatrix aLookMatrix = WorldMatrixOf(aLook);
	float aBehind = (float)(mUnknown418.GetOutVal() * 0.5 + 2.7300000190734863);
	Vec3 anOffset = Mat4TransformPoint(aLookMatrix, Vec3{0.0f, 0.0f, aBehind});
	float aNext = (float)(mUnknown6B8.GetInVal() + mUnknown6B8.mIncRate);
	Vec3 aTo;
	aTo.x = (float)mUnknownC88.GetOutVal((double)aNext);
	aTo.y = (float)mUnknownCF8.GetOutVal((double)aNext);
	aTo.z = (float)mUnknownD68.GetOutVal((double)aNext);
	double aPathZ = mUnknownD68.GetOutVal();
	double aPathY = mUnknownCF8.GetOutVal();
	Vec3 aFrom;
	aFrom.x = (float)mUnknownC88.GetOutVal();
	aFrom.y = (float)aPathY;
	aFrom.z = (float)aPathZ;
	mUnknown11D0.GetOutVal();  // (0x7170e2: read, and not used)
	Vec3 aPlace;
	aPlace.x = aFrom.x + anOffset.x;
	aPlace.y = aFrom.y + anOffset.y;
	aPlace.z = aFrom.z + anOffset.z;
	mUnknown1390[0] = mUnknown153C.x + aPlace.x;
	mUnknown1390[1] = mUnknown153C.y + aPlace.y;
	mUnknown1390[2] = mUnknown153C.z + aPlace.z;
	float anAspect = (float)((double)mWidth / (double)mHeight);
	float aFovTimesAspect = (float)((double)anAspect * 60.0);  // 0x7d7d60
	mCamera139C.SetFov(aFovTimesAspect, anAspect, 0.20000000298023224f, 500.0f);
	Transform3D aCamera;
	Translate(aCamera, mUnknown1390[0], mUnknown1390[1], mUnknown1390[2]);
	aCamera.mRot.RotateZ((float)mUnknownE48.GetOutVal());
	aCamera.mRot.RotateY((float)mUnknownEB8.GetOutVal());
	aCamera.mRot.RotateX((float)mUnknownDD8.GetOutVal());
	mCamera139C.mTransform = aCamera;
	mCamera13F4.SetFov(aFovTimesAspect, anAspect, 1.0f, 10000.0f);
	aCamera.mRot = aCamera.mRot * mUnknown1548;  // 0x70ff99
	mCamera13F4.mTransform = aCamera;
	(void)aTo;
}

void Hyperspace::PlaceShip()
{
	// 0x715480: the ship (the sidebar's place, +0x14c0) on the camera path's curves 978, 9e8 and a58, slid by 1240
	// and 178, turned along the path
	Transform3D aShip;
	float aZ = (float)mUnknownA58.GetOutVal();  // the three reads come z, y, x
	float aY = (float)mUnknown9E8.GetOutVal();
	float aX = (float)mUnknown978.GetOutVal();
	Vec3 aFrom{mUnknown153C.x + aX, mUnknown153C.y + aY, mUnknown153C.z + aZ};
	float aNext = (float)(mUnknown6B8.GetInVal() + mUnknown6B8.mIncRate);
	float aNextX = (float)mUnknown978.GetOutVal((double)aNext);
	float aNextY = (float)mUnknown9E8.GetOutVal((double)aNext);
	float aNextZ = (float)mUnknownA58.GetOutVal((double)aNext);
	Vec3 aTo{mUnknown153C.x + aNextX, mUnknown153C.y + aNextY, mUnknown153C.z + aNextZ};
	mUnknown1520 = Vec3{aTo.x - aFrom.x, aTo.y - aFrom.y, aTo.z - aFrom.z};
	Vec3 aHeading = NormalizedV(mUnknown1520);
	Vec3 aBack{-aHeading.x, -aHeading.y, -aHeading.z};
	Widget* aTube = gApp->mBoard->mGoodTube;  // +0x2194
	float anAside = (float)(((double)(float)aTube->mWidth * 0.525 + (double)(float)aTube->mX - (double)(float)(mWidth / 2)) /
		(double)(gApp->mScale * 694.4375f));
	Mat3 aTurn;
	aTurn.RotateZ((float)((double)mUnknown1520.x * -15.0));
	LookAlong(aTurn, aBack, Vec3{0.0f, 1.0f, 0.0f});
	mUnknown14FC = aTurn;
	aShip.mRot = aTurn;
	Transform3D aNear;
	ScaleBy(aNear, 0.052799999713897705f, 0.052799999713897705f, 0.052799999713897705f);
	aNear.mRot.RotateX(-1.5707963705062866f);
	aNear.mRot.RotateY(3.1415927410125732f);
	float aSlide = (float)mUnknown1240.GetOutVal();
	float aSlide2 = (float)mUnknown1240.GetOutVal();
	float aSlide3 = (float)mUnknown1240.GetOutVal();
	float aNearY = (float)(0.035000000149011612 - (double)aSlide2 * 0.30000001192092896);
	float aNearZ = (float)(1.2000000476837158 - (double)aSlide * 0.10000000149011612);
	float aNearX = (float)(((1.0049999952316284 - (double)aSlide3) + mUnknown178.GetOutVal() * 3.0) * (double)anAside);
	Translate(aNear, aNearX, aNearY, aNearZ);
	Translate(aNear, aFrom.x, aFrom.y, aFrom.z);
	Transform3D aFar;
	ScaleBy(aFar, 0.053500000387430191f, 0.053500000387430191f, 0.053500000387430191f);
	aFar.mRot.RotateY(3.1415927410125732f);
	float aFarSlide = (float)mUnknown1240.GetOutVal();
	float aFarSlide2 = (float)mUnknown1240.GetOutVal();
	double aFarSlide3 = mUnknown1240.GetOutVal();
	float aFarY = (float)(0.052999999374151230 - (double)aFarSlide2 * 0.30000001192092896);
	float aFarZ = (float)(1.2000000476837158 - (double)aFarSlide * 0.10000000149011612);
	float aFarX = (float)(((1.0 - aFarSlide3) + mUnknown178.GetOutVal() * 3.0) * (double)anAside);
	Translate(aFar, aFarX, aFarY, aFarZ);
	aFar = Compose(aFar, aShip);
	Translate(aFar, aFrom.x, aFrom.y, aFrom.z);
	mUnknown14C0 = aNear;
	mUnknown14C0.mRot = SlerpMat3(mUnknown14C0.mRot, aFar.mRot, (float)mUnknown648.GetOutVal(), false);
}

void Hyperspace::PlaceSaucer()
{
	// 0x7158f8: the saucer (+0xac8) on its own path (b08, b78 and be8), turned along it like the ship
	Transform3D aShip;
	float aZ = (float)mUnknownBE8.GetOutVal();
	float aY = (float)mUnknownB78.GetOutVal();
	float aX = (float)mUnknownB08.GetOutVal();
	Vec3 aFrom{aX, aY, aZ};
	float aNext = (float)(mUnknown6B8.GetInVal() + mUnknown6B8.mIncRate);
	Vec3 aTo{(float)mUnknownB08.GetOutVal((double)aNext), (float)mUnknownB78.GetOutVal((double)aNext),
		(float)mUnknownBE8.GetOutVal((double)aNext)};
	Vec3 aDelta{aTo.x - aFrom.x, aTo.y - aFrom.y, aTo.z - aFrom.z};
	Vec3 aHeading = NormalizedV(aDelta);
	Vec3 aBack{-aHeading.x, -aHeading.y, -aHeading.z};
	Mat3 aTurn;
	aTurn.RotateZ((float)((double)aDelta.x * -15.0));
	LookAlong(aTurn, aBack, Vec3{0.0f, 1.0f, 0.0f});
	mUnknown14FC = aTurn;
	aShip.mRot = aTurn;
	Transform3D aPlace;
	ScaleBy(aPlace, 0.053500000387430191f, 0.053500000387430191f, 0.053500000387430191f);
	Translate(aPlace, aFrom.x, aFrom.y, aFrom.z);
	Transform3D aSpin;
	ScaleBy(aSpin, 0.053500000387430191f, 0.053500000387430191f, 0.053500000387430191f);
	aSpin.mRot.RotateY(3.1415927410125732f);
	aSpin = Compose(aSpin, aShip);
	Translate(aSpin, aFrom.x, aFrom.y, aFrom.z);
	mSaucer = aPlace;
	mSaucer.mRot = SlerpMat3(mSaucer.mRot, aSpin.mRot, (float)mUnknown648.GetOutVal(), false);
}
