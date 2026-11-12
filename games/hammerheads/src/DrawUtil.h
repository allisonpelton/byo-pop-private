#ifndef __HAMMERHEADS_DRAWUTIL_H__
#define __HAMMERHEADS_DRAWUTIL_H__

// Drawing helpers of the game's own that the framework (1.2) has no counterpart of.

namespace Sexy
{

class Graphics;
class Image;

// The exe's double to int conversion (_ftol, 0x527760): truncates, and anything out of the int range (or not a number) is
// INT_MIN as on x86
inline int ToInt(double theValue)
{
	if (!(theValue > -2147483649.0 && theValue < 2147483648.0))
		return -2147483647 - 1;
	return (int)theValue;
}

// 0x405000: one cel of a strip image, drawn at a position that is not a whole number (the position is rounded to a float
// and drawn with DrawImageF). Nothing is drawn for a cel outside the strip.
void DrawImageCelF(Graphics* g, Image* theImageStrip, double theX, double theY, int theCelCol, int theCelRow);

// 0x4052a0: a bar of an empty picture with a full one drawn over it as far as theFill (0 to 1) reaches
void DrawFillBar(Graphics* g, Image* theEmptyImage, Image* theFullImage, int theX, int theY, double theFill, int theMargin,
	int theFadeWidth);

}

#endif
