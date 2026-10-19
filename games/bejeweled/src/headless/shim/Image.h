#ifndef __IMAGE_H__
#define __IMAGE_H__

// port(variant): headless shim of framework/Image.h for the verifier (games/bejeweled/docs/CODE_MODS_DESIGN.md
// 6.9). Not part of the original game. An image is only its size.

#include "Common.h"
#include "Color.h"
#include "Rect.h"

namespace Sexy
{

class Image
{
public:
	int					mWidth;
	int					mHeight;

	Image() : mWidth(0), mHeight(0) {}
	virtual ~Image() {}

	int					GetWidth() { return mWidth; }
	int					GetHeight() { return mHeight; }
};

}

#endif
