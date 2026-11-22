// SoundQueue: how the game plays its sounds (0x46c7e0..0x46c9d0 in the original). A request only marks the sound's
// cue; the queue's update, once a frame, plays each marked cue that has waited out its delay and whose last play is
// at least five frames old, so one sound asked for many times in a frame plays once. A cue may pitch its sound up by
// a random number of steps, drawn from the game's own generator.
#pragma once

namespace Sexy
{

class SexyAppBase;
class SoundInstance;

// The sounds' ids run from 1 to 0x60 (SOUND_FXSTART and on, properties/resources.xml)
const int SOUND_QUEUE_SLOTS = 0x62;

struct SoundCue
{
	int mSoundId;
	bool mPending;           // asked for, not played yet
	int mPitchRange;         // up to this many steps higher, at random
	int mDelay;              // frames still to wait before playing
	int mDelayReset;         // what mDelay becomes after each play
	bool mLoop;
	int mCooldown;           // frames since the last play, counted down from 5
	SoundInstance* mLoopInstance; // the looping cue's instance, kept to stop it
};

class SoundQueue
{
public:
	int mFrame;              // the updates run
	SexyAppBase* mApp;
	int mFirstId;
	int mLastId;
	SoundCue* mCues[SOUND_QUEUE_SLOTS];

	SoundQueue(SexyAppBase* theApp);
	~SoundQueue();

	void Request(int theSoundId, int thePitchRange, bool loop);
	void StopLoop(int theSoundId);
	void DelayFillSound();
	bool Update();
};

} // namespace Sexy
