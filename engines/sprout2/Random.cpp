#include "Random.h"

namespace engine
{

// The constant the original multiplies the draw by, a float a little under 1/32767
static const float RAND_SCALE = 3.051851e-05f;

// Each thread's state (the CRT's _holdrand), 1 when the thread starts
static unsigned int gHoldRand[RANDOM_THREAD_COUNT] = {1, 1, 1};
static RandomThread gRandomThread = RANDOM_THREAD_GAME;

int crtRand()
{
	unsigned int& aState = gHoldRand[gRandomThread];
	aState = aState * 214013u + 2531011u;
	return (int)((aState >> 16) & 0x7fff);
}

void setRandomThread(RandomThread theThread)
{
	gRandomThread = theThread;
}

RandomThread getRandomThread()
{
	return gRandomThread;
}

void resetRandomThread(RandomThread theThread)
{
	gHoldRand[theThread] = 1;
}

unsigned int getRandomState()
{
	return gHoldRand[gRandomThread];
}

double randomFloat(float theLow, float theHigh)
{
	int aDraw = crtRand();
	return (double)aDraw * RAND_SCALE * ((double)theHigh - theLow) + theLow;
}

} // namespace engine
