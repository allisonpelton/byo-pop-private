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
// slowing to mEndSpeed. The original computes on the x87, whose registers hold more than a float: the port works in
// double between the stores to memory, where the original rounds to a float (a slide that ends at 204.99997 in float
// arithmetic ends at 205 there, a pixel further)
class EaseCurve
{
public:
	// 0x494710 (pizzafrenzy)
	void set(float theStartSpeed, float theEndSpeed, float theAccelEnd, float theDecelStart);
	// 0x4947e0 (pizzafrenzy): the value as the x87 returns it, not rounded to a float
	double get(float theTime) const;

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
	// 0x14: 0x480030 (pizzafrenzy): the last key's time (0 with no key)
	float getLastTime() const { return mKeys.empty() ? 0.0f : mKeys.back().mTime; }

private:
	// 0x47fc70 (pizzafrenzy): the curve from a key to the next
	void setCurve(const PositionKey& theKey, const PositionKey& theNext);

	std::vector<PositionKey> mKeys;  // +0x0c: by time (a map in the original)
	size_t mCurrent;                 // +0x18: the current key, mKeys.size() for none
	EaseCurve mCurve;                // +0x1c
};

// A key of a number over time (the same fields as a position's, with one value)
struct ScalarKey
{
	float mTime;      // +0x00
	float mValue;     // +0x04
	float mInTime;    // +0x08
	float mInSpeed;   // +0x0c
	float mOutTime;   // +0x10
	float mOutSpeed;  // +0x14
};

// A number over time (0x50 bytes, on the interpolator 0x480d00 (pizzafrenzy), vtable 0x4fc12c (pizzafrenzy) with the
// modifier 0x42c600 (pizzafrenzy) at +0x40): the keys and curves of a position, with one value. What it sets is its
// subclass's.
class ScalarInterpolator
{
public:
	virtual ~ScalarInterpolator() {}

	// 0x04: 0x480c40 (pizzafrenzy)
	void addKey(const ScalarKey& theKey);
	// 0x08: 0x47fcf0 (pizzafrenzy)
	void clear();
	// 0x0c: 0x47fd30 (pizzafrenzy): the value at a time, as the position's is (0 with no key)
	float get(float theTime);
	// 0x14: 0x480000 (pizzafrenzy): the last key's time (0 with no key)
	float getLastTime() const { return mKeys.empty() ? 0.0f : mKeys.back().mTime; }
	// 0x18: the node takes the value
	virtual void apply(Node* theNode, float theValue) = 0;
	// The modifier's slot 0: 0x42c600 (pizzafrenzy)
	void modify(float theTime, Node* theNode) { apply(theNode, get(theTime)); }

private:
	// 0x47fbf0 (pizzafrenzy): the curve from a key to the next
	void setCurve(const ScalarKey& theKey, const ScalarKey& theNext);

	std::vector<ScalarKey> mKeys;  // +0x0c
	size_t mCurrent = 0;           // +0x18
	EaseCurve mCurve;              // +0x1c
};

// 0x42c710 (pizzafrenzy), apply 0x42c7b0 (pizzafrenzy): the node's scale
class ScaleInterpolator : public ScalarInterpolator
{
public:
	void apply(Node* theNode, float theValue) override;
};

// 0x42c7d0 (pizzafrenzy), apply 0x42c870 (pizzafrenzy): the node's opacity
class OpacityInterpolator : public ScalarInterpolator
{
public:
	void apply(Node* theNode, float theValue) override;
};

} // namespace engine

#endif
