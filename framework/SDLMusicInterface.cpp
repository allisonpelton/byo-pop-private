#include "SDLMusicInterface.h"
#include "../platform/Audio.h"
#include "../platform/Placeholders.h"

#include <algorithm>

using namespace Sexy;
using Platform::Mixer;

SDLMusicInterface::SDLMusicInterface()
{
	Mixer::Get().Init();
	SetVolume(1.0);
}

SDLMusicInterface::~SDLMusicInterface()
{
	UnloadAllMusic();
}

void SDLMusicInterface::ApplyVolume(SongInfo& theSong)
{
	Mixer::Get().SetSongGain(theSong.mHandle, (float)theSong.mVolume);
}

bool SDLMusicInterface::LoadMusic(int theSongId, const std::string& theFileName)
{
	int handle = Mixer::Get().LoadSong(theFileName);
	if (!handle)
	{
		// port: -placeholders (platform/Placeholders.h): a song that did not load plays as silence anyway; listed
		if (Platform::Placeholders())
			Platform::NoteMissing(theFileName);
		return false;
	}
	UnloadMusic(theSongId);
	SongInfo info;
	info.mHandle = handle;
	mSongs[theSongId] = info;
	return true;
}

void SDLMusicInterface::PlayMusic(int theSongId, int theOffset, bool noLoop)
{
	auto it = mSongs.find(theSongId);
	if (it == mSongs.end())
		return;
	SongInfo& s = it->second;
	s.mVolume = s.mVolumeCap;
	s.mVolumeAdd = 0.0;
	s.mStopOnFade = noLoop;
	ApplyVolume(s);
	int offset = std::max(theOffset, 0);
	Mixer::Get().PlaySong(s.mHandle, offset & 0xFFFF, (offset >> 16) & 0xFFFF, !noLoop, true);
}

void SDLMusicInterface::StopMusic(int theSongId)
{
	auto it = mSongs.find(theSongId);
	if (it == mSongs.end())
		return;
	it->second.mVolume = 0.0;
	Mixer::Get().StopSong(it->second.mHandle);
}

void SDLMusicInterface::PauseMusic(int theSongId)
{
	auto it = mSongs.find(theSongId);
	if (it != mSongs.end())
		Mixer::Get().PauseSong(it->second.mHandle, true);
}

void SDLMusicInterface::ResumeMusic(int theSongId)
{
	auto it = mSongs.find(theSongId);
	if (it != mSongs.end())
		Mixer::Get().PauseSong(it->second.mHandle, false);
}

void SDLMusicInterface::StopAllMusic()
{
	for (auto& kv : mSongs)
	{
		kv.second.mVolume = 0.0;
		Mixer::Get().StopSong(kv.second.mHandle);
	}
}

void SDLMusicInterface::UnloadMusic(int theSongId)
{
	auto it = mSongs.find(theSongId);
	if (it == mSongs.end())
		return;
	Mixer::Get().StopSong(it->second.mHandle);
	Mixer::Get().UnloadSong(it->second.mHandle);
	mSongs.erase(it);
}

void SDLMusicInterface::UnloadAllMusic()
{
	while (!mSongs.empty())
		UnloadMusic(mSongs.begin()->first);
}

void SDLMusicInterface::PauseAllMusic()
{
	for (auto& kv : mSongs)
		Mixer::Get().PauseSong(kv.second.mHandle, true);
}

void SDLMusicInterface::ResumeAllMusic()
{
	for (auto& kv : mSongs)
		Mixer::Get().PauseSong(kv.second.mHandle, false);
}

void SDLMusicInterface::FadeIn(int theSongId, int theOffset, double theSpeed, bool noLoop)
{
	auto it = mSongs.find(theSongId);
	if (it == mSongs.end())
		return;
	SongInfo& s = it->second;
	s.mVolumeAdd = theSpeed;
	s.mStopOnFade = noLoop;
	ApplyVolume(s);
	if (theOffset == -1)
		Mixer::Get().PlaySong(s.mHandle, 0, 0, !noLoop, false);  // BASS_MusicPlay: resume position
	else
		Mixer::Get().PlaySong(s.mHandle, theOffset & 0xFFFF, (theOffset >> 16) & 0xFFFF, !noLoop, true);
}

void SDLMusicInterface::FadeOut(int theSongId, bool stopSong, double theSpeed)
{
	auto it = mSongs.find(theSongId);
	if (it == mSongs.end())
		return;
	if (it->second.mVolume != 0.0)
		it->second.mVolumeAdd = -theSpeed;
	it->second.mStopOnFade = stopSong;
}

void SDLMusicInterface::FadeOutAll(bool stopSong, double theSpeed)
{
	for (auto& kv : mSongs)
	{
		kv.second.mVolumeAdd = -theSpeed;
		kv.second.mStopOnFade = stopSong;
	}
}

void SDLMusicInterface::SetSongVolume(int theSongId, double theVolume)
{
	auto it = mSongs.find(theSongId);
	if (it == mSongs.end())
		return;
	it->second.mVolume = theVolume;
	ApplyVolume(it->second);
}

void SDLMusicInterface::SetSongMaxVolume(int theSongId, double theMaxVolume)
{
	auto it = mSongs.find(theSongId);
	if (it == mSongs.end())
		return;
	it->second.mVolumeCap = theMaxVolume;
	it->second.mVolume = std::min(it->second.mVolume, theMaxVolume);
	ApplyVolume(it->second);
}

bool SDLMusicInterface::IsPlaying(int theSongId)
{
	auto it = mSongs.find(theSongId);
	return it != mSongs.end() && Mixer::Get().IsSongPlaying(it->second.mHandle);
}

void SDLMusicInterface::SetVolume(double theVolume)
{
	mGlobalVolume = theVolume;
	Mixer::Get().SetMusicMasterGain((float)(theVolume * mMaxMusicVolume / 100.0));
}

void SDLMusicInterface::SetMusicAmplify(int, double)
{
}

void SDLMusicInterface::Update()
{
	for (auto& kv : mSongs)
	{
		SongInfo& s = kv.second;
		if (s.mVolumeAdd == 0.0)
			continue;
		s.mVolume += s.mVolumeAdd;
		if (s.mVolume > s.mVolumeCap)
		{
			s.mVolume = s.mVolumeCap;
			s.mVolumeAdd = 0.0;
		}
		else if (s.mVolume < 0.0)
		{
			s.mVolume = 0.0;
			s.mVolumeAdd = 0.0;
			if (s.mStopOnFade)
				Mixer::Get().StopSong(s.mHandle);
		}
		ApplyVolume(s);
	}
}
