// BugShared: the functions several kinds' vtables name in the original (one function at one address, which each kind's
// translation calls in a line, so none is translated twice), and the helpers the kinds' code shares.
#pragma once

#include "BugObj.h"
#include "BugImg.h"

namespace Sexy
{

struct FillGridCell;

// The headings (BugObj::mHeading), as PipeObj's links are numbered
enum BugHeading
{
	HEADING_UP = 0,
	HEADING_RIGHT = 1,
	HEADING_DOWN = 2,
	HEADING_LEFT = 3,
	HEADING_COUNT = 4,
};

// The kind of command the brain holds (BugObj::mCommand)
enum BugCommandKind
{
	COMMAND_FREEZE = 1,
	COMMAND_DAZE = 2,
	COMMAND_SHOCK = 3,
};

// The shared slots: a kind whose vtable names the function calls it from its override
bool BugSharedUpdate(BugObj* theBug);                              // 0x41c640 (slot 5): the base's update
void BugSharedStepSegment(BugObj* theBug);                         // 0x41c620 (slot 15): the bug is updated, then its body
void BugSharedVanish(BugObj* theBug);                              // 0x415810 (slot 17)
bool BugSharedSetCommand(BugObj* theBug, int theCommand, int theFrames); // 0x417aa0 (slot 18)
int BugSharedTakeCommand(BugObj* theBug);                          // 0x41e1f0 (slot 19)
// 0x416420 (slot 18; the beetle, the roach and the roly-poly): SetCommand that tells the body by the command it was given
// (0x417aa0 goes by the one the bug holds)
bool BugSharedSetCommandGiven(BugObj* theBug, int theCommand, int theFrames);
// 0x418cd0 (slot 19; the roach, the roly-poly and the dragonfly): TakeCommand that ends the freeze and the daze on the body
// but never the shock's ring
int BugSharedTakeCommandNoRing(BugObj* theBug);
// 0x415b60 (slot 22): theEndEvent is the key's event, 5 in the original. 0x418130 (the roach, the roly-poly and the
// centipede) is the same code with the event 6, 0x419170 (the dragonfly) with 7, and 0x41b990 (the fly and the moth, one
// function for both) with 8: those kinds call this with their own
void BugSharedBuildFadeTrack(BugObj* theBug, int theEndEvent);

// 0x4168e0 (EBX the bug; the beetle and the caterpillar, whose brains keep the three keys at the same places): the step's
// track, three keys that move the bug's place, its turn and nothing else (the keys' values are set by each step): the
// leg to the next post (event 2), the turn in place (event 3, one frame) and the turn's last frame (event 4, one frame).
// The keys are given back, and the track's number (AddTrack's) is returned
int BugSharedBuildMoveTrack(BugObj* theBug, AnimKey*& theLegKey, AnimKey*& theTurnKey, AnimKey*& theTurnEndKey);

// The shared slots of the bodies (the original's code reads the track at +0x2a4 of its own body, which the kinds that name
// it keep at that place; here the track is passed)
// 0x424f40 (slot 13; the bodies of the skeeter, the fly and the moth): the same fields at +0x2a0 and +0x2a8, given here: a freeze
// starts the dance (theDanceKey's length is set to the frames, theDanceTrack starts), its end cuts the dance short, and any
// other command is BugImg's
void BugSharedImgSlot13(BugImg* theBody, AnimKey* theDanceKey, int theDanceTrack, int theCommand, int theFrames, int theUnused);
bool BugSharedImgUpdate(BugImg* theBody);                          // 0x4219c0 (slot 5): ObjBase's update
void BugSharedImgNotify(BugImg* theBody, ObjBase* theObj, int theTrack); // 0x4237d0 (slot 4): event 2 starts theTrack

// The helpers
PipeObj* BugPostLink(PipeObj* thePost, int theHeading);            // the post's link in a heading (the original indexes them at +0x290)
// The grid as the flies and the moths read it (their checks run to the 40th row, not the 32 there are): the original
// indexes it as one table (column times 32, plus row), so a row up to 39 is a place of the next column. A place past the
// table's end is the original's reading of memory beside the grid (port: none, the caller takes the place as blocked)
FillGridCell* BugFlatGridCell(int theColumn, int theRow);
float BugHeadingAngle(int theHeading);                             // 0x41d3b0
float BugTurnAngle(int theHeading, int theNextHeading, float theBase); // 0x417b90

} // namespace Sexy
