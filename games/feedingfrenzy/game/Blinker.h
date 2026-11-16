#ifndef __FEEDINGFRENZY_BLINKER_H__
#define __FEEDINGFRENZY_BLINKER_H__

// A picture that blinks (Blinker, 0x114 bytes, constructor 0x42a5f0, vtable 0x4a5dd8, destructor 0x42a590): it shows
// and hides itself each time the interval has passed, from the moment it is started until it is stopped. The
// barracuda's danger sign is one.

#include "ImageNode.h"

namespace feedingfrenzy
{

class Blinker : public engine::ImageNode
{
public:
	Blinker(engine::Image* theImage);  // 0x42a5f0

	// slot 0x7c: 0x42a470: the time to the next change runs down, and at the end the picture changes
	void update(engine::UpdateInfo& theInfo) override;

	// slot 0xd4: 0x42a4c0: the seconds between a change and the next
	void setInterval(float theSeconds) { mInterval = theSeconds; }
	// slot 0xd8: 0x42a4d0: the picture shows and starts to blink
	void start();
	// slot 0xdc: 0x42a4f0: the picture is hidden and stops blinking
	void stop();

private:
	float mInterval;   // +0x100
	float mCountdown;  // +0x104: the seconds to the next change
	bool mBlinking;    // +0x108
};

} // namespace feedingfrenzy

#endif
