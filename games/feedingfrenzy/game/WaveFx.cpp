#include "WaveFx.h"

using namespace feedingfrenzy;

namespace
{
// The effect's type that is the waves (the blitters choose their loop by it)
const int WAVE_EFFECT_TYPE = 1;
// What a new effect has before the stage sets them
const float DEFAULT_SIZE = 5.0f;
const float PI = 3.1415927f;
const float DEFAULT_AMOUNT_OF_PI = 0.02f;
}

// 0x44df20
WaveFx::WaveFx() : mTime(0.0f), mSize(DEFAULT_SIZE), mAmount(PI * DEFAULT_AMOUNT_OF_PI)
{
	mType = WAVE_EFFECT_TYPE;
}
