#include "VectorFont.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <mutex>
#include <sstream>

// port(variant): code-drawn art (VectorFont.h)

namespace Art
{

namespace
{

// Skeletons, y up, baseline 0, capitals 10 high. Commands: M x y (new stroke), L x y (line), A cx cy rx ry a0 a1
// (elliptic arc from angle a0 to a1 in degrees, counter-clockwise for a1 > a0; it continues the stroke when it starts
// within 0.8 units of the stroke's end), D x y (a dot). w is the skeleton's width, the advance before the weight and
// the tracking.
struct GlyphDef
{
	int mChar;
	double mWidth;
	const char* mCommands;
};

const GlyphDef kGlyphs[] =
{
	{' ', 3.2, ""},
	{'!', 0, "M 0 10 L 0 3.2 D 0 0"},
	{'"', 2.6, "M 0 10 L 0 7 M 2.6 10 L 2.6 7"},
	{'#', 7.5, "M 2.6 10 L 1.6 0 M 6 10 L 5 0 M 0.2 6.6 L 7.5 6.6 M 0 3.2 L 7.3 3.2"},
	{'$', 7, "A 3.5 7.4 3.3 2.4 25 270 A 3.5 2.6 3.5 2.6 90 -155 M 3.5 11 L 3.5 -1"},
	{'%', 8, "A 1.8 8 1.8 2 0 360 A 6.2 2 1.8 2 0 360 M 7.2 10 L 0.8 0"},
	{'&', 7.6, "M 7.5 0 L 2.2 6.2 A 3.4 8.2 1.6 1.7 230 -40 L 1.6 3.4 A 3.4 2.2 2.2 2.2 145 315 L 7.6 4.6"},
	{'\'', 0, "M 0 10 L 0 7"},
	{'(', 2.6, "A 4.6 4 4.6 6.6 115 245"},
	{')', 2.6, "A -2 4 4.6 6.6 65 -65"},
	{'*', 5, "M 2.5 10 L 2.5 5.2 M 0.2 8.8 L 4.8 6.4 M 0.2 6.4 L 4.8 8.8"},
	{'+', 6, "M 3 7 L 3 1 M 0 4 L 6 4"},
	{',', 1, "M 1 0.6 L 0 -2"},
	{'-', 4, "M 0 4 L 4 4"},
	{'.', 0, "D 0 0"},
	{'/', 5, "M 5 10 L 0 -0.5"},
	{'0', 7.6, "A 3.8 5 3.8 5 0 360"},
	{'1', 2.6, "M 0 8 L 2.6 10 L 2.6 0"},
	{'2', 7.5, "A 3.7 6.9 3.6 3.1 155 -35 L 0 0 L 7.5 0"},
	{'3', 7.4, "A 3.6 7.6 3.4 2.4 150 -90 A 3.6 2.6 3.8 2.6 90 -150"},
	{'4', 8, "M 6 0 L 6 10 L 0 2.8 L 8 2.8"},
	{'5', 7.4, "M 6.8 10 L 1.2 10 L 0.6 5.5 A 3.7 3.3 3.7 3.3 135 -150"},
	{'6', 7.6, "A 3.8 3.3 3.8 3.3 0 360 M 1.11 5.63 L 5.4 10"},
	{'7', 7.6, "M 0 10 L 7.6 10 L 2.6 0"},
	{'8', 7.6, "A 3.8 7.6 3.2 2.4 0 360 A 3.8 2.6 3.8 2.6 0 360"},
	{'9', 7.6, "A 3.8 6.7 3.8 3.3 0 360 M 6.49 4.37 L 2.2 0"},
	{':', 0, "D 0 6.6 D 0 0"},
	{';', 1, "D 1 6.6 M 1 0.6 L 0 -2"},
	{'<', 6, "M 6 7.2 L 0 4 L 6 0.8"},
	{'=', 6, "M 0 5.6 L 6 5.6 M 0 2.4 L 6 2.4"},
	{'>', 6, "M 0 7.2 L 6 4 L 0 0.8"},
	{'?', 6.6, "A 3.3 7.3 3.3 2.7 160 -60 L 3.3 3 D 3.3 0"},
	{'@', 9.6, "A 5 4.4 2 2.2 0 360 M 7 6.6 L 7 3.4 A 8 3.4 1 1.2 180 360 L 9.6 4.4 A 5 4.4 4.6 5.2 0 300"},
	{'A', 8, "M 0 0 L 4 10 L 8 0 M 1.5 3.6 L 6.5 3.6"},
	{'B', 7, "M 4.2 5.2 L 0 5.2 L 0 10 L 4 10 A 4 7.6 2.4 2.4 90 -90 M 0 5.2 L 0 0 L 4.4 0 A 4.4 2.6 2.6 2.6 -90 90 L 4 5.2"},
	{'C', 8.1, "A 4.9 5 4.9 5 50 310"},
	{'D', 7.8, "M 0 0 L 0 10 L 3 10 A 3 5 4.8 5 90 -90 L 0 0"},
	{'E', 6.6, "M 6.6 10 L 0 10 L 0 0 L 6.6 0 M 0 5 L 5.4 5"},
	{'F', 6.4, "M 6.4 10 L 0 10 L 0 0 M 0 5 L 5.2 5"},
	{'G', 9.8, "A 4.9 5 4.9 5 50 360 L 5.4 5"},
	{'H', 7.6, "M 0 0 L 0 10 M 7.6 0 L 7.6 10 M 0 5 L 7.6 5"},
	{'I', 0, "M 0 0 L 0 10"},
	{'J', 5.6, "M 5.6 10 L 5.6 2.9 A 2.8 2.9 2.8 2.9 0 -180"},
	{'K', 7.2, "M 0 0 L 0 10 M 7 10 L 0 3.4 M 2.4 5.6 L 7.2 0"},
	{'L', 6.2, "M 0 10 L 0 0 L 6.2 0"},
	{'M', 9.6, "M 0 0 L 0.4 10 L 4.8 2.2 L 9.2 10 L 9.6 0"},
	{'N', 7.6, "M 0 0 L 0 10 L 7.6 0 L 7.6 10"},
	{'O', 10, "A 5 5 5 5 0 360"},
	{'P', 6.8, "M 0 0 L 0 10 L 4 10 A 4 7.4 2.8 2.6 90 -90 L 0 4.8"},
	{'Q', 10, "A 5 5 5 5 0 360 M 6.2 3 L 10 -0.6"},
	{'R', 7.2, "M 0 0 L 0 10 L 4 10 A 4 7.4 2.8 2.6 90 -90 L 0 4.8 M 3.6 4.8 L 7.2 0"},
	{'S', 7, "A 3.5 7.4 3.3 2.6 25 270 A 3.5 2.6 3.5 2.6 90 -155"},
	{'T', 8, "M 0 10 L 8 10 M 4 10 L 4 0"},
	{'U', 7.6, "M 0 10 L 0 3.8 A 3.8 3.8 3.8 3.8 180 360 L 7.6 10"},
	{'V', 8, "M 0 10 L 4 0 L 8 10"},
	{'W', 11.6, "M 0 10 L 2.9 0 L 5.8 8.4 L 8.7 0 L 11.6 10"},
	{'X', 7.6, "M 0 10 L 7.6 0 M 0 0 L 7.6 10"},
	{'Y', 8, "M 0 10 L 4 5 L 8 10 M 4 5 L 4 0"},
	{'Z', 7.4, "M 0 10 L 7.4 10 L 0 0 L 7.4 0"},
	{'[', 2.6, "M 2.6 11 L 0 11 L 0 -1.6 L 2.6 -1.6"},
	{'\\', 5, "M 0 10 L 5 -0.5"},
	{']', 2.6, "M 0 11 L 2.6 11 L 2.6 -1.6 L 0 -1.6"},
	{'^', 5.6, "M 0 6.8 L 2.8 10 L 5.6 6.8"},
	{'_', 7, "M 0 -1.6 L 7 -1.6"},
	{'`', 1.6, "M 0 10.4 L 1.6 8.8"},
	{'a', 6.8, "A 3.4 3.5 3.4 3.5 0 360 M 6.8 7 L 6.8 0"},
	{'b', 6.8, "M 0 10 L 0 0 A 3.4 3.5 3.4 3.5 0 360"},
	{'c', 6, "A 3.5 3.5 3.5 3.5 45 315"},
	{'d', 6.8, "A 3.4 3.5 3.4 3.5 0 360 M 6.8 10 L 6.8 0"},
	{'e', 6.8, "M 0.1 3.5 L 6.8 3.5 A 3.4 3.5 3.4 3.5 0 315"},
	{'f', 4.4, "M 4.4 9.8 A 3.6 8.4 1.6 1.6 60 180 L 2 0 M 0 7 L 4.2 7"},
	{'g', 6.8, "A 3.4 3.5 3.4 3.5 0 360 M 6.8 7 L 6.8 -0.8 A 3.4 -0.8 3.4 2.2 0 -155"},
	{'h', 6.6, "M 0 10 L 0 0 M 0 3.8 A 3.3 3.8 3.3 3.2 180 0 L 6.6 0"},
	{'i', 0, "M 0 0 L 0 7 D 0 9.6"},
	{'j', 2.6, "M 2.6 7 L 2.6 -1.2 A 1 -1.2 1.6 1.8 0 -140 D 2.6 9.6"},
	{'k', 6.2, "M 0 10 L 0 0 M 5.8 7 L 0 2.6 M 2 4.1 L 6.2 0"},
	{'l', 0, "M 0 10 L 0 0"},
	{'m', 10.4, "M 0 0 L 0 7 M 0 4.2 A 2.6 4.2 2.6 2.8 180 0 L 5.2 0 M 5.2 4.2 A 7.8 4.2 2.6 2.8 180 0 L 10.4 0"},
	{'n', 6.6, "M 0 0 L 0 7 M 0 3.8 A 3.3 3.8 3.3 3.2 180 0 L 6.6 0"},
	{'o', 7, "A 3.5 3.5 3.5 3.5 0 360"},
	{'p', 6.8, "M 0 7 L 0 -3 A 3.4 3.5 3.4 3.5 0 360"},
	{'q', 6.8, "A 3.4 3.5 3.4 3.5 0 360 M 6.8 7 L 6.8 -3"},
	{'r', 4.8, "M 0 0 L 0 7 M 0 3.6 A 3.4 3.6 3.4 3.4 180 60"},
	{'s', 5.8, "A 2.9 5.3 2.7 1.7 25 270 A 2.9 1.8 2.9 1.8 90 -155"},
	{'t', 4.6, "M 1.8 9.4 L 1.8 1.6 A 3.4 1.6 1.6 1.6 180 270 L 4.6 0 M 0 7 L 4.2 7"},
	{'u', 6.6, "M 0 7 L 0 3.2 A 3.3 3.2 3.3 3.2 180 360 M 6.6 7 L 6.6 0"},
	{'v', 6.4, "M 0 7 L 3.2 0 L 6.4 7"},
	{'w', 9.8, "M 0 7 L 2.45 0 L 4.9 5.6 L 7.35 0 L 9.8 7"},
	{'x', 6.2, "M 0 7 L 6.2 0 M 0 0 L 6.2 7"},
	{'y', 6.4, "M 0 7 L 3.3 0 M 6.4 7 L 1.6 -3"},
	{'z', 6, "M 0 7 L 6 7 L 0 0 L 6 0"},
	{'{', 3, "M 3 11 L 2 10.5 L 1.6 6 L 0 4.7 L 1.6 3.4 L 2 -1.1 L 3 -1.6"},
	{'|', 0, "M 0 11 L 0 -2"},
	{'}', 3, "M 0 11 L 1 10.5 L 1.4 6 L 3 4.7 L 1.4 3.4 L 1 -1.1 L 0 -1.6"},
	{'~', 6.6, "M 0 3.6 L 1.3 4.9 L 2.6 4.9 L 4 3.6 L 5.3 3.6 L 6.6 4.9"},
};

struct Glyph
{
	double mWidth = 0;
	std::vector<std::vector<Vec2> > mStrokes;   // units, y up
	double mY0 = 0, mY1 = 0, mX0 = 0, mX1 = 0;    // skeleton extent
};

std::map<int, Glyph> gGlyphs;
std::once_flag gGlyphsOnce;

void BuildGlyphs()
{
	const double kPi = 3.14159265358979323846;
	for (const GlyphDef& aDef : kGlyphs)
	{
		Glyph aGlyph;
		aGlyph.mWidth = aDef.mWidth;
		std::istringstream aStream(aDef.mCommands);
		std::string aCmd;
		while (aStream >> aCmd)
		{
			if ((aCmd == "M") || (aCmd == "L") || (aCmd == "D"))
			{
				double x, y;
				aStream >> x >> y;
				if ((aCmd != "L") || aGlyph.mStrokes.empty())
					aGlyph.mStrokes.push_back(std::vector<Vec2>());
				aGlyph.mStrokes.back().push_back(Vec2(x, y));
			}
			else if (aCmd == "A")
			{
				double cx, cy, rx, ry, a0, a1;
				aStream >> cx >> cy >> rx >> ry >> a0 >> a1;
				int n = std::max(2, (int) std::ceil(std::fabs(a1 - a0) / 6.0));
				Vec2 aStart(cx + rx * std::cos(a0 * kPi / 180), cy + ry * std::sin(a0 * kPi / 180));
				bool isJoined = !aGlyph.mStrokes.empty() && !aGlyph.mStrokes.back().empty() &&
					(std::hypot(aGlyph.mStrokes.back().back().x - aStart.x, aGlyph.mStrokes.back().back().y - aStart.y) < 0.8);
				if (!isJoined)
					aGlyph.mStrokes.push_back(std::vector<Vec2>());
				for (int i = isJoined ? 1 : 0; i <= n; i++)
				{
					double a = (a0 + (a1 - a0) * i / n) * kPi / 180;
					aGlyph.mStrokes.back().push_back(Vec2(cx + rx * std::cos(a), cy + ry * std::sin(a)));
				}
			}
		}
		bool isFirst = true;
		for (const std::vector<Vec2>& aStroke : aGlyph.mStrokes)
		{
			for (const Vec2& p : aStroke)
			{
				if (isFirst)
				{
					aGlyph.mX0 = aGlyph.mX1 = p.x;
					aGlyph.mY0 = aGlyph.mY1 = p.y;
					isFirst = false;
				}
				aGlyph.mX0 = std::min(aGlyph.mX0, p.x); aGlyph.mX1 = std::max(aGlyph.mX1, p.x);
				aGlyph.mY0 = std::min(aGlyph.mY0, p.y); aGlyph.mY1 = std::max(aGlyph.mY1, p.y);
			}
		}
		gGlyphs[aDef.mChar] = aGlyph;
	}
}

// Latin-1 letters by their ASCII base (accents are not drawn)
int AsciiFor(int theChar)
{
	static const char* kBase =
		"AAAAAAACEEEEIIII" "DNOOOOOxOUUUUYPs"     // 0xC0..0xDF
		"aaaaaaaceeeeiiii" "dnooooo/ouuuuypy";    // 0xE0..0xFF
	if ((theChar >= 0xC0) && (theChar <= 0xFF))
		return kBase[theChar - 0xC0];
	return theChar;
}

const Glyph* GlyphFor(int theChar, const TextStyle& theStyle)
{
	std::call_once(gGlyphsOnce, BuildGlyphs);
	theChar &= 0xFF;
	if (theStyle.mCaps && (theChar >= 'a') && (theChar <= 'z'))
		theChar -= 'a' - 'A';
	std::map<int, Glyph>::const_iterator it = gGlyphs.find(theChar);
	if (it == gGlyphs.end())
		it = gGlyphs.find(AsciiFor(theChar));
	if (it == gGlyphs.end())
		it = gGlyphs.find('?');
	return &it->second;
}

}

double CharAdvance(int theChar, const TextStyle& theStyle)
{
	const Glyph* aGlyph = GlyphFor(theChar, theStyle);
	return (aGlyph->mWidth * theStyle.mWiden + theStyle.mTracking + theStyle.mWeight) * theStyle.Unit();
}

double TextWidth(const std::string& theText, const TextStyle& theStyle)
{
	double w = 0;
	for (size_t i = 0; i < theText.size(); i++)
		w += CharAdvance((unsigned char) theText[i], theStyle);
	return w;
}

void DrawChar(Mask& theMask, int theChar, double x, double y, const TextStyle& theStyle)
{
	const Glyph* aGlyph = GlyphFor(theChar, theStyle);
	double u = theStyle.Unit();
	double aLeft = x + (theStyle.mTracking / 2 + theStyle.mWeight / 2) * u;
	for (const std::vector<Vec2>& aStroke : aGlyph->mStrokes)
	{
		std::vector<Vec2> aLine;
		for (const Vec2& p : aStroke)
			aLine.push_back(Vec2(aLeft + (p.x * theStyle.mWiden + p.y * theStyle.mSlant) * u, y - p.y * u));
		theMask.Stroke(aLine, theStyle.mWeight * u, false);
	}
}

double DrawText(Mask& theMask, const std::string& theText, double x, double y, const TextStyle& theStyle)
{
	for (size_t i = 0; i < theText.size(); i++)
	{
		int c = (unsigned char) theText[i];
		DrawChar(theMask, c, x, y, theStyle);
		x += CharAdvance(c, theStyle);
	}
	return x;
}

}
