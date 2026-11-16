#ifndef __FEEDINGFRENZY_STUNBUBBLEFX_H__
#define __FEEDINGFRENZY_STUNBUBBLEFX_H__

// The burst that shows when the player's fish stuns every fish on the screen (StunBubbleFx, 0x134 bytes, constructor
// 0x41a5c0, vtable 0x4a4900): a bubble that grows from where the fish is while its particles play.

#include "ImageNode.h"
#include "Node.h"
#include "Particles.h"

namespace feedingfrenzy
{

class StunBubbleFx : public engine::Container
{
public:
	StunBubbleFx();  // 0x41a5c0

	// 0x41a390: the pictures, which join the stage
	void init();
	// 0x41a4f0: the bubble starts to grow where the fish is, with its sound
	void start(const engine::Point& thePlace);
	// 0x41a2a0: the bubble goes
	void stop();
	// 0x41a310
	void update(engine::UpdateInfo& theInfo) override;

private:
	engine::Ref<engine::ParticleSystem> mParticles;  // +0x11c
	engine::Ref<engine::ImageNode> mBubble;          // +0x120
	float mSecondsLeft;                              // +0x124
	float mDuration;                                 // +0x128
};

} // namespace feedingfrenzy

#endif
