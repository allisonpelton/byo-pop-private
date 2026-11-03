#ifndef __WORDHARMONY_ENGINE_RANDOM_H__
#define __WORDHARMONY_ENGINE_RANDOM_H__

// The original's random numbers: the C library's rand() and srand() (its single-threaded library: one global
// holdrand 0x49729c, holdrand = holdrand * 214013 + 2531011, the result (holdrand >> 16) & 0x7fff, holdrand 1 at the
// start), and the engine's two ranges over them. The host's rand() differs on Emscripten, so the port never calls it.

#include "Geometry.h"

namespace engine
{

// 0x44673f
void crtSrand(unsigned int theSeed);
// 0x446749
int crtRand();
// port: the generator's state (0x49729c), for the identical-play runs' state log
unsigned int crtRandState();

// 0x42bce0: a whole number from theLow towards theHigh, cut towards theLow; theHigh itself never comes (rand()'s
// largest value times the scale is a little under 1)
int randomInt(int theLow, int theHigh);
// 0x401050: the same over theLow to theHigh + 1, so theHigh is the largest it gives
int randomIndex(int theLow, int theHigh);
// 0x42bd10: a number between a range's two ends (x and y)
float randomFloat(const Point& theRange);

} // namespace engine

#endif
