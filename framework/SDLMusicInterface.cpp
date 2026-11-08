#include "SDLMusicInterface.h"
#include "../platform/Audio.h"
#include "../platform/MusicLevels.h"
#include "../platform/Placeholders.h"
#include "../platform/VFS.h"
#include "SDLSoundManager.h"

#include <algorithm>
#include <cmath>

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

// The CRC-32 of a song's first 32 KB, as games/bookwormadv/tools/bassproxy logs the original's BASS_MusicLoad, and how
// many bytes it covers: the proxy, which gets no length, always reads 32 KB, so a shorter song's CRCs cannot agree
static unsigned LogCrc(const std::string& theFileName, size_t* theSize)
{
	std::vector<uint8_t> aData;
	*theSize = 0;
	if (!Platform::VFS::Get().ReadAll(theFileName, aData))
		return 0;
	unsigned aCrc = 0xffffffff;
	size_t aSize = std::min<size_t>(aData.size(), 0x8000);
	*theSize = aSize;
	for (size_t i = 0; i < aSize; i++)
	{
		aCrc ^= aData[i];
		for (int b = 0; b < 8; b++)
			aCrc = (aCrc >> 1) ^ (0xedb88320 & (0 - (aCrc & 1)));
	}
	return ~aCrc;
}

int SDLMusicInterface::FindSongId(const SongInfo& theSong) const
{
	for (auto& kv : mSongs)
		if (&kv.second == &theSong)
			return kv.first;
	return -1;
}

void SDLMusicInterface::ApplyVolume(SongInfo& theSong)
{
	// BASS_ChannelSetAttributes takes the volume as 0 to 100
	if (SoundLogOn())
		SoundLog("music volume %d %d", FindSongId(theSong), (int)(theSong.mVolume * 100));
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
	if (SoundLogOn())
	{
		size_t aSize;
		unsigned aCrc = LogCrc(theFileName, &aSize);
		SoundLog("music load %d %s crc=%08x len=%u", theSongId, theFileName.c_str(), aCrc, (unsigned)aSize);
	}
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
	SoundLog("music play %d offset=%d loop=%d", theSongId, theOffset, noLoop ? 0 : 1);
	int offset = std::max(theOffset, 0);
	Mixer::Get().PlaySong(s.mHandle, offset & 0xFFFF, (offset >> 16) & 0xFFFF, !noLoop, true);
}

void SDLMusicInterface::StopMusic(int theSongId)
{
	auto it = mSongs.find(theSongId);
	if (it == mSongs.end())
		return;
	SoundLog("music stop %d", theSongId);
	it->second.mVolume = 0.0;
	Mixer::Get().StopSong(it->second.mHandle);
}

void SDLMusicInterface::PauseMusic(int theSongId)
{
	auto it = mSongs.find(theSongId);
	if (it != mSongs.end())
	{
		SoundLog("music pause %d", theSongId);
		Mixer::Get().PauseSong(it->second.mHandle, true);
	}
}

void SDLMusicInterface::ResumeMusic(int theSongId)
{
	auto it = mSongs.find(theSongId);
	if (it != mSongs.end())
	{
		SoundLog("music resume %d", theSongId);
		Mixer::Get().PauseSong(it->second.mHandle, false);
	}
}

void SDLMusicInterface::StopAllMusic()
{
	for (auto& kv : mSongs)
	{
		SoundLog("music stop %d", kv.first);
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
	{
		SoundLog("music pause %d", kv.first);
		Mixer::Get().PauseSong(kv.second.mHandle, true);
	}
}

void SDLMusicInterface::ResumeAllMusic()
{
	for (auto& kv : mSongs)
	{
		SoundLog("music resume %d", kv.first);
		Mixer::Get().PauseSong(kv.second.mHandle, false);
	}
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
	SoundLog("music fadein %d offset=%d speed=%.4f loop=%d", theSongId, theOffset, theSpeed, noLoop ? 0 : 1);
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
	SoundLog("music fadeout %d stop=%d speed=%.4f", theSongId, stopSong ? 1 : 0, theSpeed);
	if (it->second.mVolume != 0.0)
		it->second.mVolumeAdd = -theSpeed;
	it->second.mStopOnFade = stopSong;
}

void SDLMusicInterface::FadeOutAll(bool stopSong, double theSpeed)
{
	for (auto& kv : mSongs)
	{
		SoundLog("music fadeout %d stop=%d speed=%.4f", kv.first, stopSong ? 1 : 0, theSpeed);
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

// BASS_ChannelGetPosition(BASS_POS_MUSIC_ORDER): the order in the low word, the row in the high word
int SDLMusicInterface::GetMusicPosition(int theSongId)
{
	auto it = mSongs.find(theSongId);
	int anOrder = 0, aRow = 0;
	if (it == mSongs.end() || !Mixer::Get().GetSongPosition(it->second.mHandle, anOrder, aRow))
		return -1;
	return (anOrder & 0xffff) | (aRow << 16);
}

bool SDLMusicInterface::GetMusicFFT(int theSongId, float* theOut, int theSamples)
{
	auto it = mSongs.find(theSongId);
	return it != mSongs.end() && Mixer::Get().GetSongFFT(it->second.mHandle, theOut, theSamples);
}

int SDLMusicInterface::GetMusicTempo(int theSongId)
{
	auto it = mSongs.find(theSongId);
	return it == mSongs.end() ? 0 : Mixer::Get().GetSongTempo(it->second.mHandle);
}

void SDLMusicInterface::SetMusicTempo(int theSongId, int theBPM)
{
	auto it = mSongs.find(theSongId);
	if (it != mSongs.end())
		Mixer::Get().SetSongTempo(it->second.mHandle, theBPM);
}

int SDLMusicInterface::GetMusicChannelVolume(int theSongId, int theChannel)
{
	auto it = mSongs.find(theSongId);
	return it == mSongs.end() ? -1 : Mixer::Get().GetSongChannelVolume(it->second.mHandle, theChannel);
}

void SDLMusicInterface::SetMusicChannelVolume(int theSongId, int theChannel, int theVolume)
{
	auto it = mSongs.find(theSongId);
	if (it != mSongs.end())
		Mixer::Get().SetSongChannelVolume(it->second.mHandle, theChannel, theVolume);
}

void SDLMusicInterface::SetVolume(double theVolume)
{
	mGlobalVolume = theVolume;
	SoundLog("music master %.3f", theVolume);
	Mixer::Get().SetMusicMasterGain((float)(theVolume * mMaxMusicVolume / 100.0));
}

// Bookworm Adventures' SexyBassMusicInterface::SetMusicAmplify (0x59aaf0) sets BASS_MUSIC_ATTRIB_AMPLIFY to
// (int)(theAmp * 100). BASS 2.3 takes 0 to 100 and holds a larger value at 100; its default is 50. The gain is BASS's
// measured level against libopenmpt's (platform/MusicLevels.h, which the site's music player shares). A song whose amplify
// is never set keeps libopenmpt's level; a stream (a mod's Ogg in place of the module) is never amplified, as
// BASS_MusicSetAttribute fails on one. The spectrum sees the amplify too: BASS's FFT of the same song is 5.35 times larger
// at 100 than at 50 (mean magnitude), the 14.5 dB step.
void SDLMusicInterface::SetMusicAmplify(int theSongId, double theAmp)
{
	auto it = mSongs.find(theSongId);
	if (it == mSongs.end())
		return;
	// clamped as a double first: a cast of NaN or of a value out of int's range is undefined
	int anAmp = theAmp > 0.0 ? (int)std::min(theAmp * 100.0, 100.0) : 0;
	Mixer::Get().SetSongAmplify(it->second.mHandle, (float)ByopopBassAmplifyGain(anAmp));
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
			{
				SoundLog("music stop %d", kv.first);
				Mixer::Get().StopSong(s.mHandle);
			}
		}
		ApplyVolume(s);
	}
}
