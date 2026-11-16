#ifndef __SPROUT2_GEOMETRY_H__
#define __SPROUT2_GEOMETRY_H__

// The engine's value types (port: the original's are templates and structs its RTTI names only through other
// classes: engine::Vector2<float>, engine::Color). A rectangle is four values, left, top, right and bottom, as the
// engine keeps it: in floats for a component's bounds, in ints for an image's source rectangle.

#include <cmath>

namespace engine
{

// The engine's pi, a float (0x5a73d8 and 0x5a7384 (feedingfrenzy2))
const float PI = 3.1415927f;

// 0x43cdb0 (feedingfrenzy2): the sine of a float, computed in double and rounded to a float
inline float sinOf(float theAngle)
{
	return (float)sin((double)theAngle);
}

template <class T>
struct Vector2
{
	T x;
	T y;

	Vector2() : x(0), y(0) {}
	Vector2(T theX, T theY) : x(theX), y(theY) {}
};

typedef Vector2<float> Vector2f;

struct Color
{
	float r;
	float g;
	float b;
	float a;

	Color() : r(0), g(0), b(0), a(1) {}
	Color(float theRed, float theGreen, float theBlue, float theAlpha) : r(theRed), g(theGreen), b(theBlue), a(theAlpha)
	{
	}
};

template <class T>
struct Rect
{
	T left;
	T top;
	T right;
	T bottom;

	Rect() : left(0), top(0), right(0), bottom(0) {}
	Rect(T theLeft, T theTop, T theRight, T theBottom) : left(theLeft), top(theTop), right(theRight), bottom(theBottom) {}

	// 0x41eb90 (feedingfrenzy2): inside, the left and top edges included and the right and bottom ones not
	bool contains(T x, T y) const { return left <= x && x < right && top <= y && y < bottom; }
};

typedef Rect<float> RectF;
typedef Rect<int> RectI;

// Two ends a random value is drawn between (a particle's speeds, sizes, lives; read by Params::getRange)
struct FloatRange
{
	float mLow;
	float mHigh;

	FloatRange() : mLow(0), mHigh(0) {}
	FloatRange(float theLow, float theHigh) : mLow(theLow), mHigh(theHigh) {}
};

// The same of whole numbers (read by Params::getIntRange)
struct IntRange
{
	int mLow;
	int mHigh;

	IntRange() : mLow(0), mHigh(0) {}
	IntRange(int theLow, int theHigh) : mLow(theLow), mHigh(theHigh) {}
};

// 0x46d360 (feedingfrenzy2): the smaller, the first on a tie
template <class T>
inline T minOf(T a, T b)
{
	return a > b ? b : a;
}

// 0x428090 (feedingfrenzy2): the larger, the first on a tie
template <class T>
inline T maxOf(T a, T b)
{
	return b > a ? b : a;
}

// 0x46c8e0 (feedingfrenzy2): the two closer than 2^-22, their difference rounded to a float first
inline bool nearlyEqual(float a, float b)
{
	float aDifference = (float)((double)a - (double)b);
	if (aDifference < 0.0f)
		aDifference = -aDifference;
	return 2.384185791015625e-07f > aDifference;
}

// 0x42b890 (feedingfrenzy2): the value held between the two bounds
template <class T>
inline T clampOf(T theValue, T theLow, T theHigh)
{
	if (theLow > theValue)
		return theLow;
	if (theHigh < theValue)
		return theHigh;
	return theValue;
}

} // namespace engine

#endif
