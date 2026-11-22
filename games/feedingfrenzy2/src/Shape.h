#ifndef __FEEDINGFRENZY2_SHAPE_H__
#define __FEEDINGFRENZY2_SHAPE_H__

// The outlines an actor is hit by and eats with, in the pixels of its model's unscaled image (Shape, and its kinds
// shape::Box and shape::Circle, each an engine object). A model names each of its shapes by its role ("body",
// "mouth" and so on). The tests between shapes are the game's, not yet translated; so far the shapes hold what the
// actors' file gives them.

#include "Geometry.h"
#include "Object.h"

class Shape : public engine::Object
{
};

namespace shape
{

// A box (0x24 bytes; its constructor 0x4232a0 (feedingfrenzy2))
class Box : public Shape
{
public:
	Box() : mCenter(0, 0), mDiagonal(0, 0) {}

	engine::Vector2f mCenter;    // +0x0c
	engine::Vector2f mDiagonal;  // +0x14: from the centre to the lower right corner
};

// A circle (0x20 bytes; its constructor 0x483f80 (feedingfrenzy2))
class Circle : public Shape
{
public:
	Circle() : mCenter(0, 0), mRadius(0) {}

	engine::Vector2f mCenter;  // +0x0c
	float mRadius;             // +0x14
};

} // namespace shape

#endif
