#ifndef __FEEDINGFRENZY_COLLISIONRECT_H__
#define __FEEDINGFRENZY_COLLISIONRECT_H__

// A rectangle that fish are tested against each other with (CollisionRect, 0x14 bytes, constructor 0x42a6c0, vtable
// 0x4a0ac0, destructor 0x42a710). It holds a rectangle (+0x04) and answers whether it overlaps another. Its slot 0x18
// (0x42a830) draws the rectangle as an outline for the debug view, which no part of the game turns on; the port leaves it
// out.

#include "Geometry.h"

namespace feedingfrenzy
{

class CollisionRect
{
public:
	CollisionRect() {}  // 0x42a6c0

	void set(const engine::Rect& theRect) { mRect = theRect; }  // 0x00: 0x42a730
	// 0x04: 0x42a720
	void set(float theLeft, float theTop, float theRight, float theBottom) { mRect.set(theLeft, theTop, theRight, theBottom); }
	void offset(float theX, float theY) { mRect.offset(theX, theY); }  // 0x08: 0x42a750
	void scale(float theX, float theY) { mRect.scale(theX, theY); }    // 0x0c: 0x42a760
	// 0x10: 0x42a820
	bool intersects(const engine::Rect& theRect) const { return mRect.intersects(theRect); }
	// 0x14: 0x42a780: the other's rectangle is read through its slot 0x1c
	bool intersects(const CollisionRect& theOther) const { return mRect.intersects(theOther.mRect); }
	const engine::Rect& getRect() const { return mRect; }  // 0x1c: 0x42a770
	engine::Rect& getRect() { return mRect; }

private:
	engine::Rect mRect;  // +0x04
};

} // namespace feedingfrenzy

#endif
