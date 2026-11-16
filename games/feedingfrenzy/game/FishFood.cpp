#include "FishFood.h"

using namespace feedingfrenzy;

namespace
{
const char* const WINDOW_TITLE = "Feeding Frenzy";
}

FishFood::FishFood() {}

FishFood::~FishFood() {}

// 0x42b8e0: the window's title, icon, size and frame rate
void FishFood::configure(engine::AppConfig& theConfig)
{
	theConfig.mTitle = WINDOW_TITLE;
	theConfig.mIcon = WINDOW_ICON;
	theConfig.mWidth = SCREEN_WIDTH;
	theConfig.mHeight = SCREEN_HEIGHT;
	theConfig.mFrameRate = FRAME_RATE;
}

// 0x430b80: not translated yet
void FishFood::init() {}

void FishFood::shutdown() {}
