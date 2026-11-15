#ifndef __SPROUT_HOST_RENDER_H__
#define __SPROUT_HOST_RENDER_H__

// The port's side of the engine's images and render context: what the Windows layer did with DirectDraw surfaces and
// Sprout's software blitters (0x457410 (wordharmony) the image, 0x45dcb0 (wordharmony) the context,
// 0x457830 (wordharmony)-0x45dcb0 (wordharmony) the blitters). Images are 32-bit ARGB; the context draws into the
// 800x600 screen (the framework's MemoryImage bits).

#include "sprout/Font.h"
#include "sprout/Graphics.h"

#include <cstdint>
#include <vector>

namespace engine
{

class ScreenImage : public engine::Image
{
public:
	ScreenImage(int theWidth, int theHeight) : mWidth(theWidth), mHeight(theHeight), mBits(theWidth * theHeight), mSerial(nextSerial()) {}
	// 0x4572a0 (wordharmony)
	int getWidth() override { return mWidth; }
	// 0x4572b0 (wordharmony)
	int getHeight() override { return mHeight; }
	// The signal build's image operations (Image in Graphics.h): 0x4bea40 (pizzafrenzy), 0x4becb0 (pizzafrenzy),
	// 0x4be780 (pizzafrenzy), 0x4beec0 (pizzafrenzy), 0x4be830 (pizzafrenzy). Word Harmony's createScaled gives the
	// image itself back, as before.
	engine::Image* clone() override;
	void scale(float theScaleX, float theScaleY) override;
	engine::Image* createScaled(float theScaleX, float theScaleY) override;
	void fill(const engine::Color4& theColor) override;
	void applyFilter(engine::PixelFx& theFilter) override;

	int mWidth;
	int mHeight;
	std::vector<uint32_t> mBits;  // ARGB
	uint32_t mSerial;             // port: a number no other image of the run has (what ScreenGraphics compares a frame's draw calls by)

private:
	static uint32_t nextSerial();
};

// The signal build's graphics drawing into an image (App slot 0x7c, 0x4bcbd0 (pizzafrenzy), whose context draws into
// the image's surface at once): each call is drawn over the pixels the image has
class ImageGraphics : public engine::Graphics
{
public:
	explicit ImageGraphics(ScreenImage* theTarget) : mTarget(theTarget) {}
	void drawImage(engine::Image* theImage, const engine::IntRect& theSrc, float x, float y) override;
	using engine::Graphics::drawImage;
	void drawLine(const engine::Point& theFrom, const engine::Point& theTo) override;

private:
	ScreenImage* mTarget;
};

// 0x457510 (wordharmony): a PNG or JPEG decoded, with the alpha of the mask's bytes when theMask is given; the alpha
// mode as the loader sets it (a mask: blend; a PNG with alpha: alpha test at 0; otherwise opaque). Null when it does
// not decode.
ScreenImage* decodeImage(const std::vector<uint8_t>& theData, const std::vector<uint8_t>* theMask);

class ScreenGraphics : public engine::Graphics
{
public:
	// 0x45dcb0 (wordharmony)
	ScreenGraphics(uint32_t* theBits, int theWidth, int theHeight);
	~ScreenGraphics() override;
	ScreenGraphics(const ScreenGraphics&) = delete;
	ScreenGraphics& operator=(const ScreenGraphics&) = delete;

	// Port (for speed): the draw calls of a frame are recorded, not drawn, and the screen is made at the frame's end.
	// clear starts the frame (the screen is black, nothing drawn); finish draws the tiles of the screen whose calls are
	// not the last frame's and tells whether any did (Render.cpp, "retained drawing")
	void clear();
	bool finish();

	// port (for speed): what the last finish redrew, as rectangles of the screen (a run of redrawn tiles in a row is
	// one, and a run under one of the same width joins it), for a display that copies only what changed
	// (SurfaceDisplay)
	struct Rectangle
	{
		int mX, mY, mWidth, mHeight;
	};
	const std::vector<Rectangle>& redrawn() const;

	void drawImage(engine::Image* theImage, const engine::IntRect& theSrc, float x, float y) override;
	using engine::Graphics::drawImage;
	void drawLine(const engine::Point& theFrom, const engine::Point& theTo) override;

private:
	struct Retained;  // the frame's draw calls and the tiles' (Render.cpp)

	uint32_t* mBits;
	int mWidth;
	int mHeight;
	Retained* mRetained;
};

} // namespace engine

#endif
