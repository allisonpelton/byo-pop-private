#ifndef __FEEDINGFRENZY_SCREENFADER_H__
#define __FEEDINGFRENZY_SCREENFADER_H__

// A rectangle of one colour over the screen that fades in and out (ScreenFader, 0x130 bytes, constructor 0x435ca0,
// vtable 0x4a6eb8): the controller's black cover for the changes between the game's parts. It is opaque at first;
// fadeFromColor takes the cover away over the fade-in time, fadeToColor brings it over the fade-out time.

#include "Color4.h"
#include "Node.h"

namespace feedingfrenzy
{

class ScreenFader : public engine::Node
{
public:
	ScreenFader();  // 0x435ca0

	void draw(engine::Graphics* g) override;     // 0x78: 0x435bc0
	void layout() override;                      // 0x80: 0x435c30
	void update(engine::UpdateInfo& theInfo) override;  // 0x7c: 0x435a60

	void setRect(const engine::Rect& theRect) { mRect = theRect; }  // 0xd4: 0x435950
	// 0xd8: 0x435980: a fade-out time under 0 is the fade-in time
	void setDurations(float theFadeIn, float theFadeOut)
	{
		mFadeInSeconds = theFadeIn;
		mFadeOutSeconds = 0.0f <= theFadeOut ? theFadeOut : theFadeIn;
	}
	// 0xdc: 0x4359b0
	void setCoverColor(float theRed, float theGreen, float theBlue)
	{
		mCoverColor.r = theRed;
		mCoverColor.g = theGreen;
		mCoverColor.b = theBlue;
	}
	void fadeFromColor();                        // 0xe0: 0x4359c0: the cover goes (opaque to clear)
	void fadeToColor();                          // 0xe4: 0x435a00: the cover comes (clear to opaque)
	bool isIdle() const { return mState == STATE_CLEAR || mState == STATE_COVERED; }  // 0xe8: 0x435a40

private:
	enum State
	{
		STATE_CLEAR = 0,
		STATE_COVERED = 1,
		STATE_FADING_FROM = 2,
		STATE_FADING_TO = 3
	};

	engine::Rect mRect;           // +0xfc
	engine::Color4 mCoverColor;   // +0x10c
	float mFadeInSeconds;         // +0x118
	float mFadeOutSeconds;        // +0x11c
	float mSecondsLeft;           // +0x120
	int mState;                   // +0x124
};

} // namespace feedingfrenzy

#endif
