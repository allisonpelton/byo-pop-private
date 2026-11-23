#ifndef __SPROUT2_ANIMATEDIMAGE_H__
#define __SPROUT2_ANIMATEDIMAGE_H__

// A component that plays the runs of an animation's frames (AnimatedImage, 0xcc bytes and its virtual base, vtables
// 0x55b914 and 0x55ba08 (feedingfrenzy2); its constructor 0x424bb0 (feedingfrenzy2)). It shows one frame of its
// AnimationData at a time, at its position less the frame's anchor, and steps through the current run at its rate:
// once, back and forth, or round and round. A run played once stops at its end and says so through its finished
// signal, with the run's number.

#include "AnimationData.h"
#include "Component.h"
#include "sigslot.h"

namespace engine
{

// How a run goes on at its end (+0xb4)
enum AnimationMode
{
	ANIMATION_MODE_ONCE = 0,      // stops, the finished signal sent
	ANIMATION_MODE_SHUTTLE = 1,   // turns back
	ANIMATION_MODE_LOOP = 2       // starts again from the other end
};

class AnimatedImage : public Component
{
public:
	// 0x424bb0 (feedingfrenzy2): no data, the first run at its first frame, stopped, at a rate of 1, named
	// "AnimatedImage"
	AnimatedImage();

	// 0xa0 (0x4464e0 (feedingfrenzy2)): the current frame at the state's origin, where the image shows
	void paint(DrawContext* theContext) override;
	// 0xa4 (0x4381a0 (feedingfrenzy2)): with time left on the frame, the time less the rate times the frame's; with
	// none, the frames advanced until some is left. The modifiers are not applied
	void update(UpdateContext& theContext) override;
	// 0xa8 (0x433970 (feedingfrenzy2)): the first frame's rectangle at the position less its anchor; nothing without
	// frames
	void computeBounds() override;

	// The data's frames played (inlined in Visual's 0x4314d0 (feedingfrenzy2))
	void setAnimationData(AnimationData* theData) { mData = theData; }
	AnimationData* getAnimationData() const { return mData; }
	// 0x46f8f0 (feedingfrenzy2): the run of the number from its first frame plus the offset, with the run's frame time
	// left; nothing without frames or for a number below 0
	void setRun(int theRun, int theOffset);
	int getRun() const { return mRun; }
	int getFrameIndex() const { return mFrame; }
	// 0x451ea0 (feedingfrenzy2)
	void setRate(float theRate) { mRate = theRate; }
	float getRate() const { return mRate; }
	float getTimeLeft() const { return mTimeLeft; }
	void setMode(int theMode) { mMode = theMode; }
	// 1 forwards, -1 back, 0 stopped
	void setDirection(int theDirection) { mDirection = theDirection; }
	int getDirection() const { return mDirection; }
	// 0x425b70 (feedingfrenzy2): stopped where it is, no longer updated, and the finished signal sent with the run
	void stop();

	// +0x98: a run played once came to its end, or the image was stopped, with the run
	sigslot::signal1<unsigned int> mFinished;

private:
	// 0x437c70 (feedingfrenzy2): a frame on in the direction, or at the run's end what its mode says
	void advance();

	Ref<AnimationData> mData;  // +0xa8
	int mRun;                  // +0xac
	int mFrame;                // +0xb0: the frame shown, in the data's frames
	int mMode;                 // +0xb4: an AnimationMode
	int mDirection;            // +0xb8
	float mRate;               // +0xbc: the frames' times are divided by it
	float mTimeLeft;           // +0xc0: the seconds the frame still shows, at the rate
};

} // namespace engine

#endif
