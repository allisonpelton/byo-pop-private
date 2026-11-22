#ifndef __SPROUT2_MODIFIERS_H__
#define __SPROUT2_MODIFIERS_H__

// The modifiers a screen's <modifier> element makes (the handler 0x477d60 (feedingfrenzy2)), named in the
// original's RTTI outside the engine's namespace: a wave that rocks a component about its position (WaveModifier,
// vtable 0x55c43c (feedingfrenzy2), its constructor 0x431c20 (feedingfrenzy2)).

#include "Component.h"

namespace engine
{

class WaveModifier : public Object, public Modifier
{
public:
	// 0x431c20 (feedingfrenzy2): a period of 0 is taken as 1 second; the phase is where the wave's time starts
	WaveModifier(float theXPeriod, float theYPeriod, float theXAmplitude, float theYAmplitude, float thePhase);

	// 0x00 (0x43ccf0 (feedingfrenzy2)): the position moved by the change in each sine since the last frame
	void modify(UpdateContext& theContext, Component* theComponent) override;

	// (the two slots that set a period and an amplitude, 0x501107 and 0x5010f6 (feedingfrenzy2), are left out until
	// a caller is found)

private:
	float mXPeriod;      // +0x08: the seconds of one wave across
	float mYPeriod;      // +0x0c: and up and down
	float mXAmplitude;   // +0x10: in pixels
	float mYAmplitude;   // +0x14
	float mTime;         // +0x18
	float mXOffset;      // +0x1c: the offsets added so far
	float mYOffset;      // +0x20
};

} // namespace engine

#endif
