#ifndef __FEEDINGFRENZY_JOHNDORYPLAYER_H__
#define __FEEDINGFRENZY_JOHNDORYPLAYER_H__

// The player's fish that has the suction power (JohnDoryPlayer, 0x444 bytes, constructor 0x40eeb0, vtable 0x4a1fd8).
// Most of the stages' player fish are this kind. The right mouse button (or the control key) opens its mouth and
// pulls the fish in front of it towards its mouth; those it can eat, it eats at once, and the rest are held until the
// power ends. The power's charge runs out as it is used and comes back while the fish swims on.

#include "AnimImage.h"
#include "PlayerFish.h"

#include <string>
#include <vector>

namespace engine
{
class Sound;
}

namespace feedingfrenzy
{

// How far the suction has got (+0x410)
enum SuckPhase
{
	SUCK_OPENING = 0,  // the mouth opens
	SUCK_PULLING = 1,  // the fish in front are pulled in, for as long as the charge lasts
	SUCK_CLOSING = 2,  // the mouth closes
	SUCK_DONE = 3      // the fish was eaten or ended the power in another way
};

class JohnDoryPlayer : public PlayerFish
{
public:
	JohnDoryPlayer();            // 0x40eeb0
	~JohnDoryPlayer() override;  // 0x40ec10

	// FishBase
	void update(engine::UpdateInfo& theInfo) override;              // 0x7c: 0x40eac0
	void loadAnimationsOf(const std::string& theFolder) override;   // 0xe8: 0x40ed20
	void loadEffects() override;                                    // 0xec: 0x40e9e0
	void startBeingEaten(FishBase* theEater, float theSeconds) override;  // 0x144: 0x40e970
	std::string getTypeName() override;                             // 0x1b4: 0x40f820
	void updateAnimation() override;                                // 0x230: 0x40f3c0: the picture waits while the power is used

	// PlayerFish
	void rightButtonDown() override;              // 0x254: 0x40ea70
	void updateSpecial(engine::UpdateInfo& theInfo) override;  // 0x26c: 0x40e850
	void useSpecial() override;                   // 0x2bc: 0x40e6c0
	void releaseSpecial() override;               // 0x2c0: 0x40e7e0
	bool isSpecialReady() override { return true; }  // 0x2e8: 0x45e8e0
	float getSpecialProgress() override;          // 0x2ec: 0x40ea40: how much of the charge is left, 0 to 1

	// 0x31c: 0x40ef80: the fish in front of the mouth are eaten or pulled towards it, and those that got away let go
	virtual void pullFish();
	virtual void endPull();         // 0x320: 0x40e7f0: the power ends: the mouth closes
	virtual void releasePulled();   // 0x324: 0x40ebd0: the fish that were held are let go

private:
	// The pulled fish the stage has taken out since are forgotten
	void forgetRemovedFish();

	int mSavedState;                     // +0x408: what the fish was doing when the power began
	bool mSuckQueued;                    // +0x40c: the button went down when the power could not begin yet
	int mSuckPhase;                      // +0x410
	engine::AnimImage* mSuckAnimation;   // +0x414: the picture of the open mouth (owned by the animations)
	float mSuckSeconds;                  // +0x418: the charge: the seconds of suction left
	engine::Ref<engine::ParticleSystem> mSuckFxRight;  // +0x41c: the water drawn in when it faces right
	engine::Ref<engine::ParticleSystem> mSuckFxLeft;   // +0x420: the same when it faces left
	std::vector<FishBase*> mPulledFish;  // +0x424: the fish held by the pull
	bool mSuckingRight;                  // +0x434: the pull is to the right
	engine::Sound* mSuckSound;           // +0x438: the sound the power makes (null without a sound system)
};

} // namespace feedingfrenzy

#endif
