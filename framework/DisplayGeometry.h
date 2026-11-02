#ifndef __DISPLAYGEOMETRY_H__
#define __DISPLAYGEOMETRY_H__

// port: where the logical screen lands in the window, shared by every display (DDInterface, and GPUDisplay later). It
// makes no SDL calls: the display passes in the sizes SDL reports, so tools/geomtest drives it with a table.
//
// The logical screen (640x480 for Bejeweled) is letterboxed into the window's drawable, which is in pixels; on a HiDPI
// screen the window's own size, in points, is smaller, and mouse coordinates come in points.

#include "Rect.h"

namespace Sexy
{

struct DisplayGeometry
{
	int mWidth;   // logical size
	int mHeight;
	int mDisplayWidth;   // drawable size in pixels
	int mDisplayHeight;
	Rect mPresentationRect;  // where the logical screen lands inside the drawable

	DisplayGeometry();

	// Takes the drawable's size (ignored unless both are positive), letterboxes the logical screen into it and returns
	// the render scale: the next integer at or above the presentation scale, at most 4 (theForcedRenderScale instead
	// when it is above 0), lowered until the logical size times the scale fits the maximum texture size.
	int UpdateDisplaySize(int theDrawableWidth, int theDrawableHeight, int theMaxTextureWidth, int theMaxTextureHeight,
		int theForcedRenderScale);

	// Window coordinates (as SDL mouse events report them) to logical ones. theWindowWidth/Height is the window's size in
	// points, 0 when there is no window.
	void WindowToLogical(int theWindowWidth, int theWindowHeight, int theWindowX, int theWindowY, int* theX, int* theY) const;
	// The inverse, a game point to window coordinates (SexyAppBase::WarpMouse)
	void LogicalToWindow(int theWindowWidth, int theWindowHeight, int theX, int theY, int* theWindowX, int* theWindowY) const;

	// The cursor image's rectangle in the drawable: 1.3 centres it in a 64x64 area around the cursor's point, drawn at
	// the presentation scale. It is clipped to mPresentationRect.
	void CursorRect(int theCursorX, int theCursorY, int theImageWidth, int theImageHeight, float* theX, float* theY,
		float* theWidth, float* theHeight) const;
};

}

#endif
