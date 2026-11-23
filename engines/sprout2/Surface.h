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

// How a surface is drawn by default (its +0x14), as the loaders and a screen's alphaType set it
enum BlendMode
{
	BLEND_OPAQUE = 0,  // a JPEG, or a PNG without an alpha channel; alphaType "none"
	BLEND_TEST = 1,    // alphaType "alphaTest": a pixel drawn whole where its alpha passes the surface's threshold
	BLEND_ALPHA = 2,   // the alpha of a mask or of the PNG, the colours multiplied by it; alphaType "alphaBlend"
	BLEND_ADD = 3,     // alphaType "alphaAdd"
};

// Where a surface's anchor goes (0x18, 0x45f290 (feedingfrenzy2)), in pixels from its top left corner
enum Pivot
{
	PIVOT_UPPER_LEFT = 0,  // (0, 0), also for any other value above 6
	PIVOT_CENTER = 1,      // half the width and half the height
	PIVOT_BOTTOM = 2,      // half the width, the last row
	PIVOT_TOP = 3,         // half the width, the first row
	// 4 to 6: Talismania's 0x6a2270 (its data names them); Feeding Frenzy 2 never passes them (its slot treats them as 0)
	PIVOT_UPPER_RIGHT = 4, // the width, the first row
	PIVOT_LOWER_LEFT = 5,  // the first column, the last row
	PIVOT_LOWER_RIGHT = 6, // the width, the last row
};

class PixelFx;

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
	// 0x18 (0x45f290 (feedingfrenzy2)): the anchor put at a pivot
	void setPivot(int thePivot);
	// 0x1c (0x45f300) and 0x20 (0x443610) (feedingfrenzy2)
	void setAnchor(const Vector2f& theAnchor) { mAnchor = theAnchor; }
	const Vector2f& getAnchor() const { return mAnchor; }
	// 0x24 (0x462510) and 0x28 (0x440650) (feedingfrenzy2): the mode, and the alpha threshold of BLEND_TEST (kept as
	// a byte)
	void setBlendMode(int theBlendMode, int theAlphaThreshold)
	{
		mBlendMode = theBlendMode;
		mAlphaThreshold = (uint8_t)theAlphaThreshold;
	}
	int getBlendMode() const { return mBlendMode; }
	int getAlphaThreshold() const { return mAlphaThreshold; }
	// 0x30 (0x427110 (feedingfrenzy2)): every pixel set to the colour, each channel times 255 truncated
	// 0x44 (0x45f320 (feedingfrenzy2)): a new surface with the pixels, blend mode, anchor and name of this one
	Surface* clone() const;
	void fill(const Color& theColor);
	// 0x40 (0x4267c0 (feedingfrenzy2)): the next, scaled the same both ways
	Surface* scaledCopy(float theScale) const { return scaledCopy(theScale, theScale); }
	// 0x3c (0x460530 (feedingfrenzy2)): a copy scaled, named "<name>(<x>,<y>)" with the scales to two places
	Surface* scaledCopy(float theScaleX, float theScaleY) const;
	// 0x48 (0x45b770 (feedingfrenzy2)): the pixels drawn again at the scales (their sizes taken), filtered, the size
	// the old one times each (truncated), the anchor moved with them
	void scale(float theScaleX, float theScaleY);
	// 0x54 (0x5079c2 (feedingfrenzy2)), through 0x50 (0x43e000 (feedingfrenzy2)) over the whole surface: each pixel
	// put through the effect, in place
	void applyPixelFx(const PixelFx& theFx);
	// 0x38 (0x43e4d0 (feedingfrenzy2)): a new surface of the size with each pixel put through the effect (by its
	// column and row), the blend mode and anchor of this one, named "<name>(<the effect's name>)"
	Surface* pixelFxCopy(const PixelFx& theFx) const;
	// 0x34 (0x426740 (feedingfrenzy2)): the same, then scaled the same both ways (the slot 0x4c, 0x47a550
	// (feedingfrenzy2)) and named "<its name>(<x>,<y>)" with the scale to two places
	Surface* pixelFxScaledCopy(float theScale, const PixelFx& theFx) const;

	// The pixels, ARGB, row after row (the object's +0x60 (feedingfrenzy2))
	uint32_t* getPixels() { return mPixels.data(); }
	const uint32_t* getPixels() const { return mPixels.data(); }

private:
	int mBlendMode;           // +0x14
	uint8_t mAlphaThreshold;  // +0x18
	Vector2f mAnchor;         // +0x1c
	std::string mName;        // +0x3c
	int mWidth;               // +0x58
	int mHeight;              // +0x5c
	std::vector<uint32_t> mPixels;  // +0x60
};

} // namespace engine

#endif
