// PipeObj: a pipe, the post or the piece of wall at one corner of the level's grid (0x455f80..0x459380 in the original;
// 0x34c bytes there). The walls the rope runs between are chains of pipes, a post at each end; every place of the grid
// has one (gFillGrid), and the ones that are not part of a wall show the empty picture. A pipe is a cel of
// "images/pipes01" (the posts 0 to 2, the walls across 3 to 6, the walls down 7 to 10, the empty one 11) and has
// eleven tracks of its own: the one that grows it in when a cut makes it a wall, and ten that fly it about (in and out
// of the level's pictures, whose pixels it takes the colours of, away when the level ends, and the nudge of a cut
// that moves it; one of them is built twice and four are never started by this class). The level starts the tracks; the keys' events tell the pipe when one has landed (Notify).
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class Anim;
struct MorphCell;

const int PIPE_VARIANT_EMPTY = 11;     // the cel of a place that is not part of a wall
const int PIPE_KIND_UNSET = 0xb;       // what the constructor and SetupMorph make a pipe's kind: no wall (the level sets it on a wall)

// The events the pipe's keys end with (the key's mEventId), which Notify tells apart
enum PipeEvent
{
	PIPE_EVENT_ARRIVED = 1,            // the pipe has flown to its place on the picture: it takes a look again and flies back
	PIPE_EVENT_FLAG_A = 2,             // sets the first settled flag (IsSettledA)
	PIPE_EVENT_FLAG_B = 3,             // sets the second (IsSettledB)
	PIPE_EVENT_LEAVES = 4,             // the sound of leaves, once while the pipes fly (sSoundPlayed)
};

class PipeObj : public ObjBase
{
public:
	// The pipe's fields (offsets are the original's, from the object)
	int mKind;                                  // +0x27c: 0 none, 1 post, 2 wall across, 3 wall down (LevelObj's FillKind); 0xb until the look is chosen
	int mColumn;                                // +0x280: its place in the grid
	int mRow;                                   // +0x284
	int mVariant;                               // +0x288: which cel of its kind it shows (11 is the empty one)
	bool mSettledA;                             // +0x28c: set by event 2, cleared by event 1 and Init
	bool mSettledB;                             // +0x28d: set by event 3, cleared by Init
	PipeObj* mUpLink;                           // +0x290: the pipe at the upper end of the wall it is part of, or none
	PipeObj* mRightLink;                        // +0x294: ... at the right end
	PipeObj* mDownLink;                         // +0x298: ... at the lower end
	PipeObj* mLeftLink;                         // +0x29c: ... at the left end
	int mUnused2A0;                             // +0x2a0 to +0x2ac: set to 0 by the constructor, SetupMorph and Refresh; nothing in the class reads them
	int mUnused2A4;                             // +0x2a4
	int mUnused2A8;                             // +0x2a8
	int mUnused2AC;                             // +0x2ac
	int mAppearTrackIndex;                      // +0x2b0: the track (by AddTrack's number) a cut starts the pipe on
	AnimKey* mAppearKey;                        // +0x2b4: that track's first key (grows in, with the pipe sound)
	AnimKey* mSettleKey;                        // +0x2b8: its second (holds the end)
	AnimTrack* mAppearTrack;                    // +0x2bc: the track itself
	AnimKey* mDriftKey;                         // +0x2c4: the first key of the drift track: waits at the bottom of the screen
	AnimKey* mRestKey;                          // +0x2c8: a key at the pipe's place that tracks end on (made by Create)
	AnimKey* mSpinEmptyKey;                     // +0x2cc: the long key of the spin track of an empty place
	AnimKey* mSpinKey;                          // +0x2d0: the same for a wall or a post
	AnimKey* mReturnKey;                        // +0x2d4: where the return track starts from (a wall or post)
	AnimKey* mReturnEmptyKey;                   // +0x2d8: the same for an empty place
	AnimKey* mNudgeKey;                         // +0x2dc: the key of the nudge that holds the place to move to
	int mDriftTrack;                            // +0x2e0: tracks, by AddTrack's number: the drift in, started by SetupMorph
	int mReturnTrack;                           // +0x2e4: from the picture back to its place (a wall or post)
	int mReturnEmptyTrack;                      // +0x2e8: the same for an empty place
	int mSecondSpinTrack;                       // +0x2ec: a second spin (the original builds it twice and nothing starts it)
	int mSpinTrack;                             // +0x2f0: started by Refresh for a wall or post
	int mSecondSpinEmptyTrack;                  // +0x2f4: the second spin of an empty place (nothing starts it)
	int mSpinEmptyTrack;                        // +0x2f8: started by Refresh for an empty place
	int mFlyOffTrack;                           // +0x2fc: the pipe flies off, started by Retire
	int mNudgeTrack;                            // +0x300: started by StartShift
	float mShiftX;                              // +0x304: where the next nudge goes (SetShiftTarget)
	float mShiftY;                              // +0x308
	float mShiftSet;                            // +0x30c: 1 once a nudge is set; Init and SetupMorph clear it (nothing reads it)
	float mFirstOffsetX;                        // +0x310: from the pipe to the first picked cell of the level's picture, in design pixels
	float mFirstOffsetY;                        // +0x314
	float mSecondOffsetX;                       // +0x318: ... to the cell picked of the second picture
	float mSecondOffsetY;                       // +0x31c
	MorphCell* mFirstCell;                      // +0x320: the picked cells, whose colours the pipe takes
	MorphCell* mSecondCell;                     // +0x324
	AnimKey* mGatherKey;                        // +0x328: the drift track's key that flies the pipe to the first cell
	AnimKey* mGatherEndKey;                     // +0x32c: its last, which ends with event 1
	AnimKey* mSecondSpinMidKey;                 // +0x330: the second spin's keys
	AnimKey* mSecondSpinEndKey;                 // +0x334
	AnimKey* mSecondSpinEmptyMidKey;            // +0x338
	AnimKey* mSecondSpinEmptyEndKey;            // +0x33c
	AnimKey* mFlyOffKey;                        // +0x340: the fly-off's first key, a flight that slows and ends below the screen
	AnimKey* mFlyOffTurnKey;                    // +0x344
	AnimKey* mFlyOffFadeKey;                    // +0x348

	static bool sSoundPlayed;                   // 0x670191: the sound of leaves has played
	static Anim* sPipesAnim;                    // 0x670194: "images/pipes01"

	PipeObj();                                  // 0x455f80
	void Init() override;                       // 0x4560a0: the flags and the nudge are cleared
	void Notify(ObjBase* theObj) override;      // 0x4562f0: the events of the pipe's own keys

	void SetKind(int theKind) { mKind = theKind; }          // 0x456070 (EAX the pipe, ECX the kind)
	bool IsSettledA() { return mSettledA; }     // 0x456080
	bool IsSettledB() { return mSettledB; }     // 0x456090
	void SetShiftTarget(float theX, float theY); // 0x4560c0 (EAX the pipe): where the pipe moves to when a cut reaches it
	void StartShift();                          // 0x4560e0 (EAX the pipe): the nudge starts, once the pipe has its full size
	void Refresh();                             // 0x4561c0 (EAX the pipe): a new look, and the spin track that goes with it
	void Retire();                              // 0x4562d0 (EAX the pipe): the pipe flies off
	void Rebuild();                             // 0x4564c0 (EAX the pipe): flies back from the second picture's cell to its place
	void RandomizeDuration();                   // 0x4565f0 (EAX the pipe): the drift's wait is 300 to 332 frames
	void SetupMorph();                          // 0x458b00 (EAX the pipe): the picked cells and the keys that go to them
	static PipeObj* Create(int theColumn, int theRow); // 0x459120 (ECX the column, the row pushed)

private:
	void BuildAppearTrack();                    // 0x458dc0 (EBX the pipe)
	void BuildTracks();                         // 0x458aa0
	void BuildDriftTrack();                     // 0x456620
	void BuildReturnTrack();                    // 0x456cc0
	void BuildReturnEmptyTrack();               // 0x457020
	void BuildSpinTrack();                      // 0x4573f0
	void BuildSecondSpinTrack();                // 0x457740 (called twice, the second time over the first's fields)
	void BuildSpinEmptyTrack();                 // 0x457bb0
	void BuildSecondSpinEmptyTrack();           // 0x457fa0
	void BuildFlyOffTrack();                    // 0x458570
	void BuildNudgeTrack();                     // 0x458b80
	void PickMorphCells();                      // 0x456120 (ESI the pipe)
	void PlaceGatherKeys();                     // 0x456bf0 (ESI)
	void PlaceReturnKey();                      // 0x456fc0 (EAX)
	void PlaceReturnEmptyKey();                 // 0x457390 (EAX)
	void PlaceSpinKey();                        // 0x4576e0 (EAX)
	void PlaceSecondSpinKeys();                 // 0x457b00 (ESI)
	void PlaceSpinEmptyKey();                   // 0x457f40 (EAX)
	void PlaceSecondSpinEmptyKeys();            // 0x458480 (ESI)
	void PlaceFlyOffKeys();                     // 0x4589a0 (EAX)
	void StartDriftTrack();                     // 0x4561b0 (EAX)
	// 0x43c220 (EBX the pipe; it is in LevelObj's range): the four links, from the grid's kinds, to the nearest post in each
	// direction along the places that have ground
	void LinkByKinds();
};

// 0x459360: the picture of the pipes is loaded
void PreloadPipes();

} // namespace Sexy
