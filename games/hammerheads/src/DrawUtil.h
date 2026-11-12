#ifndef __HAMMERHEADS_DRAWUTIL_H__
#define __HAMMERHEADS_DRAWUTIL_H__

// Drawing helpers of the game's own that the framework (1.2) has no counterpart of.

#include <vector>

#include "Point.h"

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

// 0x4051b0: whether a point of the hammer's mask lands on a solid pixel of a cel of a picture. theImagePos is where the picture's
// top left is (the exe passes it as doubles and truncates them), theXY the point the mask is laid at, theCelRow and
// theCelCol which cel of the strip is looked at. A pixel is solid when its alpha is not zero.
bool MaskHitsImage(const double* theImagePos, const int* theXY, const std::vector<Point>& theMask, Image* theImage,
	int theCelRow, int theCelCol);

}

#endif
