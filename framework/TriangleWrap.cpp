#include "TriangleWrap.h"

#include <cmath>

using namespace Sexy;

namespace
{

typedef std::vector<TriVertex> Polygon;

float Coord(const TriVertex& theVertex, bool theU)
{
	return theU ? theVertex.u : theVertex.v;
}

// ARGB, each channel along the edge; the same colour at both ends stays as it is (0, "the colour of the call", too)
DWORD LerpColor(DWORD theFrom, DWORD theTo, float theT)
{
	if (theFrom == theTo)
		return theFrom;
	DWORD aColor = 0;
	for (int aShift = 0; aShift < 32; aShift += 8)
	{
		float aFrom = (float) ((theFrom >> aShift) & 0xFF);
		float aTo = (float) ((theTo >> aShift) & 0xFF);
		int aChannel = (int) (aFrom + (aTo - aFrom) * theT + 0.5f);
		if (aChannel < 0)
			aChannel = 0;
		else if (aChannel > 255)
			aChannel = 255;
		aColor |= ((DWORD) aChannel) << aShift;
	}
	return aColor;
}

// Where the edge a-b crosses the line u (or v) = k. Worked out from the end with the lower coordinate, so that the pieces
// on the two sides of the line, which meet along it, get exactly the same point.
TriVertex Cross(const TriVertex& theA, const TriVertex& theB, bool theU, float theK)
{
	const TriVertex& aLow = Coord(theA, theU) <= Coord(theB, theU) ? theA : theB;
	const TriVertex& aHigh = &aLow == &theA ? theB : theA;
	float aT = (theK - Coord(aLow, theU)) / (Coord(aHigh, theU) - Coord(aLow, theU));
	TriVertex aPoint;
	aPoint.x = aLow.x + (aHigh.x - aLow.x) * aT;
	aPoint.y = aLow.y + (aHigh.y - aLow.y) * aT;
	aPoint.u = aLow.u + (aHigh.u - aLow.u) * aT;
	aPoint.v = aLow.v + (aHigh.v - aLow.v) * aT;
	aPoint.color = LerpColor(aLow.color, aHigh.color, aT);
	if (theU)
		aPoint.u = theK;
	else
		aPoint.v = theK;
	return aPoint;
}

// The part of a convex polygon on one side of the line u (or v) = k; vertices on the line belong to both sides.
void Clip(const Polygon& thePolygon, bool theU, float theK, bool theKeepBelow, Polygon& theOut)
{
	theOut.clear();
	size_t aCount = thePolygon.size();
	for (size_t i = 0; i < aCount; i++)
	{
		const TriVertex& aCur = thePolygon[i];
		const TriVertex& aNext = thePolygon[(i + 1) % aCount];
		float aCurCoord = Coord(aCur, theU);
		float aNextCoord = Coord(aNext, theU);
		bool aCurIn = theKeepBelow ? aCurCoord <= theK : aCurCoord >= theK;
		bool aNextIn = theKeepBelow ? aNextCoord <= theK : aNextCoord >= theK;
		if (aCurIn)
			theOut.push_back(aCur);
		// an edge with one end strictly on each side crosses the line (an end on the line is a vertex of the piece already)
		if (aCurIn != aNextIn && aCurCoord != theK && aNextCoord != theK)
			theOut.push_back(Cross(aCur, aNext, theU, theK));
	}
}

void Range(const Polygon& thePolygon, bool theU, float& theMin, float& theMax)
{
	theMin = theMax = Coord(thePolygon[0], theU);
	for (size_t i = 1; i < thePolygon.size(); i++)
	{
		float aCoord = Coord(thePolygon[i], theU);
		if (aCoord < theMin)
			theMin = aCoord;
		if (aCoord > theMax)
			theMax = aCoord;
	}
}

// Cuts at the first whole number inside the range of u, then of v; a piece with none is in one cell, moved into 0..1 and
// added as a fan of triangles.
void SplitPolygon(const Polygon& thePolygon, bool theWrapU, bool theWrapV, std::vector<TriVertex>& theOut)
{
	if (thePolygon.size() < 3)
		return;
	float aMin[2], aMax[2];
	for (int anAxis = 0; anAxis < 2; anAxis++)
	{
		bool aU = anAxis == 0;
		Range(thePolygon, aU, aMin[anAxis], aMax[anAxis]);
		if (!(aU ? theWrapU : theWrapV))
			continue;
		float aK = floorf(aMin[anAxis]) + 1.0f;
		if (aK < aMax[anAxis])
		{
			Polygon aLow, aHigh;
			Clip(thePolygon, aU, aK, true, aLow);
			Clip(thePolygon, aU, aK, false, aHigh);
			SplitPolygon(aLow, theWrapU, theWrapV, theOut);
			SplitPolygon(aHigh, theWrapU, theWrapV, theOut);
			return;
		}
	}
	float aCellU = theWrapU ? floorf((aMin[0] + aMax[0]) * 0.5f) : 0.0f;
	float aCellV = theWrapV ? floorf((aMin[1] + aMax[1]) * 0.5f) : 0.0f;
	for (size_t i = 1; i + 1 < thePolygon.size(); i++)
	{
		const TriVertex* aCorners[3] = {&thePolygon[0], &thePolygon[i], &thePolygon[i + 1]};
		for (int j = 0; j < 3; j++)
		{
			TriVertex aVertex = *aCorners[j];
			aVertex.u -= aCellU;
			aVertex.v -= aCellV;
			theOut.push_back(aVertex);
		}
	}
}

} // namespace

void Sexy::SplitWrappedTriangles(const TriVertex theVertices[][3], int theNumTriangles, bool theWrapU, bool theWrapV,
	std::vector<TriVertex>& theOut)
{
	theOut.reserve(theOut.size() + theNumTriangles * 3);
	for (int t = 0; t < theNumTriangles; t++)
	{
		Polygon aTriangle(theVertices[t], theVertices[t] + 3);
		// too wide (or not a number): left as it is, clamped
		float aMin, aMax;
		bool aTooWide = false;
		for (int anAxis = 0; anAxis < 2; anAxis++)
		{
			Range(aTriangle, anAxis == 0, aMin, aMax);
			if ((anAxis == 0 ? theWrapU : theWrapV) && !(aMax - aMin <= (float) MAX_WRAP_CELLS))
				aTooWide = true;
		}
		if (aTooWide)
		{
			theOut.insert(theOut.end(), theVertices[t], theVertices[t] + 3);
			continue;
		}
		SplitPolygon(aTriangle, theWrapU, theWrapV, theOut);
	}
}
