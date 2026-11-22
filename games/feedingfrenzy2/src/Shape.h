#ifndef __FEEDINGFRENZY2_SHAPE_H__
#define __FEEDINGFRENZY2_SHAPE_H__

// The outlines an actor is hit by and eats with, in the pixels of its model's unscaled image (Shape, and its kinds
// shape::Box and shape::Circle, each an engine object of 26 slots, vtables 0x555a3c and 0x5559bc (feedingfrenzy2)).
// A model names each of its shapes by its role ("body", "mouth" and so on); a visual's data copies them at its scale,
// around its image's anchor. An actor keeps each shape with a transform (its scale, place and turn) and a copy
// transformed by it, and tests copies against each other: two shapes meet where their separation is 0 or less.

#include "Geometry.h"
#include "Object.h"

namespace shape
{
class Box;
class Circle;
class Edge;
}

// Where an actor's shapes are in the stage (an actor's +0x60, 0x18 bytes; set as 0x417230 (feedingfrenzy2) and kept
// by 0x43a8b0 and 0x4690f0 (feedingfrenzy2))
struct ShapeTransform
{
	float mScale;                   // +0x00
	engine::Vector2f mTranslation;  // +0x04
	engine::Vector2f mAxisScale;    // +0x0c: each axis's own scale after the first
	float mRotation;                // +0x14: in radians

	ShapeTransform() : mScale(1.0f), mTranslation(0.0f, 0.0f), mAxisScale(1.0f, 1.0f), mRotation(0.0f) {}
};

// A point laid on the transform: turned (0x5085b3 (feedingfrenzy2)), scaled (0x43c1a0), scaled on each axis
// (0x43dc80) and moved (0x43dca0), as a shape's centre is (0x43dcc0 and 0x43d820) and the mouth is (0x43dc00)
engine::Vector2f transformed(const engine::Vector2f& thePoint, const ShapeTransform& theTransform);

// What a test between two shapes finds (0x14 bytes on the tester's stack)
struct ShapeContact
{
	engine::Vector2f mPoint;    // +0x00: left as it was by the tests translated
	engine::Vector2f mNormal;   // +0x08: the way from the first shape to the second, of length 1
	float mSeparation;          // +0x10: the gap between them, below 0 where they overlap
};

class Shape : public engine::Object
{
public:
	// 0x04: a new copy
	virtual Shape* clone() const = 0;
	// 0x08: the shape copied into the target, or a new copy with none; a target of the other kind is replaced by a
	// new copy, which is returned (a target of the same kind is returned)
	virtual Shape* copyTo(Shape* theTarget) const = 0;
	// 0x0c and 0x10: the points at its left and right ends, level with its centre
	virtual engine::Vector2f getLeft() const = 0;
	virtual engine::Vector2f getRight() const = 0;
	// 0x14
	virtual engine::Vector2f getCenter() const = 0;
	// 0x20 and 0x24
	virtual float getHeight() const = 0;
	virtual float getWidth() const = 0;
	// 0x28: its size and its centre's distance from the origin times the scale
	virtual void scale(float theScale) = 0;
	// 0x2c: moved by the offset
	virtual void translate(const engine::Vector2f& theOffset) = 0;
	// 0x30: its centre turned, scaled, scaled on each axis and moved by the transform, its size scaled
	virtual void transform(const ShapeTransform& theTransform) = 0;
	// 0x34: the test against the other shape (passed to the other's test of this one's kind)
	virtual void collide(const Shape& theOther, ShapeContact& theContact) const = 0;
	// 0x38 (0x43da90 (feedingfrenzy2), both kinds'): the two meet, their separation 0 or less
	bool overlaps(const Shape& theOther) const;
	// 0x3c: the share of its height below the level (a larger y), 0 to 1
	virtual float getShareBelow(float theLevel) const = 0;
	// 0x44 and 0x48: this shape tested against a box or a circle that is the first of the two
	virtual void collideWithBox(const shape::Box& theFirst, ShapeContact& theContact) const = 0;
	virtual void collideWithCircle(const shape::Circle& theFirst, ShapeContact& theContact) const = 0;
	// 0x50: this shape tested against an edge that is the first of the two
	virtual void collideWithEdge(const shape::Edge& theFirst, ShapeContact& theContact) const = 0;
	// 0x54 and 0x58: the box's or the circle's values taken by one of its kind, which is returned; one of the other
	// kind returns a new copy of it instead (0x501b04 (feedingfrenzy2))
	virtual Shape* assignFromBox(const shape::Box& theSource) = 0;
	virtual Shape* assignFromCircle(const shape::Circle& theSource) = 0;
	// 0x60: the same for an edge's values
	virtual Shape* assignFromEdge(const shape::Edge& theSource) = 0;
	// (Slots 0x18, 0x1c, 0x40, 0x4c, 0x5c and 0x64: not yet translated.)

protected:
	// A box's and a circle's share below the level (inlined in each): none when the level is below the bottom
	// (the centre plus the half height), all of it when it is above the top, else the part below over the height
	static float shareBelow(float theLevel, float theCenter, float theHalfHeight)
	{
		if (theLevel > (double)theHalfHeight + theCenter)
			return 0.0f;
		if (theLevel < (double)theCenter - theHalfHeight)
			return 1.0f;
		return (float)(((double)theHalfHeight + theCenter - theLevel) / ((double)theHalfHeight + theHalfHeight));
	}
};

namespace shape
{

// A box (0x24 bytes; its constructor 0x4232a0 (feedingfrenzy2))
class Box : public Shape
{
public:
	Box() : mCenter(0, 0), mDiagonal(0, 0) {}

	// 0x476bc0 (feedingfrenzy2)
	Shape* clone() const override;
	// 0x43de60 (feedingfrenzy2)
	Shape* copyTo(Shape* theTarget) const override;
	// 0x437a90 and 0x403430 (feedingfrenzy2)
	engine::Vector2f getLeft() const override;
	engine::Vector2f getRight() const override;
	// 0x46c7e0 (feedingfrenzy2)
	engine::Vector2f getCenter() const override { return mCenter; }
	// 0x46dee0 and 0x4746c0 (feedingfrenzy2): twice the diagonal's height, and its width
	float getHeight() const override { return mDiagonal.y + mDiagonal.y; }
	float getWidth() const override { return mDiagonal.x + mDiagonal.x; }
	// 0x501e27 (feedingfrenzy2)
	float getShareBelow(float theLevel) const override
	{
		return shareBelow(theLevel, mCenter.y, mDiagonal.y);
	}
	// 0x413000 (feedingfrenzy2)
	void scale(float theScale) override;
	// 0x415330 (feedingfrenzy2)
	void translate(const engine::Vector2f& theOffset) override;
	// 0x43dcc0 (feedingfrenzy2)
	void transform(const ShapeTransform& theTransform) override;
	// 0x43d9b0 (feedingfrenzy2)
	void collide(const Shape& theOther, ShapeContact& theContact) const override
	{
		theOther.collideWithBox(*this, theContact);
	}
	// 0x43d8c0 (feedingfrenzy2): the larger of the gaps along each axis, its axis the normal's
	void collideWithBox(const Box& theFirst, ShapeContact& theContact) const override;
	// 0x43af80 (feedingfrenzy2): the circle's test with the box, its normal turned round
	void collideWithCircle(const Circle& theFirst, ShapeContact& theContact) const override;
	// 0x5171e9 (feedingfrenzy2): the box's centre's distance from the edge less its extent across it, the normal the
	// edge's
	void collideWithEdge(const Edge& theFirst, ShapeContact& theContact) const override;
	// 0x43dea0 (feedingfrenzy2)
	Shape* assignFromBox(const Box& theSource) override;
	// 0x501b04 (feedingfrenzy2)
	Shape* assignFromCircle(const Circle& theSource) override;
	// 0x501b04 (feedingfrenzy2): a new copy of the edge
	Shape* assignFromEdge(const Edge& theSource) override;

	engine::Vector2f mCenter;    // +0x0c
	engine::Vector2f mDiagonal;  // +0x14: from the centre to the lower right corner
};

// A circle (0x20 bytes; its constructor 0x483f80 (feedingfrenzy2))
class Circle : public Shape
{
public:
	Circle() : mCenter(0, 0), mRadius(0) {}

	// 0x48a5d0 (feedingfrenzy2)
	Shape* clone() const override;
	// 0x4455d0 (feedingfrenzy2)
	Shape* copyTo(Shape* theTarget) const override;
	// 0x50200e and 0x501fdf (feedingfrenzy2)
	engine::Vector2f getLeft() const override;
	engine::Vector2f getRight() const override;
	// 0x4108c0 (feedingfrenzy2)
	engine::Vector2f getCenter() const override { return mCenter; }
	// 0x501f73 (feedingfrenzy2), and slot 0x24 (0x40c950 (feedingfrenzy2)) the same: the diameter
	float getHeight() const override { return mRadius + mRadius; }
	float getWidth() const override { return getHeight(); }
	// 0x501f12 (feedingfrenzy2)
	float getShareBelow(float theLevel) const override { return shareBelow(theLevel, mCenter.y, mRadius); }
	// 0x492650 (feedingfrenzy2)
	void scale(float theScale) override;
	// 0x501f5b (feedingfrenzy2)
	void translate(const engine::Vector2f& theOffset) override;
	// 0x43d820 (feedingfrenzy2)
	void transform(const ShapeTransform& theTransform) override;
	// 0x43af60 (feedingfrenzy2)
	void collide(const Shape& theOther, ShapeContact& theContact) const override
	{
		theOther.collideWithCircle(*this, theContact);
	}
	// 0x43af40 (feedingfrenzy2): the box's test with the circle (0x43ce10 (feedingfrenzy2))
	void collideWithBox(const Box& theFirst, ShapeContact& theContact) const override;
	// 0x43ad30 (feedingfrenzy2): the centres' distance less the radii, along the line between them
	void collideWithCircle(const Circle& theFirst, ShapeContact& theContact) const override;
	// 0x50ad36 (feedingfrenzy2): the centre's distance from the edge less the radius, the normal the edge's
	void collideWithEdge(const Edge& theFirst, ShapeContact& theContact) const override;
	// 0x501b04 (feedingfrenzy2)
	Shape* assignFromBox(const Box& theSource) override;
	// 0x50203d (feedingfrenzy2)
	Shape* assignFromCircle(const Circle& theSource) override;
	// 0x501b04 (feedingfrenzy2): a new copy of the edge
	Shape* assignFromEdge(const Edge& theSource) override;

	engine::Vector2f mCenter;  // +0x0c
	float mRadius;             // +0x14
};

// An edge of the play area (shape::Edge, its vtable 0x555b3c (feedingfrenzy2); its constructor 0x48ee20
// (feedingfrenzy2)): the line of the points whose distance along the normal, plus the edge's distance, is 0, the
// play area on the normal's side
class Edge : public Shape
{
public:
	Edge() : mNormal(0, 0), mDistance(0) {}

	// 0x51d6a1 (feedingfrenzy2)
	Shape* clone() const override;
	// 0x501c71 (feedingfrenzy2)
	Shape* copyTo(Shape* theTarget) const override;
	// 0x501c61 (feedingfrenzy2), and 0x10 the same: the origin
	engine::Vector2f getLeft() const override { return engine::Vector2f(0.0f, 0.0f); }
	engine::Vector2f getRight() const override { return engine::Vector2f(0.0f, 0.0f); }
	// 0x501c1a (feedingfrenzy2): the line's point nearest the origin
	engine::Vector2f getCenter() const override;
	// 0x501c17 (feedingfrenzy2), and 0x24 the same
	float getHeight() const override { return 0.0f; }
	float getWidth() const override { return 0.0f; }
	// 0x502064 (feedingfrenzy2): none
	float getShareBelow(float theLevel) const override { return 0.0f; }
	// 0x501c0a (feedingfrenzy2): the distance scaled
	void scale(float theScale) override { mDistance = (float)((double)theScale * mDistance); }
	// 0x5014fa (feedingfrenzy2): nothing, as for the transform
	void translate(const engine::Vector2f& theOffset) override {}
	void transform(const ShapeTransform& theTransform) override {}
	// 0x501bf7 (feedingfrenzy2)
	void collide(const Shape& theOther, ShapeContact& theContact) const override
	{
		theOther.collideWithEdge(*this, theContact);
	}
	// 0x43b8b0 (feedingfrenzy2): the box's test with the edge, its normal turned round
	void collideWithBox(const Box& theFirst, ShapeContact& theContact) const override;
	// 0x51712b (feedingfrenzy2): the circle's test with the edge, its normal turned round (0x50ad6a
	// (feedingfrenzy2))
	void collideWithCircle(const Circle& theFirst, ShapeContact& theContact) const override;
	// 0x502058 (feedingfrenzy2): two edges leave the contact as it was
	void collideWithEdge(const Edge& theFirst, ShapeContact& theContact) const override {}
	// 0x501b04 (feedingfrenzy2): a new copy of the box or the circle
	Shape* assignFromBox(const Box& theSource) override;
	Shape* assignFromCircle(const Circle& theSource) override;
	// 0x4455f0 (feedingfrenzy2): the edge's normal and distance taken
	Shape* assignFromEdge(const Edge& theSource) override;

	engine::Vector2f mNormal;  // +0x0c
	float mDistance;           // +0x14
};

} // namespace shape

#endif
