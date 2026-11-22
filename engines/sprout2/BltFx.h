#ifndef __SPROUT2_BLTFX_H__
#define __SPROUT2_BLTFX_H__

// An effect that changes how a component's images are copied to the target (engine::BltFx, vtable 0x560d88
// (feedingfrenzy2); its constructor 0x491ab0 (feedingfrenzy2)), and the one kind the screens make, a wave that shifts
// each row sideways (engine::WaveFx, vtable 0x560ce0 (feedingfrenzy2), 0x3c bytes; its constructor 0x491a20
// (feedingfrenzy2)). A screen's <bltFx> element names one, and a component's bltFx attribute sets it in the
// component's pixel effect (Component::setPixelFx).
//
// In the original a BltFx and a PixelFx share no base: the component's field holds either, and the blitters read the
// BltFx's type at +0x0c of whatever it holds. The port derives BltFx from PixelFx, so the field has one type, and the
// blitters ask for a BltFx by dynamic_cast.

#include "Object.h"
#include "PixelFx.h"

#include <vector>

namespace engine
{

class BltFx : public Object, public PixelFx
{
public:
	enum Type
	{
		TYPE_NONE = 0,
		TYPE_WAVE = 1   // a WaveFx
	};

	BltFx() : mType(TYPE_NONE) {}

	// 0x04 (0x5014fa (feedingfrenzy2)): nothing
	void update(float theElapsed) override {}

	int getType() const { return mType; }

protected:
	int mType;   // +0x0c
};

class WaveFx : public BltFx
{
public:
	// 0x491a20 (feedingfrenzy2): 20 rows a wave, 5 pixels wide, 2 seconds a wave
	WaveFx();

	// 0x04 (0x434ae0 (feedingfrenzy2)): the wave moved on by the time; the row it starts at follows
	void update(float theElapsed) override;

	// What the screen's handler (0x4195f0 (feedingfrenzy2)) sets, the table built again from it. The handler builds
	// it once more between the width and the height, which leaves nothing behind.
	void setShape(float thePeriod, float theWidth, int theHeight);

	// 0x514048 (feedingfrenzy2): the shift of a row of the image, in 1/256 pixels: the whole pixels above bit 8 and
	// the fraction's weight in the low byte
	int getRowShift(int theRow) const;

private:
	// 0x426c20 (feedingfrenzy2): one wave of a sine over the rows, from 0 to twice the width
	void buildTable();

	int mHeight;               // +0x10: the rows in one wave
	float mWidth;              // +0x14: half the largest shift, in pixels
	float mPeriod;             // +0x18: the seconds a wave takes to pass
	float mTime;               // +0x1c: the time into the current wave
	int mOffset;               // +0x20: the row of the table the image's first row takes
	std::vector<int> mShifts;  // +0x24: each row's shift, in 1/256 pixels
};

} // namespace engine

#endif
