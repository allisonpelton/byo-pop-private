// Sexy::GPUMatrix and Sexy::GPUPolicy's presets (GPUInterface.h).
#include "GPUInterface.h"

#include <cstring>

using namespace Sexy;

GPUMatrix GPUMatrix::Identity()
{
	GPUMatrix aResult;
	std::memset(aResult.m, 0, sizeof(aResult.m));
	for (int i = 0; i < 4; i++)
		aResult.m[i][i] = 1.0f;
	return aResult;
}

GPUMatrix GPUMatrix::operator*(const GPUMatrix& theOther) const
{
	GPUMatrix aResult;
	for (int r = 0; r < 4; r++) {
		for (int c = 0; c < 4; c++) {
			float aSum = 0.0f;
			for (int k = 0; k < 4; k++)
				aSum += m[r][k] * theOther.m[k][c];
			aResult.m[r][c] = aSum;
		}
	}
	return aResult;
}

static GPUStateSet RenderState(int theType, unsigned long theValue)
{
	return GPUStateSet{GPUStateSet::RENDER, 0, theType, theValue};
}

static GPUStateSet StageState(int theStage, int theType, unsigned long theValue)
{
	return GPUStateSet{GPUStateSet::STAGE, theStage, theType, theValue};
}

static GPUStateSet SamplerState(int theStage, int theType, unsigned long theValue)
{
	return GPUStateSet{GPUStateSet::SAMPLER, theStage, theType, theValue};
}

// 0x61c070's choice of format, in its order (its palette branch is left out: no probe accepts P8)
GPUFormat GPUPolicy::TextureFormat(bool theHasAlpha, bool theHasTrans, unsigned long theFlags) const
{
	GPUFormat aFormat = GPUFMT_A8R8G8B8;
	if (!theHasAlpha && !theHasTrans) {
		if ((mAcceptedFormats & 4) == 0 || ((theFlags & 0x10) != 0 && !mDisplayIs565) || (theFlags & 8) != 0) {
			if (theFlags & 0x10)
				aFormat = GPUFMT_X8R8G8B8;
		} else {
			aFormat = GPUFMT_R5G6B5;
		}
	}
	if (aFormat == GPUFMT_A8R8G8B8) {
		if ((theFlags & 4) != 0 && (mAcceptedFormats & 2) != 0)
			aFormat = GPUFMT_A4R4G4B4;
		else if ((mAcceptedFormats & 1) == 0)
			aFormat = GPUFMT_A4R4G4B4;
	}
	return aFormat;
}

GPUPolicy GPUPolicy::Twist()
{
	GPUPolicy aPolicy;
	// PreDraw (0x61a7d0), in its order, with the app's debug render mode 0
	aPolicy.mSceneBegin = {
		RenderState(GPURS_ALPHABLENDENABLE, 1), RenderState(GPURS_SRCBLEND, 5), RenderState(GPURS_DESTBLEND, 6),
		RenderState(GPURS_LIGHTING, 0), RenderState(GPURS_ALPHATESTENABLE, 0), RenderState(GPURS_ALPHAREF, 0),
		RenderState(GPURS_ALPHAFUNC, 5),
		SamplerState(0, GPUSAMP_MINFILTER, 1), SamplerState(0, GPUSAMP_MAGFILTER, 1), SamplerState(0, GPUSAMP_MIPFILTER, 1),
		StageState(0, GPUTSS_ALPHAOP, 4),
		SamplerState(0, GPUSAMP_ADDRESSU, 3), SamplerState(0, GPUSAMP_ADDRESSV, 3),
		RenderState(GPURS_DITHERENABLE, 0), RenderState(GPURS_SPECULARENABLE, 0), RenderState(GPURS_ZWRITEENABLE, 0),
		RenderState(GPURS_ZENABLE, 0), RenderState(GPURS_CULLMODE, 1), RenderState(GPURS_FILLMODE, 3),
		StageState(0, GPUTSS_COLOROP, 4), StageState(0, GPUTSS_COLORARG1, 2), StageState(0, GPUTSS_COLORARG2, 0),
		StageState(0, GPUTSS_ALPHAOP, 4), StageState(0, GPUTSS_ALPHAARG1, 2), StageState(0, GPUTSS_ALPHAARG2, 0),
		StageState(1, GPUTSS_COLOROP, 1), StageState(1, GPUTSS_ALPHAOP, 1)};
	aPolicy.mAcceptedFormats = 1 | 2 | 4;  // 0x61a520's probe on a modern card: A8R8G8B8, A4R4G4B4, R5G6B5
	aPolicy.mDisplayIs565 = false;
	aPolicy.mPowerOfTwoPieces = false;
	aPolicy.mMinPieceSize = 8;
	aPolicy.mMaxPieceSize = 4096;
	aPolicy.mWidePieceLimit = 512;
	aPolicy.mMaxAspectRatio = 0;  // the caps give none on the cards the port runs on: no limit
	// The draws' offsets (0x61ca30, 0x620ee0, 0x61d700, 0x622140, 0x620cd0: GPUInterface.h), the same as SAF 1.3's;
	// mLineOffset is DrawLine's third vertex
	aPolicy.mBltOffset = -0.5f;
	aPolicy.mFillRectOffset = -0.5f;
	aPolicy.mTransformedOffset = -0.5f;
	aPolicy.mTransformedCentredOffset = 0.0f;
	aPolicy.mTrianglesOffset = 0.0f;
	aPolicy.mLineOffset = 0.5f;
	aPolicy.mDirectDrawCopies = false;
	aPolicy.mCaps = 0x7f;  // slot 0x04 (0x61eec0) with argument 0 on a modern card
	aPolicy.mMaxTextureBlendStages = 8;
	aPolicy.mValidateDevice = true;
	aPolicy.mCanStretchRectFromTextures = true;
	aPolicy.mDepthBits = 24;  // port: verify D16 or D24X8 in Twist's CreateDevice
	return aPolicy;
}

GPUPolicy GPUPolicy::SAF13()
{
	// SAF 1.3's D3DInterface on D3D7, as far as its released source shows; its scene-begin list and the rest are read
	// from the first such game's own export when it opts in
	GPUPolicy aPolicy;
	aPolicy.mAcceptedFormats = 1 | 2 | 4;
	aPolicy.mDisplayIs565 = false;
	aPolicy.mPowerOfTwoPieces = true;
	aPolicy.mMinPieceSize = 8;
	aPolicy.mMaxPieceSize = 1024;
	aPolicy.mWidePieceLimit = 0;
	aPolicy.mMaxAspectRatio = 0;
	aPolicy.mBltOffset = -0.5f;
	aPolicy.mFillRectOffset = -0.5f;
	aPolicy.mTransformedOffset = -0.5f;
	aPolicy.mTransformedCentredOffset = 0.0f;
	aPolicy.mTrianglesOffset = 0.0f;
	aPolicy.mLineOffset = 0.5f;
	aPolicy.mDirectDrawCopies = true;
	aPolicy.mCaps = 0;
	aPolicy.mMaxTextureBlendStages = 8;
	aPolicy.mValidateDevice = true;
	aPolicy.mCanStretchRectFromTextures = false;
	aPolicy.mDepthBits = 16;
	return aPolicy;
}
