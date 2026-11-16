// Mover.h: how a piece of a level moves (the original's Sexy::Mover). A mover has a shape (a line, a circle, a figure eight, a
// swing, a spin, a sawtooth ...), a size, and a period in updates; CalcPos gives where it is for an update. It may follow
// the mover of another piece (a platform carrying a brick). The editor's property hook (0x47b490, 0x479450 DrawMovement) is
// left out.

#ifndef __MOVER_H__
#define __MOVER_H__

#include "SharedPtr.h"

namespace Sexy
{

class DataSync;

class Mover : public RefCounted
{
public:
	Mover();				// 0x47b430
	~Mover() override {}

	// The shapes, by the absolute value of mType; its sign turns the movement around
	enum MoverShape
	{
		SHAPE_NONE = 0,
		SHAPE_VERTICAL = 1,		   // up and down by the second radius
		SHAPE_HORIZONTAL = 2,	   // left and right
		SHAPE_CIRCLE = 3,		   // an ellipse with the two radii
		SHAPE_FIGURE_EIGHT = 4,	   // lying along x: one turn of x for two of y
		SHAPE_FIGURE_EIGHT_UP = 5, // standing: two turns of x for one of y
		SHAPE_SWING_ARC = 6,	   // swings through the maximum angle on an arc (x by cosine)
		SHAPE_SWING_ARC_FLIPPED = 7,
		SHAPE_SPIN = 8,			   // turns the piece, or holds it at the maximum angle
		SHAPE_ROCK = 9,			   // rocks the piece to and fro
		SHAPE_UNUSED = 10,		   // sets nothing
		SHAPE_SAWTOOTH_UP = 11,	   // rises along y and starts again
		SHAPE_SAWTOOTH_ACROSS = 12, // moves along x and starts again
		SHAPE_CIRCLE_TURNING = 13  // a circle that turns the piece with its tangent
	};

	void Reset();					 // 0x47a980: the settings of a mover that does not move
	void SyncState(DataSync& theSync); // 0x47a9f0
	void Shift(float theDeltaX, float theDeltaY); // 0x476420: moves the whole path
	float GetCyclePosition(int theTime) const; // 0x4764f0: how far through its cycle it is at an update, 0 up to 1
	int GetTimeToPhase(int theTime, float thePhase) const; // 0x476460: the updates from theTime until the cycle is at thePhase (0 when it has no cycle)
	static int WrapTime(int theTime, int thePeriod); // 0x476400: theTime brought into 0 up to thePeriod
	// 0x478ea0: sets mX and mY (and mAngle) for the update theTime; theSubFrame is how far (in updates) the picture is
	// drawn between two updates (the slow motion's steps)
	void CalcPos(int theTime, float theSubFrame);

	int mType;				 // +0x10 (mover_type): the shape; 0 means none; negative runs backwards
	int mOffset;			 // +0x14 (mover_offset): updates added to the time
	int mRadius;			 // +0x18 (mover_radius)
	int mPeriod;			 // +0x1c (mover_time): updates for one cycle (200 unless read)
	float mPhase;			 // +0x20 (mover_phase): where in the cycle it starts, 0 to 1
	float mPathRotation;	 // +0x24 (mover_moverotation): turns the whole path about the center
	int mRadius2;			 // +0x28 (mover_radius2): the second radius; the first is used when it is 0
	int mPause1;			 // +0x2c (mover_pause1): updates it holds still at the first phase point
	int mPhase1;			 // +0x30 (mover_phase1): percent of the cycle where the first pause comes
	int mPause2;			 // +0x34 (mover_pause2)
	int mPhase2;			 // +0x38 (mover_phase2)
	float mMaxAngle;		 // +0x3c (mover_maxangle): radians; 0 means a half turn for the swing shapes
	float mPostDelayPhase;	 // +0x40 (mover_postdelayphase): where in the cycle the pauses begin, -1 to 1
	float mRotation;		 // +0x44 (mover_rotation): the angle of a piece that does not turn (radians)
	SharedPtr<Mover> mFollowed; // +0x48: the mover whose movements this one repeats
	float mFollowedX;		 // +0x4c: where that mover was at the last update
	float mFollowedY;		 // +0x50
	float mX;				 // +0x54: where the piece is now
	float mY;				 // +0x58
	float mCenterX;			 // +0x5c: the middle of the path
	float mCenterY;			 // +0x60
	float mAngle;			 // +0x64: the angle of the piece now
};

} // namespace Sexy

#endif
