#ifndef __FEEDINGFRENZY_WAVEFX_H__
#define __FEEDINGFRENZY_WAVEFX_H__

// The water's waves on a stage's background (engine::WaveFx, constructor 0x44df20, 0x24 bytes, vtable 0x4a4e3c): a
// blit effect of type 1 that the stage keeps running: the stage sets the time and its two factors each frame, and the
// background is drawn through it.

#include "BltFx.h"

namespace feedingfrenzy
{

class WaveFx : public engine::BltFx
{
public:
	WaveFx();  // 0x44df20

	float getTime() const { return mTime; }       // 0x435730
	void setTime(float theTime) { mTime = theTime; }  // 0x44df10
	void setSize(float theSize) { mSize = theSize; }  // 0x41ae10
	void setAmount(float theAmount) { mAmount = theAmount; }  // 0x44def0

private:
	float mTime;    // +0x10
	float mSize;    // +0x14
	float mAmount;  // +0x18
};

} // namespace feedingfrenzy

#endif
