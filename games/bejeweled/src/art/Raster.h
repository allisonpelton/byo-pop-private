#ifndef __ART_RASTER_H__
#define __ART_RASTER_H__

// port(variant): a small software rasteriser for the code-drawn art (features.codeArt,
// games/bejeweled/docs/CODE_MODS_DESIGN.md 4.18). It has no game dependencies.
//
// Drawing code works in logical pixels (the game's 640x480 frame); a Canvas or Mask holds them at an integer scale S, so
// the same code draws the 1x image and the HD one. Paths are flattened to polygons and rasterised with exact area
// coverage (signed-area accumulation, nonzero-like: overlapping contours of one orientation stay covered, reversed ones
// cut holes). Masks combine by max (union), product or difference; a Canvas composites paints through masks with
// premultiplied alpha. Generators use the fixed-seed noise here, never the game's random numbers.

#include <cstdint>
#include <functional>
#include <vector>

namespace Art
{

struct Vec2
{
	double x = 0, y = 0;
	Vec2() {}
	Vec2(double theX, double theY) : x(theX), y(theY) {}
	Vec2 operator+(const Vec2& o) const { return Vec2(x + o.x, y + o.y); }
	Vec2 operator-(const Vec2& o) const { return Vec2(x - o.x, y - o.y); }
	Vec2 operator*(double s) const { return Vec2(x * s, y * s); }
};

// Straight-alpha colour, 0..1
struct Rgba
{
	float r = 0, g = 0, b = 0, a = 1;
	Rgba() {}
	Rgba(float theR, float theG, float theB, float theA = 1) : r(theR), g(theG), b(theB), a(theA) {}
	static Rgba Bytes(int theR, int theG, int theB, int theA = 255) { return Rgba(theR / 255.0f, theG / 255.0f, theB / 255.0f, theA / 255.0f); }
	static Rgba Hsv(double theH, double theS, double theV, double theA = 1);
	Rgba WithAlpha(float theA) const { return Rgba(r, g, b, theA); }
};
Rgba Mix(const Rgba& a, const Rgba& b, double t);
inline Rgba Lighter(const Rgba& c, double t) { return Mix(c, Rgba(1, 1, 1, c.a), t); }
inline Rgba Darker(const Rgba& c, double t) { return Mix(c, Rgba(0, 0, 0, c.a), t); }

// A paint: the colour at a logical point
typedef std::function<Rgba(double theX, double theY)> Paint;
Paint Solid(const Rgba& theColor);
// Gradient stops: position 0..1, colour; positions ascending
struct Stop
{
	double mAt;
	Rgba mColor;
};
Rgba StopsAt(const std::vector<Stop>& theStops, double t);
Paint Linear(Vec2 theFrom, Vec2 theTo, const std::vector<Stop>& theStops);
Paint Radial(Vec2 theCentre, double theRadiusX, double theRadiusY, const std::vector<Stop>& theStops);
// A slow paint (noise, a spiral) sampled once every theStep logical px over theW x theH and interpolated between
Paint Sampled(const Paint& thePaint, int theW, int theH, double theStep);

class Path
{
public:
	std::vector<std::vector<Vec2> > mContours;

	Path& MoveTo(double x, double y);
	Path& LineTo(double x, double y);
	// An elliptic arc about (cx, cy) from angle a0 to a1 (degrees, counter-clockwise on screen: y grows down, so
	// 90 is up), continuing the current contour
	Path& ArcTo(double cx, double cy, double rx, double ry, double a0, double a1);
	Path& Close();

	Path& Rect(double x, double y, double w, double h);
	Path& RoundRect(double x, double y, double w, double h, double r);
	Path& Ellipse(double cx, double cy, double rx, double ry);
	Path& Polygon(const std::vector<Vec2>& thePoints);
	Path& Star(double cx, double cy, int thePoints, double theOuter, double theInner, double thePhase);
	// Appends a contour in reverse (a hole in the shapes before it)
	Path& Reversed(const Path& theOther);
	Path& Add(const Path& theOther);

	Path Transformed(double theScale, double theDX, double theDY) const;
	bool Empty() const;
};

// Coverage 0..1 per pixel at scale S of a logical size
class Mask
{
public:
	int mW = 0, mH = 0, mScale = 1;    // logical size, scale
	int mPW = 0, mPH = 0;              // pixels
	std::vector<float> mA;

	Mask() {}
	Mask(int theW, int theH, int theScale);
	float& At(int px, int py) { return mA[(size_t) py * mPW + px]; }
	float At(int px, int py) const { return mA[(size_t) py * mPW + px]; }

	Mask& Fill(const Path& thePath);                                          // union
	Mask& Stroke(const std::vector<Vec2>& theLine, double theWidth, bool isClosed);  // round joins and caps, union
	Mask& StrokePath(const Path& thePath, double theWidth);                   // every contour, open
	Mask& Intersect(const Mask& o);
	Mask& Subtract(const Mask& o);
	Mask& Invert();
	Mask& Blur(double theRadius);                                             // logical px, three box passes
	Mask& Shift(double theDX, double theDY);                                  // whole pixels at the scale
	Mask Grown(double theRadius) const;                                        // the shape grown (negative: shrunk), logical px
};

enum Blend
{
	BLEND_NORMAL,
	BLEND_ADD,     // colour added, alpha kept (max)
	BLEND_ERASE    // removes coverage
};

// Premultiplied RGBA float pixels at scale S of a logical size
class Canvas
{
public:
	int mW = 0, mH = 0, mScale = 1;
	int mPW = 0, mPH = 0;
	std::vector<float> mPx;   // 4 floats per pixel, premultiplied

	Canvas() {}
	Canvas(int theW, int theH, int theScale);
	Mask NewMask() const { return Mask(mW, mH, mScale); }

	Canvas& Fill(const Mask& theMask, const Paint& thePaint, float theOpacity = 1, Blend theBlend = BLEND_NORMAL);
	Canvas& Fill(const Mask& theMask, const Rgba& theColor, float theOpacity = 1, Blend theBlend = BLEND_NORMAL);
	Canvas& Fill(const Path& thePath, const Paint& thePaint, float theOpacity = 1, Blend theBlend = BLEND_NORMAL);
	Canvas& FillAll(const Paint& thePaint, float theOpacity = 1, Blend theBlend = BLEND_NORMAL);
	// Draws another canvas of the same scale at a logical offset (source-over)
	Canvas& Draw(const Canvas& theOther, int theX, int theY, float theOpacity = 1, Blend theBlend = BLEND_NORMAL);
	// A drop shadow / glow: the mask, blurred and shifted, in one colour
	Canvas& Shadow(const Mask& theMask, double theDX, double theDY, double theBlur, const Rgba& theColor, Blend theBlend = BLEND_NORMAL);

	// Straight ARGB (0xAARRGGBB) at the canvas's scale, and its box-filtered 1x copy
	void ToArgb(std::vector<uint32_t>& theOut) const;
	void ToArgb1x(std::vector<uint32_t>& theOut) const;
};

// Value noise in -1..1 (fixed seed), its fractal sum, and a seeded hash in 0..1
double Noise(double x, double y, uint32_t theSeed);
double Fbm(double x, double y, uint32_t theSeed, int theOctaves, double theGain = 0.5);
double Hash01(uint32_t a, uint32_t b, uint32_t theSeed);

// Box filter of premultiplied ARGB down by theScale (straight ARGB in and out)
void DownsampleArgb(const std::vector<uint32_t>& theSrc, int theW, int theH, int theScale, std::vector<uint32_t>& theOut);
uint32_t PackArgb(float r, float g, float b, float a);   // straight 0..1

}

#endif
