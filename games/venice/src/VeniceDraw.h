// The drawing the scene objects use: a rectangle of a picture drawn with a position, rotation, scale and colour, the clip, the
// blend mode and the overlap tests that decide what is on the screen.

#ifndef VENICE_DRAW_H
#define VENICE_DRAW_H

class ImageRecord;

// the z order the sparkles and particles are drawn at
const float ParticleZOrder = 9.0f;

// 0x441880: whether the display is accelerated (the glows, shadows and the half-pixel offsets only exist on one that is)
bool IsAccelerated();
// 0x441a20: whether the game is in a window (the objects that only show in full screen are hidden then)
bool IsWindowedOnScreen();
// 0x4418a0: turns the accelerated display on or off. A display that cannot be accelerated, or should not be, shows its message
// screen instead.
void SetAcceleratedDisplay(bool theAccelerated);
// 0x441930: puts the game in a window or on the full screen. A desktop no bigger than the game's screen has no room for a
// window, and its message screen shows instead.
void SetWindowedDisplay(bool theWindowed);
// 0x441de0: the blend mode of what is drawn next, 0 for normal and 1 for additive
void SetBlitStyleOnTarget(int theBlitStyle);
// 0x441a60: the clip to the rectangle between the corners, cut to the screen's size
void SetDrawClip(int theLeft, int theTop, int theRight, int theBottom);
// 0x442080: the source rectangle of a picture drawn with its centre at (theX + half the width, theY + half the height)
// turned and scaled about it; a flip is a negative scale. The colour (0 to 1 each) is applied unless all four are 1.
void DrawImageRecord(ImageRecord* theImage, float theSourceLeft, float theSourceTop, float theSourceRight,
	float theSourceBottom, float theX, float theY, float theRotation, float theXScale, float theYScale, float theRed,
	float theGreen, float theBlue, float theAlpha, bool theFlipX, bool theFlipY);
// 0x4416c0: the rectangle between the corners, filled with one colour and outlined in another (0 to 1 each)
void DrawBox(int theLeft, int theTop, int theRight, int theBottom, float theFillRed, float theFillGreen, float theFillBlue,
	float theFillAlpha, float theLineRed, float theLineGreen, float theLineBlue, float theLineAlpha);
// 0x441650: the whole screen painted black (before a scene that is scaled, turned or faded is drawn over nothing)
void FillScreenBlack();
// 0x438db0: whether two rectangles given by their centres and sizes overlap
bool RectsOverlap(float theX1, float theY1, float theWidth1, float theHeight1, float theX2, float theY2, float theWidth2,
	float theHeight2);
// 0x438ff0: the same for two rectangles that are turned (in degrees) about their centres: whether a corner of either is inside
// the other
bool RotatedRectsOverlap(float theX1, float theY1, float theWidth1, float theHeight1, float theRotation1, float theX2,
	float theY2, float theWidth2, float theHeight2, float theRotation2);

#endif
