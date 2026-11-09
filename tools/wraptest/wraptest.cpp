// port: framework/TriangleWrap (the texture wrap of DrawTrianglesTex: triangles cut at whole-number u and v) against what
// wrapping means: every piece lies in one cell, the pieces cover the triangle exactly once, and each piece's u and v are the
// triangle's (interpolated at that point) modulo 1. Native and pure: no window, no SDL. tools/check.sh unit runs it.
#include "TriangleWrap.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace Sexy;

static int gFailures = 0;
static int gChecks = 0;

static void Check(bool theOk, const char* theWhat, int theCase)
{
	gChecks++;
	if (!theOk)
	{
		if (gFailures < 20)
			printf("FAIL %s (case %d)\n", theWhat, theCase);
		gFailures++;
	}
}

static unsigned gSeed = 12345;
static float Random(float theLow, float theHigh)
{
	gSeed = gSeed * 1664525u + 1013904223u;
	return theLow + (theHigh - theLow) * ((gSeed >> 8) / 16777216.0f);
}

static float Area(const TriVertex& a, const TriVertex& b, const TriVertex& c)
{
	return 0.5f * ((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y));
}

// the triangle's u, v and colour channel at (x, y)
static bool Interpolate(const TriVertex t[3], float x, float y, float& u, float& v, float theChannel[4])
{
	TriVertex p(x, y);
	float aTotal = Area(t[0], t[1], t[2]);
	float w0 = Area(p, t[1], t[2]) / aTotal, w1 = Area(t[0], p, t[2]) / aTotal, w2 = Area(t[0], t[1], p) / aTotal;
	u = w0 * t[0].u + w1 * t[1].u + w2 * t[2].u;
	v = w0 * t[0].v + w1 * t[1].v + w2 * t[2].v;
	for (int i = 0; i < 4; i++)
		theChannel[i] = w0 * ((t[0].color >> (8 * i)) & 0xFF) + w1 * ((t[1].color >> (8 * i)) & 0xFF) + w2 * ((t[2].color >> (8 * i)) & 0xFF);
	return true;
}

static float DistanceToWhole(float theValue)
{
	float aFrac = theValue - floorf(theValue);
	return aFrac < 0.5f ? aFrac : 1.0f - aFrac;
}

static void RunCase(int theCase, const TriVertex theTriangle[3], bool theWrapU, bool theWrapV)
{
	TriVertex aIn[1][3] = {{theTriangle[0], theTriangle[1], theTriangle[2]}};
	std::vector<TriVertex> aOut;
	SplitWrappedTriangles(aIn, 1, theWrapU, theWrapV, aOut);
	Check(aOut.size() % 3 == 0, "a whole number of triangles", theCase);

	float aArea = fabsf(Area(theTriangle[0], theTriangle[1], theTriangle[2]));
	float aPiecesArea = 0;
	bool aInside = true, aMatches = true, aWinding = true;
	for (size_t i = 0; i + 2 < aOut.size(); i += 3)
	{
		const TriVertex* p = &aOut[i];
		float aPieceArea = Area(p[0], p[1], p[2]);
		aPiecesArea += fabsf(aPieceArea);
		if (aPieceArea * Area(theTriangle[0], theTriangle[1], theTriangle[2]) < -1e-6f)
			aWinding = false;
		for (int j = 0; j < 3; j++)
		{
			if (theWrapU && (p[j].u < -1e-3f || p[j].u > 1.001f))
				aInside = false;
			if (theWrapV && (p[j].v < -1e-3f || p[j].v > 1.001f))
				aInside = false;
			float u, v, c[4];
			Interpolate(theTriangle, p[j].x, p[j].y, u, v, c);
			if (theWrapU ? DistanceToWhole(u - p[j].u) > 2e-3f : fabsf(u - p[j].u) > 2e-3f)
				aMatches = false;
			if (theWrapV ? DistanceToWhole(v - p[j].v) > 2e-3f : fabsf(v - p[j].v) > 2e-3f)
				aMatches = false;
			for (int k = 0; k < 4; k++)
				if (fabsf(c[k] - (float) ((p[j].color >> (8 * k)) & 0xFF)) > 1.6f)
					aMatches = false;
		}
	}
	Check(aInside, "every piece's u and v are in 0..1", theCase);
	Check(aMatches, "u, v and colour are the triangle's, modulo 1", theCase);
	Check(aWinding, "the winding is kept", theCase);
	Check(fabsf(aPiecesArea - aArea) <= 1e-3f * aArea + 1e-4f, "the pieces cover the triangle exactly once", theCase);
}

int main()
{
	// a triangle inside one cell is drawn as it is
	{
		TriVertex t[1][3] = {{TriVertex(0, 0, 0.1f, 0.2f), TriVertex(10, 0, 0.9f, 0.2f), TriVertex(0, 10, 0.1f, 0.8f)}};
		std::vector<TriVertex> aOut;
		SplitWrappedTriangles(t, 1, true, true, aOut);
		bool aSame = aOut.size() == 3;
		for (int i = 0; aSame && i < 3; i++)
			aSame = aOut[i].x == t[0][i].x && aOut[i].y == t[0][i].y && aOut[i].u == t[0][i].u && aOut[i].v == t[0][i].v;
		Check(aSame, "a triangle inside one cell is unchanged", 0);
	}
	// ending on the cell's edge is no crossing
	{
		TriVertex t[1][3] = {{TriVertex(0, 0, 0.0f, 0.0f), TriVertex(10, 0, 1.0f, 0.0f), TriVertex(0, 10, 0.0f, 1.0f)}};
		std::vector<TriVertex> aOut;
		SplitWrappedTriangles(t, 1, true, true, aOut);
		Check(aOut.size() == 3, "a triangle from 0 to 1 is not cut", 1);
	}
	// a triangle that goes one past 1 in u (the tunnel's last quad of a ring) is cut in two or three
	{
		TriVertex t[1][3] = {{TriVertex(0, 0, 0.9f, 0.2f), TriVertex(10, 0, 1.1f, 0.2f), TriVertex(0, 10, 0.9f, 0.4f)}};
		std::vector<TriVertex> aOut;
		SplitWrappedTriangles(t, 1, true, true, aOut);
		Check(aOut.size() >= 6 && aOut.size() <= 9, "a triangle across u = 1 is cut in two or three", 2);
		RunCase(2, t[0], true, true);
	}
	// clamp on one axis: only the other is cut
	{
		TriVertex t[3] = {TriVertex(0, 0, 0.9f, 0.9f), TriVertex(10, 0, 1.1f, 0.9f), TriVertex(0, 10, 0.9f, 1.1f)};
		std::vector<TriVertex> aOut;
		TriVertex aIn[1][3] = {{t[0], t[1], t[2]}};
		SplitWrappedTriangles(aIn, 1, false, false, aOut);
		Check(aOut.size() == 3 && aOut[1].u == 1.1f, "no wrap leaves the triangle alone", 3);
		RunCase(3, t, true, false);
		RunCase(4, t, false, true);
	}
	// too wide: left as it is
	{
		TriVertex t[1][3] = {{TriVertex(0, 0, 0.0f, 0.0f), TriVertex(10, 0, 500.0f, 0.0f), TriVertex(0, 10, 0.0f, 0.5f)}};
		std::vector<TriVertex> aOut;
		SplitWrappedTriangles(t, 1, true, true, aOut);
		Check(aOut.size() == 3 && aOut[1].u == 500.0f, "a triangle across hundreds of cells is not cut", 5);
	}
	// random triangles, with colours, u and v from -3 to 3
	for (int i = 0; i < 20000; i++)
	{
		TriVertex t[3];
		for (int j = 0; j < 3; j++)
			t[j] = TriVertex(Random(-50, 50), Random(-50, 50), Random(-3, 3), Random(-3, 3), (i % 2) ? (DWORD) (gSeed * 2654435761u) : 0);
		if (fabsf(Area(t[0], t[1], t[2])) < 1.0f)
			continue;
		if (i % 2 == 0)
			for (int j = 0; j < 3; j++)
				t[j].color = 0xFF808080;
		RunCase(10 + i, t, true, true);
		if (i % 4 == 0)
		{
			RunCase(10 + i, t, true, false);
			RunCase(10 + i, t, false, true);
		}
	}
	if (gFailures == 0)
		printf("wraptest: %d checks, 0 failed\n", gChecks);
	else
		printf("wraptest: %d checks, %d FAILED\n", gChecks, gFailures);
	return gFailures == 0 ? 0 : 1;
}
