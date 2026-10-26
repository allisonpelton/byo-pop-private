// port: TipTop's music through TinySoundFont. See MusicSynth.h.

#define TSF_IMPLEMENTATION
#include <tsf.h>

#include "MusicSynth.h"
#include "Game.h"
#include "Music.h"
#include "platform/Audio.h"

#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

namespace tiptop
{

namespace
{

// Allocated up front, so the audio thread never allocates; the song sounds up to 116 at once (musicwav counts them),
// most of them notes fading out
const int MAX_VOICES = 256;
// The synth's own level: at the default music volume (192) the song's mean is about -24 dB and its peak -5 dB, below
// the sounds (peaks at 0 dB, means of -12 to -18 dB), and at full volume its peak stays under 0 dB
const float GAIN_DB = 0.0f;

tsf* gSynth = nullptr;
int gRate = 0;
int gSong = 0;

// The channels as a General MIDI synth starts them: program 0, and channel 10 on the drum kits
void ResetChannels()
{
	for (int c = 15; c >= 0; c--)
		tsf_channel_set_presetnumber(gSynth, c, 0, c == 9);
}

// The sequencer's MIDI output (Music.h)
void SynthMessage(uint32_t theMessage)
{
	int aStatus = theMessage & 0xff;
	int aChannel = aStatus & 0xf;
	int aData1 = (theMessage >> 8) & 0x7f;
	int aData2 = (theMessage >> 16) & 0x7f;
	switch (aStatus & 0xf0)
	{
	case 0x80:
		tsf_channel_note_off(gSynth, aChannel, aData1);
		break;
	case 0x90:
		tsf_channel_note_on(gSynth, aChannel, aData1, aData2 / 127.0f);  // velocity 0 is a note off
		break;
	case 0xb0:
		tsf_channel_midi_control(gSynth, aChannel, aData1, aData2);
		break;
	case 0xc0:
		tsf_channel_set_presetnumber(gSynth, aChannel, aData1, aChannel == 9);
		break;
	case 0xe0:
		tsf_channel_set_pitchwheel(gSynth, aChannel, aData2 << 7 | aData1);
		break;
	case 0xf0:
		if (aStatus == (int)MIDI_CLOSE)
		{
			// the notes end quickly, and the synth starts again as the mapper did when the game reopened it
			tsf_reset(gSynth);
			ResetChannels();
		}
		break;
	}
}

// The synth as a song of the mixer: the sequencer runs a pass for each millisecond of sound, and the synth renders
// the millisecond after it
class SynthSong : public Platform::ModulePlayer
{
public:
	void SetPosition(int, int) override {}

	bool Render(float* theOut, int theFrames, bool) override
	{
		while (theFrames > 0)
		{
			if (mFramesLeft == 0)
			{
				mTime = mTime + 1;
				UpdateMusic(mTime);
				mFramesLeft = (int)((uint64_t)(mTime + 1) * gRate / 1000 - (uint64_t)mTime * gRate / 1000);
			}
			int aFrames = std::min(theFrames, mFramesLeft);
			tsf_render_float(gSynth, theOut, aFrames, 0);
			theOut = theOut + aFrames * 2;
			theFrames = theFrames - aFrames;
			mFramesLeft = mFramesLeft - aFrames;
		}
		return true;
	}

	// Renders to the end of the millisecond in progress, then theMs more, into nothing
	void Skip(int theMs)
	{
		std::vector<float> aScratch((size_t)(gRate / 1000 + 1) * 2);
		while (mFramesLeft > 0)
			Render(aScratch.data(), std::min(mFramesLeft, gRate / 1000), true);
		for (int i = 0; i < theMs; i++)
		{
			Render(aScratch.data(), 1, true);
			if (mFramesLeft > 0)
				Render(aScratch.data(), mFramesLeft, true);
		}
	}

	uint32_t mTime = 0;
	int mFramesLeft = 0;
};

SynthSong* gPlayer = nullptr;  // owned by the mixer's song

} // namespace

bool StartMusicSynth(const std::string& theBankPath)
{
	// Without an audio device (the web build under Node) nothing renders the synth but the test hook musicwav
	Platform::Mixer& aMixer = Platform::Mixer::Get();
	SDL_RWops* aFile = SDL_RWFromFile(theBankPath.c_str(), "rb");
	if (aFile == nullptr)
	{
		SDL_Log("music: no SoundFont bank %s", theBankPath.c_str());
		return false;
	}
	std::vector<uint8_t> aBank((size_t)std::max<Sint64>(SDL_RWsize(aFile), 0));
	size_t aRead = aBank.empty() ? 0 : SDL_RWread(aFile, aBank.data(), 1, aBank.size());
	SDL_RWclose(aFile);
	if (aRead != aBank.size() || aBank.empty() || (gSynth = tsf_load_memory(aBank.data(), (int)aBank.size())) == nullptr)
	{
		SDL_Log("music: cannot load the SoundFont bank %s", theBankPath.c_str());
		return false;
	}
	gRate = aMixer.GetRate();
	tsf_set_output(gSynth, TSF_STEREO_INTERLEAVED, gRate, GAIN_DB);
	tsf_set_max_voices(gSynth, MAX_VOICES);
	ResetChannels();
	SetMidiOut(SynthMessage);
	gPlayer = new SynthSong();
	gSong = aMixer.AddSong(std::unique_ptr<Platform::ModulePlayer>(gPlayer));
	aMixer.PlaySong(gSong, 0, 0, true);
	SDL_Log("music: %s (%d presets)", theBankPath.c_str(), tsf_get_presetcount(gSynth));
	return true;
}

void StopMusicSynth()
{
	if (gSong != 0)
		Platform::Mixer::Get().UnloadSong(gSong);
	gSong = 0;
	gPlayer = nullptr;
	SetMidiOut(nullptr);
	if (gSynth != nullptr)
		tsf_close(gSynth);
	gSynth = nullptr;
}

bool RenderMusicWav(const std::string& theFile, int theSeconds)
{
	if (gPlayer == nullptr)
	{
		SDL_Log("musicwav: the music synth is not running");
		return false;
	}
	std::vector<float> aSound((size_t)gRate * (size_t)theSeconds * 2);
	Platform::Mixer& aMixer = Platform::Mixer::Get();
	aMixer.Lock();
	// What the audio device has played so far
	SDL_Log("musicwav: %u ms of music played, %d voices sounding", gPlayer->mTime, tsf_active_voice_count(gSynth));
	// From the start of the song and of a millisecond, after the notes of the music stopped have ended, so that two
	// runs render the same
	StopMusic();
	gPlayer->Skip(100);
	StartMusic();
	int aMostVoices = 0;
	for (int aFrame = 0; aFrame < gRate * theSeconds; aFrame = aFrame + gRate / 1000)
	{
		gPlayer->Render(aSound.data() + (size_t)aFrame * 2, std::min(gRate / 1000, gRate * theSeconds - aFrame), true);
		aMostVoices = std::max(aMostVoices, tsf_active_voice_count(gSynth));
	}
	aMixer.Unlock();

	float aPeak = 0;
	std::vector<int16_t> aPcm(aSound.size());
	for (size_t i = 0; i < aSound.size(); i++)
	{
		aPeak = std::max(aPeak, std::fabs(aSound[i]));
		aPcm[i] = (int16_t)std::lround(std::min(std::max(aSound[i], -1.0f), 1.0f) * 32767.0f);
	}
	FILE* aWav = fopen(theFile.c_str(), "wb");
	if (aWav == nullptr)
	{
		SDL_Log("musicwav: cannot write %s", theFile.c_str());
		return false;
	}
	uint32_t aBytes = (uint32_t)(aPcm.size() * 2);
	uint32_t aRate = (uint32_t)gRate;
	uint32_t aByteRate = aRate * 4;
	uint8_t aHeader[44] = {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', 16, 0, 0, 0, 1, 0, 2, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 16, 0, 'd', 'a', 't', 'a', 0, 0, 0, 0};
	uint32_t aRiffSize = aBytes + 36;
	memcpy(aHeader + 4, &aRiffSize, 4);
	memcpy(aHeader + 24, &aRate, 4);
	memcpy(aHeader + 28, &aByteRate, 4);
	memcpy(aHeader + 40, &aBytes, 4);
	fwrite(aHeader, 1, sizeof(aHeader), aWav);
	fwrite(aPcm.data(), 2, aPcm.size(), aWav);
	fclose(aWav);
	SDL_Log("musicwav: %s (%d s at %d Hz, peak %.3f, at most %d of %d voices)", theFile.c_str(), theSeconds, gRate, aPeak,
		aMostVoices, MAX_VOICES);
	return true;
}

} // namespace tiptop
