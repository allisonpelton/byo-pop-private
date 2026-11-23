#ifndef __FEEDINGFRENZY_PLAYERSHIELDFX_H__
#define __FEEDINGFRENZY_PLAYERSHIELDFX_H__

// The glow around the player's fish while it has a shield (PlayerShieldFx, 0x148 bytes, constructor 0x419490,
// vtable 0x4a3350): a particle effect that circles the fish, passing in front of it and behind it.

#include "Font.h"
#include "Node.h"
#include "Particles.h"

namespace feedingfrenzy
{

class FishBase;

class PlayerShieldFx : public engine::Container
{
public:
	PlayerShieldFx();  // 0x419490

	// 0x419360: the glow's particles
	void init();
	// 0x418f90: the glow circles this fish, by how big it is
	void attach(FishBase* thePlayer);
	// 0x418fe0 and 0x419020: the glow starts, and ends
	void show();
	void hide();
	// 0x4190d0: the glow goes round the fish
	void update(engine::UpdateInfo& theInfo) override;
	// 0x419060: the glow is drawn the first time in a frame only
	void draw(engine::Graphics* g) override;

private:
	engine::Ref<engine::ParticleSystem> mGlow;  // +0x11c
	FishBase* mPlayer;                          // +0x120
	float mAngle;                               // +0x128: where it is on its circle, in radians
	bool mInFront;                              // +0x12c: it is drawn in front of the fish
	float mRadius;                              // +0x130: how far above and below the fish it swings
	float mSpeed;                               // +0x134: radians a second
	int mDrawCount;                             // +0x138: the times it has been drawn since the last update
};

} // namespace feedingfrenzy

#endif
