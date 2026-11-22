// BugObj: the base of every bug's brain (0x425150..0x426990 in the original; the kinds' brains, BugBrainAnt (0x413f90) to
// BugBrainSkeeter (0x41def0), are translated one by one: all eleven are). A bug is made, set up, then put on a place
// of the level's grid; from then on it is updated once a frame (every frame, or fewer while a bonus slows the bugs),
// keeps its place in the grid's lists as it moves, and dies when the rope or a pea touches it, when the cell it stands
// beside is cut, or when a bonus kills it. The brain owns a body (BugImg, 0x41f3d0, the thing drawn: mBody) whose kind
// the brain's own code sets up.
// A bug may lead a column of bugs (ants, caterpillars and centipedes come in columns): the members are a list through
// mNext and mPrev, each pointing at its head (mHead). Killing the head kills the column; killing a member holds the
// ones behind it.
// The statics are the list of every bug made (the level's order for placing them), the counts of bugs made and alive,
// the score streak, and the settings the bonuses make for every bug (a slow-down, an area that holds or kills).
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class BugImg;
class Emitter;
class PipeObj;

// The bug kinds the level places, in the order of its spawn table (LevelObj::mSpawn) and of the original's switch. The
// number is also what each kind's brain sets as mKind, except the moth's, which sets 7 (the fly's)
enum BugKind
{
	BUG_SKEETER = 0,                            // 0x41def0 (BugBrainSkeeter)
	BUG_BEETLE = 1,                             // 0x416220
	BUG_DRAGONFLY = 2,                          // 0x418880
	BUG_ROLLY = 3,                              // 0x41d0d0
	BUG_ANT = 4,                                // 0x413f90: comes in a column of six
	BUG_ROACH = 5,                              // 0x41c4f0
	BUG_CATERPILLAR = 6,                        // 0x416ea0: comes in a column of eighteen
	BUG_FLY = 7,                                // 0x4198c0
	BUG_CENTIPEDE = 8,                          // 0x417840: comes in a column of sixteen
	BUG_MOTH = 9,                               // 0x41af60
	BUG_BEE = 10,                               // 0x414fa0
};

const int BUG_LIST_MAX = 0x80;                  // the list holds 128 bugs; one more is made but not listed
const int BUG_GRID_CELL = 31;                   // a place of the grid is 31 design pixels (the same as FILL_CELL_SPACING)

// The puff of dust a bug leaves when it dies (0xc bytes in the original, made by the constructor 0x41ee10): an emitter that
// is a child of the bug and emits only when told
struct BugDust
{
	ObjBase* mOwner;                            // +0x0: the bug, the emitter's parent
	Emitter* mEmitter;                          // +0x4
	int mUnused;                                // +0x8: never set (the original does not either)

	explicit BugDust(ObjBase* theOwner);        // 0x41ee10
	void Burst(float theX, float theY);         // 0x41f3a0
};

class BugObj : public ObjBase
{
public:
	// The offsets in the comments are the original's, from the object (the port's ObjBase is smaller, so they differ here)
	BugImg* mBody;                              // +0x288: the body the kind makes, or null until then (the stand-ins never make one); the bug's place on the screen is the body's
	bool mDead;                                 // +0x28c: Zap has killed it (it is not counted alive any more)
	int mWaitCounter;                           // +0x290: updates skipped since the last one run (see sSlowFrames)
	bool mPlaced;                               // +0x294: the bug has a place on the grid
	int mKind;                                  // +0x298: its BugKind (set by each kind's constructor with SetKind)
	int mHoldFrames;                            // +0x29c: frames the bug is held where it is; the command is carried out on the last
	int mCommand;                               // +0x2a0: what the hold is for (1 freeze, 2 daze, 3 shock; set by SetCommand)
	int mUnused2A4;                             // +0x2a4: set by each kind's constructor to 1 or 2; nothing translated reads it
	int mGroupSize;                             // +0x2a8: the bugs in the column it leads, itself included; 0 for a bug that is alone
	int mSegmentSteps;                          // +0x2ac: the steps the earlier bugs of its column take when it joins (the wait frames Setup is given)
	bool mIsGroupLeader;                        // +0x2b0 (the name is a guess): the constructor sets it; the level clears it on the body of a caterpillar's or a centipede's column
	BugObj* mHead;                              // +0x2b8: the head of its column (the bug itself when it has none); 0 for a bug that became head when the one before it was taken out
	BugObj* mNext;                              // +0x2bc: the next of its column
	BugObj* mPrev;                              // +0x2c0: the one before it
	int mColumn;                                // +0x2c4: its place in the grid (0x7fff before it has one)
	int mRow;                                   // +0x2c8
	int mHeading;                               // +0x2cc: the direction it faces, 0 up, 1 right, 2 down, 3 left (a kind sets it; PipeObj's links go in the same order)
	bool mAlive;                                // +0x2d4: in play: set by the constructor, cleared by Zap (and by Vanish); a bug that is not takes no commands
	bool mUnused2D5;                            // +0x2d5: set to 0 by the constructor; nothing reads it
	BugDust* mDust;                             // +0x2d8
	BugObj* mNextInCell;                        // +0x2e0: the bugs on one place are a list through these (LevelObj::LinkBug)
	BugObj* mPrevInCell;                        // +0x2e4
	BugImg* mSetupBody;                         // +0x2e8: set by SetSetupBody, which each kind's Setup calls once it has made its body (the name is a guess)
	int mTimer;                                 // +0x2ec: a count of the kind's own, cleared when the bug is destroyed
	bool mPeaTarget;                            // +0x2f0: a pea or a fire ball has been sent at it
	bool mPeaHit;                               // +0x2f1: a pea or fire ball has hit it: its death makes a bunch of bananas
	int mFadeTrack;                             // +0x2f4: the track BuildFadeTrack makes (a shrink that ends with event 5)
	int mDeathTrack;                            // +0x2f8: the track BuildDeathTrack makes, which Zap starts instead of deleting the bug
	int mSpinTrack;                             // +0x2fc: the spin a bug is flung off the screen with (FlyOff)
	int mHideTrack;                             // +0x300: hides it, with event 1
	AnimKey* mSpinKey;                          // +0x304: the spin's first key, which FlyOff gives the bug's place and turn
	AnimKey* mSpinEndKey;                       // +0x308: its last key, 1900 pixels further on
	AnimKey* mHideKey;                          // +0x30c
	bool mAirborne;                            // +0x310: set to 0 by the constructor, to 1 by the wasp, the dragonfly and the skeeter, and by a fly or moth that takes off; BonusEarthquake spares a bug that has it (a guessed name)
	bool mFlying;                               // +0x311: FlyOff has started: the bug is on its way off the screen and takes no more hits
	int mHalfWidth;                             // +0x314: the places its box covers each way from its own (set by each kind's constructor)
	int mHalfHeight;                            // +0x318
	int mRole;                                  // +0x31c: what Setup was told (0 a head, 1 a body, 2 a column's last bug)
	int mListIndex;                             // +0x320: its place in the list of bugs, or -1
	float mUnused324;                           // +0x324: 32, set by the constructor; nothing translated reads it
	float mUnused328;                           // +0x328

	BugObj();                                   // 0x425150
	~BugObj() override;                         // 0x419950 (deleting; the body 0x425300)
	void Notify(ObjBase* theObj) override;      // 0x425880 (slot 4): a key of theObj ended; event 1 kills the bug (the kinds call it for events they do not know)
	bool Update() override;                     // 0x425410 (slot 5): the kinds' updates call this and their own code

	// The virtuals the kinds' brains override (slots 14 to 24). A kind's own version calls this one first where noted
	// 0x4253b0 (slot 14): sets the bug up. theHead is the bug that leads its column (none: the bug itself); thePrevious
	// is the bug made just before it in the column. theWaitFrames (240, 28 or 60) is not used here: each kind stores it
	// with SetSegmentSteps. The kinds' versions call this one, then make their body and call SetSetupBody
	virtual void Setup(int theRole, int theWaitFrames, BugObj* theHead, BugObj* thePrevious);
	virtual void StepSegment();                 // 0x4d6be0 (slot 15, +0x3c): the bug takes a step along its column's path (nothing in the base)
	// 0x5656b0 (slot 16, +0x40): a rope has been shot along a line near the bug (AlertAlongRope): theVertical, and the place
	// on the line next to the bug's middle. The kinds turn away or stop (nothing in the base; the name is a guess)
	virtual void AvoidRope(int theVertical, float theX, float theY);
	virtual void Vanish();                      // 0x425a70 (slot 17, +0x44): leaves play without dying (KillAll calls it on bugs on the screen); the base only clears mAlive
	// 0x425a80 (slot 18): the bug and everyone behind its head (the column) are held theFrames frames and then take the
	// command: 1 freezes, 2 dazes, 3 shocks. False when the bug is not in play
	virtual bool SetCommand(int theCommand, int theFrames);
	// 0x425ac0 (slot 19): the hold ends: the command is cleared, with the pea hit mark, and returned (0 when none was waiting)
	virtual int TakeCommand();
	// 0x425b00 (slot 20): the bug dies. theFollowers: the bugs behind it die too when it is the head, or are held for good
	// when it is not; theStartTrack: its death track (BuildDeathTrack's) starts, in place of the bug being deleted;
	// thePeaHit: it counts as hit by a pea. A bug that is not in play is left alone (false). The head of a column pays the
	// score of the streak
	virtual bool Zap(bool theFollowers, bool theStartTrack, bool thePeaHit);
	virtual void PlaceAt(int theColumn, int theRow); // 0x425cd0 (slot 21, +0x54): puts the bug on that place
	virtual void BuildFadeTrack();              // 0x4d6be0 (slot 22, +0x58): nothing in the base; the kinds make mFadeTrack (0x415b60 in three of them)
	virtual void BuildDeathTrack();             // 0x4d6be0 (slot 23, +0x5c): nothing in the base; the kinds make mDeathTrack
	virtual void FlyOff();                      // 0x425d20 (slot 24, +0x60): flung off the screen, spinning

	void SetKind(int theKind, int theHalfWidth, int theHalfHeight); // 0x4258c0 (EAX the bug, ECX the kind, EDX the half-width, the half-height pushed)
	void SetIsGroupLeader(bool theValue) { mIsGroupLeader = theValue; } // 0x4258b0
	void SetSetupBody(BugImg* theBody) { mSetupBody = theBody; }        // 0x4258e0
	void SetSegmentSteps(int theSteps) { mSegmentSteps = theSteps; }    // 0x425980 (ECX the steps)
	int GetSegmentSteps() { return mSegmentSteps; }                     // 0x425990
	int GetGroupSize() { return mGroupSize; }                           // 0x4259a0
	void MarkPeaHit() { mPeaHit = true; }                               // 0x425af0
	void UnlinkFromColumn();                    // 0x4258f0 (ECX the bug): takes it out of its column's list; the one behind it becomes the head
	// 0x4259f0 (ESI the bug, EDI the post it stands at, the flags pushed): the direction a bug takes from a post: the first of
	// the four (from the one after its heading, or from a random one of the next three when theRandom) that the post has a link
	// for and that is not straight back (when theAvoidBack); the way back when none is, or 999 when none is and theAvoidBack is off
	int ChooseHeading(PipeObj* thePost, bool theRandom, bool theAvoidBack);
	// 0x425da0 (ESI the bug, EDI theCopy; a fragment BonusTarzan calls): the bug is zapped at once, and theCopy takes its
	// picture and turn (a quarter turn on) and size, and the bug is hidden
	void LeaveCopy(ObjBase* theCopy);

	// 0x4259b0 (ECX the target heading, EDX the heading): -1 when the target is the heading plus one, 1 when it is the heading plus three, otherwise 0
	static int TurnDirection(int theTarget, int theHeading);

	// The bugs made, in the order the level places them (the list is sorted by box size and mixed with the C library's rand)
	static int sCount;                          // 0x60e2ac: the bugs in the list
	static int sAlive;                          // 0x60e2b0: the bugs alive
	static int sKillStreak;                     // 0x60e2b4: the bugs killed since the last shot: each is worth that many hundred
	static float sNextDepth;                    // 0x60e2b8: the depth the next bug takes (5 above this; it falls by 0.005 a bug)
	static int sSlowFrames;                     // 0x60e2bc: updates every bug skips between two it runs (0 for none); the bonuses set it
	static float sUnused2C0;                    // 0x60e2c0: SetSlowFrames clears it; nothing reads it
	static float sZapLeft;                      // 0x60e2c4: the area the bonuses hold or kill the bugs in
	static float sZapTop;                       // 0x60e2c8
	static float sZapRight;                     // 0x60e2cc
	static float sZapBottom;                    // 0x60e2d0
	static int sZapMode;                        // 0x60e2d4: 0 nothing, 1 the bugs in the area are held ten frames more each frame, 2 they die
	static bool sAlwaysPeaHit;                  // 0x60e17e: every death counts as hit by a pea

	static void SetZapArea(int theMode, float theLeft, float theTop, float theRight, float theBottom); // 0x426950 (EAX the mode; the box pushed)
	static void SetSlowFrames(int theFrames);   // 0x426980 (EAX)
	static void SetAlwaysPeaHit(bool theValue); // 0x426940 (AL)

	static int GetComboSound();                 // 0x426040: the sound of the next kill of the streak: bugkill 1 to 9, the ninth from the ninth kill on
	static void ResetKillStreak();              // 0x426060 (the stand-in called it OnShot): a shot has been fired, so the streak ends
	static void ResetCounters();                // 0x426070
	static void SortList();                     // 0x4260c0: puts the list of bugs in order (the C library's qsort by the area of the box, the biggest first) and numbers each bug by its place
	static void AddToList(BugObj* theBug);      // 0x426110 (ECX the bug)
	static void RemoveFromList(int theIndex);   // 0x426140 (EAX the index)
	static void Reset();                        // 0x426180: no bugs
	static BugObj* Get(int theIndex);           // 0x4261b0 (EAX the index): the bug of that place in the list, or none past the end
	static void Shuffle();                      // 0x4261e0: the bugs' order is mixed (the C library's rand, each swapped with a random one)
	// 0x426250 (ECX the column, EAX the row, theBug pushed): a bug that has been placed, other than theBug, whose box (its
	// half-width and half-height each way from its place) meets theBug's box set at that place
	static bool Overlaps(int theColumn, int theRow, BugObj* theBug);
	static BugObj* FindNear(float theX, float theY, float theReach);   // 0x426300 (the stand-in called it AnyNear): the first bug in play, not already a pea's target, within theReach of the place; or none
	static bool ZapNear(float theX, float theY, float theReach);       // 0x4263d0: every bug within theReach dies, counted as hit by a pea; whether any did
	static BugObj* FindNearest(float theX, float theY, float* theDistance); // 0x4264a0: the nearest bug in play; its distance (0 when none, or 1920 or more) is written to theDistance
	static BugObj* ZapFirstNear(float theX, float theY, float theReach); // 0x4265a0: the first bug in play within theReach dies, counted as hit by a pea; it is returned
	static BugObj* PickPeaTarget();             // 0x426680: a bug at random for a pea or a fireball, or none; it is marked
	static BugObj* PickAbove(float theY);       // 0x426750 (theY on the FPU stack): a bug at random, in play and leading, above theY on the screen; or none
	static void AlertAlongRope(PipeObj* theStart, PipeObj* theEnd, int theVertical); // 0x4267c0 (EAX start, ECX end): the bugs near the rope's line are told
	static void KillAll();                      // 0x4268a0: the bugs on the screen leave play (Vanish) and the others are deleted
	static void FreezeAll();                    // 0x4268e0: every bug is held for 1500 frames with the command 1 (freeze)
	static void ReleaseAll();                   // 0x426910: every bug takes the command it is held for now

private:
	void BuildSpinTrack();                      // 0x425e40 (EBX the bug)
	void BuildHideTrack();                      // 0x425fa0 (EBX the bug)
};

} // namespace Sexy
