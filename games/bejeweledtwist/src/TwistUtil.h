// The game's small helpers (0x756e97-0x757751): a signed random number and the ways it draws an image about a point.
// They draw through gGraphicsRecorder.
#pragma once

#include "Rect.h"

namespace Sexy {

class Graphics;
class Image;

// 0x756e97: one Rand() draw as a number from -1 to 1 in steps of 1/500000
float RandSigned();

// How DrawImageScaledCentered places the image against theX, theY
enum {
	ALIGN_CENTER_X = 0x01,
	ALIGN_CENTER_Y = 0x02,
	ALIGN_BOTTOM = 0x04,  // without ALIGN_CENTER_Y
	ALIGN_RIGHT = 0x10,   // without ALIGN_CENTER_X
	ALIGN_CENTER = ALIGN_CENTER_X | ALIGN_CENTER_Y,
};

// 0x756ee7: the image scaled by theScaleX, theScaleY and placed by theAlign
void DrawImageScaledCentered(Graphics* g, Image* theImage, float theX, float theY, int theAlign, float theScaleX,
	float theScaleY);
// 0x757751: the same, centred
void DrawImageScaledCentered(Graphics* g, Image* theImage, float theX, float theY, float theScaleX, float theScaleY);

// 0x757137: theSrcRect of the image centred on theX, theY, scaled and turned by theRot (radians). Unturned, it is
// placed by the size of one cel, whatever theSrcRect's size.
void DrawImageCentered(Graphics* g, Image* theImage, const Rect& theSrcRect, float theX, float theY, double theRot,
	float theScaleX, float theScaleY);
// 0x7575c2: the same with the whole image
void DrawImageCentered(Graphics* g, Image* theImage, float theX, float theY, double theRot, float theScaleX,
	float theScaleY);

}  // namespace Sexy
