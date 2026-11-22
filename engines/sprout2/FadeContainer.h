#ifndef __SPROUT2_FADECONTAINER_H__
#define __SPROUT2_FADECONTAINER_H__

// A container that fades its opacity in or out over a time (FadeContainer, vtable 0x55c1d4 (feedingfrenzy2) with 74
// slots, 0xdc bytes; its constructor 0x474990 (feedingfrenzy2)): a screen's <group>, and the base of a screen's
// layout. A fade in ends at the target opacity and makes the container visible as it starts; a fade out takes the
// target's share of the opacity away and hides the container at its end when the share is all of it.

#include "Container.h"

namespace engine
{

class FadeContainer : public Container
{
public:
	// What is called when a fade ends, with the data set beside it
	typedef void (*FadeCallback)(void* theData);

	// 0x474990 (feedingfrenzy2): animated, no fade under way
	FadeContainer();

	// 0x68 (0x523c0e (feedingfrenzy2))
	std::string getClassName() override { return "FadeContainer"; }
	// 0xa4 (0x43b150 (feedingfrenzy2)): the children, then the fade's opacity
	void update(UpdateContext& theContext) override;

	// 0x118 (0x48f760 (feedingfrenzy2)): a fade begun, unless one the same way is under way or the opacity is already
	// where it would end; a fade of no time ends at once
	virtual void fade(bool theFadeIn, float theDuration, float theTarget, bool theDeleteWhenDone);
	// 0x11c (0x50112e) and 0x120 (0x5022bb) (feedingfrenzy2)
	virtual void setFadeCallback(FadeCallback theCallback, void* theData);
	virtual void clearFadeCallback();
	// 0x124 (0x501118 (feedingfrenzy2))
	virtual bool isFadeDone() { return mFadeTime == 0.0f; }

protected:
	float mFadeTime;           // +0xb8: the seconds left
	float mFadeDuration;       // +0xbc
	bool mFadingIn;            // +0xc0
	FadeCallback mCallback;    // +0xc4
	void* mCallbackData;       // +0xc8
	float mFadeTarget;         // +0xcc: the opacity a fade in ends at, or the share a fade out takes away
	bool mDeleteWhenDone;      // +0xd0: a fade out that hides the container marks it for deletion too
};

} // namespace engine

#endif
