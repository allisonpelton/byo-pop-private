#ifndef __WORDHARMONY_ENGINE_GEOMETRY_H__
#define __WORDHARMONY_ENGINE_GEOMETRY_H__

// The engine's points, rectangles and colours (games/wordharmony/docs/ENGINE.md). Rectangles are {left, top, right,
// bottom}; right and bottom are exclusive.

#include <cmath>

namespace engine
{

struct Point
{
	float x, y;

	Point() : x(0), y(0) {}  // 0x42b490
	Point(float theX, float theY) : x(theX), y(theY) {}  // 0x42b4c0, 0x42b580

	// 0x42b520
	void scale(float theFactor)
	{
		x = theFactor * x;
		y = theFactor * y;
	}
	// 0x42b540
	void add(const Point& p)
	{
		x = p.x + x;
		y = p.y + y;
	}
	// 0x42b560
	void subtract(const Point& p)
	{
		x = x - p.x;
		y = y - p.y;
	}
	// 0x42b580
	void set(float theX, float theY)
	{
		x = theX;
		y = theY;
	}
	// 0x42b590: turns the point about the origin (degrees; y down, so a positive angle turns it clockwise on the
	// screen)
	void rotate(float theDegrees)
	{
		double anAngle = theDegrees * 0.017453292519943295;
		double aCos = cos(anAngle);
		double aSin = sin(anAngle);
		float aY = (float)(x * aSin + y * aCos);
		x = (float)(x * aCos - y * aSin);
		y = aY;
	}
	// 0x42b5d0: to length 1; a zero point stays
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
};

struct IntRect
{
	int left, top, right, bottom;

	IntRect() : left(0), top(0), right(0), bottom(0) {}
	IntRect(int l, int t, int r, int b) : left(l), top(t), right(r), bottom(b) {}  // 0x43d3a0
	int width() const { return right - left; }            // 0x43d150
	int height() const { return bottom - top; }           // 0x43d160
	bool isEmpty() const { return !(left < right && top < bottom); }  // 0x43d170
};

struct Rect
{
	float left, top, right, bottom;

	Rect() : left(0), top(0), right(0), bottom(0) {}  // 0x45ec30, 0x43d210
	Rect(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b) {}
	// 0x43d1f0
	void set(float l, float t, float r, float b)
	{
		left = l;
		top = t;
		right = r;
		bottom = b;
	}
	float width() const { return right - left; }    // 0x43d1b0
	float height() const { return bottom - top; }   // 0x43d1c0
	bool isEmpty() const { return !(left < right && top < bottom); }  // 0x43d220
	// 0x43d250
	void scale(float sx, float sy)
	{
		left = sx * left;
		right = sx * right;
		top = sy * top;
		bottom = sy * bottom;
	}
	// 0x43d280: r lies inside
	bool contains(const Rect& r) const
	{
		return r.right <= right && r.bottom <= bottom && left <= r.left && top <= r.top;
	}
	// 0x43d2d0
	bool contains(float x, float y) const { return left <= x && x < right && top <= y && y < bottom; }
	// 0x43d320
	void offset(float dx, float dy)
	{
		left = dx + left;
		right = dx + right;
		top = dy + top;
		bottom = dy + bottom;
	}
	// 0x43d450: grows to include r
	void unite(const Rect& r)
	{
		left = left <= r.left ? left : r.left;
		right = r.right <= right ? right : r.right;
		top = top <= r.top ? top : r.top;
		bottom = bottom < r.bottom ? r.bottom : bottom;
	}
	// 0x43d4d0: left <= right and top <= bottom
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

// Three float components, 0..1 (0x43ed30; 0x43ed50 sets them)
struct Color
{
	float r, g, b;

	Color() : r(0), g(0), b(0) {}
	// 0x43ed30
	Color(float theR, float theG, float theB) : r(theR), g(theG), b(theB) {}
	// 0x43ed50
	void set(float theR, float theG, float theB)
	{
		r = theR;
		g = theG;
		b = theB;
	}
};

} // namespace engine

#endif
