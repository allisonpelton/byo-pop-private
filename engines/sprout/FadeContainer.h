#ifndef __SPROUT_FADECONTAINER_H__
#define __SPROUT_FADECONTAINER_H__

// A container that fades in or out (0xc0 bytes, constructor 0x429b70 (wordharmony), vtable 0x483708 (wordharmony);
// 0x144 bytes, constructor 0x466ff0 (pizzafrenzy), vtable 0x501908 (pizzafrenzy)): a screen's groups and the screens
// themselves. A fade runs from the opacity it starts at towards a target over a time; a fade-out ends hidden. In the
// signal build a fade in starts from clear, and a fade out can take the container out of its parent when it ends.
// The port keeps the listener build's slot numbers below; the signal build's are each 0x54 further on.

#include "Node.h"

namespace engine
{

typedef void (*FadeCallback)(void* theArg);

class FadeContainer : public Container
{
public:
	FadeContainer();  // 0x429b70 (wordharmony)

	void update(UpdateInfo& theInfo) override;  // 0x70: 0x429a80 (wordharmony); 0x94: 0x466ef0 (pizzafrenzy)
	// 0xd8: 0x429930 (wordharmony); 0x12c: 0x466d70 (pizzafrenzy), a fade in (to theTarget) or out (to 1 - theTarget)
	// over theDuration seconds; 0 sets it at once. The signal build's takes whether a fade out ends the container
	// (the listener build's has no such argument: false)
	virtual void fade(bool theIn, float theDuration, float theTarget, bool theRemoveWhenHidden = false);
	// 0xdc: 0x429a60 (wordharmony); 0x130: 0x466eb0 (pizzafrenzy), called once when a fade ends
	virtual void setCallback(FadeCallback theCallback, void* theArg);
	// 0xe0: 0x409d10 (wordharmony); 0x134: 0x401140 (pizzafrenzy)
	virtual void clearCallback();
	// 0x138: 0x466ed0 (pizzafrenzy)
	virtual bool isFadeDone() { return mTimeLeft == 0.0f; }

	float mTimeLeft;          // +0xa8
	float mDuration;          // +0xac
	bool mFadingIn;           // +0xb0
	FadeCallback mCallback;   // +0xb4
	void* mCallbackArg;       // +0xb8
	float mTarget;            // +0xbc
	bool mRemoveWhenHidden;   // +0x140 (pizzafrenzy)
};

} // namespace engine

#endif
