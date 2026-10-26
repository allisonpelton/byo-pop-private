// JeffLib::SoundPlayer (vtable 0x7c5380, code 0x6af4b0-0x6b0400; the app's, Bookworm Adventures' +0x7b4): plays
// the game's samples for it, keeping one sound instance a sample for the samples it plays again and again (the
// cached samples), and a sample from playing twice within a few updates.
#pragma once

#include <map>

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
	int mUpdateCount = 0;                                // +0x40
	Sexy::SoundInstance* mCached[MAX_SAMPLES] = {};      // +0x48: the cached samples' instances, by id

	explicit SoundPlayer(Sexy::SexyAppBase* theApp) : mApp(theApp) {}
	~SoundPlayer();

	// 0x6af650, from the app's UpdateFrames. port: the original also counted down its delayed and chained
	// samples and the fades of its songs here; nothing that is ported starts any yet.
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

private:
	bool PlayedWithin(int theId, int theMinUpdates) const;
};

}  // namespace JeffLib
