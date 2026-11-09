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
// 0x68f4c3: the transform's constructor from a place, a turn and a scale, inline here
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
	float mLightDirections[6] = {0, 0, 0, 0, 0, 0};  // +0x0c: the two lights' directions (0x7100b2's, which this class does not use)
	float mLightIntensity = 0;                     // +0x24: the lights' strength
	float mGlowFirstPart = 0;                      // +0x28: the glow's first part (the shader's constant 0, its second float)
	float mGlowSecondPart = 0;                     // +0x2c: and its second part (the third float)
	float mGlow = 0;                               // +0x30: how bright the engine burns

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
			anEngine->mLightDirections[0] = 0.0f;
			anEngine->mLightDirections[1] = aDirection[0];
			anEngine->mLightDirections[2] = aDirection[1];
			anEngine->mLightDirections[3] = aSecond[0];
			anEngine->mLightDirections[4] = aSecond[1];
			anEngine->mLightDirections[5] = aSecond[2];
			anEngine->mLightIntensity = aIntensity;
			anEngine->mGlowFirstPart = mConstants0[1];
			anEngine->mGlowSecondPart = mConstants0[2];
			anEngine->mGlow = (float)mHyperspace->mEngineGlow.GetOutVal();
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
	if (((mDrawsOwnBackground && !gApp->mVizManager->mVisible) || mState == 0) || mTransition == 9 || mTransition == 10) {
		float aFovA = (float)(mFirstBackdropFov.GetOutVal() * 4.0 * mWarmUp + 75.0);
		float aFovB = (float)(mSecondBackdropFov.GetOutVal() * 4.0 * mWarmUp + 75.0);
		float aFovC = (float)(mThirdBackdropFov.GetOutVal() * 4.0 * mWarmUp + 75.0);
		double anAspect = (double)mWidth / (double)mHeight;
		// the three backdrops, each through the camera at +0x13f4 with its own field of view
		mBackdropCamera.SetFov((float)(anAspect * (double)aFovB), (float)anAspect, 1.0f, 10000.0f);
		WorldGraphicsListener aListener(&mBackdropCamera, true);
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
		mBackdropCamera.SetFov((float)(anAspect * (double)aFovA), (float)anAspect, 1.0f, 10000.0f);
		aListener.Apply();
		gD3DInterface->SetRenderState(GPURS_DESTBLEND, 2);
		gD3DInterface->DrawObject(gApp->mSpaceObject, aMatrix, Color::White, false);
		aListener.Remove();
		mBackdropCamera.SetFov((float)(anAspect * (double)aFovC), (float)anAspect, 1.0f, 10000.0f);
		aListener.Apply();
		gD3DInterface->DrawObject(gApp->mStarsObject, aMatrix, Color::White, false);
		gD3DInterface->SetRenderState(GPURS_DESTBLEND, 6);
		aListener.Remove();
		mBackdropCamera.SetFov((float)(anAspect * 60.0), (float)anAspect, 1.0f, 10000.0f);
		if (mIs3D) {
			DrawCrystals();
			if (mWorld == 0)
				DrawSaucer();
			else if (mWorld == 1)
				DrawPlanet(gApp->mPlanetObjects[mPlanetObject]);
		}
		if (((mState != 0 && mState != 5) && (mTransition != 0xb && (mState != 0xc && mState != 0xb))) &&
			(mTunnelCountdown.GetOutVal() < 1.0 || 2 < mState))
			DrawTunnel(g);
		if (mIs3D && !mIsNewGameJump) {
			Draw3DSidebar();
			DrawCone();
			DrawParticles();
		}
		if (mDrawsTravellers)
			DrawTravellers(g);
	}
	if ((mState == 0xc || mState == 0xb) && mDrawsFinalFade)
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
	WorldGraphicsListener aListener(&mWorldCamera, !mIs3D);
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
	WorldGraphicsListener aListener(&mWorldCamera, !mIs3D);
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
	WorldGraphicsListener aListener(&mWorldCamera, !mIs3D);
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
				Mat3 aTurn = mRingCrystals[i].mOwnTurn;
				aTurn = aTurn * mRingCrystals[i].mPlanetTurn;
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
		aCrystal.mSpinX = (float)(((double)Rand(0.019999999552965164f) + 0.009999999776482582) * (double)aSign);
		aSign = Rand(1) * 2 - 1;
		aCrystal.mSpinY = (float)(((double)Rand(0.019999999552965164f) + 0.009999999776482582) * (double)aSign);
		ScaleBy(aCrystal.mTransform, 0.20000000298023224f, 0.20000000298023224f, 0.20000000298023224f);
		aCrystal.mTransform.mRot.RotateX(Rand(6.2831854820251465f));
		aCrystal.mTransform.mRot.RotateY(Rand(6.2831854820251465f));
		aCrystal.mTransform.mRot.RotateZ(Rand(6.2831854820251465f));
		float aSine = (float)std::sin((double)anAngle);
		float aCosine = (float)std::cos((double)anAngle);
		float aZ = (float)mOrbitZ.GetOutVal((double)aT);
		float aY = (float)((double)aSine * (double)aDistance + mOrbitY.GetOutVal((double)aT));
		float aX = (float)((double)aCosine * (double)aDistance + mOrbitX.GetOutVal((double)aT));
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
			aCrystal.mSpin.x = (float)(((double)Rand(0.019999999552965164f) + 0.009999999776482582) * (double)aSign);
			aSign = Rand(2) * 2 - 1;
			aCrystal.mSpin.y = (float)(((double)Rand(0.019999999552965164f) + 0.009999999776482582) * (double)aSign);
			aCrystal.mSpin.z = (float)((double)Rand(0.019999999552965164f) + 0.009999999776482582);
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
	aSidebarListener.mTubeTexture.m[2][0] = (float)(mJumpClock.GetOutVal() * 0.5);
	float aConstants[4];
	aConstants[0] = (float)mSidebarGlance.GetOutVal();
	aConstants[1] = (float)mTubeBlendBase.GetOutVal();
	double aValue = mTubeBlendBase.GetOutVal() + (1.0 - mTubeAlpha.GetOutVal());
	aConstants[2] = (float)(aValue > 0.0 ? (aValue < 1.0 ? aValue : 1.0) : 0.0);
	aConstants[3] = (float)((double)(mUpdateCnt % 0x15e) / 350.0);
	std::memcpy(aSidebarListener.mConstants0, aConstants, 0x10);
	Transform3D aSidebarCamera = mSidebarCamera.mTransform;
	float aShift[3];
	aShift[0] = aSidebarCamera.mPos.x - mShipPlace.mPos.x;
	aShift[1] = aSidebarCamera.mPos.y - mShipPlace.mPos.y;
	aShift[2] = aSidebarCamera.mPos.z - mShipPlace.mPos.z;
	float aSidebarShift[3] = {-aShift[0], aShift[1], aShift[2]};
	std::memcpy(aSidebarListener.mConstants1, aSidebarShift, 0xc);
	D3DObjectListener* anOldListener = gApp->mSidebarObject->mListener;
	gApp->mSidebarObject->mListener = &aSidebarListener;
	double anAspect = (double)mWidth / (double)mHeight;
	float aFov = (float)((double)(float)mSidebarGlance.GetOutVal() * 45.0 + 15.0);
	float aFovTimesAspect = (float)((double)aFov * (double)(float)anAspect);
	float aNear = (float)((double)(float)mSidebarGlance.GetOutVal() * -0.7999999970197678 + 1.0);
	float aFar = (float)((double)(float)mSidebarGlance.GetOutVal() * -1500.0 + 2000.0);
	float aCenterY = (float)mSidebarGlance.GetOutVal();
	aCenterY = (float)(0.008999999612569809 + -0.008999999612569809 * (double)aCenterY);
	mSidebarCamera.SetFrustum(aFovTimesAspect, (float)anAspect, 0.0f, aCenterY, aNear, aFar);
	Transform3D aTransform;
	float aBack = (float)(((1.0 - mSidebarGlance.GetOutVal()) / (double)aFov) * 73.5);
	float aGrow = (float)mSidebarGlance.GetOutVal();
	float aZ = (float)((double)mWorldCameraPos[2] + (double)aBack);
	float aX = (float)((double)(float)(0.017000000923871994 * (double)aGrow - 0.017000000923871994) + (double)mWorldCameraPos[0]);
	Translate(aTransform, aX, mWorldCameraPos[1], aZ);
	aTransform.mRot.RotateZ((float)mSidebarTurnZ.GetOutVal());
	aTransform.mRot.RotateY((float)mSidebarTurnY.GetOutVal());
	aTransform.mRot.RotateX((float)mSidebarTurnX.GetOutVal());
	mSidebarCamera.mTransform = aTransform;
	OffCenterListener aListener(&mSidebarCamera, !mIs3D);
	aListener.SetCulling(true);
	aListener.SetDepth(true, true);
	aListener.SetLighting(true);
	SetPlainMaterial();
	D3DLightColors aColors = LightColors(Color::White, Color::White, Color(0, 0, 0));
	Vec3 aDirection{0.0f, 0.0f, -1.0f};
	gD3DInterface->SetDirectionalLight(0, aDirection, aColors);
	gD3DInterface->LightEnable(0, true);
	aListener.Apply();
	Transform3D aPlace = mShipPlace;
	Mat3 aTurn;
	Vec3 anAxis{0.0f, 1.0f, 0.0f};
	float aGlance = (float)mSidebarGlance.GetOutVal();
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
	OffCenterListener aListener(&mSidebarCamera, !mIs3D);
	aListener.SetCulling(false);
	aListener.SetDepth(true, false);
	aListener.Apply();
	int anAlpha = Ftol(mConeAlpha.GetOutVal() * 255.0);
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
	Transform3D aPlace = mShipPlace;
	Mat3 aTurn;
	Vec3 anAxis{0.0f, 1.0f, 0.0f};
	float aGlance = (float)mSidebarGlance.GetOutVal();
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
	if (!gBoard->mLevelSouls.empty()) {
		do {
			GemSoul* aSoul = gBoard->mLevelSouls[i];
			Vec3 aScreen = mSidebarCamera.ProjectWorld(Vec3{aSoul->mSpacePos[0], aSoul->mSpacePos[1], aSoul->mSpacePos[2]});
			float aSize = (float)(0.0018 / (double)aScreen.z);
			double aX = aSoul->mSpaceFade.GetOutVal();
			double aX2 = aSoul->mSpaceFade.GetOutVal();
			float aPlaceX = (float)((double)mWidth * (double)aScreen.x * aX2 + (double)aSoul->mDrawX * (1.0 - aX));
			double aY = aSoul->mSpaceFade.GetOutVal();
			double aY2 = aSoul->mSpaceFade.GetOutVal();
			float aPlaceY = (float)((double)mHeight * (double)aScreen.y * aY2 + (double)aSoul->mDrawY * (1.0 - aY));
			double aScale1 = aSoul->mSpaceFade.GetOutVal();
			double aScale2 = aSoul->mSpaceFade.GetOutVal();
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
		} while (i < gBoard->mLevelSouls.size());
	}
}

///////////////////////////////////////////////////////////////////////////////
// SetTransition (0x7181b3) with gD3DInterface set


// 0x718d8e: the curves each world sets (the jump table at 0x71c153, one block of SetCurve calls per world; GTS takes the
// address of the exe's curve string; the path curves follow the jump's clock, mJumpClock)
void Hyperspace::SetWorldCurves(int theWorld)
{
	switch (theWorld) {
	case 0:  // 0x718d95
		gSaucerCameo = (gSaucerCameo + 1) % 5;  // gApp+0xf98 (0x718da2): the picture the saucer's "Image" mesh wears
		mSaucerPathX.SetCurve(GTS(0x7e5d58), &mJumpClock);
		mSaucerPathY.SetCurve(GTS(0x7e5d2c), &mJumpClock);
		mSaucerPathZ.SetCurve(GTS(0x7e5d04), &mJumpClock);
		mOrbitX.SetCurve(GTS(0x7e5cd0), &mJumpClock);
		mOrbitY.SetCurve(GTS(0x7e5c98), &mJumpClock);
		mOrbitZ.SetCurve(GTS(0x7e5c74), &mJumpClock);
		mCameraPathX.SetCurve(GTS(0x7e5ec0), &mJumpClock);
		mCameraPathY.SetCurve(GTS(0x80d3c8), &mJumpClock);
		mCameraPathZ.SetCurve(GTS(0x7e5e94), &mJumpClock);
		mSidebarTurnY.SetCurve(GTS(0x7e5c40), &mJumpClock);
		mSidebarTurnZ.SetCurve(GTS(0x7e5c18), &mJumpClock);
		mSidebarTurnX.SetCurve(GTS(0x7e5bec), &mJumpClock);
		break;
	case 1:  // 0x7190ad
		mOrbitX.SetCurve(GTS(0x7e5bc0), &mJumpClock);
		mOrbitY.SetCurve(GTS(0x7e5b94), &mJumpClock);
		mOrbitZ.SetCurve(GTS(0x7e5b70), &mJumpClock);
		mCameraPathX.SetCurve(GTS(0x7e5b44), &mJumpClock);
		mCameraPathY.SetCurve(GTS(0x7e5b18), &mJumpClock);
		mCameraPathZ.SetCurve(GTS(0x7e5af0), &mJumpClock);
		mSidebarTurnY.SetCurve(GTS(0x7e5ac0), &mJumpClock);
		mSidebarTurnZ.SetCurve(GTS(0x7e5c18), &mJumpClock);
		mSidebarTurnX.SetCurve(GTS(0x7e5a94), &mJumpClock);
		mJumpClock.SetCurve(GTS(0x7e5a64));
		mEngineFire.SetCurve(GTS(0x7e5a18));
		mEngineGlow.SetCurve(GTS(0x7e59d8), &mEngineFire);  // linked to +0x10f0
		break;
	case 2:  // 0x7193a7
		mOrbitX.SetCurve(GTS(0x7e59b0), &mJumpClock);
		mOrbitY.SetCurve(GTS(0x7e5b94), &mJumpClock);
		mOrbitZ.SetCurve(GTS(0x7e5b70), &mJumpClock);
		mCameraPathX.SetCurve(GTS(0x7e5990), &mJumpClock);
		mCameraPathY.SetCurve(GTS(0x7e5964), &mJumpClock);
		mCameraPathZ.SetCurve(GTS(0x7e5934), &mJumpClock);
		mSidebarTurnY.SetCurve(GTS(0x7e5910), &mJumpClock);
		mSidebarTurnZ.SetCurve(GTS(0x7e58e0), &mJumpClock);
		mSidebarTurnX.SetCurve(GTS(0x7e58b0), &mJumpClock);
		mJumpClock.SetCurve(GTS(0x7e5a64));
		mEngineFire.SetCurve(GTS(0x7e5a18));
		mEngineGlow.SetCurve(GTS(0x7e59d8), &mEngineFire);  // linked to +0x10f0
		break;
	case 3:  // 0x7196a1
		mOrbitX.SetCurve(GTS(0x7e588c), &mJumpClock);
		mOrbitY.SetCurve(GTS(0x7e5854), &mJumpClock);
		mOrbitZ.SetCurve(GTS(0x7e5b70), &mJumpClock);
		mCameraPathX.SetCurve(GTS(0x7e5990), &mJumpClock);
		mCameraPathY.SetCurve(GTS(0x7e5834), &mJumpClock);
		mCameraPathZ.SetCurve(GTS(0x7e580c), &mJumpClock);
		mSidebarTurnY.SetCurve(GTS(0x7e57e4), &mJumpClock);
		mSidebarTurnZ.SetCurve(GTS(0x7e57b8), &mJumpClock);
		mSidebarTurnX.SetCurve(GTS(0x7e5790), &mJumpClock);
		mJumpClock.SetCurve(GTS(0x7e5a64));
		mEngineFire.SetCurve(GTS(0x7e5a18));
		mEngineGlow.SetCurve(GTS(0x7e59d8), &mEngineFire);  // linked to +0x10f0
		break;
	case 4:  // 0x71999b
		mOrbitX.SetCurve(GTS(0x7e5cd0), &mJumpClock);
		mOrbitY.SetCurve(GTS(0x7e5c98), &mJumpClock);
		mOrbitZ.SetCurve(GTS(0x7e5c74), &mJumpClock);
		mCameraPathX.SetCurve(GTS(0x7e5ec0), &mJumpClock);
		mCameraPathY.SetCurve(GTS(0x80d3c8), &mJumpClock);
		mCameraPathZ.SetCurve(GTS(0x7e5e94), &mJumpClock);
		mSidebarTurnY.SetCurve(GTS(0x7e5c40), &mJumpClock);
		mSidebarTurnZ.SetCurve(GTS(0x7e5c18), &mJumpClock);
		mSidebarTurnX.SetCurve(GTS(0x7e5bec), &mJumpClock);
		break;
	case 5:  // 0x719bde
		mOrbitX.SetCurve(GTS(0x7e5768), &mJumpClock);
		mOrbitY.SetCurve(GTS(0x7e5b94), &mJumpClock);
		mOrbitZ.SetCurve(GTS(0x7e5b70), &mJumpClock);
		mCameraPathX.SetCurve(GTS(0x7e5748), &mJumpClock);
		mCameraPathY.SetCurve(GTS(0x7e5724), &mJumpClock);
		mCameraPathZ.SetCurve(GTS(0x7e56f8), &mJumpClock);
		mSidebarTurnY.SetCurve(GTS(0x7e56d8), &mJumpClock);
		mSidebarTurnZ.SetCurve(GTS(0x7e56b0), &mJumpClock);
		mSidebarTurnX.SetCurve(GTS(0x7e5684), &mJumpClock);
		mJumpClock.SetCurve(GTS(0x7e5a64));
		mEngineFire.SetCurve(GTS(0x7e5a18));
		mEngineGlow.SetCurve(GTS(0x7e59d8), &mEngineFire);  // linked to +0x10f0
		break;
	case 6:  // 0x719ed8
		mOrbitX.SetCurve(GTS(0x7e5654), &mJumpClock);
		mOrbitY.SetCurve(GTS(0x7e5614), &mJumpClock);
		mOrbitZ.SetCurve(GTS(0x7e5c74), &mJumpClock);
		mCameraPathX.SetCurve(GTS(0x7e55f4), &mJumpClock);
		mCameraPathY.SetCurve(GTS(0x80d3c8), &mJumpClock);
		mCameraPathZ.SetCurve(GTS(0x7e55d0), &mJumpClock);
		mSidebarTurnY.SetCurve(GTS(0x7e5c40), &mJumpClock);
		mSidebarTurnZ.SetCurve(GTS(0x7e5c18), &mJumpClock);
		mSidebarTurnX.SetCurve(GTS(0x7e55a8), &mJumpClock);
		break;
	case 7:  // 0x71a11b
		mOrbitX.SetCurve(GTS(0x7e557c), &mJumpClock);
		mOrbitY.SetCurve(GTS(0x7e5b94), &mJumpClock);
		mOrbitZ.SetCurve(GTS(0x7e5b70), &mJumpClock);
		mCameraPathX.SetCurve(GTS(0x7e5554), &mJumpClock);
		mCameraPathY.SetCurve(GTS(0x7e552c), &mJumpClock);
		mCameraPathZ.SetCurve(GTS(0x7e5500), &mJumpClock);
		mSidebarTurnY.SetCurve(GTS(0x7e54d8), &mJumpClock);
		mSidebarTurnZ.SetCurve(GTS(0x7e54b0), &mJumpClock);
		mSidebarTurnX.SetCurve(GTS(0x7e5478), &mJumpClock);
		mJumpClock.SetCurve(GTS(0x7e5a64));
		mEngineFire.SetCurve(GTS(0x7e5a18));
		mEngineGlow.SetCurve(GTS(0x7e59d8), &mEngineFire);  // linked to +0x10f0
		break;
	case 8:  // 0x71a415
		mOrbitX.SetCurve(GTS(0x7e544c), &mJumpClock);
		mOrbitY.SetCurve(GTS(0x7e5414), &mJumpClock);
		mOrbitZ.SetCurve(GTS(0x7e5c74), &mJumpClock);
		mCameraPathX.SetCurve(GTS(0x7e56d8), &mJumpClock);
		mCameraPathY.SetCurve(GTS(0x7e5834), &mJumpClock);
		mCameraPathZ.SetCurve(GTS(0x7e53ec), &mJumpClock);
		mSidebarTurnY.SetCurve(GTS(0x7e53c4), &mJumpClock);
		mSidebarTurnZ.SetCurve(GTS(0x7e53a4), &mJumpClock);
		mSidebarTurnX.SetCurve(GTS(0x7e5370), &mJumpClock);
		break;
	case 9:  // 0x71a658
		mOrbitX.SetCurve(GTS(0x7e5350), &mJumpClock);
		mOrbitY.SetCurve(GTS(0x7e5310), &mJumpClock);
		mOrbitZ.SetCurve(GTS(0x7e5c74), &mJumpClock);
		mCameraPathX.SetCurve(GTS(0x7e52e0), &mJumpClock);
		mCameraPathY.SetCurve(GTS(0x7e5834), &mJumpClock);
		mCameraPathZ.SetCurve(GTS(0x7e52b8), &mJumpClock);
		mSidebarTurnY.SetCurve(GTS(0x7e5290), &mJumpClock);
		mSidebarTurnZ.SetCurve(GTS(0x7e53a4), &mJumpClock);
		mSidebarTurnX.SetCurve(GTS(0x7e526c), &mJumpClock);
		break;
	case 10:  // 0x71a89b
		mOrbitX.SetCurve(GTS(0x7e5240), &mJumpClock);
		mOrbitY.SetCurve(GTS(0x7e5208), &mJumpClock);
		mOrbitZ.SetCurve(GTS(0x7e5c74), &mJumpClock);
		mCameraPathX.SetCurve(GTS(0x7e51e4), &mJumpClock);
		mCameraPathY.SetCurve(GTS(0x7e51c0), &mJumpClock);
		mCameraPathZ.SetCurve(GTS(0x7e5190), &mJumpClock);
		mSidebarTurnY.SetCurve(GTS(0x7e5168), &mJumpClock);
		mSidebarTurnZ.SetCurve(GTS(0x7e5140), &mJumpClock);
		mSidebarTurnX.SetCurve(GTS(0x7e5834), &mJumpClock);
		break;
	case 11:  // 0x71aade
		mOrbitX.SetCurve(GTS(0x7e5118), &mJumpClock);
		mOrbitY.SetCurve(GTS(0x7e5208), &mJumpClock);
		mOrbitZ.SetCurve(GTS(0x7e5c74), &mJumpClock);
		mCameraPathX.SetCurve(GTS(0x7e50f0), &mJumpClock);
		mCameraPathY.SetCurve(GTS(0x7e50c8), &mJumpClock);
		mCameraPathZ.SetCurve(GTS(0x7e50a4), &mJumpClock);
		mSidebarTurnY.SetCurve(GTS(0x7e5834), &mJumpClock);
		mSidebarTurnZ.SetCurve(GTS(0x7e5834), &mJumpClock);
		mSidebarTurnX.SetCurve(GTS(0x7e5080), &mJumpClock);
		break;
	case 12:  // 0x71ad21
		mOrbitX.SetCurve(GTS(0x7e5044), &mJumpClock);
		mOrbitY.SetCurve(GTS(0x7e5000), &mJumpClock);
		mOrbitZ.SetCurve(GTS(0x7e5b70), &mJumpClock);
		mCameraPathX.SetCurve(GTS(0x7e4fc8), &mJumpClock);
		mCameraPathY.SetCurve(GTS(0x7e5834), &mJumpClock);
		mCameraPathZ.SetCurve(GTS(0x7e4fa0), &mJumpClock);
		mSidebarTurnY.SetCurve(GTS(0x7e4f6c), &mJumpClock);
		mSidebarTurnZ.SetCurve(GTS(0x7e53a4), &mJumpClock);
		mSidebarTurnX.SetCurve(GTS(0x7e4f30), &mJumpClock);
		mJumpClock.SetCurve(GTS(0x7e5a64));
		mEngineFire.SetCurve(GTS(0x7e5a18));
		mEngineGlow.SetCurve(GTS(0x7e59d8), &mEngineFire);  // linked to +0x10f0
		break;
	case 13:  // 0x71b01b
		mOrbitX.SetCurve(GTS(0x7e4f08), &mJumpClock);
		mOrbitY.SetCurve(GTS(0x7e5b94), &mJumpClock);
		mOrbitZ.SetCurve(GTS(0x7e5b70), &mJumpClock);
		mCameraPathX.SetCurve(GTS(0x7e4ee0), &mJumpClock);
		mCameraPathY.SetCurve(GTS(0x7e4eb8), &mJumpClock);
		mCameraPathZ.SetCurve(GTS(0x7e4e84), &mJumpClock);
		mSidebarTurnY.SetCurve(GTS(0x7e4e58), &mJumpClock);
		mSidebarTurnZ.SetCurve(GTS(0x7e53a4), &mJumpClock);
		mSidebarTurnX.SetCurve(GTS(0x7e4e24), &mJumpClock);
		mJumpClock.SetCurve(GTS(0x7e5a64));
		mEngineFire.SetCurve(GTS(0x7e5a18));
		mEngineGlow.SetCurve(GTS(0x7e59d8), &mEngineFire);  // linked to +0x10f0
		break;
	}
}

// 0x70e83c: five curves to 1 and the board's brightness to 0, as a jump into the camera's path begins
void ResetJumpCurves(Hyperspace* theSpace)
{
	theSpace->mSidebarBehind.SetConstant(1.0);
	theSpace->mCrystalGrow.SetConstant(1.0);
	theSpace->mSidebarGlance.SetConstant(1.0);
	theSpace->mCameraSlerp.SetConstant(1.0);
	theSpace->mSidebarShake.SetConstant(1.0);
	theSpace->mBoardBrightness.SetConstant(0.0);
}

void Hyperspace::SetTransition3D(int theTransition)
{
	// 0x7181b3 (the caller stored mTransition, +0x150); the gD3DInterface == 0 branches are SetTransition's own
	// (transition 14, the trial's last jump with its limit dialog, is registration code, left out)
	if (theTransition == 8) {
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
		mJumpClock.SetCurve(GTS(0x7e61fc));
		mEngineFire.SetCurve(GTS(0x7e61b0));
		mEngineGlow.SetCurve(GTS(0x7e6174), &mEngineFire);  // linked to +0x10f0
		mSidebarBehind.SetCurve(GTS(0x7e6150));
		mSidebarShake.SetCurve(GTS(0x7e612c));
		mVizSpeed.SetCurve(GTS(0x7e6108));
		mBoardBrightness.SetCurve(GTS(0x7e60e4));
		mTubeBlendBase.SetCurve(GTS(0x7e60bc));
		mTubeAlpha.SetCurve(GTS(0x7e6098), &mTubeBlendBase);  // linked to +0x338
		mSidebarGlance.SetCurve(GTS(0x7e6068));
		mCrystalSpin.SetCurve(GTS(0x7e6048));
		mCrystalGrow.SetCurve(GTS(0x7e6028));
		mTunnelCountdown.SetCurve(GTS(0x7e6008));
		mCrystalSize.SetCurve(GTS(0x7e5fd8));
		mCameraSlerp.SetCurve(GTS(0x7e5fb4));
		mSidebarSlide.SetCurve(GTS(0x7e5f90));
		mConeAlpha.SetCurve(GTS(0x7e5f60), &mJumpClock);
		SetWorldCurves(mWorld);
		if (aMakeCrystals)
			MakeCrystals();
	} else if (theTransition == 9) {
		mDrawsOwnBackground = true;
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
		double aZ = mCameraPathZ.GetOutVal();
		double aY = mCameraPathY.GetOutVal();
		float aX = (float)mCameraPathX.GetOutVal();
		mCameraPathStart = Vec3{aX, (float)aY, (float)aZ};
		ResetJumpCurves(this);
		mIs3D = true;
		mCrystalSpin.SetCurve(GTS(0x7e4d74));
		mCrystalSize.SetCurve(GTS(0x7e4924));
		mEngineFire.SetCurve(GTS(0x7e4d48));
		mEngineGlow.SetCurve(GTS(0x7e4d20), &mEngineFire);  // linked to +0x10f0
		mJumpClock.SetCurve(GTS(0x7e4cf4));
		mOrbitX.SetCurve(GTS(0x7e4cc8), &mJumpClock);
		mOrbitY.SetCurve(GTS(0x7e4c94), &mJumpClock);
		mOrbitZ.SetCurve(GTS(0x7e4c6c), &mJumpClock);
		mCameraPathX.SetCurve(GTS(0x7e56d8), &mJumpClock);
		mCameraPathY.SetCurve(GTS(0x80d3c8), &mJumpClock);
		mCameraPathZ.SetCurve(GTS(0x7e4c40), &mJumpClock);
		mSidebarTurnY.SetCurve(GTS(0x7e4c1c), &mJumpClock);
		mSidebarTurnZ.SetCurve(GTS(0x7e53a4), &mJumpClock);
		mSidebarTurnX.SetCurve(GTS(0x7e4be8), &mJumpClock);
		mConeAlpha.SetCurve(GTS(0x7e4bb4), &mJumpClock);
		mFirstHalfPending = false;
	} else if (theTransition == 10) {
		mDrawsOwnBackground = true;
		ChooseViz(true);
		mState = 3;
		gApp->PlaySoundEx(SOUND_TRANS_SPACENOISE, 0, 0.0f, 2.0f, true, true);
		ResetJumpCurves(this);
		mIs3D = false;
		mCrystalSpin.SetCurve(GTS(0x7e4b08));
		mCrystalSize.SetCurve(GTS(0x7e4924));
		mEngineFire.SetConstant(0.0);
		mEngineGlow.SetConstant(0.0);
		mJumpClock.SetCurve(GTS(0x7e4cf4));
		mOrbitX.SetCurve(GTS(0x7e4ae0), &mJumpClock);
		mOrbitY.SetCurve(GTS(0x7e4aac), &mJumpClock);
		mOrbitZ.SetCurve(GTS(0x7e5c74), &mJumpClock);
		mCameraPathX.SetCurve(GTS(0x7e56d8), &mJumpClock);
		mCameraPathY.SetCurve(GTS(0x80d3c8), &mJumpClock);
		mCameraPathZ.SetCurve(GTS(0x7e4c40), &mJumpClock);
		mSidebarTurnX.SetConstant(0.0);
		mSidebarTurnZ.SetConstant(0.0);
		mSidebarTurnY.SetConstant(0.0);
		mFirstHalfPending = false;
	} else if (theTransition == 12) {
		mBoardBrightness.SetConstant(1.0);
		mVizSpeed.SetConstant(1.0);
		mSidebarGlance.SetConstant(0.0);
		mTubeBlendBase.SetConstant(0.0);
		mTubeAlpha.SetConstant(0.0);
		mSidebarTurnX.SetConstant(0.0);
		mSidebarTurnZ.SetConstant(0.0);
		mSidebarTurnY.SetConstant(0.0);
		mCameraPathX.SetConstant(0.0);
		mCameraPathY.SetConstant(0.0);
		mCameraPathZ.SetConstant(0.0);
		mIs3D = false;
		ChooseViz(true);
		mTransition = 12;
		mState = 11;
		mDrawsFinalFade = true;
		mLevel = 0;
		mBoardBrightnessFinal.SetCurve(GTS(0x7e4a7c));
		mFlash.SetCurve(GTS(0x7e4a48));
		mBoardBrightnessFinal.SetCurve(GTS(0x7e4a1c));
		gBoard->SetBoardFade(0.0f);
		mDrawsOwnBackground = true;
		gApp->mBoard->SetVisible(false);
		gApp->mVizManager->SetVisible(false);
	}
	Update();
}


///////////////////////////////////////////////////////////////////////////////
// Update (0x715c21) with gD3DInterface set

// The states of a normal (registered) level-up jump, transition 8, end to end:
//   1  the jump has been asked for: mVizSpeed (the VizManager's speed, set by SetTransition's curves) runs; when it is
//      done the VizManager's picture goes into +0x160, the VizManager is hidden, +0x155 and +0x156 are set and the
//      board takes no mouse (mMouseVisible, +0x55 = 0) (SOUND_TRANS_SPACENOISE)                          -> 2
//   2  the camera flies off; mTunnelCountdown (the countdown the tunnel shows under 1) runs; when it is done the engines
//      ignite (SOUND_TRANS_ENGINEIGNITION, the engine loop starts)                                           -> 3
//   3  the jump proper on the path's clock mJumpClock: 0xfa from its end the closing curves, 0x8c SOUND_TRANS_ARRIVAL, 0x3c the souls are let go (+0x158 clear), and
//      0x23 updates from its end the new level is made (mIs3D off, NewLevel, ClearBoard, FillInBlanks) and the state
//      is 4 (state 5 stays)                                                                                -> 4
//   4  when 6B8 ends: the VizManager is shown again and mVizSpeed gets the arrival curve                  -> 5
//   5  mVizSpeed crosses 0.3: the travellers retarget; when it has been triggered the board takes the mouse -> 10
//   10 the jump is over (the board sees it and clears mClicked). (States 6 to 9 belonged to transition 14, the trial's
//      limit, registration code left out.) Transition 9 and 10 (the game's start) run 3 to 5 the same way.
// Every state change hangs on a curve of the SetTransition: a curve with no string (the port's GameTextTable.inc lacking
// the address, so GTS(addr) is "") has no step (IncInVal() is false, as for a finished one) and every state runs on at
// once; regenerate the table with games/bejeweledtwist/tools/gametext.py after adding a GTS use.
void Hyperspace::Update3D()
{
	// 0x716336 and on: the common code of every transition, which the 2D chain (0x715d68) leaves to them only in 3D
	if (!mVizSpeed.IncInVal() && mState == 1) {
		// the jump's first step is over: the VizManager's picture goes into +0x160 and space takes the screen
		Graphics aG(mOldPicture);
		gApp->mVizManager->Draw(&aG);
		gApp->mVizManager->SetVisible(false);
		ChooseViz(false);
		mDrawsOwnBackground = true;
		mIs3D = true;
		gApp->mBoard->mMouseVisible = false;  // +0x55: the board takes no mouse while space is on screen
		mState++;
		gApp->PlaySoundEx(SOUND_TRANS_SPACENOISE, 0, 0.0f, 2.0f, true, true);
	}
	gApp->mVizManager->mSpeed = mVizSpeed.GetOutVal();
	// the VizManager's rectangle for its draw hook, +0x1bc to +0x1c8: all of the screen
	gApp->mVizManager->mClipRect[0] = -1;
	gApp->mVizManager->mClipRect[1] = -1;
	gApp->mVizManager->mClipRect[2] = 1;
	gApp->mVizManager->mClipRect[3] = 1;
	if (gApp->mBoard != nullptr && !gApp->mBoard->mIsReplay) {
		mBoardBrightness.IncInVal();
		gApp->mBoard->SetBoardFade((float)mBoardBrightness.GetOutVal());
		if (mTransition == 11)
			gApp->mBoard->ApplyBoardFade();
	}
	mCrystalStep.IncInVal();
	mEngineFire.IncInVal();
	mSidebarGlance.IncInVal();
	mTubeBlendBase.IncInVal();
	if (1 < mState && mState < 10) {
		mCrystalSpin.IncInVal();
		if (mTransition == 8 || mTransition == 9 || mTransition == 10) {
			mSidebarBehind.IncInVal();
			mSidebarShake.IncInVal();
		}
		mCrystalGrow.IncInVal();
		mCameraSlerp.IncInVal();
		mSidebarSlide.IncInVal();
		mSidebarAside.IncInVal();
		if (mTunnelCountdown.CheckUpdatesFromEndThreshold(0x7d))
			gApp->PlaySample(SOUND_TRANS_PREIGNITION);
		if (!mTunnelCountdown.IncInVal() && mState == 2) {
			mState = 3;
			gApp->PlaySample(SOUND_TRANS_ENGINEIGNITION);
			mSpaceSound = gApp->mSoundManager->GetSoundInstance(SOUND_TRANS_ENGINELOOP1);  // 0x75fa0b
			if (mSpaceSound != nullptr) {
				mSpaceSound->Play(true, false);
				mSpaceSound->SetVolume(2.0);
			}
		}
		mCrystalSize.IncInVal();
	}
	if (2 < mState && mState < 10) {
		if (mTransition != 13 && !mJumpClock.IncInVal()) {
			if (gApp->mMainMenu->mChallengeMenu != nullptr)
				gApp->mMainMenu->mChallengeMenu->mVisible = false;
			gApp->mVizManager->SetVisible(true);
			mState++;
			mVizSpeed.SetCurve(GTS(0x7e496c));
		}
		if (mJumpClock.CheckUpdatesFromEndThreshold(0x17c) && mFirstHalfPending) {
			mFirstHalfPending = false;
			mCrystalSpin.SetCurve(GTS(0x7e4948));
			mCrystalSize.SetCurve(GTS(0x7e4924));
			gApp->mVizManager->TakePendingViz();  // 0x7581e5
			Graphics aG(mOldPicture);
			gApp->mVizManager->Draw(&aG);
		}
		if (mJumpClock.CheckUpdatesFromEndThreshold(0xfa) && mTransition != 11) {
			mCameraSlerp.SetCurve(GTS(0x7e48fc));
			mCrystalGrow.SetCurve(GTS(0x7e48dc));
			mSidebarShake.SetCurve(GTS(0x7e48bc));
			mSidebarBehind.SetCurve(GTS(0x7e48bc));
		}
		if (mJumpClock.CheckUpdatesFromEndThreshold(0x8c))
			gApp->PlaySample(SOUND_TRANS_ARRIVAL);
		if (mJumpClock.CheckUpdatesFromEndThreshold(0x3c)) {
			for (GemSoul* aSoul : gBoard->mLevelSouls)
				aSoul->mIsVisible = false;
			mDrawsTravellers = false;
		}
		if (mJumpClock.CheckUpdatesFromEndThreshold(0x23)) {
			{
				mIs3D = false;
				if (mState != 5)
					mState = 4;
				mBoardBrightness.SetCurve(GTS(0x7e4874));
				if (mTransition == 11) {
					mTubeBlendBase.SetCurve(GTS(0x7e4840));
					mTubeAlpha.SetCurve(GTS(0x7e481c), &mTubeBlendBase);  // linked to +0x338
					mSidebarGlance.SetCurve(GTS(0x7e47e8));
				} else {
					mTubeBlendBase.SetCurve(GTS(0x7e47b4));
					mTubeAlpha.SetCurve(GTS(0x7e478c), &mTubeBlendBase);  // linked to +0x338
					mSidebarGlance.SetCurve(GTS(0x7e4758));
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
		if (mVizSpeed.CheckInThreshold(0.3))  // 0x81d278
			gApp->mBoard->RetargetTravellers();
		if (mVizSpeed.HasBeenTriggered()) {
			gApp->mBoard->mMouseVisible = true;  // +0x55 = 1
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

// 0x70ff99: a vector copy, inline here
void Hyperspace::UpdateWorld3D()
{
	// the crystals' spin (0x716e31)
	unsigned int i = 0;
	if (!mCrystals.empty()) {
		do {
			Mat3 aRot = mCrystals[i].mTransform.mRot;
			aRot.RotateX(mCrystals[i].mSpinX);
			aRot.RotateY(mCrystals[i].mSpinY);
			mCrystals[i].mTransform.mRot = aRot;
			i++;
		} while (i < mCrystals.size());
	}
	i = 0;
	if (!mRingCrystals.empty()) {
		do {
			RingCrystal& aCrystal = mRingCrystals[i];
			Mat3 aRot = aCrystal.mTransform.mRot;
			aRot.RotateX(aCrystal.mSpin.x);
			aRot.RotateY(aCrystal.mSpin.y);
			aCrystal.mTransform.mRot = aRot;
			Mat3 aSpin = aCrystal.mOwnTurn;
			aSpin.RotateY(aCrystal.mSpin.z);
			aCrystal.mOwnTurn = aSpin;
			i++;
		} while (i < mRingCrystals.size());
	}
	// the world's camera (0x716f83): a camera at the origin looking at (0, 0, -1) (0x674035 builds its rows by the
	// same cross products as 0x70ffbd, from its place minus the point, so its third row is +z), the sidebar's
	// offset behind it
	Transform3D aLook;
	LookAlong(aLook.mRot, Vec3{0.0f, 0.0f, 1.0f}, Vec3{0.0f, 1.0f, 0.0f});
	GPUMatrix aLookMatrix = WorldMatrixOf(aLook);
	float aBehind = (float)(mSidebarBehind.GetOutVal() * 0.5 + 2.7300000190734863);
	Vec3 anOffset = Mat4TransformPoint(aLookMatrix, Vec3{0.0f, 0.0f, aBehind});
	float aNext = (float)(mJumpClock.GetInVal() + mJumpClock.mIncRate);
	Vec3 aTo;
	aTo.x = (float)mCameraPathX.GetOutVal((double)aNext);
	aTo.y = (float)mCameraPathY.GetOutVal((double)aNext);
	aTo.z = (float)mCameraPathZ.GetOutVal((double)aNext);
	double aPathZ = mCameraPathZ.GetOutVal();
	double aPathY = mCameraPathY.GetOutVal();
	Vec3 aFrom;
	aFrom.x = (float)mCameraPathX.GetOutVal();
	aFrom.y = (float)aPathY;
	aFrom.z = (float)aPathZ;
	mSidebarShake.GetOutVal();  // (0x7170e2: read, and not used)
	Vec3 aPlace;
	aPlace.x = aFrom.x + anOffset.x;
	aPlace.y = aFrom.y + anOffset.y;
	aPlace.z = aFrom.z + anOffset.z;
	mWorldCameraPos[0] = mCameraPathStart.x + aPlace.x;
	mWorldCameraPos[1] = mCameraPathStart.y + aPlace.y;
	mWorldCameraPos[2] = mCameraPathStart.z + aPlace.z;
	float anAspect = (float)((double)mWidth / (double)mHeight);
	float aFovTimesAspect = (float)((double)anAspect * 60.0);  // 0x7d7d60
	mWorldCamera.SetFov(aFovTimesAspect, anAspect, 0.20000000298023224f, 500.0f);
	Transform3D aCamera;
	Translate(aCamera, mWorldCameraPos[0], mWorldCameraPos[1], mWorldCameraPos[2]);
	aCamera.mRot.RotateZ((float)mSidebarTurnZ.GetOutVal());
	aCamera.mRot.RotateY((float)mSidebarTurnY.GetOutVal());
	aCamera.mRot.RotateX((float)mSidebarTurnX.GetOutVal());
	mWorldCamera.mTransform = aCamera;
	mBackdropCamera.SetFov(aFovTimesAspect, anAspect, 1.0f, 10000.0f);
	aCamera.mRot = aCamera.mRot * mCameraTurn;  // 0x70ff99
	mBackdropCamera.mTransform = aCamera;
	(void)aTo;
}

void Hyperspace::PlaceShip()
{
	// 0x715480: the ship (the sidebar's place, +0x14c0) on the camera path's curves 978, 9e8 and a58, slid by 1240
	// and 178, turned along the path
	Transform3D aShip;
	float aZ = (float)mOrbitZ.GetOutVal();  // the three reads come z, y, x
	float aY = (float)mOrbitY.GetOutVal();
	float aX = (float)mOrbitX.GetOutVal();
	Vec3 aFrom{mCameraPathStart.x + aX, mCameraPathStart.y + aY, mCameraPathStart.z + aZ};
	float aNext = (float)(mJumpClock.GetInVal() + mJumpClock.mIncRate);
	float aNextX = (float)mOrbitX.GetOutVal((double)aNext);
	float aNextY = (float)mOrbitY.GetOutVal((double)aNext);
	float aNextZ = (float)mOrbitZ.GetOutVal((double)aNext);
	Vec3 aTo{mCameraPathStart.x + aNextX, mCameraPathStart.y + aNextY, mCameraPathStart.z + aNextZ};
	mShipHeading = Vec3{aTo.x - aFrom.x, aTo.y - aFrom.y, aTo.z - aFrom.z};
	Vec3 aHeading = NormalizedV(mShipHeading);
	Vec3 aBack{-aHeading.x, -aHeading.y, -aHeading.z};
	Widget* aTube = gApp->mBoard->mGoodTube;  // +0x2194
	float anAside = (float)(((double)(float)aTube->mWidth * 0.525 + (double)(float)aTube->mX - (double)(float)(mWidth / 2)) /
		(double)(gApp->mScale * 694.4375f));
	Mat3 aTurn;
	aTurn.RotateZ((float)((double)mShipHeading.x * -15.0));
	LookAlong(aTurn, aBack, Vec3{0.0f, 1.0f, 0.0f});
	mShipTurn = aTurn;
	aShip.mRot = aTurn;
	Transform3D aNear;
	ScaleBy(aNear, 0.052799999713897705f, 0.052799999713897705f, 0.052799999713897705f);
	aNear.mRot.RotateX(-1.5707963705062866f);
	aNear.mRot.RotateY(3.1415927410125732f);
	float aSlide = (float)mSidebarSlide.GetOutVal();
	float aSlide2 = (float)mSidebarSlide.GetOutVal();
	float aSlide3 = (float)mSidebarSlide.GetOutVal();
	float aNearY = (float)(0.035000000149011612 - (double)aSlide2 * 0.30000001192092896);
	float aNearZ = (float)(1.2000000476837158 - (double)aSlide * 0.10000000149011612);
	float aNearX = (float)(((1.0049999952316284 - (double)aSlide3) + mSidebarAside.GetOutVal() * 3.0) * (double)anAside);
	Translate(aNear, aNearX, aNearY, aNearZ);
	Translate(aNear, aFrom.x, aFrom.y, aFrom.z);
	Transform3D aFar;
	ScaleBy(aFar, 0.053500000387430191f, 0.053500000387430191f, 0.053500000387430191f);
	aFar.mRot.RotateY(3.1415927410125732f);
	float aFarSlide = (float)mSidebarSlide.GetOutVal();
	float aFarSlide2 = (float)mSidebarSlide.GetOutVal();
	double aFarSlide3 = mSidebarSlide.GetOutVal();
	float aFarY = (float)(0.052999999374151230 - (double)aFarSlide2 * 0.30000001192092896);
	float aFarZ = (float)(1.2000000476837158 - (double)aFarSlide * 0.10000000149011612);
	float aFarX = (float)(((1.0 - aFarSlide3) + mSidebarAside.GetOutVal() * 3.0) * (double)anAside);
	Translate(aFar, aFarX, aFarY, aFarZ);
	aFar = Compose(aFar, aShip);
	Translate(aFar, aFrom.x, aFrom.y, aFrom.z);
	mShipPlace = aNear;
	mShipPlace.mRot = SlerpMat3(mShipPlace.mRot, aFar.mRot, (float)mCameraSlerp.GetOutVal(), false);
}

// 0x714ea9: the list insert, inline here
void Hyperspace::PlaceSaucer()
{
	// 0x7158f8: the saucer (+0xac8) on its own path (b08, b78 and be8), turned along it like the ship
	Transform3D aShip;
	float aZ = (float)mSaucerPathZ.GetOutVal();
	float aY = (float)mSaucerPathY.GetOutVal();
	float aX = (float)mSaucerPathX.GetOutVal();
	Vec3 aFrom{aX, aY, aZ};
	float aNext = (float)(mJumpClock.GetInVal() + mJumpClock.mIncRate);
	Vec3 aTo{(float)mSaucerPathX.GetOutVal((double)aNext), (float)mSaucerPathY.GetOutVal((double)aNext),
		(float)mSaucerPathZ.GetOutVal((double)aNext)};
	Vec3 aDelta{aTo.x - aFrom.x, aTo.y - aFrom.y, aTo.z - aFrom.z};
	Vec3 aHeading = NormalizedV(aDelta);
	Vec3 aBack{-aHeading.x, -aHeading.y, -aHeading.z};
	Mat3 aTurn;
	aTurn.RotateZ((float)((double)aDelta.x * -15.0));
	LookAlong(aTurn, aBack, Vec3{0.0f, 1.0f, 0.0f});
	mShipTurn = aTurn;
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
	mSaucer.mRot = SlerpMat3(mSaucer.mRot, aSpin.mRot, (float)mCameraSlerp.GetOutVal(), false);
}

///////////////////////////////////////////////////////////////////////////////
// The ship's thrust: DrawParticles (0x71378b) and the particles' step and making (0x715c21's tail, 0x714ea9)

namespace {

// DrawPrimitiveEx's listener slots 0 and 1 are a WorldGraphicsListener's Apply and Remove
class PrimitiveListener : public D3DDrawPrimitiveListener {
public:
	explicit PrimitiveListener(WorldGraphicsListener* theListener) : mListener(theListener) {}
	void Before() override { mListener->Apply(); }
	void After() override { mListener->Remove(); }
	WorldGraphicsListener* mListener;
};

struct ThrustVertex {  // FVF 0x142: position, diffuse, one texture coordinate
	float x, y, z;
	unsigned int mColor;
	float u, v;
};

// 0x4c0c80: a vector times a 3 by 3 matrix (the rows added up by the vector's parts)
Vec3 VecTimesMat3(const Vec3& theVector, const Mat3& theMatrix)
{
	const float* m = theMatrix.m;
	Vec3 aResult;
	aResult.x = m[6] * theVector.z + m[0] * theVector.x + m[3] * theVector.y;
	aResult.y = m[7] * theVector.z + m[4] * theVector.y + m[1] * theVector.x;
	aResult.z = m[8] * theVector.z + m[5] * theVector.y + m[2] * theVector.x;
	return aResult;
}

}  // namespace

void Hyperspace::DrawParticles()
{
	// 0x71378b: the thrust's puffs as quads turned with the ship (type 0), and the trails as two strips between a trail
	// particle and the engine's last one (type 1), through the sidebar's camera (+0x144c)
	OffCenterListener aListener(&mSidebarCamera, false);
	aListener.SetCulling(false);
	aListener.SetDepth(true, false);
	PrimitiveListener aPrimitiveListener(&aListener);
	bool aHavePrevious[9] = {false, false, false, false, false, false, false, false, false};
	ThrustVertex aPrevious[9][4];
	for (std::list<ThrustParticle>::iterator anItr = mParticles.begin(); anItr != mParticles.end(); ++anItr) {
		ThrustParticle& aParticle = *anItr;
		float anAlpha = aParticle.mAlpha <= 1.0 ? aParticle.mAlpha : 1.0f;
		int anAlphaByte = Ftol((double)anAlpha * 255.0);
		unsigned int aColor = (unsigned int)Color(255, 255, 255, anAlphaByte).ToInt();
		float aSize = aParticle.mSize;
		Transform3D aPlace;
		aPlace.mRot.RotateZ(aParticle.mAngle);
		Transform3D aShip = TransformOfMat3(mShipTurn);
		aPlace = Compose(aPlace, aShip);
		Translate(aPlace, aParticle.mPos.x, aParticle.mPos.y, aParticle.mPos.z);
		if (aParticle.mType == 1) {
			GPUMatrix aMatrix = WorldMatrixOf(aPlace);
			Vec3 aTop = Mat4TransformPoint(aMatrix, Vec3{0.0f, -aSize, 0.0f});
			Vec3 aBottom = Mat4TransformPoint(aMatrix, Vec3{0.0f, aSize, 0.0f});
			Vec3 aLeft = Mat4TransformPoint(aMatrix, Vec3{-aSize, 0.0f, 0.0f});
			Vec3 aRight = Mat4TransformPoint(aMatrix, Vec3{aSize, 0.0f, 0.0f});
			ThrustVertex aNew[4] = {
				{aTop.x, aTop.y, aTop.z, aColor, 0.5f, 0.0f},
				{aBottom.x, aBottom.y, aBottom.z, aColor, 0.5f, 1.0f},
				{aLeft.x, aLeft.y, aLeft.z, aColor, 0.0f, 0.5f},
				{aRight.x, aRight.y, aRight.z, aColor, 1.0f, 0.5f},
			};
			int anEngine = aParticle.mEngine;
			if (aHavePrevious[anEngine]) {
				ThrustVertex aFirst[4] = {aNew[0], aNew[1], aPrevious[anEngine][0], aPrevious[anEngine][1]};
				ThrustVertex aSecond[4] = {aNew[2], aNew[3], aPrevious[anEngine][2], aPrevious[anEngine][3]};
				aListener.mWorld = Transform3D();
				gD3DInterface->SetTexture(0, aParticle.mImage);
				gD3DInterface->DrawPrimitiveEx(0x142, GPUPT_TRIANGLESTRIP, aFirst, 2, Color::White, 1, 0.0f, 0.0f, true,
					&aPrimitiveListener);
				gD3DInterface->DrawPrimitiveEx(0x142, GPUPT_TRIANGLESTRIP, aSecond, 2, Color::White, 1, 0.0f, 0.0f, true,
					&aPrimitiveListener);
			}
			for (int i = 0; i < 4; i++)
				aPrevious[anEngine][i] = aNew[i];
			aHavePrevious[anEngine] = true;
		} else {
			ThrustVertex aQuad[4] = {
				{-aSize, aSize, 0.0f, aColor, 0.0f, 0.0f},
				{aSize, aSize, 0.0f, aColor, 1.0f, 0.0f},
				{-aSize, -aSize, 0.0f, aColor, 0.0f, 1.0f},
				{aSize, -aSize, 0.0f, aColor, 1.0f, 1.0f},
			};
			aListener.mWorld = aPlace;
			gD3DInterface->SetTexture(0, aParticle.mImage);
			gD3DInterface->DrawPrimitiveEx(0x142, GPUPT_TRIANGLESTRIP, aQuad, 2, Color::White, 1, 0.0f, 0.0f, true,
				&aPrimitiveListener);
		}
	}
}

namespace {

// The tunnel's vertex (0x34 bytes, made by 0x70f22c; FVF 0x402d2: position, normal, diffuse, specular, two texture sets,
// the second of three floats)
struct CubeVertex {
	float x, y, z;           // +0x00
	float nx, ny, nz;        // +0x0c
	unsigned int mDiffuse;   // +0x18
	unsigned int mSpecular;  // +0x1c: 0
	float u, v;              // +0x20
	float mSecondTexCoord[3];     // +0x28: a second set of texture coordinates, always 0
};

// 0x70f22c
CubeVertex MakeCubeVertex(float x, float y, float z, float nx, float ny, float nz, float theU, float theV, unsigned int theColor)
{
	CubeVertex aVertex;
	aVertex.x = x;
	aVertex.mSpecular = 0;
	aVertex.y = y;
	aVertex.mDiffuse = theColor;
	aVertex.z = z;
	aVertex.nx = nx;
	aVertex.ny = ny;
	aVertex.nz = nz;
	aVertex.u = theU;
	aVertex.v = theV;
	aVertex.mSecondTexCoord[2] = 0.0f;
	aVertex.mSecondTexCoord[1] = 0.0f;
	aVertex.mSecondTexCoord[0] = 0.0f;
	return aVertex;
}

}  // namespace

void Hyperspace::DrawTunnel3D(Graphics* g)
{
	// 0x711b65's Direct3D half: the level's picture on a cube turning through the tunnel (two passes: pass 0 the
	// picture's cube, pass 1 a larger, additive rainbow cube about it), six faces to a pass
	(void)g;
	if (gApp->mVizManager->mVisible)  // 0x711b83: +0x54 of the VizManager
		return;
	WorldGraphicsListener aListener(&mWorldCamera, false);  // 0x4bd5d0, ebp-0x164
	PrimitiveListener aPrimitiveListener(&aListener);
	// 0x4bd4c0, 0x674035: a transform at the origin looking at (0, 0, -1) (LookAlong's rows, the third one its place
	// minus the point, +z); its matrix (0x4c1940) is not used
	Transform3D aLook;
	LookAlong(aLook.mRot, Vec3{0.0f, 0.0f, 1.0f}, Vec3{0.0f, 1.0f, 0.0f});
	int aPass = 0;  // ebp-0x28
	do {
		double aRatioA = mCrystalGrow.GetOutVal();  // ebp-0x4c
		double aRatioB = mCrystalGrow.GetOutVal();  // ebp-0x30
		double aSize = mCrystalSize.GetOutVal();
		float aHalfWidth = (float)((((double)mWidth / (double)mHeight) * (1.0 - aRatioA) + aRatioB) * aSize);  // ebp-0x14
		float aHalfHeight = (float)mCrystalSize.GetOutVal();      // ebp+8
		float aHalfDepth = (float)mCrystalSize.GetOutVal();       // ebp-0x20
		bool aFirstHalfPending = mFirstHalfPending;                                // ebp-0x32
		float anAlpha = (float)(mCrystalSize.GetOutVal() * 2.0);  // ebp-0x10
		if (aPass == 1) {
			double aGrow = mCrystalSize.GetOutVal();
			anAlpha = (float)(mCrystalGrow.GetOutVal() * aGrow);
		}
		if (!(anAlpha <= 1.0))
			anAlpha = 1.0f;
		unsigned int aColor = Color(255, 255, 255, Ftol((double)anAlpha * 255.0)).ToInt();  // 0x401740, 0x401860
		if (aPass == 1) {
			aHalfWidth = (float)((double)aHalfWidth * 1.5);  // 0x81d3d0
			aHalfHeight = (float)((double)aHalfHeight * 1.5);
			aHalfDepth = (float)((double)aHalfDepth * 1.5);
		}
		float aMinusW = -aHalfWidth;
		float aMinusH = -aHalfHeight;
		float aMinusD = -aHalfDepth;
		// The 24 vertices, four to a face (a strip of two triangles): the normal, then each corner's place and picture
		CubeVertex aVertices[24];
		aVertices[0] = MakeCubeVertex(aMinusW, aHalfHeight, aHalfDepth, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, aColor);
		aVertices[1] = MakeCubeVertex(aHalfWidth, aHalfHeight, aHalfDepth, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, aColor);
		aVertices[2] = MakeCubeVertex(aMinusW, aMinusH, aHalfDepth, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, aColor);
		aVertices[3] = MakeCubeVertex(aHalfWidth, aMinusH, aHalfDepth, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, aColor);
		aVertices[4] = MakeCubeVertex(aHalfWidth, aHalfHeight, aMinusD, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, aColor);
		aVertices[5] = MakeCubeVertex(aMinusW, aHalfHeight, aMinusD, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, aColor);
		aVertices[6] = MakeCubeVertex(aHalfWidth, aMinusH, aMinusD, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, aColor);
		aVertices[7] = MakeCubeVertex(aMinusW, aMinusH, aMinusD, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, aColor);
		aVertices[8] = MakeCubeVertex(aHalfWidth, aHalfHeight, aMinusD, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, aColor);
		aVertices[9] = MakeCubeVertex(aHalfWidth, aMinusH, aMinusD, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, aColor);
		aVertices[10] = MakeCubeVertex(aHalfWidth, aHalfHeight, aHalfDepth, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, aColor);
		aVertices[11] = MakeCubeVertex(aHalfWidth, aMinusH, aHalfDepth, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, aColor);
		aVertices[12] = MakeCubeVertex(aMinusW, aMinusH, aMinusD, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, aColor);
		aVertices[13] = MakeCubeVertex(aMinusW, aHalfHeight, aMinusD, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, aColor);
		aVertices[14] = MakeCubeVertex(aMinusW, aMinusH, aHalfDepth, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, aColor);
		aVertices[15] = MakeCubeVertex(aMinusW, aHalfHeight, aHalfDepth, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, aColor);
		aVertices[16] = MakeCubeVertex(aMinusW, aHalfHeight, aHalfDepth, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, aColor);
		aVertices[17] = MakeCubeVertex(aMinusW, aHalfHeight, aMinusD, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, aColor);
		aVertices[18] = MakeCubeVertex(aHalfWidth, aHalfHeight, aHalfDepth, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, aColor);
		aVertices[19] = MakeCubeVertex(aHalfWidth, aHalfHeight, aMinusD, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, aColor);
		aVertices[20] = MakeCubeVertex(aMinusW, aMinusH, aHalfDepth, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, aColor);
		aVertices[21] = MakeCubeVertex(aHalfWidth, aMinusH, aHalfDepth, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, aColor);
		aVertices[22] = MakeCubeVertex(aMinusW, aMinusH, aMinusD, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, aColor);
		aVertices[23] = MakeCubeVertex(aHalfWidth, aMinusH, aMinusD, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, aColor);
		// where the cube is carried: along the jump's path (+0x8e8's direction, +0x153c's start) once the old level is
		// gone (+0x15b clear), else it stays at the origin
		float aPlaceX;  // ebp-0x58
		float aPlaceY;  // ebp-0x54
		float aPlaceZ;  // ebp-0x50
		if (aFirstHalfPending) {
			aPlaceX = 0.0f;
			aPlaceY = 0.0f;
			aPlaceZ = 0.0f;
		} else {
			float aStep = (float)mCrystalStep.GetOutVal();
			float aDirectionX = (float)mCrystalDirection[0];  // ebp-0x2c
			float aDirectionY = (float)mCrystalDirection[1];  // ebp-0x1c
			float aDirectionZ = (float)mCrystalDirection[2];  // ebp-0x44
			aDirectionX = aStep * aDirectionX;
			aDirectionY = aStep * aDirectionY;
			aDirectionZ = aStep * aDirectionZ;
			aPlaceX = aDirectionX + mCameraPathStart.x;
			aPlaceY = mCameraPathStart.y + aDirectionY;
			aPlaceZ = mCameraPathStart.z + aDirectionZ;
		}
		float aSpin = (float)(mCrystalSpin.GetOutVal() * 5.0);  // ebp-0x44; 0x81d220
		int aFace = 0;  // ebp-0x24
		do {
			// the card's abilities: caps bit 4 is vs_2_0, bit 2 ps_2_0 (the port's device has both)
			bool aHasVertexShader = false;  // ebp+0xb
			if (gD3DInterface != nullptr)
				aHasVertexShader = (gD3DInterface->GetCaps(0) & 4) != 0;
			bool aHasPixelShader = false;  // ebp-0x31
			if (gD3DInterface != nullptr)
				aHasPixelShader = (gD3DInterface->GetCaps(0) & 2) != 0;
			float aFaceAlpha = 1.0f;  // ebp-0x10: how much of the face shows
			bool aSkipFace = false;
			if (aPass == 0) {
				mSingleFaceAlpha.SetInVal(mCrystalSpin.GetOutVal());
				if (aFace == 1) {
					aFaceAlpha = (float)mSingleFaceAlpha.GetOutVal();
					if (0.0f == aFaceAlpha)
						aSkipFace = true;
				}
				if (!aSkipFace) {
					// 0x85d838 (ctor 0x710f11, built once at the first use): a D3DDrawPrimitiveListener holding a
					// CubeFixedFunctionShader (0x70f279), which only a card without ps_2_0 sets as the listener's inner
					if (aHasVertexShader)
						gD3DInterface->SetVertexShader(gTwistShaders[TWISTSHADER_CUBE_V]);
					if (aHasPixelShader)
						gD3DInterface->SetPixelShader(gTwistShaders[TWISTSHADER_CUBE_P]);
					else
						NotPorted("CubeFixedFunctionShader 0x70f279 as the listener's inner (a card without ps_2_0)");
					gD3DInterface->SetTexture(0, mOldPicture);
				}
			} else {
				mFirstFaceAlpha.SetInVal(mCrystalSpin.GetOutVal());
				mSecondFaceAlpha.SetInVal(mCrystalSpin.GetOutVal());
				if (aFace == 0 || aFace == 1) {
					aFaceAlpha = (float)(aFace == 0 ? mFirstFaceAlpha.GetOutVal() : mSecondFaceAlpha.GetOutVal());
					if (0.0f == aFaceAlpha)
						aSkipFace = true;
				}
				if (!aSkipFace) {
					// 0x85d828 (ctor 0x710f36, built once at the first use): the same around a
					// RainbowCubeFixedFunctionShader (0x70f389)
					if (aHasVertexShader)
						gD3DInterface->SetVertexShader(gTwistShaders[TWISTSHADER_RAINBOW_CUBE_V]);
					if (aHasPixelShader)
						gD3DInterface->SetPixelShader(gTwistShaders[TWISTSHADER_RAINBOW_CUBE_P]);
					else
						NotPorted("RainbowCubeFixedFunctionShader 0x70f389 as the listener's inner (a card without ps_2_0)");
					gD3DInterface->SetTexture(0, IMAGE_RAINBOWCASE1);
					gD3DInterface->SetTexture(1, IMAGE_RAINBOWCASE2);
				}
			}
			if (!aSkipFace) {
				// the cube's place: turned about y and z by the spin, carried by the camera's look and moved
				Transform3D aCube;
				aCube.mRot.RotateY(aSpin);  // 0x68fa0b
				aCube.mRot.RotateZ((float)((double)aSpin * -0.18000000715255737));  // 0x673fd0; 0x7e43d8
				if (aFirstHalfPending) {
					// the old level's cube leaves to the right, down and away from the camera; the three products are
					// pushed z first (0x712808 to 0x712849), so 2.1 is x and -2.7 is z
					double aPathA = mTunnelCountdown.GetOutVal();
					double aPathB = mTunnelCountdown.GetOutVal();
					float aMoveZ = (float)(aPathA * -2.700000047683716);  // 0x7e43d0
					float aMoveY = (float)(aPathB * -1.2999999523162842);  // 0x7d7ea8
					float aMoveX = (float)(mTunnelCountdown.GetOutVal() * 2.0999999046325684);  // 0x7e32a8
					Translate(aCube, aMoveX, aMoveY, aMoveZ);  // 0x6dd0e2
				}
				aCube = Compose(aCube, aLook);  // 0x68fa70
				Translate(aCube, aPlaceX, aPlaceY, aPlaceZ);
				aListener.mWorld = aCube;
				if (aPass == 0) {
					aListener.SetCulling(true);
					aListener.SetDepth(true, true);
				} else {
					aListener.SetCulling(false);
					aListener.SetDepth(true, false);
				}
				if (aHasVertexShader) {
					// the vertex shader does the turning; each corner's alpha is scaled by the face's
					CubeVertex* aCorner = &aVertices[aFace * 4];
					int aLeft = 4;
					do {
						unsigned int aDiffuse = aCorner->mDiffuse;
						Color aCornerColor((int)((aDiffuse >> 16) & 0xff), (int)((aDiffuse >> 8) & 0xff),
							(int)(aDiffuse & 0xff), (int)(aDiffuse >> 24));  // 0x401880
						aCornerColor.mAlpha = Ftol((double)aCornerColor.mAlpha * (double)aFaceAlpha);
						aCorner->mDiffuse = aCornerColor.ToInt();
						aCorner++;
						aLeft--;
					} while (aLeft != 0);
					// the draw mode is the pass: 0 normal, 1 additive
					gD3DInterface->DrawPrimitiveEx(0x402d2, GPUPT_TRIANGLESTRIP, &aVertices[aFace * 4], 2, Color::White,
						aPass, 0.0f, 0.0f, true, &aPrimitiveListener);
				} else {
					// 0x712930 to 0x712e18: the corners turned by the cube's and the camera's matrices (0x44f4a0, 0x44f560)
					// and shaded here, a grey from the depth and a specular grey, for a card without vs_2_0
					NotPorted("DrawTunnel3D's CPU-transformed cube 0x712930 (a card without vs_2_0)");
				}
				gD3DInterface->SetVertexShader(nullptr);
				gD3DInterface->SetPixelShader(nullptr);
				aListener.SetInner(nullptr);  // 0x4bd6f0
			}
			aFace++;
		} while (aFace < 6);
		aPass++;
	} while (aPass < 2);
}

void Hyperspace::UpdateThrust()
{
	// 0x715c21's tail (0x7172be to 0x717dbe), in 2D and in 3D: the puffs step and the dead ones go...
	for (std::list<ThrustParticle>::iterator anItr = mParticles.begin(); anItr != mParticles.end();) {
		ThrustParticle& aParticle = *anItr;
		Vec3 aStep{aParticle.mVel.x + aParticle.mDir.x, aParticle.mVel.y + aParticle.mDir.y,
			aParticle.mVel.z + aParticle.mDir.z};  // 0x4c0760
		aParticle.mPos.x = aStep.x + aParticle.mPos.x;  // 0x68b2a8
		aParticle.mPos.y = aStep.y + aParticle.mPos.y;
		aParticle.mPos.z = aStep.z + aParticle.mPos.z;
		aParticle.mDir.x = aParticle.mDamp * aParticle.mDir.x;  // 0x4c0710
		aParticle.mDir.y = aParticle.mDir.y * aParticle.mDamp;
		aParticle.mDir.z = aParticle.mDamp * aParticle.mDir.z;
		float aSize = aParticle.mSizeRate + aParticle.mSize;
		aParticle.mSize = aSize;
		if (!(0.0f < aSize))
			aParticle.mDead = true;
		float anAlpha = aParticle.mAlphaRate + aParticle.mAlpha;
		aParticle.mAlpha = anAlpha;
		if (!(0.0f < anAlpha))
			aParticle.mDead = true;
		if (aParticle.mDead)
			anItr = mParticles.erase(anItr);
		else
			++anItr;
	}
	// ...a trail's particle is held no nearer than the drift of the engine's next newer one allows (the walk goes from
	// the newest to the oldest: 0x70fa7a steps to each node's prev)
	ThrustParticle* aLast[9] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
	for (std::list<ThrustParticle>::reverse_iterator anItr = mParticles.rbegin(); anItr != mParticles.rend(); ++anItr) {
		ThrustParticle* aParticle = &*anItr;
		if (aParticle->mType == 1) {
			ThrustParticle* aBefore = aLast[aParticle->mEngine];
			if (aBefore != nullptr) {
				float aReach = (float)(((double)aBefore->mDir.z + (double)aBefore->mVel.z) * 3.5);
				float aLimit = (float)((double)aBefore->mPos.z + (double)aReach);
				if (aParticle->mPos.z < aLimit)
					aParticle->mPos.z = aLimit;
			}
			aLast[aParticle->mEngine] = aParticle;
		}
	}
	// the nine engines' places and ways (on the ship, in its own space)
	static const float kPlace[9][3] = {
		{-3.577f, -0.607f, 7.89f}, {3.577f, -0.607f, 7.89f}, {-2.495f, -0.608f, 6.5f}, {2.495f, -0.608f, 6.5f},
		{-3.396f, 1.74f, -11.155f}, {3.396f, 1.74f, -11.155f}, {-2.359f, 1.765f, -12.489f}, {2.359f, 1.765f, -12.489f},
		{0.0f, -0.169f, -11.684f}};
	static const float kWay[9][3] = {
		{-0.5f, 0.0f, -0.8f}, {0.5f, 0.0f, -0.8f}, {-0.3f, 0.0f, -0.85f}, {0.3f, 0.0f, -0.85f},
		{-0.5f, 0.0f, -0.8f}, {0.5f, 0.0f, -0.8f}, {-0.3f, 0.0f, -0.85f}, {0.3f, 0.0f, -0.85f},
		{0.0f, 0.0f, -1.0f}};
	int anEngine = 0;
	do {
		const float* aBase = kPlace[anEngine];
		const float* aWay = kWay[anEngine];
		Vec3 aTenth{(float)((double)aWay[0] * 0.1), (float)((double)aWay[1] * 0.1), (float)((double)aWay[2] * 0.1)};
		Vec3 aNoDrift{(float)(mShipHeading.x * 0.0f), (float)(mShipHeading.y * 0.0f), (float)(mShipHeading.z * 0.0f)};
		// the glow, just behind the engine
		ThrustParticle aGlow;
		Vec3 aFront{(float)((double)aBase[0] + (double)aWay[0] * 0.5), (float)((double)aBase[1] + (double)aWay[1] * 0.5),
			(float)((double)aBase[2] + (double)aWay[2] * 0.5)};
		aGlow.mPos = TransformPoint(mShipPlace, aFront);
		Vec3 aTurned = VecTimesMat3(aTenth, mShipPlace.mRot);
		aGlow.mType = 0;
		aGlow.mDir = aTurned;
		aGlow.mVel = aNoDrift;
		aGlow.mDamp = 0.80000001192092896f;
		aGlow.mAlpha = (float)mEngineGlow.GetOutVal();
		aGlow.mAlphaRate = -1.0f;
		aGlow.mEngine = anEngine;
		aGlow.mSize = 0.06f;
		aGlow.mAngle = (float)((double)mUpdateCnt * 0.03);
		aGlow.mImage = IMAGE_SHIPTHRUSTGLOW;
		if (anEngine == 8) {
			// the middle engine's glow is two, bigger: one ahead of it, and a puff's picture behind
			aGlow.mPos.x = (float)((double)aTurned.x * 0.8) + aGlow.mPos.x;
			aGlow.mPos.y = (float)((double)aTurned.y * 0.8) + aGlow.mPos.y;
			aGlow.mPos.z = (float)((double)aTurned.z * 0.8) + aGlow.mPos.z;
			aGlow.mSize = 0.13f;
			mParticles.push_back(aGlow);  // 0x714ea9
			aGlow.mSize = 0.13f;
			aGlow.mImage = IMAGE_SHIPTHRUSTPARTICLE;
			aGlow.mPos.x = (float)((double)aTurned.x * -0.8) + aGlow.mPos.x;
			aGlow.mPos.y = (float)((double)aTurned.y * -0.8) + aGlow.mPos.y;
			aGlow.mPos.z = (float)((double)aTurned.z * -0.8) + aGlow.mPos.z;
		}
		mParticles.push_back(aGlow);
		// the trail
		ThrustParticle aTrail;
		Vec3 aNear{(float)((double)aBase[0] + (double)aWay[0] * 0.1), (float)((double)aBase[1] + (double)aWay[1] * 0.1),
			(float)((double)aBase[2] + (double)aWay[2] * 0.1)};
		aTrail.mPos = TransformPoint(mShipPlace, aNear);
		float aReach = (float)mEngineFire.GetOutVal();
		aTrail.mDir = VecTimesMat3(Vec3{aWay[0] * aReach, aWay[1] * aReach, aWay[2] * aReach}, mShipPlace.mRot);
		aTrail.mType = 1;
		aTrail.mVel = aNoDrift;
		aTrail.mDamp = 0.80000001192092896f;
		aTrail.mAlpha = (float)mEngineGlow.GetOutVal();
		aTrail.mAlphaRate = -0.05f;
		aTrail.mEngine = anEngine;
		aTrail.mSize = 0.03f;
		aTrail.mImage = IMAGE_SHIPTHRUSTPARTICLE;
		if (anEngine == 8) {
			aTrail.mSize = 0.1f;
			aTrail.mAlphaRate = -0.03f;
		}
		aTrail.mSizeRate = -0.002f;
		mParticles.push_back(aTrail);
		// the puffs: none to five of them (the draws in the original's order)
		int aPuff = 0;
		if (0 < Rand() % 6) {
			do {
				ThrustParticle aPuffParticle;
				float aSpreadX = RandSigned();
				float aSpreadY = RandSigned();
				float aSpreadZ = RandSigned();
				Vec3 aSpread{(float)((double)aSpreadX * 0.2), (float)((double)aSpreadY * 0.2), (float)((double)aSpreadZ * 0.2)};
				float anAlong = RandFloat();
				Vec3 aWhere;
				aWhere.x = (float)((double)aBase[0] + (double)(anAlong * aWay[0]) * 0.1) + aSpread.x;
				aWhere.y = (float)((double)aBase[1] + (double)(aWay[1] * anAlong) * 0.1) + aSpread.y;
				aWhere.z = (float)((double)aBase[2] + (double)(anAlong * aWay[2]) * 0.1) + aSpread.z;
				aPuffParticle.mPos = TransformPoint(mShipPlace, aWhere);
				float aPuffReach = (float)mEngineFire.GetOutVal();
				aPuffParticle.mDir = VecTimesMat3(Vec3{aWay[0] * aPuffReach, aPuffReach * aWay[1], aWay[2] * aPuffReach},
					mShipPlace.mRot);
				aPuffParticle.mType = 0;
				aPuffParticle.mVel = Vec3{(float)((double)mShipHeading.x * 0.9), (float)((double)mShipHeading.y * 0.9),
					(float)((double)mShipHeading.z * 0.9)};
				aPuffParticle.mDamp = 0.80000001192092896f;
				aPuffParticle.mAlpha = (float)(mEngineGlow.GetOutVal() * 1.2000000476837158);
				aPuffParticle.mAlphaRate = (float)((double)RandSigned() * 0.0099999997764825821 - 0.05000000074505806);
				aPuffParticle.mSize = 0.012f;
				aPuffParticle.mSizeRate = (float)((double)RandSigned() * 0.00030000001424923539 - 0.0010000000474974513);
				aPuffParticle.mEngine = anEngine;
				aPuffParticle.mImage = IMAGE_SHIPTHRUSTPARTICLE;
				mParticles.push_back(aPuffParticle);
				aPuff++;
			} while (aPuff < Rand() % 6);
		}
		anEngine++;
	} while (anEngine < 9);
}
