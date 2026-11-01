// Sexy::CustomBassMusicInterface (vtable 0x7e1c5c, 22 slots, 0x3c bytes; 0x6ed1ed-0x6eee08): the framework's BASS
// music with tracks. Both songs are long MO3 modules holding many tunes, each from its own order (a track); a track
// keeps a tempo and a volume for every channel of the module in each of two states (0 normal, 1 danger, which the
// board sets), and Update steps the channels' volumes towards them by 1 a frame. The port's base is
// SDLMusicInterface, which plays modules through libopenmpt and gives BASS's music attributes (BPM, VOL_CHAN) as
// GetMusicTempo and Get/SetMusicChannelVolume.
#pragma once

#include "SDLMusicInterface.h"

#include <map>
#include <vector>

namespace Sexy {

class CustomBassMusicInterface : public SDLMusicInterface {
public:
	struct Channel {                     // 0x14 bytes
		int mVolume = 0;                 // +0x0: set on the module, 0 to 100
		int mTarget[2] = {};             // +0x4: per state
		int mDefault = 0;                // +0xc: the module's volume when the track was made
		bool mDirty = false;             // +0x10: mVolume is to be set even if it is already the target
	};
	struct Track {                       // 0x24 bytes
		int mOffset = 0;                 // +0x0: the order it starts at
		std::vector<Channel> mChannels;  // +0x4
		int mTempo[2] = {};              // +0x14: BPM per state
		int mSongId;                     // +0x1c
		int mTrackId;                    // +0x20
		Track(int theSongId, int theTrackId) : mSongId(theSongId), mTrackId(theTrackId) {}  // 0x6eeb6a
	};

	int mState = 0;                      // +0x18: 0 normal, 1 danger
	int mTempo = 0;                      // +0x1c: the BPM last set
	int mCurTrack = -1;                  // +0x20: the track playing, -1 none
	int mLoadTempo = 0;                  // +0x28: the BPM of the song loaded last (never read)
	float mVolumeFactor = 1.0f;          // +0x2c: scales the channels' targets in Update
	std::map<int, Track*> mTracks;       // +0x30: by track id

	CustomBassMusicInterface();                        // 0x6eeda3
	~CustomBassMusicInterface() override;              // 0x6eebec

	bool LoadMusic(int theSongId, const std::string& theFileName) override;  // 0x6edeb5 (slot 1)
	void StopAllMusic() override;                      // 0x6ed1ed (slot 6)
	void Update() override;                            // 0x6edfbd (slot 19)
	// 0x6edc8a (slot 21): plays a track from its order at its state's tempo, its channels at their targets (BASS's
	// two extra play arguments, always 0 but for Blitz's replays, are left out)
	virtual void PlayTrack(int theTrackId, bool noLoop);

	Track* FindTrack(int theTrackId);                  // 0x6edc41
	// 0x6eec7a: the track of theSongId's module starting at its first order, with theTempo (the module's own when 0)
	// and every channel at the module's volume; null when the song is not loaded
	Track* AddTrack(int theSongId, int theTrackId, int theTempo);
	// 0x6edf21: theChannel's volume in both states (the track's default for -1), at once with theNow; theTrackId -1
	// is the track playing
	void SetChannelVolume(int theChannel, int theVolume, bool theNow, int theTrackId);
	// 0x6ee128: every channel of theTrackId's track back to its default at once (the original's calls set the
	// channels of the track playing, whatever theTrackId is)
	void ResetChannels(int theTrackId = -1);
	void SetTrackTempo(int theState, int theTempo);    // 0x6ee1ec: of the track playing
};

}
