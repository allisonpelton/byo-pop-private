// port(variant): code-drawn art (CodeArt.h): the settings, the palettes, the shared drawing helpers and the five fonts.
// The other images are drawn in CodeArtImages.cpp.

#include "CodeArtDraw.h"

#include "GameRules.h"
#include "platform/VFS.h"

#include <stb_image_write.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>

using namespace Art;

namespace CodeArt
{

namespace
{

bool gEnabled = false;
Settings gSettings;
const Palette* gPalette = NULL;
GemStyle gGemStyle = GEMS_CEL;
std::vector<std::string> gModFonts;   // fonts a mod supplies (their image or descriptor): left to the files

Palette MakePalette(Rgba thePanel, Rgba theDark, Rgba theLight, Rgba theInk, Rgba theGlow, Rgba theWell, Rgba theBackTop,
	Rgba theBackBottom, Rgba theLogoTop, Rgba theLogoBottom, Rgba theLogoGlow, Rgba theBar, Rgba theOrb, Rgba theLampLeft,
	Rgba theLampRight, Rgba theVortex)
{
	Palette aPalette;
	aPalette.mPanel = thePanel;
	aPalette.mPanelDark = theDark;
	aPalette.mPanelLight = theLight;
	aPalette.mInk = theInk;
	aPalette.mGlow = theGlow;
	aPalette.mWell = theWell;
	aPalette.mBackTop = theBackTop;
	aPalette.mBackBottom = theBackBottom;
	aPalette.mLogoTop = theLogoTop;
	aPalette.mLogoBottom = theLogoBottom;
	aPalette.mLogoGlow = theLogoGlow;
	aPalette.mBar = theBar;
	aPalette.mOrb = theOrb;
	aPalette.mLampLeft = theLampLeft;
	aPalette.mLampRight = theLampRight;
	aPalette.mVortex = theVortex;
	return aPalette;
}

Rgba MakeRgb(int r, int g, int b)
{
	return Rgba::Bytes(r, g, b);
}

struct NamedPalette
{
	const char* mName;
	Palette mPalette;
};

const std::vector<NamedPalette>& Palettes()
{
	static const std::vector<NamedPalette> sPalettes = {
		{"blue", MakePalette(MakeRgb(44, 92, 158), MakeRgb(14, 32, 70), MakeRgb(140, 190, 240), MakeRgb(225, 240, 255), MakeRgb(255, 206, 84), MakeRgb(10, 22, 48),
			MakeRgb(40, 70, 120), MakeRgb(6, 12, 30), MakeRgb(255, 238, 160), MakeRgb(236, 124, 34), MakeRgb(255, 170, 60), MakeRgb(110, 220, 90), MakeRgb(150, 90, 230),
			MakeRgb(230, 70, 60), MakeRgb(150, 235, 120), MakeRgb(60, 170, 255))},
		{"violet", MakePalette(MakeRgb(96, 64, 150), MakeRgb(34, 18, 64), MakeRgb(196, 170, 240), MakeRgb(245, 236, 255), MakeRgb(120, 230, 255), MakeRgb(24, 12, 44),
			MakeRgb(80, 50, 120), MakeRgb(14, 6, 28), MakeRgb(210, 250, 255), MakeRgb(90, 150, 255), MakeRgb(120, 200, 255), MakeRgb(240, 120, 220), MakeRgb(60, 200, 230),
			MakeRgb(240, 90, 140), MakeRgb(160, 240, 230), MakeRgb(210, 110, 255))},
		{"emerald", MakePalette(MakeRgb(30, 118, 92), MakeRgb(8, 46, 36), MakeRgb(150, 220, 190), MakeRgb(230, 255, 244), MakeRgb(255, 220, 110), MakeRgb(6, 30, 24),
			MakeRgb(24, 90, 72), MakeRgb(4, 22, 18), MakeRgb(255, 245, 180), MakeRgb(220, 160, 40), MakeRgb(255, 210, 90), MakeRgb(120, 230, 140), MakeRgb(240, 100, 120),
			MakeRgb(240, 90, 70), MakeRgb(190, 250, 150), MakeRgb(80, 240, 180))},
		{"ember", MakePalette(MakeRgb(150, 62, 36), MakeRgb(60, 18, 8), MakeRgb(240, 170, 120), MakeRgb(255, 240, 224), MakeRgb(255, 230, 120), MakeRgb(40, 10, 4),
			MakeRgb(120, 46, 24), MakeRgb(26, 6, 2), MakeRgb(255, 250, 200), MakeRgb(255, 150, 40), MakeRgb(255, 120, 40), MakeRgb(255, 190, 60), MakeRgb(255, 150, 50),
			MakeRgb(255, 80, 40), MakeRgb(255, 230, 120), MakeRgb(255, 130, 40))},
		{"night", MakePalette(MakeRgb(46, 52, 68), MakeRgb(14, 16, 24), MakeRgb(150, 160, 186), MakeRgb(220, 228, 240), MakeRgb(140, 200, 255), MakeRgb(8, 10, 16),
			MakeRgb(34, 38, 54), MakeRgb(4, 5, 10), MakeRgb(240, 244, 255), MakeRgb(140, 160, 200), MakeRgb(150, 190, 255), MakeRgb(120, 200, 255), MakeRgb(110, 140, 220),
			MakeRgb(220, 80, 90), MakeRgb(170, 220, 250), MakeRgb(130, 170, 255))},
	};
	return sPalettes;
}

// ---------------------------------------------------------------------------------------------------
// Fonts: the five bitmap fonts drawn with VectorFont. Each keeps the original's height, ascent and space width, so text
// stays where the game puts it; the strips' widths follow the glyphs. The legacy strips (alphadigits4, scorefont1,
// overfont2) give each character a cell of its advance; the descriptor fonts (afont1, mainfont1) pad each cell for the
// outline and say so with LayerSetCharOffsets.

enum FontFill
{
	FILL_WHITE,   // tinted by the game (score, headers, banner)
	FILL_GOLD     // drawn as it is (the point popups)
};

struct FontSpec
{
	const char* mStem;          // images/<stem>
	const char* mDescriptor;    // data/<stem>.txt
	bool mLegacy;               // "space ascent (char width)*" strip; else an ImageFont descriptor
	int mHeight, mAscent, mSpace;
	const char* mChars;         // NULL: printable ASCII
	double mCap, mWeight, mTracking, mWiden;
	bool mMonoDigits;           // the digits share one advance (a counting score keeps its place)
	double mOutline;            // dark edge, px
	float mOutlineAlpha;
	FontFill mFill;
	double mBevel;              // raised edges inside the letters, px (0: flat)
	bool mCapsMap;              // lower case drawn with the capitals (SetCharMap)
};

const FontSpec kFonts[] = {
	{"alphadigits4", "data/alphadigits4.txt", true, 34, 32, 23, "0123456789+", 22, 2.0, 2.0, 1.0, true, 1.6, 1.0f, FILL_GOLD, 1.2, false},
	{"scorefont1", "data/scorefont1.txt", true, 22, 19, 15, "0123456789", 14, 1.8, 2.1, 1.0, true, 1.0, 0.85f, FILL_WHITE, 0, false},
	{"afont1", "data/afont1.txt", false, 22, 19, 10, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-,.?$*!+@'\":;/()%&#=", 14, 1.6, 1.2, 1.0,
		false, 0.9, 0.7f, FILL_WHITE, 0, true},
	{"mainfont1", "data/mainfont1.txt", false, 16, 11, 6, NULL, 8.5, 1.5, 1.0, 0.95, false, 0.8, 0.45f, FILL_WHITE, 0, false},
	{"overfont2", "data/overfont2.txt", true, 50, 47, 24, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!-?$*", 34, 1.9, 1.8, 1.0, false, 2.2,
		0.9f, FILL_WHITE, 1.4, false},
};
const int NUM_FONTS = sizeof(kFonts) / sizeof(kFonts[0]);

struct GlyphCell
{
	int mChar;
	int mX, mW;        // the cell in the strip
	int mAdvance;
	double mPen;       // where the glyph's pen starts, strip x
};

struct FontLayout
{
	TextStyle mStyle;
	std::vector<GlyphCell> mCells;
	int mWidth = 0;
	int mPad = 0;
	double mBaseline = 0;
};

std::string FontChars(const FontSpec& theFont)
{
	if (theFont.mChars != NULL)
		return theFont.mChars;
	std::string aChars;
	for (int c = 33; c <= 126; c++)
		aChars += (char) c;
	return aChars;
}

FontLayout LayoutFont(const FontSpec& theFont)
{
	FontLayout aLayout;
	TextStyle& aStyle = aLayout.mStyle;
	aStyle.mCap = theFont.mCap;
	aStyle.mWeight = theFont.mWeight;
	aStyle.mWiden = theFont.mWiden;
	double aUnit = aStyle.Unit();
	// A legacy cell is the advance: the tracking leaves the outline room inside it
	aStyle.mTracking = theFont.mLegacy ? std::max(theFont.mTracking, 2 * (theFont.mOutline + 0.5) / aUnit) : theFont.mTracking;
	aLayout.mPad = theFont.mLegacy ? 0 : (int) std::ceil(theFont.mOutline + 1);

	std::string aChars = FontChars(theFont);
	bool hasLower = aChars.find('a') != std::string::npos;
	// Capitals and digits stand on the ascent line, outline and all; text with descenders keeps its baseline there
	aLayout.mBaseline = theFont.mAscent - (hasLower ? 0.5 : theFont.mWeight * aUnit / 2 + theFont.mOutline);

	double aMono = 0;
	if (theFont.mMonoDigits)
	{
		for (int c = '0'; c <= '9'; c++)
			aMono = std::max(aMono, CharAdvance(c, aStyle));
	}
	int x = 0;
	for (size_t i = 0; i < aChars.size(); i++)
	{
		int c = (unsigned char) aChars[i];
		double anAdvance = CharAdvance(c, aStyle);
		double aCellAdvance = (theFont.mMonoDigits && (c >= '0') && (c <= '9')) ? aMono : anAdvance;
		GlyphCell aCell;
		aCell.mChar = c;
		aCell.mAdvance = std::max(1, (int) std::lround(aCellAdvance));
		aCell.mX = x;
		aCell.mW = aCell.mAdvance + 2 * aLayout.mPad;
		aCell.mPen = x + aLayout.mPad + (aCell.mAdvance - anAdvance) / 2.0;
		x += aCell.mW;
		aLayout.mCells.push_back(aCell);
	}
	aLayout.mWidth = x;
	return aLayout;
}

std::string QuotedChar(int c)
{
	if (c == '\'')
		return "\"'\"";
	if (c == '\\')
		return "'\\\\'";
	return std::string("'") + (char) c + "'";
}

std::string IntText(int theValue)
{
	char aBuf[16];
	snprintf(aBuf, sizeof(aBuf), "%d", theValue);
	return aBuf;
}

std::string FontDescriptor(const FontSpec& theFont)
{
	FontLayout aLayout = LayoutFont(theFont);
	std::string aText;
	if (theFont.mLegacy)
	{
		aText = IntText(theFont.mSpace) + " " + IntText(theFont.mAscent) + "\n";
		for (size_t i = 0; i < aLayout.mCells.size(); i++)
			aText += std::string(1, (char) aLayout.mCells[i].mChar) + " " + IntText(aLayout.mCells[i].mAdvance) + " ";
		return aText + "\n";
	}

	std::string aChars, aWidths, aRects, anOffsets;
	for (size_t i = 0; i < aLayout.mCells.size(); i++)
	{
		const GlyphCell& aCell = aLayout.mCells[i];
		std::string aSep = (i == 0) ? "" : ((i % 16 == 0) ? ",\n  " : ", ");
		aChars += aSep + QuotedChar(aCell.mChar);
		aWidths += aSep + IntText(aCell.mAdvance);
		aRects += aSep + "(" + IntText(aCell.mX) + ", 0, " + IntText(aCell.mW) + ", " + IntText(theFont.mHeight) + ")";
		anOffsets += aSep + "(" + IntText(-aLayout.mPad) + ", 0)";
	}
	aText += "Define CharList\n ( " + aChars + " );\n\n";
	aText += "Define WidthList\n ( " + aWidths + " );\n\n";
	aText += "Define RectList\n ( " + aRects + " );\n\n";
	aText += "Define OffsetList\n ( " + anOffsets + " );\n\n";
	aText += "CreateLayer Main;\n";
	aText += std::string("LayerSetImage Main '../images/") + theFont.mStem + "';\n";
	aText += "LayerSetAscent Main " + IntText(theFont.mAscent) + ";\n";
	aText += "LayerSetCharWidths Main CharList WidthList;\n";
	aText += "LayerSetCharWidths Main (' ') (" + IntText(theFont.mSpace) + ");\n";
	aText += "LayerSetImageMap Main CharList RectList;\n";
	aText += "LayerSetCharOffsets Main CharList OffsetList;\n";
	if (theFont.mCapsMap)
	{
		std::string aLower, anUpper;
		for (int c = 'a'; c <= 'z'; c++)
		{
			aLower += std::string(c == 'a' ? "" : ", ") + QuotedChar(c);
			anUpper += std::string(c == 'a' ? "" : ", ") + QuotedChar(c - 'a' + 'A');
		}
		aText += "Define Lowercase (" + aLower + ");\n";
		aText += "Define Uppercase (" + anUpper + ");\n";
		aText += "SetCharMap Lowercase Uppercase;\n";
	}
	return aText;
}

const FontSpec* FontNamed(const std::string& theStem)
{
	for (int i = 0; i < NUM_FONTS; i++)
	{
		if (theStem == kFonts[i].mStem)
			return &kFonts[i];
	}
	return NULL;
}

bool ModHasFile(const std::string& thePath)
{
	std::string aModId;
	return !Platform::VFS::Get().Resolve(thePath, &aModId).empty() && !aModId.empty();
}

// A mod's font image (any of the names the loader probes) or descriptor
bool ModHasFont(const FontSpec& theFont)
{
	if (ModHasFile(theFont.mDescriptor))
		return true;
	static const char* const kExtensions[] = {".png", ".gif", ".jpg", ".jpeg", ".tga"};
	std::string aStem = theFont.mStem;
	const std::string aNames[] = {aStem, "_" + aStem, aStem + "_", aStem + "@2x", aStem + "@3x", aStem + "@4x"};
	for (const std::string& aName : aNames)
	{
		for (const char* anExtension : kExtensions)
		{
			if (ModHasFile("images/" + aName + anExtension))
				return true;
		}
	}
	return false;
}

int Density()
{
	return std::max(1, std::min(4, gSettings.mScale));
}

bool WritePng(const std::vector<uint32_t>& theBits, int theWidth, int theHeight, const std::string& theFileName)
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

} // namespace

// ---------------------------------------------------------------------------------------------------
// Settings

Settings SettingsFor(const Sexy::GameFeatures& theFeatures, int theGemColors)
{
	Settings aSettings;
	aSettings.mGems = theFeatures.mCodeArtGems;
	aSettings.mPalette = theFeatures.mCodeArtPalette;
	aSettings.mGemColors = std::max(7, std::min(12, theGemColors));
#ifdef __EMSCRIPTEN__
	aSettings.mScale = 2;   // the browser's load time and memory
#else
	aSettings.mScale = 4;
#endif
	return aSettings;
}

const Palette& PaletteNamed(const std::string& theName)
{
	for (const NamedPalette& aPalette : Palettes())
	{
		if (theName == aPalette.mName)
			return aPalette.mPalette;
	}
	return Palettes()[0].mPalette;
}

const Settings& CurrentSettings()
{
	return gSettings;
}

const Palette& CurrentPalette()
{
	return (gPalette != NULL) ? *gPalette : PaletteNamed("blue");
}

GemStyle CurrentGemStyle()
{
	return gGemStyle;
}

std::string Text(const char* theId, const char* theDefault)
{
	return gSettings.mText ? gSettings.mText(theId, theDefault) : std::string(theDefault);
}

void Enable(const Settings& theSettings)
{
	gSettings = theSettings;
	gPalette = &PaletteNamed(theSettings.mPalette);
	gGemStyle = GemStyleFromName(theSettings.mGems);
	gModFonts.clear();
	for (int i = 0; i < NUM_FONTS; i++)
	{
		if (ModHasFont(kFonts[i]))
			gModFonts.push_back(kFonts[i].mStem);
		else
			Platform::VFS::Get().SetGeneratedFile(kFonts[i].mDescriptor, FontDescriptor(kFonts[i]));
	}
	Platform::SetImageGenerator([](const std::string& theGamePath, Platform::LoadedImage& theOut) {
		return Generate(theGamePath, theOut);
	});
	gEnabled = true;
}

void Disable()
{
	Platform::SetImageGenerator(Platform::ImageGenerator());
	for (int i = 0; i < NUM_FONTS; i++)
		Platform::VFS::Get().SetGeneratedFile(kFonts[i].mDescriptor, "");
	gModFonts.clear();
	gEnabled = false;
}

bool IsEnabled()
{
	return gEnabled;
}

bool Generate(const std::string& theGamePath, Platform::LoadedImage& theOut)
{
	if (theGamePath.compare(0, 7, "images/") != 0)
		return false;
	std::string aName = theGamePath.substr(7);
	if (GenerateFontImage(aName, theOut) || GenerateImage(aName, theOut))
	{
		theOut.mSourcePath = "(code art) " + theGamePath;
		return true;
	}
	return false;
}

std::vector<std::string> ImageNames()
{
	std::vector<std::string> aNames;
	for (const std::string& aName : GeneratedImageNames())
		aNames.push_back("images/" + aName);
	for (int i = 0; i < NUM_FONTS; i++)
		aNames.push_back(std::string("images/") + kFonts[i].mStem);
	return aNames;
}

bool Dump(const std::string& theDir, std::string* theLog)
{
	bool isOk = true;
	for (const std::string& aPath : ImageNames())
	{
		std::string aName = aPath.substr(7);
		Platform::LoadedImage anImage;
		auto aStart = std::chrono::steady_clock::now();
		bool isDrawn = Generate(aPath, anImage);
		double aMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - aStart).count();
		char aLine[160];
		if (!isDrawn)
		{
			snprintf(aLine, sizeof(aLine), "%s: left to a mod\n", aName.c_str());
			*theLog += aLine;
			continue;
		}
		snprintf(aLine, sizeof(aLine), "%s %dx%d @%dx %.0f ms\n", aName.c_str(), anImage.mWidth, anImage.mHeight, anImage.mHDScale, aMs);
		*theLog += aLine;
		isOk &= WritePng(anImage.mBits, anImage.mWidth, anImage.mHeight, theDir + "/" + aName + ".png");
		if (!anImage.mHDBits.empty())
			isOk &= WritePng(anImage.mHDBits, anImage.mHDWidth, anImage.mHDHeight, theDir + "/" + aName + "@" + IntText(anImage.mHDScale) + "x.png");
	}
	for (int i = 0; i < NUM_FONTS; i++)
	{
		if (std::find(gModFonts.begin(), gModFonts.end(), kFonts[i].mStem) != gModFonts.end())
			continue;
		FILE* aFile = fopen((theDir + "/" + kFonts[i].mStem + ".txt").c_str(), "wb");
		if (aFile == NULL)
		{
			isOk = false;
			continue;
		}
		std::string aText = FontDescriptor(kFonts[i]);
		fwrite(aText.data(), 1, aText.size(), aFile);
		fclose(aFile);
	}
	return isOk;
}

// ---------------------------------------------------------------------------------------------------
// Fonts

bool GenerateFontImage(const std::string& theName, Platform::LoadedImage& theOut)
{
	const FontSpec* aFont = FontNamed(theName);
	if ((aFont == NULL) || (std::find(gModFonts.begin(), gModFonts.end(), theName) != gModFonts.end()))
		return false;
	FontLayout aLayout = LayoutFont(*aFont);
	Canvas aCanvas(aLayout.mWidth, aFont->mHeight, Density());
	Mask aGlyphs = aCanvas.NewMask();
	for (const GlyphCell& aCell : aLayout.mCells)
		DrawChar(aGlyphs, aCell.mChar, aCell.mPen, aLayout.mBaseline, aLayout.mStyle);

	if (aFont->mOutline > 0)
		aCanvas.Fill(aGlyphs.Grown(aFont->mOutline), Rgba(0.04f, 0.03f, 0.08f, aFont->mOutlineAlpha));
	double aTop = aLayout.mBaseline - aFont->mCap;
	std::vector<Stop> aStops;
	if (aFont->mFill == FILL_GOLD)
		aStops = {{0, MakeRgb(255, 250, 196)}, {0.5, MakeRgb(255, 206, 64)}, {1, MakeRgb(236, 112, 28)}};
	else
		aStops = {{0, Rgba(1, 1, 1)}, {1, Rgba(0.8f, 0.84f, 0.9f)}};
	aCanvas.Fill(aGlyphs, Linear(Vec2(0, aTop), Vec2(0, aLayout.mBaseline), aStops));
	if (aFont->mBevel > 0)
		Emboss(aCanvas, aGlyphs, aFont->mBevel, aFont->mBevel * 0.5, Rgba(1, 1, 1, 0.5f), Rgba(0, 0, 0, 0.3f));
	ToLoadedImage(aCanvas, theOut);
	return true;
}

// ---------------------------------------------------------------------------------------------------
// Drawing helpers

void ToLoadedImage(const Canvas& theCanvas, Platform::LoadedImage& theOut)
{
	theOut = Platform::LoadedImage();
	theOut.mWidth = theCanvas.mW;
	theOut.mHeight = theCanvas.mH;
	if (theCanvas.mScale > 1)
	{
		theCanvas.ToArgb(theOut.mHDBits);
		theCanvas.ToArgb1x(theOut.mBits);
		theOut.mHDScale = theCanvas.mScale;
		theOut.mHDWidth = theCanvas.mPW;
		theOut.mHDHeight = theCanvas.mPH;
	}
	else
		theCanvas.ToArgb(theOut.mBits);
	theOut.mHasAlpha = true;
}

Mask ShapeMask(const Canvas& theCanvas, const Path& thePath)
{
	Mask aMask = theCanvas.NewMask();
	aMask.Fill(thePath);
	return aMask;
}

Mask Shifted(const Mask& theMask, double theDX, double theDY)
{
	Mask aMask = theMask;
	aMask.Shift(theDX, theDY);
	return aMask;
}

void Emboss(Canvas& theCanvas, const Mask& theShape, double theDepth, double theSoft, const Rgba& theLight, const Rgba& theDark)
{
	double d = std::fabs(theDepth);
	Mask aTopLeft = theShape;
	aTopLeft.Subtract(Shifted(theShape, d, d));
	Mask aBottomRight = theShape;
	aBottomRight.Subtract(Shifted(theShape, -d, -d));
	if (theSoft > 0)
	{
		aTopLeft.Blur(theSoft).Intersect(theShape);
		aBottomRight.Blur(theSoft).Intersect(theShape);
	}
	theCanvas.Fill(aTopLeft, theDepth > 0 ? theLight : theDark);
	theCanvas.Fill(aBottomRight, theDepth > 0 ? theDark : theLight);
}

namespace
{

// A canvas over the box [x0, x1) x [y0, y1) of theCanvas, at whole logical pixels: *theX and *theY get its origin
Canvas Local(const Canvas& theCanvas, double x0, double y0, double x1, double y1, int* theX, int* theY)
{
	*theX = (int) std::floor(x0);
	*theY = (int) std::floor(y0);
	return Canvas((int) std::ceil(x1) - *theX, (int) std::ceil(y1) - *theY, theCanvas.mScale);
}

} // namespace

void Ball(Canvas& theCanvas, double cx, double cy, double theRadius, const Rgba& theColor)
{
	int ox, oy;
	double m = theRadius * 0.4 + 2;
	Canvas aLocal = Local(theCanvas, cx - theRadius - m, cy - theRadius - m, cx + theRadius + m, cy + theRadius + m, &ox, &oy);
	double x = cx - ox, y = cy - oy;
	Mask aBall = ShapeMask(aLocal, Path().Ellipse(x, y, theRadius, theRadius));
	aLocal.Shadow(aBall, theRadius * 0.06, theRadius * 0.1, theRadius * 0.12, Rgba(0, 0, 0, 0.45f));
	aLocal.Fill(aBall, Radial(Vec2(x - theRadius * 0.35, y - theRadius * 0.4), theRadius * 1.5, theRadius * 1.5,
		{{0, Lighter(theColor, 0.55)}, {0.45, theColor}, {1, Darker(theColor, 0.65)}}));
	// light bounced in at the lower right, a darker edge, and the highlight
	Mask aRim = aBall;
	aRim.Subtract(Shifted(aBall, -theRadius * 0.12, -theRadius * 0.12));
	aRim.Blur(theRadius * 0.1).Intersect(aBall);
	aLocal.Fill(aRim, Lighter(theColor, 0.35).WithAlpha(0.55f));
	Mask anEdge = aBall;
	anEdge.Subtract(aBall.Grown(-std::max(0.8, theRadius * 0.05)));
	aLocal.Fill(anEdge, Darker(theColor, 0.7).WithAlpha(0.6f));
	Mask aShine = ShapeMask(aLocal, Path().Ellipse(x - theRadius * 0.15, y - theRadius * 0.5, theRadius * 0.55, theRadius * 0.32));
	aShine.Blur(theRadius * 0.06);
	aLocal.Fill(aShine, Linear(Vec2(0, y - theRadius * 0.85), Vec2(0, y - theRadius * 0.15), {{0, Rgba(1, 1, 1, 0.9f)}, {1, Rgba(1, 1, 1, 0)}}));
	theCanvas.Draw(aLocal, ox, oy);
}

void Ring(Canvas& theCanvas, double cx, double cy, double r0, double r1, const Palette& thePalette)
{
	int ox, oy;
	double m = 3;
	Canvas aLocal = Local(theCanvas, cx - r1 - m, cy - r1 - m, cx + r1 + m, cy + r1 + m, &ox, &oy);
	double x = cx - ox, y = cy - oy;
	Path aPath;
	aPath.Ellipse(x, y, r1, r1);
	aPath.Reversed(Path().Ellipse(x, y, r0, r0));
	Mask aRing = ShapeMask(aLocal, aPath);
	aLocal.Shadow(aRing, 0.8, 1.2, 1.2, Rgba(0, 0, 0, 0.4f));
	Rgba aMetal = Mix(thePalette.mPanelLight, Rgba(0.75f, 0.78f, 0.82f), 0.5);
	aLocal.Fill(aRing, Linear(Vec2(x - r1, y - r1), Vec2(x + r1, y + r1),
		{{0, Lighter(aMetal, 0.6)}, {0.5, aMetal}, {1, Darker(aMetal, 0.55)}}));
	Emboss(aLocal, aRing, 1.0, 0.6, Rgba(1, 1, 1, 0.5f), Rgba(0, 0, 0, 0.4f));
	theCanvas.Draw(aLocal, ox, oy);
}

double FitCap(const std::string& theText, TextStyle theStyle, double theWidth, double theMaxCap)
{
	theStyle.mCap = theMaxCap;
	double aWidth = TextWidth(theText, theStyle);
	return (aWidth > theWidth) ? theMaxCap * theWidth / aWidth : theMaxCap;
}

std::vector<std::string> SplitLines(const std::string& theText)
{
	std::vector<std::string> aLines(1);
	for (char c : theText)
	{
		if (c == '|')
			aLines.push_back(std::string());
		else
			aLines.back() += c;
	}
	return aLines;
}

void Label(Canvas& theCanvas, const std::string& theText, double cx, double y, const LabelStyle& theStyle)
{
	if (theText.empty())
		return;
	const TextStyle& aText = theStyle.mText;
	double aWidth = TextWidth(theText, aText);
	double aMargin = theStyle.mOutline + theStyle.mGlow * 2 + std::max(std::fabs(theStyle.mShadowX), std::fabs(theStyle.mShadowY)) +
		theStyle.mShadowBlur * 2 + aText.mWeight * aText.Unit() + std::fabs(aText.mSlant) * aText.mCap + 2;
	int ox, oy;
	Canvas aLocal = Local(theCanvas, cx - aWidth / 2 - aMargin, y - aText.mCap - aMargin, cx + aWidth / 2 + aMargin,
		y + 0.35 * aText.mCap + aMargin, &ox, &oy);
	double aBaseline = y - oy;
	Mask aGlyphs = aLocal.NewMask();
	DrawText(aGlyphs, theText, cx - aWidth / 2 - ox, aBaseline, aText);

	if ((theStyle.mGlow > 0) && (theStyle.mGlowColor.a > 0))
		aLocal.Shadow(aGlyphs.Grown(theStyle.mGlow * 0.5), 0, 0, theStyle.mGlow, theStyle.mGlowColor);
	Mask anOutline = (theStyle.mOutline > 0) ? aGlyphs.Grown(theStyle.mOutline) : aGlyphs;
	if (theStyle.mShadowColor.a > 0)
		aLocal.Shadow(anOutline, theStyle.mShadowX, theStyle.mShadowY, theStyle.mShadowBlur, theStyle.mShadowColor);
	if (theStyle.mOutline > 0)
		aLocal.Fill(anOutline, theStyle.mOutlineColor);
	std::vector<Stop> aStops = theStyle.mStops.empty() ? std::vector<Stop>{{0, theStyle.mTop}, {1, theStyle.mBottom}} : theStyle.mStops;
	aLocal.Fill(aGlyphs, Linear(Vec2(0, aBaseline - aText.mCap), Vec2(0, aBaseline), aStops));
	if (theStyle.mBevel > 0)
		Emboss(aLocal, aGlyphs, theStyle.mBevel, theStyle.mBevel * 0.5, Rgba(1, 1, 1, 0.45f), Rgba(0, 0, 0, 0.35f));
	theCanvas.Draw(aLocal, ox, oy);
}

}
