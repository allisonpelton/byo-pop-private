// BlendedImage.h: a picture that can be drawn at a position between two pixels. It keeps the picture drawn at sixteen
// sub-pixel offsets (the fraction of x and of y in quarters), each made the first time it is wanted. The original uses it
// for the pegs, so a peg that moves does not jump from pixel to pixel.

#ifndef __BLENDEDIMAGE_H__
#define __BLENDEDIMAGE_H__

#include "Rect.h"

namespace Sexy
{

class Graphics;
class Image;
class MemoryImage;

class BlendedImage
{
public:
	BlendedImage();			// 0x406960
	virtual ~BlendedImage(); // 0x4069e0

	// Takes the part of a picture the rect names as the picture to draw (the copies made so far are dropped).
	void Create(Image* theSource, const Rect& theRect); // 0x4069b0
	void Create(Image* theSource);						 // 0x408ff0: the whole picture
	void DeleteImages();								 // 0x401e70
	void Draw(Graphics* g, float theX, float theY);		 // 0x401eb0

	Image* mSource;			 // +0x4: the picture the copies are made from (not owned)
	Rect mRect;				 // +0x8: the part of it that is drawn
	Image* mOffsetImages[4][4]; // +0x18: the copy for each quarter of x (the first index) and of y; null until it is drawn

private:
	void MakeOffsetImage(int theXQuarter, int theYQuarter); // 0x401d30
};

} // namespace Sexy

#endif
