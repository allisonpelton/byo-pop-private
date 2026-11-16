#ifndef __SPROUT_GEOMETRY_H__
#define __SPROUT_GEOMETRY_H__

// The engine's points, rectangles and colours (games/wordharmony/docs/ENGINE.md). Rectangles are {left, top, right,
// bottom}; right and bottom are exclusive.

#include <cmath>

namespace engine
{

struct Point
{
	float x, y;

	Point() : x(0), y(0) {}  // 0x42b490 (wordharmony)
	Point(float theX, float theY) : x(theX), y(theY) {}  // 0x42b4c0 (wordharmony), 0x42b580 (wordharmony)

	// 0x42b520 (wordharmony)
	void scale(float theFactor)
	{
		x = theFactor * x;
		y = theFactor * y;
	}
	// 0x42b540 (wordharmony)
	void add(const Point& p)
	{
		x = p.x + x;
		y = p.y + y;
	}
	// 0x42b560 (wordharmony)
	void subtract(const Point& p)
	{
		x = x - p.x;
		y = y - p.y;
	}
	// 0x42b580 (wordharmony)
	void set(float theX, float theY)
	{
		x = theX;
		y = theY;
	}
	// 0x42b590 (wordharmony): turns the point about the origin (degrees; y down, so a positive angle turns it clockwise
	// on the screen)
	void rotate(float theDegrees)
	{
		double anAngle = theDegrees * 0.017453292519943295;
		double aCos = cos(anAngle);
		double aSin = sin(anAngle);
		float aY = (float)(x * aSin + y * aCos);
		x = (float)(x * aCos - y * aSin);
		y = aY;
	}
	// 0x42b5d0 (wordharmony): to length 1; a zero point stays
	void normalize()
	{
		double aLength = sqrt((double)x * x + (double)y * y);
		if (aLength != 0)
		{
			double anInverse = 1.0f / aLength;
			x = (float)(anInverse * x);
			y = (float)(anInverse * y);
		}
	}
	// 0x440930 (feedingfrenzy)
	float length() const { return (float)sqrt((double)y * y + (double)x * x); }
	// 0x440950 (feedingfrenzy)
	float lengthSquared() const { return (float)((double)y * y + (double)x * x); }
	// 0x440970 (feedingfrenzy): the dot product of this point with p
	float dot(const Point& p) const { return (float)((double)p.x * x + (double)p.y * y); }
};

struct IntRect
{
	int left, top, right, bottom;

	IntRect() : left(0), top(0), right(0), bottom(0) {}
	IntRect(int l, int t, int r, int b) : left(l), top(t), right(r), bottom(b) {}  // 0x43d3a0 (wordharmony)
	int width() const { return right - left; }            // 0x43d150 (wordharmony)
	int height() const { return bottom - top; }           // 0x43d160 (wordharmony)
	bool isEmpty() const { return !(left < right && top < bottom); }  // 0x43d170 (wordharmony)
};

struct Rect
{
	float left, top, right, bottom;

	Rect() : left(0), top(0), right(0), bottom(0) {}  // 0x45ec30 (wordharmony), 0x43d210 (wordharmony)
	Rect(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b) {}
	// 0x43d1f0 (wordharmony)
	void set(float l, float t, float r, float b)
	{
		left = l;
		top = t;
		right = r;
		bottom = b;
	}
	float width() const { return right - left; }    // 0x43d1b0 (wordharmony)
	float height() const { return bottom - top; }   // 0x43d1c0 (wordharmony)
	bool isEmpty() const { return !(left < right && top < bottom); }  // 0x43d220 (wordharmony)
	// 0x43d250 (wordharmony)
	void scale(float sx, float sy)
	{
		left = sx * left;
		right = sx * right;
		top = sy * top;
		bottom = sy * bottom;
	}
	// 0x43d280 (wordharmony): r lies inside
	bool contains(const Rect& r) const
	{
		return r.right <= right && r.bottom <= bottom && left <= r.left && top <= r.top;
	}
	// 0x43d2d0 (wordharmony)
	bool contains(float x, float y) const { return left <= x && x < right && top <= y && y < bottom; }
	// 0x440ba0 (feedingfrenzy): the two rectangles overlap; a rectangle with no width or no height overlaps nothing
	bool intersects(const Rect& r) const
	{
		if (!(0.0f < r.right - r.left) || !(0.0f < r.bottom - r.top) || !(0.0f < right - left) || !(0.0f < bottom - top))
			return false;
		return left < r.right && top < r.bottom && r.left < right && r.top < bottom;
	}
	// 0x43d320 (wordharmony)
	void offset(float dx, float dy)
	{
		left = dx + left;
		right = dx + right;
		top = dy + top;
		bottom = dy + bottom;
	}
	// 0x43d450 (wordharmony): grows to include r
	void unite(const Rect& r)
	{
		left = left <= r.left ? left : r.left;
		right = r.right <= right ? right : r.right;
		top = top <= r.top ? top : r.top;
		bottom = bottom < r.bottom ? r.bottom : bottom;
	}
	// 0x43d4d0 (wordharmony): left <= right and top <= bottom
	void normalize()
	{
		if (right < left)
		{
			float t = right;
			right = left;
			left = t;
		}
		if (bottom < top)
		{
			float t = bottom;
			bottom = top;
			top = t;
		}
	}
};

// Three float components, 0..1 (0x43ed30 (wordharmony); 0x43ed50 (wordharmony) sets them)
struct Color
{
	float r, g, b;

	Color() : r(0), g(0), b(0) {}
	// 0x43ed30 (wordharmony)
	Color(float theR, float theG, float theB) : r(theR), g(theG), b(theB) {}
	// 0x43ed50 (wordharmony)
	void set(float theR, float theG, float theB)
	{
		r = theR;
		g = theG;
		b = theB;
	}
};

} // namespace engine

#endif
