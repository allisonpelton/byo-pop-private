#ifndef __FEEDINGFRENZY_OYSTER_H__
#define __FEEDINGFRENZY_OYSTER_H__

// The oyster (Oyster, 0x3b0 bytes, constructor 0x40af30, vtable 0x4a0778, destructor 0x40a3e0): a fish that stays where
// it is on the sea floor. From time to time its shell opens, with a pearl in it that the player may eat while the shell
// is open; then the shell closes on any fish that is at its mouth and can be eaten by it. A marked oyster has a pearl
// only some of the times it opens, and an oyster that is not marked opens until its pearl is eaten. The original's
// object has a float at +0x398 that is set (2.0) when the shell opens and never read.

#include "AIFish.h"
#include "FishAttachFx.h"
#include "Geometry.h"
#include "Particles.h"
#include "Selector.h"

namespace feedingfrenzy
{

// What the pearl in an oyster is (0x40a470)
enum PearlKind
{
	PEARL_WHITE = 0,  // it scores
	PEARL_BLACK = 1,  // it scores five times as much, with a message
	PEARL_LIFE = 2    // it gives an extra fish
};

// Where the shell is (+0x384)
enum ShellPhase
{
	SHELL_CLOSED = 0,   // it has closed: the fish in its mouth are done, and it goes back to waiting
	SHELL_OPENING = 1,  // the pictures play forward and the pearl grows
	SHELL_OPEN = 2,     // it waits open, with the pearl, for the seconds it was given
	SHELL_CLOSING = 3   // the pictures play backward and the pearl shrinks
};

class Oyster : public AIFish
{
public:
	Oyster();  // 0x40af30

	// slot 0xe4: 0x40a480: the shell's pictures (jpeg files from the first, each centred), then the three pearls that
	// one shows at a time: the white one, the black one and the bubble of the extra fish
	void loadAnimations(const char* theOpenPath, const char* theTurnPath, const char* theEatPath,
	                    const char* theExtraPath) override;
	// slot 0xe8: 0x40b900: the shell's pictures are "open."; the sound the pearl makes when it is eaten is made
	void loadAnimationsOf(const std::string& theFolder) override;
	// slot 0xec: 0x40b790: the chomp's particles and the pearl's glimmer (the stun effect and the thought bubble of other
	// fish are not made)
	void loadEffects() override;
	// slot 0xfc: 0x40af00: the points of the fish's file are the white pearl's
	void setPoints(float thePoints) override;
	// slot 0x138: 0x419740
	bool canEat(FishBase*) override { return false; }
	// slot 0x13c: 0x40aa00: the pearl may be eaten while the shell is open and the pearl shows
	bool canBeEatenByLevel(float theEaterLevel) override;
	// slot 0x140: 0x40aa40: any fish may eat it
	bool canBeEatenBy(FishBase* theEater) override;
	// slot 0x144: 0x40b470: the eater starts to eat the pearl; the oyster does not wait with a decision
	void startBeingEaten(FishBase* theEater, float theSeconds) override;
	// slot 0x148: 0x40abd0: the pearl is eaten and goes
	bool finishBeingEaten(FishBase* theEater) override;
	// slot 0x160: 0x40ba30: the shell takes a fish in
	void eat(FishBase* theVictim) override;
	// slot 0x17c: 0x40b150: it is put in the stage waiting, with the pearl that its first opening will show
	void putInPlay() override;
	// slot 0x224: 0x40adc0: the hit rectangle, moved to where the stage's layer shows
	void updatePlacedHitRect() override;
	// slot 0x228: 0x40ac30: the nose and the tail where the stage's layer shows the oyster, and the body and mouth
	// rectangles round them
	void updatePlacedRects() override;
	// slot 0x230: 0x419550
	void updateAnimation() override;
	// slot 0x244: 0x40aa50: the pearl goes to the eater's nose and shrinks; the oyster goes on with its shell
	void updateBeingEaten(engine::UpdateInfo& theInfo) override;
	// slot 0x254: 0x40a760: it waits until it is time to open
	void updateWander(engine::UpdateInfo& theInfo) override;
	// slot 0x260: 0x40a7b0: the shell opens, waits, and closes
	void updateSpecial(engine::UpdateInfo& theInfo) override;
	// slot 0x284: 0x40a9c0: a waiting oyster that has a pearl or is marked opens, and an open one goes back to waiting
	void decide() override;
	// slot 0x288: 0x40a730: it waits for the seconds of a wander
	void beginWander() override;
	// slot 0x298: 0x40b040: the shell starts to open, and the pearl, if there is one, shows
	void beginSpecial() override;
	// slot 0x2a4: 0x40af20: it opens no more
	void beginLeave() override;

	// slot 0x2dc: 0x40a470
	int getPearlKind() { return mPearlKind; }

private:
	// 0x40ae60: the pearl that the next opening shows, and the points it is worth
	void pickPearl();
	// slot 0x2d8: 0x40b530: the shell closes on the fish at its mouth that it can eat, or opens again for a while when
	// one that it cannot eat is there
	void bite();

	int mShellPhase;                           // +0x384
	engine::Selector* mPearl;                  // +0x388 (the fish's child): the three pearls, one shows at a time
	engine::ParticleSystem* mChompFx;          // +0x38c (the fish's child)
	bool mPearlPresent;                        // +0x390: the oyster has a pearl to show
	int mPearlKind;                            // +0x394: a PearlKind
	float mPearlPoints;                        // +0x39c: what the white pearl is worth
	bool mLeaving;                             // +0x3a0
	engine::Ref<FishAttachFx> mGlimmer;        // +0x3a4
	engine::Point mPearlOffset;                // +0x3a8: where the pearl rests from the oyster
};

} // namespace feedingfrenzy

#endif
