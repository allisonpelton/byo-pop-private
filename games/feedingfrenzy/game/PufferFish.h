#ifndef __FEEDINGFRENZY_PUFFERFISH_H__
#define __FEEDINGFRENZY_PUFFERFISH_H__

// The puffer fish (PufferFish, 0x3bc bytes, constructor 0x40ce50, vtable 0x4a1048, destructor 0x40c660): a fish that
// swims about and puffs up when the player's fish, which is bigger than it, comes near (or now and then by chance). Puffed
// up, it stings: it pushes away any fish that touches it and stuns it, and cannot be eaten. The original's object has a
// float at +0x398 (the level the puffer had when it began to puff) that is set and never read.

#include "AIFish.h"
#include "AnimImage.h"
#include "CollisionRect.h"

namespace feedingfrenzy
{

// How far the puffer is in its puffing (+0x390)
enum PuffPhase
{
	PUFF_NONE = 0,     // it has deflated: it goes back to swimming
	PUFF_WAIT = 1,     // it has stopped and waits a moment
	PUFF_INFLATE = 2,  // the pictures play forward
	PUFF_HOLD = 3,     // it stays puffed up
	PUFF_DEFLATE = 4   // the pictures play backward
};

class PufferFish : public AIFish
{
public:
	PufferFish();  // 0x40ce50

	// slot 0xe8: 0x40d470: the pictures of the fish's folder, and the puffing's ("puff\puff_cycle."), each centred
	void loadAnimationsOf(const std::string& theFolder) override;
	// slot 0x128: 0x40cf60: the hit rectangle of the fish's file is kept, as the puffing grows the one the fish has
	void setHitRect(const CollisionRect& theRect) override;
	// slot 0x138: 0x40c6e0: a puffed fish eats nothing
	bool canEat(FishBase* theVictim) override;
	// slot 0x140: 0x40cda0: a puffed fish cannot be eaten
	bool canBeEatenBy(FishBase* theEater) override;
	// slot 0x1e4: 0x40d250: a fish that touches the puffed fish is pushed away and stunned, and the player's costs points
	void onContact(FishBase* theFish) override;
	// slot 0x230: 0x40cd80: the pictures stay as the puffing has them
	void updateAnimation() override;
	// slot 0x260: 0x40caa0: the puffing, a phase at a time
	void updateSpecial(engine::UpdateInfo& theInfo) override;
	// slot 0x280: 0x40c960: the player's fish, when it is bigger and near, makes the fish puff
	void checkReactions() override;
	// slot 0x284: 0x40c700: a puffed fish goes on puffing; any other puffs by chance or swims on
	void decide() override;
	// slot 0x298: 0x40c760: the fish puffs, unless it has not been in its state for long enough or is stunned, frozen,
	// leaving, being eaten or already puffed
	void beginSpecial() override;
	// slot 0x2cc: 0x40cdd0: a fish that is puffed deflates when it is frozen
	void freeze() override;

private:
	// slot 0x2d8: 0x40cc70: the hit rectangle grows by the share of the puffing's pictures that have played
	void updatePuffHitRect();
	// 0x40c800: the puffing's pictures play forward
	void startPuff();
	// 0x40c930: the puffing's pictures play backward
	void startDeflate();

	int mPuffIndex;                   // +0x384: the puffing's place in the selector of the pictures
	float mPhaseSeconds;              // +0x388: the seconds left of the phase
	float mSecondsSincePenalty;       // +0x38c: the seconds since the puffer cost the player points
	int mPuffPhase;                   // +0x390: a PuffPhase
	engine::AnimImage* mPuffAnim;     // +0x394 (a child of the pictures): the pictures that play
	float mPuffGrowth;                // +0x39c: how much taller the hit rectangle is, as a share of its height, when puffed
	CollisionRect mBaseHitRect;       // +0x3a0: the hit rectangle of the fish's file
};

} // namespace feedingfrenzy

#endif
