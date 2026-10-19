#ifndef __COMPAT2004_H__
#define __COMPAT2004_H__

// The early-2004 SexyAppFramework (BookWorm Deluxe, Typer Shark, AstroPop) and MSVC 7.0 behaviours that the
// games of that generation depend on and the port's 1.3-based framework or the host C library do
// differently. Game code uses these instead of the port's versions; the framework itself is unchanged.
// docs/FRAMEWORK_2004.md section 8 lists every difference and how it is handled. Library: sexycompat2004
// (framework/CMakeLists.txt); a game links it to opt in. Addresses in the comments are those of BookWorm
// Deluxe 1.03's game image, where the map was derived.

#include "Dialog.h"
#include "Slider.h"
#include "MTRand.h"
#include "Rect.h"

namespace Sexy
{

class WidgetManager;
class Graphics;

// The original's C library random numbers: MSVC's rand()/srand() (holdrand = holdrand * 214013 + 2531011,
// result (holdrand >> 16) & 0x7fff, initial holdrand 1). The 2004 SexyApp::Init seeds the main thread
// with srand(GetTickCount()); a game's Init reseeds this emulation the same way (or with -seed=N in test
// runs). The host's rand() differs on Emscripten, so the ports never call it.
void	CrtSRand(unsigned int theSeed);
int		CrtRand();

// The 2004 (SAF 1.2) MTRand::SRand(seed): mt[0] = seed (4357 for 0), mt[i] = mt[i-1] * 69069, next draw
// regenerates the block. The port's MTRand(seed)/SRand(seed) use 1.3's initialisation, which gives other
// numbers, so games seed their own generators with this.
void	SeedMTRand2004(MTRand& theRand, unsigned long theSeed);

// The 2004 Dialog's buttons (docs/FRAMEWORK_2004.md section 3, D1): yes/ok/footer is `mId + 2000` and
// no/cancel `mId + 3000`, labelled with the plain "YES"/"NO"/"OK"/"CANCEL" (or the footer) strings, and
// their ButtonPress/ButtonDepress/ButtonDownTick go to the app's ButtonListener with those ids (the app
// decodes 2000+dialog id / 3000+dialog id). There is no DialogListener and no mResult. A 2004 game's
// dialogs derive from this instead of Dialog; dialogs that override ButtonPress/ButtonDepress compare
// `theId == mId + 2000` as the originals do and call these.
class Dialog2004 : public Dialog
{
public:
	enum
	{
		ID_YES_2004 = 2000,  // + mId
		ID_NO_2004 = 3000    // + mId
	};

	Dialog2004(Image* theComponentImage, Image* theButtonComponentImage, int theId, bool isModal,
		const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter,
		int theButtonMode);

	virtual void	ButtonPress(int theId);
	virtual void	ButtonDepress(int theId);
	virtual void	ButtonDownTick(int theId);
	// The 2004 layout wraps mDialogLines with the 2004 word wrap (WriteWordWrapped2004); otherwise the same
	// as the port's (no grey fallback box: every 2004 dialog has a component image)
	virtual void	Draw(Graphics* g);
	virtual int		GetPreferredHeight(int theWidth);

	using Dialog::ButtonPress;
};

// The 2004 Slider: a click on the track outside the thumb does nothing (1.3 jumps the thumb there), and
// SetValue does not clamp. Drawing, dragging and the listener calls are the same.
class Slider2004 : public Slider
{
public:
	Slider2004(Image* theTrackImage, Image* theThumbImage, int theId, SliderListener* theListener);

	virtual void	SetValue(double theValue);
	virtual void	MouseDown(int x, int y, int theClickCount);
	using Slider::MouseDown;
};

// The 2004 text writers. In this framework they are Widget virtuals taking a Graphics* (slots 36-38,
// docs/FRAMEWORK_2004.md section 3, 1.4); the port's Graphics methods are SAF 1.3's, which differ in ways
// game texts reach: after a wrap at a space the next line starts at the very next character (1.3 skips
// every further space; 41 of BookWorm's word definitions have double spaces), a trailing newline adds no
// height, and a '^' with fewer than 8 characters left ends the string even before a "^^". Colour codes
// `^RRGGBB^` (6 hex digits; an invalid digit counts 0) keep the current alpha; the colour is not restored
// by WriteString2004 and is restored by WriteWordWrapped2004. Justification: 0 centre, 1 right, else left.
// 0x43b420 (Widget slot 36)
int		WriteString2004(Graphics* g, const SexyString& theString, int theX, int theY, int theWidth = -1,
			int theJustification = -1, bool drawString = true, int theOffset = 0, int theLength = -1);
// 0x439f70 (Widget slot 37); returns the height written (descent + last baseline offset)
int		WriteWordWrapped2004(Graphics* g, const Rect& theRect, const SexyString& theLine, int theLineSpacing,
			int theJustification);
// 0x4389f0 (Widget slot 38): WriteWordWrapped2004 measured with g's font, Rect(0, 0, theWidth, 0), left
int		GetWordWrappedHeight2004(Graphics* g, int theWidth, const SexyString& theLine, int theLineSpacing);

// The game's GetTickCount(): SexyAppBase::GetClockTime() (real milliseconds, or the virtual clock that -fast
// advances), so timing a game reads (level clocks, idle timers) repeats in scripted runs. The port's
// GetTickCount() is real time even under -fast.
DWORD	GameTickCount();

// GetTickCount() of the last mouse press anywhere (the 2004 WidgetManager's field at +0x168, set by every
// WidgetManager::MouseDown; 0 until the first press): WidgetManager::mLastMouseDownTick, which the framework
// sets from GetClockTime(). Compare it with GameTickCount() (BookWorm: Lex's 12-second idle test).
DWORD	GetLastMouseDownTick(WidgetManager* theWidgetManager);

}

#endif
