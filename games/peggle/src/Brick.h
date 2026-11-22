// Brick.h: a curved or straight brick of a level (the original's Sexy::Brick, a Poly of class 6). The file holds the numbers
// of the shape, the way a level's editor draws it: a piece of a ring (an inner and an outer side, each a curve or a straight
// side), or a straight bar. InitFromParams makes the Poly's points from those numbers. The level editor's functions (the
// property hook, the point drag, the reflection, the anchoring to the next brick) are left out. The pictures are made and
// drawn with the rest of the drawing.

#ifndef __BRICK_H__
#define __BRICK_H__

#include "BlendedImage.h"
#include "Poly.h"

namespace Sexy
{

class MemoryImage;

// The values of mType (+0x16c, brick_type): the shape of the brick's two sides
enum BrickType
{
	BRICK_RING = 0,		  // both sides curved
	BRICK_STRAIGHT_INNER = 1, // the inner side is a straight line between its ends, the outer is curved
	BRICK_SLANT_INNER = 2,	  // the inner side is a straight line moved in from the outer ends by the thickness, the outer is curved
	BRICK_STRAIGHT_OUTER = 3, // the inner side is curved, the outer is a straight line between its ends
	BRICK_SLANT_OUTER = 4,	  // the inner side is curved, the outer is a straight line moved out from the inner ends by the thickness
	BRICK_BAR = 5			  // a straight bar of a length (mRadius), with its ends cut at mLeftAngle and mRightAngle
};

class Brick : public Poly
{
public:
	Brick();			// 0x481bd0
	~Brick() override;	// 0x483af0 (0x481d10)

	void SyncState(DataSync& theSync) override;	  // 2, 0x488030
	int GetClass() const override { return CLASS_BRICK; } // 3, 0x481d00
	void DeleteImage() override;				  // 21, 0x477ba0
	void DrawShadow(Graphics* g) override;		  // 24, 0x483b30
	void Draw(Graphics* g) override;			  // 25, 0x483d70

	// The slot 34 of the original: the picture the brick is drawn from, which only a peg has (IMAGE_BRICK)
	Image* GetTextureImage() const; // 0x477b80

	void InitFromParams(); // 0x485670: makes the points (and through them the lines) from the numbers of the shape

	float mRadius;		   // +0x148: the inner radius of the ring; the length of a bar. At most 600 for a bar
	float mAngleSpan;	   // +0x14c: how many degrees of the ring the brick covers (not saved for a bar)
	float mLeftAngle;	   // +0x150 (brick_leftangle): a bar's first end is cut at this angle
	float mRightAngle;	   // +0x154 (brick_rightangle): the other end's angle (written twice in the files)
	float mThickness;	   // +0x158 (brick_thickness): the outer radius less the inner
	float mRotation;	   // +0x15c (brick_rotation): the angle, in degrees, of the middle of the brick round the ring's centre
	float mOffsetX;		   // +0x160: where the Poly's origin is, from the middle of the shape
	float mOffsetY;		   // +0x164
	int mSubdivisions;	   // +0x168 (brick_subdiv): the points added along each curved side
	int mType;			   // +0x16c (brick_type): see BrickType
	int mCorners[4];	   // +0x170: the points (and the lines that start at them) where the inner side ends, the outer side starts, the inner side starts (always 0) and the outer side ends
	bool mTextureFlip;	   // +0x180 (brick_texflip): the picture's strip is turned over
	bool mAnchoredStart;   // +0x181: fixed to the end of another brick (the editor's anchoring)
	bool mAnchoredEnd;	   // +0x182: likewise at the other end
	BlendedImage mBlendedImages[10]; // +0x184: the picture at a sub-pixel offset, for each of the ten kinds
	MemoryImage* mBrickImages[10];	 // +0x4f4: the picture of each kind (null until it is made)
	MemoryImage* mShadowImage;		 // +0x518: the black copy the shadow is drawn from
};

} // namespace Sexy

#endif
