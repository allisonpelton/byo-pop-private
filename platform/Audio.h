#ifndef __PLATFORM_AUDIO_H__
#define __PLATFORM_AUDIO_H__

// Software mixer on top of SDL2 audio, shared by native and Emscripten builds.
//
// Output is float stereo at the device's native rate (48 kHz on the web, where the AudioContext is
// created by us because SDL would otherwise inherit a 96/192 kHz context and underrun).
// Sound effects are decoded and resampled once at load time; tracker music (S3M/MOD/XM/IT) is
// rendered by libxmp-lite at the device rate inside the audio callback. Other module formats (MO3 via
// libopenmpt, platform/openmpt/) are rendered by a ModulePlayer a game registers.

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Platform
{

// A tracker-module renderer besides the built-in libxmp-lite. A game that needs another format (e.g. MO3,
// platform/openmpt/OpenMPTPlayer.h) registers a detector and a factory with Mixer::RegisterModuleFormat
// before it loads songs; games that register none never link that library.
class ModulePlayer
{
public:
	virtual ~ModulePlayer() {}
	// Restarts playback at a module position (BASS_MusicPlayEx offset = MAKELONG(order, row)).
	virtual void SetPosition(int theOrder, int theRow) = 0;
	// Writes theFrames interleaved stereo frames (float, +-1.0) at the mixer rate to theOut. Returns false
	// once a module that does not loop has ended (theOut is then silence from there on).
	virtual bool Render(float* theOut, int theFrames, bool theLoop) = 0;
};

typedef bool (*ModuleDetector)(const std::vector<uint8_t>& theFile);
typedef std::unique_ptr<ModulePlayer> (*ModuleFactory)(const std::vector<uint8_t>& theFile, int theRate);

struct SoundData
{
	std::vector<float> mSamples;  // interleaved stereo at Mixer::GetRate()
	size_t mFrames = 0;
};

class Mixer
{
public:
	static Mixer& Get();
	// Closes the device if it is still open: the instance is a function-local static, destroyed at exit,
	// and SDL's audio thread must not mix into it after that (~SexyAppBase normally closes it first).
	~Mixer();

	bool Init();
	void Shutdown();
	bool IsInitialized() const { return mDevice != 0; }
	int GetRate() const { return mRate; }

	// Decodes Ogg Vorbis (stb_vorbis) or WAV (SDL) from the VFS and resamples to the device rate.
	std::shared_ptr<SoundData> LoadSound(const std::string& theGamePath);
	std::shared_ptr<SoundData> LoadSoundData(const std::vector<uint8_t>& theFile, const std::string& theName);

	// --- voices (sound effects) ---
	// Returns a voice handle (>0) or 0 if no voice is free.
	int StartVoice(const std::shared_ptr<SoundData>& theData, float theGainL, float theGainR, double thePitch, bool theLoop);
	void SetVoiceGain(int theVoice, float theGainL, float theGainR);
	void SetVoicePitch(int theVoice, double thePitch);
	void StopVoice(int theVoice);
	bool IsVoicePlaying(int theVoice);
	void StopAllVoices();

	// --- songs (tracker modules via libxmp or a registered ModulePlayer, or Ogg/WAV tracks from mods) ---
	// Files theDetector accepts are loaded with theFactory instead of libxmp-lite (first match wins).
	static void RegisterModuleFormat(ModuleDetector theDetector, ModuleFactory theFactory);
	// Returns a song handle (>0) or 0 on failure. A mod remap of theGamePath (VFS::GetRemap) can set a
	// start order inside a multi-song module, a stream loop point and a per-track gain.
	int LoadSong(const std::string& theGamePath);
	void UnloadSong(int theSong);
	// theOrder/theRow: start position (BASS_MusicPlayEx offset = MAKELONG(order,row)).
	void PlaySong(int theSong, int theOrder, int theRow, bool theLoop, bool theResetPosition = true);
	void StopSong(int theSong);
	void PauseSong(int theSong, bool thePaused);
	void SetSongGain(int theSong, float theGain);
	bool IsSongPlaying(int theSong);

	void SetMusicMasterGain(float theGain) { mMusicMaster = theGain; }
	void SetSfxMasterGain(float theGain) { mSfxMaster = theGain; }

	void Lock();
	void Unlock();

	// Mixes theFrames stereo frames into theOut without an audio device (tests and offline checks).
	void RenderOffline(float* theOut, int theFrames) { Mix(theOut, theFrames); }

private:
	struct Voice;
	struct Song;
	static void Callback(void* theUser, uint8_t* theStream, int theLen);
	void Mix(float* theOut, int theFrames);

	uint32_t mDevice = 0;
	int mRate = 48000;
	float mMusicMaster = 1.0f;
	float mSfxMaster = 1.0f;
	bool mSilent = false;  // web test runs (web/shell.html ?silent=1): mix as usual, output silence
	std::vector<std::unique_ptr<Voice>> mVoices;
	std::vector<std::unique_ptr<Song>> mSongs;
	int mNextVoiceSerial = 1;
	std::vector<short> mSongScratch;
	std::vector<float> mModuleScratch;
};

}

#endif
