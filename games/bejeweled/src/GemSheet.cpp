// port(variant): more gem colours, the gem sheet's generated rows (GemSheet.h). Not part of the original game.

#include "GemSheet.h"
#include "MemoryImage.h"
#include "platform/ModManager.h"

#include <stb_image_write.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

using namespace Sexy;
using Platform::JsonValue;

namespace
{

// The glyph in the generated rows' corner: centred at (39, 39) of the 49 px gem, which each frame draws from x 2
const double GLYPH_X = 41.0;
const double GLYPH_Y = 39.0;
const double GLYPH_RADIUS = 5.5;
const double GLYPH_OUTLINE = 1.4;
const int GLYPH_SAMPLES = 4;   // per axis, for the edges' coverage
const double PI = 3.14159265358979323846;

enum GlyphShape
{
	GLYPH_CIRCLE,
	GLYPH_SQUARE,
	GLYPH_TRIANGLE,
	GLYPH_DIAMOND,
	GLYPH_STAR,
	GLYPH_CROSS,
	NUM_GLYPHS
};

// The default looks, white gem recoloured: rose, teal, violet, onyx, lime (hue, saturation, lightness)
const double DEFAULT_LOOKS[][3] = {{330, 0.85, 0.50}, {170, 0.80, 0.40}, {265, 0.75, 0.45}, {0, 0.0, 0.05}, {85, 0.85, 0.45}};
const int NUM_DEFAULT_LOOKS = sizeof(DEFAULT_LOOKS) / sizeof(DEFAULT_LOOKS[0]);

double Clamp01(double theValue)
{
	return std::max(0.0, std::min(1.0, theValue));
}

// r, g, b, l and s 0..1, h in degrees
void RgbToHls(double r, double g, double b, double* h, double* l, double* s)
{
	double aMax = std::max(r, std::max(g, b));
	double aMin = std::min(r, std::min(g, b));
	*l = (aMax + aMin) / 2;
	if (aMax == aMin)
	{
		*h = 0;
		*s = 0;
		return;
	}
	double d = aMax - aMin;
	*s = (*l <= 0.5) ? d / (aMax + aMin) : d / (2 - aMax - aMin);
	if (aMax == r)
		*h = (g - b) / d + ((g < b) ? 6 : 0);
	else if (aMax == g)
		*h = (b - r) / d + 2;
	else
		*h = (r - g) / d + 4;
	*h *= 60;
}

double HueChannel(double p, double q, double t)
{
	t = t - std::floor(t);
	if (t < 1.0 / 6)
		return p + (q - p) * 6 * t;
	if (t < 0.5)
		return q;
	if (t < 2.0 / 3)
		return p + (q - p) * (2.0 / 3 - t) * 6;
	return p;
}

void HlsToRgb(double h, double l, double s, double* r, double* g, double* b)
{
	if (s <= 0)
	{
		*r = *g = *b = l;
		return;
	}
	double q = (l < 0.5) ? l * (1 + s) : l + s - l * s;
	double p = 2 * l - q;
	double t = h / 360;
	*r = HueChannel(p, q, t + 1.0 / 3);
	*g = HueChannel(p, q, t);
	*b = HueChannel(p, q, t - 1.0 / 3);
}

uint32_t Recolour(uint32_t thePixel, const GemSheet::RowLook& theLook)
{
	uint32_t anAlpha = thePixel & 0xFF000000;
	if (anAlpha == 0)
		return 0;
	double r = ((thePixel >> 16) & 0xFF) / 255.0;
	double g = ((thePixel >> 8) & 0xFF) / 255.0;
	double b = (thePixel & 0xFF) / 255.0;
	double h, l, s;
	if (theLook.mTinted)
	{
		// The base's shading as lightness, darkened so the white gem's bright body takes the colour; its highlights stay
		double aLuminance = 0.299 * r + 0.587 * g + 0.114 * b;
		h = theLook.mTintHue;
		s = theLook.mTintSaturation;
		l = Clamp01(std::pow(aLuminance, 1.6) + theLook.mTintLightness - 0.5);
	}
	else
		RgbToHls(r, g, b, &h, &l, &s);
	h = std::fmod(h + theLook.mHue, 360.0);
	if (h < 0)
		h += 360;
	s = Clamp01(s * theLook.mSaturation);
	l = Clamp01(l + theLook.mLightness);
	HlsToRgb(h, l, s, &r, &g, &b);
	return anAlpha | ((uint32_t) (r * 255 + 0.5) << 16) | ((uint32_t) (g * 255 + 0.5) << 8) | (uint32_t) (b * 255 + 0.5);
}

// x and y relative to the glyph's centre, in radii
bool InGlyph(int theShape, double x, double y)
{
	switch (theShape)
	{
	case GLYPH_CIRCLE:
		return x * x + y * y <= 1;
	case GLYPH_SQUARE:
		return (std::fabs(x) <= 0.8) && (std::fabs(y) <= 0.8);
	case GLYPH_TRIANGLE:
		return (y >= -1) && (y <= 0.7) && (std::fabs(x) <= (y + 1) * 0.62);
	case GLYPH_DIAMOND:
		return std::fabs(x) + std::fabs(y) <= 1;
	case GLYPH_STAR:
	{
		double aPoint = (std::cos(5 * (std::atan2(y, x) + PI / 2)) + 1) / 2;
		return std::sqrt(x * x + y * y) <= 0.45 + 0.55 * aPoint;
	}
	default:
		return ((std::fabs(x) <= 0.33) && (std::fabs(y) <= 1)) || ((std::fabs(y) <= 0.33) && (std::fabs(x) <= 1));
	}
}

// theColor over thePixel at theCoverage (straight alpha)
uint32_t Over(uint32_t thePixel, uint32_t theColor, double theCoverage)
{
	double aDestAlpha = (thePixel >> 24) / 255.0;
	double anAlpha = theCoverage + aDestAlpha * (1 - theCoverage);
	if (anAlpha <= 0)
		return 0;
	uint32_t aResult = (uint32_t) (anAlpha * 255 + 0.5) << 24;
	for (int aShift = 0; aShift <= 16; aShift += 8)
	{
		double aSource = (theColor >> aShift) & 0xFF;
		double aDest = (thePixel >> aShift) & 0xFF;
		double aMix = (aSource * theCoverage + aDest * aDestAlpha * (1 - theCoverage)) / anAlpha;
		aResult |= (uint32_t) std::min(255.0, aMix + 0.5) << aShift;
	}
	return aResult;
}

// A white glyph with a dark outline centred at (theX, theY) of theBits (theWidth wide, theHeight tall), at theScale
void DrawGlyph(uint32_t* theBits, int theWidth, int theHeight, double theX, double theY, int theScale, int theShape)
{
	double aRadius = GLYPH_RADIUS * theScale;
	double anOuter = aRadius + GLYPH_OUTLINE * theScale;
	int aLeft = std::max(0, (int) std::floor(theX - anOuter) - 1);
	int aRight = std::min(theWidth, (int) std::ceil(theX + anOuter) + 1);
	int aTop = std::max(0, (int) std::floor(theY - anOuter) - 1);
	int aBottom = std::min(theHeight, (int) std::ceil(theY + anOuter) + 1);
	for (int py = aTop; py < aBottom; py++)
	{
		for (int px = aLeft; px < aRight; px++)
		{
			int anInside = 0;
			int anOutline = 0;
			for (int sy = 0; sy < GLYPH_SAMPLES; sy++)
			{
				for (int sx = 0; sx < GLYPH_SAMPLES; sx++)
				{
					double x = px + (sx + 0.5) / GLYPH_SAMPLES - theX;
					double y = py + (sy + 0.5) / GLYPH_SAMPLES - theY;
					if (InGlyph(theShape, x / aRadius, y / aRadius))
						anInside++;
					if (InGlyph(theShape, x / anOuter, y / anOuter))
						anOutline++;
				}
			}
			if (anOutline == 0)
				continue;
			const double aSamples = GLYPH_SAMPLES * GLYPH_SAMPLES;
			uint32_t& aPixel = theBits[py * theWidth + px];
			aPixel = Over(aPixel, 0x14141E, anOutline / aSamples);
			aPixel = Over(aPixel, 0xFFFFFF, anInside / aSamples);
		}
	}
}

// Row theRow of theBits (theWidth wide, rows of ROW_HEIGHT * theScale) made from row theLook.mFrom
void MakeRow(uint32_t* theBits, int theWidth, int theScale, int theRow, const GemSheet::RowLook& theLook, int theGlyph)
{
	int aRowPixels = GemSheet::ROW_HEIGHT * theScale * theWidth;
	const uint32_t* aSource = theBits + theLook.mFrom * aRowPixels;
	uint32_t* aDest = theBits + theRow * aRowPixels;
	for (int i = 0; i < aRowPixels; i++)
		aDest[i] = Recolour(aSource[i], theLook);
	if (!theLook.mGlyph)
		return;
	int aFrames = theWidth / (GemSheet::FRAME_WIDTH * theScale);
	for (int aFrame = 0; aFrame < aFrames; aFrame++)
	{
		for (int aSubRow = 0; aSubRow < 2; aSubRow++)
		{
			double x = (aFrame * GemSheet::FRAME_WIDTH + GLYPH_X) * theScale;
			double y = (aSubRow * GemSheet::SUB_ROW_HEIGHT + GLYPH_Y) * theScale;
			DrawGlyph(aDest, theWidth, GemSheet::ROW_HEIGHT * theScale, x, y, theScale, theGlyph);
		}
	}
}

bool IsGemSheet(const std::string& theTarget)
{
	std::string aName;
	for (size_t i = 0; i < theTarget.size(); i++)
		aName += (theTarget[i] == '\\') ? '/' : (char) tolower((unsigned char) theTarget[i]);
	return (aName == "images/gemsheet6") || (aName == "images/gemsheet6.png");
}

void Warn(std::vector<RulesMessage>* theWarnings, const std::string& theMod, const std::string& theText)
{
	if (theWarnings != NULL)
		theWarnings->push_back({theMod, theText});
}

// Entry theEntryIndex of an extendRows op's "generate", the look of generated row theIndex
GemSheet::RowLook ReadLook(const JsonValue& theEntry, const std::string& theMod, bool theGlyph, int theIndex, int theEntryIndex,
	std::vector<RulesMessage>* theWarnings)
{
	GemSheet::RowLook aLook = GemSheet::DefaultLook(theIndex);
	aLook.mModId = theMod;
	aLook.mGlyph = theGlyph;
	std::string aWhere = "imageOps: generate[" + std::to_string(theEntryIndex) + "]";
	if (!theEntry.IsObject())
	{
		Warn(theWarnings, theMod, aWhere + " is not an object; that row gets the default look");
		return aLook;
	}
	aLook = GemSheet::RowLook();
	aLook.mModId = theMod;
	aLook.mGlyph = theEntry["glyph"].IsBool() ? theEntry["glyph"].AsBool() : theGlyph;
	if (theEntry.Has("from"))
	{
		const JsonValue& aFrom = theEntry["from"];
		if (aFrom.IsNumber() && (aFrom.AsNumber() >= 0) && (aFrom.AsNumber() < GemSheet::MAX_ROWS))
			aLook.mFrom = aFrom.AsInt();
		else
			Warn(theWarnings, theMod, aWhere + ".from: a row 0.." + std::to_string(GemSheet::MAX_ROWS - 1) + "; row 1 is used");
	}
	if (theEntry.Has("tint"))
	{
		const JsonValue& aTint = theEntry["tint"];
		bool isColor = aTint.IsArray() && (aTint.Size() == 3);
		for (size_t i = 0; isColor && (i < 3); i++)
			isColor = aTint[i].IsNumber() && (aTint[i].AsNumber() >= 0) && (aTint[i].AsNumber() <= 255);
		if (isColor)
		{
			aLook.mTinted = true;
			RgbToHls(aTint[0].AsNumber() / 255, aTint[1].AsNumber() / 255, aTint[2].AsNumber() / 255,
				&aLook.mTintHue, &aLook.mTintLightness, &aLook.mTintSaturation);
		}
		else
			Warn(theWarnings, theMod, aWhere + ".tint: [red, green, blue], each 0..255; ignored");
	}
	if (theEntry.Has("hue"))
	{
		if (theEntry["hue"].IsNumber())
			aLook.mHue = theEntry["hue"].AsNumber();
		else
			Warn(theWarnings, theMod, aWhere + ".hue: degrees; ignored");
	}
	if (theEntry.Has("saturation"))
	{
		if (theEntry["saturation"].IsNumber() && (theEntry["saturation"].AsNumber() >= 0))
			aLook.mSaturation = theEntry["saturation"].AsNumber();
		else
			Warn(theWarnings, theMod, aWhere + ".saturation: a factor of 0 or more; ignored");
	}
	if (theEntry.Has("lightness"))
	{
		if (theEntry["lightness"].IsNumber() && (std::fabs(theEntry["lightness"].AsNumber()) <= 1))
			aLook.mLightness = theEntry["lightness"].AsNumber();
		else
			Warn(theWarnings, theMod, aWhere + ".lightness: -1..1; ignored");
	}
	return aLook;
}

}

std::vector<GemSheet::RowLook> GemSheet::ModLooks(const std::vector<Platform::ModImageOp>& theOps,
	std::vector<RulesMessage>* theWarnings)
{
	std::vector<RowLook> aLooks;
	for (size_t i = 0; i < theOps.size(); i++)
	{
		const JsonValue& anOp = theOps[i].mOp;
		const std::string& aMod = theOps[i].mModId;
		if (!anOp.IsObject() || !anOp["target"].IsString() || !anOp["op"].IsString())
		{
			Warn(theWarnings, aMod, "imageOps: each entry needs \"target\" and \"op\" strings");
			continue;
		}
		if (anOp["op"].AsString() != "extendRows")
		{
			Warn(theWarnings, aMod, "imageOps: unknown op \"" + anOp["op"].AsString() + "\" (did you mean extendRows?)");
			continue;
		}
		if (!IsGemSheet(anOp["target"].AsString()))
		{
			Warn(theWarnings, aMod, "imageOps: extendRows extends images/gemsheet6 only, not \"" + anOp["target"].AsString() + "\"");
			continue;
		}
		if (anOp.Has("rowHeight") && (anOp["rowHeight"].AsNumber(-1) != ROW_HEIGHT))
		{
			Warn(theWarnings, aMod, "imageOps: images/gemsheet6 has rows of " + std::to_string((int) ROW_HEIGHT) + " px (rowHeight); op ignored");
			continue;
		}
		if (anOp.Has("rows") && (!anOp["rows"].IsNumber() || (anOp["rows"].AsNumber() < 1) || (anOp["rows"].AsNumber() > MAX_ROWS)))
			Warn(theWarnings, aMod, "imageOps: rows: the sheet has at most " + std::to_string((int) MAX_ROWS) + " (as board.gemColors)");
		bool aGlyph = anOp["glyph"].IsBool() ? anOp["glyph"].AsBool() : true;
		const JsonValue& aGenerate = anOp["generate"];
		if (anOp.Has("generate") && !aGenerate.IsArray())
			Warn(theWarnings, aMod, "imageOps: generate: an array of rows; ignored");
		for (size_t e = 0; e < aGenerate.Size() && aGenerate.IsArray(); e++)
			aLooks.push_back(ReadLook(aGenerate[e], aMod, aGlyph, (int) aLooks.size(), (int) e, theWarnings));
	}
	return aLooks;
}

GemSheet::RowLook GemSheet::DefaultLook(int theIndex)
{
	const double* aLook = DEFAULT_LOOKS[std::max(0, theIndex) % NUM_DEFAULT_LOOKS];
	RowLook aResult;
	aResult.mTinted = true;
	aResult.mTintHue = aLook[0];
	aResult.mTintSaturation = aLook[1];
	aResult.mTintLightness = aLook[2];
	return aResult;
}

int GemSheet::Rows(const MemoryImage* theSheet)
{
	return theSheet->mHeight / ROW_HEIGHT;
}

void GemSheet::Extend(MemoryImage* theSheet, int theRows, const std::vector<RowLook>& theLooks)
{
	int aHave = Rows(theSheet);
	theRows = std::min(theRows, (int) MAX_ROWS);
	if ((aHave < 1) || (aHave >= theRows))
		return;

	// The HD pixels follow when they match the 1x ones (a mod's @2x art); otherwise the sheet is drawn from its 1x pixels
	int aWidth = theSheet->mWidth;
	int aScale = theSheet->HasMirrorableHDBits() ? theSheet->mHDScale : 1;
	std::vector<uint32_t> aBits((size_t) aWidth * theRows * ROW_HEIGHT, 0);
	memcpy(&aBits[0], theSheet->GetBits(), (size_t) aWidth * aHave * ROW_HEIGHT * sizeof(uint32_t));
	std::vector<uint32_t> anHD;
	if (aScale > 1)
	{
		anHD.assign((size_t) aWidth * aScale * theRows * ROW_HEIGHT * aScale, 0);
		memcpy(&anHD[0], &theSheet->mHDBits[0], (size_t) aWidth * aScale * aHave * ROW_HEIGHT * aScale * sizeof(uint32_t));
	}

	for (int aRow = aHave; aRow < theRows; aRow++)
	{
		int anIndex = aRow - aHave;
		RowLook aLook = (anIndex < (int) theLooks.size()) ? theLooks[anIndex] : DefaultLook(aRow - ORIGINAL_ROWS);
		if ((aLook.mFrom < 0) || (aLook.mFrom >= aRow))
			aLook.mFrom = std::min(1, aRow - 1);
		int aGlyph = std::max(0, aRow - ORIGINAL_ROWS) % NUM_GLYPHS;
		MakeRow(&aBits[0], aWidth, 1, aRow, aLook, aGlyph);
		if (aScale > 1)
			MakeRow(&anHD[0], aWidth * aScale, aScale, aRow, aLook, aGlyph);
	}

	theSheet->SetBits((ulong*) &aBits[0], aWidth, theRows * ROW_HEIGHT, true);
	if (aScale > 1)
		theSheet->SetHDBits(&anHD[0], aWidth * aScale, theRows * ROW_HEIGHT * aScale, aScale);
}

static bool WritePng(const uint32_t* theBits, int theWidth, int theHeight, const std::string& theFileName)
{
	std::vector<uint8_t> aRGBA((size_t) theWidth * theHeight * 4);
	for (size_t i = 0; i < (size_t) theWidth * theHeight; i++)
	{
		aRGBA[i * 4] = (uint8_t) (theBits[i] >> 16);
		aRGBA[i * 4 + 1] = (uint8_t) (theBits[i] >> 8);
		aRGBA[i * 4 + 2] = (uint8_t) theBits[i];
		aRGBA[i * 4 + 3] = (uint8_t) (theBits[i] >> 24);
	}
	return stbi_write_png(theFileName.c_str(), theWidth, theHeight, 4, &aRGBA[0], theWidth * 4) != 0;
}

bool GemSheet::WritePngs(MemoryImage* theSheet, const std::string& theFileName)
{
	if (!WritePng((const uint32_t*) theSheet->GetBits(), theSheet->mWidth, theSheet->mHeight, theFileName))
		return false;
	if (!theSheet->HasValidHDBits())
		return true;
	std::string aName = theFileName;
	size_t aDot = aName.rfind('.');
	if ((aDot != std::string::npos) && (aName.find_first_of("/\\", aDot) == std::string::npos))
		aName.erase(aDot);
	aName += "@" + std::to_string(theSheet->mHDScale) + "x.png";
	return WritePng(&theSheet->mHDBits[0], theSheet->mHDWidth, theSheet->mHDHeight, aName);
}
