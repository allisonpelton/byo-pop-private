#ifndef __SDLMUSICINTERFACE_H__
#define __SDLMUSICINTERFACE_H__

// MusicInterface on libxmp-lite (replaces BassMusicInterface). Reproduces BASS semantics as used by
// the original framework: per-song volume with a cap, FadeIn/FadeOut stepping the volume once per
// Update() (100 Hz), offsets as BASS_MusicPlayEx positions (MAKELONG(order,row)), and the global
// music volume scaled by mMaxMusicVolume = 40 (BASS_SetGlobalVolumes range 0-100).

#include "MusicInterface.h"
#include <map>

namespace Sexy
{

class SDLMusicInterface : public MusicInterface
{
public:
	struct SongInfo
	{
		int mHandle = 0;
		double mVolume = 0.0;
		double mVolumeCap = 1.0;
		double mVolumeAdd = 0.0;
		bool mStopOnFade = false;
	};

	std::map<int, SongInfo> mSongs;
	int mMaxMusicVolume = 40;
	double mGlobalVolume = 1.0;

protected:
	void ApplyVolume(SongInfo& theSong);
	int FindSongId(const SongInfo& theSong) const;  // port: for the sound log

public:
	SDLMusicInterface();
	virtual ~SDLMusicInterface();

	virtual bool LoadMusic(int theSongId, const std::string& theFileName);
	virtual void PlayMusic(int theSongId, int theOffset = 0, bool noLoop = false);
	virtual void StopMusic(int theSongId);
	virtual void PauseMusic(int theSongId);
	virtual void ResumeMusic(int theSongId);
	virtual void StopAllMusic();
	virtual void UnloadMusic(int theSongId);
	virtual void UnloadAllMusic();
	virtual void PauseAllMusic();
	virtual void ResumeAllMusic();
	virtual void FadeIn(int theSongId, int theOffset = -1, double theSpeed = 0.002, bool noLoop = false);
	virtual void FadeOut(int theSongId, bool stopSong = true, double theSpeed = 0.004);
	virtual void FadeOutAll(bool stopSong = true, double theSpeed = 0.004);
	virtual void SetSongVolume(int theSongId, double theVolume);
	virtual void SetSongMaxVolume(int theSongId, double theMaxVolume);
	virtual bool IsPlaying(int theSongId);
	virtual int GetMusicPosition(int theSongId);
	virtual bool GetMusicFFT(int theSongId, float* theOut, int theSamples);
	virtual void SetVolume(double theVolume);
	virtual void SetMusicAmplify(int theSongId, double theAmp);
	virtual void Update();
};

}

#endif
