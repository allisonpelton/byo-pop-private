// RopeObj: the rope the monkey shoots (0x461320..0x464320 in the original). Two arrows stand at the cell the rope was
// shot from and fly out along the row or column to the pipes at the ends of the stretch being cut; a chain of 120 small
// parts is laid along the path between them, and bolts of light run down the pipes where an arrow arrives. While the
// player aims, the same arrows and parts show faintly where the rope would go. A bug touching the rope breaks it (it
// snaps, the parts gather at the shot point and the monkey loses a life); a rope that reaches both ends tells the level
// to cut the ground. The bonus ropes (the player's rope mode 1 and 2) burn or smoke instead.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class Anim;
class BoltObj;
class PipeObj;
struct EmitterSet;

const int ROPE_ARROWS = 2;             // one arrow for each end of the stretch
const int ROPE_PARTS = 120;            // the chain's parts (a stretch is at most about 80 of them long)
const int ROPE_SPARKS = 6;             // the little objects that spin round each arrow
const int ROPE_PART_SPACING = 16;      // design pixels between two parts
const int ROPE_ARROW_ANIMS = 4;        // one picture of arrows for each rope mode, 0 to 3

// What the rope is doing (RopeObj::mState)
enum RopeState
{
	ROPE_IDLE = 0,                     // not shown
	ROPE_FLYING = 1,                   // shot: the arrows fly out to the ends
	ROPE_CUTTING = 2,                  // both arrows have arrived and the ground is being cut
	ROPE_SNAPPED = 3,                  // a bug broke it
	ROPE_AIMING = 4,                   // the preview of where a shot would go
};

// What RopeObj::HitBox says
enum RopeHit
{
	ROPE_HIT_NONE = 0,
	ROPE_HIT_BURN = 2,                 // a flame rope's arrow touches the box
	ROPE_HIT_TOUCH = 3,                // an arrow of the other ropes or the rope itself touches it
};

// The tracks of a part, as AddTrack numbers them (each kept in the part: the first is only ever started by number 0)
enum RopePartTrack
{
	ROPE_PART_GHOST = 0,               // pulses faintly: the aiming preview
	ROPE_PART_SOLID = 1,               // pulses: the rope shot (the part's mButtonId)
	ROPE_PART_FLY_OFF = 2,             // drifts off and away, spinning (mHoverState)
	ROPE_PART_GATHER = 3,              // runs to the shot point (mUnusedF0)
};

// The events the arrows' keys end with (the key's mEventId), which Notify tells apart
enum RopeEvent
{
	ROPE_EVENT_ARROW_FLIGHT = 1,       // the arrow's flight key (nothing reads it)
	ROPE_EVENT_ARROW_ARRIVED = 2,      // the arrow has reached its end of the stretch
	ROPE_EVENT_ARROW_GATHER = 3,       // the arrow's last key (nothing reads it)
	ROPE_EVENT_PART_FLY_OFF = 4,       // a part's flying-off track began its last key (nothing reads it)
	ROPE_EVENT_ARROW_SPARK = 5,        // the arrow's aiming loop came round: a burst of sparks
};

class RopeObj : public ObjBase
{
public:
	// The rope's fields (offsets are the original's, from the object)
	BoltObj* mBolts[ROPE_ARROWS];      // +0x27c: the bolts that run down the pipe at each end when an arrow arrives
	ObjBase* mArrows[ROPE_ARROWS];     // +0x284: the two arrows (mButtonId is the arrow's number, mHoverState 1 once it has arrived)
	ObjBase* mSparks[ROPE_SPARKS];     // +0x28c: the little objects that spin round the arrows
	int mUnused2a4[2];                 // +0x2a4: nothing is kept here
	AnimKey* mArrowFlightKey[ROPE_ARROWS];   // +0x2ac: the arrow's flight to its end (its duration follows the distance)
	AnimKey* mArrowArrivedKey[ROPE_ARROWS];  // +0x2b4: the arrow waits at its end (event 2)
	AnimKey* mArrowGatherEndKey[ROPE_ARROWS]; // +0x2bc: the arrow's last key, fading out (event 3)
	AnimKey* mArrowIdleKey[ROPE_ARROWS];     // +0x2c4: the arrow's idle key (a track that loops on it)
	AnimKey* mArrowGatherFromKey[ROPE_ARROWS]; // +0x2cc: the arrow runs to the shot point, starting where it is
	AnimKey* mArrowGatherToKey[ROPE_ARROWS]; // +0x2d4: ... and ending at the shot point
	AnimKey* mArrowAimKey[ROPE_ARROWS];      // +0x2dc: the aiming loop's first key (the far side of the cell)
	AnimKey* mArrowAimEventKey[ROPE_ARROWS]; // +0x2e4: ... and its second (event 5), at the cell
	ObjBase* mParts[ROPE_PARTS];       // +0x2ec: the chain
	AnimKey* mPartGatherFromKey[ROPE_PARTS]; // +0x4cc: the part's run to the shot point (first key: where it is)
	AnimKey* mPartGatherToKey[ROPE_PARTS];   // +0x6ac: ... (second key: the shot point)
	PipeObj* mStartWall;               // +0x88c: the pipe at the first end of the stretch
	PipeObj* mEndWall;                 // +0x890: the pipe at the second end
	int mBoltTimer[ROPE_ARROWS];       // +0x894: frames each bolt has left to run
	int mMode;                         // +0x89c: the player's rope mode as the rope was shot (0 usual, 1 flame, 2 smoke)
	int mShieldFrames;                 // +0x8a0: frames the rope cannot cost a life for (guess: nothing in this class sets it)
	bool mUnused8a4;                   // +0x8a4: nothing uses it
	bool mEndAlive[ROPE_ARROWS];       // +0x8a5: the arrow is still flying (false once it has arrived)
	int mCellX;                        // +0x8a8: the cell the rope was shot from
	int mCellY;                        // +0x8ac
	int mUnused8b0[2];                 // +0x8b0: nothing is kept here
	float mAimX;                       // +0x8b8: where the aim was when it was last set
	float mAimY;                       // +0x8bc
	int mFrom;                         // +0x8c0: the stretch's first end, a column (along a row) or a row
	int mTo;                           // +0x8c4: its second end
	int mState;                        // +0x8c8: RopeState
	int mVertical;                     // +0x8cc: 1 along a column, 0 along a row
	bool mAimReady;                    // +0x8d0: the aiming preview is laid out and shown
	int mSnapTimer;                    // +0x8d4: frames the snap's sparks last
	int mArrowIdleTrack[ROPE_ARROWS];  // +0x8d8: the arrow's tracks, by AddTrack's number
	int mArrowAimTrack[ROPE_ARROWS];   // +0x8e0
	int mArrowFlightTrack[ROPE_ARROWS]; // +0x8e8
	int mArrowGatherTrack[ROPE_ARROWS]; // +0x8f0
	EmitterSet* mArrowEmitters[ROPE_ARROWS]; // +0x8f8: the sparks and swells at each arrow
	PipeObj* mSparkWalls[ROPE_ARROWS]; // +0x900: the pipes the sparks burst on (the bonus ropes only), or none
	ObjBase* mSnapFlash;               // +0x908: the flash where the rope snapped

	// The pictures (globals in the original; loaded by PreloadRope)
	static Anim* sArrowAnims[ROPE_ARROW_ANIMS]; // 0x6701f4: the arrows, one picture for each rope mode
	static Anim* sPartAnim;            // 0x670204: the chain's parts
	static Anim* sArrowAuraAnim;       // 0x670208: the glow the sparks round an arrow draw
	static Anim* sSnapAnim;            // 0x67020c: the flash of a snap

	RopeObj();
	~RopeObj();
	void Init() override;                              // 0x461510: no rope shown
	void Notify(ObjBase* theObj) override;             // 0x461800: an arrow arrives, or its aiming loop comes round
	bool Update() override;                            // 0x4615c0

	void Start(int theVertical, int theCellX, int theCellY, int theFrom, int theTo); // 0x462270 (EDI vertical, ESI cell Y, EAX the object)
	void SetAim(int theVertical, int theCellX, int theCellY, int theFrom, int theTo); // 0x462360
	void ResetIfFinished();                            // 0x462600
	bool IsAimReady() { return mAimReady; }            // 0x462620
	int GetState() { return mState; }                  // 0x462630
	bool Snap(float theX, float theY);                 // 0x462640: a bug has broken the rope at this place
	int HitBox(float theX, float theY, float theWidth, float theHeight); // 0x462950: RopeHit

	// The level's rope, or nullptr when there is none (the level's object at +0x9000)
	static RopeObj* Current();

private:
	void LayBolt(int theEnd);                          // 0x461a90
	void LayParts(int theTrack);                       // 0x461c90
	void SetArrowAimKeys(int theArrow, int theVertical, float theAimX, float theAimY, float theEnd); // 0x462130
	void SetArrowTarget(int theArrow, int theVertical, int theCellX, int theCellY, int theOtherEnd); // 0x461fd0
	void BuildSparks();                                // 0x462c90
	void BuildArrows();                                // 0x462f00
	void BuildParts();                                 // 0x463730
	void BuildSnapFlash();                             // 0x463f40
};

} // namespace Sexy
