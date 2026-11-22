// BugImgRoach: the roach's body (0x423d50..0x424300 in the original, "Roach" there; 0x2d0 bytes). It is the picture of the
// roach, a child of its brain (BugBrainRoach), and walks and spins on its own tracks while the brain moves the brain's own
// place. Unlike the beetle's, the picture hangs on the wall's line itself (no side to flip to), has the one set of cels
// (the walk's, on the body itself, not on its keys) and wobbles from side to side as it walks: a step is 10 frames in
// which the cels run through and the roach turns a little (6 degrees) one way, then back the other, the turn's side set
// by the side it faces when the step ends. The brain sends it commands (BugObj::SetCommand's, through slot 13): 100 asks for
// a spin of as many frames (three degrees a frame, which is what a frozen roach does) and 0x65 ends it.
#pragma once

#include "BugImg.h"

namespace Sexy
{

class Anim;

// The events the body's keys end with (the key's mEventId), which Notify tells apart
enum RoachBodyEvent
{
	ROACH_EVENT_STEP_DONE = 1,   // the walk track's last key: one step is walked
	ROACH_EVENT_SPIN_DONE = 5,   // the spin's last key: it walks again
};

class BugImgRoach : public BugImg
{
public:
	// The offsets in the comments are the original's, from the object
	bool mUnused290;               // +0x290: set to 0 by the constructor; nothing reads it
	bool mUnused2A4;               // +0x2a4: set to 0 by the constructor; nothing reads it
	int mWalkTrack;                // +0x2ac: one step (AddTrack's number)
	int mSpinTrack;                // +0x2b8 (the name is a guess): the spin of a freeze: a turn of the asked length, then 75 frames settling to none (event 5)
	AnimKey* mWalkKey;             // +0x2c0: the walk track's first key (10 frames, the cels counting up, a turn of 6 degrees one way at its start)
	AnimKey* mWalkEndKey;          // +0x2c4: its second key, which raises event 1 (the turn of 6 degrees the other way)
	AnimKey* mSpinKey;             // +0x2c8 (the name is a guess): the spin track's first key, whose length the freeze sets
	AnimKey* mSpinTurnKey;         // +0x2cc (the name is a guess): its second key (75 frames), whose turn the freeze sets: three degrees for every frame frozen

	BugImgRoach();                 // 0x423d50
	~BugImgRoach() override;       // 0x423dd0 (the body 0x423df0)
	void Notify(ObjBase* theObj) override;   // 0x423e00 (slot 4)
	bool Update() override;                  // 0x4219c0 (slot 5; shared): ObjBase's update
	void Slot13(int theCommand, int theFrames, int theUnused) override; // 0x423ea0 (calls BugImg's first)

	static Anim* sWalkAnim;        // 0x60e29c: "images/cockroach_walk"

private:
	void BuildWalkTrack();         // 0x423f80 (EBX the body)
	void BuildSpinTrack();         // 0x424180
};

// 0x4242e0 (GameWidget's preloads): the picture of the walk (96 by 64, 28 cels)
void PreloadCockroach();

} // namespace Sexy
