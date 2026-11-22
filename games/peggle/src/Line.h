// Line.h: a straight wall of a level (the original's Sexy::Line, a PhysObj of kind 4). A ball bounces off it from the side its
// normal points to, or from both when the line is not one-sided. The level editor's functions (outline, points, hit tests,
// dragging, reflecting) are left out.

#ifndef __LINE_H__
#define __LINE_H__

#include "PhysObj.h"
#include "SexyVector.h"

namespace Sexy
{

class Line : public PhysObj
{
public:
	Line(float theX1 = 0, float theY1 = 0, float theX2 = 0, float theY2 = 0); // 0x481480

	void SyncState(DataSync& theSync) override;			   // 2, 0x4814f0
	int GetClass() const override { return CLASS_LINE; }	   // 3, 0x47df80
	void SetPos(float theX, float theY) override;			   // 17, 0x478440: moves the first end to (theX, theY), keeping the line's length
	void Translate(float theDeltaX, float theDeltaY) override; // 18, 0x475620
	void SetVelocity(float theDeltaX, float theDeltaY) override; // 19, 0x475600
	void Draw(Graphics* g) override;					   // 25, 0x476390
	float GetXPos() const override { return mX1; }		   // 30, 0x47df90
	float GetYPos() const override { return mY1; }		   // 31, 0x47dfa0

	void Init(float theX1, float theY1, float theX2, float theY2); // 0x4782f0: sets the ends and what is worked out from them

	// The tests of a ball (centre thePos, speed theVel for one update, radius theRadius) against the line. They return true
	// when the ball hits, with the time of the hit as a part of the update in theTime, the ball's place at the hit in
	// theContact, the normal in theNormal and the speed the wall gives in theBounce.
	bool CheckCollision(const SexyVector2& thePos, const SexyVector2& theVel, float theRadius, float& theTime,
						SexyVector2& theContact, SexyVector2& theNormal, SexyVector2& theBounce, bool theCheckSecondEnd); // 0x478940
	bool CheckPenetration(const SexyVector2& thePos, float theRadius, SexyVector2& theContact, SexyVector2& theNormal,
						  SexyVector2& theBounce); // 0x478500: the ball is already inside the line's thickness
	bool CheckEdgeTimeCollision(const SexyVector2& thePos, const SexyVector2& theVel, float theRadius, SexyVector2& theContact,
								SexyVector2& theNormal, SexyVector2& theBounce, float& theTime); // 0x475890: the ball reaches the line's side
	bool CheckVertexTimeCollision(const SexyVector2& thePos, const SexyVector2& theVel, float theRadius, float& theTime,
								  SexyVector2& theContact, float theVertexX, float theVertexY, SexyVector2& theNormal,
								  SexyVector2& theBounce); // 0x4756c0: the ball reaches an end of the line
	int IntersectCircle(float theX, float theY, float theRadius, float* thePoint1, float* thePoint2); // 0x475bd0: the points where the line crosses the circle (the number of them)
	void GetWallBounce(const SexyVector2& theNormal, SexyVector2& theBounce) const; // 0x475660: the speed of the wall along the normal

	float mDeltaX;			// +0xe4: the second end minus the first
	float mDeltaY;			// +0xe8
	float mX1;				// +0xec: the first end
	float mX2;				// +0xf0: the second end
	float mY1;				// +0xf4
	float mY2;				// +0xf8
	float mVelocityX;		// +0xfc: how far the line moved in the last update (set by the mover)
	float mVelocityY;		// +0x100
	float mNormalX;			// +0x104: the unit normal of the line
	float mNormalY;			// +0x108
	float mOffset;			// +0x10c: the line is normal . (x, y) + mOffset = 0
	bool mVertical;			// +0x110: the line runs more up and down than across
	bool mOneSided;			// +0x111: only the side the normal points to is solid
};

extern bool gAllowParallelContact; // 0x650a54: set at the start of a ball's update; a ball that runs along a wall without approaching it is taken by it once

} // namespace Sexy

#endif
