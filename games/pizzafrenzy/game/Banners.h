#ifndef __PIZZAFRENZY_BANNERS_H__
#define __PIZZAFRENZY_BANNERS_H__

// The game screen's two banners and the effect they are made of. The message banner (0x138 bytes, constructor
// 0x42bb30, vtable 0x4fbec0; the screen keeps it at +0x208) shows a line of text that grows over a delivery, drifts up
// and fades: a frenzy's bonus. The speed bonus banner (0x15c bytes, constructor 0x44fb70, vtable 0x4ffe48; +0x204) is
// a dark strip with "SPEED BONUS!" that flies across the screen; nothing in the exe starts it (its flag at +0x150 is
// only ever cleared), so it stays hidden.

#include "Interpolator.h"
#include "FadeContainer.h"
#include "Node.h"
#include "Ramp.h"
#include "TextItem.h"
#include <map>
#include <string>

namespace pizzafrenzy
{

// The sounds of an effect, each at a time (0x24 bytes, constructor 0x42d440, vtable 0x4fc1e4): updated with the
// effect's time, it plays each one the time has passed, once (and rewinds when the time goes back)
class SoundTrack
{
public:
	// 0x42d320 (via 0x42c190): a sound at a time; the track starts over
	void add(float theTime, const std::string& theName);
	// 0x42cb30: the sounds the time has passed are played, volume and pitch 1
	void update(float theTime);

private:
	std::map<float, std::string> mSounds;                   // +0x0c
	std::map<float, std::string>::iterator mNext = mSounds.begin();  // +0x18: the next sound to play
};

// A node that is shown by values over time (0x388 bytes, constructor 0x42c980, vtable 0x4fc000): its scale, opacity
// and position follow keys while its time runs, first those of the way in and then, when told, those of the way out
class InOutEffect : public engine::Container
{
public:
	InOutEffect();  // 0x42c980

	// 0x94: 0x42c4c0 (the states are read at "State" below)
	void update(engine::UpdateInfo& theInfo) override;
	// 0x42c950: the way in is ready at its start, and the effect is idle; a finished effect removes itself when
	// theRemoveWhenDone
	void reset(bool theRemoveWhenDone);
	// 0x42c690: the way in is ready at its start, and the effect is idle
	void rewind();
	// 0x42c6b0: the way in plays (listening for updates), from its start unless it is playing
	void start();
	// 0x42c260: the way out has been played
	bool isDone() const { return mState == STATE_DONE; }

	engine::ScaleInterpolator mScaleIn;      // +0x128: the keys of the way in
	engine::ScaleInterpolator mScaleOut;     // +0x178: the way out's
	engine::OpacityInterpolator mOpacityIn;  // +0x1c8
	engine::OpacityInterpolator mOpacityOut; // +0x218
	engine::PositionInterpolator mMoveIn;    // +0x268
	engine::PositionInterpolator mMoveOut;   // +0x2b8
	SoundTrack mSoundsIn;                    // +0x308
	SoundTrack mSoundsOut;                   // +0x32c

private:
	enum State
	{
		STATE_IDLE = 0,
		STATE_IN = 1,        // the way in plays
		STATE_HOLD = 2,      // it has played and the effect waits
		STATE_OUT_WAIT = 3,  // nothing in the exe sets it: the way in plays, then the way out is made ready
		STATE_OUT = 4,       // the way out plays
		STATE_DONE = 5       // it has played
	};

	// 0x42c3a0: the way in is ready at its start (its time runs as long as its longest key list)
	void readyIn();
	// 0x42c430: the way out is
	void readyOut();
	// 0x42c1a0: the way in takes its values at the time that has come
	void applyIn();
	// 0x42c200: the way out does
	void applyOut();

	engine::Ramp mTime;           // +0x350: the time of the way being played
	int mState;                   // +0x378
	bool mRemoveWhenDone;         // +0x37c
};

class MessageBanner : public engine::Container
{
public:
	MessageBanner();  // 0x42bb30

	// 0x42bb00: the line shown
	void setText(const std::string& theText) { mText->setText(theText); }
	// 0x42bb10: the effect starts from its beginning
	void play()
	{
		mEffect->rewind();
		mEffect->start();
	}

private:
	engine::Ref<InOutEffect> mEffect;  // +0x128
	engine::Ref<engine::TextItem> mText;  // +0x12c
};

class SpeedBonusBanner : public engine::FadeContainer
{
public:
	SpeedBonusBanner();  // 0x44fb70
	// 0x44fd10: the strip, its effect and its text are made, and the banner is hidden
	void setup();
	// 0x44fcd0: a banner that is waiting for its effect's end fades out when it comes
	void updateBanner();

private:
	engine::Ref<engine::TextItem> mBlankLabel;  // +0x144: made, centred and added, and given no text
	engine::Ref<InOutEffect> mEffect;           // +0x148
	engine::Ref<engine::TextItem> mTitle;       // +0x14c: "SPEED BONUS!"
	bool mWaiting;                              // +0x150: set by nothing
};

} // namespace pizzafrenzy

#endif
