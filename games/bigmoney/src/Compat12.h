#ifndef __BIGMONEY_COMPAT12_H__
#define __BIGMONEY_COMPAT12_H__

// SexyAppFramework 1.2 and MSVC 7.0 behaviour that Big Money depends on and the port's (1.3-based)
// framework or the host C library do differently. Game code uses these instead of the port's
// versions; the shared framework is unchanged. games/bigmoney/docs/TRANSLATION_PLAN.md lists every
// 1.2 -> 1.3 difference that was checked.

#include "Slider.h"

namespace Sexy
{

// The original's C library random numbers: MSVC's rand()/srand() (per-thread
// holdrand = holdrand * 214013 + 2531011, result (holdrand >> 16) & 0x7fff, initial holdrand 1).
// Board, GameSelector and the other game code call rand(); SexyAppBase::Init seeds the main thread
// with srand(GetTickCount()) (WinBMApp::Init reseeds this emulation the same way, or with -seed=N
// for test runs). The host's rand() is different on Emscripten, so the port never calls it.
void	CrtSRand(unsigned int theSeed);
int		CrtRand();

// 1.2's Slider: a click on the track outside the thumb does nothing (1.3 jumps the thumb there), and
// SetValue does not clamp. Everything else (drawing, dragging, listener calls) is the same.
class Slider12 : public Slider
{
public:
	Slider12(Image* theTrackImage, Image* theThumbImage, int theId, SliderListener* theListener);

	virtual void	SetValue(double theValue) override;
	virtual void	MouseDown(int x, int y, int theClickCount) override;
};

}

#endif
