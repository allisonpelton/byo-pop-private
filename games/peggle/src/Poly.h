// Poly.h: a shape of a level made of lines (the original's Sexy::Poly, a PhysObj of kind 5): a list of points, joined by Line
// pieces that the ball bounces off, with a position, a turn and a scale. A level's pictures and its bricks (the Brick and Gun
// pieces are made from it) are Polys too. The lines are kept in step with the position and the turn only when something asks for
// them (EnsureLines), since a piece that a mover turns would otherwise rebuild them every update. The level editor's functions
// (outline, points, hit tests, dragging, reflecting, its property hook) are left out.

#ifndef __POLY_H__
#define __POLY_H__

#include "Line.h"
#include "PhysObj.h"
#include "Rect.h"
#include "SexyVector.h"

#include <vector>

namespace Sexy
{

class Poly : public PhysObj
{
public:
	Poly();			   // 0x481670
	~Poly() override;  // 0x481770 (0x483300)

	void SyncState(DataSync& theSync) override;					 // 2, 0x488010
	int GetClass() const override { return CLASS_POLY; }		 // 3, 0x481720
	void SetPos(float theX, float theY) override;				 // 17, 0x476d70
	void Translate(float theDeltaX, float theDeltaY) override;	 // 18, 0x476d30
	void SetVelocity(float theDeltaX, float theDeltaY) override; // 19, 0x47bcd0
	void Update() override;										 // 22, 0x47bd50
	void UpdateSlowMo(float theSubFrame) override;				 // 23, 0x47d8c0
	void DrawShadow(Graphics* g) override;						 // 24, 0x47a160
	void Draw(Graphics* g) override;							 // 25, 0x47e7e0
	void NotifyCollision() override;							 // 28, 0x47b9d0
	void GetCenter(float* theCenter) const override;			 // 29, 0x481750
	float GetXPos() const override { return mX; }				 // 30, 0x481730
	float GetYPos() const override { return mY; }				 // 31, 0x481740
	void SetActiveWithGrowAnim(bool theActive) override;		 // 33, 0x476de0

	// The part of the Poly that the Brick and the Gun share: what is read before their own. With theSyncPoints the list of points is
	// read too (a Poly has it, the others are made from their own numbers).
	void SyncCommon(DataSync& theSync, bool theSyncPoints); // 0x487d00
	void ResetState();										// 0x476b90: the turn, scale and grow state back to their defaults

	void ActualSetPos(float theX, float theY);		// 0x47bb90: puts the lines where the piece is now
	void CalcBoundingBox();							// 0x47bab0: the box round the lines
	void CalcRotatedBoundingBox(float theAngle);	// 0x47a010: the box round the points turned by theAngle (the lines wait until EnsureLines)
	void EnsureLines();								// 0x47d880: brings the lines up to date with a move or a turn that was only noted
	void MakeRect(float theX, float theY, float theWidth, float theHeight); // 0x486d00: a rectangle of that box, as the points of its corners
	void InitFromPoints();							// 0x484730: makes the lines from the points
	void InitRotatedFromPoints(float theAngle);		// 0x47d5b0: the same with the points turned by theAngle
	void CheckShape();								// 0x47d3a0: finds out if the points close up and if the shape is convex
	void SetRotation(float theAngle);				// 0x47d810
	void SetScale(float theScale);					// 0x484a80
	bool PointInside(float theX, float theY);		// 0x47d930: counts the lines a ray to the right crosses
	bool GetStandardRect(Rect& theRect);			// 0x47e390: the box of an unturned rectangle of four lines, as numbers of pixels

	// 0x47dfe0: tests a ball (centre thePos, speed theVel, radius theRadius) against the lines. The results are as Line's, and
	// what a turning Poly gives the ball at the contact is added to theSurfaceVelocity.
	bool CheckCollision(const SexyVector2& thePos, const SexyVector2& theVel, float theRadius, float& theTime,
						SexyVector2& theContact, SexyVector2& theNormal, SexyVector2& theBounce, SexyVector2& theSurfaceVelocity);

	std::vector<SexyVector2> mPoints;		// +0xe4: the shape's points, round the origin; a closed shape ends with its first point again
	std::vector<SexyVector2> mLineOffsets;	// +0xf4: where each line starts, from the position
	std::vector<SharedPtr<Line> > mLines;	// +0x104: one line for each two points
	float mX;								// +0x114: the position
	float mY;								// +0x118
	float mRotation;						// +0x11c (poly_rotation): the turn, in radians
	float mAngularVelocity;					// +0x120: how far the mover turned it in the last update, -pi to pi
	float mScale;							// +0x124 (poly_scale)
	float mPreviousRotation;				// +0x128: the turn as of the last update
	bool mIsRectangle;						// +0x12c: the shape is a rectangle that GetStandardRect may give
	bool mConvex;							// +0x12d: no turn of the outline is the other way round (set by CheckShape)
	bool mClosed;							// +0x12e: the outline ends where it starts (set by CheckShape)
	bool mUnknown12F;						// +0x12f: a flag of the level file
	bool mMirrorImage;						// +0x130 (poly_imagemirror): the picture is drawn mirrored
	int mNormalDir;							// +0x134 (poly_normaldir): 1 solid on the normals' side, 0 solid on both, -1 the other way
	int mGrowTimer;							// +0x138: updates left of the grow or shrink animation
	int mGrowType;							// +0x13c (poly_growtype): 0x100 the piece is shrinking away, 0x1000 it comes back
	int mCountdown;							// +0x140: updates until the piece may come back
	bool mUnknown144;						// +0x144: a flag of the level file (set unless read)
	bool mRotationDirty;					// +0x145: the lines are not turned as the piece is
	bool mPositionDirty;					// +0x146: the lines are not where the piece is
};

} // namespace Sexy

#endif
