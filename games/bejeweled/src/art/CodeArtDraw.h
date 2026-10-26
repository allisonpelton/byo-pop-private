#ifndef __ART_CODEARTDRAW_H__
#define __ART_CODEARTDRAW_H__

// port(variant): what the code-drawn art's generators share (CodeArt.cpp, CodeArtImages.cpp): the palettes, the
// settings in force, and drawing helpers on top of Raster and VectorFont (bevels, glossy balls, labels).

#include "CodeArt.h"
#include "GemArt.h"
#include "Raster.h"
#include "VectorFont.h"

#include <string>
#include <vector>

namespace CodeArt
{

// One look's colours. Panels go from mPanelDark through mPanel to mPanelLight; mInk is the labels on them, mGlow the
// lit state and accents, mWell the dark insets (the score box, the bar slots).
struct Palette
{
	Art::Rgba mPanel, mPanelDark, mPanelLight, mInk, mGlow, mWell;
	Art::Rgba mBackTop, mBackBottom;               // the title screen's backdrop, centre and edge
	Art::Rgba mLogoTop, mLogoBottom, mLogoGlow;    // the title lettering
	Art::Rgba mBar;                                // the loading bar
	Art::Rgba mOrb;                                // the hint orb
	Art::Rgba mLampLeft, mLampRight;               // the lamps by the time bar (the right one holds the level number)
	Art::Rgba mVortex;                             // the level-complete vortex and the wipe line
};

const Palette& PaletteNamed(const std::string& theName);   // blue for an unknown name

// The settings Enable stored, and the look they select
const Settings& CurrentSettings();
const Palette& CurrentPalette();
Art::GemStyle CurrentGemStyle();
std::string Text(const char* theId, const char* theDefault);   // a label through Settings::mText

// The drawn image: 1x bits with their alpha, and the HD bits when theCanvas has a scale above 1
void ToLoadedImage(const Art::Canvas& theCanvas, Platform::LoadedImage& theOut);

// Drawing helpers (logical pixels)
Art::Mask ShapeMask(const Art::Canvas& theCanvas, const Art::Path& thePath);
Art::Mask Shifted(const Art::Mask& theMask, double theDX, double theDY);
// Edges lit from the upper left: theDepth > 0 raises the shape (light top-left rim, dark bottom-right), < 0 sinks it
void Emboss(Art::Canvas& theCanvas, const Art::Mask& theShape, double theDepth, double theSoft, const Art::Rgba& theLight,
	const Art::Rgba& theDark);
// A glossy ball of radius r: shaded from the upper left, a rim and a highlight
void Ball(Art::Canvas& theCanvas, double cx, double cy, double r, const Art::Rgba& theColor);
// A metal ring between radii r0 < r1 around a ball
void Ring(Art::Canvas& theCanvas, double cx, double cy, double r0, double r1, const Palette& thePalette);

struct LabelStyle
{
	Art::TextStyle mText;
	Art::Rgba mTop = Art::Rgba(1, 1, 1), mBottom = Art::Rgba(1, 1, 1);   // fill, from the cap line to the baseline
	std::vector<Art::Stop> mStops;                                        // ... or these stops, when there are any
	double mOutline = 0;                                                  // dark edge, px
	Art::Rgba mOutlineColor = Art::Rgba(0, 0, 0, 0.8f);
	double mShadowX = 0, mShadowY = 0, mShadowBlur = 0;                   // drop shadow (none when its colour is clear)
	Art::Rgba mShadowColor = Art::Rgba(0, 0, 0, 0);
	double mGlow = 0;                                                     // outer glow radius
	Art::Rgba mGlowColor = Art::Rgba(1, 1, 1, 0);
	double mBevel = 0;                                                    // raised edges inside the letters
};
// Draws one line centred on cx with its baseline at y (drawn on a small canvas of its own, then composited)
void Label(Art::Canvas& theCanvas, const std::string& theText, double cx, double y, const LabelStyle& theStyle);
// The capital height at which theText fits theWidth, at most theMaxCap
double FitCap(const std::string& theText, Art::TextStyle theStyle, double theWidth, double theMaxCap);
std::vector<std::string> SplitLines(const std::string& theText);   // at '|'

// The image generators (CodeArtImages.cpp): fills theOut and returns true when theName ("alphaback1") is one of
// them
bool GenerateImage(const std::string& theName, Platform::LoadedImage& theOut);
const std::vector<std::string>& GeneratedImageNames();

// The fonts (CodeArt.cpp)
bool GenerateFontImage(const std::string& theName, Platform::LoadedImage& theOut);

}

#endif
