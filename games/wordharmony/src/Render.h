#ifndef __WORDHARMONY_RENDER_H__
#define __WORDHARMONY_RENDER_H__

// The port's side of the engine's images and render context: what the Windows layer did with DirectDraw surfaces and
// Sprout's software blitters (0x457410 the image, 0x45dcb0 the context, 0x457830-0x45dcb0 the blitters). Images are
// 32-bit ARGB; the context draws into the 800x600 screen (the framework's MemoryImage bits).

#include "engine/Font.h"
#include "engine/Graphics.h"

#include <cstdint>
#include <vector>

namespace wordharmony
{

class ScreenImage : public engine::Image
{
public:
	ScreenImage(int theWidth, int theHeight) : mWidth(theWidth), mHeight(theHeight), mBits(theWidth * theHeight), mSerial(nextSerial()) {}
	// 0x4572a0
	int getWidth() override { return mWidth; }
	// 0x4572b0
	int getHeight() override { return mHeight; }

	int mWidth;
	int mHeight;
	std::vector<uint32_t> mBits;  // ARGB
	uint32_t mSerial;             // port: a number no other image of the run has (what ScreenGraphics compares a frame's draw calls by)

private:
	static uint32_t nextSerial();
};

// 0x457510: a PNG or JPEG decoded, with the alpha of the mask's bytes when theMask is given; the alpha mode as the
// loader sets it (a mask: blend; a PNG with alpha: alpha test at 0; otherwise opaque). Null when it does not decode.
ScreenImage* decodeImage(const std::vector<uint8_t>& theData, const std::vector<uint8_t>* theMask);

class ScreenGraphics : public engine::Graphics
{
public:
	// 0x45dcb0
	ScreenGraphics(uint32_t* theBits, int theWidth, int theHeight);
	~ScreenGraphics() override;
	ScreenGraphics(const ScreenGraphics&) = delete;
	ScreenGraphics& operator=(const ScreenGraphics&) = delete;

	// Port (for speed): the draw calls of a frame are recorded, not drawn, and the screen is made at the frame's end. clear
	// starts the frame (the screen is black, nothing drawn); finish draws the tiles of the screen whose calls are not the last
	// frame's and tells whether any did (Render.cpp, "retained drawing")
	void clear();
	bool finish();

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

} // namespace wordharmony

#endif
