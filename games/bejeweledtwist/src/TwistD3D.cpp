#include "TwistD3D.h"

#include "DDImage.h"
#include "DDInterface.h"
#include "GPU2D.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "PakInterface.h"
#include "SexyAppBase.h"
#include "SexyMatrix.h"
#include "TwistUtil.h"

#include <SDL.h>

#include <cstring>

using namespace Sexy;

TwistD3D* Sexy::gD3DInterface = nullptr;
TwistShader* Sexy::gTwistShaders[TWISTSHADER_COUNT] = {};

///////////////////////////////////////////////////////////////////////////////
// Matrices

GPUMatrix Sexy::WorldMatrixOf(const Transform3D& t)
{
	// 0x4c1940: each rotation row times its scale, the place in the last row
	GPUMatrix m;
	m.m[0][0] = t.mRot.m[0] * t.mScale.x;
	m.m[0][1] = t.mRot.m[1] * t.mScale.x;
	m.m[0][2] = t.mRot.m[2] * t.mScale.x;
	m.m[0][3] = 0.0f;
	m.m[1][0] = t.mRot.m[3] * t.mScale.y;
	m.m[1][1] = t.mRot.m[4] * t.mScale.y;
	m.m[1][2] = t.mRot.m[5] * t.mScale.y;
	m.m[1][3] = 0.0f;
	m.m[2][0] = t.mRot.m[6] * t.mScale.z;
	m.m[2][1] = t.mRot.m[7] * t.mScale.z;
	m.m[2][2] = t.mRot.m[8] * t.mScale.z;
	m.m[2][3] = 0.0f;
	m.m[3][0] = t.mPos.x;
	m.m[3][1] = t.mPos.y;
	m.m[3][2] = t.mPos.z;
	m.m[3][3] = 1.0f;
	return m;
}

GPUMatrix Sexy::ViewMatrixOf(const Transform3D& t)
{
	// 0x4c17e0: the rotation's transpose divided by the scale, and the place turned back, in the original's order
	float nx = -t.mPos.x, ny = -t.mPos.y, nz = -t.mPos.z;
	float sx = 1.0f / t.mScale.x, sy = 1.0f / t.mScale.y, sz = 1.0f / t.mScale.z;
	const float* r = t.mRot.m;
	GPUMatrix m;
	m.m[0][0] = sx * r[0];
	m.m[0][1] = sy * r[3];
	m.m[0][2] = sz * r[6];
	m.m[0][3] = 0.0f;
	m.m[1][0] = r[1] * sx;
	m.m[1][1] = r[4] * sy;
	m.m[1][2] = r[7] * sz;
	m.m[1][3] = 0.0f;
	m.m[2][0] = sx * r[2];
	m.m[2][1] = sy * r[5];
	m.m[2][2] = sz * r[8];
	m.m[2][3] = 0.0f;
	m.m[3][0] = ny * r[1] * sx + nx * sx * r[0] + nz * sx * r[2];
	m.m[3][1] = nz * sy * r[5] + ny * r[4] * sy + nx * sy * r[3];
	m.m[3][2] = nx * sz * r[6] + r[7] * sz * ny + sz * r[8] * nz;
	m.m[3][3] = 1.0f;
	return m;
}

GPUMatrix Sexy::ProjectionMatrixOf(const PerspectiveCamera& theCamera)
{
	// 0x6741c5
	GPUMatrix m;
	std::memset(&m, 0, sizeof(m));
	m.m[0][0] = theCamera.mScaleX;
	m.m[1][1] = theCamera.mScaleY;
	m.m[2][2] = theCamera.mDepthScale;
	m.m[2][3] = 1.0f;
	m.m[3][2] = theCamera.mDepthOffset;
	return m;
}

static GPUMatrix Transposed(const GPUMatrix& theMatrix)
{
	GPUMatrix m;
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			m.m[i][j] = theMatrix.m[j][i];
	return m;
}

// 0x622f90: a colour as D3DCOLORVALUE, each channel over 255 times theScale
static GPUColorValue ColorValueOf(const Color& theColor, float theScale)
{
	GPUColorValue v;
	v.r = (float)theColor.mRed / 255.0f * theScale;
	v.g = (float)theColor.mGreen / 255.0f * theScale;
	v.b = (float)theColor.mBlue / 255.0f * theScale;
	v.a = (float)theColor.mAlpha / 255.0f * theScale;
	return v;
}

///////////////////////////////////////////////////////////////////////////////
// D3DObject

D3DObject::~D3DObject()
{
	if (gD3DInterface == nullptr)
		return;
	for (D3DMesh& aMesh : mMeshes) {
		if (aMesh.mVertexBuffer != 0)
			gD3DInterface->mDevice->DeleteBuffer(aMesh.mVertexBuffer);
		if (aMesh.mIndexBuffer != 0)
			gD3DInterface->mDevice->DeleteBuffer(aMesh.mIndexBuffer);
	}
}

D3DObject* Sexy::LoadD3DObject(const std::string& theFileName, D3DObjectListener* theListener)
{
	D3DObject* anObject = new D3DObject();
	anObject->mListener = theListener;
	anObject->mFileName = theFileName;
	if (gD3DInterface != nullptr && !gD3DInterface->LoadObject(anObject)) {
		delete anObject;
		return nullptr;
	}
	return anObject;
}

///////////////////////////////////////////////////////////////////////////////
// WorldGraphicsListener

WorldGraphicsListener::WorldGraphicsListener(const PerspectiveCamera* theCamera, bool theClearDepth)
{
	mCamera = theCamera;
	if (theClearDepth)
		gD3DInterface->ClearDepth();
}

void WorldGraphicsListener::SetCulling(bool theCull)
{
	mCullMode = theCull ? 3 : 1;
}

void WorldGraphicsListener::SetDepth(bool theTest, bool theWrite)
{
	mZEnable = theTest || theWrite;
	mZWrite = theWrite;
	mZFunc = theTest ? 4 : 8;
}

void WorldGraphicsListener::Apply()
{
	gD3DInterface->SetRenderState(GPURS_ZWRITEENABLE, mZWrite);
	gD3DInterface->SetRenderState(GPURS_ZENABLE, mZEnable);
	gD3DInterface->SetRenderState(GPURS_ZFUNC, mZFunc);
	gD3DInterface->SetRenderState(GPURS_CULLMODE, mCullMode);
	GPUMatrix aWorld = WorldMatrixOf(mWorld);
	// The camera's transform with its z scale negated (the original's left-handed view)
	Transform3D aCamera = mCamera->mTransform;
	aCamera.mScale.z = -aCamera.mScale.z;
	GPUMatrix aView = ViewMatrixOf(aCamera);
	GPUMatrix aProjection = ProjectionMatrixOf(*mCamera);
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

void WorldGraphicsListener::Remove()
{
	if (mInner != nullptr)
		mInner->Remove();
	gD3DInterface->SetRenderState(GPURS_ZWRITEENABLE, 0);
	gD3DInterface->SetRenderState(GPURS_ZENABLE, 0);
	gD3DInterface->SetRenderState(GPURS_ZFUNC, 4);
	gD3DInterface->SetRenderState(GPURS_CULLMODE, 1);
	gD3DInterface->SetRenderState(GPURS_LIGHTING, 0);
}

///////////////////////////////////////////////////////////////////////////////
// TwistD3D

TwistD3D::TwistD3D(GPUInterface* theDevice, GPU2D* the2D)
{
	mDevice = theDevice;
	m2D = the2D;
	mWorld = mView = mProjection = GPUMatrix::Identity();
}

TwistD3D::~TwistD3D()
{
	// port: the original keeps its pooled targets to the end
	for (PooledTarget& aTarget : mTargets)
		delete aTarget.mImage;
	for (TwistShader*& aShader : gTwistShaders) {
		delete aShader;
		aShader = nullptr;
	}
}

void TwistD3D::MakeShaders()
{
	for (int i = 0; i < TWISTSHADER_COUNT; i++) {
		TwistShaderId anId = (TwistShaderId)i;
		TwistShader* aShader = new TwistShader();
		aShader->mName = TwistShaderName(anId);
		aShader->mIsVertex = i >= TWISTSHADER_CUBE_V;
		aShader->mHandle = aShader->mIsVertex ? mDevice->CreateVertexShader(aShader->mName, TwistShaderSource(anId))
			: mDevice->CreatePixelShader(aShader->mName, TwistShaderSource(anId));
		gTwistShaders[i] = aShader;
	}
}

unsigned long TwistD3D::GetCaps(int theWhich)
{
	return mDevice->GetCaps(theWhich);
}

void TwistD3D::PushTransform(const SexyMatrix3& theTransform, bool theConcatenate)
{
	m2D->PushTransform(theTransform, theConcatenate);
}

void TwistD3D::PopTransform()
{
	m2D->PopTransform();
}

namespace {

// The .p3d reader (0x4614e0 strings, 0x461470 u16, 0x4614a0 u32, 0x461790 bytes): little-endian, nothing compressed
struct P3DReader {
	std::vector<unsigned char> mData;
	size_t mPos = 0;
	bool mFailed = false;

	bool Bytes(void* theDest, size_t theCount)
	{
		if (mPos + theCount > mData.size()) {
			mFailed = true;
			return false;
		}
		std::memcpy(theDest, mData.data() + mPos, theCount);
		mPos += theCount;
		return true;
	}
	int U16()
	{
		unsigned short v = 0;
		Bytes(&v, 2);
		return (short)v;
	}
	unsigned long U32()
	{
		uint32_t v = 0;
		Bytes(&v, 4);
		return v;
	}
	unsigned char U8()
	{
		unsigned char v = 0;
		Bytes(&v, 1);
		return v;
	}
	std::string String()
	{
		int aLength = U16();
		if (aLength <= 0)
			return std::string();
		std::string s((size_t)aLength, '\0');
		Bytes(&s[0], (size_t)aLength);
		return s;
	}
};

}  // namespace

bool TwistD3D::LoadObject(D3DObject* theObject)
{
	// 0x61fd00
	PFILE* aFile = p_fopen(theObject->mFileName.c_str(), "rb");
	if (aFile == nullptr)
		return false;
	P3DReader r;
	p_fseek(aFile, 0, SEEK_END);
	long aSize = p_ftell(aFile);
	p_fseek(aFile, 0, SEEK_SET);
	if (aSize > 0) {
		r.mData.resize((size_t)aSize);
		p_fread(r.mData.data(), 1, (size_t)aSize, aFile);
	}
	p_fclose(aFile);
	if (r.U32() != 0x3dbeef00)
		return false;
	unsigned long aVersion = r.U32();
	theObject->mMeshes.clear();
	D3DObjectListener* aListener = theObject->mListener;
	if (aListener != nullptr)
		aListener->LoadBegin(theObject);
	std::string aDir = GetFileDir(theObject->mFileName, true);
	int aFrameCount = r.U16();
	for (int f = 0; f < aFrameCount && !r.mFailed; f++) {
		std::string aFrameName = r.String();
		int aMeshCount = r.U16();
		for (int i = 0; i < aMeshCount && !r.mFailed; i++) {
			if (aVersion >= 2 && r.U8() == 0)
				continue;
			theObject->mMeshes.push_back(D3DMesh());
			D3DMesh& aMesh = theObject->mMeshes.back();
			aMesh.mFrameName = aFrameName;
			aMesh.mName = r.String();
			int aPropertyCount = r.U16();
			for (int k = 0; k < aPropertyCount && !r.mFailed; k++) {
				std::string aKey = r.String();
				std::string aValue = r.String();
				aMesh.mProperties[aKey] = aValue;
				if (aListener != nullptr)
					aListener->LoadProperty(theObject, aFrameName, aMesh.mName, aKey, aValue);
			}
			// Each texture through the listener first, else from the .p3d's folder; one that cannot be had ends the
			// load there, the meshes read so far kept
			static const char* const kKeys[2] = {"texture0.fileName", "bump.fileName"};
			bool aMissing = false;
			for (int t = 0; t < 2; t++) {
				std::map<std::string, std::string>::iterator anItr = aMesh.mProperties.find(kKeys[t]);
				if (anItr == aMesh.mProperties.end() || anItr->second.empty())
					continue;
				SharedImageRef anImage;
				if (aListener != nullptr)
					anImage = aListener->LoadTexture(theObject, aFrameName, aMesh.mName, kKeys[t], anItr->second);
				if ((Image*)anImage == nullptr)
					anImage = gSexyAppBase->GetSharedImage(aDir + anItr->second, "", nullptr);
				MemoryImage* aMemoryImage = (MemoryImage*)anImage;
				if (aMemoryImage == nullptr) {
					aMissing = true;
					break;
				}
				aMemoryImage->mD3DFlags |= (t == 0) ? 0x81 : 0x01;
				(t == 0 ? aMesh.mTexture : aMesh.mBump) = anImage;
			}
			if (aMissing) {
				SDL_Log("TwistD3D: %s: a texture of %s is missing", theObject->mFileName.c_str(), aMesh.mName.c_str());
				return true;
			}
			r.U16();
			aMesh.mFVF = r.U32();
			aMesh.mStride = 0x24;
			aMesh.mVertexCount = r.U16();
			aMesh.mVertices.resize((size_t)aMesh.mVertexCount * 0x24);
			if (aMesh.mVertexCount > 0)
				r.Bytes(aMesh.mVertices.data(), aMesh.mVertices.size());
			aMesh.mIndexCount = r.U16() * 3;
			aMesh.mIndices.resize((size_t)aMesh.mIndexCount);
			if (aMesh.mIndexCount > 0)
				r.Bytes(aMesh.mIndices.data(), aMesh.mIndices.size() * 2);
		}
	}
	return true;
}

bool TwistD3D::PrepareImage(Image* theImage)
{
	return mDevice->PrepareTexture(theImage) != nullptr;
}

void TwistD3D::FillRect(const Rect& theRect, const Color& theColor, int theDrawMode)
{
	m2D->FillRect(theRect, theColor, theDrawMode);
}

void TwistD3D::DrawTrianglesTex(Image* theTexture, const TriVertex theVertices[][3], int theNumTriangles,
	const Color& theColor, int theDrawMode, float theTX, float theTY, bool theBlend)
{
	m2D->DrawTrianglesTex(theTexture, theVertices, theNumTriangles, theColor, theDrawMode, theTX, theTY, theBlend);
}

void TwistD3D::DrawTrianglesTexStrip(Image* theTexture, const TriVertex theVertices[], int theNumTriangles,
	const Color& theColor, int theDrawMode, float theTX, float theTY, bool theBlend)
{
	m2D->DrawTrianglesTexStrip(theTexture, theVertices, theNumTriangles, theColor, theDrawMode, theTX, theTY, theBlend);
}

// A vertex's size for an FVF (D3D9's rule for the parts Twist uses)
static int StrideOf(unsigned long theFVF)
{
	int aSize = 0;
	if ((theFVF & 0xe) == GPUFVF_XYZ)
		aSize += 12;
	else if ((theFVF & 0xe) == GPUFVF_XYZRHW)
		aSize += 16;
	if (theFVF & GPUFVF_NORMAL)
		aSize += 12;
	if (theFVF & GPUFVF_DIFFUSE)
		aSize += 4;
	if (theFVF & GPUFVF_SPECULAR)
		aSize += 4;
	// Each set's floats by its D3DFVF_TEXCOORDSIZE bits (bits 16 + 2i: 0 two, 1 three, 2 four, 3 one)
	static const int kTexFloats[4] = {2, 3, 4, 1};
	int aNumTex = (int)((theFVF >> 8) & 0xf);
	for (int i = 0; i < aNumTex; i++)
		aSize += 4 * kTexFloats[(theFVF >> (16 + 2 * i)) & 3];
	return aSize;
}

void TwistD3D::DrawPrimitiveEx(unsigned long theFVF, GPUPrimitiveType theType, const void* theVertices,
	int thePrimitiveCount, const Color& theColor, int theDrawMode, float theTX, float theTY, bool theBlend,
	D3DDrawPrimitiveListener* theListener)
{
	int aVertexCount;
	switch ((int)theType) {
	case 1: aVertexCount = thePrimitiveCount; break;
	case 2: aVertexCount = thePrimitiveCount * 2; break;
	case 3: aVertexCount = thePrimitiveCount + 1; break;
	case 4: aVertexCount = thePrimitiveCount * 3; break;
	case 5:
	case 6: aVertexCount = thePrimitiveCount + 2; break;
	default: return;
	}
	if (aVertexCount == 0 || thePrimitiveCount == 0 || !m2D->PreDraw())
		return;
	m2D->Flush();
	// The draw's colour as ARGB from each channel's low byte (0x62237a)
	unsigned long aColor = ((unsigned long)theColor.mAlpha << 24) | ((unsigned long)(theColor.mRed & 0xff) << 16) |
		((unsigned long)(theColor.mGreen & 0xff) << 8) | (unsigned long)(theColor.mBlue & 0xff);
	m2D->ApplyDrawMode(theDrawMode);
	m2D->SetLinearFilter(theBlend);
	if (theListener != nullptr)
		theListener->Before();
	int aStride = StrideOf(theFVF);
	const void* aVertices = theVertices;
	std::vector<unsigned char> aCopy;
	const SexyMatrix3* aTransform = m2D->GetTransform();
	if ((theFVF & GPUFVF_XYZRHW) != 0 && (aColor != 0 || theTX != 0.0f || theTY != 0.0f || aTransform != nullptr)) {
		// Pre-transformed vertices are copied and moved; one whose diffuse (+0x10) is 0 takes the draw's colour
		aCopy.assign((const unsigned char*)theVertices, (const unsigned char*)theVertices + (size_t)aStride * aVertexCount);
		for (int i = 0; i < aVertexCount; i++) {
			unsigned char* aVertex = aCopy.data() + (size_t)aStride * i;
			float aXY[2];
			std::memcpy(aXY, aVertex, 8);
			aXY[0] = aXY[0] + theTX;
			aXY[1] = aXY[1] + theTY;
			uint32_t aDiffuse;
			std::memcpy(&aDiffuse, aVertex + 0x10, 4);
			if (aDiffuse == 0) {
				aDiffuse = (uint32_t)aColor;
				std::memcpy(aVertex + 0x10, &aDiffuse, 4);
			}
			if (aTransform != nullptr) {
				SexyVector2 aPoint = (*aTransform) * SexyVector2(aXY[0], aXY[1]);
				aXY[0] = aPoint.x;
				aXY[1] = aPoint.y;
			}
			std::memcpy(aVertex, aXY, 8);
		}
		aVertices = aCopy.data();
	}
	if (mDevice->GetVertexShader() != 0)
		SetVertexShaderMatrices();
	mDevice->DrawPrimitiveUP(theType, thePrimitiveCount, theFVF, aVertices, aStride);
	if (theListener != nullptr)
		theListener->After();
}

bool TwistD3D::SetRenderTarget(Image* theImage)
{
	return m2D->SetRenderTarget(theImage);
}

void TwistD3D::Clear(int theRed, int theGreen, int theBlue, int theAlpha)
{
	// 0x621a30: colour only, and without flushing the batch (quads already batched draw over the clear, as in the
	// original)
	unsigned long aColor = ((((unsigned long)(theRed & 0xff) | ((unsigned long)theAlpha << 8)) << 8 |
		(unsigned long)(theGreen & 0xff)) << 8) | (unsigned long)(theBlue & 0xff);
	mDevice->Clear(1, aColor, 1.0f);
}

Image* TwistD3D::SwapScreen(DDImage** theImage, bool* theSwapped)
{
	// 0x621ae0
	if (*theImage == nullptr) {
		DDImage* anImage = new DDImage(gSexyAppBase->mDDInterface);
		*theImage = anImage;
		anImage->mD3DFlags = 0x10;
		anImage->Create(gSexyAppBase->mDDInterface->mWidth, gSexyAppBase->mDDInterface->mHeight);
		anImage->SetImageMode(false, false);
		*theSwapped = false;
	} else if (*theSwapped) {
		// The original makes its saved surface the target (slot 0x94), then swaps; what it saves after the swap is the
		// screen's surface again, never null
		m2D->Flush();
		m2D->SetRenderTarget(nullptr);
		mDevice->SwapScreen(*theImage);
		return *theImage;
	}
	// The image becomes the target and is cleared to black, then the screen's buffer and the image's change places;
	// the screen is the target again (+0xb8). port: the screen is made the target before the swap, so the device never
	// takes the image as a texture while it is bound
	m2D->SetRenderTarget(*theImage);
	mDevice->Clear(1, 0xff000000, 1.0f);
	m2D->Flush();
	m2D->SetRenderTarget(nullptr);
	mDevice->SwapScreen(*theImage);
	*theSwapped = true;
	return *theImage;
}

DDImage* TwistD3D::HoldTarget(TargetHold* theHold, int theWidth, int theHeight, int theFlags, const char* theName)
{
	// 0x4f6630
	ReleaseTarget(theHold);
	theFlags |= 0x10;
	// 0x4f6830: the first free target of this size and flags, else a new one
	int aCount = (int)mTargets.size();
	for (int i = 0; i < aCount; i++) {
		PooledTarget& aTarget = mTargets[i];
		if (aTarget.mHold == nullptr && aTarget.mImage->mWidth == theWidth && aTarget.mImage->mHeight == theHeight &&
			aTarget.mImage->mD3DFlags == theFlags) {
			theHold->mImage = aTarget.mImage;
			theHold->mSwapped = aTarget.mSwapped;
			theHold->mIndex = i + 1;
			aTarget.mHold = theHold;
			aTarget.mName = theName != nullptr ? theName : "NULL";
			return theHold->mImage;
		}
	}
	PooledTarget aTarget;
	aTarget.mImage = new DDImage(gSexyAppBase->mDDInterface);
	aTarget.mImage->mD3DFlags = theFlags;
	aTarget.mImage->Create(theWidth, theHeight);
	aTarget.mImage->SetImageMode(false, false);
	PrepareImage(aTarget.mImage);  // slot 0x44
	aTarget.mHold = theHold;
	aTarget.mName = theName != nullptr ? theName : "NULL";
	mTargets.push_back(aTarget);
	theHold->mImage = aTarget.mImage;
	theHold->mSwapped = false;
	theHold->mIndex = aCount + 1;
	return theHold->mImage;
}

bool TwistD3D::ReleaseTarget(TargetHold* theHold)
{
	// 0x4f66e0, 0x4f6b00
	if (theHold->mIndex == 0)
		return false;
	PooledTarget& aTarget = mTargets[theHold->mIndex - 1];
	theHold->mImage = nullptr;
	theHold->mSwapped = false;
	theHold->mIndex = 0;
	aTarget.mHold = nullptr;
	aTarget.mName.clear();
	return true;
}

DDImage* TwistD3D::HoldScreen(TargetHold* theHold, const char* theName)
{
	// 0x4f6670
	DDInterface* anInterface = gSexyAppBase->mDDInterface;
	if (HoldTarget(theHold, anInterface->mWidth, anInterface->mHeight, 0, theName) == nullptr)
		return nullptr;
	SwapScreen(&theHold->mImage, &theHold->mSwapped);  // slot 0xa0
	// 0x4f6ac0: the entry keeps the saved surface
	mTargets[theHold->mIndex - 1].mSwapped = theHold->mSwapped;
	return theHold->mImage;
}

void TwistD3D::SetTextureStageState(int theStage, int theType, unsigned long theValue)
{
	m2D->SetTextureStageState(theStage, theType, theValue);
}

void TwistD3D::SetRenderState(int theState, unsigned long theValue)
{
	m2D->SetRenderState(theState, theValue);
}

void TwistD3D::SetSamplerState(int theStage, int theType, unsigned long theValue)
{
	m2D->SetSamplerState(theStage, theType, theValue);
}

void TwistD3D::SetTextureLinearFilter(int theStage, bool theLinear)
{
	// 0x622b70: only the cached MINFILTER is compared; then MIN, MAG and MIP through the sampler slot
	unsigned long aFilter = theLinear ? 2 : 1;
	if (m2D->GetSamplerState(theStage, GPUSAMP_MINFILTER) == aFilter)
		return;
	m2D->Flush();
	m2D->SetSamplerState(theStage, GPUSAMP_MINFILTER, aFilter);
	m2D->SetSamplerState(theStage, GPUSAMP_MAGFILTER, aFilter);
	m2D->SetSamplerState(theStage, GPUSAMP_MIPFILTER, aFilter);
}

void TwistD3D::SetTextureWrap(int theStage, bool theWrap)
{
	// 0x622c10: WRAP (1) or CLAMP (3) on u and v
	unsigned long anAddress = theWrap ? 1 : 3;
	m2D->SetSamplerState(theStage, GPUSAMP_ADDRESSU, anAddress);
	m2D->SetSamplerState(theStage, GPUSAMP_ADDRESSV, anAddress);
}

void TwistD3D::SetTexture(int theStage, Image* theImage)
{
	m2D->SetTexture(theStage, theImage);
}

void TwistD3D::SetPixelShader(TwistShader* theShader)
{
	// 0x622dd0: the bound shader is the device's (a scene's end unbinds both); PreDraw, then the change
	GPUPixelShader aHandle = (theShader != nullptr) ? theShader->mHandle : 0;
	if (mDevice->GetPixelShader() == aHandle)
		return;
	m2D->PreDraw();
	m2D->SetPixelShader(aHandle);
}

void TwistD3D::SetVertexShader(TwistShader* theShader)
{
	// 0x622ea0
	GPUVertexShader aHandle = (theShader != nullptr) ? theShader->mHandle : 0;
	if (mDevice->GetVertexShader() == aHandle)
		return;
	m2D->PreDraw();
	m2D->SetVertexShader(aHandle);
}

void TwistD3D::SetTransform(int theType, const GPUMatrix& theMatrix)
{
	if (theType == GPUTS_WORLD)
		mWorld = theMatrix;
	else if (theType == GPUTS_VIEW)
		mView = theMatrix;
	else if (theType == GPUTS_PROJECTION)
		mProjection = theMatrix;
	mDevice->SetTransform(theType, theMatrix);
}

void TwistD3D::LightEnable(int theIndex, bool theEnable)
{
	mDevice->LightEnable(theIndex, theEnable);
}

void TwistD3D::SetPointLight(int theIndex, const Vec3& thePosition, const D3DLightColors& theColors, float theRange,
	const Vec3& theAttenuation)
{
	// 0x622ff0
	GPULight aLight;
	std::memset(&aLight, 0, sizeof(aLight));
	aLight.mType = 1;
	aLight.mDiffuse = ColorValueOf(theColors.mDiffuse, theColors.mIntensity);
	aLight.mSpecular = ColorValueOf(theColors.mSpecular, theColors.mIntensity);
	aLight.mAmbient = ColorValueOf(theColors.mAmbient, theColors.mIntensity);
	aLight.mPosition = GPUVector3{thePosition.x, thePosition.y, thePosition.z};
	aLight.mRange = theRange;
	aLight.mFalloff = 1.0f;
	aLight.mAttenuation0 = theAttenuation.x;
	aLight.mAttenuation1 = theAttenuation.y;
	aLight.mAttenuation2 = theAttenuation.z;
	aLight.mTheta = 0.0f;
	aLight.mPhi = 3.1415927f;
	mDevice->SetLight(theIndex, aLight);
}

void TwistD3D::SetDirectionalLight(int theIndex, const Vec3& theDirection, const D3DLightColors& theColors)
{
	// 0x623100 (its range, which Direct3D ignores for a directional light, is not carried)
	GPULight aLight;
	std::memset(&aLight, 0, sizeof(aLight));
	aLight.mType = 3;
	aLight.mDiffuse = ColorValueOf(theColors.mDiffuse, theColors.mIntensity);
	aLight.mSpecular = ColorValueOf(theColors.mSpecular, theColors.mIntensity);
	aLight.mAmbient = ColorValueOf(theColors.mAmbient, theColors.mIntensity);
	aLight.mDirection = GPUVector3{theDirection.x, theDirection.y, theDirection.z};
	aLight.mFalloff = 1.0f;
	aLight.mAttenuation1 = 1.0f;
	aLight.mPhi = 3.1415927f;
	mDevice->SetLight(theIndex, aLight);
}

void TwistD3D::SetMaterial(const Color& theDiffuse, const Color& theSpecular, const Color& theAmbient,
	const Color& theEmissive, float thePower)
{
	// 0x623220
	GPUMaterial aMaterial;
	aMaterial.mDiffuse = ColorValueOf(theDiffuse, 1.0f);
	aMaterial.mSpecular = ColorValueOf(theSpecular, 1.0f);
	aMaterial.mAmbient = ColorValueOf(theAmbient, 1.0f);
	aMaterial.mEmissive = ColorValueOf(theEmissive, 1.0f);
	aMaterial.mPower = thePower;
	mDevice->SetMaterial(aMaterial);
}

void TwistD3D::SetShaderConstant(TwistShader* theShader, int theRegister, const float theValue[4])
{
	if (theShader == nullptr)
		return;
	if (!theShader->mIsVertex && theShader->mHandle == mDevice->GetPixelShader())
		mDevice->SetPixelShaderConstantF(theRegister, theValue, 1);
	else if (theShader->mIsVertex && theShader->mHandle == mDevice->GetVertexShader())
		mDevice->SetVertexShaderConstantF(theRegister, theValue, 1);
}

void TwistD3D::SetVertexShaderMatrices()
{
	// 0x623fd0: the transposed world-view at c4 and world-view-projection at c8 (the constant tables' matWorldView and
	// matWorldViewProj). Its c0-c3 are left out: both vertex shaders define their own c0 and c1 and read none of c0-c3.
	GPUMatrix aWorldView = mWorld * mView;
	GPUMatrix aWorldViewProj = aWorldView * mProjection;
	GPUMatrix aWV = Transposed(aWorldView);
	GPUMatrix aWVP = Transposed(aWorldViewProj);
	mDevice->SetVertexShaderConstantF(4, &aWV.m[0][0], 4);
	mDevice->SetVertexShaderConstantF(8, &aWVP.m[0][0], 4);
}

void TwistD3D::DrawObject(D3DObject* theObject, const GPUMatrix& theWorld, const Color& theColor, bool the2DStates)
{
	// 0x6242f0
	m2D->Flush();
	if (the2DStates)
		NotPorted("TwistD3D::DrawObject: an object with its own states (0x6242f0's flag)");
	GPUMatrix aSavedWorld = mWorld;
	SetTransform(GPUTS_WORLD, theWorld);
	if (mDevice->GetVertexShader() != 0)
		SetVertexShaderMatrices();
	DrawMeshes(theObject);
	SetTransform(GPUTS_WORLD, aSavedWorld);
	(void)theColor;
}

void TwistD3D::DrawMeshes(D3DObject* theObject)
{
	// 0x623c40
	D3DObjectListener* aListener = theObject->mListener;
	if (aListener != nullptr)
		aListener->BeginObject(theObject);
	for (D3DMesh& aMesh : theObject->mMeshes) {
		if (aMesh.mVertexBuffer == 0 && !aMesh.mVertices.empty()) {
			aMesh.mVertexBuffer = mDevice->CreateVertexBuffer(aMesh.mFVF, aMesh.mVertices.data(), aMesh.mVertexCount,
				aMesh.mStride);
			aMesh.mVertices.clear();
		}
		if (aMesh.mIndexBuffer == 0 && !aMesh.mIndices.empty()) {
			aMesh.mIndexBuffer = mDevice->CreateIndexBuffer(aMesh.mIndices.data(), aMesh.mIndexCount);
			aMesh.mIndices.clear();
		}
		SetTexture(0, (Image*)aMesh.mTexture);
		if ((Image*)aMesh.mBump != nullptr)
			SetTexture(1, (Image*)aMesh.mBump);
		if (aListener != nullptr)
			aListener->BeginMesh(aMesh, (Image*)aMesh.mBump != nullptr);
		mDevice->DrawIndexedPrimitive(GPUPT_TRIANGLELIST, aMesh.mVertexBuffer, aMesh.mIndexBuffer, 0,
			aMesh.mIndexCount / 3);
		if (aListener != nullptr)
			aListener->EndMesh(theObject, aMesh);
	}
	if (aListener != nullptr)
		aListener->EndObject(theObject);
}

void TwistD3D::SetDepthTestMode(int theMode)
{
	m2D->SetDepthTestMode(theMode);
}

void TwistD3D::SetDepthWriteMode(int theMode)
{
	m2D->SetDepthWriteMode(theMode);
}

void TwistD3D::SetAlphaRef(int theRef)
{
	m2D->SetAlphaRef(theRef);
}

void TwistD3D::SetDepthZ(float theWriteZ, float theOtherZ)
{
	m2D->SetDepthZ(theWriteZ, theOtherZ);
}

void TwistD3D::ClearDepth()
{
	// 0x624cd0
	m2D->Flush();
	mDevice->Clear(2, 0xff000000, 1.0f);
	mDepthClearPending = false;
}
