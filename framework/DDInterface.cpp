// Portable DDInterface: SDL2 renderer implementation of the SexyAppFramework display layer.
// See DDInterface.h for the overall design. Geometry follows the reference D3DInterface.cpp
// (BltRotated/BltMirror/StretchBlt/BltTransformed build the same transforms), but in SDL's
// pixel convention, so the D3D7 half-pixel offsets are not needed.

#include "DDInterface.h"
#include "DDImage.h"
#include "Graphics.h"
#include "SexyAppBase.h"
#include "SexyMatrix.h"
#include "TriVertex.h"

#include <SDL.h>
#include <math.h>

using namespace Sexy;

static_assert(sizeof(ulong) == 4, "MemoryImage bits are 32-bit ARGB");

namespace Sexy
{

// Textures cached on MemoryImage::mD3DData. There is one SDL texture per filtering mode, created on
// demand: SDL2 can't reliably switch a texture's scale mode between batched draws (the D3D9 backend
// only applies it when another texture was bound in between).
struct DDTextureData
{
	enum { NEAREST, LINEAR, NUM_FILTERS };

	SDL_Texture*			mTextures[NUM_FILTERS];
	int						mBitsChangedCount[NUM_FILTERS];	// image's mBitsChangedCount when uploaded
	int						mWidth;				// texture size in pixels
	int						mHeight;
	int						mScale;				// texels per logical pixel (1, or the HD scale)
	int						mImageWidth;		// logical image size the textures are built for
	int						mImageHeight;

	DDTextureData() :
		mWidth(0), mHeight(0), mScale(1), mImageWidth(0), mImageHeight(0)
	{
		for (int i = 0; i < NUM_FILTERS; i++)
		{
			mTextures[i] = NULL;
			mBitsChangedCount[i] = -1;
		}
	}

	void ReleaseTextures()
	{
		for (int i = 0; i < NUM_FILTERS; i++)
		{
			if (mTextures[i] != NULL)
				SDL_DestroyTexture(mTextures[i]);
			mTextures[i] = NULL;
			mBitsChangedCount[i] = -1;
		}
	}
};

}


static std::vector<uint32_t> gPixelBuffer;
static const int kPixelBufferSize = 65536; // pixels a texture upload converts at a time (256 KB)
static std::vector<uint32_t> gReadBuffer;
static std::vector<SDL_Vertex> gVertices;
static std::vector<int> gIndices;

static inline int ClampByte(int theValue)
{
	return (theValue < 0) ? 0 : ((theValue > 255) ? 255 : theValue);
}

static SDL_Color MakeVertexColor(int theRed, int theGreen, int theBlue, int theAlpha, bool premultiplied)
{
	int r = ClampByte(theRed);
	int g = ClampByte(theGreen);
	int b = ClampByte(theBlue);
	int a = ClampByte(theAlpha);

	if (premultiplied)
	{
		r = (r * a + 127) / 255;
		g = (g * a + 127) / 255;
		b = (b * a + 127) / 255;
	}

	SDL_Color aColor = { (Uint8) r, (Uint8) g, (Uint8) b, (Uint8) a };
	return aColor;
}

static inline SDL_Color MakeVertexColor(const Color& theColor, bool premultiplied)
{
	return MakeVertexColor(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha, premultiplied);
}

static inline uint32_t Premultiply(uint32_t thePixel)
{
	uint32_t a = thePixel >> 24;
	if (a == 255)
		return thePixel;
	if (a == 0)
		return 0;

	uint32_t r = (((thePixel >> 16) & 0xFF) * a + 127) / 255;
	uint32_t g = (((thePixel >> 8) & 0xFF) * a + 127) / 255;
	uint32_t b = ((thePixel & 0xFF) * a + 127) / 255;
	return (a << 24) | (r << 16) | (g << 8) | b;
}

static inline void SetVertex(SDL_Vertex& theVertex, float x, float y, const SDL_Color& theColor, float u, float v)
{
	theVertex.position.x = x;
	theVertex.position.y = y;
	theVertex.color = theColor;
	theVertex.tex_coord.x = u;
	theVertex.tex_coord.y = v;
}

static inline bool IsFractional(float theValue)
{
	return theValue != floorf(theValue);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
DDInterface::DDInterface(SexyAppBase* theApp)
{
	mApp = theApp;
	mWindow = NULL;
	mRenderer = NULL;
	mScreenImage = NULL;
	mWidth = (theApp != NULL) ? theApp->mWidth : 640;
	mHeight = (theApp != NULL) ? theApp->mHeight : 480;
	mDisplayWidth = mWidth;
	mDisplayHeight = mHeight;
	mPresentationRect = Rect(0, 0, mWidth, mHeight);
	mIs3D = false;
	mCan3D = false;
	mIsWindowed = true;
	mVSync = false;
	// Unscaled 1x blits sample texels exactly, as at 1x in 1.3. Linear filtering them at a render scale
	// above 1 pulls in the neighbouring cell of sprite sheets and fonts (visible slivers between glyphs).
	mPixelPerfect = true;
	mFrameCount = 0;
	mSoftwareStretch = false;
	mSoftwareTransforms = false;
	mStretchUse = 0;

	mCursorImage = NULL;
	mCursorX = 0;
	mCursorY = 0;
	mCursorVisible = false;

	mScreenTexture = NULL;
	mRenderScale = 1;
	mForcedRenderScale = 0;

	mBlendNormal = SDL_BLENDMODE_BLEND;
	mBlendAdditive = SDL_BLENDMODE_ADD;
	mPremultiplied = false;
	mMaxTextureWidth = 4096;
	mMaxTextureHeight = 4096;

	mClipEnabled = false;

	// The port always works in 32-bit ARGB (used by MemoryImage::GetNativeAlphaData & co.)
	mRGBBits = 32;
	mRedMask = 0xFF0000;
	mGreenMask = 0x00FF00;
	mBlueMask = 0x0000FF;
	mRedBits = 8;
	mGreenBits = 8;
	mBlueBits = 8;
	mRedShift = 16;
	mGreenShift = 8;
	mBlueShift = 0;
}

DDInterface::~DDInterface()
{
	Cleanup();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int DDInterface::Init(SDL_Window* theWindow, bool vsync)
{
	ReleaseRenderer();

	mWindow = theWindow;
	mVSync = vsync;
	if (mApp != NULL)
	{
		mWidth = mApp->mWidth;
		mHeight = mApp->mHeight;
	}

	if ((mWindow == NULL) || (mWidth <= 0) || (mHeight <= 0))
		return RESULT_FAIL;

	Uint32 aFlags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE;
	if (vsync)
		aFlags |= SDL_RENDERER_PRESENTVSYNC;

	mRenderer = SDL_CreateRenderer(mWindow, -1, aFlags);
	if (mRenderer == NULL)
		mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);
	if (mRenderer == NULL)
		return RESULT_DD_CREATE_FAIL;

	SDL_RendererInfo anInfo;
	if (SDL_GetRendererInfo(mRenderer, &anInfo) == 0)
	{
		mIs3D = (anInfo.flags & SDL_RENDERER_ACCELERATED) != 0;
		mCan3D = mIs3D;
		mMaxTextureWidth = (anInfo.max_texture_width > 0) ? anInfo.max_texture_width : 16384;
		mMaxTextureHeight = (anInfo.max_texture_height > 0) ? anInfo.max_texture_height : 16384;
	}

	SetupBlendModes();

	if (mScreenImage == NULL)
	{
		mScreenImage = new DDImage(this);
		mScreenImage->mIsScreen = true;
	}
	if ((mScreenImage->mWidth != mWidth) || (mScreenImage->mHeight != mHeight))
		mScreenImage->Create(mWidth, mHeight);

	UpdateDisplaySize();
	if (mScreenTexture == NULL)
	{
		ReleaseRenderer();
		return RESULT_SURFACE_FAIL;
	}

	BeginFrame();
	return RESULT_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void DDInterface::SetupBlendModes()
{
	SDL_BlendMode aNormal = SDL_ComposeCustomBlendMode(
		SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD,
		SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);
	SDL_BlendMode anAdditive = SDL_ComposeCustomBlendMode(
		SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD,
		SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
	// Without separate alpha blending: same result, since the screen is always opaque
	SDL_BlendMode anAdditiveSimple = SDL_ComposeCustomBlendMode(
		SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD,
		SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);

	mPremultiplied = false;
	mBlendNormal = SDL_BLENDMODE_BLEND;
	mBlendAdditive = SDL_BLENDMODE_ADD;

	SDL_Texture* aTestTexture = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 1, 1);
	if (aTestTexture != NULL)
	{
		if ((SDL_SetTextureBlendMode(aTestTexture, aNormal) == 0) && (SDL_SetRenderDrawBlendMode(mRenderer, aNormal) == 0))
		{
			SDL_BlendMode aModes[2] = { anAdditive, anAdditiveSimple };
			for (int i = 0; i < 2; i++)
			{
				if ((SDL_SetTextureBlendMode(aTestTexture, aModes[i]) == 0) && (SDL_SetRenderDrawBlendMode(mRenderer, aModes[i]) == 0))
				{
					mPremultiplied = true;
					mBlendNormal = aNormal;
					mBlendAdditive = aModes[i];
					break;
				}
			}
		}
		SDL_DestroyTexture(aTestTexture);
	}

	SDL_SetRenderDrawBlendMode(mRenderer, (SDL_BlendMode) mBlendNormal);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void DDInterface::ReleaseTextures()
{
	std::set<MemoryImage*>::iterator anItr;
	for (anItr = mTextureImageSet.begin(); anItr != mTextureImageSet.end(); ++anItr)
	{
		MemoryImage* anImage = *anItr;
		DDTextureData* aData = (DDTextureData*) anImage->mD3DData;
		if (aData != NULL)
		{
			aData->ReleaseTextures();
			delete aData;
			anImage->mD3DData = NULL;
		}
	}
	mTextureImageSet.clear();

	if (mScreenTexture != NULL)
	{
		SDL_DestroyTexture(mScreenTexture);
		mScreenTexture = NULL;
	}
	mClipEnabled = false;
}

void DDInterface::ReleaseRenderer()
{
	ReleaseTextures();

	if (mRenderer != NULL)
	{
		SDL_DestroyRenderer(mRenderer);
		mRenderer = NULL;
	}
}

void DDInterface::Cleanup()
{
	ClearStretchCopies();
	if (mScreenImage != NULL)
	{
		DDImage* aScreenImage = mScreenImage;
		mScreenImage = NULL;
		delete aScreenImage;
	}

	ReleaseRenderer();
	mWindow = NULL; // owned by the app
	mCursorImage = NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void DDInterface::UpdateDisplaySize()
{
	if (mRenderer == NULL)
		return;

	// SDL_GetRendererOutputSize reports the target's size while a render target is set
	SDL_Texture* aTarget = SDL_GetRenderTarget(mRenderer);
	if (aTarget != NULL)
	{
		SDL_SetRenderTarget(mRenderer, NULL);
		mClipEnabled = false;
	}

	int aWidth = 0;
	int aHeight = 0;
	if (SDL_GetRendererOutputSize(mRenderer, &aWidth, &aHeight) != 0)
		aWidth = aHeight = 0;
	int aRenderScale = DisplayGeometry::UpdateDisplaySize(aWidth, aHeight, mMaxTextureWidth, mMaxTextureHeight, mForcedRenderScale);

	if ((mScreenTexture == NULL) || (aRenderScale != mRenderScale))
		CreateScreenTexture(aRenderScale);
	else if (aTarget != NULL)
		SDL_SetRenderTarget(mRenderer, aTarget);
}

bool DDInterface::CreateScreenTexture(int theRenderScale)
{
	if (mScreenTexture != NULL)
	{
		SDL_DestroyTexture(mScreenTexture);
		mScreenTexture = NULL;
	}

	for (int aScale = theRenderScale; aScale >= 1; aScale--)
	{
		mScreenTexture = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, mWidth * aScale, mHeight * aScale);
		if (mScreenTexture != NULL)
		{
			mRenderScale = aScale;
			break;
		}
	}

	if (mScreenTexture == NULL)
		return false;

	SDL_SetTextureBlendMode(mScreenTexture, SDL_BLENDMODE_NONE);
	SDL_SetTextureScaleMode(mScreenTexture, SDL_ScaleModeLinear);

	SDL_SetRenderTarget(mRenderer, mScreenTexture);
	mClipEnabled = false;
	SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, 255);
	SDL_RenderClear(mRenderer);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void DDInterface::BeginFrame()
{
	if (mRenderer == NULL)
		return;

	UpdateDisplaySize();

	if (!PreDraw())
		return;

	SetClip(NULL);
	SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, 255);
	SDL_RenderClear(mRenderer);
}

void DDInterface::Present()
{
	if (mRenderer == NULL)
		return;

	SDL_SetRenderTarget(mRenderer, NULL);
	mClipEnabled = false;
	SDL_RenderSetClipRect(mRenderer, NULL);

	SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, 255);
	SDL_RenderClear(mRenderer);

	if (mScreenTexture != NULL)
	{
		SDL_Rect aDestRect = { mPresentationRect.mX, mPresentationRect.mY, mPresentationRect.mWidth, mPresentationRect.mHeight };
		SDL_RenderCopy(mRenderer, mScreenTexture, NULL, &aDestRect);
	}

	DrawCursor();

	SDL_RenderPresent(mRenderer);
	mFrameCount++;
}

// Draws the custom cursor straight onto the backbuffer, at the presentation scale.
void DDInterface::DrawCursor()
{
	if ((!mCursorVisible) || (mCursorImage == NULL) || (mScreenTexture == NULL))
		return;

	MemoryImage* anImage = dynamic_cast<MemoryImage*>(mCursorImage);
	DDTextureData* aData = GetTextureData(anImage);
	if (aData == NULL)
		return;

	float aScaleX = mPresentationRect.mWidth / (float) mWidth;
	float aScaleY = mPresentationRect.mHeight / (float) mHeight;

	SDL_Rect aSrcRect = { 0, 0, anImage->mWidth * aData->mScale, anImage->mHeight * aData->mScale };
	SDL_FRect aDestRect;
	CursorRect(mCursorX, mCursorY, anImage->mWidth, anImage->mHeight, &aDestRect.x, &aDestRect.y, &aDestRect.w, &aDestRect.h);
	SDL_Rect aClipRect = { mPresentationRect.mX, mPresentationRect.mY, mPresentationRect.mWidth, mPresentationRect.mHeight };

	SDL_Texture* aTexture = PrepareTexture(anImage, aData, Graphics::DRAWMODE_NORMAL, IsFractional(aScaleX) || IsFractional(aScaleY));
	if (aTexture == NULL)
		return;

	SDL_RenderSetClipRect(mRenderer, &aClipRect);
	SDL_RenderCopyF(mRenderer, aTexture, &aSrcRect, &aDestRect);
	SDL_RenderSetClipRect(mRenderer, NULL);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// The window's size in points for DisplayGeometry, 0 when there is no window
static void GetWindowPoints(SDL_Window* theWindow, int* theWidth, int* theHeight)
{
	*theWidth = 0;
	*theHeight = 0;
	if (theWindow != NULL)
		SDL_GetWindowSize(theWindow, theWidth, theHeight);
}

void DDInterface::WindowToLogical(int theWindowX, int theWindowY, int* theX, int* theY)
{
	int aWindowWidth, aWindowHeight;
	GetWindowPoints(mWindow, &aWindowWidth, &aWindowHeight);
	DisplayGeometry::WindowToLogical(aWindowWidth, aWindowHeight, theWindowX, theWindowY, theX, theY);
}

void DDInterface::LogicalToWindow(int theX, int theY, int* theWindowX, int* theWindowY)
{
	int aWindowWidth, aWindowHeight;
	GetWindowPoints(mWindow, &aWindowWidth, &aWindowHeight);
	DisplayGeometry::LogicalToWindow(aWindowWidth, aWindowHeight, theX, theY, theWindowX, theWindowY);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool DDInterface::PreDraw()
{
	if ((mRenderer == NULL) || (mScreenTexture == NULL))
		return false;

	if (SDL_GetRenderTarget(mRenderer) != mScreenTexture)
	{
		SDL_SetRenderTarget(mRenderer, mScreenTexture);
		mClipEnabled = false; // SDL resets the clip rect when the target changes
	}

	return true;
}

// Returns false when the clip rectangle is empty (nothing can be drawn).
bool DDInterface::SetClip(const Rect* theClipRect)
{
	if (theClipRect == NULL)
	{
		if (mClipEnabled)
		{
			SDL_RenderSetClipRect(mRenderer, NULL);
			mClipEnabled = false;
		}
		return true;
	}

	if ((theClipRect->mWidth <= 0) || (theClipRect->mHeight <= 0))
		return false;

	if (mClipEnabled && (mCurClipRect == *theClipRect))
		return true;

	SDL_Rect aRect = { theClipRect->mX * mRenderScale, theClipRect->mY * mRenderScale,
		theClipRect->mWidth * mRenderScale, theClipRect->mHeight * mRenderScale };
	SDL_RenderSetClipRect(mRenderer, &aRect);
	mClipEnabled = true;
	mCurClipRect = *theClipRect;
	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
DDTextureData* DDInterface::GetTextureData(MemoryImage* theImage)
{
	if ((mRenderer == NULL) || (theImage == NULL) || (theImage->mWidth <= 0) || (theImage->mHeight <= 0))
		return NULL;

	// port: HD pixels (any MemoryImage, see MemoryImage.h) are the texture while they are valid
	DDImage* aScreenTest = dynamic_cast<DDImage*>(theImage);
	bool useHD = ((aScreenTest == NULL) || (!aScreenTest->mIsScreen)) && theImage->HasValidHDBits() &&
		(theImage->mHDWidth <= mMaxTextureWidth) && (theImage->mHDHeight <= mMaxTextureHeight);

	int aScale = useHD ? theImage->mHDScale : 1;
	int aTexWidth = useHD ? theImage->mHDWidth : theImage->mWidth;
	int aTexHeight = useHD ? theImage->mHDHeight : theImage->mHeight;

	DDTextureData* aData = (DDTextureData*) theImage->mD3DData;
	if (aData == NULL)
	{
		aData = new DDTextureData();
		theImage->mD3DData = aData;
		mTextureImageSet.insert(theImage);
	}

	if ((aData->mScale != aScale) || (aData->mWidth != aTexWidth) || (aData->mHeight != aTexHeight) ||
		(aData->mImageWidth != theImage->mWidth) || (aData->mImageHeight != theImage->mHeight))
	{
		aData->ReleaseTextures();
		aData->mScale = aScale;
		aData->mWidth = aTexWidth;
		aData->mHeight = aTexHeight;
		aData->mImageWidth = theImage->mWidth;
		aData->mImageHeight = theImage->mHeight;
	}

	return aData;
}

// Returns the image's texture for one draw (blend mode set), creating and uploading it as needed.
SDL_Texture* DDInterface::PrepareTexture(MemoryImage* theImage, DDTextureData* theData, int theDrawMode, bool linearHint)
{
	bool aLinear = (theData->mScale > 1) || linearHint || ((mRenderScale > 1) && (!mPixelPerfect));
	int aFilter = aLinear ? DDTextureData::LINEAR : DDTextureData::NEAREST;

	SDL_Texture* aTexture = theData->mTextures[aFilter];
	if (aTexture == NULL)
	{
		aTexture = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, theData->mWidth, theData->mHeight);
		if (aTexture == NULL)
			return NULL;

		SDL_SetTextureScaleMode(aTexture, aLinear ? SDL_ScaleModeLinear : SDL_ScaleModeNearest);
		theData->mTextures[aFilter] = aTexture;
		theData->mBitsChangedCount[aFilter] = -1;
	}

	DDImage* aDDImage = dynamic_cast<DDImage*>(theImage);
	bool isScreen = (aDDImage != NULL) && aDDImage->mIsScreen; // its pixels change without BitsChanged

	if ((theData->mBitsChangedCount[aFilter] != theImage->mBitsChangedCount) || isScreen)
	{
		int aWidth = theData->mWidth;
		int aHeight = theData->mHeight;
		const uint32_t* aPixels = NULL;
		bool isPalletized = false;
		if (theData->mScale > 1)
			aPixels = &theImage->mHDBits[0];
		else if ((theImage->mBits == NULL) && (theImage->mColorTable != NULL) && (theImage->mColorIndices != NULL))
			isPalletized = true; // expanded below without un-palletizing the image
		else
		{
			aPixels = (const uint32_t*) theImage->GetBits();
			if (aPixels == NULL)
				return NULL;
		}

		if ((!isPalletized) && (!mPremultiplied))
			SDL_UpdateTexture(aTexture, NULL, aPixels, aWidth * 4);
		else
		{
			// converted a strip of rows at a time, so the buffer stays small instead of growing to the largest image
			int aStripRows = std::max(1, kPixelBufferSize / aWidth);
			gPixelBuffer.resize((size_t) std::min(aStripRows, aHeight) * aWidth);
			for (int aY = 0; aY < aHeight; aY += aStripRows)
			{
				int aRows = std::min(aStripRows, aHeight - aY);
				int aCount = aRows * aWidth;
				int aStart = aY * aWidth;
				if (isPalletized)
				{
					for (int i = 0; i < aCount; i++)
						gPixelBuffer[i] = (uint32_t) theImage->mColorTable[theImage->mColorIndices[aStart + i]];
				}
				else
				{
					for (int i = 0; i < aCount; i++)
						gPixelBuffer[i] = aPixels[aStart + i];
				}
				if (mPremultiplied)
				{
					for (int i = 0; i < aCount; i++)
						gPixelBuffer[i] = Premultiply(gPixelBuffer[i]);
				}
				SDL_Rect aRect = {0, aY, aWidth, aRows};
				SDL_UpdateTexture(aTexture, &aRect, &gPixelBuffer[0], aWidth * 4);
			}
		}
		theData->mBitsChangedCount[aFilter] = theImage->mBitsChangedCount;
	}

	int aBlendMode = (theDrawMode == Graphics::DRAWMODE_ADDITIVE) ? mBlendAdditive : mBlendNormal;
	SDL_SetTextureBlendMode(aTexture, (SDL_BlendMode) aBlendMode);
	return aTexture;
}

SDL_Texture* DDInterface::GetTexture(MemoryImage* theImage, int* theTexWidth, int* theTexHeight)
{
	DDTextureData* aData = GetTextureData(theImage);
	if (aData == NULL)
		return NULL;

	SDL_Texture* aTexture = PrepareTexture(theImage, aData, Graphics::DRAWMODE_NORMAL, false);
	if (theTexWidth != NULL)
		*theTexWidth = aData->mWidth;
	if (theTexHeight != NULL)
		*theTexHeight = aData->mHeight;
	return aTexture;
}

bool DDInterface::CheckDestination(DDImage* theImage)
{
	return theImage->mIsScreen;
}

void DDInterface::RemoveImageData(MemoryImage* theImage)
{
	// a source's stretched copies go with it (taken out of the list before they are deleted, since deleting a copy
	// comes back here)
	std::vector<MemoryImage*> aCopies;
	for (size_t i = 0; i < mStretchCopies.size(); )
	{
		if (mStretchCopies[i].mSource == theImage)
		{
			aCopies.push_back(mStretchCopies[i].mCopy);
			mStretchCopies.erase(mStretchCopies.begin() + i);
		}
		else
			i++;
	}
	for (MemoryImage* aCopy : aCopies)
		delete aCopy;

	if ((theImage == NULL) || (theImage->mD3DData == NULL))
		return;

	DDTextureData* aData = (DDTextureData*) theImage->mD3DData;
	aData->ReleaseTextures();
	delete aData;

	theImage->mD3DData = NULL;
	mTextureImageSet.erase(theImage);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Draws theSrcRect of theImage onto the logical quad theX/theY (corners: top-left, top-right,
// bottom-right, bottom-left of the source rectangle).
void DDInterface::DrawTexturedQuad(Image* theImage, const float theX[4], const float theY[4], const Rect& theSrcRect, bool mirror, const Color& theColor, int theDrawMode, const Rect* theClipRect, bool linearHint)
{
	MemoryImage* aSrcImage = dynamic_cast<MemoryImage*>(theImage);
	if ((aSrcImage == NULL) || (theSrcRect.mWidth <= 0) || (theSrcRect.mHeight <= 0))
		return;

	if (!PreDraw())
		return;

	DDTextureData* aData = GetTextureData(aSrcImage);
	if (aData == NULL)
		return;

	SDL_Texture* aTexture = PrepareTexture(aSrcImage, aData, theDrawMode, linearHint);
	if (aTexture == NULL)
		return;

	if (!SetClip(theClipRect))
		return;

	float aUScale = aData->mScale / (float) aData->mWidth;
	float aVScale = aData->mScale / (float) aData->mHeight;
	float u0 = theSrcRect.mX * aUScale;
	float u1 = (theSrcRect.mX + theSrcRect.mWidth) * aUScale;
	float v0 = theSrcRect.mY * aVScale;
	float v1 = (theSrcRect.mY + theSrcRect.mHeight) * aVScale;
	if (mirror)
		std::swap(u0, u1);

	const float aU[4] = { u0, u1, u1, u0 };
	const float aV[4] = { v0, v0, v1, v1 };

	SDL_Color aColor = MakeVertexColor(theColor, mPremultiplied);
	float aScale = (float) mRenderScale;

	// port: SDL's software renderer turns an axis-aligned quad back into a copy whose source rect is
	// truncated from the UVs ((int) (u * width)), which can land a texel short and shift sprite-sheet
	// cells by a pixel. Give it the exact rectangle instead.
	if ((!mIs3D) && (theX[0] == theX[3]) && (theX[1] == theX[2]) && (theY[0] == theY[1]) && (theY[2] == theY[3]) &&
		(theX[1] > theX[0]) && (theY[3] > theY[0]))
	{
		SDL_Rect aSrcRect = { theSrcRect.mX * aData->mScale, theSrcRect.mY * aData->mScale, theSrcRect.mWidth * aData->mScale, theSrcRect.mHeight * aData->mScale };
		SDL_FRect aDestRect = { theX[0] * aScale, theY[0] * aScale, (theX[1] - theX[0]) * aScale, (theY[3] - theY[0]) * aScale };
		SDL_SetTextureColorMod(aTexture, aColor.r, aColor.g, aColor.b);
		SDL_SetTextureAlphaMod(aTexture, aColor.a);
		SDL_RenderCopyExF(mRenderer, aTexture, &aSrcRect, &aDestRect, 0, NULL, mirror ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
		return;
	}

	SDL_Vertex aVertices[4];
	for (int i = 0; i < 4; i++)
		SetVertex(aVertices[i], theX[i] * aScale, theY[i] * aScale, aColor, aU[i], aV[i]);

	static const int anIndices[6] = { 0, 1, 2, 0, 2, 3 };
	SDL_RenderGeometry(mRenderer, aTexture, aVertices, 4, anIndices, 6);
}

void DDInterface::Blt(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode, const Rect* theClipRect, bool mirror)
{
	float aRight = theX + theSrcRect.mWidth;
	float aBottom = theY + theSrcRect.mHeight;

	const float aX[4] = { theX, aRight, aRight, theX };
	const float aY[4] = { theY, theY, aBottom, aBottom };
	DrawTexturedQuad(theImage, aX, aY, theSrcRect, mirror, theColor, theDrawMode, theClipRect, IsFractional(theX) || IsFractional(theY));
}

void DDInterface::StretchBlt(Image* theImage, const FRect& theDestRect, const Rect& theSrcRect, const Rect* theClipRect, const Color& theColor, int theDrawMode, bool fastStretch, bool mirror)
{
	float aLeft = (float) theDestRect.mX;
	float aTop = (float) theDestRect.mY;
	float aRight = (float) (theDestRect.mX + theDestRect.mWidth);
	float aBottom = (float) (theDestRect.mY + theDestRect.mHeight);

	const float aX[4] = { aLeft, aRight, aRight, aLeft };
	const float aY[4] = { aTop, aTop, aBottom, aBottom };
	bool isScaled = (theDestRect.mWidth != theSrcRect.mWidth) || (theDestRect.mHeight != theSrcRect.mHeight);
	if (mSoftwareStretch && !mirror && isScaled)
	{
		MemoryImage* aSource = dynamic_cast<MemoryImage*>(theImage);
		int aWidth = (int) theDestRect.mWidth;
		int aHeight = (int) theDestRect.mHeight;
		MemoryImage* aCopy = aSource != NULL ? GetStretchCopy(aSource, theSrcRect, aWidth, aHeight, fastStretch) : NULL;
		if (aCopy != NULL)
		{
			Blt(aCopy, aLeft, aTop, Rect(0, 0, aWidth, aHeight), theColor, theDrawMode, theClipRect);
			return;
		}
	}
	DrawTexturedQuad(theImage, aX, aY, theSrcRect, mirror, theColor, theDrawMode, theClipRect, !fastStretch && isScaled);
}

MemoryImage* DDInterface::GetStretchCopy(MemoryImage* theSource, const Rect& theSrcRect, int theWidth, int theHeight, bool fast)
{
	if ((theWidth <= 0) || (theHeight <= 0) || (theSrcRect.mWidth <= 0) || (theSrcRect.mHeight <= 0) ||
		((long long) theWidth * theHeight > 2048 * 2048))
		return NULL;
	// the fast stretch samples the source unchecked, so a rectangle not inside the image keeps SDL's (the averaging
	// stretch clamps its reads)
	if (fast && ((theSrcRect.mX < 0) || (theSrcRect.mY < 0) || (theSrcRect.mX + theSrcRect.mWidth > theSource->mWidth) ||
		(theSrcRect.mY + theSrcRect.mHeight > theSource->mHeight)))
		return NULL;
	mStretchUse++;
	StretchCopyEntry* anEntry = NULL;
	for (StretchCopyEntry& e : mStretchCopies)
	{
		if ((e.mSource == theSource) && (e.mSrcRect == theSrcRect) && (e.mWidth == theWidth) && (e.mHeight == theHeight) &&
			(e.mFast == fast))
		{
			anEntry = &e;
			break;
		}
	}
	if (anEntry == NULL)
	{
		long long aBytes = (long long) theWidth * theHeight * 4;
		for (const StretchCopyEntry& e : mStretchCopies)
			aBytes += (long long) e.mWidth * e.mHeight * 4;
		while ((mStretchCopies.size() >= kStretchCacheSize) || ((aBytes > kCopyCacheBytes) && !mStretchCopies.empty()))
		{
			size_t anOldest = 0;
			for (size_t i = 1; i < mStretchCopies.size(); i++)
			{
				if (mStretchCopies[i].mLastUse < mStretchCopies[anOldest].mLastUse)
					anOldest = i;
			}
			MemoryImage* aCopy = mStretchCopies[anOldest].mCopy;
			aBytes -= (long long) mStretchCopies[anOldest].mWidth * mStretchCopies[anOldest].mHeight * 4;
			mStretchCopies.erase(mStretchCopies.begin() + anOldest);
			delete aCopy;
		}
		StretchCopyEntry e;
		e.mSource = theSource;
		e.mSrcRect = theSrcRect;
		e.mWidth = theWidth;
		e.mHeight = theHeight;
		e.mFast = fast;
		e.mSourceVersion = theSource->mBitsChangedCount - 1;  // made below
		e.mCopy = new MemoryImage(mApp);
		e.mLastUse = 0;
		mStretchCopies.push_back(e);
		anEntry = &mStretchCopies.back();
	}
	anEntry->mLastUse = mStretchUse;
	if (anEntry->mSourceVersion != theSource->mBitsChangedCount)
	{
		anEntry->mSourceVersion = theSource->mBitsChangedCount;
		anEntry->mCopy->Create(theWidth, theHeight);
		if (fast)
		{
			// 0x41a3e0's fast stretch: 16.16 steps from the source rectangle's corner
			ulong* aSrcBits = theSource->GetBits();
			ulong* aDestBits = anEntry->mCopy->GetBits();
			int aStepX = (int) ((double) theSrcRect.mWidth * 65536.0 / theWidth);
			int aStepY = (int) ((double) theSrcRect.mHeight * 65536.0 / theHeight);
			int aSrcY = theSrcRect.mY * 65536;
			for (int y = 0; y < theHeight; y++)
			{
				ulong* aSrcRow = aSrcBits + (aSrcY >> 16) * theSource->mWidth;
				int aSrcX = theSrcRect.mX * 65536;
				for (int x = 0; x < theWidth; x++)
				{
					*aDestBits++ = aSrcRow[aSrcX >> 16];
					aSrcX += aStepX;
				}
				aSrcY += aStepY;
			}
			anEntry->mCopy->BitsChanged();
		}
		else
			anEntry->mCopy->StretchCopy(theSource, Rect(0, 0, theWidth, theHeight),
				FRect(theSrcRect.mX, theSrcRect.mY, theSrcRect.mWidth, theSrcRect.mHeight));
	}
	return anEntry->mCopy;
}

void DDInterface::ClearStretchCopies()
{
	std::vector<StretchCopyEntry> anEntries;
	anEntries.swap(mStretchCopies);
	for (StretchCopyEntry& e : anEntries)
		delete e.mCopy;
	std::vector<TransformCopyEntry> aCopies;
	aCopies.swap(mTransformCopies);
	for (TransformCopyEntry& e : aCopies)
		delete e.mCopy;
}

MemoryImage* DDInterface::GetTransformCopy(int theWidth, int theHeight)
{
	int aWidth = (theWidth + 15) & ~15;
	int aHeight = (theHeight + 15) & ~15;
	mStretchUse++;
	for (TransformCopyEntry& e : mTransformCopies)
	{
		if ((e.mCopy->mWidth == aWidth) && (e.mCopy->mHeight == aHeight))
		{
			e.mLastUse = mStretchUse;
			return e.mCopy;
		}
	}
	long long aBytes = (long long) aWidth * aHeight * 4;
	for (const TransformCopyEntry& e : mTransformCopies)
		aBytes += (long long) e.mCopy->mWidth * e.mCopy->mHeight * 4;
	while ((mTransformCopies.size() >= kTransformCacheSize) || ((aBytes > kCopyCacheBytes) && !mTransformCopies.empty()))
	{
		size_t anOldest = 0;
		for (size_t i = 1; i < mTransformCopies.size(); i++)
		{
			if (mTransformCopies[i].mLastUse < mTransformCopies[anOldest].mLastUse)
				anOldest = i;
		}
		MemoryImage* aCopy = mTransformCopies[anOldest].mCopy;
		aBytes -= (long long) aCopy->mWidth * aCopy->mHeight * 4;
		mTransformCopies.erase(mTransformCopies.begin() + anOldest);
		delete aCopy;
	}
	TransformCopyEntry e;
	e.mCopy = new MemoryImage(mApp);
	e.mCopy->Create(aWidth, aHeight);
	e.mCopy->mRotateRowsByImageWidth = true;
	e.mLastUse = mStretchUse;
	mTransformCopies.push_back(e);
	return e.mCopy;
}

void DDInterface::BltRotated(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect* theClipRect, const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY)
{
	SexyTransform2D aTransform;
	aTransform.Translate(-theRotCenterX, -theRotCenterY);
	aTransform.RotateRad((float) theRot);
	aTransform.Translate(theX + theRotCenterX, theY + theRotCenterY);

	bool aLinear = (theRot != 0) || IsFractional(theX) || IsFractional(theY);
	BltTransformed(theImage, theClipRect, theColor, theDrawMode, theSrcRect, aTransform, aLinear, 0, 0, false);
}

// The transform maps the source rectangle's corners, relative to its top-left corner (or to its
// center when 'center' is set, as used by BltMatrix), then (theX, theY) is added.
void DDInterface::BltTransformed(Image* theImage, const Rect* theClipRect, const Color& theColor, int theDrawMode, const Rect& theSrcRect, const SexyMatrix3& theTransform, bool linearFilter, float theX, float theY, bool center)
{
	float aStartX = 0;
	float aStartY = 0;
	if (center)
	{
		aStartX = -theSrcRect.mWidth / 2.0f;
		aStartY = -theSrcRect.mHeight / 2.0f;
	}

	float aRight = aStartX + theSrcRect.mWidth;
	float aBottom = aStartY + theSrcRect.mHeight;
	const SexyVector2 aCorners[4] = { SexyVector2(aStartX, aStartY), SexyVector2(aRight, aStartY), SexyVector2(aRight, aBottom), SexyVector2(aStartX, aBottom) };

	float aX[4];
	float aY[4];
	for (int i = 0; i < 4; i++)
	{
		SexyVector2 aPoint = theTransform * aCorners[i];
		aX[i] = aPoint.x + theX;
		aY[i] = aPoint.y + theY;
	}

	DrawTexturedQuad(theImage, aX, aY, theSrcRect, false, theColor, theDrawMode, theClipRect, linearFilter);
}

// TriVertex u/v are normalized over the whole image; a non-zero vertex colour (ARGB) replaces theColor
// (as in D3DInterface::DrawTrianglesTex).
void DDInterface::DrawTrianglesTex(Image* theTexture, const TriVertex theVertices[][3], int theNumTriangles, const Rect* theClipRect, const Color& theColor, int theDrawMode, float tx, float ty, bool blend)
{
	MemoryImage* aSrcImage = dynamic_cast<MemoryImage*>(theTexture);
	if ((aSrcImage == NULL) || (theNumTriangles <= 0))
		return;

	if (!PreDraw())
		return;

	DDTextureData* aData = GetTextureData(aSrcImage);
	if (aData == NULL)
		return;

	SDL_Texture* aTexture = PrepareTexture(aSrcImage, aData, theDrawMode, blend);
	if (aTexture == NULL)
		return;

	if (!SetClip(theClipRect))
		return;

	float aUScale = aSrcImage->mWidth * aData->mScale / (float) aData->mWidth;
	float aVScale = aSrcImage->mHeight * aData->mScale / (float) aData->mHeight;
	SDL_Color aDefaultColor = MakeVertexColor(theColor, mPremultiplied);
	float aScale = (float) mRenderScale;

	gVertices.resize(theNumTriangles * 3);
	SDL_Vertex* aVertex = &gVertices[0];
	for (int i = 0; i < theNumTriangles; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			const TriVertex& aTriVertex = theVertices[i][j];
			DWORD aVertexColor = aTriVertex.color;
			SDL_Color aColor = (aVertexColor != 0) ?
				MakeVertexColor((aVertexColor >> 16) & 0xFF, (aVertexColor >> 8) & 0xFF, aVertexColor & 0xFF, (aVertexColor >> 24) & 0xFF, mPremultiplied) :
				aDefaultColor;

			// +0.5: D3D7 and the software rasterizer both take triangle vertices without the half-pixel
			// correction that Blt uses
			SetVertex(*aVertex++, (aTriVertex.x + tx + 0.5f) * aScale, (aTriVertex.y + ty + 0.5f) * aScale, aColor, aTriVertex.u * aUScale, aTriVertex.v * aVScale);
		}
	}

	SDL_RenderGeometry(mRenderer, aTexture, &gVertices[0], theNumTriangles * 3, NULL, 0);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Untextured rectangles (logical coordinates) in the draw colour. SDL_RenderFillRectsF supports the
// additive mode on every renderer, including the software fallback.
static std::vector<SDL_FRect> gRects;

static void AddRect(float theX, float theY, float theWidth, float theHeight, float theScale)
{
	SDL_FRect aRect = { theX * theScale, theY * theScale, theWidth * theScale, theHeight * theScale };
	gRects.push_back(aRect);
}

void DDInterface::FillRects(const Color& theColor, int theDrawMode)
{
	if (!gRects.empty())
	{
		SDL_Color aColor = MakeVertexColor(theColor, mPremultiplied);
		SDL_SetRenderDrawBlendMode(mRenderer, (SDL_BlendMode) ((theDrawMode == Graphics::DRAWMODE_ADDITIVE) ? mBlendAdditive : mBlendNormal));
		SDL_SetRenderDrawColor(mRenderer, aColor.r, aColor.g, aColor.b, aColor.a);
		SDL_RenderFillRectsF(mRenderer, &gRects[0], (int) gRects.size());
	}

	gRects.clear();
}

void DDInterface::FillRect(const Rect& theRect, const Color& theColor, int theDrawMode, const Rect* theClipRect)
{
	if ((theRect.mWidth <= 0) || (theRect.mHeight <= 0))
		return;

	if ((!PreDraw()) || (!SetClip(theClipRect)))
		return;

	gRects.clear();
	AddRect((float) theRect.mX, (float) theRect.mY, (float) theRect.mWidth, (float) theRect.mHeight, (float) mRenderScale);
	FillRects(theColor, theDrawMode);
}

void DDInterface::FillPoly(const Point theVertices[], int theNumVertices, const Rect* theClipRect, const Color& theColor, int theDrawMode, int tx, int ty)
{
	if (theNumVertices < 3)
		return;

	if ((!PreDraw()) || (!SetClip(theClipRect)))
		return;

	SDL_SetRenderDrawBlendMode(mRenderer, (SDL_BlendMode) ((theDrawMode == Graphics::DRAWMODE_ADDITIVE) ? mBlendAdditive : mBlendNormal));

	SDL_Color aColor = MakeVertexColor(theColor, mPremultiplied);
	float aScale = (float) mRenderScale;

	gVertices.resize(theNumVertices);
	for (int i = 0; i < theNumVertices; i++)
		SetVertex(gVertices[i], (theVertices[i].mX + tx) * aScale, (theVertices[i].mY + ty) * aScale, aColor, 0, 0);

	// triangle fan
	gIndices.resize((theNumVertices - 2) * 3);
	for (int i = 0; i < theNumVertices - 2; i++)
	{
		gIndices[i * 3] = 0;
		gIndices[i * 3 + 1] = i + 1;
		gIndices[i * 3 + 2] = i + 2;
	}

	SDL_RenderGeometry(mRenderer, NULL, &gVertices[0], theNumVertices, &gIndices[0], (theNumVertices - 2) * 3);
}

// Lines set the same pixels as the software MemoryImage::DrawLine: horizontal and vertical lines are
// rectangles, other lines use its Bresenham stepping. Each pixel is a logical 1x1 rectangle, so lines
// stay one logical pixel wide at any render scale. The end points were already clipped by Graphics.
// The 1.3 3D path also draws anti-aliased lines this way.
void DDInterface::DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode, bool antiAlias)
{
	if (!PreDraw())
		return;

	SetClip(NULL);

	float aScale = (float) mRenderScale;
	gRects.clear();

	if (theStartY == theEndY)
	{
		int aStartX = (int) std::min(theStartX, theEndX);
		int aEndX = (int) std::max(theStartX, theEndX);
		AddRect((float) aStartX, (float) (int) theStartY, (float) (aEndX - aStartX + 1), 1, aScale);
		FillRects(theColor, theDrawMode);
		return;
	}
	else if (theStartX == theEndX)
	{
		int aStartY = (int) std::min(theStartY, theEndY);
		int aEndY = (int) std::max(theStartY, theEndY);
		AddRect((float) (int) theStartX, (float) aStartY, 1, (float) (aEndY - aStartY + 1), aScale);
		FillRects(theColor, theDrawMode);
		return;
	}

	double aMinX = std::min(theStartX, theEndX);
	double aMinY = std::min(theStartY, theEndY);
	double aMaxX = std::max(theStartX, theEndX);
	double aMaxY = std::max(theStartY, theEndY);

	double dv = theEndY - theStartY;
	double dh = theEndX - theStartX;
	int G, DeltaG1, DeltaG2;
	int inc = 1;
	int aCurX;
	int aCurY;

	if (fabs(dv) < fabs(dh))
	{
		// Mostly horizontal
		if (dh < 0)
		{
			dh = -dh;
			dv = -dv;
			std::swap(theStartX, theEndX);
			std::swap(theStartY, theEndY);
		}
		if (dv < 0)
		{
			dv = -dv;
			inc = -1;
		}

		AddRect((float) (int) theStartX, (float) (int) theStartY, 1, 1, aScale);

		aCurY = (int) theStartY;
		aCurX = (int) (theStartX + 1);

		G = (int) (2 * dv - dh);
		DeltaG1 = (int) (2 * (dv - dh));
		DeltaG2 = (int) (2 * dv);

		G = (int) (G + DeltaG2 * (theStartY - (int) theStartY));

		while (aCurX <= theEndX)
		{
			if (G > 0)
			{
				G += DeltaG1;
				aCurY += inc;

				if (aCurX<aMinX || aCurY<aMinY || aCurX>aMaxX || aCurY>aMaxY)
					break;
			}
			else
				G += DeltaG2;

			AddRect((float) aCurX, (float) aCurY, 1, 1, aScale);
			aCurX++;
		}
	}
	else
	{
		// Mostly vertical
		if (dv < 0)
		{
			dh = -dh;
			dv = -dv;
			std::swap(theStartX, theEndX);
			std::swap(theStartY, theEndY);
		}
		if (dh < 0)
		{
			dh = -dh;
			inc = -1;
		}

		AddRect((float) (int) theStartX, (float) (int) theStartY, 1, 1, aScale);

		aCurX = (int) theStartX;
		aCurY = (int) (theStartY + 1);

		G = (int) (2 * dh - dv);
		DeltaG1 = (int) (2 * (dh - dv));
		DeltaG2 = (int) (2 * dh);

		G = (int) (G + DeltaG2 * (theStartX - (int) theStartX));

		while (aCurY <= theEndY)
		{
			if (G > 0)
			{
				G += DeltaG1;
				aCurX += inc;

				if (aCurX<aMinX || aCurY<aMinY || aCurX>aMaxX || aCurY>aMaxY)
					break;
			}
			else
				G += DeltaG2;

			AddRect((float) aCurX, (float) aCurY, 1, 1, aScale);
			aCurY++;
		}
	}

	FillRects(theColor, theDrawMode);
}

void DDInterface::FillScanLines(const Span* theSpans, int theSpanCount, const Color& theColor, int theDrawMode)
{
	if (theSpanCount <= 0)
		return;

	if (!PreDraw())
		return;

	SetClip(NULL);

	float aScale = (float) mRenderScale;
	gRects.clear();
	for (int i = 0; i < theSpanCount; i++)
	{
		const Span& aSpan = theSpans[i];
		if (aSpan.mWidth > 0)
			AddRect((float) aSpan.mX, (float) aSpan.mY, (float) aSpan.mWidth, 1, aScale);
	}

	FillRects(theColor, theDrawMode);
}

void DDInterface::ClearRect(const Rect& theRect)
{
	if ((theRect.mWidth <= 0) || (theRect.mHeight <= 0))
		return;

	if (!PreDraw())
		return;

	// the screen has no alpha: clearing gives opaque black
	SetClip(NULL);
	SDL_SetRenderDrawBlendMode(mRenderer, SDL_BLENDMODE_NONE);
	SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, 255);
	SDL_Rect aRect = { theRect.mX * mRenderScale, theRect.mY * mRenderScale, theRect.mWidth * mRenderScale, theRect.mHeight * mRenderScale };
	SDL_RenderFillRect(mRenderer, &aRect);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool DDInterface::ReadScreenFull(std::vector<uint32_t>& theBits, int& theWidth, int& theHeight)
{
	if (!PreDraw())
		return false;
	theWidth = mWidth * mRenderScale;
	theHeight = mHeight * mRenderScale;
	theBits.resize((size_t) theWidth * theHeight);
	if (SDL_RenderReadPixels(mRenderer, NULL, SDL_PIXELFORMAT_ARGB8888, &theBits[0], theWidth * 4) != 0)
		return false;
	for (size_t i = 0; i < theBits.size(); i++)
		theBits[i] |= 0xFF000000;
	return true;
}

bool DDInterface::ReadScreen(ulong* theBits)
{
	if ((theBits == NULL) || (!PreDraw()))
		return false;

	int aScale = mRenderScale;
	int aWidth = mWidth * aScale;
	int aHeight = mHeight * aScale;

	gReadBuffer.resize(aWidth * aHeight);
	if (SDL_RenderReadPixels(mRenderer, NULL, SDL_PIXELFORMAT_ARGB8888, &gReadBuffer[0], aWidth * 4) != 0)
		return false;

	if (aScale == 1)
	{
		for (int i = 0; i < mWidth * mHeight; i++)
			theBits[i] = gReadBuffer[i] | 0xFF000000;
		return true;
	}

	// box-downsample to the logical size
	int aCount = aScale * aScale;
	for (int y = 0; y < mHeight; y++)
	{
		for (int x = 0; x < mWidth; x++)
		{
			uint32_t r = 0, g = 0, b = 0;
			for (int sy = 0; sy < aScale; sy++)
			{
				const uint32_t* aRow = &gReadBuffer[(y * aScale + sy) * aWidth + x * aScale];
				for (int sx = 0; sx < aScale; sx++)
				{
					uint32_t aPixel = aRow[sx];
					r += (aPixel >> 16) & 0xFF;
					g += (aPixel >> 8) & 0xFF;
					b += aPixel & 0xFF;
				}
			}

			theBits[y * mWidth + x] = 0xFF000000 | ((r / aCount) << 16) | ((g / aCount) << 8) | (b / aCount);
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void DDInterface::SetCursorImage(Image* theImage)
{
	mCursorImage = theImage;
}

void DDInterface::SetCursorPos(int theX, int theY)
{
	mCursorX = theX;
	mCursorY = theY;
}
