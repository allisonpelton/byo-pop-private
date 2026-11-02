// Sexy::GPUDisplay (GPUDisplay.h): DDInterface's place for a game's 3D mode, drawing through GPU2D and the device.
#include "GPUDisplay.h"
#include "DDImage.h"
#include "GPU2D.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"
#include "SexyMatrix.h"
#include "platform/GLContext.h"

#include <SDL.h>

#include <set>
#include <string>

using namespace Sexy;

static GPUDisplay* gGPUDisplay = NULL;

// GetBits of an image that has a texture but no bits (MemoryImage.h): the device's read-back
static bool RecoverBitsFromGPU(MemoryImage* theImage)
{
	return (gGPUDisplay != NULL) && (gGPUDisplay->mDevice != NULL) && gGPUDisplay->mDevice->RecoverBits(theImage);
}

GPUDisplay::GPUDisplay(SexyAppBase* theApp, const GPUPolicy& thePolicy) :
	DDInterface(theApp)
{
	mPolicy = thePolicy;
	mContext = NULL;
	mDevice = NULL;
	mGPU2D = NULL;
	mRenderScale = 1;
	mMaxTextureWidth = 4096;
	mMaxTextureHeight = 4096;
}

GPUDisplay::~GPUDisplay()
{
	// ~DDInterface's Cleanup is the base's: the GL state goes here
	Cleanup();
}

void GPUDisplay::LogOnce(const char* theText)
{
	static std::set<std::string> aLogged;
	if (aLogged.insert(theText).second)
		SDL_Log("GPUDisplay: %s", theText);
}

int GPUDisplay::Init(SDL_Window* theWindow, bool vsync)
{
	mWindow = theWindow;
	mVSync = vsync;
	if (mApp != NULL)
	{
		mWidth = mApp->mWidth;
		mHeight = mApp->mHeight;
	}
	if ((mWindow == NULL) || (mWidth <= 0) || (mHeight <= 0))
		return RESULT_FAIL;

	if (mContext == NULL)
	{
		mContext = GLContext::Create(mWindow);
		if (mContext == NULL)
			return RESULT_3D_FAIL;
		SDL_GL_SetSwapInterval(vsync ? 1 : 0);
		mDevice = GPUInterface::Create(mContext, mPolicy, mWidth, mHeight);
		if (mDevice == NULL)
		{
			delete mContext;
			mContext = NULL;
			return RESULT_3D_FAIL;
		}
		mGPU2D = new GPU2D(mDevice, mPolicy);
		mGPU2D->SetScreenSize(mWidth, mHeight);
		SDL_Log("GPUDisplay: %s", mContext->Description().c_str());
	}
	else
	{
		mDevice->Resize(mWidth, mHeight);
		mGPU2D->SetScreenSize(mWidth, mHeight);
	}
	gGPUDisplay = this;
	gMemoryImageRecoverBits = RecoverBitsFromGPU;

	mIs3D = true;
	if (mScreenImage == NULL)
	{
		mScreenImage = new DDImage(this);
		mScreenImage->mIsScreen = true;
	}
	if ((mScreenImage->mWidth != mWidth) || (mScreenImage->mHeight != mHeight))
		mScreenImage->Create(mWidth, mHeight);

	UpdateDisplaySize();
	return RESULT_OK;
}

void GPUDisplay::Cleanup()
{
	if (mScreenImage != NULL)
	{
		DDImage* aScreenImage = mScreenImage;
		mScreenImage = NULL;
		delete aScreenImage;
	}
	delete mGPU2D;
	mGPU2D = NULL;
	delete mDevice;
	mDevice = NULL;
	delete mContext;
	mContext = NULL;
	if (gGPUDisplay == this)
	{
		gGPUDisplay = NULL;
		gMemoryImageRecoverBits = NULL;
	}
	mWindow = NULL;
	mCursorImage = NULL;
}

void GPUDisplay::UpdateDisplaySize()
{
	if (mWindow == NULL)
		return;
	int aWidth = 0, aHeight = 0;
	SDL_GL_GetDrawableSize(mWindow, &aWidth, &aHeight);
	// Render scale 1: the screen is the original's back buffer at the logical size
	DisplayGeometry::UpdateDisplaySize(aWidth, aHeight, mMaxTextureWidth, mMaxTextureHeight, 1);
	mRenderScale = 1;
}

void GPUDisplay::ReleaseTextures()
{
	// No SDL renderer: a lost GL context is the device's ContextLost / ContextRestored
}

void GPUDisplay::BeginFrame()
{
	UpdateDisplaySize();
	if (mDevice != NULL)
		mDevice->SetSceneAllowed((mWindow == NULL) || ((SDL_GetWindowFlags(mWindow) & SDL_WINDOW_MINIMIZED) == 0));
}

void GPUDisplay::Present()
{
	if (mDevice == NULL)
		return;
	mGPU2D->EndScene();
	mDevice->Present(mPresentationRect, mDisplayWidth, mDisplayHeight);
	DrawCursorGPU();
	mContext->Swap();
	mFrameCount++;
}

void GPUDisplay::DrawCursorGPU()
{
	if ((!mCursorVisible) || (mCursorImage == NULL))
		return;
	float x, y, w, h;
	CursorRect(mCursorX, mCursorY, mCursorImage->mWidth, mCursorImage->mHeight, &x, &y, &w, &h);
	mDevice->PresentImage(mCursorImage, Rect(0, 0, mCursorImage->mWidth, mCursorImage->mHeight), x, y, w, h,
		mDisplayWidth, mDisplayHeight);
}

void GPUDisplay::RemoveImageData(MemoryImage* theImage)
{
	if (mGPU2D != NULL)
		mGPU2D->ImageDeleted(theImage);
	if (mDevice != NULL)
		mDevice->ImageDeleted(theImage);
}

// Check3D (0x404a70): the screen, or a render target (flag 0x10), answered by SetRenderTarget (slot 0x98)
bool GPUDisplay::CheckDestination(DDImage* theImage)
{
	if (mGPU2D == NULL)
		return false;
	if (theImage->mIsScreen)
		return mGPU2D->SetRenderTarget(NULL);
	if ((theImage->mD3DFlags & 0x10) != 0)
		return mGPU2D->SetRenderTarget(theImage);
	return false;
}

void GPUDisplay::FillRect(const Rect& theRect, const Color& theColor, int theDrawMode, const Rect* theClipRect)
{
	mGPU2D->FillRect(theRect, theColor, theDrawMode);
}

void GPUDisplay::FillPoly(const Point theVertices[], int theNumVertices, const Rect* theClipRect, const Color& theColor,
	int theDrawMode, int tx, int ty)
{
	mGPU2D->FillPoly(theVertices, theNumVertices, theClipRect, theColor, theDrawMode, tx, ty);
}

void GPUDisplay::DrawLine(double x1, double y1, double x2, double y2, const Color& theColor, int theDrawMode,
	bool antiAlias)
{
	// DrawLineAA comes to the same slot (0x64)
	mGPU2D->DrawLine(x1, y1, x2, y2, theColor, theDrawMode);
}

void GPUDisplay::FillScanLines(const Span* theSpans, int theSpanCount, const Color& theColor, int theDrawMode)
{
	// Image::FillScanLines (0x423e50): one FillRect a span
	for (int i = 0; i < theSpanCount; i++)
		mGPU2D->FillRect(Rect(theSpans[i].mX, theSpans[i].mY, theSpans[i].mWidth, 1), theColor, theDrawMode);
}

void GPUDisplay::Blt(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Color& theColor,
	int theDrawMode, const Rect* theClipRect, bool mirror)
{
	if (mirror)
		mGPU2D->BltMirror(theImage, theX, theY, theSrcRect, NULL, theColor, theDrawMode, false);
	else if (theClipRect != NULL)
		mGPU2D->BltF(theImage, theX, theY, theSrcRect, theClipRect, theColor, theDrawMode);
	else
		mGPU2D->Blt(theImage, (int) theX, (int) theY, theSrcRect, theColor, theDrawMode);
}

void GPUDisplay::StretchBlt(Image* theImage, const FRect& theDestRect, const Rect& theSrcRect, const Rect* theClipRect,
	const Color& theColor, int theDrawMode, bool fastStretch, bool mirror)
{
	Rect aDestRect((int) theDestRect.mX, (int) theDestRect.mY, (int) theDestRect.mWidth, (int) theDestRect.mHeight);
	mGPU2D->StretchBlt(theImage, aDestRect, theSrcRect, theClipRect, theColor, theDrawMode, fastStretch, mirror);
}

void GPUDisplay::BltRotated(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect* theClipRect,
	const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY)
{
	mGPU2D->BltRotated(theImage, theX, theY, theSrcRect, theClipRect, theColor, theDrawMode, theRot, theRotCenterX,
		theRotCenterY);
}

void GPUDisplay::BltTransformed(Image* theImage, const Rect* theClipRect, const Color& theColor, int theDrawMode,
	const Rect& theSrcRect, const SexyMatrix3& theTransform, bool linearFilter, float theX, float theY, bool center)
{
	mGPU2D->BltTransformed(theImage, theClipRect, theColor, theDrawMode, theSrcRect, theTransform, linearFilter, theX,
		theY, center);
}

void GPUDisplay::DrawTrianglesTex(Image* theTexture, const TriVertex theVertices[][3], int theNumTriangles,
	const Rect* theClipRect, const Color& theColor, int theDrawMode, float tx, float ty, bool blend)
{
	// Twist's slot 0x74 takes no clip rectangle
	mGPU2D->DrawTrianglesTex(theTexture, theVertices, theNumTriangles, theColor, theDrawMode, tx, ty, blend);
}

void GPUDisplay::ClearRect(const Rect& theRect)
{
	LogOnce("ClearRect on the screen (the original clears the screen image's bits, which shows nothing)");
}

bool GPUDisplay::ReadScreen(ulong* theBits)
{
	if (mDevice == NULL)
		return false;
	mGPU2D->Flush();
	MemoryImage anImage(mApp);
	if (!mDevice->ReadBack(NULL, &anImage))
		return false;
	memcpy(theBits, anImage.GetBits(), mWidth * mHeight * sizeof(ulong));
	return true;
}

bool GPUDisplay::ReadScreenFull(std::vector<uint32_t>& theBits, int& theWidth, int& theHeight)
{
	theWidth = mWidth;
	theHeight = mHeight;
	theBits.resize((size_t) mWidth * mHeight);
	return ReadScreen((ulong*) theBits.data());
}
