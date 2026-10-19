#ifndef __MEMORYIMAGE_H__
#define __MEMORYIMAGE_H__

// port(variant): headless shim of framework/MemoryImage.h for the verifier
// (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9). Not part of the original game. The size, and bits only when
// something asks for them (the board's banner, ColorizeText, runs only from Draw).

#include "Image.h"

#include <stdint.h>
#include <vector>

namespace Sexy
{

class SexyAppBase;

class MemoryImage : public Image
{
public:
	std::vector<ulong>		mBits;
	std::vector<uint32_t>	mHDBits;
	int						mHDScale;

	MemoryImage() : mHDScale(1) {}
	MemoryImage(SexyAppBase* theApp) : mHDScale(1) {}

	virtual void			Create(int theWidth, int theHeight)
	{
		mWidth = theWidth;
		mHeight = theHeight;
		mBits.clear();
	}
	virtual ulong*			GetBits()
	{
		mBits.resize((size_t) mWidth * (size_t) mHeight + 1, 0);
		return &mBits[0];
	}
	virtual void			BitsChanged() {}
	virtual void			SetImageMode(bool hasTrans, bool hasAlpha) {}
	void					SetHDBits(const uint32_t* theBits, int theWidth, int theHeight, int theScale) {}
	bool					HasMirrorableHDBits() const { return false; }
};

}

#endif
