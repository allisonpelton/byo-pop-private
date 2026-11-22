// BugImg: the picture object every bug kind's body derives from (0x41f3d0..0x41f9c0 in the original; Ant, Bee and the
// other kinds, 0x41f9c0..0x425150, are not translated yet). A kind's body is the thing drawn and animated, and its
// brain (BugObj, a kind's BugBrain...) is the logic on the grid that owns it through BugObj::mBody. BugImg adds the two
// effects every kind can show: a ring of spinning stars over the bug when a rope has touched it (a daze), and a
// shock ring when it is shocked. Both are children that stay hidden until the brain's command asks for them.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class Anim;

// The commands a brain sends its body (BugObj::SetCommand and TakeCommand, through slot 13). BugImg answers the two that
// show an effect; the others are answered by each kind's own code
enum BugCommand
{
	BUG_CMD_FREEZE_ON = 100,      // 0x64: the kind's own (the frozen look, for as many frames as the second argument says)
	BUG_CMD_FREEZE_OFF = 0x65,    // the kind's own
	BUG_CMD_STARS_ON = 0x66,      // the spinning stars show (BugImg)
	BUG_CMD_STARS_OFF = 0x67,     // the kind's own
	BUG_CMD_RING_ON = 0x68,       // the shock ring shows (BugImg)
	BUG_CMD_RING_OFF = 0x69,      // the kind's own
};

class BugImg : public ObjBase
{
public:
	int mStarsTrack;              // +0x27c: the track (by AddTrack's number) of the stars child that shows it: fades in, turns, fades out
	int mRingTrack;               // +0x280: the same for the shock ring child
	ObjBase* mSpinStars;          // +0x284: the stars (a child of this body, hidden)
	ObjBase* mShockRing;          // +0x288: the shock ring (a child of this body, hidden)
	// +0x28c: a flag the DragonFly sets while it is frozen (BUG_CMD_FREEZE_ON) and clears (BUG_CMD_FREEZE_OFF); the brain
	// reads it as the body's own (BugObj::Update does not start the daze on a frozen body). The other kinds never set it:
	// the original leaves the byte as the memory had it, the port starts it false
	bool mFrozen;

	static Anim* sSpinStarsAnim;  // 0x60e254: "images/spinstars"
	static Anim* sShockRingAnim;  // 0x60e258: "images/monkey_shock_ring"

	BugImg();                     // 0x41f3d0
	~BugImg() override;           // 0x41f430 (the body 0x41f450)
	void Slot13(int theCommand, int theFrames, int theUnused) override; // 0x41f460 (the kinds' slot 13 calls it first)

private:
	void BuildStars();            // 0x41f4c0
	void BuildRing();             // 0x41f7a0
};

// 0x41f980 (GameWidget's preloads): the pictures of the two effects
void PreloadSpinStars();

} // namespace Sexy
