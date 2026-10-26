// port(variant): code-drawn art (CodeArt.h): one generator per game image, in the original's size and role. Positions
// in the backdrop (alphaback1) and the title screen (titlescreen6) follow where the game's code draws over them: the
// side buttons' rectangles (WinDMApp::InitButtons), the score (Board::Draw, centred at x 93, baseline 106), the time
// bar (237, 444, 342 x 25), the level number on the right lamp (606, 465), the hint orb under over1-hint1 (64, 303),
// the loading bar (TitleScreen::Draw, 143, 275) and the version text (537, 221).

#include "CodeArtDraw.h"

#include <algorithm>
#include <cmath>

using namespace Art;

namespace CodeArt
{

namespace
{

const double kPi = 3.14159265358979323846;

const Palette& P()
{
	return CurrentPalette();
}

int Density(int theMax = 4)
{
	return std::max(1, std::min(theMax, CurrentSettings().mScale));
}

double Smooth(double e0, double e1, double x)
{
	double t = std::max(0.0, std::min(1.0, (x - e0) / (e1 - e0)));
	return t * t * (3 - 2 * t);
}

// A canvas for an image of theW x theH, at most theMaxScale times as dense
Canvas NewCanvas(int theW, int theH, int theMaxScale = 4)
{
	return Canvas(theW, theH, Density(theMaxScale));
}

// A canvas over the box [x0, x1) x [y0, y1) of theCanvas at whole logical pixels: *theX, *theY get its origin
Canvas LocalCanvas(const Canvas& theCanvas, double x0, double y0, double x1, double y1, int* theX, int* theY)
{
	*theX = (int) std::floor(x0);
	*theY = (int) std::floor(y0);
	return Canvas((int) std::ceil(x1) - *theX, (int) std::ceil(y1) - *theY, theCanvas.mScale);
}

// The edge band of a shape, theWidth px wide, inside it
Mask EdgeOf(const Mask& theShape, double theWidth)
{
	Mask anEdge = theShape;
	anEdge.Subtract(theShape.Grown(-theWidth));
	return anEdge;
}

TextStyle Caps(double theCap, double theWeight, double theTracking = 1.0, double theWiden = 1.0)
{
	TextStyle aStyle;
	aStyle.mCap = theCap;
	aStyle.mWeight = theWeight;
	aStyle.mTracking = theTracking;
	aStyle.mWiden = theWiden;
	aStyle.mCaps = true;
	return aStyle;
}

// The title lettering: a chiselled metal gradient, a dark edge and a glow in the palette's colours
LabelStyle LogoStyle(double theCap, double theOutline, double theGlow)
{
	const Palette& p = P();
	LabelStyle aStyle;
	aStyle.mText = Caps(theCap, 2.3, 1.0, 0.9);
	aStyle.mText.mSlant = 0.06;
	aStyle.mStops = {{0, Lighter(p.mLogoTop, 0.55)}, {0.46, p.mLogoTop}, {0.54, Darker(p.mLogoBottom, 0.12)}, {1, Lighter(p.mLogoBottom, 0.2)}};
	aStyle.mOutline = theOutline;
	aStyle.mOutlineColor = Darker(p.mLogoBottom, 0.8).WithAlpha(0.95f);
	aStyle.mGlow = theGlow;
	aStyle.mGlowColor = p.mLogoGlow.WithAlpha(0.6f);
	aStyle.mShadowX = theOutline * 0.8;
	aStyle.mShadowY = theOutline * 1.3;
	aStyle.mShadowBlur = theOutline;
	aStyle.mShadowColor = Rgba(0, 0, 0, 0.5f);
	aStyle.mBevel = std::max(0.8, theCap * 0.022);
	return aStyle;
}

// ---------------------------------------------------------------------------------------------------
// Panels and parts

// A raised plate, drawn on a canvas of its own
void Plate(Canvas& theCanvas, double x, double y, double w, double h, double r, double theLift)
{
	const Palette& p = P();
	int ox, oy;
	Canvas aLocal = LocalCanvas(theCanvas, x - 6, y - 6, x + w + 6, y + h + 6, &ox, &oy);
	Mask aShape = ShapeMask(aLocal, Path().RoundRect(x - ox, y - oy, w, h, r));
	aLocal.Shadow(aShape, 1.5, 2, 2.5, Rgba(0, 0, 0, 0.4f));
	aLocal.Fill(aShape, Linear(Vec2(0, y - oy), Vec2(0, y - oy + h),
		{{0, Mix(p.mPanel, p.mPanelLight, 0.12 + theLift)}, {1, Mix(p.mPanel, p.mPanelDark, 0.2)}}));
	Emboss(aLocal, aShape, 1.8, 1.0, Rgba(1, 1, 1, 0.35f), Rgba(0, 0, 0, 0.45f));
	aLocal.Fill(EdgeOf(aShape, 0.8), Darker(p.mPanelDark, 0.3).WithAlpha(0.55f));
	theCanvas.Draw(aLocal, ox, oy);
}

// A sunken well (the score box, the bar slots, the edit box)
void Well(Canvas& theCanvas, double x, double y, double w, double h, double r)
{
	const Palette& p = P();
	int ox, oy;
	Canvas aLocal = LocalCanvas(theCanvas, x - 3, y - 3, x + w + 3, y + h + 3, &ox, &oy);
	Mask aShape = ShapeMask(aLocal, Path().RoundRect(x - ox, y - oy, w, h, r));
	aLocal.Fill(aShape, Linear(Vec2(0, y - oy), Vec2(0, y - oy + h), {{0, Darker(p.mWell, 0.3)}, {1, Lighter(p.mWell, 0.08)}}));
	// the rim's shadow falls in from the upper left
	Mask aShade = aShape;
	aShade.Invert();
	aShade.Shift(1.5, 2).Blur(2.2).Intersect(aShape);
	aLocal.Fill(aShade, Rgba(0, 0, 0, 0.7f));
	Emboss(aLocal, aShape, -1.2, 0.6, Lighter(p.mPanelLight, 0.3).WithAlpha(0.5f), Rgba(0, 0, 0, 0.5f));
	theCanvas.Draw(aLocal, ox, oy);
}

// A glossy capsule (the loading bar, the time bar)
void Capsule(Canvas& theCanvas, double x, double y, double w, double h, const Rgba& theColor, const Rgba& theEdge)
{
	Mask aShape = ShapeMask(theCanvas, Path().RoundRect(x + 0.5, y + 0.5, w - 1, h - 1, (h - 1) / 2));
	theCanvas.Fill(aShape, Linear(Vec2(0, y), Vec2(0, y + h),
		{{0, Lighter(theColor, 0.5)}, {0.42, theColor}, {0.8, Darker(theColor, 0.25)}, {1, Darker(theColor, 0.45)}}));
	Mask aShine = ShapeMask(theCanvas, Path().RoundRect(x + h * 0.35, y + h * 0.13, w - h * 0.7, h * 0.3, h * 0.15));
	aShine.Blur(h * 0.04);
	theCanvas.Fill(aShine, Linear(Vec2(0, y + h * 0.13), Vec2(0, y + h * 0.45), {{0, Rgba(1, 1, 1, 0.75f)}, {1, Rgba(1, 1, 1, 0.1f)}}));
	theCanvas.Fill(EdgeOf(aShape, 1.0), theEdge);
}

enum ButtonState
{
	BUTTON_UP,
	BUTTON_DOWN,
	BUTTON_LIT
};

struct SideButton
{
	const char* mTextId;
	const char* mDefault;
	int mX, mY, mW, mH;
};

// WinDMApp::InitButtons
const SideButton kNewGame = {"CODEART_NEW_GAME", "NEW GAME", 18, 142, 156, 41};
const SideButton kNormal = {"CODEART_NORMAL", "NORMAL", 18, 185, 78, 31};
const SideButton kTimed = {"CODEART_TIMETRIAL", "TIME TRIAL", 96, 185, 78, 31};
const SideButton kOptions = {"CODEART_OPTIONS", "OPTIONS", 18, 241, 156, 41};
const SideButton kQuit = {"CODEART_QUIT", "QUIT", 18, 405, 156, 40};

// A side button's face at (x, y): drawn into the backdrop (up) and as the button's own images (down, lit), which the
// widget draws over the backdrop at the same place
void DrawSideButton(Canvas& theCanvas, const SideButton& theButton, double x, double y, ButtonState theState)
{
	const Palette& p = P();
	double w = theButton.mW, h = theButton.mH;
	int ox, oy;
	Canvas aLocal = LocalCanvas(theCanvas, x, y, x + w, y + h, &ox, &oy);
	double lx = x - ox, ly = y - oy;
	Mask aFace = ShapeMask(aLocal, Path().RoundRect(lx + 1.5, ly + 1.5, w - 3, h - 3, 7));
	Rgba aTop, aBottom, anInk;
	if (theState == BUTTON_UP)
	{
		aTop = Mix(p.mPanel, p.mPanelLight, 0.32);
		aBottom = Mix(p.mPanel, p.mPanelDark, 0.25);
		anInk = p.mInk;
	}
	else if (theState == BUTTON_DOWN)
	{
		aTop = Mix(p.mPanel, p.mPanelDark, 0.5);
		aBottom = Mix(p.mPanel, p.mPanelDark, 0.22);
		anInk = Mix(p.mInk, p.mPanel, 0.2);
	}
	else
	{
		aTop = Lighter(p.mGlow, 0.35);
		aBottom = Darker(p.mGlow, 0.22);
		anInk = Darker(p.mPanelDark, 0.2);
	}
	aLocal.Fill(aFace, Linear(Vec2(0, ly), Vec2(0, ly + h), {{0, aTop}, {1, aBottom}}));
	if (theState == BUTTON_DOWN)
	{
		Mask aShade = aFace;
		aShade.Invert();
		aShade.Shift(1.2, 1.6).Blur(1.8).Intersect(aFace);
		aLocal.Fill(aShade, Rgba(0, 0, 0, 0.55f));
	}
	Emboss(aLocal, aFace, (theState == BUTTON_DOWN) ? -1.2 : 1.5, 0.7, Rgba(1, 1, 1, 0.45f), Rgba(0, 0, 0, 0.45f));
	aLocal.Fill(EdgeOf(aFace, 0.9), Darker(p.mPanelDark, 0.4).WithAlpha(0.7f));

	std::string aText = Text(theButton.mTextId, theButton.mDefault);
	LabelStyle aLabel;
	aLabel.mText = Caps(0, 1.7, 1.1, 0.9);
	aLabel.mText.mCap = FitCap(aText, aLabel.mText, w - 16, h * 0.33);
	aLabel.mTop = Lighter(anInk, 0.2);
	aLabel.mBottom = anInk;
	if (theState == BUTTON_LIT)
	{
		aLabel.mShadowY = 1;
		aLabel.mShadowColor = Rgba(1, 1, 1, 0.45f);
	}
	else
	{
		aLabel.mShadowX = 1;
		aLabel.mShadowY = 1;
		aLabel.mShadowBlur = 0.6;
		aLabel.mShadowColor = Rgba(0, 0, 0, 0.6f);
	}
	double aShift = (theState == BUTTON_DOWN) ? 1 : 0;
	Label(aLocal, aText, lx + w / 2 + aShift, ly + h / 2 + aLabel.mText.mCap / 2 + aShift, aLabel);
	theCanvas.Draw(aLocal, ox, oy);
}

void DrawSideButtonImage(Canvas& theCanvas, const SideButton& theButton, ButtonState theState)
{
	DrawSideButton(theCanvas, theButton, 0, 0, theState);
}

// A white cursor shape with a dark edge and a soft shadow
void CursorShape(Canvas& theCanvas, const Mask& theShape)
{
	const Palette& p = P();
	Mask anEdge = theShape.Grown(1.3);
	theCanvas.Shadow(anEdge, 1.5, 2, 1.5, Rgba(0, 0, 0, 0.4f));
	theCanvas.Fill(anEdge, Darker(p.mPanelDark, 0.5));
	theCanvas.Fill(theShape, Linear(Vec2(0, theCanvas.mH * 0.3), Vec2(0, theCanvas.mH * 0.95),
		{{0, Rgba(1, 1, 1)}, {1, Mix(Rgba(1, 1, 1), p.mGlow, 0.35)}}));
}

// ---------------------------------------------------------------------------------------------------
// The generators

void DrawAlphaback(Canvas& c)
{
	const Palette& p = P();
	c.FillAll(Linear(Vec2(0, 0), Vec2(0, 480), {{0, Mix(p.mPanel, p.mPanelLight, 0.16)}, {1, Mix(p.mPanel, p.mPanelDark, 0.38)}}));
	// brushed metal
	c.FillAll(Sampled([](double x, double y) {
		double n = Fbm(x * 0.012, y * 0.45, 7, 3);
		return (n > 0) ? Rgba(1, 1, 1, (float) (0.07 * n)) : Rgba(0, 0, 0, (float) (-0.09 * n));
	}, c.mW, c.mH, 1));

	// The board's window and the lip around it; the lip's shadow falls on the board
	Path aWindow;
	aWindow.RoundRect(191, 9, 435, 423, 11);
	Mask aWindowMask = ShapeMask(c, aWindow);
	Path aLip;
	aLip.RoundRect(185, 3, 447, 435, 16);
	aLip.Reversed(aWindow);
	Mask aLipMask = ShapeMask(c, aLip);
	c.Shadow(aLipMask, 1.5, 2, 2.5, Rgba(0, 0, 0, 0.35f));
	c.Fill(aLipMask, Linear(Vec2(0, 3), Vec2(0, 438), {{0, Mix(p.mPanel, p.mPanelDark, 0.3)}, {1, Mix(p.mPanel, p.mPanelDark, 0.55)}}));
	Emboss(c, aLipMask, 1.6, 0.8, Rgba(1, 1, 1, 0.35f), Rgba(0, 0, 0, 0.5f));
	c.Fill(aWindowMask, Rgba(0, 0, 0, 1), 1, BLEND_ERASE);
	Mask anInShadow = aWindowMask;
	anInShadow.Invert();
	anInShadow.Shift(2.5, 3).Blur(3.5).Intersect(aWindowMask);
	c.Fill(anInShadow, Rgba(0, 0, 0, 0.6f));

	// The left column: the title, the score, the buttons and the hint orb
	LabelStyle aLogo = LogoStyle(0, 1.4, 4);
	aLogo.mText.mCap = FitCap(Text("CODEART_TITLE", "BEJEWELED"), aLogo.mText, 166, 28);
	Label(c, Text("CODEART_TITLE", "BEJEWELED"), 95, 46, aLogo);

	LabelStyle aSmall;
	aSmall.mText = Caps(7, 1.6, 2.2);
	aSmall.mTop = aSmall.mBottom = p.mInk.WithAlpha(0.85f);
	aSmall.mShadowX = aSmall.mShadowY = 0.8;
	aSmall.mShadowColor = Rgba(0, 0, 0, 0.5f);
	Label(c, Text("CODEART_SCORE", "SCORE"), 93, 74, aSmall);
	Well(c, 17, 79, 152, 35, 8);

	Plate(c, 11, 135, 170, 88, 10, 0);
	DrawSideButton(c, kNewGame, kNewGame.mX, kNewGame.mY, BUTTON_UP);
	DrawSideButton(c, kNormal, kNormal.mX, kNormal.mY, BUTTON_UP);
	DrawSideButton(c, kTimed, kTimed.mX, kTimed.mY, BUTTON_UP);

	Plate(c, 11, 234, 170, 158, 10, 0);
	DrawSideButton(c, kOptions, kOptions.mX, kOptions.mY, BUTTON_UP);
	Ring(c, 90.5, 339, 30, 38, p);
	Ball(c, 90.5, 339, 30.5, p.mOrb);

	Plate(c, 11, 398, 170, 54, 10, 0);
	DrawSideButton(c, kQuit, kQuit.mX, kQuit.mY, BUTTON_UP);

	// The bottom: the time bar's slot between two lamps; the level number sits on the right one, in black
	Well(c, 233, 440, 350, 33, 15);
	Ring(c, 207, 457, 12, 16, p);
	Ball(c, 207, 457, 12.5, p.mLampLeft);
	Ring(c, 606, 457, 14, 18, p);
	Ball(c, 606, 457, 14.5, p.mLampRight);
}

// The title's gems, (colour, centre): the seven colours in an arc under the name, clear of the version text at (537, 221)
struct TitleGem
{
	int mColor;
	double mX, mY;
};
const TitleGem kTitleGems[] = {{3, 164, 214}, {5, 216, 205}, {2, 268, 199}, {1, 320, 197}, {6, 372, 199}, {4, 424, 205},
	{0, 476, 214}};

void DrawTitleScreen(Canvas& c)
{
	const Palette& p = P();
	c.FillAll(Radial(Vec2(320, 190), 470, 400, {{0, p.mBackTop}, {1, p.mBackBottom}}));
	Rgba aMist = Mix(p.mBackTop, p.mVortex, 0.5);
	c.FillAll(Sampled([aMist](double x, double y) {
		double n = Fbm(x / 150.0, y / 150.0, 11, 4);
		return aMist.WithAlpha((float) (std::max(0.0, n) * 0.4));
	}, c.mW, c.mH, 4));
	// stars
	Path aStars;
	for (int i = 0; i < 70; i++)
	{
		double x = 14 + Hash01(i, 0, 23) * 612, y = 14 + Hash01(i, 1, 23) * 452;
		double r = 0.8 + 2.4 * Hash01(i, 2, 23) * Hash01(i, 3, 23);
		aStars.Star(x, y, 4, r * 2.2, r * 0.45, 90);
	}
	Mask aStarMask = ShapeMask(c, aStars);
	c.Shadow(aStarMask, 0, 0, 1.6, Rgba(1, 1, 1, 0.5f));
	c.Fill(aStarMask, Rgba(1, 1, 1, 0.8f));

	// the frame
	Path anInner;
	anInner.RoundRect(9, 9, 622, 462, 12);
	Mask anInnerMask = ShapeMask(c, anInner);
	Mask aFrame = anInnerMask;
	aFrame.Invert();
	Mask aShade = aFrame;
	aShade.Shift(2.5, 3).Blur(4).Intersect(anInnerMask);
	c.Fill(aShade, Rgba(0, 0, 0, 0.55f));
	c.Fill(aFrame, Linear(Vec2(0, 0), Vec2(640, 480), {{0, Mix(p.mPanel, p.mPanelLight, 0.25)}, {1, Mix(p.mPanel, p.mPanelDark, 0.45)}}));
	Emboss(c, aFrame, 2.0, 1.0, Rgba(1, 1, 1, 0.4f), Rgba(0, 0, 0, 0.5f));
	Mask aLine = ShapeMask(c, Path().RoundRect(7.8, 7.8, 624.4, 464.4, 13.2));
	aLine.Subtract(anInnerMask);
	c.Fill(aLine, Darker(p.mPanelDark, 0.4));

	// the name
	std::string aTitle = Text("CODEART_TITLE", "BEJEWELED");
	LabelStyle aLogo = LogoStyle(0, 3.5, 14);
	aLogo.mText.mCap = FitCap(aTitle, aLogo.mText, 520, 96);
	Label(c, aTitle, 320, 150, aLogo);

	// the gems, each in a halo
	for (const TitleGem& aGem : kTitleGems)
	{
		Rgba aHalo = Mix(Rgba(1, 1, 1), p.mLogoGlow, 0.5);
		c.Fill(Path().Ellipse(aGem.mX, aGem.mY, 36, 36), Radial(Vec2(aGem.mX, aGem.mY), 36, 36,
			{{0, aHalo.WithAlpha(0.4f)}, {1, aHalo.WithAlpha(0)}}));
	}
	for (const TitleGem& aGem : kTitleGems)
		DrawGem(c, CurrentGemStyle(), aGem.mColor, aGem.mX, aGem.mY, 46, 46);

	// the loading bar's slot
	Well(c, 139, 271, 348, 32, 15);
}

void DrawLoadingBar(Canvas& c)
{
	Capsule(c, 0, 0, 340, 24, P().mBar, Darker(P().mBar, 0.65).WithAlpha(0.85f));
}

// Grey: the game colours it from green to red
void DrawTimerBar(Canvas& c)
{
	Capsule(c, 0, 0, 342, 25, Rgba(0.84f, 0.84f, 0.84f), Rgba(0.3f, 0.3f, 0.3f, 0.85f));
}

void DrawTrialCircle(Canvas& c)
{
	const Palette& p = P();
	Ring(c, 52.5, 53.5, 40, 50, p);
	c.Fill(Path().Ellipse(52.5, 53.5, 40.5, 40.5), Radial(Vec2(45, 44), 52, 52, {{0, p.mPanel}, {1, p.mPanelDark}}));
	LabelStyle aLabel;
	aLabel.mText = Caps(14, 1.8);
	aLabel.mTop = Lighter(p.mGlow, 0.4);
	aLabel.mBottom = p.mGlow;
	aLabel.mOutline = 1.2;
	Label(c, Text("CODEART_TRIAL", "TRIAL"), 52.5, 60.5, aLabel);
}

void DrawArrowCursor(Canvas& c)
{
	// the tip is the image's centre: the game draws custom cursors centred on the mouse
	Mask aShape = ShapeMask(c, Path().Polygon({Vec2(27, 27), Vec2(27, 46), Vec2(31.6, 41.6), Vec2(35.2, 49.4), Vec2(38.6, 47.9),
		Vec2(35, 40.3), Vec2(41.2, 40.3)}));
	CursorShape(c, aShape);
}

void DrawHandCursor(Canvas& c)
{
	// the fingertip at the centre (29, 29); one fill per part, so their windings never cancel
	Mask aHand = c.NewMask();
	aHand.Fill(Path().RoundRect(25.5, 27, 7, 18, 3.5));
	aHand.Fill(Path().RoundRect(22, 39, 25, 16, 6));
	aHand.Fill(Path().RoundRect(32.5, 35, 6, 10, 3));
	aHand.Fill(Path().RoundRect(38, 36.5, 5.5, 9, 2.75));
	aHand.Fill(Path().RoundRect(42.5, 38.5, 5, 8, 2.5));
	aHand.Fill(Path().Ellipse(20.5, 44, 3.8, 6.5));
	CursorShape(c, aHand);
	// the finger lines
	Mask aLines = c.NewMask();
	aLines.Stroke({Vec2(32.5, 41), Vec2(32.5, 45)}, 0.8, false);
	aLines.Stroke({Vec2(38, 42), Vec2(38, 46)}, 0.8, false);
	aLines.Stroke({Vec2(42.6, 43), Vec2(42.6, 46.5)}, 0.8, false);
	c.Fill(aLines, Rgba(0, 0, 0, 0.35f));
}

void DrawGrabCursor(Canvas& c)
{
	Mask aFist = c.NewMask();
	aFist.Fill(Path().RoundRect(16, 24, 23, 17, 6));
	for (int i = 0; i < 4; i++)
		aFist.Fill(Path().Ellipse(19.5 + i * 5.5, 23.5, 3.2, 3.6));
	aFist.Fill(Path().Ellipse(15.5, 31, 3.5, 5.5));
	CursorShape(c, aFist);
}

void DrawTextCursor(Canvas& c)
{
	Mask aBeam = c.NewMask();
	aBeam.Stroke({Vec2(23.5, 16.5), Vec2(30.5, 16.5)}, 1.8, false);
	aBeam.Stroke({Vec2(23.5, 37.5), Vec2(30.5, 37.5)}, 1.8, false);
	aBeam.Stroke({Vec2(27, 16.5), Vec2(27, 37.5)}, 1.8, false);
	CursorShape(c, aBeam);
}

void DrawGemSheetImage(Platform::LoadedImage& theOut)
{
	int aColors = std::max(1, std::min(12, CurrentSettings().mGemColors));
	int aScale = Density();
	theOut = Platform::LoadedImage();
	theOut.mWidth = 780;
	theOut.mHeight = 104 * aColors;
	theOut.mHasAlpha = true;
	if (aScale == 1)
	{
		DrawGemSheet(CurrentGemStyle(), aColors, 1, theOut.mBits);
		return;
	}
	DrawGemSheet(CurrentGemStyle(), aColors, aScale, theOut.mHDBits);
	theOut.mHDScale = aScale;
	theOut.mHDWidth = 780 * aScale;
	theOut.mHDHeight = 104 * aColors * aScale;
	DownsampleArgb(theOut.mHDBits, theOut.mHDWidth, theOut.mHDHeight, aScale, theOut.mBits);
}

// The selection cursor: corner brackets that breathe over 6 frames, drawn at the gem's cell less 2
void DrawTarget(Canvas& c)
{
	const Palette& p = P();
	for (int f = 0; f < 6; f++)
	{
		Canvas aFrame(52, 52, c.mScale);
		double t = 0.5 - 0.5 * std::cos(2 * kPi * f / 6);
		double d = 3.2 + 1.8 * t, anArm = 13 - 2 * t, e = 52 - d;
		Mask aMask = aFrame.NewMask();
		aMask.Stroke({Vec2(d, d + anArm), Vec2(d, d), Vec2(d + anArm, d)}, 3.2, false);
		aMask.Stroke({Vec2(e - anArm, d), Vec2(e, d), Vec2(e, d + anArm)}, 3.2, false);
		aMask.Stroke({Vec2(d, e - anArm), Vec2(d, e), Vec2(d + anArm, e)}, 3.2, false);
		aMask.Stroke({Vec2(e - anArm, e), Vec2(e, e), Vec2(e, e - anArm)}, 3.2, false);
		aFrame.Shadow(aMask, 0, 0, 2.2, p.mGlow.WithAlpha((float) (0.55 + 0.35 * t)));
		aFrame.Fill(aMask.Grown(0.9), Rgba(0, 0, 0, 0.6f));
		aFrame.Fill(aMask, Linear(Vec2(0, 0), Vec2(52, 52), {{0, Rgba(1, 1, 1)}, {1, Lighter(p.mGlow, 0.3)}}));
		c.Draw(aFrame, f * 52, 0);
	}
}

// The dialog box: a 3 x 3 grid of 32 px pieces whose middle row and column repeat, so everything but the corners
// is flat along its edge
void DrawDialogBox(Canvas& c)
{
	const Palette& p = P();
	Mask aBox = ShapeMask(c, Path().RoundRect(2, 2, 88, 88, 12));
	c.Shadow(aBox, 3, 4, 2.5, Rgba(0, 0, 0, 0.5f));
	c.Fill(aBox, Mix(p.mPanel, p.mPanelDark, 0.3));
	Emboss(c, aBox, 2.0, 1.0, Rgba(1, 1, 1, 0.4f), Rgba(0, 0, 0, 0.5f));
	c.Fill(EdgeOf(aBox, 1.2), Darker(p.mPanelDark, 0.5).WithAlpha(0.85f));
	Path anInner;
	anInner.RoundRect(8, 8, 76, 76, 7);
	Mask aLine = c.NewMask();
	aLine.StrokePath(anInner, 1.0);
	c.Fill(aLine, p.mGlow.WithAlpha(0.45f));
}

// The dialog button: 32 x 10 corners; the middle rows (10..21) and columns (32..63) repeat
void DrawBoxButton(Canvas& c)
{
	const Palette& p = P();
	Mask aFace = ShapeMask(c, Path().RoundRect(1, 1, 94, 30, 8));
	c.Fill(aFace, Mix(p.mPanel, p.mPanelLight, 0.28));
	c.Fill(aFace, [](double, double y) {
		if (y < 10)
			return Rgba(1, 1, 1, (float) (0.4 * (10 - y) / 9));
		if (y >= 22)
			return Rgba(0, 0, 0, (float) (0.4 * (y - 22) / 9));
		return Rgba(0, 0, 0, 0);
	});
	c.Fill(EdgeOf(aFace, 1.0), Darker(p.mPanelDark, 0.4).WithAlpha(0.8f));
}

void DrawWaitBar(Canvas& c)
{
	Capsule(c, 0, 0, 32, 16, P().mBar, Darker(P().mBar, 0.6));
}

void DrawHintGlow(Canvas& c)
{
	const Palette& p = P();
	// over the orb at (26.5, 36): a sheen and the word
	double cx = 26.5, cy = 36;
	c.Fill(Path().Ellipse(cx, cy, 25.5, 25.5), Radial(Vec2(cx, cy - 4), 26, 26,
		{{0, Rgba(1, 1, 1, 0.55f)}, {0.7, Rgba(1, 1, 1, 0.22f)}, {1, Rgba(1, 1, 1, 0)}}));
	LabelStyle aLabel;
	aLabel.mText = Caps(10, 1.8, 1.0, 0.9);
	aLabel.mText.mCap = FitCap(Text("CODEART_HINT", "HINT"), aLabel.mText, 44, 10);
	aLabel.mOutline = 1.2;
	aLabel.mOutlineColor = Darker(p.mPanelDark, 0.4).WithAlpha(0.9f);
	aLabel.mGlow = 3;
	aLabel.mGlowColor = p.mGlow.WithAlpha(0.9f);
	Label(c, Text("CODEART_HINT", "HINT"), cx, cy + aLabel.mText.mCap / 2, aLabel);
}

// 3 slices across: the groove's rounded ends in the outer 32 px, the middle flat
void DrawSliderTrack(Canvas& c)
{
	Well(c, 3, 12, 90, 8, 4);
}

void DrawSliderThumb(Canvas& c)
{
	Ring(c, 15.5, 15.5, 10.5, 13.5, P());
	Ball(c, 15.5, 15.5, 11, P().mGlow);
}

// Left half checked, right half not
void DrawCheckbox(Canvas& c)
{
	const Palette& p = P();
	for (int h = 0; h < 2; h++)
	{
		double ox = h * 31;
		Well(c, ox + 4, 5, 23, 23, 5);
		if (h != 0)
			continue;
		Mask aTick = c.NewMask();
		aTick.Stroke({Vec2(ox + 9.5, 16.5), Vec2(ox + 14, 21.5), Vec2(ox + 22, 10.5)}, 3.4, false);
		c.Shadow(aTick, 0, 0, 2, p.mGlow.WithAlpha(0.8f));
		c.Fill(aTick.Grown(0.8), Rgba(0, 0, 0, 0.55f));
		c.Fill(aTick, Linear(Vec2(0, 10), Vec2(0, 22), {{0, Lighter(p.mGlow, 0.5)}, {1, p.mGlow}}));
	}
}

// Five textures, two 51 px cells each, light grey: the game colours them per level
void DrawCheckerTextures(Canvas& c)
{
	for (int t = 0; t < 5; t++)
	{
		for (int k = 0; k < 2; k++)
		{
			Canvas aCell(51, 51, c.mScale);
			uint32_t aSeed = 100 + t * 2 + k;
			aCell.FillAll([t, aSeed](double x, double y) {
				double v;
				switch (t)
				{
				case 0:   // clouds
					v = 0.84 + 0.13 * Fbm(x / 16.0, y / 16.0, aSeed, 4);
					break;
				case 1:   // marble
					v = 0.82 + 0.12 * std::sin((x + y) / 6.0 + 3.0 * Fbm(x / 18.0, y / 18.0, aSeed, 3));
					break;
				case 2:   // brushed
					v = 0.84 + 0.12 * Fbm(x / 30.0, y / 1.6, aSeed, 3);
					break;
				case 3:   // ripples
					v = 0.83 + 0.1 * std::sin(std::hypot(x - 10, y - 8) / 2.6 + 2.0 * Fbm(x / 22.0, y / 22.0, aSeed, 2));
					break;
				default:  // grain
					v = 0.82 + 0.08 * Fbm(x / 5.0, y / 5.0, aSeed, 3) + 0.06 * Fbm(x / 20.0, y / 20.0, aSeed + 7, 2);
					break;
				}
				float g = (float) std::max(0.0, std::min(1.0, v));
				return Rgba(g, g, g);
			});
			Mask aTile = ShapeMask(aCell, Path().Rect(0, 0, 51, 51));
			Emboss(aCell, aTile, 1.2, 0.8, Rgba(1, 1, 1, 0.35f), Rgba(0, 0, 0, 0.18f));
			c.Draw(aCell, t * 102 + k * 51, 0);
		}
	}
}

// The level-complete vortex, 9 frames, drawn additively: four arms turned a ninth of a quarter turn a frame
void DrawBlackHole(Canvas& c)
{
	const Palette& p = P();
	Rgba aColor = p.mVortex;
	for (int f = 0; f < 9; f++)
	{
		Canvas aFrame(256, 256, c.mScale);
		double aPhase = f * (kPi / 2) / 9;
		aFrame.FillAll(Sampled([aPhase, aColor](double x, double y) {
			double dx = x - 128, dy = y - 128;
			double r = std::hypot(dx, dy);
			double a = std::atan2(dy, dx);
			double s = std::cos(4 * (a - aPhase) + 3.2 * std::log(1 + r / 18.0));
			double anArm = std::pow(0.5 + 0.5 * s, 5);
			double i = anArm * Smooth(3, 28, r) * (1 - Smooth(72, 122, r));
			double aWhite = std::max(0.0, i - 0.75) * 2.0;
			return Rgba((float) std::min(1.0, aColor.r * i * 1.2 + aWhite), (float) std::min(1.0, aColor.g * i * 1.2 + aWhite),
				(float) std::min(1.0, aColor.b * i * 1.2 + aWhite), 1.0f);
		}, 256, 256, 1));
		c.Draw(aFrame, f * 256, 0);
	}
}

// The vortex's dark heart, coloured by the game's alpha
void DrawHoleMask(Canvas& c)
{
	Rgba aCore = Darker(P().mVortex, 0.9);
	c.FillAll([aCore](double x, double y) {
		double r = std::hypot(x - 64, y - 64);
		double a = 1 - Smooth(8, 62, r);
		return aCore.WithAlpha((float) a);
	});
}

// The wipe's line of sparks: drawn every 128 px, so it repeats every 128
void DrawElect(Canvas& c)
{
	const Palette& p = P();
	Canvas aWide(384, 16, c.mScale);
	for (int aStrand = 0; aStrand < 2; aStrand++)
	{
		std::vector<Vec2> aLine;
		for (int i = 0; i <= 384 * 2; i++)
		{
			double x = i * 0.5, u = 2 * kPi * x / 128;
			double y = 8 + ((aStrand == 0) ? 2.6 * std::sin(3 * u + 0.4) + 1.6 * std::sin(7 * u + 1.9) + 0.9 * std::sin(13 * u + 0.7)
				: 2.2 * std::sin(4 * u + 2.1) + 1.4 * std::sin(9 * u + 0.3) + 0.8 * std::sin(17 * u + 2.6));
			aLine.push_back(Vec2(x, y));
		}
		Mask aCore = aWide.NewMask();
		aCore.Stroke(aLine, (aStrand == 0) ? 1.2 : 0.8, false);
		aWide.Shadow(aCore.Grown(0.6), 0, 0, 2.0, p.mVortex.WithAlpha(0.9f), BLEND_ADD);
		aWide.Fill(aCore, Rgba(1, 1, 1, (aStrand == 0) ? 1.0f : 0.7f), 1, BLEND_ADD);
	}
	c.Draw(aWide, -128, 0);
}

// The hint's sparkle, 14 frames of 40 px: a star that grows and fades, white on black (drawn additively)
void DrawSparkle(Canvas& c)
{
	c.FillAll(Solid(Rgba(0, 0, 0)));
	for (int f = 0; f < 14; f++)
	{
		double s = std::sin(kPi * (f + 0.5) / 14);
		double cx = f * 40 + 20, cy = 20;
		Path aStar;
		aStar.Star(cx, cy, 4, 3 + 15 * s, 0.9 + 1.4 * s, 90);
		Path aSmall;
		aSmall.Star(cx, cy, 4, 1.5 + 7 * s, 0.7 + 0.8 * s, 45);
		Mask aMask = ShapeMask(c, aStar.Add(aSmall));
		c.Shadow(aMask, 0, 0, 1.5 + 2 * s, Rgba(1, 1, 1, (float) (0.5 + 0.4 * s)), BLEND_ADD);
		c.Fill(aMask, Rgba(1, 1, 1), (float) (0.6 + 0.4 * s), BLEND_ADD);
	}
}

void DrawEditBox(Canvas& c)
{
	Well(c, 1, 2, 94, 28, 6);
}

// A tutorial bubble: theShape filled in the glow colour with a white rim
void Bubble(Canvas& c, const Mask& theShape)
{
	const Palette& p = P();
	Mask aRim = theShape.Grown(1.6);
	c.Shadow(aRim, 1.5, 2.5, 2, Rgba(0, 0, 0, 0.45f));
	c.Fill(aRim.Grown(0.9), Darker(p.mPanelDark, 0.4).WithAlpha(0.7f));
	c.Fill(aRim, Rgba(1, 1, 1));
	c.Fill(theShape, Linear(Vec2(0, 0), Vec2(0, c.mH), {{0, Mix(p.mPanel, p.mPanelLight, 0.2)}, {1, Mix(p.mPanel, p.mPanelDark, 0.3)}}));
}

void BubbleText(Canvas& c, const std::string& theText, double cx, double theTop, double theBottom, double theWidth, double theMaxCap)
{
	std::vector<std::string> aLines = SplitLines(theText);
	LabelStyle aLabel;
	aLabel.mText = Caps(theMaxCap, 1.7, 1.0, 0.9);
	for (const std::string& aLine : aLines)
		aLabel.mText.mCap = std::min(aLabel.mText.mCap, FitCap(aLine, aLabel.mText, theWidth, theMaxCap));
	double aCap = std::min(aLabel.mText.mCap, (theBottom - theTop) / (aLines.size() * 1.5 - 0.5));
	aLabel.mText.mCap = aCap;
	aLabel.mOutline = 1.0;
	aLabel.mOutlineColor = Darker(P().mPanelDark, 0.4).WithAlpha(0.9f);
	aLabel.mTop = Rgba(1, 1, 1);
	aLabel.mBottom = Lighter(P().mGlow, 0.3);
	double aHeight = aCap * (aLines.size() * 1.5 - 0.5);
	double y = (theTop + theBottom - aHeight) / 2 + aCap;
	for (const std::string& aLine : aLines)
	{
		Label(c, aLine, cx, y, aLabel);
		y += aCap * 1.5;
	}
}

// Under a pair of gems, pointing up at both (the gems' centres are at x 21.5 and 72.5)
void DrawSwapper(Canvas& c)
{
	Path aShape;
	aShape.RoundRect(4, 23, 92, 47, 10);
	for (double x : {21.5, 72.5})
	{
		aShape.Polygon({Vec2(x, 3), Vec2(x + 9, 14), Vec2(x - 9, 14)});
		aShape.Rect(x - 3.5, 12, 7, 13);
	}
	Bubble(c, ShapeMask(c, aShape));
	BubbleText(c, Text("CODEART_SWAP", "SWAP THESE|GEMS!"), 50, 27, 66, 82, 11);
}

// Above a gem, pointing down at it (its centre is at x 29.5)
void DrawClickArrow(Canvas& c)
{
	Path aShape;
	aShape.RoundRect(8, 3, 43, 26, 6);
	aShape.Polygon({Vec2(3, 25), Vec2(56, 25), Vec2(29.5, 52)});
	Bubble(c, ShapeMask(c, aShape));
	BubbleText(c, Text("CODEART_CLICK", "CLICK"), 29.5, 6, 26, 38, 10);
}

struct ImageSpec
{
	const char* mName;
	int mW, mH;
	int mMaxScale;          // soft images need less
	void (*mDraw)(Canvas&);
};

void DrawNewGameDown(Canvas& c) { DrawSideButtonImage(c, kNewGame, BUTTON_DOWN); }
void DrawNormalDown(Canvas& c) { DrawSideButtonImage(c, kNormal, BUTTON_DOWN); }
void DrawNormalLit(Canvas& c) { DrawSideButtonImage(c, kNormal, BUTTON_LIT); }
void DrawTimedDown(Canvas& c) { DrawSideButtonImage(c, kTimed, BUTTON_DOWN); }
void DrawTimedLit(Canvas& c) { DrawSideButtonImage(c, kTimed, BUTTON_LIT); }
void DrawOptionsDown(Canvas& c) { DrawSideButtonImage(c, kOptions, BUTTON_DOWN); }
void DrawQuitDown(Canvas& c) { DrawSideButtonImage(c, kQuit, BUTTON_DOWN); }

// Every image of gImageNames (Res.h) but the gem sheet, which GemArt draws, and the fonts (CodeArt.cpp)
const ImageSpec kImages[] = {
	{"titlescreen6", 640, 480, 4, DrawTitleScreen},
	{"titlegreenbar2", 340, 24, 4, DrawLoadingBar},
	{"trialcircle2", 105, 107, 4, DrawTrialCircle},
	{"cursor1", 54, 54, 4, DrawArrowCursor},
	{"pointer2", 58, 58, 4, DrawHandCursor},
	{"overhand", 54, 54, 4, DrawGrabCursor},
	{"typer", 54, 54, 4, DrawTextCursor},
	{"alphaback1", 640, 480, 4, DrawAlphaback},
	{"target1", 312, 52, 4, DrawTarget},
	{"dialogbox2", 96, 96, 4, DrawDialogBox},
	{"boxbutton2", 96, 32, 4, DrawBoxButton},
	{"waitbar", 32, 16, 4, DrawWaitBar},
	{"over2-newgame-pushed1", 156, 41, 4, DrawNewGameDown},
	{"over3-normal-pushed1", 78, 31, 4, DrawNormalDown},
	{"over7-normal-selected1", 78, 31, 4, DrawNormalLit},
	{"over4-timetrial-pushed1", 78, 31, 4, DrawTimedDown},
	{"over8-timetrial-selected1", 78, 31, 4, DrawTimedLit},
	{"over5-options-pushed1", 156, 41, 4, DrawOptionsDown},
	{"over6-quit-pushed1", 156, 40, 4, DrawQuitDown},
	{"timerbar", 342, 25, 4, DrawTimerBar},
	{"sparkle", 560, 40, 2, DrawSparkle},
	{"over1-hint1", 52, 72, 4, DrawHintGlow},
	{"slider-track", 96, 32, 4, DrawSliderTrack},
	{"slider-thumb", 31, 31, 4, DrawSliderThumb},
	{"checkbox1", 62, 32, 4, DrawCheckbox},
	{"checkertextures2", 510, 51, 4, DrawCheckerTextures},
	{"blackhole", 2304, 256, 2, DrawBlackHole},
	{"holemask", 128, 128, 2, DrawHoleMask},
	{"elect", 128, 16, 4, DrawElect},
	{"editbox", 96, 32, 4, DrawEditBox},
	{"swapper1", 100, 75, 4, DrawSwapper},
	{"swapper2", 58, 55, 4, DrawClickArrow},
};

} // namespace

const std::vector<std::string>& GeneratedImageNames()
{
	static std::vector<std::string> sNames;
	if (sNames.empty())
	{
		for (const ImageSpec& anImage : kImages)
			sNames.push_back(anImage.mName);
		sNames.push_back("gemsheet6");
	}
	return sNames;
}

bool GenerateImage(const std::string& theName, Platform::LoadedImage& theOut)
{
	if (theName == "gemsheet6")
	{
		DrawGemSheetImage(theOut);
		return true;
	}
	for (const ImageSpec& anImage : kImages)
	{
		if (theName != anImage.mName)
			continue;
		Canvas aCanvas = NewCanvas(anImage.mW, anImage.mH, anImage.mMaxScale);
		anImage.mDraw(aCanvas);
		ToLoadedImage(aCanvas, theOut);
		return true;
	}
	return false;
}

}
