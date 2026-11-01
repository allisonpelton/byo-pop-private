// The game's small helpers (0x756e97-0x757a41): a signed random number, the ways it draws an image about a point (through
// gGraphicsRecorder) and the colour of a font's layer.
#pragma once

#include "Color.h"
#include "Rect.h"

#include <string>

namespace Sexy {

class Font;
class Graphics;
class Image;

// port: logs once that theWhat is not ported yet
void NotPorted(const char* theWhat);

// 0x756e97: one Rand() draw as a number from -1 to 1 in steps of 1/500000
float RandSigned();
// 0x756ec2: one Rand() draw as a number from 0 to 1 in steps of 1/1000000
float RandFloat();

// 0xe03690, 0xe07690: sin and cos at 4096 steps round the circle, made once by the app's constructor (0x766ce7)
extern float gSinTable[4096];
extern float gCosTable[4096];
void MakeTrigTables();

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

// 0x7578ff: the colour of an ImageFont's layer, counted in the font's order (nothing if it has fewer)
void SetFontLayerColor(Font* theFont, int theLayer, const Color& theColor);
// 0x75796a: the colour of an ImageFont's layer, counted in the font's order (no colour at all if it has fewer)
Color GetFontLayerColor(Font* theFont, int theLayer);
// 0x757784: the colour of the font's layer named theName (case ignored), among the layers it draws
void SetFontLayerColor(Font* theFont, const std::string& theName, const Color& theColor);

}  // namespace Sexy
