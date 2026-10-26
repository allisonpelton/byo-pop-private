#include "GemArt.h"

#include <algorithm>
#include <cmath>
#ifndef __EMSCRIPTEN__
#include <atomic>
#include <thread>
#endif

// port(variant): code-drawn gems (GemArt.h). The geometry and the ray caster follow games/bejeweled/tools/gemsheet.py,
// the su models and their shading tools/make_su_gems_mod.py; model units are the 84-unit cell of those scripts.

namespace Art
{

namespace
{

const double kPi = 3.14159265358979323846;
const int CELL = 52;
const int FRAMES = 15;
const double UNIT_PX = 0.6;                 // 1x pixels per model unit
const double ANCHOR_X = 26.75, ANCHOR_Y = 24.75;
const double EYE = 650.0;
const double SPIN_STEP = 12.0, GLOW_STEP = 24.0;
const double OUTLINE = 1.6;                 // model units
const double STEPS[3] = {0.35, 0.6, 0.85};  // Lambert thresholds between the four cel tones
const double GLOW_K = 0.45, GLOW_Z = 0.35;  // the circling light: strength, height

struct V3
{
	double x = 0, y = 0, z = 0;
	V3() {}
	V3(double a, double b, double c) : x(a), y(b), z(c) {}
	V3 operator+(const V3& o) const { return V3(x + o.x, y + o.y, z + o.z); }
	V3 operator-(const V3& o) const { return V3(x - o.x, y - o.y, z - o.z); }
	V3 operator*(double s) const { return V3(x * s, y * s, z * s); }
	double Dot(const V3& o) const { return x * o.x + y * o.y + z * o.z; }
	V3 Unit() const { double l = std::sqrt(Dot(*this)); return (l > 0) ? (*this) * (1 / l) : *this; }
};

struct M3
{
	double m[3][3];
	V3 operator*(const V3& v) const
	{
		return V3(m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z, m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
			m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z);
	}
	V3 TransposeTimes(const V3& v) const
	{
		return V3(m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z, m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z,
			m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z);
	}
};

M3 RotY(double theDegrees)
{
	double t = theDegrees * kPi / 180, c = std::cos(t), s = std::sin(t);
	M3 r = {{{c, 0, s}, {0, 1, 0}, {-s, 0, c}}};
	return r;
}

const V3 LIGHT = V3(-0.6, 0.7, 1.0).Unit();      // view space, upper-left front
const V3 KEY = V3(-1.0, 1.0, 1.3).Unit();
const V3 FILL = V3(1.0, -1.0, 0.8).Unit();

struct Plane
{
	V3 n;
	double d;
};

struct Model
{
	std::vector<Plane> mPlanes;
	std::vector<V3> mVerts;
	V3 mAxes;                   // half-axes of the smooth proxy ellipsoid
	bool mEllipsoid = false;    // hit the proxy itself (Pearl, onyx): no facets
	Rgba mTones[4];
	Rgba mLine;
	bool mSmooth = false;       // shaded from the proxy's normal
	bool mPastel = false;       // Pearl's pastel tint toward the rim
	bool mRainbow = false;      // Bismuth: a hue per facet
	double mRim = 0.25;
	double mGlint = 0.993;
};

// --- geometry (gemsheet.py) ---------------------------------------------------------------------------------

typedef std::vector<std::pair<double, double> > Poly;   // counter-clockwise, y up

Poly NGon(int theCount, double rx, double ry, double thePhase = 90.0)
{
	Poly p;
	for (int i = 0; i < theCount; i++)
	{
		double a = (thePhase + 360.0 * i / theCount) * kPi / 180;
		p.push_back(std::make_pair(rx * std::cos(a), ry * std::sin(a)));
	}
	return p;
}

Poly Teardrop(double r, double cy, double theApex, int theCount)
{
	bool isUp = theApex > cy;
	double aDist = std::fabs(theApex - cy);
	double aHalf = std::acos(r / aDist) * 180 / kPi;
	double aToward = isUp ? 90.0 : -90.0;
	Poly anArc;
	for (int i = 0; i < theCount; i++)
	{
		double a = (aToward + aHalf + (360.0 - 2 * aHalf) * i / (theCount - 1)) * kPi / 180;
		anArc.push_back(std::make_pair(r * std::cos(a), cy + r * std::sin(a)));
	}
	Poly p;
	if (isUp)
	{
		p.push_back(std::make_pair(0.0, theApex));
		p.insert(p.end(), anArc.begin(), anArc.end());
	}
	else
	{
		p = anArc;
		p.push_back(std::make_pair(0.0, theApex));
	}
	return p;
}

Poly Chamfered(const Poly& thePoly, double theCut)
{
	Poly p;
	size_t n = thePoly.size();
	for (size_t i = 0; i < n; i++)
	{
		const std::pair<double, double>& c = thePoly[i];
		const std::pair<double, double>& a = thePoly[(i + n - 1) % n];
		const std::pair<double, double>& b = thePoly[(i + 1) % n];
		double ax = a.first - c.first, ay = a.second - c.second, al = std::hypot(ax, ay);
		double bx = b.first - c.first, by = b.second - c.second, bl = std::hypot(bx, by);
		p.push_back(std::make_pair(c.first + ax / al * theCut, c.second + ay / al * theCut));
		p.push_back(std::make_pair(c.first + bx / bl * theCut, c.second + by / bl * theCut));
	}
	return p;
}

Poly Rect(double hw, double hh)
{
	Poly p;
	p.push_back(std::make_pair(hw, hh));
	p.push_back(std::make_pair(-hw, hh));
	p.push_back(std::make_pair(-hw, -hh));
	p.push_back(std::make_pair(hw, -hh));
	return p;
}

// A step cut (make_su_gems_mod.step_cut): a vertical girdle plane per outline edge, crown rings stepping in, the table;
// the back mirrors the front
std::vector<Plane> StepCut(const Poly& thePoly, double theGirdle, const std::vector<std::pair<double, double> >& theSteps)
{
	std::vector<Plane> aPlanes;
	size_t n = thePoly.size();
	std::vector<double> mx(n), my(n), o(n);
	for (size_t i = 0; i < n; i++)
	{
		double ex = thePoly[(i + 1) % n].first - thePoly[i].first, ey = thePoly[(i + 1) % n].second - thePoly[i].second;
		double l = std::hypot(ex, ey);
		mx[i] = ey / l;
		my[i] = -ex / l;
		o[i] = mx[i] * thePoly[i].first + my[i] * thePoly[i].second;
		aPlanes.push_back(Plane{V3(mx[i], my[i], 0), o[i]});
	}
	std::vector<Plane> aCrown;
	std::vector<double> anOff = o;
	double z = theGirdle;
	for (size_t s = 0; s < theSteps.size(); s++)
	{
		double anInset = theSteps[s].first, z2 = theSteps[s].second;
		double dz = z2 - z, ln = std::hypot(dz, anInset);
		for (size_t i = 0; i < n; i++)
			aCrown.push_back(Plane{V3(mx[i] * dz / ln, my[i] * dz / ln, anInset / ln), (dz * anOff[i] + anInset * z) / ln});
		for (size_t i = 0; i < n; i++)
			anOff[i] -= anInset;
		z = z2;
	}
	aCrown.push_back(Plane{V3(0, 0, 1), z});
	for (size_t i = 0; i < aCrown.size(); i++)
	{
		aPlanes.push_back(aCrown[i]);
		aPlanes.push_back(Plane{V3(aCrown[i].n.x, aCrown[i].n.y, -aCrown[i].n.z), aCrown[i].d});
	}
	return aPlanes;
}

// The solid's corners: feasible intersections of plane triples
std::vector<V3> SolidVertices(const std::vector<Plane>& thePlanes)
{
	std::vector<V3> aVerts;
	size_t n = thePlanes.size();
	for (size_t i = 0; i < n; i++)
	{
		for (size_t j = i + 1; j < n; j++)
		{
			for (size_t k = j + 1; k < n; k++)
			{
				const V3 &a = thePlanes[i].n, &b = thePlanes[j].n, &c = thePlanes[k].n;
				double det = a.x * (b.y * c.z - b.z * c.y) - a.y * (b.x * c.z - b.z * c.x) + a.z * (b.x * c.y - b.y * c.x);
				if (std::fabs(det) < 1e-9)
					continue;
				double d1 = thePlanes[i].d, d2 = thePlanes[j].d, d3 = thePlanes[k].d;
				// Cramer's rule
				double x = (d1 * (b.y * c.z - b.z * c.y) - a.y * (d2 * c.z - b.z * d3) + a.z * (d2 * c.y - b.y * d3)) / det;
				double y = (a.x * (d2 * c.z - b.z * d3) - d1 * (b.x * c.z - b.z * c.x) + a.z * (b.x * d3 - d2 * c.x)) / det;
				double z = (a.x * (b.y * d3 - d2 * c.y) - a.y * (b.x * d3 - d2 * c.x) + d1 * (b.x * c.y - b.y * c.x)) / det;
				V3 p(x, y, z);
				bool isInside = true;
				for (size_t m = 0; m < n && isInside; m++)
					isInside = thePlanes[m].n.Dot(p) <= thePlanes[m].d + 1e-6;
				if (isInside)
					aVerts.push_back(p);
			}
		}
	}
	return aVerts;
}

std::vector<V3> EllipsoidPoints(const V3& theAxes, int theRings = 24, int theCount = 48)
{
	std::vector<V3> p;
	p.push_back(V3(0, theAxes.y, 0));
	p.push_back(V3(0, -theAxes.y, 0));
	for (int i = 1; i < theRings; i++)
	{
		double aLat = kPi * i / theRings - kPi / 2;
		for (int k = 0; k < theCount; k++)
		{
			double aLon = 2 * kPi * k / theCount;
			p.push_back(V3(theAxes.x * std::cos(aLat) * std::cos(aLon), theAxes.y * std::sin(aLat), theAxes.z * std::cos(aLat) * std::sin(aLon)));
		}
	}
	return p;
}

std::vector<std::pair<double, double> > Hull2D(std::vector<std::pair<double, double> > thePoints)
{
	std::sort(thePoints.begin(), thePoints.end());
	thePoints.erase(std::unique(thePoints.begin(), thePoints.end()), thePoints.end());
	if (thePoints.size() < 3)
		return thePoints;
	auto aCross = [](const std::pair<double, double>& o, const std::pair<double, double>& a, const std::pair<double, double>& b)
	{
		return (a.first - o.first) * (b.second - o.second) - (a.second - o.second) * (b.first - o.first);
	};
	std::vector<std::pair<double, double> > h(2 * thePoints.size());
	size_t k = 0;
	for (size_t i = 0; i < thePoints.size(); i++)
	{
		while (k >= 2 && aCross(h[k - 2], h[k - 1], thePoints[i]) <= 0)
			k--;
		h[k++] = thePoints[i];
	}
	for (size_t i = thePoints.size() - 1, t = k + 1; i > 0; i--)
	{
		while (k >= t && aCross(h[k - 2], h[k - 1], thePoints[i - 1]) <= 0)
			k--;
		h[k++] = thePoints[i - 1];
	}
	h.resize(k - 1);
	return h;
}

std::vector<std::pair<double, double> > Project(const Model& theModel, const M3& theRot)
{
	std::vector<std::pair<double, double> > p;
	for (const V3& v : theModel.mVerts)
	{
		V3 r = theRot * v;
		double s = EYE / (EYE - r.z);
		p.push_back(std::make_pair(r.x * s, r.y * s));
	}
	return Hull2D(p);
}

// --- models ---------------------------------------------------------------------------------------------------

Rgba Tone(int r, int g, int b) { return Rgba::Bytes(r, g, b); }

void SetTones(Model& theModel, Rgba a, Rgba b, Rgba c, Rgba d)
{
	theModel.mTones[0] = a; theModel.mTones[1] = b; theModel.mTones[2] = c; theModel.mTones[3] = d;
	theModel.mLine = Rgba(a.r * 0.3f, a.g * 0.3f, a.b * 0.3f);
}

// Four tones from one colour: shadow, the colour, lighter, highlight
void BaseTones(Model& theModel, int r, int g, int b)
{
	Rgba c = Tone(r, g, b);
	SetTones(theModel, Rgba(c.r * 0.6f, c.g * 0.6f, c.b * 0.6f), c, Lighter(c, 0.38), Lighter(c, 0.75));
}

void Finish(Model& theModel)
{
	if (theModel.mVerts.empty())
		theModel.mVerts = SolidVertices(theModel.mPlanes);
	double ax = 0, ay = 0, az = 0;
	for (const V3& v : theModel.mVerts)
	{
		ax = std::max(ax, std::fabs(v.x)); ay = std::max(ay, std::fabs(v.y)); az = std::max(az, std::fabs(v.z));
	}
	if (theModel.mAxes.x == 0)
		theModel.mAxes = V3(ax, ay, az);
}

typedef std::vector<std::pair<double, double> > Steps;

Model Faceted(const Poly& thePoly, const Steps& theSteps = Steps{{5.0, 10.0}, {5.0, 15.0}}, double theGirdle = 4.0)
{
	Model m;
	m.mPlanes = StepCut(thePoly, theGirdle, theSteps);
	return m;
}

Model Smooth(const V3& theAxes)
{
	Model m;
	m.mEllipsoid = true;
	m.mSmooth = true;
	m.mAxes = theAxes;
	m.mVerts = EllipsoidPoints(theAxes);
	return m;
}

// The port's own cuts (cel and glossy)
Model OwnModel(int theColor)
{
	static const int kColors[12][3] = {{250, 212, 28}, {232, 232, 242}, {38, 122, 250}, {232, 30, 48}, {184, 62, 226},
		{255, 136, 24}, {46, 194, 64}, {255, 110, 188}, {22, 194, 198}, {124, 86, 255}, {86, 86, 100}, {166, 234, 38}};
	Model m;
	switch (theColor)
	{
	case 0: m = Faceted(NGon(4, 28, 37), Steps{{6.0, 10.0}, {6.0, 15.0}}); break;                       // rhombus
	case 1: m = Faceted(NGon(16, 34, 34, 90 + 11.25), Steps{{4.0, 8.0}, {5.0, 12.0}, {5.0, 15.0}}); break; // round
	case 2: m = Faceted(Chamfered(NGon(3, 42, 42, 90), 7), Steps{{6.0, 10.0}, {5.0, 15.0}}); break;      // triangle
	case 3: m = Faceted(Chamfered(NGon(4, 33 * std::sqrt(2.0), 33 * std::sqrt(2.0), 45), 6)); break;    // square
	case 4: m = Faceted(NGon(6, 34, 37, 90)); break;                                                     // hexagon
	case 5: m = Faceted(NGon(14, 36, 27, 90 + 360.0 / 28)); break;                                       // oval
	case 6: m = Faceted(Chamfered(Rect(25, 35), 9)); break;                                              // emerald cut
	case 7: m = Faceted(NGon(5, 36, 36, -90)); break;                                                    // shield
	case 8: m = Faceted(Chamfered(Rect(36, 23), 7)); break;                                              // baguette
	case 9: m = Faceted(Chamfered(NGon(3, 42, 42, -90), 7), Steps{{6.0, 10.0}, {5.0, 15.0}}); break;     // triangle down
	case 10: m = Smooth(V3(31, 31, 15)); break;                                                           // cabochon
	default: m = Faceted(NGon(8, 34, 34, 90 + 22.5)); break;                                             // octagon
	}
	BaseTones(m, kColors[theColor % 12][0], kColors[theColor % 12][1], kColors[theColor % 12][2]);
	Finish(m);
	return m;
}

// tools/make_su_gems_mod.py su_gems()
Model SuModel(int theColor)
{
	Model m;
	switch (theColor)
	{
	case 0:
		m = Faceted(Poly{{0, 37}, {-29, 9}, {0, -37}, {29, 9}}, Steps{{6.0, 10.0}, {6.0, 15.0}});
		SetTones(m, Tone(186, 140, 0), Tone(238, 196, 18), Tone(255, 232, 84), Tone(255, 248, 186));
		break;
	case 1:
		m = Smooth(V3(33, 24, 15));
		m.mPastel = true;
		m.mGlint = 0.996;
		SetTones(m, Tone(214, 196, 216), Tone(236, 224, 238), Tone(250, 244, 250), Tone(255, 255, 255));
		break;
	case 2:
		m = Faceted(Teardrop(23, -11, 36, 13));
		SetTones(m, Tone(22, 88, 196), Tone(52, 140, 238), Tone(120, 192, 255), Tone(200, 232, 255));
		break;
	case 3:
		m = Faceted(Chamfered(NGon(4, 33 * std::sqrt(2.0), 33 * std::sqrt(2.0), 45), 9), Steps{{6.0, 10.0}, {6.0, 15.0}});
		SetTones(m, Tone(150, 0, 24), Tone(214, 22, 46), Tone(255, 84, 94), Tone(255, 170, 170));
		break;
	case 4:
		m = Faceted(NGon(16, 26, 35, 90 + 360.0 / 32));
		SetTones(m, Tone(108, 30, 150), Tone(164, 72, 206), Tone(210, 134, 240), Tone(240, 204, 255));
		break;
	case 5:
		m = Faceted(NGon(6, 37, 26 / std::sin(60 * kPi / 180), 0));
		SetTones(m, Tone(168, 66, 8), Tone(226, 110, 30), Tone(255, 160, 72), Tone(255, 212, 150));
		break;
	case 6:
		m = Faceted(Chamfered(NGon(3, 42, 42, -90), 7), Steps{{6.0, 10.0}, {5.0, 15.0}});
		SetTones(m, Tone(36, 128, 18), Tone(88, 190, 40), Tone(158, 232, 90), Tone(214, 250, 172));
		break;
	case 7:
		m = Faceted(NGon(5, 36, 36, 90));
		SetTones(m, Tone(196, 74, 136), Tone(238, 128, 178), Tone(255, 180, 214), Tone(255, 226, 240));
		break;
	case 8:
		m = Faceted(Teardrop(23, 11, -36, 13));
		SetTones(m, Tone(18, 30, 120), Tone(36, 66, 186), Tone(84, 124, 232), Tone(164, 190, 255));
		break;
	case 9:
		m = Faceted(NGon(8, 34, 34, 90 + 22.5));
		m.mRainbow = true;
		SetTones(m, Tone(70, 70, 70), Tone(120, 120, 120), Tone(175, 175, 175), Tone(225, 225, 225));
		break;
	case 10:
		m = Faceted(NGon(20, 35, 35, 90 + 9), Steps{{4.0, 9.0}, {5.0, 13.0}, {5.0, 16.0}});
		SetTones(m, Tone(104, 0, 52), Tone(164, 18, 84), Tone(218, 60, 126), Tone(250, 152, 192));
		break;
	default:
		return OwnModel(theColor);
	}
	Finish(m);
	return m;
}

Model ModelFor(GemStyle theStyle, int theColor)
{
	return (theStyle == GEMS_SU) ? SuModel(theColor) : OwnModel(theColor);
}

// --- ray casting --------------------------------------------------------------------------------------------

struct Sample
{
	float mDist;        // inside the silhouette, model units (negative outside)
	bool mHit;
	int mFace;          // entering plane (facets)
	V3 mModelPoint;     // hit point, model space
	V3 mDir;            // view ray
};

struct Cast
{
	int mX0, mY0, mW, mH;   // supersample window
	std::vector<Sample> mSamples;
	M3 mRot;
	std::vector<V3> mViewNormals;
};

bool RayEllipsoid(const V3& theEye, const V3& theDir, const V3& theAxes, double* theT)
{
	V3 o(theEye.x / theAxes.x, theEye.y / theAxes.y, theEye.z / theAxes.z);
	V3 d(theDir.x / theAxes.x, theDir.y / theAxes.y, theDir.z / theAxes.z);
	double a = d.Dot(d), b = 2 * o.Dot(d), c = o.Dot(o) - 1;
	double aDisc = b * b - 4 * a * c;
	if (aDisc < 0)
		return false;
	*theT = (-b - std::sqrt(aDisc)) / (2 * a);
	return true;
}

// The rays of a theSize x theSize output (supersampled theSS times) with unit_px output pixels per model unit and
// the model's image-plane origin at output pixel theOrigin
void CastRays(const Model& theModel, const M3& theRot, int theSize, double theUnitPx, double theOX, double theOY, int theSS, Cast& theCast)
{
	theCast.mRot = theRot;
	theCast.mViewNormals.clear();
	for (const Plane& p : theModel.mPlanes)
		theCast.mViewNormals.push_back(theRot * p.n);
	std::vector<std::pair<double, double> > aHull = Project(theModel, theRot);
	size_t ne = aHull.size();
	std::vector<double> enx(ne), eny(ne), ec(ne);
	double hx0 = 1e30, hx1 = -1e30, hy0 = 1e30, hy1 = -1e30;
	for (size_t i = 0; i < ne; i++)
	{
		const std::pair<double, double>& a = aHull[i];
		const std::pair<double, double>& b = aHull[(i + 1) % ne];
		double nx = -(b.second - a.second), ny = b.first - a.first;     // inward (counter-clockwise hull)
		double l = std::hypot(nx, ny);
		enx[i] = nx / l; eny[i] = ny / l;
		ec[i] = enx[i] * a.first + eny[i] * a.second;
		hx0 = std::min(hx0, a.first); hx1 = std::max(hx1, a.first);
		hy0 = std::min(hy0, a.second); hy1 = std::max(hy1, a.second);
	}
	int n = theSize * theSS;
	auto aPixel = [&](double m, double o, bool isX) { return isX ? (o + m * theUnitPx) * theSS : (o - m * theUnitPx) * theSS; };
	int x0 = std::max(0, (int) std::floor(aPixel(hx0 - 1, theOX, true)));
	int x1 = std::min(n, (int) std::ceil(aPixel(hx1 + 1, theOX, true)));
	int y0 = std::max(0, (int) std::floor(aPixel(hy1 + 1, theOY, false)));
	int y1 = std::min(n, (int) std::ceil(aPixel(hy0 - 1, theOY, false)));
	theCast.mX0 = x0; theCast.mY0 = y0;
	theCast.mW = std::max(0, x1 - x0); theCast.mH = std::max(0, y1 - y0);
	theCast.mSamples.assign((size_t) theCast.mW * theCast.mH, Sample());
	V3 anEye(0, 0, EYE);
	std::vector<double> aNum(theModel.mPlanes.size());
	for (size_t i = 0; i < theModel.mPlanes.size(); i++)
		aNum[i] = theModel.mPlanes[i].d - theCast.mViewNormals[i].Dot(anEye);
	for (int y = 0; y < theCast.mH; y++)
	{
		double my = (theOY - (y0 + y + 0.5) / theSS) / theUnitPx;
		for (int x = 0; x < theCast.mW; x++)
		{
			double mx = ((x0 + x + 0.5) / theSS - theOX) / theUnitPx;
			Sample& s = theCast.mSamples[(size_t) y * theCast.mW + x];
			double aDist = 1e30;
			for (size_t i = 0; i < ne; i++)
				aDist = std::min(aDist, enx[i] * mx + eny[i] * my - ec[i]);
			s.mDist = (float) aDist;
			s.mHit = false;
			s.mFace = 0;
			if (aDist <= 0)
				continue;
			V3 aDir = V3(mx, my, -EYE).Unit();
			s.mDir = aDir;
			double t;
			if (theModel.mEllipsoid)
			{
				// the proxy ellipsoid in model space
				V3 anEyeM = theRot.TransposeTimes(anEye), aDirM = theRot.TransposeTimes(aDir);
				if (!RayEllipsoid(anEyeM, aDirM, theModel.mAxes, &t))
					continue;
				s.mHit = true;
				s.mModelPoint = anEyeM + aDirM * t;
				continue;
			}
			double tIn = -1e30, tOut = 1e30;
			int aFace = 0;
			for (size_t i = 0; i < theModel.mPlanes.size(); i++)
			{
				double aDenom = aDir.Dot(theCast.mViewNormals[i]);
				if (aDenom < 0)
				{
					t = aNum[i] / aDenom;
					if (t > tIn)
					{
						tIn = t;
						aFace = (int) i;
					}
				}
				else if (aDenom > 0)
					tOut = std::min(tOut, aNum[i] / aDenom);
			}
			if (tIn < tOut)
			{
				s.mHit = true;
				s.mFace = aFace;
				s.mModelPoint = theRot.TransposeTimes(anEye + aDir * tIn);
			}
		}
	}
}

int Level(double theLight)
{
	return (theLight >= STEPS[0]) + (theLight >= STEPS[1]) + (theLight >= STEPS[2]);
}

double Clamp01(double v)
{
	return (v < 0) ? 0 : ((v > 1) ? 1 : v);
}

// The colour of one sample (straight RGB; alpha is the silhouette's)
Rgba Shade(GemStyle theStyle, const Model& theModel, const Cast& theCast, const Sample& s, bool hasGlow, double theGlow)
{
	if (!s.mHit)
		return theModel.mLine;
	V3 v = s.mDir * -1;
	V3 aSmoothM = V3(s.mModelPoint.x / (theModel.mAxes.x * theModel.mAxes.x), s.mModelPoint.y / (theModel.mAxes.y * theModel.mAxes.y),
		s.mModelPoint.z / (theModel.mAxes.z * theModel.mAxes.z)).Unit();
	V3 aSmooth = theCast.mRot * aSmoothM;
	V3 n = (theModel.mSmooth || theModel.mEllipsoid) ? aSmooth : theCast.mViewNormals[s.mFace];
	V3 aGlowDir;
	if (hasGlow)
	{
		double a = theGlow * kPi / 180;
		aGlowDir = V3(std::cos(a), std::sin(a), GLOW_Z).Unit();
	}
	double ndv = Clamp01(n.Dot(v));

	if (theStyle == GEMS_GLOSSY)
	{
		// glass: the body colour lit by the key light, a sky seen in the reflection, a hard highlight
		double aDiffuse = Clamp01(n.Dot(KEY));
		double aFill = Clamp01(n.Dot(FILL));
		V3 r = n * (2 * n.Dot(v)) - v;
		double aSky = Clamp01((r.y + 0.15) / 0.5);
		double aFresnel = std::pow(1 - ndv, 2.0);
		double aSpec = std::pow(Clamp01(n.Dot((KEY + v).Unit())), 80.0);
		double aGlowLight = hasGlow ? GLOW_K * std::pow(Clamp01(n.Dot(aGlowDir)), 2.0) : 0;
		double t = Clamp01(0.18 + 0.55 * aDiffuse + 0.25 * aFill * aFill + aGlowLight);
		Rgba c = (t < 0.5) ? Mix(theModel.mTones[0], theModel.mTones[1], t * 2) : Mix(theModel.mTones[1], theModel.mTones[2], (t - 0.5) * 2);
		c = Mix(c, theModel.mTones[3], aSky * (0.25 + 0.5 * aFresnel));
		c = Mix(c, Rgba(1, 1, 1), Clamp01(aSpec * 1.2 + aGlowLight * 0.3));
		if (s.mDist < OUTLINE * 0.7)
			c = Mix(c, theModel.mTones[0], 0.7);
		return c;
	}

	double aLight = Clamp01(n.Dot(LIGHT));
	if (hasGlow)
		aLight += GLOW_K * std::pow(Clamp01(n.Dot(aGlowDir)), 2.0);
	int aLevel = Level(aLight);
	Rgba c = theModel.mTones[aLevel];
	if (theModel.mRainbow)
	{
		const V3& nm = theModel.mPlanes[s.mFace].n;
		double aHue = (std::atan2(nm.y, std::fabs(nm.x)) / kPi + 0.5) * 0.85 + 0.33 * std::round(std::fabs(nm.z) * 3);
		aHue -= std::floor(aHue);
		const Rgba& t = theModel.mTones[aLevel];
		c = Rgba::Hsv(aHue, 0.6, std::max(t.r, std::max(t.g, t.b)));
	}
	if (theModel.mPastel)
	{
		static const Rgba kTints[3] = {Rgba::Bytes(196, 186, 242), Rgba::Bytes(250, 196, 226), Rgba::Bytes(192, 240, 226)};
		double az = std::fmod(std::atan2(aSmoothM.x, aSmoothM.z) / (2 * kPi), 0.5);
		if (az < 0)
			az += 0.5;
		az *= 6.0;
		int k = (int) std::floor(az) % 3;
		double f = az - std::floor(az);
		Rgba aTint = Mix(kTints[k], kTints[(k + 1) % 3], f);
		c = Mix(c, aTint, std::pow(Clamp01(1 - ndv), 1.5) * 0.75);
	}
	if (ndv < theModel.mRim)
		c = Rgba(theModel.mTones[0].r * 0.8f, theModel.mTones[0].g * 0.8f, theModel.mTones[0].b * 0.8f);
	if (aSmooth.Dot((LIGHT + v).Unit()) > theModel.mGlint)
		c = Rgba(1, 1, 1);
	if (s.mDist < OUTLINE)
		c = theModel.mLine;
	return c;
}

// Shades a cast into a theSize x theSize premultiplied tile (4 floats per pixel)
void ShadeTile(GemStyle theStyle, const Model& theModel, const Cast& theCast, int theSize, int theSS, bool hasGlow, double theGlow,
	std::vector<float>& theTile)
{
	theTile.assign((size_t) theSize * theSize * 4, 0.0f);
	float aNorm = 1.0f / (theSS * theSS);
	for (int y = 0; y < theCast.mH; y++)
	{
		int ty = (theCast.mY0 + y) / theSS;
		for (int x = 0; x < theCast.mW; x++)
		{
			const Sample& s = theCast.mSamples[(size_t) y * theCast.mW + x];
			if (s.mDist <= 0)
				continue;
			Rgba c = Shade(theStyle, theModel, theCast, s, hasGlow, theGlow);
			float* p = &theTile[((size_t) ty * theSize + (theCast.mX0 + x) / theSS) * 4];
			p[0] += c.r * aNorm; p[1] += c.g * aNorm; p[2] += c.b * aNorm; p[3] += aNorm;
		}
	}
}

void BBoxCentre(const Model& theModel, double* cx, double* cy)
{
	M3 anId = RotY(0);
	std::vector<std::pair<double, double> > h = Project(theModel, anId);
	double x0 = 1e30, x1 = -1e30, y0 = 1e30, y1 = -1e30;
	for (const std::pair<double, double>& p : h)
	{
		x0 = std::min(x0, p.first); x1 = std::max(x1, p.first);
		y0 = std::min(y0, p.second); y1 = std::max(y1, p.second);
	}
	*cx = (x0 + x1) / 2;
	*cy = (y0 + y1) / 2;
}

}

GemStyle GemStyleFromName(const std::string& theName)
{
	if (theName == "glossy")
		return GEMS_GLOSSY;
	if (theName == "su")
		return GEMS_SU;
	return GEMS_CEL;
}

int GemModelCount(GemStyle theStyle)
{
	return (theStyle == GEMS_SU) ? 11 : 12;
}

void DrawGemSheet(GemStyle theStyle, int theColors, int theScale, std::vector<uint32_t>& theOut)
{
	theColors = std::max(1, std::min(12, theColors));
	int aCell = CELL * theScale;
	int aW = FRAMES * aCell, aH = 2 * theColors * aCell;
	theOut.assign((size_t) aW * aH, 0);
	int aSS = (theScale >= 3) ? 2 : 3;
	double aUnitPx = UNIT_PX * theScale;
	// One colour's two rows; the colours share nothing, so native builds draw them on several threads
	auto drawColor = [&](int c) {
		Cast aCast;
		std::vector<float> aTile;
		Model aModel = ModelFor(theStyle, c);
		double bx, by;
		BBoxCentre(aModel, &bx, &by);
		double ox = ANCHOR_X * theScale - bx * aUnitPx, oy = ANCHOR_Y * theScale + by * aUnitPx;
		for (int aRow = 0; aRow < 2; aRow++)
		{
			for (int f = 0; f < FRAMES; f++)
			{
				bool isGlow = aRow == 1;
				// the glow row is the rest pose: frame 0's rays serve it all
				if (!isGlow || (f == 0))
					CastRays(aModel, RotY(isGlow ? 0.0 : f * SPIN_STEP), aCell, aUnitPx, ox, oy, aSS, aCast);
				ShadeTile(theStyle, aModel, aCast, aCell, aSS, isGlow, 90.0 + f * GLOW_STEP, aTile);
				int aTop = (2 * c + aRow) * aCell, aLeft = f * aCell;
				for (int y = 0; y < aCell; y++)
				{
					for (int x = 0; x < aCell; x++)
					{
						// the game samples x 2..50 and y 0..48 of a cell: the gutter stays empty
						if ((x < 2 * theScale) || (x >= 51 * theScale) || (y >= 49 * theScale))
							continue;
						const float* p = &aTile[((size_t) y * aCell + x) * 4];
						if (p[3] <= 0)
							continue;
						theOut[(size_t) (aTop + y) * aW + aLeft + x] = PackArgb(p[0] / p[3], p[1] / p[3], p[2] / p[3], p[3]);
					}
				}
			}
		}
	};
#ifdef __EMSCRIPTEN__
	for (int c = 0; c < theColors; c++)
		drawColor(c);
#else
	std::atomic<int> aNext(0);
	int aThreads = std::max(1, std::min({(int) std::thread::hardware_concurrency(), theColors, 4}));
	std::vector<std::thread> aWorkers;
	for (int t = 0; t < aThreads; t++)
	{
		aWorkers.emplace_back([&]() {
			for (int c = aNext++; c < theColors; c = aNext++)
				drawColor(c);
		});
	}
	for (std::thread& aWorker : aWorkers)
		aWorker.join();
#endif
}

void DrawGem(Canvas& theCanvas, GemStyle theStyle, int theColor, double cx, double cy, double theW, double theH)
{
	Model aModel = ModelFor(theStyle, theColor);
	std::vector<std::pair<double, double> > h = Project(aModel, RotY(0));
	double x0 = 1e30, x1 = -1e30, y0 = 1e30, y1 = -1e30;
	for (const std::pair<double, double>& p : h)
	{
		x0 = std::min(x0, p.first); x1 = std::max(x1, p.first);
		y0 = std::min(y0, p.second); y1 = std::max(y1, p.second);
	}
	double aFit = std::min(theW / (x1 - x0), theH / (y1 - y0));    // logical px per model unit
	int s = theCanvas.mScale;
	int aSize = (int) std::ceil(std::max(theW, theH) + 4) * s;
	double ox = aSize / 2.0 - (x0 + x1) / 2 * aFit * s, oy = aSize / 2.0 + (y0 + y1) / 2 * aFit * s;
	Cast aCast;
	std::vector<float> aTile;
	CastRays(aModel, RotY(0), aSize, aFit * s, ox, oy, 3, aCast);
	ShadeTile(theStyle, aModel, aCast, aSize, 3, false, 0, aTile);
	int aLeft = (int) std::lround(cx * s - aSize / 2.0), aTop = (int) std::lround(cy * s - aSize / 2.0);
	for (int y = 0; y < aSize; y++)
	{
		int ty = aTop + y;
		if (ty < 0 || ty >= theCanvas.mPH)
			continue;
		for (int x = 0; x < aSize; x++)
		{
			int tx = aLeft + x;
			if (tx < 0 || tx >= theCanvas.mPW)
				continue;
			const float* p = &aTile[((size_t) y * aSize + x) * 4];
			if (p[3] <= 0)
				continue;
			float* d = &theCanvas.mPx[((size_t) ty * theCanvas.mPW + tx) * 4];
			float inv = 1 - p[3];
			for (int k = 0; k < 4; k++)
				d[k] = p[k] + d[k] * inv;
		}
	}
}

}
