#ifndef __FEEDINGFRENZY2_SHAPE_H__
#define __FEEDINGFRENZY2_SHAPE_H__

// The outlines an actor is hit by and eats with, in the pixels of its model's unscaled image (Shape, and its kinds
// shape::Box and shape::Circle, each an engine object of 26 slots, vtables 0x555a3c and 0x5559bc (feedingfrenzy2)).
// A model names each of its shapes by its role ("body", "mouth" and so on); a visual's data copies them at its scale,
// around its image's anchor. The tests between shapes are the game's, not yet translated.

#include "Geometry.h"
#include "Object.h"

class Shape : public engine::Object
{
public:
	// 0x04: a new copy
	virtual Shape* clone() const = 0;
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
	// (Slot 0x08, the shape copied into another of its kind, and the tests from 0x18 on: not yet translated.)
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
	// 0x437a90 and 0x403430 (feedingfrenzy2)
	engine::Vector2f getLeft() const override;
	engine::Vector2f getRight() const override;
	// 0x46c7e0 (feedingfrenzy2)
	engine::Vector2f getCenter() const override { return mCenter; }
	// 0x46dee0 and 0x4746c0 (feedingfrenzy2): twice the diagonal's height, and its width
	float getHeight() const override { return mDiagonal.y + mDiagonal.y; }
	float getWidth() const override { return mDiagonal.x + mDiagonal.x; }
	// 0x413000 (feedingfrenzy2)
	void scale(float theScale) override;
	// 0x415330 (feedingfrenzy2)
	void translate(const engine::Vector2f& theOffset) override;

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
	// 0x50200e and 0x501fdf (feedingfrenzy2)
	engine::Vector2f getLeft() const override;
	engine::Vector2f getRight() const override;
	// 0x4108c0 (feedingfrenzy2)
	engine::Vector2f getCenter() const override { return mCenter; }
	// 0x501f73 (feedingfrenzy2), and slot 0x24 (0x40c950 (feedingfrenzy2)) the same: the diameter
	float getHeight() const override { return mRadius + mRadius; }
	float getWidth() const override { return getHeight(); }
	// 0x492650 (feedingfrenzy2)
	void scale(float theScale) override;
	// 0x501f5b (feedingfrenzy2)
	void translate(const engine::Vector2f& theOffset) override;

	engine::Vector2f mCenter;  // +0x0c
	float mRadius;             // +0x14
};

} // namespace shape

#endif
