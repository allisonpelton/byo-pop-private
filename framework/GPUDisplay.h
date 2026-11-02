#ifndef __GPUDISPLAY_H__
#define __GPUDISPLAY_H__

// port: the display of a game that draws through framework/GPUInterface.h in its 3D mode (Bejeweled Twist's Direct3D 9
// path). It stands in for DDInterface (SexyAppBase::CreateDDInterface) for a whole run, since the original fixes the
// mode for a run (a 3D change restarts the game); a game's 2D runs keep DDInterface.
//
// It owns the GL context, the device and GPU2D. The screen is the device's screen target: every draw whose destination
// is the screen image, or a DDImage with flag 0x10 (a render target), goes to GPU2D after CheckDestination has bound
// that destination, as Twist's Check3D (0x404a70) does through SetRenderTarget (slot 0x98); every other draw stays in
// software. Present ends the scene, scales the screen into the window as DDInterface does (render scale 1, the same
// letterboxing), draws the custom cursor over it and swaps.
//
// Not carried, and logged: the original's purge of a render target's bits and of a nobits image's after its first
// texture (the port keeps every image's bits, MemoryImage::PurgeBits), so GetBits of a nobits image gives its own bits
// rather than its quantised texture; FillScanLinesWithCoverage's DirectDraw lock (drawn as DDInterface does, through
// a temporary image); DDImage::Blt's DirectDraw short-cut (0x412c30), a plain Blt here; ClearRect on the screen, which
// clears the screen image's bits in the original and so shows nothing.

#include "DDInterface.h"
#include "GPUInterface.h"

namespace Sexy
{

class GLContext;
class GPU2D;

class GPUDisplay : public DDInterface
{
public:
	GPUDisplay(SexyAppBase* theApp, const GPUPolicy& thePolicy);
	~GPUDisplay() override;

	GPUPolicy mPolicy;
	GLContext* mContext;
	GPUInterface* mDevice;
	GPU2D* mGPU2D;

	int Init(SDL_Window* theWindow, bool vsync) override;
	void Cleanup() override;
	void UpdateDisplaySize() override;
	void ReleaseTextures() override;

	void BeginFrame() override;
	void Present() override;

	void RemoveImageData(MemoryImage* theImage) override;
	bool CheckDestination(DDImage* theImage) override;

	void FillRect(const Rect& theRect, const Color& theColor, int theDrawMode, const Rect* theClipRect) override;
	void FillPoly(const Point theVertices[], int theNumVertices, const Rect* theClipRect, const Color& theColor,
		int theDrawMode, int tx, int ty) override;
	void DrawLine(double x1, double y1, double x2, double y2, const Color& theColor, int theDrawMode,
		bool antiAlias) override;
	void FillScanLines(const struct Span* theSpans, int theSpanCount, const Color& theColor, int theDrawMode) override;
	void Blt(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode,
		const Rect* theClipRect, bool mirror = false) override;
	void StretchBlt(Image* theImage, const FRect& theDestRect, const Rect& theSrcRect, const Rect* theClipRect,
		const Color& theColor, int theDrawMode, bool fastStretch, bool mirror = false) override;
	void BltRotated(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect* theClipRect,
		const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY) override;
	void BltTransformed(Image* theImage, const Rect* theClipRect, const Color& theColor, int theDrawMode,
		const Rect& theSrcRect, const SexyMatrix3& theTransform, bool linearFilter, float theX, float theY,
		bool center) override;
	void DrawTrianglesTex(Image* theTexture, const TriVertex theVertices[][3], int theNumTriangles,
		const Rect* theClipRect, const Color& theColor, int theDrawMode, float tx, float ty, bool blend) override;
	void ClearRect(const Rect& theRect) override;

	bool ReadScreen(ulong* theBits) override;
	bool ReadScreenFull(std::vector<uint32_t>& theBits, int& theWidth, int& theHeight) override;

private:
	void LogOnce(const char* theText);
	void DrawCursorGPU();
};

}  // namespace Sexy

#endif
