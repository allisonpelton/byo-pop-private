#ifndef __FEEDINGFRENZY_FISHATTACHFX_H__
#define __FEEDINGFRENZY_FISHATTACHFX_H__

// A particle effect that stays with a fish (FishAttachFx, 0x140 bytes, constructor 0x41ad50, vtable 0x4a4a98, destructor
// 0x41a940): the star bubble's sparkle, the shield bubble's glow and the pearl's glimmer. It stands at a place from the
// fish while the effect runs, and goes on updating after it has been stopped, until its particles are gone. The
// original's object has a point at +0x124 that nothing reads, and a flag at +0x135 that nothing sets (the effect joins
// the stage's group at every start).

#include "Font.h"
#include "Node.h"
#include "Particles.h"
#include <string>

namespace feedingfrenzy
{

class FishBase;

class FishAttachFx : public engine::Container
{
public:
	FishAttachFx();  // 0x41ad50

	// 0x41ac10: reads the particle file for the fish. The effect joins a group of the stage when it starts: the one of
	// the middle ground, or the one behind. (The original takes a node the streams' particles come out in as well; the
	// start sets it again, to the stage's play group, before any particle is made.)
	void init(const std::string& theFile, FishBase* theFish, const engine::Point& theOffset, bool theInMidGround);
	// 0x41aab0: the effect goes to its place, joins the stage and starts emitting, its particles in the play group's
	// coordinates
	void start();
	// 0x41aba0: the effect stops emitting; it keeps updating until the particles in the air are done
	void stop();
	// slot 0x7c: 0x41a9f0: while it runs the effect follows the fish; when it has been stopped and the particles are done
	// it stops updating
	void update(engine::UpdateInfo& theInfo) override;

private:
	engine::Ref<engine::ParticleSystem> mParticles;  // +0x11c
	FishBase* mFish;                                 // +0x120
	engine::Point mOffset;                           // +0x12c: from the fish's place
	bool mStopped;                                   // +0x134
	bool mInMidGround;                               // +0x136: the stage group it joins is the middle ground's
};

} // namespace feedingfrenzy

#endif
