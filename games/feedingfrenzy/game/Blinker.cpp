#include "Blinker.h"

using namespace engine;

namespace feedingfrenzy
{

// The seconds between two changes that a new blinker has
const float DEFAULT_INTERVAL = 1.0f;
// The time to the next change of a blinker that is not blinking (it changes at the first frame of its blinking)
const float IDLE_COUNTDOWN = -1.0f;

// 0x42a5f0
Blinker::Blinker(Image* theImage)
    : ImageNode(theImage), mInterval(DEFAULT_INTERVAL), mCountdown(IDLE_COUNTDOWN), mBlinking(true)
{
	setFlags(NODE_DRAWABLE | NODE_LISTENERS);
}

// 0x42a470
void Blinker::update(UpdateInfo& theInfo)
{
	if (mBlinking)
	{
		mCountdown -= theInfo.mSeconds;
		if (mCountdown < 0.0f)
		{
			setVisible(!isVisible());
			mCountdown = mInterval;
		}
	}
}

// 0x42a4d0
void Blinker::start()
{
	setVisible(true);
	mBlinking = true;
	mCountdown = mInterval;
}

// 0x42a4f0
void Blinker::stop()
{
	setVisible(false);
	mCountdown = IDLE_COUNTDOWN;
	mBlinking = false;
}

} // namespace feedingfrenzy
