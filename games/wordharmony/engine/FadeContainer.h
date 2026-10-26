#ifndef __WORDHARMONY_ENGINE_FADECONTAINER_H__
#define __WORDHARMONY_ENGINE_FADECONTAINER_H__

// A container that fades in or out (0xc0 bytes, constructor 0x429b70, vtable 0x483708): a screen's groups and the
// screens themselves. A fade runs from the opacity it starts at towards a target over a time; a fade-out ends hidden.

#include "Node.h"

namespace engine
{

typedef void (*FadeCallback)(void* theArg);

class FadeContainer : public Container
{
public:
	FadeContainer();  // 0x429b70

	void update(UpdateInfo& theInfo) override;  // 0x70: 0x429a80
	// 0xd8: 0x429930, a fade in (to theTarget) or out (to 1 - theTarget) over theDuration seconds; 0 sets it at once
	virtual void fade(bool theIn, float theDuration, float theTarget);
	// 0xdc: 0x429a60, called once when a fade ends
	virtual void setCallback(FadeCallback theCallback, void* theArg);
	// 0xe0: 0x409d10
	virtual void clearCallback();

	float mTimeLeft;          // +0xa8
	float mDuration;          // +0xac
	bool mFadingIn;           // +0xb0
	FadeCallback mCallback;   // +0xb4
	void* mCallbackArg;       // +0xb8
	float mTarget;            // +0xbc
};

} // namespace engine

#endif
