#ifndef __SPROUT2_SURFACE_H__
#define __SPROUT2_SURFACE_H__

// An image in memory (engine::Surface, the interface of 22 slots at vtable 0x56320c (feedingfrenzy2); its Windows
// implementations dxdriver::AbstractSurface and dxdriver::EngineSurface, vtable 0x562824 (feedingfrenzy2), 0x6c bytes).
// An engine surface keeps its pixels in system memory, 32-bit ARGB rows with no padding, and is drawn by the draw
// context's own blitters; the port keeps the three classes as one. A surface has a name (the file it came from), an
// anchor (the point drawn at the position given), and a blend mode saying how it is drawn by default. Each slot is
// added here as the code that calls it is translated.

#include "Geometry.h"
#include "Object.h"

#include <cstdint>
#include <string>
#include <vector>

namespace engine
{

// How a surface is drawn by default (its +0x14), as the loaders set it
enum BlendMode
{
	BLEND_OPAQUE = 0,  // a JPEG, or a PNG without an alpha channel
	BLEND_ALPHA = 2,   // the alpha of a mask or of the PNG, the colours multiplied by it
};

class Surface : public Object
{
public:
	// 0x461c60 (feedingfrenzy2), with AbstractSurface's 0x461cb0 (feedingfrenzy2): no pixels, no name
	Surface();
	~Surface() override;

	// 0x44d5e0 (feedingfrenzy2): the pixels made the size; the port clears them, where the original leaves the new
	// buffer as the allocation found it (every loader then writes each pixel)
	void allocate(int theWidth, int theHeight);

	// 0x45f530 (feedingfrenzy2), from the engine's loadImage and loadDiskImage: the surface named after the file and
	// read from it, a .jpg or a .png (the ending compared as written). A file "_name.png" beside "name.jpg" or
	// "name.png" is its alpha mask; without one, a PNG with an alpha channel is drawn blended and the rest opaque. An
	// image that cannot be opened leaves the surface empty
	void load(const std::string& thePath, bool theFromDisk);
	// 0x2c (0x463580 (feedingfrenzy2)): the alpha taken from a PNG mask, its grey or its own alpha, then the colours
	// multiplied by it when asked; the surface is then drawn blended
	void loadMask(const std::string& thePath, bool theFromDisk, bool thePremultiply);
	// 0x4754b0 (feedingfrenzy2), with the lock and unlock around it (0x46c500 (feedingfrenzy2)): each colour
	// multiplied by the pixel's alpha, rounded
	void premultiplyAlpha();

	// 0x00 (0x45f3e0) and 0x04 (0x45e780) (feedingfrenzy2)
	const std::string& getName() const { return mName; }
	void setName(const std::string& theName) { mName = theName; }
	// 0x08 (0x443760) and 0x0c (0x4437d0) (feedingfrenzy2)
	int getWidth() const { return mWidth; }
	int getHeight() const { return mHeight; }
	// 0x1c (0x45f300) and 0x20 (0x443610) (feedingfrenzy2)
	void setAnchor(const Vector2f& theAnchor) { mAnchor = theAnchor; }
	const Vector2f& getAnchor() const { return mAnchor; }
	// 0x24 (0x462510) and 0x28 (0x440650) (feedingfrenzy2)
	void setBlendMode(int theBlendMode, bool theBlendFlag)
	{
		mBlendMode = theBlendMode;
		mBlendFlag = theBlendFlag;
	}
	int getBlendMode() const { return mBlendMode; }

	// The pixels, ARGB, row after row (the object's +0x60 (feedingfrenzy2))
	uint32_t* getPixels() { return mPixels.data(); }
	const uint32_t* getPixels() const { return mPixels.data(); }

private:
	int mBlendMode;           // +0x14
	bool mBlendFlag;          // +0x18
	Vector2f mAnchor;         // +0x1c
	std::string mName;        // +0x3c
	int mWidth;               // +0x58
	int mHeight;              // +0x5c
	std::vector<uint32_t> mPixels;  // +0x60
};

} // namespace engine

#endif
