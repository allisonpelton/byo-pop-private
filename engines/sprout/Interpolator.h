#ifndef __SPROUT_INTERPOLATOR_H__
#define __SPROUT_INTERPOLATOR_H__

// The signal build's keyframed values (Pizza Frenzy's; Iggle Pop's Mac build names PositionInterpolator,
// OpacityInterpolator and the scale ones). A value is kept at key times; between two keys it follows an ease curve
// that leaves the first at its out speed for its out time and reaches the second at its in speed over its in time,
// with a straight run between. Before the first key the value is the first key's, after the last the last's.

#include "Geometry.h"
#include <vector>

namespace engine
{

class Node;

// The curve over one stretch between two keys (0x24 bytes at the interpolator's +0x1c): from 0 to 1 as the time goes
// from 0 to 1, first speeding up from mStartSpeed until mAccelEnd, then straight at mRunSpeed until mDecelStart, then
// slowing to mEndSpeed
class EaseCurve
{
public:
	// 0x494710 (pizzafrenzy)
	void set(float theStartSpeed, float theEndSpeed, float theAccelEnd, float theDecelStart);
	// 0x4947e0 (pizzafrenzy)
	float get(float theTime) const;

	float mStartSpeed;  // +0x00
	float mEndSpeed;    // +0x04
	float mAccelEnd;    // +0x08
	float mDecelStart;  // +0x0c
	float mRunSpeed;    // +0x10
	float mAccel;       // +0x14: half the acceleration of the first part
	float mDecel;       // +0x18: half the (negative) acceleration of the last part
	float mAccelValue;  // +0x1c: the value at mAccelEnd
	float mDecelValue;  // +0x20: the value at mDecelStart
};

// A key: its time, its value and how the curve meets it (each key's fields follow its time in the map's node, +0xc on)
struct PositionKey
{
	float mTime;      // +0x00
	Point mValue;     // +0x04
	float mInTime;    // +0x0c: the part of the stretch before the key the curve slows over
	float mInSpeed;   // +0x10
	float mOutTime;   // +0x14: the part of the stretch after the key the curve speeds up over
	float mOutSpeed;  // +0x18
};

// A position over time (0x50 bytes, constructor 0x42c890 (pizzafrenzy), on the interpolator 0x480eb0 (pizzafrenzy);
// vtable 0x4fc1c0 (pizzafrenzy)): its modifier (+0x40, vtable 0x4fc1bc (pizzafrenzy)) sets a node's position
class PositionInterpolator
{
public:
	PositionInterpolator();  // 0x42c890 (pizzafrenzy)

	// 0x04: 0x480c70 (pizzafrenzy): a key at a time that already has one is not added; the key added (or the one
	// there) becomes the current one
	void addKey(const PositionKey& theKey);
	// 0x08: 0x47fe60 (pizzafrenzy)
	void clear();
	// 0x0c: 0x47fea0 (pizzafrenzy): the value at a time; the current key moves to the one the time is past, and the
	// curve is made again when it moves
	Point get(float theTime);
	// 0x18: 0x42c930 (pizzafrenzy)
	void apply(Node* theNode, const Point& theValue);
	// The modifier's slot 0: 0x42c650 (pizzafrenzy)
	void modify(float theTime, Node* theNode) { apply(theNode, get(theTime)); }

private:
	// 0x47fc70 (pizzafrenzy): the curve from a key to the next
	void setCurve(const PositionKey& theKey, const PositionKey& theNext);

	std::vector<PositionKey> mKeys;  // +0x0c: by time (a map in the original)
	size_t mCurrent;                 // +0x18: the current key, mKeys.size() for none
	EaseCurve mCurve;                // +0x1c
};

} // namespace engine

#endif
