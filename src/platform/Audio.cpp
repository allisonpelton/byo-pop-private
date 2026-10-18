#include "Audio.h"
#include "VFS.h"

#include <SDL.h>
#include <xmp.h>
#include <algorithm>
#include <cmath>
#include <cstring>

#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace Platform
{

static const int kMaxVoices = 48;
static const int kVoiceSerialShift = 8;  // handle = serial << 8 | slot

struct Mixer::Voice
{
	std::shared_ptr<SoundData> mData;
	double mPos = 0.0;
	double mStep = 1.0;
	float mGainL = 1.0f, mGainR = 1.0f;
	bool mLoop = false;
	bool mActive = false;
	int mHandle = 0;
};

// A song is either a tracker module (libxmp) or an audio track supplied by a mod. Ogg tracks are
// decoded while they play (a rendered 30-minute suite would need ~700 MB as float PCM); WAV tracks
// are decoded up front. Both are resampled to the device rate with linear interpolation.
struct Mixer::Song
{
	xmp_context mCtx = nullptr;

	std::vector<uint8_t> mFile;              // Ogg bytes, kept alive for the decoder
	stb_vorbis* mVorbis = nullptr;
	std::shared_ptr<SoundData> mPcm;         // decoded WAV track (already at the device rate)
	int mSourceRate = 0;
	int mSourceChannels = 0;
	int64_t mSourceFrames = 0;               // total length in source frames (0 = unknown)
	int64_t mPcmPos = 0;
	std::vector<float> mDecoded;             // Ogg decode buffer (interleaved, source channels)
	size_t mDecodedPos = 0;                  // next unread frame in mDecoded
	size_t mDecodedFrames = 0;
	float mFrameA[2] = {0, 0};               // resampler: previous and next source frame
	float mFrameB[2] = {0, 0};
	double mFrac = 0;
	bool mEnded = false;

	int64_t mStartFrame = 0;                 // source frame playback starts at (mod remap "start")
	int64_t mLoopStartFrame = 0;             // source frame playback loops back to ("loopStart")
	int mDefaultOrder = -1;                  // module order a mod remap starts this track at
	float mTrackGain = 1.0f;                 // per-track gain from a mod remap

	bool mLoaded = false;
	bool mPlaying = false;
	bool mPaused = false;
	bool mLoop = true;
	float mGain = 1.0f;
	float mCurGain = 1.0f;  // smoothed per buffer to avoid zipper noise on fades

	bool IsStream() const { return mVorbis || mPcm; }

	void SeekSource(int64_t theFrame)
	{
		theFrame = std::max<int64_t>(0, mSourceFrames > 0 ? std::min(theFrame, mSourceFrames - 1) : theFrame);
		if (mVorbis)
			stb_vorbis_seek(mVorbis, (unsigned)theFrame);
		mPcmPos = theFrame;
		mDecodedPos = mDecodedFrames = 0;
		mEnded = false;
	}

	// Reads the next source frame as stereo; at the end of the data it loops or reports the end.
	bool ReadSourceFrame(float* theOut)
	{
		for (int attempt = 0; attempt < 2; attempt++)
		{
			if (mPcm)
			{
				if (mPcmPos < (int64_t)mPcm->mFrames)
				{
					theOut[0] = mPcm->mSamples[mPcmPos * 2];
					theOut[1] = mPcm->mSamples[mPcmPos * 2 + 1];
					mPcmPos++;
					return true;
				}
			}
			else
			{
				if (mDecodedPos >= mDecodedFrames)
				{
					mDecoded.resize(1024 * mSourceChannels);
					int got = stb_vorbis_get_samples_float_interleaved(mVorbis, mSourceChannels, mDecoded.data(), (int)mDecoded.size());
					mDecodedFrames = got > 0 ? (size_t)got : 0;
					mDecodedPos = 0;
				}
				if (mDecodedPos < mDecodedFrames)
				{
					const float* f = &mDecoded[mDecodedPos * mSourceChannels];
					theOut[0] = f[0];
					theOut[1] = mSourceChannels > 1 ? f[1] : f[0];
					mDecodedPos++;
					return true;
				}
			}
			if (!mLoop)
				break;
			SeekSource(mLoopStartFrame);
		}
		mEnded = true;
		return false;
	}

	void Restart(int64_t theFrame)
	{
		SeekSource(theFrame);
		mFrac = 0;
		if (!ReadSourceFrame(mFrameA))
			mFrameA[0] = mFrameA[1] = 0;
		if (!ReadSourceFrame(mFrameB))
			mFrameB[0] = mFrameB[1] = 0;
	}
};

Mixer& Mixer::Get()
{
	static Mixer sMixer;
	return sMixer;
}

Mixer::~Mixer()
{
	Shutdown();
}

bool Mixer::Init()
{
	if (mDevice)
		return true;
	if (!SDL_WasInit(SDL_INIT_AUDIO) && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
	{
		SDL_Log("audio init failed: %s", SDL_GetError());
		return false;
	}
#ifdef __EMSCRIPTEN__
	// SDL adopts the AudioContext's native rate (often 96/192 kHz) and sizes its ScriptProcessor in
	// frames at that rate, which underruns and crackles. SDL reuses an existing context, so create one
	// at 48 kHz first.
	EM_ASM({
		Module['SDL2'] = Module['SDL2'] || {};
		if (!Module['SDL2'].audioContext) {
			var Ctx = window.AudioContext || window.webkitAudioContext;
			if (Ctx) {
				try { Module['SDL2'].audioContext = new Ctx({ sampleRate: 48000 }); }
				catch (e) { Module['SDL2'].audioContext = new Ctx(); }
				var ctx = Module['SDL2'].audioContext;
				var resume = function() { if (ctx.state !== 'running') ctx.resume(); };
				// The array is parenthesized: a comma outside parentheses would end EM_ASM's code argument.
				(['pointerdown', 'keydown', 'touchstart']).forEach(function(t) { document.addEventListener(t, resume, true); });
			}
		}
	});
	const int bufferFrames = 2048;
#else
	const int bufferFrames = 1024;
#endif
	SDL_AudioSpec want = {}, have = {};
	want.freq = 48000;
	want.format = AUDIO_F32SYS;
	want.channels = 2;
	want.samples = bufferFrames;
	want.callback = &Mixer::Callback;
	want.userdata = this;
	mDevice = SDL_OpenAudioDevice(nullptr, 0, &want, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
	if (!mDevice)
	{
		SDL_Log("SDL_OpenAudioDevice failed: %s", SDL_GetError());
		return false;
	}
	mRate = have.freq;
	mVoices.clear();
	for (int i = 0; i < kMaxVoices; i++)
		mVoices.emplace_back(new Voice());
	SDL_PauseAudioDevice(mDevice, 0);
	SDL_Log("audio: %d Hz, %d frames/buffer", have.freq, have.samples);
	return true;
}

void Mixer::Shutdown()
{
	if (!mDevice)
		return;
	SDL_CloseAudioDevice(mDevice);
	mDevice = 0;
	for (auto& s : mSongs)
	{
		if (s && s->mCtx)
		{
			xmp_end_player(s->mCtx);
			xmp_release_module(s->mCtx);
			xmp_free_context(s->mCtx);
		}
	}
	mSongs.clear();
	mVoices.clear();
}

void Mixer::Lock()
{
	if (mDevice)
		SDL_LockAudioDevice(mDevice);
}

void Mixer::Unlock()
{
	if (mDevice)
		SDL_UnlockAudioDevice(mDevice);
}

// ---------------------------------------------------------------------------------------------
// Sound loading

static void ResampleToStereo(const float* theIn, int theInFrames, int theInChannels, int theInRate, int theOutRate, SoundData& theOut)
{
	if (theInFrames <= 0)
		return;
	double ratio = (double)theInRate / theOutRate;
	size_t outFrames = (size_t)std::ceil(theInFrames / ratio);
	theOut.mSamples.resize(outFrames * 2);
	theOut.mFrames = outFrames;
	for (size_t i = 0; i < outFrames; i++)
	{
		double src = i * ratio;
		int i0 = (int)src;
		int i1 = std::min(i0 + 1, theInFrames - 1);
		float t = (float)(src - i0);
		for (int c = 0; c < 2; c++)
		{
			int ch = std::min(c, theInChannels - 1);
			float a = theIn[i0 * theInChannels + ch];
			float b = theIn[i1 * theInChannels + ch];
			theOut.mSamples[i * 2 + c] = a + (b - a) * t;
		}
	}
}

std::shared_ptr<SoundData> Mixer::LoadSound(const std::string& theGamePath)
{
	std::vector<uint8_t> file;
	if (!VFS::Get().ReadAll(theGamePath, file) || file.empty())
		return nullptr;
	return LoadSoundData(file, theGamePath);
}

std::shared_ptr<SoundData> Mixer::LoadSoundData(const std::vector<uint8_t>& file, const std::string& theName)
{
	auto data = std::make_shared<SoundData>();
	// Detect Ogg by content: mods may remap a game path to a file with another name.
	if (file.size() > 4 && !memcmp(file.data(), "OggS", 4))
	{
		int channels = 0, rate = 0;
		short* pcm = nullptr;
		int frames = stb_vorbis_decode_memory(file.data(), (int)file.size(), &channels, &rate, &pcm);
		if (frames <= 0 || !pcm)
			return nullptr;
		std::vector<float> f((size_t)frames * channels);
		for (size_t i = 0; i < f.size(); i++)
			f[i] = pcm[i] / 32768.0f;
		free(pcm);
		ResampleToStereo(f.data(), frames, channels, rate, mRate, *data);
		return data;
	}
	// WAV (and anything else SDL can read)
	SDL_RWops* rw = SDL_RWFromConstMem(file.data(), (int)file.size());
	SDL_AudioSpec spec;
	Uint8* buf = nullptr;
	Uint32 len = 0;
	if (!SDL_LoadWAV_RW(rw, 1, &spec, &buf, &len))
	{
		SDL_Log("cannot decode sound %s (supported: Ogg Vorbis, WAV)", theName.c_str());
		return nullptr;
	}
	SDL_AudioCVT cvt;
	if (SDL_BuildAudioCVT(&cvt, spec.format, spec.channels, spec.freq, AUDIO_F32SYS, spec.channels, spec.freq) < 0)
	{
		SDL_FreeWAV(buf);
		return nullptr;
	}
	std::vector<uint8_t> conv((size_t)len * cvt.len_mult);
	memcpy(conv.data(), buf, len);
	SDL_FreeWAV(buf);
	cvt.buf = conv.data();
	cvt.len = (int)len;
	SDL_ConvertAudio(&cvt);
	int frames = cvt.len_cvt / (int)(sizeof(float) * spec.channels);
	ResampleToStereo((const float*)conv.data(), frames, spec.channels, spec.freq, mRate, *data);
	return data;
}

// ---------------------------------------------------------------------------------------------
// Voices

int Mixer::StartVoice(const std::shared_ptr<SoundData>& theData, float theGainL, float theGainR, double thePitch, bool theLoop)
{
	if (!mDevice || !theData || theData->mFrames == 0)
		return 0;
	Lock();
	int slot = -1;
	for (int i = 0; i < (int)mVoices.size(); i++)
	{
		if (!mVoices[i]->mActive)
		{
			slot = i;
			break;
		}
	}
	int handle = 0;
	if (slot >= 0)
	{
		Voice& v = *mVoices[slot];
		v.mData = theData;
		v.mPos = 0.0;
		v.mStep = thePitch;
		v.mGainL = theGainL;
		v.mGainR = theGainR;
		v.mLoop = theLoop;
		v.mActive = true;
		v.mHandle = handle = (mNextVoiceSerial++ << kVoiceSerialShift) | slot;
		if (mNextVoiceSerial > 0x7fffff)
			mNextVoiceSerial = 1;
	}
	Unlock();
	return handle;
}

#define VOICE_FOR_HANDLE(h) \
	int slot_ = (h) & ((1 << kVoiceSerialShift) - 1); \
	Voice* v = (h) > 0 && slot_ < (int)mVoices.size() && mVoices[slot_]->mHandle == (h) ? mVoices[slot_].get() : nullptr

void Mixer::SetVoiceGain(int theVoice, float theGainL, float theGainR)
{
	Lock();
	VOICE_FOR_HANDLE(theVoice);
	if (v)
	{
		v->mGainL = theGainL;
		v->mGainR = theGainR;
	}
	Unlock();
}

void Mixer::SetVoicePitch(int theVoice, double thePitch)
{
	Lock();
	VOICE_FOR_HANDLE(theVoice);
	if (v)
		v->mStep = thePitch;
	Unlock();
}

void Mixer::StopVoice(int theVoice)
{
	Lock();
	VOICE_FOR_HANDLE(theVoice);
	if (v)
	{
		v->mActive = false;
		v->mData.reset();
	}
	Unlock();
}

bool Mixer::IsVoicePlaying(int theVoice)
{
	Lock();
	VOICE_FOR_HANDLE(theVoice);
	bool playing = v && v->mActive;
	Unlock();
	return playing;
}

void Mixer::StopAllVoices()
{
	Lock();
	for (auto& v : mVoices)
	{
		v->mActive = false;
		v->mData.reset();
	}
	Unlock();
}

// ---------------------------------------------------------------------------------------------
// Songs

int Mixer::LoadSong(const std::string& theGamePath)
{
	std::vector<uint8_t> file;
	if (!VFS::Get().ReadAll(theGamePath, file) || file.empty())
		return 0;
	std::unique_ptr<Song> song(new Song());
	const VFSRemap* remap = VFS::Get().GetRemap(theGamePath);
	if (remap)
	{
		song->mDefaultOrder = remap->mStartOrder;
		song->mTrackGain = (float)remap->mVolume;
	}
	bool isOgg = file.size() > 4 && !memcmp(file.data(), "OggS", 4);
	bool isWav = file.size() > 4 && !memcmp(file.data(), "RIFF", 4);
	if (isOgg)
	{
		// Mods may replace tracker music with rendered audio; it is decoded while it plays.
		song->mFile.swap(file);
		int err = 0;
		song->mVorbis = stb_vorbis_open_memory(song->mFile.data(), (int)song->mFile.size(), &err, nullptr);
		if (!song->mVorbis)
		{
			SDL_Log("cannot open Ogg music %s (error %d)", theGamePath.c_str(), err);
			return 0;
		}
		stb_vorbis_info info = stb_vorbis_get_info(song->mVorbis);
		song->mSourceRate = (int)info.sample_rate;
		song->mSourceChannels = info.channels;
		song->mSourceFrames = stb_vorbis_stream_length_in_samples(song->mVorbis);
	}
	else if (isWav)
	{
		song->mPcm = LoadSoundData(file, theGamePath);
		if (!song->mPcm)
			return 0;
		song->mSourceRate = mRate;
		song->mSourceChannels = 2;
		song->mSourceFrames = (int64_t)song->mPcm->mFrames;
	}
	if (song->IsStream())
	{
		if (remap)
		{
			song->mStartFrame = (int64_t)std::max(0.0, remap->mStartTime * song->mSourceRate);
			song->mLoopStartFrame = (int64_t)std::max(0.0, remap->mLoopStart * song->mSourceRate);
		}
		if (song->mSourceFrames > 0 && song->mLoopStartFrame >= song->mSourceFrames)
			song->mLoopStartFrame = 0;
		if (song->mSourceFrames > 0 && song->mStartFrame >= song->mSourceFrames)
			song->mStartFrame = 0;
	}
	else
	{
		song->mCtx = xmp_create_context();
		if (xmp_load_module_from_memory(song->mCtx, file.data(), (long)file.size()) != 0)
		{
			xmp_free_context(song->mCtx);
			return 0;
		}
		xmp_start_player(song->mCtx, mRate, 0);
		xmp_set_player(song->mCtx, XMP_PLAYER_INTERP, XMP_INTERP_SPLINE);
	}
	song->mLoaded = true;
	Lock();
	mSongs.push_back(std::move(song));
	int handle = (int)mSongs.size();
	Unlock();
	return handle;
}

void Mixer::UnloadSong(int theSong)
{
	if (theSong <= 0 || theSong > (int)mSongs.size() || !mSongs[theSong - 1])
		return;
	Lock();
	std::unique_ptr<Song> s = std::move(mSongs[theSong - 1]);
	Unlock();
	if (s->mCtx)
	{
		xmp_end_player(s->mCtx);
		xmp_release_module(s->mCtx);
		xmp_free_context(s->mCtx);
	}
	if (s->mVorbis)
		stb_vorbis_close(s->mVorbis);
}

#define SONG_FOR_HANDLE(h) \
	Song* s = (h) > 0 && (h) <= (int)mSongs.size() ? mSongs[(h) - 1].get() : nullptr

void Mixer::PlaySong(int theSong, int theOrder, int theRow, bool theLoop, bool theResetPosition)
{
	Lock();
	SONG_FOR_HANDLE(theSong);
	if (s)
	{
		s->mLoop = theLoop;
		if (s->IsStream())
		{
			// streams ignore the module order; a mod remap's "start" picks the entry point
			if (theResetPosition || s->mEnded || !s->mPlaying)
				s->Restart(theResetPosition || s->mEnded ? s->mStartFrame : s->mPcmPos);
		}
		else if (theResetPosition)
		{
			// A mod remap can start a track partway into a multi-song module; the game's own offset
			// is relative to that.
			int order = theOrder + std::max(0, s->mDefaultOrder);
			xmp_restart_module(s->mCtx);
			if (order > 0)
				xmp_set_position(s->mCtx, order);
			if (theRow > 0)
				xmp_set_row(s->mCtx, theRow);
		}
		s->mLoop = theLoop;
		s->mPlaying = true;
		s->mPaused = false;
		s->mCurGain = s->mGain;
	}
	Unlock();
}

void Mixer::StopSong(int theSong)
{
	Lock();
	SONG_FOR_HANDLE(theSong);
	if (s)
		s->mPlaying = false;
	Unlock();
}

void Mixer::PauseSong(int theSong, bool thePaused)
{
	Lock();
	SONG_FOR_HANDLE(theSong);
	if (s)
		s->mPaused = thePaused;
	Unlock();
}

void Mixer::SetSongGain(int theSong, float theGain)
{
	Lock();
	SONG_FOR_HANDLE(theSong);
	if (s)
		s->mGain = theGain;
	Unlock();
}

bool Mixer::IsSongPlaying(int theSong)
{
	Lock();
	SONG_FOR_HANDLE(theSong);
	bool playing = s && s->mPlaying && !s->mPaused;
	Unlock();
	return playing;
}

// ---------------------------------------------------------------------------------------------
// Mixing

void Mixer::Callback(void* theUser, uint8_t* theStream, int theLen)
{
	Mixer* m = (Mixer*)theUser;
	m->Mix((float*)theStream, theLen / (int)(sizeof(float) * 2));
}

void Mixer::Mix(float* theOut, int theFrames)
{
	memset(theOut, 0, sizeof(float) * 2 * theFrames);

	for (auto& vp : mVoices)
	{
		Voice& v = *vp;
		if (!v.mActive || !v.mData)
			continue;
		const float* src = v.mData->mSamples.data();
		const double len = (double)v.mData->mFrames;
		const float gl = v.mGainL * mSfxMaster, gr = v.mGainR * mSfxMaster;
		for (int i = 0; i < theFrames; i++)
		{
			if (v.mPos >= len)
			{
				if (v.mLoop)
					v.mPos = std::fmod(v.mPos, len);
				else
				{
					v.mActive = false;
					break;
				}
			}
			int i0 = (int)v.mPos;
			int i1 = i0 + 1 < (int)len ? i0 + 1 : (v.mLoop ? 0 : i0);
			float t = (float)(v.mPos - i0);
			float l = src[i0 * 2] + (src[i1 * 2] - src[i0 * 2]) * t;
			float r = src[i0 * 2 + 1] + (src[i1 * 2 + 1] - src[i0 * 2 + 1]) * t;
			theOut[i * 2] += l * gl;
			theOut[i * 2 + 1] += r * gr;
			v.mPos += v.mStep;
		}
		if (!v.mActive)
			v.mData.reset();
	}

	mSongScratch.resize((size_t)theFrames * 2);
	for (auto& sp : mSongs)
	{
		if (!sp)
			continue;
		Song& s = *sp;
		if (!s.mPlaying || s.mPaused)
			continue;
		// ramp gain across the buffer so per-tick fades don't click
		float g0 = s.mCurGain * mMusicMaster * s.mTrackGain, g1 = s.mGain * mMusicMaster * s.mTrackGain;
		if (s.IsStream())
		{
			const double step = (double)s.mSourceRate / mRate;
			for (int i = 0; i < theFrames; i++)
			{
				float g = g0 + (g1 - g0) * ((float)i / theFrames);
				float t = (float)s.mFrac;
				theOut[i * 2] += (s.mFrameA[0] + (s.mFrameB[0] - s.mFrameA[0]) * t) * g;
				theOut[i * 2 + 1] += (s.mFrameA[1] + (s.mFrameB[1] - s.mFrameA[1]) * t) * g;
				s.mFrac += step;
				while (s.mFrac >= 1.0)
				{
					s.mFrac -= 1.0;
					s.mFrameA[0] = s.mFrameB[0];
					s.mFrameA[1] = s.mFrameB[1];
					if (!s.ReadSourceFrame(s.mFrameB))
						s.mFrameB[0] = s.mFrameB[1] = 0;
				}
				if (s.mEnded && s.mFrameA[0] == 0 && s.mFrameA[1] == 0)
				{
					s.mPlaying = false;
					break;
				}
			}
			s.mCurGain = s.mGain;
			continue;
		}
		int ret = xmp_play_buffer(s.mCtx, mSongScratch.data(), theFrames * 4, s.mLoop ? 0 : 1);
		if (ret != 0)
		{
			s.mPlaying = false;
			continue;
		}
		for (int i = 0; i < theFrames; i++)
		{
			float g = g0 + (g1 - g0) * ((float)i / theFrames);
			theOut[i * 2] += mSongScratch[i * 2] * (g / 32768.0f);
			theOut[i * 2 + 1] += mSongScratch[i * 2 + 1] * (g / 32768.0f);
		}
		s.mCurGain = s.mGain;
	}

	for (int i = 0; i < theFrames * 2; i++)
		theOut[i] = std::max(-1.0f, std::min(1.0f, theOut[i]));
}

}
