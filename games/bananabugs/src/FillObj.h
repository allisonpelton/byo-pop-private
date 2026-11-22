// FillObj: one cell of the ground the monkey cuts away (0x428230..0x42ad60 in the original). A level's ground is a
// grid of 40 by 32 cells, each a small picture of the level's background (a cel of "images/fills/fill_NN") hidden
// under a cell object. A cut turns the cells on one side of the rope into flies that the picture fills in over a few
// frames, spreading out from where the rope was shot; the cell that the wave reaches last tells the level the cut is
// done. Cells under the rope's preview glow with a flash of the net fly's picture. The statics keep the counts the
// level reads (how many cells have settled, how many wait on the cut) and the picture sheet shared by every cell.
// Each cell has a slot in the grid shared with the pipes (the walls) and other objects of the level.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class Anim;
class FillObj;
class BugObj;
class BonusCoin;
class PipeObj;

const int FILL_GRID_COLUMNS = 40;
const int FILL_GRID_ROWS = 32;
const int FILL_CELL_SPACING = 31;      // a cell is 31 design pixels square
const int FILL_SHIFT_MAX = 3;          // the pipes at a cell's corners a cut may queue
const int FILL_WAVE_FIRST = 100;       // the cut's wave numbers run from 100 to 110, then back to 100
const int FILL_WAVE_LAST = 110;
const int OBJ_TYPE_FILL_CELL = 12;     // the type the cells mark themselves with

// One place of the level's grid (0x20 bytes; the original's eight slots, in this order). The level lays it out
// (LevelObj); the cells, pipes and bugs find each other through it
struct FillGridCell
{
	int mColumn;                       // slot 0: its column
	int mRow;                          // slot 1: its row
	PipeObj* mPipe;                    // slot 2: the pipe (wall) at this corner, if any
	FillObj* mCell;                    // slot 3: the FillObj of this place
	int mKind;                         // slot 4: what the level's layout makes of the place (LevelObj's FillKind, +10 once checked)
	int mUnused5;                      // slot 5: set to 0 by the level and never read
	BugObj* mFirstBug;                // slot 6: the first of the bugs standing on this place, each pointing on to the next
	BugObj* mLastBug;                 // slot 7: the last of them
};

// The level's grid, columns first: the original's table at 0x602eb0 (32 bytes a cell, 32 cells a column)
extern FillGridCell gFillGrid[FILL_GRID_COLUMNS][FILL_GRID_ROWS];

// The tracks of a cell, as AddTrack numbers them
enum FillCellTrack
{
	FILL_CELL_TRACK_APPEAR = 0,        // grows in, spinning (also what Init starts)
	FILL_CELL_TRACK_REPLAY = 1,        // the pop-out
	FILL_CELL_TRACK_HIGHLIGHT = 2,     // the glow of a cell under the rope's preview
	FILL_CELL_TRACK_UNHIGHLIGHT = 3,   // the glow fades
};

// The events the cell's keys end with (the key's mEventId), which Notify tells apart
enum FillCellEvent
{
	FILL_EVENT_LANDED = 1,             // the picture has landed: the cell is counted as cut and hidden, and a bonus under it uncovered
	FILL_EVENT_RESET_SHOWN = 2,        // the cell may be counted again
	FILL_EVENT_SETTLED = 3,            // the cell has grown in
	FILL_EVENT_SHIFT_PIPES = 4,        // the pipes at the cell's corners are queued to move
	FILL_EVENT_FILL_END = 5,           // the picture's last key but one
	FILL_EVENT_GLOW_REPEAT = 6,        // a glow key ended: the net fly flashes again
	FILL_EVENT_GLOW_FADE = 7,          // the net fly's flash is part way through
	FILL_EVENT_UNUSED_POP = 8,         // the pop-out's last key; Notify does nothing with it
	FILL_EVENT_PICTURE_START = 98,     // the picture's first key (with the sound)
	FILL_EVENT_WAVE_PLACEHOLDER = 99,  // the key whose event becomes the cut's wave number
};

class FillObj : public ObjBase
{
public:
	// The cell's fields (offsets are the original's, from the object)
	int mColumn;                       // +0x27c
	int mRow;                          // +0x280
	int mUnused284;                    // +0x284: nothing reads it (the bonus under the cell keeps its own pointer to the cell at this offset of its own object)
	int mUnused288;                    // +0x288: nothing reads it
	int mSide;                         // +0x28c: which side of the rope the cell lies on (2 or 3 while a cut is on); the level writes and reads it, this class never does
	bool mHighlighted;                 // +0x290: the cell glows (the rope's preview crosses it)
	bool mCut;                         // +0x291: the cell has been cut away
	bool mCutShown;                    // +0x292: the cell has already been counted and hidden
	int mMark;                         // +0x294: set by the level as it lays the grid out; negative for a cell inside the field (Settle)
	int mShiftPending;                 // +0x298: pipes are queued to move once the wave reaches this cell
	PipeObj* mShiftPipes[FILL_SHIFT_MAX]; // +0x29c: the pipes at the cell's corners that move when it is cut
	int mShiftCount;                   // +0x2ac: how many of mShiftPipes are queued
	bool mLastCut;                     // +0x2b0: this is the cell the wave reached last
	BonusCoin* mBonus;               // +0x2b4: the bonus uncovered when the cell goes, if any
	bool mFillDone;                    // +0x2b8: the picture's last key but one ended (guess: nothing in this class reads it)
	int mWave;                         // +0x2bc: the wave number this cell was cut in; 0 when no pipe move waits
	ObjBase* mNetFly;                  // +0x2c0: the net fly picture that flashes on a glowing cell
	ObjBase* mFillPicture;             // +0x2c4: the cel of the level's picture that fills in
	AnimKey* mWaveKey;                 // +0x2c8: the picture's key whose event is the wave number
	AnimKey* mNetFlyKey;               // +0x2cc: the net fly's flash key
	AnimKey* mNetFlyKeyAfter;          // +0x2d0: the net fly's flash key of the second track
	AnimTrack* mHighlightTrack;        // +0x2d4: the cell's glow track

	// The wave and the counts the cells keep between them (globals in the original)
	static int sFillWave;              // 0x6005b8: the wave number the next cut uses, 100 to 110
	static float sNextDepth;           // 0x600fe0: the depth the next cell takes, 0.0005 above the last
	static int sCellsShown;            // 0x60e4e4: cells in the field
	static int sCellsSettled;          // 0x60e4e8: cells that have grown in
	static int sCellsCut;              // 0x60e4ec: cells the current cut has started on
	static int sCellsArrived;          // 0x60e4f0: cells the current cut's wave has reached
	static int sFarthestDistance;      // 0x60e4f4: the greatest frames the wave takes to a cell
	static FillObj* sFarthestCell;     // 0x60e4f8: that cell
	static int sPendingWave;           // 0x60e4fc: the wave number pipe moves have been released up to
	static int sBusy;                  // 0x60e500: cut waves still to land
	static Anim* sFillSheet;           // 0x60e504: the level's picture, cut in 31-pixel cels
	static Anim* sNetFlyAnim;          // 0x60e508: "images/netfly"

	FillObj();
	~FillObj();
	void Init() override;                              // 0x428300: back to a whole, hidden, idle cell
	void Notify(ObjBase* theObj) override;             // 0x428360: the keys' events
	bool Update() override;                            // 0x428e90

	void QueuePipe(PipeObj* thePipe, float theX, float theY); // 0x4287a0
	void QueueCornerPipes();                           // 0x4287e0
	void ReleaseQueuedPipes();                         // 0x428940
	void SetBonus(BonusCoin* theBonus);              // 0x428970
	void ShowPicture();                                // 0x428990
	void Cut(int theFromColumn, int theFromRow);       // 0x4289e0
	void Highlight(int theFromColumn, int theFromRow); // 0x428ac0
	void Unhighlight();                                // 0x428c90
	void SetMark(int theMark);                         // 0x428cc0
	void Settle();                                     // 0x428e00
	void Replay();                                     // 0x428e60

	static FillObj* Create(int theColumn, int theRow); // 0x42a9c0
	static void ClearBonuses();                        // 0x42aba0
	static void StartWave();                           // 0x42abf0
	static bool IsIdle() { return sBusy == 0; }        // 0x42ac30
	static void AddBusy() { sBusy = sBusy + 1; }       // 0x42ac40
	static void MarkFarthest();                        // 0x42ac50
	static bool IsReady();                             // 0x42ac70
	static void LoadSheet();                           // 0x42ac90
	static void FreeSheet();                           // 0x42ad10

	// The grid's slots: the pipe at a corner and the cell of a place (no check of the place, as in the original)
	static PipeObj* GetWall(int theColumn, int theRow);
	static FillObj* GetTile(int theColumn, int theRow);

private:
	void BuildNetFly();                                // 0x42a0d0
	void BuildCellTracks(int theRow);                  // 0x4298f0
	void BuildPictureTracks(int theColumn);            // 0x428ee0
};

void PreloadNetFly(); // 0x42ad40: the net fly's picture (FillObj::sNetFlyAnim), loaded once

} // namespace Sexy
