// Sexy::GPU2D (GPU2D.h): the original's 2D layer above the device, as Bejeweled Twist's D3D9Interface does it.
#include "GPU2D.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "TriVertex.h"

#include <SDL.h>
#include <math.h>

using namespace Sexy;

static const unsigned long FVF_2D = GPUFVF_XYZRHW | GPUFVF_DIFFUSE | GPUFVF_SPECULAR | GPUFVF_TEX1;

enum { STATE_RENDER = 0, STATE_STAGE = 1, STATE_SAMPLER = 2 };

static long long StateKey(int theKind, int theStage, int theType)
{
	return ((long long) theKind << 40) | ((long long) theStage << 32) | (unsigned int) theType;
}

// A Color as the builders pack it: the low byte of each field, so out-of-range values wrap
static unsigned long PackColor(const Color& theColor)
{
	return ((unsigned long) (theColor.mAlpha & 0xff) << 24) | ((unsigned long) (theColor.mRed & 0xff) << 16) |
		((unsigned long) (theColor.mGreen & 0xff) << 8) | (unsigned long) (theColor.mBlue & 0xff);
}

static GPU2DVertex MakeVertex(float theX, float theY, float theZ, unsigned long theColor, float theU, float theV)
{
	GPU2DVertex aVertex = { theX, theY, theZ, 1.0f, theColor, 0, theU, theV };
	return aVertex;
}

GPU2D::GPU2D(GPUInterface* theDevice, const GPUPolicy& thePolicy)
{
	mDevice = theDevice;
	mPolicy = thePolicy;
	// the constructor 0x618da0
	mDepthTestMode = 0;
	mDepthWriteMode = 0;
	mAlphaRef = 0;
	mDepthZ[0] = 0.25f;
	mDepthZ[1] = 0.5f;
	mBatch.reserve(BATCH_SIZE);
	mDrawCalls = 0;
	mPrimitives = 0;
	mFilter = 0;
	mSceneCount = -1;
	mScreenWidth = 0;
	mScreenHeight = 0;
	for (int i = 0; i < STAGES; i++)
	{
		mTextureImages[i] = NULL;
		mTexturePieces[i] = 0;
	}
}

GPU2D::~GPU2D()
{
}

///////////////////////////////////////////////////////////////////////////////
// State

unsigned long GPU2D::GetSamplerState(int theStage, int theType) const
{
	std::map<long long, unsigned long>::const_iterator anItr = mStates.find(StateKey(STATE_SAMPLER, theStage, theType));
	return (anItr != mStates.end()) ? anItr->second : 0xffffffff;
}

void GPU2D::SetCachedState(int theKind, int theStage, int theType, unsigned long theValue)
{
	// 0x623380: a state never set reads as 0xffffffff
	long long aKey = StateKey(theKind, theStage, theType);
	std::map<long long, unsigned long>::iterator anItr = mStates.find(aKey);
	unsigned long aCurrent = (anItr != mStates.end()) ? anItr->second : 0xffffffff;
	if (aCurrent == theValue)
		return;

	Flush();
	if (theKind == STATE_RENDER)
		mDevice->SetRenderState(theType, theValue);
	else if (theKind == STATE_STAGE)
		mDevice->SetTextureStageState(theStage, theType, theValue);
	else
		mDevice->SetSamplerState(theStage, theType, theValue);
	mStates[aKey] = theValue;
}

void GPU2D::SetRenderState(int theState, unsigned long theValue)
{
	SetCachedState(STATE_RENDER, 0, theState, theValue);
}

void GPU2D::SetTextureStageState(int theStage, int theType, unsigned long theValue)
{
	SetCachedState(STATE_STAGE, theStage, theType, theValue);
}

void GPU2D::SetSamplerState(int theStage, int theType, unsigned long theValue)
{
	SetCachedState(STATE_SAMPLER, theStage, theType, theValue);
}

void GPU2D::SetTexturePiece(int theStage, Image* theImage, int thePiece)
{
	// 0x622c70: the cache at this+0x158
	if ((theStage < 0) || (theStage >= STAGES))
		return;
	if (theImage == NULL)
		thePiece = 0;
	if ((mTextureImages[theStage] == theImage) && (mTexturePieces[theStage] == thePiece))
		return;

	Flush();
	mDevice->SetTexturePiece(theStage, theImage, thePiece);
	mTextureImages[theStage] = theImage;
	mTexturePieces[theStage] = thePiece;
}

void GPU2D::SetTexture(int theStage, Image* theImage)
{
	// 0x622cc0
	if (theImage == NULL)
	{
		SetTexturePiece(theStage, NULL, 0);
		return;
	}
	MemoryImage* aMemoryImage = dynamic_cast<MemoryImage*>(theImage);
	if ((aMemoryImage != NULL) && ((aMemoryImage->mD3DFlags & 0x40) != 0))
	{
		// a cube texture (flag 0x20) is prepared and bound like any image; the device holds it as a cube
		SDL_Log("GPU2D: a volume texture is bound (nothing bound)");
		return;
	}
	if (mDevice->PrepareTexture(theImage) != NULL)
		SetTexturePiece(theStage, theImage, 0);
}

bool GPU2D::SetRenderTarget(Image* theImage)
{
	if (mDevice->GetRenderTarget() != theImage)
		Flush();
	return mDevice->SetRenderTarget(theImage);
}

void GPU2D::SetVertexShader(GPUVertexShader theShader)
{
	if (mDevice->GetVertexShader() == theShader)
		return;
	Flush();
	mDevice->SetVertexShader(theShader);
}

void GPU2D::SetPixelShader(GPUPixelShader theShader)
{
	if (mDevice->GetPixelShader() == theShader)
		return;
	Flush();
	mDevice->SetPixelShader(theShader);
}

void GPU2D::ImageDeleted(Image* theImage)
{
	for (int i = 0; i < STAGES; i++)
	{
		if (mTextureImages[i] == theImage)
		{
			mTextureImages[i] = NULL;
			mTexturePieces[i] = 0;
		}
	}
}

bool GPU2D::PreDraw()
{
	if (!mDevice->BeginSceneIfNeeded())
		return false;

	int aSceneCount = mDevice->GetSceneCount();
	if (aSceneCount != mSceneCount)
	{
		// the device applied the policy's list at the scene's start: the cache follows it; 0x61a7d0 sets the filter
		// cache to 0 (POINT) without setting the filter
		mSceneCount = aSceneCount;
		mFilter = 0;
		for (size_t i = 0; i < mPolicy.mSceneBegin.size(); i++)
		{
			const GPUStateSet& aSet = mPolicy.mSceneBegin[i];
			int aKind = (aSet.mKind == GPUStateSet::RENDER) ? STATE_RENDER : (aSet.mKind == GPUStateSet::STAGE) ? STATE_STAGE : STATE_SAMPLER;
			mStates[StateKey(aKind, aSet.mStage, aSet.mType)] = aSet.mValue;
		}
	}
	return true;
}

void GPU2D::EndScene()
{
	Flush();
	mDevice->EndScene();
}

///////////////////////////////////////////////////////////////////////////////
// The draw-mode layer (slots 0x114-0x13c)

void GPU2D::ApplyDepthStates()
{
	if ((mDepthWriteMode == 0) && (mDepthTestMode == 0))
	{
		SetRenderState(GPURS_ALPHATESTENABLE, 0);
		SetRenderState(GPURS_ALPHAREF, 0);
		SetRenderState(GPURS_ZENABLE, 0);
		SetRenderState(GPURS_ZWRITEENABLE, 0);
		SetRenderState(GPURS_ZFUNC, 8);  // ALWAYS
		return;
	}

	SetRenderState(GPURS_ALPHATESTENABLE, 1);
	SetRenderState(GPURS_ZWRITEENABLE, mDepthWriteMode != 0);
	unsigned long aFunc;
	if (mDepthTestMode == 0)
	{
		SetRenderState(GPURS_ZENABLE, mDepthWriteMode != 0);
		aFunc = 8;  // ALWAYS
	}
	else
	{
		SetRenderState(GPURS_ZENABLE, 1);
		aFunc = (mDepthTestMode == 2) ? 4 : 7;  // LESSEQUAL, GREATEREQUAL
	}
	SetRenderState(GPURS_ZFUNC, aFunc);
	SetRenderState(GPURS_ALPHAREF, (mDepthWriteMode != 0) ? mAlphaRef : 0);
}

void GPU2D::SetDepthTestMode(int theMode)
{
	if (theMode == mDepthTestMode)
		return;
	Flush();
	mDepthTestMode = theMode;
	ApplyDepthStates();
}

void GPU2D::SetDepthWriteMode(int theMode)
{
	if (theMode == mDepthWriteMode)
		return;
	Flush();
	mDepthWriteMode = theMode;
	ApplyDepthStates();
}

void GPU2D::SetAlphaRef(int theRef)
{
	if (theRef == mAlphaRef)
		return;
	Flush();
	mAlphaRef = theRef;
	ApplyDepthStates();
}

void GPU2D::SetDepthZ(float theWriteZ, float theOtherZ)
{
	if ((theWriteZ == mDepthZ[0]) && (theOtherZ == mDepthZ[1]))
		return;
	Flush();
	mDepthZ[0] = theWriteZ;
	mDepthZ[1] = theOtherZ;
	ApplyDepthStates();
}

void GPU2D::ApplyDrawMode(int theDrawMode)
{
	// 0x61fb90: SRCBLEND, then DESTBLEND
	if (mDepthWriteMode == 1)
	{
		SetRenderState(GPURS_SRCBLEND, 1);   // ZERO
		SetRenderState(GPURS_DESTBLEND, 2);  // ONE
	}
	else if (theDrawMode == Graphics::DRAWMODE_NORMAL)
	{
		SetRenderState(GPURS_SRCBLEND, 5);   // SRCALPHA
		SetRenderState(GPURS_DESTBLEND, 6);  // INVSRCALPHA
	}
	else
	{
		SetRenderState(GPURS_SRCBLEND, 5);
		SetRenderState(GPURS_DESTBLEND, 2);
	}
}

void GPU2D::SetLinearFilter(bool theLinear)
{
	// 0x61c9b0: MAG, MIN, MIP, through the sampler cache
	int aFilter = theLinear ? 1 : 0;
	if (aFilter == mFilter)
		return;
	unsigned long aValue = theLinear ? GPUTEXF_LINEAR : GPUTEXF_POINT;
	SetSamplerState(0, GPUSAMP_MAGFILTER, aValue);
	SetSamplerState(0, GPUSAMP_MINFILTER, aValue);
	SetSamplerState(0, GPUSAMP_MIPFILTER, aValue);
	mFilter = aFilter;
}

///////////////////////////////////////////////////////////////////////////////
// The batch (0x624880, 0x624830, 0x624710)

void GPU2D::Flush()
{
	if (mBatch.empty())
		return;
	mDevice->DrawPrimitiveUP(GPUPT_TRIANGLELIST, (int) mBatch.size() / 3, FVF_2D, &mBatch[0], sizeof(GPU2DVertex));
	mPrimitives += (int) mBatch.size() / 3;
	mDrawCalls++;
	mBatch.clear();
}

void GPU2D::DrawNow(GPUPrimitiveType theType, int theCount, const GPU2DVertex* theVertices)
{
	mDevice->DrawPrimitiveUP(theType, theCount, FVF_2D, theVertices, sizeof(GPU2DVertex));
}

void GPU2D::AddPrimitives(GPUPrimitiveType theType, int theCount, const GPU2DVertex* theVertices)
{
	if ((theType != GPUPT_TRIANGLELIST) && (theType != GPUPT_TRIANGLESTRIP) && (theType != GPUPT_TRIANGLEFAN))
	{
		Flush();
		DrawNow(theType, theCount, theVertices);
		mPrimitives += theCount;
		mDrawCalls++;
		return;
	}
	if ((theType != GPUPT_TRIANGLELIST) && (theCount * 3 > BATCH_SIZE))
	{
		// more than the batch holds: drawn at once in its own type
		Flush();
		DrawNow(theType, theCount, theVertices);
		mPrimitives += theCount;
		mDrawCalls++;
		return;
	}
	if ((theType != GPUPT_TRIANGLELIST) && (BATCH_SIZE - (int) mBatch.size() < theCount * 3))
		Flush();

	if (theType == GPUPT_TRIANGLELIST)
	{
		for (int i = 0; i < theCount; i++)
		{
			if (BATCH_SIZE - (int) mBatch.size() < 3)
				Flush();
			mBatch.push_back(theVertices[i * 3]);
			mBatch.push_back(theVertices[i * 3 + 1]);
			mBatch.push_back(theVertices[i * 3 + 2]);
		}
	}
	else
	{
		// a strip as (n-2, n-1, new), a fan as (0, n-1, new), from the three first vertices on
		size_t aFirst = mBatch.size();
		mBatch.push_back(theVertices[0]);
		mBatch.push_back(theVertices[1]);
		mBatch.push_back(theVertices[2]);
		for (int i = 1; i < theCount; i++)
		{
			size_t anEnd = mBatch.size();
			GPU2DVertex aFirstOne = (theType == GPUPT_TRIANGLESTRIP) ? mBatch[anEnd - 2] : mBatch[aFirst];
			GPU2DVertex aLast = mBatch[anEnd - 1];
			mBatch.push_back(aFirstOne);
			mBatch.push_back(aLast);
			mBatch.push_back(theVertices[i + 2]);
		}
	}
	if ((int) mBatch.size() + 3 > BATCH_SIZE)
		Flush();
}

///////////////////////////////////////////////////////////////////////////////
// Software clipping (0x614f40, 0x614fd0, 0x6152c0, 0x6154f0, 0x60f910, 0x61d540)

static float GetCoord(const GPU2DVertex& theVertex, int theCoord)
{
	return (theCoord == 0) ? theVertex.mX : theVertex.mY;
}

static GPU2DVertex Interpolate(const GPU2DVertex& v1, const GPU2DVertex& v2, float t)
{
	GPU2DVertex aVertex = v1;
	aVertex.mX = v1.mX + t * (v2.mX - v1.mX);
	aVertex.mY = (v2.mY - v1.mY) * t + v1.mY;
	aVertex.mU = (v2.mU - v1.mU) * t + v1.mU;
	aVertex.mV = (v2.mV - v1.mV) * t + v1.mV;
	if (v1.mDiffuse != v2.mDiffuse)
	{
		// rounded to nearest (x87 fistp), where SAF truncates
		int aShifts[4] = { 24, 16, 8, 0 };
		unsigned long aColor = 0;
		for (int i = 0; i < 4; i++)
		{
			float a = (float) ((v1.mDiffuse >> aShifts[i]) & 0xff);
			float b = (float) ((v2.mDiffuse >> aShifts[i]) & 0xff);
			long aChannel = lrintf(a + t * (b - a));
			aColor |= ((unsigned long) aChannel & 0xff) << aShifts[i];
		}
		aVertex.mDiffuse = aColor;
	}
	return aVertex;
}

// Sutherland-Hodgman against one edge: theGreater false clips coord < theClip away, true clips coord >= theClip
static void ClipPoints(int theCoord, float theClip, bool theGreater, const std::vector<GPU2DVertex>& theIn,
	std::vector<GPU2DVertex>& theOut)
{
	theOut.clear();
	if (theIn.size() < 2)
		return;

	for (size_t i = 0; i < theIn.size(); i++)
	{
		const GPU2DVertex& v1 = (i == 0) ? theIn[theIn.size() - 1] : theIn[i - 1];
		const GPU2DVertex& v2 = theIn[i];
		float c1 = GetCoord(v1, theCoord);
		float c2 = GetCoord(v2, theCoord);
		bool aOut1 = theGreater ? (c1 >= theClip) : (c1 < theClip);
		bool aOut2 = theGreater ? (c2 >= theClip) : (c2 < theClip);
		if (!aOut1)
		{
			if (!aOut2)
				theOut.push_back(v2);
			else
				theOut.push_back(Interpolate(v1, v2, (theClip - c1) / (c2 - c1)));
		}
		else if (!aOut2)
		{
			theOut.push_back(Interpolate(v1, v2, (theClip - c1) / (c2 - c1)));
			theOut.push_back(v2);
		}
	}
}

void GPU2D::DrawClipped(const Rect& theClipRect, const VertexList& theList)
{
	VertexList l1 = theList, l2;
	ClipPoints(0, (float) theClipRect.mX, false, l1, l2);
	ClipPoints(1, (float) theClipRect.mY, false, l2, l1);
	ClipPoints(0, (float) (theClipRect.mX + theClipRect.mWidth), true, l1, l2);
	ClipPoints(1, (float) (theClipRect.mY + theClipRect.mHeight), true, l2, l1);
	if (l1.size() > 2)
		AddPrimitives(GPUPT_TRIANGLEFAN, (int) l1.size() - 2, &l1[0]);
}

///////////////////////////////////////////////////////////////////////////////
// The transform stack

void GPU2D::PushTransform(const SexyMatrix3& theTransform, bool theConcatenate)
{
	if (mTransformStack.empty() || !theConcatenate)
		mTransformStack.push_back(theTransform);
	else
		mTransformStack.push_back(mTransformStack.back() * theTransform);
}

void GPU2D::PopTransform()
{
	if (!mTransformStack.empty())
		mTransformStack.pop_back();
}

static void TransformPoint(const SexyMatrix3& theMatrix, float& theX, float& theY)
{
	SexyVector2 aPoint = theMatrix * SexyVector2(theX, theY);
	theX = aPoint.x;
	theY = aPoint.y;
}

///////////////////////////////////////////////////////////////////////////////
// Texture pieces: the source rectangle walked over the pieces it covers (0x61c6e0 for each)

void GPU2D::BltPieces(Image* theImage, const GPUPieceLayout& theLayout, const Rect& theSrcRect, unsigned long theColor,
	const SexyMatrix3* theMatrix, float theX, float theY, bool theCenter, const Rect* theClipRect)
{
	if ((theSrcRect.mWidth <= 0) || (theSrcRect.mHeight <= 0))
		return;

	float z = VertexZ();

	// BltTransformed (0x61d700): the start, and the offset added after the matrix
	float aStartX = 0, aStartY = 0, anOffset = mPolicy.mTransformedOffset;
	if (theCenter)
	{
		aStartX = -theSrcRect.mWidth * 0.5f;
		aStartY = -theSrcRect.mHeight * 0.5f;
		anOffset = mPolicy.mTransformedCentredOffset;
	}
	bool aClip = (theMatrix != NULL) && (theClipRect != NULL);

	int aSrcY = theSrcRect.mY;
	float aDestY = aStartY;
	while (aSrcY < theSrcRect.mY + theSrcRect.mHeight)
	{
		int aSrcX = theSrcRect.mX;
		float aDestX = aStartX;
		int aRowHeight = 0;
		while (aSrcX < theSrcRect.mX + theSrcRect.mWidth)
		{
			// 0x61c6e0: the piece holding (x, y), and u, v against its texture's size
			int aWidth = theSrcRect.mX + theSrcRect.mWidth - aSrcX;
			int aHeight = theSrcRect.mY + theSrcRect.mHeight - aSrcY;
			int aPieceIndex = (aSrcY / theLayout.mPieceHeight) * theLayout.mColumns + aSrcX / theLayout.mPieceWidth;
			const GPUPiece& aPiece = theLayout.mPieces[aPieceIndex];
			int aLeft = aSrcX % theLayout.mPieceWidth;
			int aTop = aSrcY % theLayout.mPieceHeight;
			int aRight = std::min(aLeft + aWidth, aPiece.mWidth);
			int aBottom = std::min(aTop + aHeight, aPiece.mHeight);
			aWidth = aRight - aLeft;
			aHeight = aBottom - aTop;
			float u1 = (float) aLeft / aPiece.mWidth;
			float v1 = (float) aTop / aPiece.mHeight;
			float u2 = (float) aRight / aPiece.mWidth;
			float v2 = (float) aBottom / aPiece.mHeight;

			SetTexturePiece(0, theImage, aPieceIndex);

			GPU2DVertex aQuad[4];
			if (theMatrix == NULL)
			{
				// Blt (0x61ca30): TL, TR, BL, BR at the piece's place - 0.5
				float x = theX + aDestX + mPolicy.mBltOffset;
				float y = theY + aDestY + mPolicy.mBltOffset;
				aQuad[0] = MakeVertex(x, y, z, theColor, u1, v1);
				aQuad[1] = MakeVertex(x + aWidth, y, z, theColor, u2, v1);
				aQuad[2] = MakeVertex(x, y + aHeight, z, theColor, u1, v2);
				aQuad[3] = MakeVertex(x + aWidth, y + aHeight, z, theColor, u2, v2);
			}
			else
			{
				float aCorners[4][2] = { { aDestX, aDestY }, { aDestX + aWidth, aDestY }, { aDestX, aDestY + aHeight },
					{ aDestX + aWidth, aDestY + aHeight } };
				float aUV[4][2] = { { u1, v1 }, { u2, v1 }, { u1, v2 }, { u2, v2 } };
				for (int i = 0; i < 4; i++)
				{
					float x = aCorners[i][0];
					float y = aCorners[i][1];
					TransformPoint(*theMatrix, x, y);
					aQuad[i] = MakeVertex(x + anOffset + theX, y + anOffset + theY, z, theColor, aUV[i][0], aUV[i][1]);
				}
			}

			bool aClipped = false;
			// the draw hook (this+0x10, 0x61d700): (5, 2, quad, 0x20, &rect), the rectangle starting as the clip rectangle
			// argument; false ends the whole call. The region mesh's call (4, 2, ...) per cell is not carried, as the mesh is not.
			Rect* aRect = const_cast<Rect*>(theClipRect);
			bool aHooked = false;
			if ((theMatrix != NULL) && (mDrawHook != NULL))
			{
				if (!mDrawHook(mDrawHookContext, 5, 2, aQuad, (int) sizeof(GPU2DVertex), &aRect))
					return;  // 0x61e2e6: the whole BltTransformed ends, not only this piece
				aHooked = true;
			}
			// no clipping against the whole screen (this[10], this[11]), whatever the destination
			if ((aHooked ? (aRect != NULL) : aClip) && !((aRect->mX == 0) && (aRect->mY == 0) && (aRect->mWidth == mScreenWidth) &&
				(aRect->mHeight == mScreenHeight)))
			{
				// the left edge is the clip rectangle argument's x, and the right edge its x plus the (hook's) width, as
				// 0x61db65 reads *param_8; port: the hook's own x for a null argument, where the original reads address 0
				int aEdgeX = (theClipRect != NULL) ? theClipRect->mX : aRect->mX;
				float aLeftEdge = (float) aEdgeX, aTopEdge = (float) aRect->mY;
				float aRightEdge = (float) (aRect->mWidth + aEdgeX);
				float aBottomEdge = (float) (aRect->mY + aRect->mHeight);
				for (int i = 0; i < 4; i++)
				{
					if ((aQuad[i].mX < aLeftEdge) || (aQuad[i].mX >= aRightEdge) || (aQuad[i].mY < aTopEdge) || (aQuad[i].mY >= aBottomEdge))
						aClipped = true;
				}
			}
			if (aClipped)
			{
				VertexList aList;
				aList.push_back(aQuad[0]);
				aList.push_back(aQuad[1]);
				aList.push_back(aQuad[3]);
				aList.push_back(aQuad[2]);
				DrawClipped(*aRect, aList);
			}
			else
			{
				AddPrimitives(GPUPT_TRIANGLESTRIP, 2, aQuad);
			}

			aSrcX += aWidth;
			aDestX += aWidth;
			aRowHeight = aHeight;
		}
		aSrcY += aRowHeight;
		aDestY += aRowHeight;
	}
}

///////////////////////////////////////////////////////////////////////////////
// The draws

void GPU2D::Blt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode)
{
	if (!mTransformStack.empty())
	{
		BltF(theImage, (float) theX, (float) theY, theSrcRect, NULL, theColor, theDrawMode);
		return;
	}
	if (!PreDraw())
		return;
	const GPUPieceLayout* aLayout = PrepareImage(theImage);
	if (aLayout == NULL)
		return;
	ApplyDrawMode(theDrawMode);
	SetLinearFilter(false);
	BltPieces(theImage, *aLayout, theSrcRect, PackColor(theColor), NULL, (float) theX, (float) theY, false, NULL);
}

void GPU2D::BltF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect* theClipRect,
	const Color& theColor, int theDrawMode)
{
	SexyTransform2D aTransform;
	aTransform.Translate(theX, theY);
	BltTransformed(theImage, theClipRect, theColor, theDrawMode, theSrcRect, aTransform, true, 0, 0, false);
}

void GPU2D::BltMirror(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect* theClipRect,
	const Color& theColor, int theDrawMode, bool theLinear)
{
	SexyTransform2D aTransform;
	aTransform.Translate((float) -theSrcRect.mWidth, 0);
	aTransform.Scale(-1, 1);
	aTransform.Translate(theX, theY);
	BltTransformed(theImage, theClipRect, theColor, theDrawMode, theSrcRect, aTransform, theLinear, 0, 0, false);
}

void GPU2D::StretchBlt(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, const Rect* theClipRect,
	const Color& theColor, int theDrawMode, bool theFastStretch, bool theMirror)
{
	float aScaleX = (float) theDestRect.mWidth / theSrcRect.mWidth;
	float aScaleY = (float) theDestRect.mHeight / theSrcRect.mHeight;
	SexyTransform2D aTransform;
	if (theMirror)
	{
		aTransform.Translate((float) -theSrcRect.mWidth, 0);
		aScaleX = -aScaleX;
	}
	aTransform.Scale(aScaleX, aScaleY);
	aTransform.Translate((float) theDestRect.mX, (float) theDestRect.mY);
	BltTransformed(theImage, theClipRect, theColor, theDrawMode, theSrcRect, aTransform, !theFastStretch, 0, 0, false);
}

void GPU2D::BltRotated(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect* theClipRect,
	const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY)
{
	SexyTransform2D aTransform;
	aTransform.Translate(-theRotCenterX, -theRotCenterY);
	aTransform.RotateRad((float) theRot);
	aTransform.Translate(theX + theRotCenterX, theY + theRotCenterY);
	BltTransformed(theImage, theClipRect, theColor, theDrawMode, theSrcRect, aTransform, true, 0, 0, false);
}

void GPU2D::BltTransformed(Image* theImage, const Rect* theClipRect, const Color& theColor, int theDrawMode,
	const Rect& theSrcRect, const SexyMatrix3& theTransform, bool theLinear, float theX, float theY, bool theCenter)
{
	if (!PreDraw())
		return;
	const GPUPieceLayout* aLayout = PrepareImage(theImage);
	if (aLayout == NULL)
		return;
	ApplyDrawMode(theDrawMode);

	// 0x620af0
	SexyMatrix3 aMatrix = theTransform;
	if (mTransformStack.empty())
	{
		SetLinearFilter(theLinear);
	}
	else
	{
		SetLinearFilter(true);
		if ((theX == 0) && (theY == 0))
		{
			aMatrix = mTransformStack.back() * theTransform;
		}
		else
		{
			SexyTransform2D aTransform;
			if (theCenter)
				aTransform.Translate(-theSrcRect.mWidth * 0.5f, -theSrcRect.mHeight * 0.5f);
			aTransform = theTransform * aTransform;
			aTransform.Translate(theX, theY);
			aMatrix = mTransformStack.back() * aTransform;
			theX = 0;
			theY = 0;
			theCenter = false;
		}
	}
	BltPieces(theImage, *aLayout, theSrcRect, PackColor(theColor), &aMatrix, theX, theY, theCenter, theClipRect);
}

void GPU2D::FillRect(const Rect& theRect, const Color& theColor, int theDrawMode)
{
	if (!PreDraw())
		return;
	ApplyDrawMode(theDrawMode);
	SetTexture(0, NULL);

	unsigned long aColor = PackColor(theColor);
	float z = VertexZ();
	float x = theRect.mX + mPolicy.mFillRectOffset;
	float y = theRect.mY + mPolicy.mFillRectOffset;
	// TL, TR, BL, BR; under a stack SAF's TL, BL, TR, BR
	float aCorners[4][2] = { { x, y }, { x + theRect.mWidth, y }, { x, y + theRect.mHeight },
		{ x + theRect.mWidth, y + theRect.mHeight } };
	if (!mTransformStack.empty())
	{
		std::swap(aCorners[1][0], aCorners[2][0]);
		std::swap(aCorners[1][1], aCorners[2][1]);
	}
	GPU2DVertex aQuad[4];
	for (int i = 0; i < 4; i++)
	{
		float cx = aCorners[i][0];
		float cy = aCorners[i][1];
		if (!mTransformStack.empty())
		{
			TransformPoint(mTransformStack.back(), cx, cy);
			cx += mPolicy.mFillRectOffset;
			cy += mPolicy.mFillRectOffset;
		}
		aQuad[i] = MakeVertex(cx, cy, z, aColor, 0, 0);
	}
	AddPrimitives(GPUPT_TRIANGLESTRIP, 2, aQuad);
}

void GPU2D::DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor,
	int theDrawMode)
{
	if (!PreDraw())
		return;
	ApplyDrawMode(theDrawMode);
	SetTexture(0, NULL);

	unsigned long aColor = PackColor(theColor);
	float z = VertexZ();
	float x1 = (float) theStartX, y1 = (float) theStartY, x2 = (float) theEndX, y2 = (float) theEndY;
	if (!mTransformStack.empty())
	{
		TransformPoint(mTransformStack.back(), x1, y1);
		TransformPoint(mTransformStack.back(), x2, y2);
	}
	GPU2DVertex aLine[3] = { MakeVertex(x1, y1, z, aColor, 0, 0), MakeVertex(x2, y2, z, aColor, 0, 0),
		MakeVertex(x2 + mPolicy.mLineOffset, y2 + mPolicy.mLineOffset, z, aColor, 0, 0) };
	AddPrimitives(GPUPT_LINESTRIP, mPolicy.mLineEndPixel ? 2 : 1, aLine);
}

static unsigned long TriColor(const TriVertex& theVertex, unsigned long theColor)
{
	return (theVertex.color != 0) ? theVertex.color : theColor;
}

void GPU2D::DrawTriangle(const TriVertex& theP1, const TriVertex& theP2, const TriVertex& theP3, const Color& theColor,
	int theDrawMode)
{
	if (!PreDraw())
		return;
	ApplyDrawMode(theDrawMode);

	unsigned long aColor = PackColor(theColor);
	float z = VertexZ();
	const TriVertex* aPoints[3] = { &theP1, &theP2, &theP3 };
	GPU2DVertex aTriangle[3];
	for (int i = 0; i < 3; i++)
	{
		float x = aPoints[i]->x, y = aPoints[i]->y;
		if (!mTransformStack.empty())
			TransformPoint(mTransformStack.back(), x, y);
		aTriangle[i] = MakeVertex(x, y, z, TriColor(*aPoints[i], aColor), 0, 0);
	}
	SetTexture(0, NULL);
	if (mDrawHook != NULL)
	{
		// 0x621170: (4, 1, triangle, 0x20, &rect), the rectangle starting as the whole target (0, 0, this[10], this[11]);
		// false draws nothing, and a rectangle that is not the whole target draws the triangle through 0x61d540
		Rect aWhole(0, 0, mScreenWidth, mScreenHeight);
		Rect* aRect = &aWhole;
		if (!mDrawHook(mDrawHookContext, 4, 1, aTriangle, (int) sizeof(GPU2DVertex), &aRect))
			return;
		if ((aRect != NULL) && ((aRect->mX != 0) || (aRect->mY != 0) || (aRect->mWidth != mScreenWidth) ||
			(aRect->mHeight != mScreenHeight)))
		{
			VertexList aList(aTriangle, aTriangle + 3);
			DrawClipped(*aRect, aList);
			return;
		}
	}
	AddPrimitives(GPUPT_TRIANGLELIST, 1, aTriangle);
}

void GPU2D::DrawTrianglesTex(Image* theTexture, const TriVertex theVertices[][3], int theNumTriangles,
	const Color& theColor, int theDrawMode, float theTX, float theTY, bool theBlend)
{
	if (!PreDraw())
		return;
	const GPUPieceLayout* aLayout = PrepareImage(theTexture);
	if (aLayout == NULL)
		return;
	// one-piece images only: a cut image draws nothing
	float aMaxU = aLayout->mMaxU, aMaxV = aLayout->mMaxV;
	if ((aMaxU > 1.0f) || (aMaxV > 1.0f))
		return;
	ApplyDrawMode(theDrawMode);
	SetLinearFilter(theBlend);
	SetTexturePiece(0, theTexture, 0);
	Flush();

	unsigned long aColor = PackColor(theColor);
	float z = VertexZ();
	static const int CACHE_TRIANGLES = 100;
	GPU2DVertex aCache[CACHE_TRIANGLES * 3];
	int aCount = 0;
	for (int t = 0; t < theNumTriangles; t++)
	{
		static const int anOrder[3] = { 0, 2, 1 };
		for (int i = 0; i < 3; i++)
		{
			const TriVertex& aVertex = theVertices[t][anOrder[i]];
			float x = aVertex.x + theTX, y = aVertex.y + theTY;
			if (!mTransformStack.empty())
				TransformPoint(mTransformStack.back(), x, y);
			aCache[aCount * 3 + i] = MakeVertex(x, y, z, TriColor(aVertex, aColor), aVertex.u * aMaxU, aVertex.v * aMaxV);
		}
		aCount++;
		if (mDrawHook != NULL)
		{
			// 0x61e310: (4, 1, the triangle just added, 0x20, &rect), the rectangle starting null. False drops the
			// triangle; a rectangle flushes the batch before it and draws the triangle through 0x61d540 with that rectangle
			Rect* aRect = NULL;
			GPU2DVertex* aSlot = &aCache[(aCount - 1) * 3];
			if (!mDrawHook(mDrawHookContext, 4, 1, aSlot, (int) sizeof(GPU2DVertex), &aRect))
			{
				aCount--;
				continue;
			}
			if (aRect != NULL)
			{
				if (aCount > 1)
					DrawNow(GPUPT_TRIANGLELIST, aCount - 1, aCache);
				VertexList aList(aSlot, aSlot + 3);
				DrawClipped(*aRect, aList);
				aCount = 0;
				continue;
			}
		}
		if (aCount == CACHE_TRIANGLES)
		{
			DrawNow(GPUPT_TRIANGLELIST, aCount, aCache);
			aCount = 0;
		}
	}
	if (aCount > 0)
		DrawNow(GPUPT_TRIANGLELIST, aCount, aCache);
}

void GPU2D::DrawTriangleTex(const TriVertex& theP1, const TriVertex& theP2, const TriVertex& theP3, const Color& theColor,
	int theDrawMode, Image* theTexture, bool theBlend)
{
	// 0x6220b0: the blend argument lands in tx, and blend is true
	TriVertex aTriangle[1][3] = { { theP1, theP2, theP3 } };
	DrawTrianglesTex(theTexture, aTriangle, 1, theColor, theDrawMode, theBlend ? 1.0f : 0.0f, 0, true);
}

void GPU2D::DrawTrianglesTexStrip(Image* theTexture, const TriVertex theVertices[], int theNumTriangles,
	const Color& theColor, int theDrawMode, float theTX, float theTY, bool theBlend)
{
	// 0x6221d0: triangles (i, i+1, i+2), 100 at a time
	static const int CACHE_TRIANGLES = 100;
	TriVertex aList[CACHE_TRIANGLES][3];
	int aCount = 0;
	for (int i = 0; i < theNumTriangles; i++)
	{
		aList[aCount][0] = theVertices[i];
		aList[aCount][1] = theVertices[i + 1];
		aList[aCount][2] = theVertices[i + 2];
		aCount++;
		if (aCount == CACHE_TRIANGLES)
		{
			DrawTrianglesTex(theTexture, aList, aCount, theColor, theDrawMode, theTX, theTY, theBlend);
			aCount = 0;
		}
	}
	if (aCount > 0)
		DrawTrianglesTex(theTexture, aList, aCount, theColor, theDrawMode, theTX, theTY, theBlend);
}

const GPUPieceLayout* GPU2D::PrepareImage(Image* theImage)
{
	if (theImage == NULL)
		return NULL;
	MemoryImage* aMemoryImage = dynamic_cast<MemoryImage*>(theImage);
	if ((aMemoryImage != NULL) && ((aMemoryImage->mD3DFlags & 0x60) != 0))
	{
		SDL_Log("GPU2D: a cube or volume texture is drawn in 2D (drawn as nothing)");
		return NULL;
	}
	return mDevice->PrepareTexture(theImage);
}

void GPU2D::FillPoly(const Point theVertices[], int theNumVertices, const Rect* theClipRect, const Color& theColor,
	int theDrawMode, int theTX, int theTY)
{
	if (theNumVertices <= 2)
		return;
	if (!PreDraw())
		return;
	ApplyDrawMode(theDrawMode);
	SetTexture(0, NULL);

	unsigned long aColor = PackColor(theColor);
	float z = VertexZ();
	VertexList aList;
	for (int i = 0; i < theNumVertices; i++)
	{
		float x = (float) (theVertices[i].mX + theTX), y = (float) (theVertices[i].mY + theTY);
		if (!mTransformStack.empty())
			TransformPoint(mTransformStack.back(), x, y);
		aList.push_back(MakeVertex(x, y, z, aColor, 0, 0));
	}
	if (theClipRect == NULL)
		AddPrimitives(GPUPT_TRIANGLEFAN, theNumVertices - 2, &aList[0]);
	else
		DrawClipped(*theClipRect, aList);
}
