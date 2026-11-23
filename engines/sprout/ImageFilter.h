#ifndef __SPROUT_IMAGEFILTER_H__
#define __SPROUT_IMAGEFILTER_H__

// The signal build's image filter that shifts the colours (0x47b550 (pizzafrenzy); its object is made by
// 0x47b460 (pizzafrenzy) and set by 0x47b110 (pizzafrenzy)): each colour goes to hue, saturation and value
// (0x47b130 (pizzafrenzy)), the shifts are added, and it comes back to red, green and blue (0x47b2d0 (pizzafrenzy)).

#include <cstdint>

namespace engine
{

// What an image's slot 0x3c (0x4be830 (pizzafrenzy)) and 0x28 (0x4beb50 (pizzafrenzy)) call for each pixel (Iggle Pop's
// engine::PixelFx): its place, colour and alpha in, the new colour and alpha out
class PixelFx
{
public:
	virtual ~PixelFx() {}
	// slot 0
	virtual void filter(int x, int y, uint8_t theRed, uint8_t theGreen, uint8_t theBlue, uint8_t theAlpha,
	                    uint8_t* theNewRed, uint8_t* theNewGreen, uint8_t* theNewBlue, uint8_t* theNewAlpha) = 0;
};

class HsvShiftFilter : public PixelFx
{
public:
	// 0x47b460 (pizzafrenzy)
	HsvShiftFilter() : mHue(0), mSaturation(0), mValue(0) {}
	// 0x47b110 (pizzafrenzy)
	void setShift(float theHue, float theSaturation, float theValue);
	// 0x41a6b0 (feedingfrenzy): the three shifts, which Feeding Frenzy's fish name their shifted pictures by
	float getHue() const { return mHue; }
	float getSaturation() const { return mSaturation; }
	float getValue() const { return mValue; }
	// 0x47b550 (pizzafrenzy): the shifted colour of one (the alpha comes back as it was)
	void filter(uint8_t theRed, uint8_t theGreen, uint8_t theBlue, uint8_t theAlpha, uint8_t* theNewRed,
	            uint8_t* theNewGreen, uint8_t* theNewBlue, uint8_t* theNewAlpha) const;
	// 0x47b550 (pizzafrenzy) as the image calls it: the place is not read
	void filter(int x, int y, uint8_t theRed, uint8_t theGreen, uint8_t theBlue, uint8_t theAlpha, uint8_t* theNewRed,
	            uint8_t* theNewGreen, uint8_t* theNewBlue, uint8_t* theNewAlpha) override
	{
		filter(theRed, theGreen, theBlue, theAlpha, theNewRed, theNewGreen, theNewBlue, theNewAlpha);
	}

private:
	float mHue;         // +0x08, in degrees
	float mSaturation;  // +0x0c
	float mValue;       // +0x10
};

} // namespace engine

#endif
