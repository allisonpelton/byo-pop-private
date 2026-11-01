#ifndef __MUSICINTERFACE_H__
#define __MUSICINTERFACE_H__

#include "Common.h"

namespace Sexy
{

class MusicInterface
{
public:		

public:
	MusicInterface();
	virtual ~MusicInterface();

	virtual bool			LoadMusic(int theSongId, const std::string& theFileName);
	virtual void			PlayMusic(int theSongId, int theOffset = 0, bool noLoop = false);		
	virtual void			StopMusic(int theSongId);
	virtual void			PauseMusic(int theSongId);
	virtual void			ResumeMusic(int theSongId);
	virtual void			StopAllMusic();		

	virtual void			UnloadMusic(int theSongId);
	virtual void			UnloadAllMusic();
	virtual void			PauseAllMusic();
	virtual void			ResumeAllMusic();
	
	virtual void			FadeIn(int theSongId, int theOffset = -1, double theSpeed = 0.002, bool noLoop = false);
	virtual void			FadeOut(int theSongId, bool stopSong = true, double theSpeed = 0.004);
	virtual void			FadeOutAll(bool stopSong = true, double theSpeed = 0.004);
	virtual void			SetSongVolume(int theSongId, double theVolume);
	virtual void			SetSongMaxVolume(int theSongId, double theMaxVolume);
	virtual bool			IsPlaying(int theSongId);
	// port: where a playing module is, as BASS_ChannelGetPosition(BASS_POS_MUSIC_ORDER) gives it: MAKELONG(order,
	// row), or -1 when the song is not playing or is not a module (Bookworm Adventures' song queue reads it).
	virtual int				GetMusicPosition(int theSongId);
	// port: the song's spectrum, as BASS_ChannelGetData(channel, theOut, BASS_DATA_FFT...) gives it: theSamples / 2
	// magnitudes from its last theSamples output samples (Platform::Mixer::GetSongFFT). In BASS 2.3 (Bookworm
	// Adventures ships 2.3.0.1) 0x80000001 is a 1024-sample FFT and 0x80000002 a 2048-sample one, so read the flag
	// against the game's own bass.dll; false when the song is not playing.
	virtual bool			GetMusicFFT(int theSongId, float* theOut, int theSamples);
	// port: a module's tempo and channel volumes, as BASS's music attributes BASS_MUSIC_ATTRIB_BPM (1 to 255) and
	// BASS_MUSIC_ATTRIB_VOL_CHAN + theChannel (0 to 100 in BASS 2.2, 100 until set) give and set them (Bejeweled
	// Twist's music changes both as the game goes). GetMusicTempo is 0 for a song that is not a module, and
	// GetMusicChannelVolume -1 past its channels; the values set are kept when the song is played again from another
	// position.
	virtual int				GetMusicTempo(int theSongId);
	virtual void			SetMusicTempo(int theSongId, int theBPM);
	virtual int				GetMusicChannelVolume(int theSongId, int theChannel);
	virtual void			SetMusicChannelVolume(int theSongId, int theChannel, int theVolume);

	virtual void			SetVolume(double theVolume);
	virtual void			SetMusicAmplify(int theSongId, double theAmp);
	virtual void			Update();
};

}

#endif //__MUSICINTERFACE_H__
