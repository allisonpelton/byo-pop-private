#ifndef __SPROUT2_RANDOM_H__
#define __SPROUT2_RANDOM_H__

// The engine's random numbers: the C library's rand() of the original (MSVC's, 0x4aa61d (feedingfrenzy2)). The
// library keeps the generator's state per thread, each thread's starting at 1, and nothing in the exe seeds it, so
// every run draws the same numbers. The splash's loading and each stage's load run on a thread of their own in the
// original and so draw from their own state; the port runs them on the game's thread and switches the state it draws
// from around them.

namespace engine
{

enum RandomThread
{
	RANDOM_THREAD_GAME = 0,     // the window's thread: the game
	RANDOM_THREAD_LOADER = 1,   // the splash's loading (FeedingFrenzyTwo::SplashState's Runnable)
	RANDOM_THREAD_STAGE_LOADER = 2,  // a stage's load (Stage::StageLoader, a new thread for each stage)
	RANDOM_THREAD_COUNT
};

// 0x4aa61d (feedingfrenzy2): 0 to 32767, from the current thread's state
int crtRand();
// The thread whose state the draws come from
void setRandomThread(RandomThread theThread);
RandomThread getRandomThread();
// The thread's state put back to a new thread's
void resetRandomThread(RandomThread theThread);
// For logs that compare a run with the original's: the current thread's state
unsigned int getRandomState();

// 0x42ab00 (feedingfrenzy2): a number from the low end towards the high one, kept at the x87's precision for the
// caller to round
double randomFloat(float theLow, float theHigh);

} // namespace engine

#endif
