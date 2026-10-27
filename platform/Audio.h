#ifndef __PLATFORM_AUDIO_H__
#define __PLATFORM_AUDIO_H__

// Software mixer on top of SDL2 audio, shared by native and Emscripten builds.
//
// Output is float stereo at the device's native rate (48 kHz on the web, where the AudioContext is
// created by us because SDL would otherwise inherit a 96/192 kHz context and underrun).
// Sound effects are decoded and resampled once at load time; tracker music (S3M/MOD/XM/IT) is
// rendered by libxmp-lite at the device rate inside the audio callback. Other module formats (MO3 via
// libopenmpt, platform/openmpt/) are rendered by a ModulePlayer a game registers.

#include <atomic>
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
	// The module position the player has rendered up to (BASS_ChannelGetPosition with BASS_POS_MUSIC_ORDER);
	// false when the player cannot tell.
	virtual bool GetPosition(int& theOrder, int& theRow) { return false; }
};

typedef bool (*ModuleDetector)(const std::vector<uint8_t>& theFile);
typedef std::unique_ptr<ModulePlayer> (*ModuleFactory)(const std::vector<uint8_t>& theFile, int theRate);

// A decoded sound, kept as 16-bit samples at the file's own rate with its own one or two channels: a voice resamples
// it as it mixes. (A copy at the device rate as stereo floats took four to sixteen times the memory.)
struct SoundData
{
	std::vector<int16_t> mSamples;  // interleaved, mChannels per frame
	int mChannels = 1;
	int mRate = 48000;
	size_t mFrames = 0;

	size_t Bytes() const { return mSamples.capacity() * sizeof(int16_t); }
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
	// A song a game renders itself, such as a synthesizer its own sequencer plays live (TipTop's MIDI music):
	// the mixer calls thePlayer's Render in the audio callback, so the game changes the player's state only
	// between Lock and Unlock. Returns the song handle, played and freed like a loaded song's.
	int AddSong(std::unique_ptr<ModulePlayer> thePlayer);
	void UnloadSong(int theSong);
	// theOrder/theRow: start position (BASS_MusicPlayEx offset = MAKELONG(order,row)).
	void PlaySong(int theSong, int theOrder, int theRow, bool theLoop, bool theResetPosition = true);
	void StopSong(int theSong);
	void PauseSong(int theSong, bool thePaused);
	void SetSongGain(int theSong, float theGain);
	bool IsSongPlaying(int theSong);
	// The order and row a playing module has been rendered up to (libxmp's frame info, or a ModulePlayer's
	// GetPosition). False for a song that is not playing, and for Ogg and WAV tracks.
	bool GetSongPosition(int theSong, int& theOrder, int& theRow);
	// The spectrum of the song's last theSamples frames (a power of two from 64 to 4096), as BASS_ChannelGetData's
	// FFT flags give it: theSamples / 2 magnitudes of the song's own output before its gain and the music volume, the
	// two channels averaged, with a Hann window, scaled so that a full-scale sine reads 1.0 in its bin. The bins are
	// at the mixer's rate. False for a song that is not playing.
	bool GetSongFFT(int theSong, float* theOut, int theSamples);

#ifdef __EMSCRIPTEN__
	// On the web the page's starting volume (web/settings.js, docs/WEB_PLAYER.md, "Site settings") reaches a game with no
	// volume option of its own (Cosmic Bugs, Water Bugs) through these two gains. The game sets them itself (its music
	// volume at each level), so the page's level is kept apart and multiplies what the game sets: SetSiteGains is the
	// page's, 1 leaves the game's gains as they are.
	void SetMusicMasterGain(float theGain) { mMusicAsked = theGain; mMusicMaster = theGain * mSiteMusic; }
	void SetSfxMasterGain(float theGain) { mSfxAsked = theGain; mSfxMaster = theGain * mSiteSfx; }
	void SetSiteGains(float theMusic, float theSfx)
	{
		mSiteMusic = theMusic;
		mSiteSfx = theSfx;
		SetMusicMasterGain(mMusicAsked);
		SetSfxMasterGain(mSfxAsked);
	}
	float GetSiteMusicGain() const { return mSiteMusic; }
	float GetSiteSfxGain() const { return mSiteSfx; }
	// the gains in effect (the game's times the page's), which the page's tests read
	float GetMusicMasterGain() const { return mMusicMaster; }
	float GetSfxMasterGain() const { return mSfxMaster; }
#else
	void SetMusicMasterGain(float theGain) { mMusicMaster = theGain; }
	void SetSfxMasterGain(float theGain) { mSfxMaster = theGain; }
#endif

#ifdef __EMSCRIPTEN__
	// The web page's own mute (its sound button, and the hold while the starting volume is applied, docs/WEB_PLAYER.md): like
	// ?silent=1, the mixer goes on mixing, so that playback timing is as it was, and the device gets silence. It is
	// below every game's volume, mute and options (so it changes and saves none of them), and it reaches every game
	// that plays through the mixer, SexyAppFramework or not.
	void SetPageMute(bool theMuted) { mPageMuted = theMuted; }
	bool IsPageMuted() const { return mPageMuted; }
#endif

	void Lock();
	void Unlock();

	// Mixes theFrames stereo frames into theOut without an audio device (tests and offline checks).
	void RenderOffline(float* theOut, int theFrames) { Mix(theOut, theFrames); }
	// For tools without an audio device: makes the voices at theRate, so that StartVoice works and RenderOffline
	// mixes them.
	void InitOffline(int theRate);

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
#ifdef __EMSCRIPTEN__
	std::atomic<bool> mPageMuted{false};  // the web page's mute (SetPageMute): likewise
	float mMusicAsked = 1.0f, mSfxAsked = 1.0f;  // the gains the game last set (SetMusicMasterGain, SetSfxMasterGain)
	float mSiteMusic = 1.0f, mSiteSfx = 1.0f;    // the page's starting volume for a game with no volume option (SetSiteGains)
#endif
	bool mOffline = false; // InitOffline: voices without a device
	std::vector<std::unique_ptr<Voice>> mVoices;
	std::vector<std::unique_ptr<Song>> mSongs;
	int mNextVoiceSerial = 1;
	std::vector<short> mSongScratch;
	std::vector<float> mModuleScratch;
};

}

#endif
