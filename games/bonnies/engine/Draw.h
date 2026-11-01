#ifndef __BONNIES_DRAW_H__
#define __BONNIES_DRAW_H__

// The graphics' image primitives and effects (0x424df0-0x428190). Positions and sizes are fractions of the screen, as
// in Graphics.h; colours are 0xAARRGGBB.

#include "Graphics.h"

namespace bonnies
{

// 0x424df0: a panel with rounded corners of theRadius pixels: theFill inside, an inner edge of theInner theInnerWidth
// pixels wide and an outer one of theOuter; the rectangle theInX0..theInY1 inside it is left alone
void drawPanel(float theX, float theY, float theW, float theH, int theRadius, uint32_t theFill, uint32_t theInner,
	int theInnerWidth, uint32_t theOuter, int theOuterWidth, float theInX0, float theInY0, float theInX1, float theInY1);
// 0x425130: a rectangle shaded across in theColor
void drawSweep(float theX, float theY, float theW, float theH, uint32_t theColor);
// 0x4253d0: theFrame brightened (theBright) around theCx, theCy
void drawFrameGlow(const Frame* theFrame, float theX, float theY, float theAlpha, int theBright, int theCx, int theCy,
	int theMode);
// 0x4259b0: theFrame in theColor (0x123456: in rainbow colours)
void drawFrameTinted(const Frame* theFrame, float theX, float theY, uint32_t theColor);
// 0x425d50: theFrame scaled and centred on theX, theY (its top left corner there with theCorner)
void drawFrameScaled(const Frame* theFrame, float theX, float theY, float theScale, float theAlpha, uint32_t theTint,
	int theCorner);
// 0x426290: drawFrameScaled turned by theAngle degrees
void drawFrameRotated(const Frame* theFrame, float theX, float theY, float theAngle, float theScale, float theAlpha,
	uint32_t theTint, int theCorner);
// 0x426f50: theFrame with its top left corner at theX, theY, faded by theAlpha (its own alpha ignored with theOpaque)
void drawFrame(const Frame* theFrame, float theX, float theY, float theAlpha, int theOpaque);
// 0x427340: theFrame turned by theTurn quarter turns
void drawFrameTurned(const Frame* theFrame, float theX, float theY, int theTurn);
// 0x4275c0: a line theThickness pixels wide (with theMinor, a line mostly across is drawn along its shorter extent)
void drawLine(float theX1, float theY1, float theX2, float theY2, uint32_t theColor, int theThickness, int theMinor);
// 0x427ab0: darkens the screen towards its corners
void drawVignette(float theAmount);
// 0x427c70: a frame theBorder pixels wide coloured by angle from theColor1 through theColor2 to theColor3
void drawAngleFrame(float theX, float theY, float theW, float theH, uint32_t theColor1, uint32_t theColor2,
	uint32_t theColor3, int theBorder, float theTurn);
// 0x428030, 0x4280f0: save the clip rectangle and narrow it (pixels, or screen fractions); 0x428150: restore it
void pushClip(int theX0, int theY0, int theX1, int theY1);
void pushClip(float theX0, float theY0, float theX1, float theY1);
void popClip();

} // namespace bonnies

#endif
