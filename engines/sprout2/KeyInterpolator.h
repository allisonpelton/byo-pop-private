#ifndef __SPROUT2_KEYINTERPOLATOR_H__
#define __SPROUT2_KEYINTERPOLATOR_H__

// A value kept at key times (the original's Interpolator<T> and KeyInterpolator<T>, templates over float,
// engine::Color and engine::Vector2<float>; only the float one is translated so far). Between two keys the value
// follows an ease curve that leaves the first key at its out speed over its out time and reaches the second at its in
// speed over its in time, with a straight run between. Before the first key the value is the first key's, after the
// last the last's. The interpolator keeps the key it last used and walks from there, and builds the curve again only
// when it moved.

#include <map>

namespace engine
{

// The curve over one stretch between two keys (0x24 bytes at the interpolator's +0x20): from 0 to 1 as the time goes
// from 0 to 1, first speeding up from mStartSpeed until mAccelEnd, then straight at mRunSpeed until mDecelStart, then
// slowing to mEndSpeed
class EaseCurve
{
public:
	// 0x4357a0 (feedingfrenzy2)
	void set(float theStartSpeed, float theEndSpeed, float theAccelEnd, float theDecelStart);
	// Inline in 0x43bdb0 (feedingfrenzy2): kept at the x87's precision for the caller to round
	double get(double theTime) const;

	float mStartSpeed = 0;  // +0x00
	float mEndSpeed = 0;    // +0x04
	float mAccelEnd = 0;    // +0x08
	float mDecelStart = 0;  // +0x0c
	float mRunSpeed = 0;    // +0x10
	float mAccel = 0;       // +0x14: half the acceleration of the first part
	float mDecel = 0;       // +0x18: half the (negative) acceleration of the last part
	float mAccelValue = 0;  // +0x1c: the value at mAccelEnd
	float mDecelValue = 0;  // +0x20: the value at mDecelStart
};

// A key of a float (0x18 bytes; 0x484300 (feedingfrenzy2) makes one at a time with a value and these speeds and
// times, which give a straight line)
struct FloatKey
{
	float mTime;            // +0x00
	float mValue;           // +0x04
	float mInTime = 0.0f;   // +0x08: the part of the stretch before the key the curve slows over
	float mInSpeed = 1.0f;  // +0x0c
	float mOutTime = 0.0f;  // +0x10: the part of the stretch after the key the curve speeds up over
	float mOutSpeed = 1.0f; // +0x14
};

// KeyInterpolator<float> (0x48 bytes with its engine::Object; vtable 0x55ba38 (feedingfrenzy2)). The port keeps it as
// a member of what holds it, uncounted.
class FloatKeyInterpolator
{
public:
	FloatKeyInterpolator() : mCurrent(mKeys.end()) {}
	// The current key is an iterator into the map: a copy would point into the other's
	FloatKeyInterpolator(const FloatKeyInterpolator&) = delete;
	FloatKeyInterpolator& operator=(const FloatKeyInterpolator&) = delete;

	// 0x04 (0x436f00 (feedingfrenzy2)): a key at a time that already has one is not added; the key added (or the one
	// there) becomes the current one
	void addKey(const FloatKey& theKey);
	// 0x08 (0x437240 (feedingfrenzy2)): every key taken out
	void clear();
	// 0x0c (0x43bdb0 (feedingfrenzy2)): the value at the time, kept at the x87's precision for the caller to round
	double getValue(float theTime);

private:
	// A key's fields after its time, in the map's node from +0x10
	struct KeyValue
	{
		float mValue;     // node +0x10
		float mInTime;    // node +0x14
		float mInSpeed;   // node +0x18
		float mOutTime;   // node +0x1c
		float mOutSpeed;  // node +0x20
	};
	typedef std::map<float, KeyValue> KeyMap;

	// 0x435850 (feedingfrenzy2): the curve from the current key's out speed and time to the next one's in speed and
	// time
	void setCurve(KeyMap::iterator theCurrent, KeyMap::iterator theNext);

	KeyMap mKeys;                // +0x0c
	KeyMap::iterator mCurrent;   // +0x18: the key the last value was read after
	EaseCurve mCurve;            // +0x20: from the current key to the next
};

} // namespace engine

#endif
