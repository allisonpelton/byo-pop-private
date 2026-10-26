// JeffLib::SoundPlayer (vtable 0x7c5380, code 0x6af4b0-0x6b0400; the app's, Bookworm Adventures' +0x7b4): plays
// the game's samples for it, keeping one sound instance a sample for the samples it plays again and again (the
// cached samples), and a sample from playing twice within a few updates. It also starts samples after a delay,
// plays a sample when another is done (a chain) and loops samples that it can fade out.
#pragma once

#include <map>
#include <vector>

namespace Sexy {
class SexyAppBase;
class SoundInstance;
}

namespace JeffLib {

class SoundPlayer {
public:
	enum { MAX_SAMPLES = 256 };

	Sexy::SexyAppBase* mApp;                             // +0x04
	std::map<int, int> mLastPlayed;                      // +0x08: a sample's id, the update it last started

	// A sample to start when its count has run down (the original's 12-byte elements of the vector at +0x14)
	struct DelayedSample {
		int mId;
		int mCount;   // updates left
		int mPitch;   // in semitones
	};
	// A sample to start once another is done playing (16 bytes, the vector at +0x24)
	struct ChainedSample {
		int mId;                           // the one playing
		int mNextId;                       // the one to start when it is done
		int mCount;                        // counts down once the first is done: the next starts at 0
		Sexy::SoundInstance* mInstance;    // the first's, released when the next starts
	};
	// A looping sample (the value of the map at +0x34, by id)
	struct LoopedSample {
		Sexy::SoundInstance* mInstance;
		float mFade;                       // volume taken off an update; 0 is no fade
	};
	std::vector<DelayedSample> mDelayed;                 // +0x14
	std::vector<ChainedSample> mChained;                 // +0x24
	std::map<int, LoopedSample> mLooping;                // +0x34

	int mUpdateCount = 0;                                // +0x40
	Sexy::SoundInstance* mCached[MAX_SAMPLES] = {};      // +0x48: the cached samples' instances, by id

	explicit SoundPlayer(Sexy::SexyAppBase* theApp) : mApp(theApp) {}
	~SoundPlayer();

	// 0x6af650, from the app's UpdateFrames: counts the update, starts the delayed samples and the chained ones
	// whose time has come and fades the looping samples out. A looping sample that fades out ends the update
	// there, so the ones after it fade one update later.
	void Update();

	// 0x6af910: plays a sample unless it started within the last theMinUpdates updates (its cached instance if
	// it has one, else one that releases itself)
	void PlaySampleMinTime(int theId, int theMinUpdates);
	// 0x6af9d0: the same at a pan, unless it started within the last 5 updates
	void PlaySamplePan(int theId, int thePan);
	// 0x6afaa0: plays a sample on its cached instance (made the first time), at a pan, unless it started within
	// the last theMinUpdates updates
	void PlayCachedSample(int theId, int thePan, int theMinUpdates);
	// whether a cached sample is playing (what the game's code asks the instance itself)
	bool IsCachedSamplePlaying(int theId) const;

	// 0x6b00a0 (the app's PlayDelayedSample, 0x44f580): plays a sample, shifted by thePitch semitones, once
	// theDelay updates have passed (that is theDelay + 1 of them)
	void PlayDelayedSample(int theId, int theDelay, int thePitch);
	// 0x6afb80: plays a sample (shifted by thePitch when not 0) and then another, theDelay updates after the first
	// is done
	void PlayChainedSample(int theId, int theNextId, int theDelay, int thePitch);
	// 0x6afc00: plays a sample on a loop, unless one loops already
	void LoopSample(int theId);
	// 0x6afc90: stops a looping sample
	void StopSample(int theId);
	// 0x6afd10: has a looping sample lose theFade of its volume an update, until it is gone
	void FadeOutSample(int theId, float theFade);
	// 0x6afd70: plays a sample shifted by thePitch semitones, unless it started within the last theMinUpdates
	// updates (a pitch of 0 plays it as PlaySampleMinTime does)
	void PlayPitchShiftedSample(int theId, int thePitch, int theMinUpdates);

private:
	bool PlayedWithin(int theId, int theMinUpdates) const;
};

}  // namespace JeffLib
