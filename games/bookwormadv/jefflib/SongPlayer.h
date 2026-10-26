// What JeffLib::SexyBassMusicInterface (vtable 0x7c59c4; the app's music interface in the original) adds to the
// framework's music interface: the game's music is one tracker module in which each song starts at an order of
// its own, so it is loaded twice (songs 1 and 2) and a new song fades in on one copy while the other fades out.
// port: the original was the app's music interface itself; this works on the app's (vtable slots 20 and 21, and
// the queue of slot 22 that its Update served, which is not ported yet).
#pragma once

#include <string>

namespace Sexy {
class MusicInterface;
}

namespace JeffLib {

class SongPlayer {
public:
	Sexy::MusicInterface* mMusic;
	int mCurrentSong = -1;    // +0x20: 1 or 2, the copy playing (-1 before the first song)
	int mCurrentOrder = -1;   // +0x24: the order the song playing started at
	int mLastOrder = -1;      // +0x28: the one before it
	int mQueuedOrder = -1;    // +0x2c: the song to play when this one is done (slot 22, not ported yet)

	explicit SongPlayer(Sexy::MusicInterface* theMusic) : mMusic(theMusic) {}

	// slot 20, 0x6c4c50: loads the module as songs 1 and 2, with an amplification (when not 0). port: the
	// original's BASS flags are left out.
	bool LoadSong(const std::string& theFileName, double theAmplify);
	// slot 21, 0x6c4cd0: plays the song that starts at an order (a BASS position), fading the one playing out
	// and this one in at theSpeed (at once when it is no more than 0.00001). Nothing when it is playing already.
	void PlaySong(int theOrder, float theSpeed, bool theLoop);
};

}  // namespace JeffLib
