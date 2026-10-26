// What JeffLib::SexyBassMusicInterface (vtable 0x7c59c4; the app's music interface in the original) adds to the
// framework's music interface: the game's music is one tracker module in which each song starts at an order of
// its own, so it is loaded twice (songs 1 and 2) and a new song fades in on one copy while the other fades out.
// port: the original was the app's music interface itself; this works on the app's (vtable slots 20 to 22, and
// the part of its Update, slot 19, that served slot 22's queue).
#pragma once

#include <string>

namespace Sexy {
class MusicInterface;
}

namespace JeffLib {

class SongPlayer {
public:
	Sexy::MusicInterface* mMusic;
	int mSpectrumWidth = 0;              // +0x18: of the media player's spectrum (GameApp::Init: 320)
	int mSpectrumHeight = 0;             // +0x1c: (240)
	unsigned char* mSpectrum = nullptr;  // +0x3c: a level for each cell, row 0 at the bottom; 0 is unlit
	int mCurrentSong = -1;    // +0x20: 1 or 2, the copy playing (-1 before the first song)
	int mCurrentOrder = -1;   // +0x24: the order the song playing started at
	int mLastOrder = -1;      // +0x28: the one before it
	int mQueuedOrder = -1;    // +0x2c: the song to play when this one is done (slot 22)
	int mQueuedAtOrder = -1;  // +0x30: done when the song playing reaches this order (-1: when it stops)
	int mQueuedAtRow = -1;    // +0x34: and this row of it (-1: any)
	float mQueuedSpeed = 0;   // +0x38: the queued song's fade

	explicit SongPlayer(Sexy::MusicInterface* theMusic) : mMusic(theMusic) {}
	~SongPlayer();  // 0x6c4c30: frees the spectrum

	// slot 20, 0x6c4c50: loads the module as songs 1 and 2, with an amplification (when not 0). port: the
	// original's BASS flags are left out.
	bool LoadSong(const std::string& theFileName, double theAmplify);
	// slot 21, 0x6c4cd0: plays the song that starts at an order (a BASS position), fading the one playing out
	// and this one in at theSpeed (at once when it is no more than 0.00001). Nothing when it is playing already.
	void PlaySong(int theOrder, float theSpeed, bool theLoop);
	// slot 22, 0x6c4e50: plays a song (looped) when the one playing stops or reaches an order (and row), at once
	// when none is playing
	void PlaySongWhenDone(int theOrder, float theSpeed, int theAtOrder, int theAtRow);
	// 0x6c4eb0 (slot 19, after the music interface's own Update): plays the queued song once the one playing is
	// done
	void Update();
	// 0x6c50a0: every fourth update, the spectrum of the song playing drawn into mSpectrum, each cell lit holding
	// its row plus one. Mode 0 has a column for each of the first bins (half the width), with a column halfway
	// between neighbours; mode 1 has 28 bars on a logarithmic scale. Other modes leave it unlit.
	void UpdateSpectrum(int theUpdateCnt, int theMode);
};

}  // namespace JeffLib
