#include "Raster.h"

#include <algorithm>
#include <cmath>
#include <memory>

// port(variant): code-drawn art (Raster.h)

namespace Art
{

static const double kPi = 3.14159265358979323846;

static inline float Clamp01(float v)
{
	return (v < 0) ? 0 : ((v > 1) ? 1 : v);
}

// ---------------------------------------------------------------------------------------------------
// Colours and paints

Rgba Rgba::Hsv(double theH, double theS, double theV, double theA)
{
	double h = (theH - std::floor(theH)) * 6.0;
	int i = (int) h;
	double f = h - i;
	double p = theV * (1 - theS), q = theV * (1 - theS * f), t = theV * (1 - theS * (1 - f));
	double r, g, b;
	switch (i % 6)
	{
	case 0: r = theV; g = t; b = p; break;
	case 1: r = q; g = theV; b = p; break;
	case 2: r = p; g = theV; b = t; break;
	case 3: r = p; g = q; b = theV; break;
	case 4: r = t; g = p; b = theV; break;
	default: r = theV; g = p; b = q; break;
	}
	return Rgba((float) r, (float) g, (float) b, (float) theA);
}

Rgba Mix(const Rgba& a, const Rgba& b, double t)
{
	float u = (float) t;
	return Rgba(a.r + (b.r - a.r) * u, a.g + (b.g - a.g) * u, a.b + (b.b - a.b) * u, a.a + (b.a - a.a) * u);
}

Paint Solid(const Rgba& theColor)
{
	return [theColor](double, double) { return theColor; };
}

Rgba StopsAt(const std::vector<Stop>& theStops, double thePosition)
{
	if (theStops.empty())
		return Rgba();
	if (thePosition <= theStops.front().mAt)
		return theStops.front().mColor;
	for (size_t i = 1; i < theStops.size(); i++)
	{
		if (thePosition <= theStops[i].mAt)
		{
			double aSpan = theStops[i].mAt - theStops[i - 1].mAt;
			return Mix(theStops[i - 1].mColor, theStops[i].mColor, (aSpan > 0) ? (thePosition - theStops[i - 1].mAt) / aSpan : 1.0);
		}
	}
	return theStops.back().mColor;
}

Paint Linear(Vec2 theFrom, Vec2 theTo, const std::vector<Stop>& theStops)
{
	Vec2 d = theTo - theFrom;
	double aLen2 = std::max(1e-9, d.x * d.x + d.y * d.y);
	return [theFrom, d, aLen2, theStops](double x, double y)
	{
		return StopsAt(theStops, ((x - theFrom.x) * d.x + (y - theFrom.y) * d.y) / aLen2);
	};
}

Paint Radial(Vec2 theCentre, double theRadiusX, double theRadiusY, const std::vector<Stop>& theStops)
{
	return [theCentre, theRadiusX, theRadiusY, theStops](double x, double y)
	{
		double dx = (x - theCentre.x) / theRadiusX, dy = (y - theCentre.y) / theRadiusY;
		return StopsAt(theStops, std::sqrt(dx * dx + dy * dy));
	};
}

// ---------------------------------------------------------------------------------------------------
// Paths

Path& Path::MoveTo(double x, double y)
{
	mContours.push_back(std::vector<Vec2>(1, Vec2(x, y)));
	return *this;
}

Path& Path::LineTo(double x, double y)
{
	if (mContours.empty())
		return MoveTo(x, y);
	mContours.back().push_back(Vec2(x, y));
	return *this;
}

Path& Path::QuadTo(double cx, double cy, double x, double y)
{
	Vec2 p0 = mContours.empty() ? Vec2(x, y) : mContours.back().back();
	int n = std::max(4, (int) (std::hypot(cx - p0.x, cy - p0.y) + std::hypot(x - cx, y - cy)) / 2);
	for (int i = 1; i <= n; i++)
	{
		double t = i / (double) n, u = 1 - t;
		LineTo(u * u * p0.x + 2 * u * t * cx + t * t * x, u * u * p0.y + 2 * u * t * cy + t * t * y);
	}
	return *this;
}

Path& Path::CubicTo(double c1x, double c1y, double c2x, double c2y, double x, double y)
{
	Vec2 p0 = mContours.empty() ? Vec2(x, y) : mContours.back().back();
	int n = std::max(6, (int) (std::hypot(c1x - p0.x, c1y - p0.y) + std::hypot(c2x - c1x, c2y - c1y) + std::hypot(x - c2x, y - c2y)) / 2);
	for (int i = 1; i <= n; i++)
	{
		double t = i / (double) n, u = 1 - t;
		double a = u * u * u, b = 3 * u * u * t, c = 3 * u * t * t, d = t * t * t;
		LineTo(a * p0.x + b * c1x + c * c2x + d * x, a * p0.y + b * c1y + c * c2y + d * y);
	}
	return *this;
}

Path& Path::ArcTo(double cx, double cy, double rx, double ry, double a0, double a1)
{
	double aSpan = std::fabs(a1 - a0) * kPi / 180.0;
	// a point every quarter logical pixel along the arc: one pixel apart at 4x
	int n = std::max(8, (int) std::ceil(aSpan * std::max(rx, ry) / 0.25));
	n = std::min(n, 1440);
	for (int i = 0; i <= n; i++)
	{
		double a = (a0 + (a1 - a0) * i / n) * kPi / 180.0;
		double x = cx + rx * std::cos(a), y = cy - ry * std::sin(a);
		if ((i == 0) && !mContours.empty() && !mContours.back().empty())
		{
			const Vec2& p = mContours.back().back();
			if (std::hypot(p.x - x, p.y - y) < 1e-6)
				continue;
		}
		if (i == 0 && mContours.empty())
			MoveTo(x, y);
		else
			LineTo(x, y);
	}
	return *this;
}

Path& Path::Close()
{
	if (!mContours.empty() && (mContours.back().size() > 1))
		mContours.back().push_back(mContours.back().front());
	return *this;
}

Path& Path::Rect(double x, double y, double w, double h)
{
	MoveTo(x, y);
	LineTo(x + w, y);
	LineTo(x + w, y + h);
	LineTo(x, y + h);
	return Close();
}

Path& Path::RoundRect(double x, double y, double w, double h, double r)
{
	r = std::max(0.0, std::min(r, std::min(w, h) / 2));
	if (r <= 0)
		return Rect(x, y, w, h);
	mContours.push_back(std::vector<Vec2>());
	ArcTo(x + w - r, y + r, r, r, 90, 0);
	ArcTo(x + w - r, y + h - r, r, r, 0, -90);
	ArcTo(x + r, y + h - r, r, r, -90, -180);
	ArcTo(x + r, y + r, r, r, 180, 90);
	return Close();
}

Path& Path::Ellipse(double cx, double cy, double rx, double ry)
{
	mContours.push_back(std::vector<Vec2>());
	ArcTo(cx, cy, rx, ry, 0, 360);
	return Close();
}

Path& Path::Polygon(const std::vector<Vec2>& thePoints)
{
	if (thePoints.empty())
		return *this;
	mContours.push_back(thePoints);
	return Close();
}

Path& Path::Star(double cx, double cy, int thePoints, double theOuter, double theInner, double thePhase)
{
	std::vector<Vec2> aPoints;
	for (int i = 0; i < thePoints * 2; i++)
	{
		double a = (thePhase + 180.0 * i / thePoints) * kPi / 180.0;
		double r = (i % 2) ? theInner : theOuter;
		aPoints.push_back(Vec2(cx + r * std::cos(a), cy - r * std::sin(a)));
	}
	return Polygon(aPoints);
}

Path& Path::Reversed(const Path& theOther)
{
	for (size_t i = 0; i < theOther.mContours.size(); i++)
		mContours.push_back(std::vector<Vec2>(theOther.mContours[i].rbegin(), theOther.mContours[i].rend()));
	return *this;
}

Path& Path::Add(const Path& theOther)
{
	mContours.insert(mContours.end(), theOther.mContours.begin(), theOther.mContours.end());
	return *this;
}

Path Path::Transformed(double theScale, double theDX, double theDY) const
{
	Path aPath = *this;
	for (size_t i = 0; i < aPath.mContours.size(); i++)
	{
		for (size_t j = 0; j < aPath.mContours[i].size(); j++)
		{
			Vec2& p = aPath.mContours[i][j];
			p = Vec2(p.x * theScale + theDX, p.y * theScale + theDY);
		}
	}
	return aPath;
}

bool Path::Empty() const
{
	for (size_t i = 0; i < mContours.size(); i++)
	{
		if (mContours[i].size() > 1)
			return false;
	}
	return true;
}

// ---------------------------------------------------------------------------------------------------
// Coverage: signed-area accumulation of the path's edges over a pixel window, then a running sum per row

namespace
{

struct Accumulator
{
	int mX0, mY0, mW, mH;
	std::vector<float> mA;

	Accumulator(int x0, int y0, int w, int h) : mX0(x0), mY0(y0), mW(w), mH(h), mA((size_t) w * h + 2, 0.0f) {}

	void Line(Vec2 p0, Vec2 p1)
	{
		p0.x -= mX0; p1.x -= mX0;
		p0.y -= mY0; p1.y -= mY0;
		if (std::fabs(p0.y - p1.y) <= 1e-9)
			return;
		double aDir = 1.0;
		if (p0.y > p1.y)
		{
			std::swap(p0, p1);
			aDir = -1.0;
		}
		// x outside the window: an edge along the window's side covers what it would have
		p0.x = std::max(0.0, std::min((double) mW, p0.x));
		p1.x = std::max(0.0, std::min((double) mW, p1.x));
		double dxdy = (p1.x - p0.x) / (p1.y - p0.y);
		double x = p0.x;
		if (p0.y < 0)
			x -= p0.y * dxdy;
		int y0 = std::max(0, (int) std::floor(p0.y));
		int y1 = std::min(mH, (int) std::ceil(p1.y));
		for (int y = y0; y < y1; y++)
		{
			size_t aLine = (size_t) y * mW;
			double dy = std::min((double) (y + 1), p1.y) - std::max((double) y, p0.y);
			double xnext = x + dxdy * dy;
			double aDeltaY = dy * aDir;
			double x0 = std::min(x, xnext), x1 = std::max(x, xnext);
			double x0floor = std::floor(x0);
			int x0i = (int) x0floor;
			double x1ceil = std::ceil(x1);
			int x1i = (int) x1ceil;
			if (x1i <= x0i + 1)
			{
				double xmf = 0.5 * (x + xnext) - x0floor;
				mA[aLine + x0i] += (float) (aDeltaY - aDeltaY * xmf);
				mA[aLine + x0i + 1] += (float) (aDeltaY * xmf);
			}
			else
			{
				double anInverseWidth = 1.0 / (x1 - x0);
				double x0f = x0 - x0floor;
				double a0 = 0.5 * anInverseWidth * (1.0 - x0f) * (1.0 - x0f);
				double x1f = x1 - x1ceil + 1.0;
				double am = 0.5 * anInverseWidth * x1f * x1f;
				mA[aLine + x0i] += (float) (aDeltaY * a0);
				if (x1i == x0i + 2)
					mA[aLine + x0i + 1] += (float) (aDeltaY * (1.0 - a0 - am));
				else
				{
					double a1 = anInverseWidth * (1.5 - x0f);
					mA[aLine + x0i + 1] += (float) (aDeltaY * (a1 - a0));
					for (int xi = x0i + 2; xi < x1i - 1; xi++)
						mA[aLine + xi] += (float) (aDeltaY * anInverseWidth);
					double a2 = a1 + (x1i - x0i - 3) * anInverseWidth;
					mA[aLine + x1i - 1] += (float) (aDeltaY * (1.0 - a2 - am));
				}
				mA[aLine + x1i] += (float) (aDeltaY * am);
			}
			x = xnext;
		}
	}
};

// The pixel window of a path (scaled), clipped to the mask; false when empty
bool PathWindow(const Path& thePath, int theW, int theH, int* x0, int* y0, int* x1, int* y1)
{
	double minX = 1e30, minY = 1e30, maxX = -1e30, maxY = -1e30;
	for (size_t i = 0; i < thePath.mContours.size(); i++)
	{
		for (size_t j = 0; j < thePath.mContours[i].size(); j++)
		{
			const Vec2& p = thePath.mContours[i][j];
			minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
			minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
		}
	}
	*x0 = std::max(0, (int) std::floor(minX) - 1);
	*y0 = std::max(0, (int) std::floor(minY) - 1);
	*x1 = std::min(theW, (int) std::ceil(maxX) + 1);
	*y1 = std::min(theH, (int) std::ceil(maxY) + 1);
	return (*x0 < *x1) && (*y0 < *y1);
}

// Rasterises a path in pixel coordinates and merges it into the mask by max
void MergePath(Mask& theMask, const Path& thePixelPath)
{
	int x0, y0, x1, y1;
	if (!PathWindow(thePixelPath, theMask.mPW, theMask.mPH, &x0, &y0, &x1, &y1))
		return;
	Accumulator anAcc(x0, y0, x1 - x0, y1 - y0);
	for (size_t i = 0; i < thePixelPath.mContours.size(); i++)
	{
		const std::vector<Vec2>& c = thePixelPath.mContours[i];
		if (c.size() < 2)
			continue;
		for (size_t j = 0; j + 1 < c.size(); j++)
			anAcc.Line(c[j], c[j + 1]);
		// an unclosed contour closes implicitly
		if ((c.front().x != c.back().x) || (c.front().y != c.back().y))
			anAcc.Line(c.back(), c.front());
	}
	// One running sum over all rows: what an edge at the window's right side puts past a row's end lands on the next
	// row's first cell, where it cancels the sum the row left over
	double aSum = 0;
	for (int y = 0; y < anAcc.mH; y++)
	{
		for (int x = 0; x < anAcc.mW; x++)
		{
			aSum += anAcc.mA[(size_t) y * anAcc.mW + x];
			float aCov = (float) std::min(1.0, std::fabs(aSum));
			// the running sum's rounding leaves crumbs: they are no coverage (Distance would take them for edges)
			if (aCov < 1e-4f)
				aCov = 0;
			else if (aCov > 1 - 1e-4f)
				aCov = 1;
			float& m = theMask.At(x0 + x, y0 + y);
			m = std::max(m, aCov);
		}
	}
}

}

Mask::Mask(int theW, int theH, int theScale) : mW(theW), mH(theH), mScale(theScale), mPW(theW * theScale), mPH(theH * theScale),
	mA((size_t) theW * theScale * theH * theScale, 0.0f)
{
}

Mask& Mask::Fill(const Path& thePath)
{
	MergePath(*this, thePath.Transformed(mScale, 0, 0));
	return *this;
}

Mask& Mask::Stroke(const std::vector<Vec2>& theLine, double theWidth, bool isClosed)
{
	double aRadius = theWidth / 2;
	size_t aCount = theLine.size();
	if (aCount == 0)
		return *this;
	for (size_t i = 0; i < aCount; i++)
	{
		const Vec2& p = theLine[i];
		Fill(Path().Ellipse(p.x, p.y, aRadius, aRadius));
	}
	size_t aSegments = isClosed ? aCount : aCount - 1;
	for (size_t i = 0; i < aSegments; i++)
	{
		const Vec2& a = theLine[i];
		const Vec2& b = theLine[(i + 1) % aCount];
		double dx = b.x - a.x, dy = b.y - a.y;
		double aLen = std::hypot(dx, dy);
		if (aLen < 1e-9)
			continue;
		double nx = -dy / aLen * aRadius, ny = dx / aLen * aRadius;
		Fill(Path().Polygon({Vec2(a.x + nx, a.y + ny), Vec2(b.x + nx, b.y + ny), Vec2(b.x - nx, b.y - ny), Vec2(a.x - nx, a.y - ny)}));
	}
	return *this;
}

Mask& Mask::StrokePath(const Path& thePath, double theWidth)
{
	for (size_t i = 0; i < thePath.mContours.size(); i++)
		Stroke(thePath.mContours[i], theWidth, false);
	return *this;
}

Mask& Mask::Union(const Mask& o)
{
	for (size_t i = 0; i < mA.size(); i++)
		mA[i] = std::max(mA[i], o.mA[i]);
	return *this;
}

Mask& Mask::Intersect(const Mask& o)
{
	for (size_t i = 0; i < mA.size(); i++)
		mA[i] *= o.mA[i];
	return *this;
}

Mask& Mask::Subtract(const Mask& o)
{
	for (size_t i = 0; i < mA.size(); i++)
		mA[i] *= 1.0f - o.mA[i];
	return *this;
}

Mask& Mask::Invert()
{
	for (size_t i = 0; i < mA.size(); i++)
		mA[i] = 1.0f - mA[i];
	return *this;
}

Mask& Mask::Scale(float theFactor)
{
	for (size_t i = 0; i < mA.size(); i++)
		mA[i] = Clamp01(mA[i] * theFactor);
	return *this;
}

static void BoxPass(std::vector<float>& theData, int theW, int theH, int theRadius, bool isHorizontal)
{
	if (theRadius <= 0)
		return;
	float aNorm = 1.0f / (2 * theRadius + 1);
	if (isHorizontal)
	{
		std::vector<float> aLine(theW);
		for (int l = 0; l < theH; l++)
		{
			float* aRow = &theData[(size_t) l * theW];
			std::copy(aRow, aRow + theW, aLine.begin());
			// edges extend with zeros
			double aSum = 0;
			for (int i = 0; i <= theRadius && i < theW; i++)
				aSum += aLine[i];
			for (int i = 0; i < theW; i++)
			{
				aRow[i] = (float) (aSum * aNorm);
				int anOut = i - theRadius, anIn = i + theRadius + 1;
				if (anOut >= 0)
					aSum -= aLine[anOut];
				if (anIn < theW)
					aSum += aLine[anIn];
			}
		}
		return;
	}
	// down the columns, all of them at once, so the rows are read in order
	std::vector<float> aSource = theData;
	std::vector<double> aSum(theW, 0.0);
	for (int i = 0; i <= theRadius && i < theH; i++)
	{
		const float* aRow = &aSource[(size_t) i * theW];
		for (int x = 0; x < theW; x++)
			aSum[x] += aRow[x];
	}
	for (int y = 0; y < theH; y++)
	{
		float* anOutRow = &theData[(size_t) y * theW];
		for (int x = 0; x < theW; x++)
			anOutRow[x] = (float) (aSum[x] * aNorm);
		int anOut = y - theRadius, anIn = y + theRadius + 1;
		if (anOut >= 0)
		{
			const float* aRow = &aSource[(size_t) anOut * theW];
			for (int x = 0; x < theW; x++)
				aSum[x] -= aRow[x];
		}
		if (anIn < theH)
		{
			const float* aRow = &aSource[(size_t) anIn * theW];
			for (int x = 0; x < theW; x++)
				aSum[x] += aRow[x];
		}
	}
}

Mask& Mask::Blur(double theRadius)
{
	int r = (int) std::lround(theRadius * mScale / 1.7);
	if (r <= 0)
		return *this;
	for (int i = 0; i < 3; i++)
	{
		BoxPass(mA, mPW, mPH, r, true);
		BoxPass(mA, mPW, mPH, r, false);
	}
	return *this;
}

Mask& Mask::Shift(double theDX, double theDY)
{
	int dx = (int) std::lround(theDX * mScale), dy = (int) std::lround(theDY * mScale);
	std::vector<float> aNew(mA.size(), 0.0f);
	for (int y = 0; y < mPH; y++)
	{
		int sy = y - dy;
		if (sy < 0 || sy >= mPH)
			continue;
		for (int x = 0; x < mPW; x++)
		{
			int sx = x - dx;
			if (sx >= 0 && sx < mPW)
				aNew[(size_t) y * mPW + x] = mA[(size_t) sy * mPW + sx];
		}
	}
	mA.swap(aNew);
	return *this;
}

// Squared Euclidean distance transform of one line (Felzenszwalb and Huttenlocher)
static void Edt1D(const double* theValues, int theCount, double* theDistances, int* theLocations, double* z)
{
	int k = 0;
	theLocations[0] = 0;
	z[0] = -1e30;
	z[1] = 1e30;
	for (int q = 1; q < theCount; q++)
	{
		double s = ((theValues[q] + (double) q * q) - (theValues[theLocations[k]] + (double) theLocations[k] * theLocations[k])) / (2.0 * q - 2.0 * theLocations[k]);
		while (s <= z[k])
		{
			k--;
			s = ((theValues[q] + (double) q * q) - (theValues[theLocations[k]] + (double) theLocations[k] * theLocations[k])) / (2.0 * q - 2.0 * theLocations[k]);
		}
		k++;
		theLocations[k] = q;
		z[k] = s;
		z[k + 1] = 1e30;
	}
	k = 0;
	for (int q = 0; q < theCount; q++)
	{
		while (z[k + 1] < q)
			k++;
		theDistances[q] = (double) (q - theLocations[k]) * (q - theLocations[k]) + theValues[theLocations[k]];
	}
}

// Squared distance (pixels) from each pixel to the nearest pixel where theTarget is set. First each pixel's distance
// to the nearest target in its column (two sweeps over the rows), then the exact transform along each row.
static std::vector<double> Edt(const std::vector<char>& theTarget, int theWidth, int theHeight)
{
	const double kInf = 1e20;
	const float kFar = (float) (theWidth + theHeight);
	std::vector<float> aColumn((size_t) theWidth * theHeight);
	for (int x = 0; x < theWidth; x++)
		aColumn[x] = theTarget[x] ? 0.0f : kFar;
	for (int y = 1; y < theHeight; y++)
	{
		const char* t = &theTarget[(size_t) y * theWidth];
		const float* aAbove = &aColumn[(size_t) (y - 1) * theWidth];
		float* g = &aColumn[(size_t) y * theWidth];
		for (int x = 0; x < theWidth; x++)
			g[x] = t[x] ? 0.0f : aAbove[x] + 1;
	}
	for (int y = theHeight - 2; y >= 0; y--)
	{
		const float* aBelow = &aColumn[(size_t) (y + 1) * theWidth];
		float* g = &aColumn[(size_t) y * theWidth];
		for (int x = 0; x < theWidth; x++)
			g[x] = std::min(g[x], aBelow[x] + 1);
	}
	std::vector<double> aGrid((size_t) theWidth * theHeight);
	std::vector<double> f(theWidth), d(theWidth), z(theWidth + 1);
	std::vector<int> v(theWidth);
	for (int y = 0; y < theHeight; y++)
	{
		const float* g = &aColumn[(size_t) y * theWidth];
		for (int x = 0; x < theWidth; x++)
			f[x] = (g[x] >= kFar) ? kInf : (double) g[x] * g[x];
		Edt1D(&f[0], theWidth, &d[0], &v[0], &z[0]);
		std::copy(d.begin(), d.end(), aGrid.begin() + (size_t) y * theWidth);
	}
	return aGrid;
}

// Coverage this close to 0 or 1 is not an edge (a blur's far tail, rounding)
static const float kEdge = 1e-3f;

std::vector<float> Mask::Distance() const
{
	size_t n = mA.size();
	std::vector<char> anIn(n), anOut(n);
	for (size_t i = 0; i < n; i++)
	{
		anIn[i] = mA[i] >= 0.5f;
		anOut[i] = !anIn[i];
	}
	std::vector<double> aToIn = Edt(anIn, mPW, mPH);
	std::vector<double> aToOut = Edt(anOut, mPW, mPH);
	std::vector<float> aDist(n);
	for (size_t i = 0; i < n; i++)
	{
		double d;
		if ((mA[i] > kEdge) && (mA[i] < 1.0f - kEdge))
			d = 0.5 - mA[i];                                   // an edge pixel: its coverage places the edge
		else if (anIn[i])
			d = -(std::sqrt(aToOut[i]) - 0.5);
		else
			d = std::sqrt(aToIn[i]) - 0.5;
		aDist[i] = (float) (d / mScale);
	}
	return aDist;
}

Mask Mask::Grown(double theRadius) const
{
	Mask aMask(mW, mH, mScale);
	size_t n = mA.size();
	bool isGrowing = theRadius >= 0;
	std::vector<char> aTarget(n);
	for (size_t i = 0; i < n; i++)
		aTarget[i] = isGrowing ? (mA[i] >= 0.5f) : (mA[i] < 0.5f);
	std::vector<double> aSquared = Edt(aTarget, mPW, mPH);
	float aScale = (float) mScale;
	for (size_t i = 0; i < n; i++)
	{
		// the signed distance (Distance) where this side needs it; the other side is wholly in or out
		double d;
		if ((mA[i] > kEdge) && (mA[i] < 1.0f - kEdge))
			d = 0.5 - mA[i];
		else if (isGrowing)
			d = (mA[i] >= 0.5f) ? -1e9 : std::sqrt(aSquared[i]) - 0.5;
		else
			d = (mA[i] < 0.5f) ? 1e9 : -(std::sqrt(aSquared[i]) - 0.5);
		aMask.mA[i] = Clamp01((float) (theRadius * aScale - d) + 0.5f);
	}
	return aMask;
}

Mask Mask::Inner(double theWidth) const
{
	Mask aMask(mW, mH, mScale);
	std::vector<float> aDist = Distance();
	for (size_t i = 0; i < aDist.size(); i++)
		aMask.mA[i] = Clamp01((float) (-aDist[i] / theWidth)) * mA[i];
	return aMask;
}

Paint Sampled(const Paint& thePaint, int theW, int theH, double theStep)
{
	int aCols = (int) std::ceil(theW / theStep) + 2, aRows = (int) std::ceil(theH / theStep) + 2;
	auto aGrid = std::make_shared<std::vector<Rgba> >((size_t) aCols * aRows);
	for (int j = 0; j < aRows; j++)
	{
		for (int i = 0; i < aCols; i++)
			(*aGrid)[(size_t) j * aCols + i] = thePaint(i * theStep, j * theStep);
	}
	return [aGrid, aCols, aRows, theStep](double x, double y) {
		double gx = std::max(0.0, std::min(aCols - 1.001, x / theStep)), gy = std::max(0.0, std::min(aRows - 1.001, y / theStep));
		int i = (int) gx, j = (int) gy;
		float fx = (float) (gx - i), fy = (float) (gy - j);
		const Rgba* g = &(*aGrid)[(size_t) j * aCols + i];
		Rgba aTop = Mix(g[0], g[1], fx), aBottom = Mix(g[aCols], g[aCols + 1], fx);
		return Mix(aTop, aBottom, fy);
	};
}

// ---------------------------------------------------------------------------------------------------
// Canvas

Canvas::Canvas(int theW, int theH, int theScale) : mW(theW), mH(theH), mScale(theScale), mPW(theW * theScale), mPH(theH * theScale),
	mPx((size_t) theW * theScale * theH * theScale * 4, 0.0f)
{
}

static inline void Composite(float* thePixel, const Rgba& theColor, float k, Blend theBlend)
{
	float anAlpha = theColor.a * k;
	if (anAlpha <= 0)
		return;
	if (theBlend == BLEND_NORMAL)
	{
		float inv = 1 - anAlpha;
		thePixel[0] = theColor.r * anAlpha + thePixel[0] * inv;
		thePixel[1] = theColor.g * anAlpha + thePixel[1] * inv;
		thePixel[2] = theColor.b * anAlpha + thePixel[2] * inv;
		thePixel[3] = anAlpha + thePixel[3] * inv;
	}
	else if (theBlend == BLEND_ADD)
	{
		thePixel[0] = std::min(1.0f, thePixel[0] + theColor.r * anAlpha);
		thePixel[1] = std::min(1.0f, thePixel[1] + theColor.g * anAlpha);
		thePixel[2] = std::min(1.0f, thePixel[2] + theColor.b * anAlpha);
		thePixel[3] = std::max(thePixel[3], std::max(thePixel[0], std::max(thePixel[1], thePixel[2])));
	}
	else
	{
		float inv = 1 - anAlpha;
		thePixel[0] *= inv; thePixel[1] *= inv; thePixel[2] *= inv; thePixel[3] *= inv;
	}
}

Canvas& Canvas::Fill(const Mask& theMask, const Paint& thePaint, float theOpacity, Blend theBlend)
{
	double inv = 1.0 / mScale;
	for (int y = 0; y < mPH; y++)
	{
		const float* m = &theMask.mA[(size_t) y * mPW];
		float* p = &mPx[(size_t) y * mPW * 4];
		for (int x = 0; x < mPW; x++, p += 4)
		{
			float k = m[x] * theOpacity;
			if (k <= 0)
				continue;
			Composite(p, thePaint((x + 0.5) * inv, (y + 0.5) * inv), k, theBlend);
		}
	}
	return *this;
}

Canvas& Canvas::Fill(const Mask& theMask, const Rgba& theColor, float theOpacity, Blend theBlend)
{
	for (size_t i = 0, n = (size_t) mPW * mPH; i < n; i++)
	{
		float k = theMask.mA[i] * theOpacity;
		if (k > 0)
			Composite(&mPx[i * 4], theColor, k, theBlend);
	}
	return *this;
}

Canvas& Canvas::Fill(const Path& thePath, const Paint& thePaint, float theOpacity, Blend theBlend)
{
	return Fill(NewMask().Fill(thePath), thePaint, theOpacity, theBlend);
}

Canvas& Canvas::FillAll(const Paint& thePaint, float theOpacity, Blend theBlend)
{
	double inv = 1.0 / mScale;
	for (int y = 0; y < mPH; y++)
	{
		float* p = &mPx[(size_t) y * mPW * 4];
		for (int x = 0; x < mPW; x++, p += 4)
			Composite(p, thePaint((x + 0.5) * inv, (y + 0.5) * inv), theOpacity, theBlend);
	}
	return *this;
}

Canvas& Canvas::Draw(const Canvas& theOther, int theX, int theY, float theOpacity, Blend theBlend)
{
	int ox = theX * mScale, oy = theY * mScale;
	for (int y = 0; y < theOther.mPH; y++)
	{
		int ty = oy + y;
		if (ty < 0 || ty >= mPH)
			continue;
		for (int x = 0; x < theOther.mPW; x++)
		{
			int tx = ox + x;
			if (tx < 0 || tx >= mPW)
				continue;
			const float* s = &theOther.mPx[((size_t) y * theOther.mPW + x) * 4];
			if (s[3] <= 0)
				continue;
			Rgba c(s[0] / s[3], s[1] / s[3], s[2] / s[3], s[3]);
			Composite(&mPx[((size_t) ty * mPW + tx) * 4], c, theOpacity, theBlend);
		}
	}
	return *this;
}

Canvas& Canvas::Shadow(const Mask& theMask, double theDX, double theDY, double theBlur, const Rgba& theColor, Blend theBlend)
{
	Mask aMask = theMask;
	aMask.Blur(theBlur).Shift(theDX, theDY);
	return Fill(aMask, theColor, 1, theBlend);
}

Mask Canvas::AlphaMask() const
{
	Mask aMask = NewMask();
	for (size_t i = 0; i < aMask.mA.size(); i++)
		aMask.mA[i] = mPx[i * 4 + 3];
	return aMask;
}

void Canvas::MultiplyAlpha(const Mask& theMask)
{
	for (size_t i = 0; i < theMask.mA.size(); i++)
	{
		float k = theMask.mA[i];
		for (int c = 0; c < 4; c++)
			mPx[i * 4 + c] *= k;
	}
}

uint32_t PackArgb(float r, float g, float b, float a)
{
	auto aByte = [](float v) { return (uint32_t) std::lround(Clamp01(v) * 255.0f); };
	return (aByte(a) << 24) | (aByte(r) << 16) | (aByte(g) << 8) | aByte(b);
}

void Canvas::ToArgb(std::vector<uint32_t>& theOut) const
{
	theOut.resize((size_t) mPW * mPH);
	for (size_t i = 0; i < theOut.size(); i++)
	{
		const float* p = &mPx[i * 4];
		float a = p[3];
		theOut[i] = (a > 0) ? PackArgb(p[0] / a, p[1] / a, p[2] / a, a) : 0;
	}
}

void Canvas::ToArgb1x(std::vector<uint32_t>& theOut) const
{
	theOut.resize((size_t) mW * mH);
	int s = mScale;
	float aNorm = 1.0f / (s * s);
	for (int y = 0; y < mH; y++)
	{
		for (int x = 0; x < mW; x++)
		{
			float acc[4] = {0, 0, 0, 0};
			for (int sy = 0; sy < s; sy++)
			{
				const float* p = &mPx[(((size_t) (y * s + sy)) * mPW + x * s) * 4];
				for (int sx = 0; sx < s; sx++, p += 4)
				{
					acc[0] += p[0]; acc[1] += p[1]; acc[2] += p[2]; acc[3] += p[3];
				}
			}
			float a = acc[3] * aNorm;
			theOut[(size_t) y * mW + x] = (a > 0) ? PackArgb(acc[0] / acc[3], acc[1] / acc[3], acc[2] / acc[3], a) : 0;
		}
	}
}

void DownsampleArgb(const std::vector<uint32_t>& theSrc, int theW, int theH, int theScale, std::vector<uint32_t>& theOut)
{
	int aWidth = theW / theScale, h = theH / theScale;
	theOut.assign((size_t) aWidth * h, 0);
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < aWidth; x++)
		{
			double acc[4] = {0, 0, 0, 0};
			for (int sy = 0; sy < theScale; sy++)
			{
				for (int sx = 0; sx < theScale; sx++)
				{
					uint32_t p = theSrc[(size_t) (y * theScale + sy) * theW + x * theScale + sx];
					double a = (p >> 24) / 255.0;
					acc[0] += ((p >> 16) & 0xFF) * a;
					acc[1] += ((p >> 8) & 0xFF) * a;
					acc[2] += (p & 0xFF) * a;
					acc[3] += a;
				}
			}
			if (acc[3] > 0)
				theOut[(size_t) y * aWidth + x] = PackArgb((float) (acc[0] / acc[3] / 255.0), (float) (acc[1] / acc[3] / 255.0),
					(float) (acc[2] / acc[3] / 255.0), (float) (acc[3] / (theScale * theScale)));
		}
	}
}

// ---------------------------------------------------------------------------------------------------
// Noise

static inline uint32_t HashU(uint32_t a, uint32_t b, uint32_t theSeed)
{
	uint32_t h = a * 0x8DA6B343u ^ b * 0xD8163841u ^ theSeed * 0xCB1AB31Fu;
	h ^= h >> 15;
	h *= 0x2C1B3C6Du;
	h ^= h >> 12;
	h *= 0x297A2D39u;
	h ^= h >> 15;
	return h;
}

double Hash01(uint32_t a, uint32_t b, uint32_t theSeed)
{
	return HashU(a, b, theSeed) / 4294967295.0;
}

double Noise(double x, double y, uint32_t theSeed)
{
	double fx = std::floor(x), fy = std::floor(y);
	int ix = (int) fx, iy = (int) fy;
	double tx = x - fx, ty = y - fy;
	tx = tx * tx * (3 - 2 * tx);
	ty = ty * ty * (3 - 2 * ty);
	auto v = [&](int i, int j) { return Hash01((uint32_t) (ix + i), (uint32_t) (iy + j), theSeed) * 2 - 1; };
	double a = v(0, 0) + (v(1, 0) - v(0, 0)) * tx;
	double b = v(0, 1) + (v(1, 1) - v(0, 1)) * tx;
	return a + (b - a) * ty;
}

double Fbm(double x, double y, uint32_t theSeed, int theOctaves, double theGain)
{
	double aSum = 0, anAmp = 1, aNorm = 0;
	for (int i = 0; i < theOctaves; i++)
	{
		aSum += anAmp * Noise(x, y, theSeed + i * 101);
		aNorm += anAmp;
		anAmp *= theGain;
		x *= 2.03;
		y *= 2.03;
	}
	return aSum / aNorm;
}

}
